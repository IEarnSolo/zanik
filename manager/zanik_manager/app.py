from __future__ import annotations

import json
import os
import sys
import traceback
from pathlib import Path

from PySide6.QtCore import QObject, QThread, QTimer, Qt, Signal, Slot
from PySide6.QtGui import QDesktopServices
from PySide6.QtWidgets import (QApplication, QCheckBox, QComboBox, QDialog, QDialogButtonBox, QFileDialog,
                               QFormLayout, QHBoxLayout, QLabel, QLineEdit, QMainWindow, QMessageBox,
                               QPlainTextEdit, QProgressBar, QProgressDialog, QPushButton, QScrollArea,
                               QSpinBox, QTabWidget, QVBoxLayout, QWidget)
from PySide6.QtCore import QUrl

from .assets import AssetExtractor, discover_steam_game_roots, dragonwilds_is_running, game_root_id
from .cache import BundleCache
from .config import ConfigModel, content_hash
from .journal import JournalStore
from .lifecycle import PterodactylProvider, ReleasePayload
from .onboarding import PanelDiscovery, discover_panel, is_remote_http
from .paths import ensure_layout
from .profiles import ProfileStore, ServerProfile
from .pterodactyl import PterodactylClient
from .security import CredentialVault, credential_target, redact
from .support import create_support_bundle


def distribution_root() -> Path:
    return Path(__file__).resolve().parents[2]


def resource_path(*parts: str) -> Path:
    root = distribution_root()
    candidate = root.joinpath(*parts)
    if candidate.exists():
        return candidate
    repo = Path(__file__).resolve().parents[2]
    return repo.joinpath(*parts)


class Worker(QObject):
    done = Signal()

    def __init__(self, function):
        super().__init__(); self.function = function
        self.result = None; self.error = None

    @Slot()
    def run(self):
        try:
            self.result = self.function()
        except Exception as error:
            self.error = (str(error), traceback.format_exc())
        finally:
            self.done.emit()


class AddServerDialog(QDialog):
    def __init__(self, vault: CredentialVault, parent=None):
        super().__init__(parent); self.vault = vault; self.profile = None; self.discovery: PanelDiscovery | None = None
        self.setWindowTitle("Add a Pterodactyl server")
        layout = QFormLayout(self)
        self.origin = QLineEdit(); self.origin.setPlaceholderText("https://panel.example.com")
        self.token = QLineEdit(); self.token.setEchoMode(QLineEdit.Password)
        self.name = QLineEdit(); self.name.setPlaceholderText("My Dragonwilds server")
        self.server = QComboBox(); self.server.setEnabled(False)
        self.test = QPushButton("Connect and list servers"); self.test.clicked.connect(self.connect_panel)
        self.buttons = QDialogButtonBox(QDialogButtonBox.Save | QDialogButtonBox.Cancel)
        self.buttons.button(QDialogButtonBox.Save).setEnabled(False)
        self.buttons.accepted.connect(self.accept_profile); self.buttons.rejected.connect(self.reject)
        layout.addRow("Panel URL", self.origin); layout.addRow("Client API key", self.token)
        layout.addRow("Profile name", self.name); layout.addRow(self.test)
        layout.addRow("Server", self.server); layout.addRow(self.buttons)
        self.origin.textChanged.connect(self.invalidate_connection)
        self.token.textChanged.connect(self.invalidate_connection)

    def invalidate_connection(self):
        self.discovery = None
        self.server.clear(); self.server.setEnabled(False)
        self.buttons.button(QDialogButtonBox.Save).setEnabled(False)

    def confirm_remote_http(self) -> bool:
        box = QMessageBox(self); box.setIcon(QMessageBox.Warning)
        box.setWindowTitle("Unencrypted panel connection")
        box.setText("This Pterodactyl panel uses HTTP, so its API traffic is not encrypted.")
        box.setInformativeText("Your API key could be intercepted in transit. Continue only if you trust this network and understand the risk.")
        proceed = box.addButton("Continue with HTTP", QMessageBox.AcceptRole)
        cancel = box.addButton("Cancel", QMessageBox.RejectRole)
        box.setDefaultButton(cancel); box.setEscapeButton(cancel)
        box.exec()
        return box.clickedButton() is proceed

    def connect_panel(self):
        self.invalidate_connection()
        try:
            discovery = discover_panel(self.origin.text(), self.token.text(), self.confirm_remote_http)
            if discovery is None:
                return
            self.discovery = discovery
            for server in discovery.servers:
                self.server.addItem(server.get("name", server.get("identifier", "Server")), server)
            self.server.setEnabled(bool(discovery.servers)); self.buttons.button(QDialogButtonBox.Save).setEnabled(bool(discovery.servers))
        except Exception as error:
            QMessageBox.critical(self, "Connection failed", str(error))

    def accept_profile(self):
        if self.discovery is None or not self.server.currentData():
            QMessageBox.critical(self, "Reconnect required", "Connect with the current panel URL and API key before saving.")
            return
        server = self.server.currentData(); origin = self.discovery.origin
        account = self.discovery.account
        account_id = str(account.get("id") or account.get("uuid") or account.get("email") or "account")
        target = credential_target(origin, account_id); self.vault.write(target, self.token.text(), account_id)
        self.profile = ServerProfile.create(name=self.name.text().strip() or server.get("name", "Dragonwilds server"),
                                            panel_origin=origin, account_id=account_id, credential_target=target,
                                            server_id=server.get("identifier", ""), server_uuid=server.get("uuid", ""),
                                            allow_insecure_http=self.discovery.allow_insecure_http)
        self.accept()


class MainWindow(QMainWindow):
    activity_signal = Signal(str)
    operation_progress_signal = Signal(str, int)

    def __init__(self, prompt_first_run: bool = True):
        super().__init__(); self.setWindowTitle("Zanik Manager"); self.resize(980, 680)
        self.paths = ensure_layout(); self.store = ProfileStore(self.paths["root"] / "profiles.json")
        self.vault = CredentialVault(); self.profiles = self.store.load(); self.current_config_hash = ""
        self.threads = []; self.busy_dialog = None; self.last_progress_message = ""
        central = QWidget(); outer = QVBoxLayout(central)
        target = QHBoxLayout(); self.profile_box = QComboBox(); self.add_button = QPushButton("Add server")
        self.add_button.clicked.connect(self.add_server); self.profile_box.currentIndexChanged.connect(self.refresh_header)
        target.addWidget(QLabel("Active server:")); target.addWidget(self.profile_box, 1); target.addWidget(self.add_button)
        self.warning = QLabel(); self.warning.setStyleSheet("color:#b45309;font-weight:600")
        outer.addLayout(target); outer.addWidget(self.warning)
        self.tabs = QTabWidget(); outer.addWidget(self.tabs, 1); self.setCentralWidget(central)
        self._servers_tab(); self._assets_tab(); self._config_tab(); self._doctor_tab(); self._activity_tab()
        self.activity_signal.connect(self.log)
        self.operation_progress_signal.connect(self.update_operation_progress)
        self.reload_profiles()
        if prompt_first_run and not self.profiles:
            self.add_server()

    def _servers_tab(self):
        page = QWidget(); layout = QVBoxLayout(page)
        self.server_summary = QLabel("Add a server to begin."); self.server_summary.setWordWrap(True)
        row = QHBoxLayout()
        for label, handler in (("Refresh", self.run_doctor), ("Install / repair", self.install),
                               ("Update", self.install), ("Recover interrupted", self.recover_interrupted),
                               ("Uninstall", self.uninstall), ("Open panel", self.open_panel)):
            button = QPushButton(label); button.clicked.connect(handler); row.addWidget(button)
        layout.addWidget(self.server_summary); layout.addLayout(row); layout.addStretch(); self.tabs.addTab(page, "Servers")

    def _assets_tab(self):
        page = QWidget(); layout = QFormLayout(page)
        self.game_root = QLineEdit(); roots = discover_steam_game_roots()
        if roots: self.game_root.setText(str(roots[0]))
        browse = QPushButton("Browse…"); browse.clicked.connect(self.choose_game_root)
        row = QHBoxLayout(); row.addWidget(self.game_root); row.addWidget(browse)
        self.bundle_box = QComboBox(); self.extract_button = QPushButton("Extract and validate assets")
        self.extract_button.clicked.connect(self.extract_assets)
        upload = QPushButton("Upload and activate selected bundle"); upload.clicked.connect(self.upload_assets)
        open_button = QPushButton("Open bundle folder"); open_button.clicked.connect(self.open_asset_folder)
        layout.addRow("Dragonwilds client", row); layout.addRow(self.extract_button)
        layout.addRow("Validated bundle", self.bundle_box); layout.addRow(upload); layout.addRow(open_button)
        self.tabs.addTab(page, "Assets")

    def _config_tab(self):
        page = QWidget(); layout = QVBoxLayout(page)
        self.config_editors=QTabWidget(); form_page=QWidget(); form=QFormLayout(form_page); self.config_fields={}
        model=self.config_model(); current_category=""
        for key,spec in model.properties.items():
            if spec["category"] != current_category:
                current_category=spec["category"]; heading=QLabel(current_category.replace("-"," ").title()); heading.setStyleSheet("font-weight:700;margin-top:8px")
                form.addRow(heading)
            if spec["type"]=="boolean":
                field=QCheckBox(spec["description"]); field.setChecked(bool(spec["default"]))
                if spec.get("dangerous"): field.setStyleSheet("color:#b45309")
            elif spec["type"]=="integer":
                field=QSpinBox(); field.setRange(spec.get("minimum",0),spec.get("maximum",2147483647)); field.setValue(spec["default"]); field.setToolTip(spec["description"])
            else:
                field=QLineEdit(str(spec["default"])); field.setToolTip(spec["description"])
            self.config_fields[key]=field; form.addRow(spec["title"],field)
        scroll=QScrollArea(); scroll.setWidgetResizable(True); scroll.setWidget(form_page)
        self.config_text = QPlainTextEdit(); self.config_text.setPlaceholderText("Load the server configuration to edit it.")
        self.config_editors.addTab(scroll,"Guided settings"); self.config_editors.addTab(self.config_text,"Advanced raw editor")
        row = QHBoxLayout(); load = QPushButton("Load from server"); save = QPushButton("Validate and save")
        load.clicked.connect(self.load_config); save.clicked.connect(self.save_config)
        row.addWidget(load); row.addWidget(save); row.addStretch()
        layout.addWidget(QLabel("Mutation and QoL features are off by default.")); layout.addWidget(self.config_editors, 1); layout.addLayout(row)
        self.tabs.addTab(page, "Configuration")

    def _doctor_tab(self):
        page = QWidget(); layout = QVBoxLayout(page); row = QHBoxLayout()
        quick = QPushButton("Run quick Doctor"); deep = QPushButton("Run deep Doctor"); support = QPushButton("Create support ZIP")
        quick.clicked.connect(self.run_doctor); deep.clicked.connect(lambda: self.run_doctor(True)); support.clicked.connect(self.export_support)
        row.addWidget(quick); row.addWidget(deep); row.addWidget(support); row.addStretch()
        self.doctor_output = QPlainTextEdit(); self.doctor_output.setReadOnly(True)
        layout.addLayout(row); layout.addWidget(self.doctor_output, 1); self.tabs.addTab(page, "Doctor")

    def _activity_tab(self):
        page = QWidget(); layout = QVBoxLayout(page); self.activity = QPlainTextEdit(); self.activity.setReadOnly(True)
        self.progress = QProgressBar(); self.progress.setRange(0, 100)
        layout.addWidget(self.activity, 1); layout.addWidget(self.progress); self.tabs.addTab(page, "Activity")

    def log(self, message: str):
        self.activity.appendPlainText(redact(message)); self.statusBar().showMessage(message, 5000)

    def begin_operation(self, title: str, determinate: bool) -> None:
        self.last_progress_message = ""
        self.progress.setRange(0, 100); self.progress.setValue(0)
        dialog = QProgressDialog(title, "", 0, 100 if determinate else 0, self)
        dialog.setWindowTitle(title); dialog.setCancelButton(None)
        dialog.setWindowModality(Qt.ApplicationModal); dialog.setMinimumDuration(0)
        dialog.setAutoClose(False); dialog.setAutoReset(False); dialog.setValue(0)
        dialog.show()
        self.busy_dialog = dialog
        self.log(title)

    def update_operation_progress(self, message: str, percent: int = -1) -> None:
        safe_message = redact(message)
        if self.busy_dialog:
            self.busy_dialog.setLabelText(safe_message)
            if percent >= 0:
                if self.busy_dialog.maximum() == 0:
                    self.busy_dialog.setRange(0, 100)
                value = max(0, min(100, percent))
                self.busy_dialog.setValue(value); self.progress.setValue(value)
        if safe_message and safe_message != self.last_progress_message:
            self.last_progress_message = safe_message
            self.log(safe_message)

    def finish_operation(self) -> None:
        if self.busy_dialog:
            self.busy_dialog.close(); self.busy_dialog.deleteLater(); self.busy_dialog = None
        self.progress.setRange(0, 100)

    def run_async(self, function, completed=lambda result: None, *, title="Working…", determinate=False):
        if self.threads:
            QMessageBox.information(self, "Operation in progress",
                                    "Wait for the current operation to finish before starting another one.")
            return False
        self.begin_operation(title, determinate)
        thread = QThread(self); worker = Worker(function); worker.moveToThread(thread)
        thread.started.connect(worker.run)
        worker.done.connect(thread.quit)
        thread.finished.connect(lambda: self.operation_thread_finished(thread, worker, completed))
        self.threads.append((thread, worker)); thread.start(); return True

    def operation_thread_finished(self, thread, worker, completed) -> None:
        self.threads.remove((thread, worker))
        if worker.error:
            self.operation_failed(*worker.error)
            return
        self.progress.setValue(100); self.finish_operation(); completed(worker.result)

    def operation_failed(self, message: str, details: str):
        self.finish_operation()
        safe_message = redact(message); safe_details = redact(details)
        QMessageBox.critical(self, "Operation failed", safe_message + "\n\nTechnical details are available in Activity.")
        self.activity.appendPlainText("Operation failed: " + safe_message + "\n" + safe_details)
        self.statusBar().showMessage("Operation failed: " + safe_message, 5000)

    def active(self) -> ServerProfile | None:
        return self.profile_box.currentData()

    def provider(self) -> PterodactylProvider:
        profile = self.active()
        if not profile: raise RuntimeError("Add and select a server first")
        token = self.vault.read(profile.credential_target)
        if not token: raise RuntimeError("The API key is missing from Windows Credential Manager")
        status = lambda message: self.operation_progress_signal.emit(message, -1)
        return PterodactylProvider(PterodactylClient(profile.panel_origin, token, status=status),
                                   JournalStore(self.paths["journals"]))

    def reload_profiles(self):
        self.profile_box.clear()
        for profile in self.profiles: self.profile_box.addItem(profile.name, profile)
        self.refresh_header(); self.refresh_bundles()

    def refresh_header(self):
        profile = self.active()
        insecure = bool(profile and profile.allow_insecure_http and is_remote_http(profile.panel_origin))
        self.warning.setText("⚠ This panel connection uses HTTP; API traffic is not encrypted." if insecure else "")
        if profile: self.server_summary.setText(f"{profile.name}\n{profile.panel_origin} · {profile.server_id}\nInstall mode: {profile.install_mode}")
        if profile:
            incomplete=JournalStore(self.paths["journals"]).incomplete(profile.id)
            if incomplete:
                self.warning.setText((self.warning.text()+"\n" if self.warning.text() else "")+f"An interrupted {incomplete[-1].action} operation needs attention.")

    def add_server(self):
        dialog = AddServerDialog(self.vault, self)
        if dialog.exec() == QDialog.Accepted and dialog.profile:
            self.profiles.append(dialog.profile); self.store.save(self.profiles); self.reload_profiles()

    def recover_interrupted(self):
        profile=self.active()
        if not profile: return
        store=JournalStore(self.paths["journals"]); incomplete=store.incomplete(profile.id)
        if not incomplete: QMessageBox.information(self,"Recovery","No interrupted operation was found."); return
        journal=incomplete[-1]
        box=QMessageBox(self); box.setWindowTitle("Interrupted operation")
        box.setText(f"Recover interrupted {journal.action} operation {journal.operation_id}?")
        box.setInformativeText("Retry first restores the last safe state and then reopens the workflow. Rollback only restores it.")
        box.setStandardButtons(QMessageBox.Retry|QMessageBox.Reset|QMessageBox.Cancel)
        box.button(QMessageBox.Reset).setText("Rollback only")
        choice=box.exec()
        if choice in {QMessageBox.Retry,QMessageBox.Reset}:
            retry=choice==QMessageBox.Retry
            self.run_async(lambda:self.provider().rollback(journal,profile.server_id,profile.server_root),
                           lambda _: (self.log("Rollback completed."),self.refresh_header(),self.install() if retry and journal.action=="install" else None),
                           title="Recovering the interrupted operation…")

    def open_panel(self):
        if self.active(): QDesktopServices.openUrl(QUrl(self.active().panel_origin + "/server/" + self.active().server_id))

    def install(self):
        profile = self.active()
        if not profile: return
        try:
            adapter = self.provider().client.wrapper_adapter(profile.server_id)
        except Exception as error:
            QMessageBox.critical(self, "Startup inspection failed", str(error)); return
        dialog = QDialog(self); dialog.setWindowTitle("Choose installation mode"); form = QFormLayout(dialog)
        choices = QComboBox(); choices.addItem("Wrapper — recommended", "wrapper"); choices.addItem("Replace executable — compatibility", "replace-executable")
        if not adapter: choices.setCurrentIndex(1)
        note = QLabel("This egg has a supported editable startup variable; wrapper mode can be configured automatically." if adapter else
                      "This egg has no recognized wrapper variable. Wrapper mode will install files and show the exact manual startup command.")
        note.setWordWrap(True); buttons = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel)
        buttons.accepted.connect(dialog.accept); buttons.rejected.connect(dialog.reject)
        form.addRow(note); form.addRow("Mode", choices); form.addRow(buttons)
        if dialog.exec() != QDialog.Accepted: return
        selected = choices.currentData()
        if selected == "replace-executable" and QMessageBox.warning(self, "Core-file replacement",
            "This compatibility mode replaces the executable path while preserving the official binary. Continue?",
            QMessageBox.Yes | QMessageBox.No) != QMessageBox.Yes: return
        archive = resource_path("payload", "server.zip")
        if not archive.exists():
            matches = sorted((resource_path("dist", "release")).glob("zanik-server-*.zip"))
            if not matches: QMessageBox.critical(self, "Missing payload", "The matching server payload is missing."); return
            archive = matches[-1]
        def progress(message, percent): self.operation_progress_signal.emit(message, percent)
        def action(): return self.provider().install(profile.id, profile.server_id, profile.server_root,
                                                     ReleasePayload(archive), selected, selected == "replace-executable",
                                                     wrapper_adapter=adapter if selected == "wrapper" else None,
                                                     progress=progress)
        def done(result):
            profile.install_mode = selected; self.store.save(self.profiles)
            if selected == "wrapper" and not adapter:
                self.log(f"Files installed; startup action remains. Backup: {result['backup']}")
                QMessageBox.information(self, "Startup action required", f"The server remains stopped and installation is not yet active. Set the egg startup executable to:\n{profile.server_root}/zanik/bin/zanik-server --server-root {profile.server_root} --\n\nThen start the server and run Doctor to verify it.")
            else:
                self.log(f"Installation complete; backup: {result['backup']}")
                asset_note = ("\n\nNo local asset bundle is ready yet. Open the Assets tab to extract, "
                              "upload, and activate the website assets." if self.bundle_box.count() == 0 else "")
                QMessageBox.information(self, "Installation complete",
                                        "Zanik was installed successfully and the server was started." + asset_note)
        self.run_async(action, done, title="Installing Zanik…", determinate=True)

    def uninstall(self):
        profile = self.active()
        if not profile or QMessageBox.warning(self, "Uninstall Zanik", "Uninstall Zanik from the selected server? Persistent data will be kept.",
                                               QMessageBox.Yes | QMessageBox.No) != QMessageBox.Yes: return
        self.run_async(lambda: self.provider().uninstall(profile.id, profile.server_id, profile.server_root),
                       lambda result: self.log(f"Uninstalled; recovery backup: {result['backup']}"),
                       title="Uninstalling Zanik…")

    def run_doctor(self, deep=False):
        profile = self.active()
        if not profile: return
        self.run_async(lambda: self.provider().diagnose(profile.server_id, profile.server_root, deep),
                       lambda result: (self.doctor_output.setPlainText(json.dumps(result, indent=2, sort_keys=True)),
                                       self.server_summary.setText("Healthy" if result.get("healthy") else "Doctor found problems")),
                       title="Running deep Doctor…" if deep else "Running Doctor…")

    def choose_game_root(self):
        selected = QFileDialog.getExistingDirectory(self, "Select the RSDragonwilds installation", self.game_root.text())
        if selected: self.game_root.setText(selected); self.refresh_bundles()

    def refresh_bundles(self):
        self.bundle_box.clear(); root = Path(self.game_root.text()) if self.game_root.text() else None
        if not root: return
        for item in BundleCache(self.paths["assets"]).list(game_root_id(root)):
            self.bundle_box.addItem(f"Build {item.build} · {item.manifest_hash[:12]}", str(item.path))

    def extract_assets(self):
        root = Path(self.game_root.text())
        if not root.is_dir(): QMessageBox.critical(self, "Game not found", "Select a valid Dragonwilds client directory."); return
        if dragonwilds_is_running():
            QMessageBox.warning(
                self, "Close Dragonwilds first",
                "Dragonwilds is currently running. Close it before starting extraction, then try again.\n\n"
                "Zanik Manager will launch and close the game automatically while it generates mappings. "
                "Do not relaunch or interact with the game until extraction finishes."
            )
            return
        output = self.paths["root"] / "extraction-output"
        tool_root = resource_path("asset-tools")
        phase_percent = {"mappings": 10, "extract": 40, "bundle": 80, "complete": 100}
        def event(value):
            message = value.get("message", str(value)); phase = value.get("phase", "extract")
            self.operation_progress_signal.emit(message, phase_percent.get(phase, -1))
        self.run_async(lambda: AssetExtractor(tool_root, BundleCache(self.paths["assets"])).extract(root, output, event),
                       lambda item: (self.refresh_bundles(), self.log(f"Validated asset bundle: {item.path}"),
                                     QMessageBox.information(self, "Asset extraction complete",
                                                             f"The validated asset bundle is ready at:\n{item.path}")),
                       title="Extracting Dragonwilds web assets…", determinate=True)

    def upload_assets(self):
        profile = self.active(); value = self.bundle_box.currentData()
        if not profile or not value: return
        from .assets import AssetDeployer
        def progress(current, total, path):
            percent = 15 + int(current * 75 / max(total, 1))
            self.operation_progress_signal.emit(f"Uploading and verifying {path}", percent)
        self.run_async(lambda: AssetDeployer(self.provider()).activate(profile.id, profile.server_id, profile.server_root,
                                                                       Path(value), progress),
                       lambda result: (self.log(f"Activated {result['fileCount']} asset files"),
                                       QMessageBox.information(self, "Assets activated",
                                                               f"Activated {result['fileCount']} validated asset files.")),
                       title="Uploading and activating web assets…", determinate=True)

    def open_asset_folder(self):
        value = self.bundle_box.currentData()
        path = Path(value).parent if value else self.paths["assets"]
        QDesktopServices.openUrl(QUrl.fromLocalFile(str(path)))

    def config_model(self):
        return ConfigModel(resource_path("config", "zanik-config.schema.json"))

    def load_config(self):
        profile = self.active()
        if not profile: return
        def action(): return self.provider().client.read_file(profile.server_id, f"{profile.server_root}/zanik/config/zanik.conf")
        def done(data):
            values=self.config_model().parse(data.decode()); self.current_config_hash = content_hash(data); self.config_text.setPlainText(data.decode())
            for key,value in values.items():
                field=self.config_fields[key]
                if isinstance(field,QCheckBox): field.setChecked(bool(value))
                elif isinstance(field,QSpinBox): field.setValue(int(value))
                else: field.setText(str(value))
        self.run_async(action, done, title="Loading server configuration…")

    def save_config(self):
        profile = self.active()
        if not profile or not self.current_config_hash: return
        try:
            model = self.config_model()
            if self.config_editors.currentIndex()==0:
                values={}
                for key,field in self.config_fields.items():
                    values[key]=field.isChecked() if isinstance(field,QCheckBox) else field.value() if isinstance(field,QSpinBox) else field.text()
                rendered=model.render(values); self.config_text.setPlainText(rendered)
            else:
                rendered=model.render(model.parse(self.config_text.toPlainText()))
            content = rendered.encode()
        except Exception as error:
            QMessageBox.critical(self, "Invalid configuration", str(error)); return
        def done(result):
            self.current_config_hash = result["sha256"]
            if QMessageBox.question(self, "Configuration saved", "Restart the server now to apply these settings?",
                                    QMessageBox.Yes | QMessageBox.No) == QMessageBox.Yes:
                self.provider().client.power(profile.server_id, "restart")
            self.log(f"Configuration saved; backup: {result['backup']}")
        self.run_async(lambda: self.provider().save_config(profile.server_id, profile.server_root, content, self.current_config_hash), done,
                       title="Saving server configuration…")

    def export_support(self):
        profile = self.active()
        if not profile: return
        output, _ = QFileDialog.getSaveFileName(self, "Save support bundle", "zanik-support.zip", "ZIP files (*.zip)")
        if not output: return
        diagnostics = json.loads(self.doctor_output.toPlainText() or "{}")
        safe_profiles = [{k: v for k, v in vars(item).items() if k != "credential_target"} for item in self.profiles]
        create_support_bundle(Path(output), diagnostics, safe_profiles, list(self.paths["logs"].glob("*.log")))
        self.log(f"Created support bundle: {output}")


def main() -> int:
    os.environ.setdefault("QT_ENABLE_HIGHDPI_SCALING", "1")
    smoke_test = "--smoke-test" in sys.argv
    application = QApplication(sys.argv); application.setApplicationName("Zanik Manager")
    window = MainWindow(prompt_first_run=not smoke_test); window.show()
    if smoke_test:
        QTimer.singleShot(250, application.quit)
    return application.exec()


if __name__ == "__main__":
    raise SystemExit(main())
