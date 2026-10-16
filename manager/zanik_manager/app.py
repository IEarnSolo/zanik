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

from .assets import (AssetDeployer, AssetExtractor, asset_setup_mode, discover_steam_game_roots,
                     expected_asset_contract, game_root_id, request_dragonwilds_close,
                     resume_activity_message)
from .cache import BundleCache, format_bundle_timestamp
from .config import ConfigModel, content_hash, update_website_settings, website_configuration_ready
from .journal import JournalStore
from .lifecycle import PterodactylProvider, ReleasePayload
from .onboarding import PanelDiscovery, discover_panel, is_remote_http
from .paths import ensure_layout
from .payload_contract import compatibility_summary
from .preferences import PreferenceStore, preferred_profile_index
from .profiles import ProfileStore, ServerProfile
from .pterodactyl import PterodactylClient
from .security import CredentialVault, credential_target, redact
from .support import create_support_bundle
from .updates import (GitHubReleaseChecker, current_manager_version, prune_updater_backups,
                      write_startup_handshake)


def distribution_root() -> Path:
    return Path(__file__).resolve().parents[2]


def resource_path(*parts: str) -> Path:
    root = distribution_root()
    candidate = root.joinpath(*parts)
    if candidate.exists():
        return candidate
    repo = Path(__file__).resolve().parents[2]
    return repo.joinpath(*parts)


def button_help(button: QPushButton, text: str) -> QPushButton:
    button.setToolTip(text)
    return button


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
        self.test = button_help(QPushButton("Connect and list servers"),
                                "Validate this panel URL and API key, then list the servers this account can manage.")
        self.test.clicked.connect(self.connect_panel)
        self.buttons = QDialogButtonBox(QDialogButtonBox.Save | QDialogButtonBox.Cancel)
        self.buttons.button(QDialogButtonBox.Save).setEnabled(False)
        self.buttons.button(QDialogButtonBox.Save).setToolTip("Save the selected server as a Manager profile after a successful connection test.")
        self.buttons.button(QDialogButtonBox.Cancel).setToolTip("Close this dialog without saving a server profile.")
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
        proceed.setToolTip("Send the API key over this unencrypted HTTP connection and continue only on a trusted network.")
        cancel.setToolTip("Cancel without sending the API key to the panel.")
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
        self.preference_store = PreferenceStore(self.paths["root"] / "preferences.json")
        self.preferences = self.preference_store.load()
        self.vault = CredentialVault(); self.profiles = self.store.load(); self.current_config_hash = ""
        self.current_config_profile_id = ""
        self.installation_states = {}
        self.threads = []; self.background_threads = []; self.busy_dialog = None; self.last_progress_message = ""
        self.manager_version = current_manager_version(distribution_root())
        self.release_checker = GitHubReleaseChecker(self.paths["updates"])
        self.validated_manager_update = None
        self.manager_release_status = None
        central = QWidget(); outer = QVBoxLayout(central)
        target = QHBoxLayout(); self.profile_box = QComboBox(); self.add_button = button_help(
            QPushButton("Add server"), "Add a Pterodactyl panel connection and select one server to manage.")
        self.add_button.clicked.connect(self.add_server); self.profile_box.currentIndexChanged.connect(self.active_profile_changed)
        target.addWidget(QLabel("Active server:")); target.addWidget(self.profile_box, 1); target.addWidget(self.add_button)
        self.warning = QLabel(); self.warning.setStyleSheet("color:#b45309;font-weight:600")
        outer.addLayout(target); outer.addWidget(self.warning)
        self.tabs = QTabWidget(); outer.addWidget(self.tabs, 1); self.setCentralWidget(central)
        self._servers_tab(); self._assets_tab(); self._config_tab(); self._doctor_tab(); self._activity_tab()
        self._update_status_bar()
        self.activity_signal.connect(self.log)
        self.operation_progress_signal.connect(self.update_operation_progress)
        self.reload_profiles()
        QTimer.singleShot(1000, self.retry_pending_asset_cleanup)
        QTimer.singleShot(1500, lambda: self.check_manager_update(False))
        self.update_timer = QTimer(self); self.update_timer.setInterval(10 * 60 * 1000)
        self.update_timer.timeout.connect(self.scheduled_update_checks); self.update_timer.start()
        if prompt_first_run and not self.profiles:
            self.add_server()

    def _servers_tab(self):
        page = QWidget(); layout = QVBoxLayout(page)
        self.server_summary = QLabel("Add a server to begin."); self.server_summary.setWordWrap(True)
        row = QHBoxLayout()
        refresh = button_help(QPushButton("Refresh"), "Run a quick, read-only health check and refresh the selected server summary.")
        refresh.clicked.connect(self.run_doctor); row.addWidget(refresh)
        self.install_button = button_help(QPushButton("Install Zanik"),
                                          "Run the complete first-install workflow on a server where Zanik is not installed.")
        self.install_button.clicked.connect(self.install); self.install_button.setEnabled(False); row.addWidget(self.install_button)
        self.update_server_button = button_help(QPushButton("Update / repair Zanik"),
                                                "Reinstall the embedded Zanik payload using the existing mode, preserve data and assets, then validate website setup and asset compatibility.")
        self.update_server_button.clicked.connect(self.update_or_repair); self.update_server_button.setEnabled(False); row.addWidget(self.update_server_button)
        actions = (("Recover interrupted", self.recover_interrupted, "Resume or roll back an interrupted Manager transaction for this server."),
                   ("Uninstall", self.uninstall, "Remove verified Zanik-managed files while preserving server data by default."),
                   ("Open panel", self.open_panel, "Open this server's page in the configured Pterodactyl panel."))
        for label, handler, tooltip in actions:
            button = button_help(QPushButton(label), tooltip); button.clicked.connect(handler); row.addWidget(button)
        layout.addWidget(self.server_summary); layout.addLayout(row); layout.addStretch(); self.tabs.addTab(page, "Servers")

    def _assets_tab(self):
        page = QWidget(); layout = QFormLayout(page)
        self.game_root = QLineEdit(); roots = discover_steam_game_roots()
        if roots: self.game_root.setText(str(roots[0]))
        browse = button_help(QPushButton("Browse…"), "Select the local licensed Dragonwilds client used to generate website assets.")
        browse.clicked.connect(self.choose_game_root)
        row = QHBoxLayout(); row.addWidget(self.game_root); row.addWidget(browse)
        self.bundle_box = QComboBox(); self.extract_button = button_help(
            QPushButton("Extract and validate assets"),
            "Generate a complete validated website asset bundle from the selected local Dragonwilds client.")
        self.extract_button.clicked.connect(self.extract_assets)
        self.resume_extract_button = button_help(QPushButton("Resume extraction"),
                                                 "Continue a verified interrupted extraction without repeating completed stages.")
        self.resume_extract_button.clicked.connect(self.resume_asset_extraction)
        self.discard_extract_button = button_help(QPushButton("Discard generated work"),
                                                  "Safely remove only the verified temporary extraction workspace; cached bundles are kept.")
        self.discard_extract_button.clicked.connect(self.discard_asset_extraction)
        recovery_row = QHBoxLayout(); recovery_row.addWidget(self.resume_extract_button); recovery_row.addWidget(self.discard_extract_button)
        self.upload_asset_button = button_help(QPushButton("Upload and activate selected bundle"),
                                               "Upload the selected validated bundle to the explicitly selected target server and activate it atomically.")
        self.upload_asset_button.clicked.connect(lambda: self.upload_assets())
        open_button = button_help(QPushButton("Open bundle folder"),
                                  "Open the Windows folder containing the selected validated bundle.")
        open_button.clicked.connect(self.open_asset_folder)
        layout.addRow("Dragonwilds client", row); layout.addRow(self.extract_button); layout.addRow(recovery_row)
        self.asset_target_box = QComboBox()
        layout.addRow("Validated bundle", self.bundle_box)
        layout.addRow("Target server", self.asset_target_box)
        layout.addRow(self.upload_asset_button); layout.addRow(open_button)
        self.game_root.editingFinished.connect(self.refresh_bundles)
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
        row = QHBoxLayout(); load = button_help(QPushButton("Load from server"),
                                               "Download and display the selected server's current Zanik configuration.")
        save = button_help(QPushButton("Validate and save"),
                           "Validate these settings, back up the remote configuration, and save only if it has not changed remotely.")
        load.clicked.connect(lambda: self.load_config()); save.clicked.connect(self.save_config)
        row.addWidget(load); row.addWidget(save); row.addStretch()
        layout.addWidget(QLabel("Mutation and QoL features are off by default.")); layout.addWidget(self.config_editors, 1); layout.addLayout(row)
        self.tabs.addTab(page, "Configuration")

    def _doctor_tab(self):
        page = QWidget(); layout = QVBoxLayout(page); row = QHBoxLayout()
        quick = button_help(QPushButton("Run quick Doctor"),
                            "Check installation, configuration, launcher, assets, and server status without a full asset hash audit.")
        deep = button_help(QPushButton("Run deep Doctor"),
                           "Run the full server-side asset verification in addition to the normal Doctor checks.")
        support = button_help(QPushButton("Create support ZIP"),
                              "Create a reviewable, redacted support archive without credentials, authorization data, or game assets.")
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

    def _update_status_bar(self):
        container = QWidget(); layout = QHBoxLayout(container)
        layout.setContentsMargins(4, 0, 4, 0); layout.setSpacing(6)
        self.manager_update_label = QLabel(f"Manager {self.manager_version} · Not checked")
        self.manager_check_button = button_help(QPushButton("Check"), "Check GitHub for a newer stable Zanik Manager release.")
        self.manager_check_button.clicked.connect(lambda: self.check_manager_update(True))
        self.manager_install_button = button_help(QPushButton("Update and relaunch"),
                                                  "Download and verify the newer Manager, replace this portable copy transactionally, and relaunch it.")
        self.manager_install_button.clicked.connect(self.install_manager_update); self.manager_install_button.hide()
        self.game_update_label = QLabel("Game · No server selected")
        self.game_check_button = button_help(QPushButton("Check"),
                                             "Compare the active server's installed Dragonwilds build with the public dedicated-server build. This works before Zanik is installed and never installs an update.")
        self.game_check_button.clicked.connect(lambda: self.check_game_update(True))
        for widget in (QLabel("│"), self.manager_update_label, self.manager_check_button,
                       self.manager_install_button, QLabel("│"), self.game_update_label, self.game_check_button):
            layout.addWidget(widget)
        self.statusBar().addPermanentWidget(container)
        self._set_update_label(self.manager_update_label, f"Manager {self.manager_version} · Not checked", "neutral")
        self._set_update_label(self.game_update_label, "Game · No server selected", "neutral")

    @staticmethod
    def _set_update_label(label: QLabel, text: str, state: str, tooltip: str = ""):
        colors = {"current": "#2f855a", "available": "#b7791f", "neutral": "#6b7280"}
        label.setText(text); label.setStyleSheet(f"color:{colors[state]};font-weight:600")
        label.setToolTip(tooltip)

    def run_background(self, key: str, function, completed):
        if any(item[0] == key for item in self.background_threads):
            return False
        thread = QThread(self); worker = Worker(function); worker.moveToThread(thread)
        thread.started.connect(worker.run); worker.done.connect(thread.quit)
        def finished():
            self.background_threads.remove((key, thread, worker))
            if worker.error:
                completed(None, worker.error[0])
            else:
                completed(worker.result, None)
            worker.deleteLater(); thread.deleteLater()
        thread.finished.connect(finished)
        self.background_threads.append((key, thread, worker)); thread.start(); return True

    def scheduled_update_checks(self):
        self.check_manager_update(False); self.check_game_update(False)

    def check_manager_update(self, manual: bool = False):
        self._set_update_label(self.manager_update_label, f"Manager {self.manager_version} · Checking…", "neutral")
        self.manager_check_button.setEnabled(False)
        def done(result, error):
            self.manager_check_button.setEnabled(True)
            if error:
                self._set_update_label(self.manager_update_label, f"Manager {self.manager_version} · Unknown", "neutral",
                                       "The last update check failed. See Activity for details.")
                self.log(f"Manager update check failed: {error}"); return
            self.manager_release_status = result
            self.validated_manager_update = result if result.get("updateAvailable") else None
            if result.get("updateAvailable"):
                latest = result["latestVersion"]
                self._set_update_label(self.manager_update_label, f"Manager {self.manager_version} · {latest} available", "available",
                                       f"Stable Zanik Manager {latest} is available from GitHub and its release metadata was validated.")
                self.manager_install_button.show()
            else:
                self._set_update_label(self.manager_update_label, f"Manager {self.manager_version} · Up to date", "current")
                self.manager_install_button.hide()
            if manual: self.log("Manager update check completed.")
            if self.active():
                self.check_installation_state(self.active().id)
        self.run_background("manager-update-check", lambda: self.release_checker.check(self.manager_version, force=manual), done)

    def check_game_update(self, manual: bool = False):
        profile = self.active()
        if not profile:
            self._set_update_label(self.game_update_label, "Game · No server selected", "neutral")
            self.game_check_button.setEnabled(False); return
        self.game_check_button.setEnabled(True)
        self._set_update_label(self.game_update_label, f"Game ({profile.name}) · Checking…", "neutral")
        profile_id = profile.id
        def done(result, error):
            if not self.active() or self.active().id != profile_id:
                QTimer.singleShot(0, lambda: self.check_game_update(False))
                return
            if error:
                self._set_update_label(self.game_update_label, f"Game ({profile.name}) · Unknown", "neutral",
                                       "The read-only dedicated-server update check failed. See Activity for details.")
                self.log(f"Game update check failed for {profile.name}: {error}"); return
            status = result.get("status")
            mod_state = ("Zanik is not installed." if result.get("zanikInstalled") is False else
                         "Zanik is installed but disabled." if result.get("zanikEnabled") is False else
                         "Zanik is installed." if result.get("zanikInstalled") else
                         "Zanik installation state is unknown.")
            if status == "update-available" or result.get("updateAvailable") is True:
                self._set_update_label(self.game_update_label, f"Game ({profile.name}) · Update available", "available",
                                       "Update Dragonwilds through the host or Steam workflow, then revalidate Zanik compatibility and regenerate assets if needed. " + mod_state)
            elif status == "current" or result.get("updateAvailable") is False:
                suffix = "Current" if result.get("fresh", False) else "Current (cached)"
                self._set_update_label(self.game_update_label, f"Game ({profile.name}) · {suffix}", "current",
                                       f"Installed build {result.get('installedBuild', 'unknown')}; latest build {result.get('latestBuild', 'unknown')}. {mod_state}")
            else:
                label = "Offline (cached unavailable)" if status == "offline-no-cache" else "Unknown"
                explanation = (result.get("message") or result.get("reason", "The game build could not be checked.")) + " " + mod_state
                if status in {"launcher-unavailable", "unsupported-or-timeout"}:
                    explanation += " Use Update / repair Zanik on the Servers tab if this installation should be running through Zanik."
                self._set_update_label(self.game_update_label, f"Game ({profile.name}) · {label}", "neutral",
                                       explanation)
            if manual:
                self.log(f"Game update check completed for {profile.name}: {status}; "
                         f"Zanik {'installed' if result.get('zanikInstalled') else 'not installed'}.")
        self.run_background("game-update-check", lambda: self.provider(profile, background=True).check_game_update(
            profile.server_id, profile.server_root, force=manual), done)

    def install_manager_update(self):
        result = self.validated_manager_update
        if not result: return
        if QMessageBox.question(self, "Update Zanik Manager",
                                f"Install verified Zanik Manager {result['latestVersion']} and relaunch now?\n\n"
                                "Your profiles, credentials, cached assets, and settings are stored separately and will not be changed.",
                                QMessageBox.Yes | QMessageBox.No, QMessageBox.Yes) != QMessageBox.Yes:
            return
        def done(staged):
            if not staged.get("ready"):
                QDesktopServices.openUrl(QUrl.fromLocalFile(staged["staged"]))
                QMessageBox.information(self, "Verified update staged", staged["message"] + "\n\nIts folder has been opened for manual replacement.")
                return
            GitHubReleaseChecker.launch(staged)
            QApplication.quit()
        self.run_async(lambda: self.release_checker.stage(result, distribution_root()), done,
                       title="Downloading and verifying Manager update…", determinate=False)

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
        self.refresh_bundles()
        safe_message = redact(message); safe_details = redact(details)
        root = Path(self.game_root.text()) if self.game_root.text() else None
        if root and root.is_dir():
            extractor = AssetExtractor(resource_path("asset-tools"), BundleCache(self.paths["assets"]))
            try:
                status = extractor.resume_compatibility(self.asset_work_root(root), root)
            except (OSError, RuntimeError):
                status = extractor.resumable(self.asset_work_root(root))
            if status:
                if status.get("compatible", True):
                    safe_message += f"\n\nVerified work was preserved at stage {status['stage']}. Use Resume extraction or Discard generated work on the Assets tab."
                else:
                    safe_message += "\n\nThe saved extraction belongs to an older game or tool version. Discard it from the Assets tab before starting fresh."
        QMessageBox.critical(self, "Operation failed", safe_message + "\n\nTechnical details are available in Activity.")
        self.activity.appendPlainText("Operation failed: " + safe_message + "\n" + safe_details)
        self.statusBar().showMessage("Operation failed: " + safe_message, 5000)

    def retry_pending_asset_cleanup(self):
        if self.threads:
            QTimer.singleShot(1000, self.retry_pending_asset_cleanup)
            return
        extractor = AssetExtractor(resource_path("asset-tools"), BundleCache(self.paths["assets"]))
        pending = [path for path in self.paths["asset_work"].iterdir()
                   if path.is_dir() and (extractor.resumable(path) or {}).get("stage") == "complete"]
        if not pending:
            return
        def done(cleaned):
            if cleaned:
                self.log(f"Cleaned {len(cleaned)} completed deferred asset workspace(s).")
            self.refresh_bundles()
        self.run_async(lambda: extractor.cleanup_completed(self.paths["asset_work"]), done,
                       title="Checking deferred asset cleanup…")

    def active(self) -> ServerProfile | None:
        return self.profile_box.currentData()

    def provider(self, profile: ServerProfile | None = None, *, background: bool = False) -> PterodactylProvider:
        profile = profile or self.active()
        if not profile: raise RuntimeError("Add and select a server first")
        token = self.vault.read(profile.credential_target)
        if not token: raise RuntimeError("The API key is missing from Windows Credential Manager")
        status = ((lambda message: self.activity_signal.emit(message)) if background else
                  (lambda message: self.operation_progress_signal.emit(message, -1)))
        return PterodactylProvider(PterodactylClient(profile.panel_origin, token, status=status),
                                   JournalStore(self.paths["journals"]))

    def reload_profiles(self):
        remembered = self.preferences.get("lastProfileId", "")
        self.profile_box.blockSignals(True)
        self.profile_box.clear()
        for profile in self.profiles: self.profile_box.addItem(profile.name, profile)
        selected = preferred_profile_index(self.profiles, remembered)
        if self.profiles:
            self.profile_box.setCurrentIndex(selected)
            self.preference_store.save_last_profile(self.profiles[selected].id)
            self.preferences["lastProfileId"] = self.profiles[selected].id
        self.profile_box.blockSignals(False)
        self.refresh_asset_targets(); self.refresh_header(); self.refresh_bundles()
        self.active_profile_changed()

    def active_profile_changed(self):
        self.refresh_header()
        profile = self.active()
        if not profile or self.current_config_profile_id != profile.id:
            self.current_config_hash = ""; self.current_config_profile_id = ""
            self.reset_config_editor()
        if profile:
            self.preference_store.save_last_profile(profile.id); self.preferences["lastProfileId"] = profile.id
            self.install_button.setEnabled(False); self.update_server_button.setEnabled(False)
            self.server_summary.setText(f"{profile.name}\nChecking installation…")
            for index in range(self.asset_target_box.count()):
                if self.asset_target_box.itemData(index).id == profile.id:
                    self.asset_target_box.setCurrentIndex(index)
                    break
            QTimer.singleShot(150, lambda profile_id=profile.id:
                              self.check_game_update(False) if self.active() and self.active().id == profile_id else None)
            QTimer.singleShot(250, lambda profile_id=profile.id: self.load_startup_config(profile_id))
            QTimer.singleShot(100, lambda profile_id=profile.id: self.check_installation_state(profile_id))
        elif hasattr(self, "game_update_label"):
            self._set_update_label(self.game_update_label, "Game · No server selected", "neutral")
            self.game_check_button.setEnabled(False)
            self.install_button.setEnabled(False); self.update_server_button.setEnabled(False)

    def refresh_asset_targets(self):
        selected = self.asset_target_box.currentData().id if self.asset_target_box.count() and self.asset_target_box.currentData() else ""
        self.asset_target_box.clear()
        for profile in self.profiles:
            self.asset_target_box.addItem(f"{profile.name} · {profile.panel_origin} · {profile.server_id}", profile)
        wanted = selected or (self.active().id if self.active() else "")
        for index in range(self.asset_target_box.count()):
            if self.asset_target_box.itemData(index).id == wanted:
                self.asset_target_box.setCurrentIndex(index); break

    def refresh_header(self):
        profile = self.active()
        insecure = bool(profile and profile.allow_insecure_http and is_remote_http(profile.panel_origin))
        self.warning.setText("⚠ This panel connection uses HTTP; API traffic is not encrypted." if insecure else "")
        if profile: self.server_summary.setText(f"{profile.name}\n{profile.panel_origin} · {profile.server_id}\nInstall mode: {profile.install_mode}")
        if profile:
            incomplete=JournalStore(self.paths["journals"]).incomplete(profile.id)
            if incomplete:
                self.warning.setText((self.warning.text()+"\n" if self.warning.text() else "")+f"An interrupted {incomplete[-1].action} operation needs attention.")

    def check_installation_state(self, profile_id: str | None = None):
        profile = self.active()
        if not profile or (profile_id and profile.id != profile_id):
            return
        self.install_button.setEnabled(False); self.update_server_button.setEnabled(False)
        self.server_summary.setText(f"{profile.name}\nChecking installation…")
        selected_id = profile.id
        try:
            payload_metadata = json.loads(resource_path("payload", "server-payload.json").read_text(encoding="utf-8-sig"))
            expected_contract = {"schemaVersion": payload_metadata["payloadContractSchemaVersion"],
                                 "contractSha256": payload_metadata["payloadContractSha256"]}
        except (OSError, ValueError, KeyError, TypeError):
            expected_contract = None
        try:
            legacy_contracts = json.loads(resource_path("config", "server-payload-legacy-contracts.json").read_text(
                encoding="utf-8-sig")).get("releases", {})
        except (OSError, ValueError, TypeError):
            legacy_contracts = {}
        def done(result, error):
            if not self.active() or self.active().id != selected_id:
                return
            if error:
                self.installation_states.pop(selected_id, None)
                self.server_summary.setText(f"{profile.name}\nInstallation state unavailable; use Refresh to retry.")
                self.log(f"Could not determine Zanik installation state for {profile.name}: {error}")
                return
            self.installation_states[selected_id] = result
            installed = bool(result.get("installed"))
            self.install_button.setEnabled(not installed)
            self.update_server_button.setEnabled(installed)
            if installed:
                mode = result.get("mode") or profile.install_mode or "unknown mode"
                summary = compatibility_summary(result, self.manager_version, self.manager_release_status)
                self.server_summary.setText(f"{profile.name}\n{summary}\nInstall mode: {mode}")
            else:
                self.server_summary.setText(f"{profile.name}\nZanik is not installed")
        self.run_background(f"installation-state-{selected_id}",
                            lambda: self.provider(profile, background=True).installation_status(
                                profile.server_id, profile.server_root, expected_contract, legacy_contracts), done)

    def add_server(self):
        dialog = AddServerDialog(self.vault, self)
        if dialog.exec() == QDialog.Accepted and dialog.profile:
            self.profiles.append(dialog.profile); self.store.save(self.profiles)
            self.preference_store.save_last_profile(dialog.profile.id); self.preferences["lastProfileId"] = dialog.profile.id
            self.reload_profiles()

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
        box.button(QMessageBox.Retry).setToolTip("Restore the last safe state, then retry the interrupted workflow.")
        box.button(QMessageBox.Reset).setToolTip("Restore the last safe state without retrying the operation.")
        box.button(QMessageBox.Cancel).setToolTip("Leave the recovery journal unchanged and close this prompt.")
        choice=box.exec()
        if choice in {QMessageBox.Retry,QMessageBox.Reset}:
            retry=choice==QMessageBox.Retry
            self.run_async(lambda:self.provider().rollback(journal,profile.server_id,profile.server_root),
                           lambda _: (self.log("Rollback completed."),self.refresh_header(),self.install() if retry and journal.action=="install" else None),
                           title="Recovering the interrupted operation…")

    def open_panel(self):
        if self.active(): QDesktopServices.openUrl(QUrl(self.active().panel_origin + "/server/" + self.active().server_id))

    def install(self):
        self.install_profile(self.active())

    def update_or_repair(self):
        profile = self.active()
        if not profile:
            return
        state = self.installation_states.get(profile.id)
        if not state or not state.get("installed"):
            QMessageBox.information(self, "Zanik is not installed",
                                    "Install Zanik before using Update / repair Zanik.")
            self.check_installation_state(profile.id); return
        mode = state.get("mode") or profile.install_mode
        if mode not in {"wrapper", "replace-executable"}:
            QMessageBox.critical(self, "Installation mode unavailable",
                                 "The existing installation mode could not be identified safely. Run Doctor and repair the installation metadata.")
            return
        embedded = resource_path("VERSION").read_text(encoding="utf-8").strip()
        current = state.get("version") or "unknown"
        if state.get("payloadCompatibility") == "current":
            action = (f"Repair {profile.name} using the server payload already compatible with Manager {embedded}?\n\n"
                      f"The installed Zanik release is {current}. This is an intentional verified repair; no server update is required.")
        else:
            action = f"Update or repair {profile.name} from Zanik {current} using the server payload in Manager {embedded}?"
        message = (action + f"\n\nInstallation mode: {mode}. Configuration, authorization, persistent data, sessions, "
                   "and active website assets will be preserved.")
        if mode == "replace-executable":
            message += "\n\nThis mode manages the game executable path while preserving the verified official executable."
        if QMessageBox.warning(self, "Update / repair Zanik", message,
                               QMessageBox.Yes | QMessageBox.No, QMessageBox.Yes) != QMessageBox.Yes:
            return
        self.install_profile(profile, forced_mode=mode,
                             replacement_acknowledged=(mode == "replace-executable"), maintenance=True)

    def install_profile(self, profile: ServerProfile | None, forced_mode: str | None = None,
                        replacement_acknowledged: bool = False, repair_context: bool = False,
                        maintenance: bool = False):
        if not profile: return
        try:
            adapter = self.provider(profile).client.wrapper_adapter(profile.server_id)
        except Exception as error:
            QMessageBox.critical(self, "Startup inspection failed", str(error)); return
        selected = forced_mode
        if not selected:
            dialog = QDialog(self); dialog.setWindowTitle("Choose installation mode"); form = QFormLayout(dialog)
            choices = QComboBox(); choices.addItem("Wrapper — recommended", "wrapper"); choices.addItem("Replace executable — compatibility", "replace-executable")
            if not adapter: choices.setCurrentIndex(1)
            note = QLabel("This egg has a supported editable startup variable; wrapper mode can be configured automatically." if adapter else
                          "This egg has no recognized wrapper variable. Wrapper mode will install files and show the exact manual startup command.")
            note.setWordWrap(True); buttons = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel)
            buttons.button(QDialogButtonBox.Ok).setToolTip("Continue with the selected installation mode.")
            buttons.button(QDialogButtonBox.Cancel).setToolTip("Cancel without changing the server.")
            buttons.accepted.connect(dialog.accept); buttons.rejected.connect(dialog.reject)
            form.addRow(note); form.addRow("Mode", choices); form.addRow(buttons)
            if dialog.exec() != QDialog.Accepted: return
            selected = choices.currentData()
        if selected == "replace-executable" and not replacement_acknowledged and QMessageBox.warning(self, "Core-file replacement",
            "This compatibility mode replaces the executable path while preserving the official binary. Continue?",
            QMessageBox.Yes | QMessageBox.No) != QMessageBox.Yes: return
        archive = resource_path("payload", "server.zip")
        if not archive.exists():
            matches = sorted((resource_path("dist", "release")).glob("zanik-server-*.zip"))
            if not matches: QMessageBox.critical(self, "Missing payload", "The matching server payload is missing."); return
            archive = matches[-1]
        def progress(message, percent): self.operation_progress_signal.emit(message, percent)
        def action(): return self.provider(profile).install(profile.id, profile.server_id, profile.server_root,
                                                     ReleasePayload(archive), selected, selected == "replace-executable",
                                                     wrapper_adapter=adapter if selected == "wrapper" else None,
                                                     progress=progress)
        def done(result):
            profile.install_mode = selected; self.store.save(self.profiles)
            self.check_installation_state(profile.id)
            if selected == "wrapper" and not adapter:
                self.log(f"Files installed; startup action remains. Backup: {result['backup']}")
                QMessageBox.information(self, "Startup action required", f"The server remains stopped and installation is not yet active. Set the egg startup executable to:\n{profile.server_root}/zanik/bin/zanik-server --server-root {profile.server_root} --\n\nThen start the server and run Doctor to verify it.")
                if maintenance:
                    self.post_update_checks(profile)
            else:
                self.log(f"Installation complete; backup: {result['backup']}")
                if maintenance:
                    self.log(f"Update / repair complete; backup: {result['backup']}")
                    QMessageBox.information(self, "Update / repair complete",
                                            "Zanik was updated and verified. Existing configuration, persistent data, and website assets were preserved.")
                    self.post_update_checks(profile)
                elif repair_context:
                    QMessageBox.information(self, "Launcher repaired",
                                            "The replacement launcher was repaired and the server was started. Existing configuration and website assets were preserved.")
                else:
                    QMessageBox.information(self, "Installation complete",
                                            "Zanik was installed successfully and the server was started.")
                    self.offer_website_settings(profile)
        self.run_async(action, done, title="Installing Zanik…", determinate=True)

    def offer_launcher_repair(self, profile: ServerProfile, launcher: dict):
        self.log(f"Steam displaced the replacement launcher on {profile.name}; website service is not active.")
        box = QMessageBox(self); box.setWindowTitle("Zanik launcher needs repair")
        box.setIcon(QMessageBox.Warning)
        box.setText("The server restarted, but Steam replaced Zanik's compatibility launcher with the official game executable.")
        box.setInformativeText(
            f"Your activated assets are safe. Repair the launcher now on {profile.name} ({profile.server_id}) so Zanik and its website load again. "
            "This uses the existing replacement mode and preserves the newly updated official executable.")
        repair = box.addButton("Repair launcher now", QMessageBox.AcceptRole)
        later = box.addButton("Later", QMessageBox.RejectRole)
        repair.setToolTip("Reinstall the managed compatibility launcher while preserving the official executable and all Zanik data.")
        later.setToolTip("Leave the assets installed but keep the server running without Zanik until you repair it later.")
        box.setDefaultButton(repair); box.exec()
        if box.clickedButton() is repair:
            self.install_profile(profile, forced_mode="replace-executable",
                                 replacement_acknowledged=True, repair_context=True)

    def uninstall(self):
        profile = self.active()
        if not profile or QMessageBox.warning(self, "Uninstall Zanik", "Uninstall Zanik from the selected server? Persistent data will be kept.",
                                               QMessageBox.Yes | QMessageBox.No) != QMessageBox.Yes: return
        self.run_async(lambda: self.provider().uninstall(profile.id, profile.server_id, profile.server_root),
                       lambda result: (self.log(f"Uninstalled; recovery backup: {result['backup']}"),
                                       self.check_installation_state(profile.id)),
                       title="Uninstalling Zanik…")

    def run_doctor(self, deep=False):
        profile = self.active()
        if not profile: return
        self.run_async(lambda: self.provider().diagnose(profile.server_id, profile.server_root, deep),
                       lambda result: (self.doctor_output.setPlainText(json.dumps(result, indent=2, sort_keys=True)),
                                       self.server_summary.setText("Healthy" if result.get("healthy") else "Doctor found problems"),
                                       self.check_installation_state(profile.id)),
                       title="Running deep Doctor…" if deep else "Running Doctor…")

    def choose_game_root(self):
        selected = QFileDialog.getExistingDirectory(self, "Select the RSDragonwilds installation", self.game_root.text())
        if selected: self.game_root.setText(selected); self.refresh_bundles()

    def refresh_bundles(self):
        self.bundle_box.clear(); root = Path(self.game_root.text()) if self.game_root.text() else None
        if not root: return
        for item in BundleCache(self.paths["assets"]).list(game_root_id(root)):
            self.bundle_box.addItem(
                f"Build {item.build} · validated {format_bundle_timestamp(item.created_at)} · {item.manifest_hash[:12]}",
                str(item.path))
        tool_root = resource_path("asset-tools")
        status = AssetExtractor(tool_root, BundleCache(self.paths["assets"])).resumable(self.asset_work_root(root))
        self.resume_extract_button.setEnabled(bool(status)); self.discard_extract_button.setEnabled(bool(status))
        if status:
            self.resume_extract_button.setText(f"Resume extraction ({status['stage']})")
        else:
            self.resume_extract_button.setText("Resume extraction")

    def asset_recovery_root(self, root: Path) -> Path:
        return self.paths["mapping_recovery"] / game_root_id(root)

    def asset_work_root(self, root: Path) -> Path:
        return self.paths["asset_work"] / game_root_id(root)

    def confirm_asset_preflight(self, root: Path, extractor: AssetExtractor) -> tuple[bool, bool] | None:
        recovery = self.asset_recovery_root(root)
        status = extractor.mapping_preflight(root, recovery)
        close_game = bool(status.get("gameRunning"))
        if close_game:
            box = QMessageBox(self); box.setWindowTitle("Dragonwilds must close")
            box.setText("Dragonwilds is running and must close before website assets can be extracted.")
            box.setInformativeText("Zanik Manager will request a normal close, then briefly open and close the game automatically to generate mappings. It will never force-kill the game.")
            proceed = box.addButton("Close game and continue", QMessageBox.AcceptRole)
            cancel = box.addButton("Cancel", QMessageBox.RejectRole)
            proceed.setToolTip("Request a normal Dragonwilds shutdown, then continue extraction after it exits.")
            cancel.setToolTip("Cancel extraction without closing or changing Dragonwilds.")
            box.setDefaultButton(cancel); box.exec()
            if box.clickedButton() is not proceed: return None
        preserve = bool(status.get("existingLoader"))
        if preserve:
            paths = "\n".join(status.get("loaderPaths", []))
            message = "Dragonwilds already has a UE4SS or proxy-loader installation."
            if status.get("unfinishedRecovery"):
                message = "An interrupted Zanik mapping operation must be recovered before extraction."
            box = QMessageBox(self); box.setWindowTitle("Preserve existing game tools")
            box.setText(message)
            box.setInformativeText("Zanik Manager will back up and verify the complete existing loader, temporarily use its pinned mapper, then restore the original files exactly.\n\n" + paths)
            proceed = box.addButton("Preserve, restore, and continue", QMessageBox.AcceptRole)
            cancel = box.addButton("Cancel", QMessageBox.RejectRole)
            proceed.setToolTip("Back up and verify the existing loader, use Zanik's mapper temporarily, then restore it exactly.")
            cancel.setToolTip("Cancel extraction without changing the existing loader.")
            box.setDefaultButton(cancel); box.exec()
            if box.clickedButton() is not proceed: return None
        return close_game, preserve

    def choose_or_confirm_game_root(self, profile: ServerProfile | None = None) -> Path | None:
        value = profile.game_root if profile and profile.game_root else self.game_root.text()
        root = Path(value) if value else Path()
        if not root.is_dir():
            selected = QFileDialog.getExistingDirectory(self, "Select the RSDragonwilds installation", value)
            if not selected: return None
            root = Path(selected)
        self.game_root.setText(str(root))
        if profile:
            profile.game_root = str(root); self.store.save(self.profiles)
        self.refresh_bundles()
        return root

    def offer_website_settings(self, profile: ServerProfile, continuation=None):
        config_path = f"{profile.server_root}/zanik/config/zanik.conf"
        def continue_flow():
            (continuation or (lambda: self.offer_asset_setup(profile)))()
        def loaded(data):
            try:
                model = self.config_model(); values = model.parse(data.decode())
            except Exception as error:
                self.log(f"Could not prepare website settings: {error}")
                QMessageBox.warning(self, "Website settings unavailable",
                                    "The server configuration could not be loaded. You can set the website port and public URL later on the Configuration tab. Website asset setup can still continue.")
                continue_flow(); return
            dialog = QDialog(self); dialog.setWindowTitle("Configure the Zanik website")
            form = QFormLayout(dialog)
            explanation = QLabel(
                "Zanik's website needs its own TCP port. Use a port allocated and open for this server—not the Dragonwilds game port. "
                "The public URL is the address players and administrators will use to open the website and receive login links.")
            explanation.setWordWrap(True)
            port = QSpinBox(); port.setRange(1, 65535); port.setValue(int(values["STATUS_PORT"]))
            public_url = QLineEdit(str(values["PUBLIC_BASE_URL"])); public_url.setPlaceholderText("http://server.example:29105")
            later_note = QLabel("You can skip this step and change both values later: select this server, open the Configuration tab, then edit Networking.")
            later_note.setWordWrap(True)
            buttons = QDialogButtonBox()
            save_button = buttons.addButton("Save and continue", QDialogButtonBox.AcceptRole)
            skip_button = buttons.addButton("Skip for now", QDialogButtonBox.RejectRole)
            save_button.setToolTip("Validate and save the website port and public URL, restart this server, then continue to asset setup.")
            skip_button.setToolTip("Leave the current networking settings unchanged and continue to asset setup; edit them later on Configuration.")
            buttons.accepted.connect(dialog.accept); buttons.rejected.connect(dialog.reject)
            form.addRow(explanation); form.addRow("Zanik TCP port", port); form.addRow("Public website URL", public_url)
            form.addRow(later_note); form.addRow(buttons)
            if dialog.exec() != QDialog.Accepted:
                self.log(f"Website port and public URL setup for {profile.name} was skipped.")
                continue_flow(); return
            try:
                rendered = update_website_settings(model, data.decode(), port.value(), public_url.text())
            except Exception as error:
                QMessageBox.critical(self, "Invalid website settings", str(error))
                self.offer_website_settings(profile, continuation=continuation); return
            expected = content_hash(data); content = rendered.encode()
            def action():
                try:
                    provider = self.provider(profile)
                    result = provider.save_config(profile.server_id, profile.server_root, content, expected)
                    self.operation_progress_signal.emit("Restarting the server to apply website settings", 85)
                    provider.client.power(profile.server_id, "restart")
                    return {"result": result}
                except Exception as error:
                    return {"error": str(error), "details": traceback.format_exc()}
            def saved(outcome):
                if outcome.get("error"):
                    safe_message = redact(outcome["error"]); safe_details = redact(outcome["details"])
                    self.activity.appendPlainText("Website settings were not saved: " + safe_message + "\n" + safe_details)
                    QMessageBox.warning(self, "Website settings were not saved",
                                        safe_message + "\n\nYou can retry later from the Configuration tab. Website asset setup can still continue.")
                else:
                    result = outcome["result"]
                    self.log(f"Website settings saved; backup: {result['backup']}")
                continue_flow()
            self.run_async(action, saved, title="Saving website settings…", determinate=True)
        self.run_async(lambda: self.provider(profile).client.read_file(profile.server_id, config_path), loaded,
                       title=f"Loading website settings for {profile.name}…")

    def post_update_checks(self, profile: ServerProfile):
        contract = expected_asset_contract(resource_path("asset-tools"))
        def action():
            provider = self.provider(profile)
            config = provider.read_configuration(profile.server_id, profile.server_root)
            configured = False; config_reason = "The server configuration is unavailable."
            if config.get("exists") and config.get("data"):
                try:
                    values = self.config_model().parse(config["data"].decode("utf-8-sig"))
                    configured = website_configuration_ready(values)
                    config_reason = ("Website networking is configured." if configured else
                                     "The public website URL still uses a local placeholder.")
                except Exception as error:
                    config_reason = f"The preserved configuration is invalid: {error}"
            assets = provider.asset_contract_status(profile.server_id, profile.server_root, contract)
            return {"websiteConfigured": configured, "websiteReason": config_reason, "assets": assets}
        def checked(result):
            def continue_assets(): self.offer_update_assets(profile, result["assets"])
            if result["websiteConfigured"]:
                self.log(f"Website configuration is already set up on {profile.name}.")
                continue_assets(); return
            box = QMessageBox(self); box.setWindowTitle("Website setup needs attention")
            box.setText(f"Review the Zanik website settings for {profile.name}.")
            box.setInformativeText(result["websiteReason"] + " Configure the website port and public URL now?")
            configure = box.addButton("Configure website now", QMessageBox.AcceptRole)
            later = box.addButton("Later", QMessageBox.RejectRole)
            configure.setToolTip("Set the separate Zanik website port and its public URL, then continue asset checks.")
            later.setToolTip("Keep the current values and continue asset checks; edit them later under Configuration → Networking.")
            box.setDefaultButton(configure); box.exec()
            if box.clickedButton() is configure:
                self.offer_website_settings(profile, continuation=continue_assets)
            else:
                self.log(f"Website configuration review for {profile.name} was deferred.")
                continue_assets()
        self.run_async(action, checked, title=f"Validating {profile.name} after update…", determinate=False)

    def offer_update_assets(self, profile: ServerProfile, status: dict):
        if status.get("valid"):
            self.log(f"Active website assets on {profile.name} match the packaged extraction contract; extraction is not needed.")
            return
        reasons = status.get("reasons") or ["The active website asset bundle could not be validated."]
        box = QMessageBox(self); box.setWindowTitle("Website assets need attention")
        box.setText(f"{profile.name} needs a new validated website asset bundle.")
        box.setInformativeText("\n".join(f"• {reason}" for reason in reasons[:6]))
        setup = box.addButton("Extract and deploy assets now", QMessageBox.AcceptRole)
        later = box.addButton("Later", QMessageBox.RejectRole)
        setup.setToolTip("Extract or reuse a compatible local bundle, upload it to this server, and activate it atomically.")
        later.setToolTip("Keep the current server assets unchanged and perform setup later from the Assets tab.")
        box.setDefaultButton(setup); box.exec()
        if box.clickedButton() is setup:
            self.guided_asset_setup(profile)
        else:
            self.log(f"Website asset refresh for {profile.name} was deferred.")

    def offer_asset_setup(self, profile: ServerProfile):
        def checked(result):
            assets = result.get("assets", {})
            if assets.get("installed") and assets.get("compatible") is not False:
                self.log(f"Website assets are already active on {profile.name}.")
                return
            box = QMessageBox(self); box.setWindowTitle("Set up website assets")
            box.setText(f"{profile.name} does not have a healthy active website asset bundle.")
            box.setInformativeText("Zanik Manager can extract the required assets from your licensed Dragonwilds client, upload them to this server, and activate them now. Dragonwilds must initially be closed; it will briefly open and close automatically for mapping generation.")
            setup = box.addButton("Set up website assets now", QMessageBox.AcceptRole)
            later = box.addButton("Later", QMessageBox.RejectRole)
            setup.setToolTip("Extract or reuse a validated local bundle, upload it to this server, and activate it.")
            later.setToolTip("Finish installation without website assets; set them up later from the Assets tab.")
            box.setDefaultButton(setup); box.exec()
            if box.clickedButton() is setup:
                self.guided_asset_setup(profile)
            else:
                self.log(f"Website asset setup for {profile.name} was deferred.")
        self.run_async(lambda: self.provider(profile).diagnose(profile.server_id, profile.server_root), checked,
                       title=f"Checking website assets on {profile.name}…")

    def guided_asset_setup(self, profile: ServerProfile):
        root = self.choose_or_confirm_game_root(profile)
        if not root: return
        tool_root = resource_path("asset-tools"); cache = BundleCache(self.paths["assets"])
        extractor = AssetExtractor(tool_root, cache)
        all_cached = cache.list(game_root_id(root))
        current_executable = extractor.current_fingerprints(root)["gameExecutableSha256"] if all_cached else None
        cached = [item for item in all_cached
                  if extractor.bundle_compatible_with_game(item.path, root, current_executable)]
        if all_cached and not cached:
            self.log("Existing cached asset bundles target an older Dragonwilds executable and will not be reused.")
        use_cached = False
        if cached:
            use_cached = QMessageBox.question(
                self, "Reuse validated assets",
                f"A cached bundle for build {cached[0].build} is available. Validate and reuse it instead of opening the game again?",
                QMessageBox.Yes | QMessageBox.No, QMessageBox.Yes) == QMessageBox.Yes
        work = self.asset_work_root(root)
        resume_status = None if use_cached else extractor.resume_compatibility(work, root)
        if resume_status and not resume_status["compatible"]:
            self.offer_stale_asset_cleanup(root, extractor, resume_status,
                                           lambda: self.guided_asset_setup(profile))
            return
        mode = asset_setup_mode(use_cached, resume_status)
        if mode == "resume":
            self.log(resume_activity_message(resume_status["stage"]))
        preflight = (False, False) if mode != "new" else self.confirm_asset_preflight(root, extractor)
        if preflight is None: return
        close_game, preserve = preflight
        output = self.paths["root"] / "extraction-output"
        def event(value):
            stage = value.get("phase", "extract")
            percent = {"preserve": 10, "mappings": 15, "restore": 25,
                       "extract": 35, "bundle": 55, "complete": 65}.get(stage, -1)
            self.operation_progress_signal.emit(value.get("message", str(value)), percent)
        def action():
            if close_game:
                self.operation_progress_signal.emit("Requesting Dragonwilds to close", 5)
                if not request_dragonwilds_close():
                    raise RuntimeError("Dragonwilds did not close normally. Close it manually and retry; it was not force-terminated.")
            if mode == "cached":
                item = cached[0]
                self.operation_progress_signal.emit("Validating the cached asset bundle", 20)
                extractor.validate_bundle(item.path)
            else:
                if preserve: self.operation_progress_signal.emit("Preserving the existing UE4SS installation", 10)
                if mode == "resume":
                    self.operation_progress_signal.emit(
                        f"Resuming verified extraction from {resume_status['stage']}", 15)
                item = extractor.extract(root, output, event, preserve_existing_loader=preserve,
                                         recovery_root=self.asset_recovery_root(root),
                                         work_root=work, resume=(mode == "resume"))
            self.operation_progress_signal.emit(f"Uploading assets to {profile.name}", 70)
            def progress(current, total, path):
                self.operation_progress_signal.emit(f"Uploading and verifying {path}", 70 + int(current * 20 / max(total, 1)))
            provider = self.provider(profile)
            activated = AssetDeployer(provider).activate(profile.id, profile.server_id, profile.server_root,
                                                         item.path, progress,
                                                         lambda message: self.operation_progress_signal.emit(message, 93))
            if activated.get("restartRequested"):
                self.operation_progress_signal.emit("Waiting for the restarted server and verifying the Zanik launcher", 94)
                activated["launcherStatus"] = provider.verify_launcher_after_restart(
                    profile.server_id, profile.server_root,
                    previous_uptime=activated.get("restartPreviousUptime"))
            self.operation_progress_signal.emit("Verifying active website assets", 95)
            health = provider.diagnose(profile.server_id, profile.server_root)
            if not health.get("assets", {}).get("installed") or health.get("assets", {}).get("compatible") is False:
                raise RuntimeError("Assets were uploaded, but the server did not report a healthy active manifest")
            return item, activated, health
        def done(result):
            item, activated, _ = result
            profile.last_asset_manifest = item.manifest_hash; self.store.save(self.profiles); self.refresh_bundles()
            self.log(f"Activated {activated['fileCount']} asset files on {profile.name}")
            if activated.get("restartRequested"):
                self.log(f"Restart requested for {profile.name} so native catalog tracking loads the activated assets")
            self.asset_activation_complete(profile, activated)
        title = (f"Resuming and deploying website assets on {profile.name}…" if mode == "resume"
                 else f"Setting up website assets on {profile.name}…")
        self.run_async(action, done, title=title, determinate=True)

    def extract_assets(self):
        root = self.choose_or_confirm_game_root()
        if not root: return
        output = self.paths["root"] / "extraction-output"
        tool_root = resource_path("asset-tools")
        extractor = AssetExtractor(tool_root, BundleCache(self.paths["assets"]))
        work = self.asset_work_root(root); resume_status = extractor.resume_compatibility(work, root)
        if resume_status and not resume_status["compatible"]:
            self.offer_stale_asset_cleanup(root, extractor, resume_status, self.extract_assets)
            return
        if resume_status:
            if QMessageBox.question(
                    self, "Resume verified extraction",
                    resume_activity_message(resume_status["stage"]) + " Resume now?",
                    QMessageBox.Yes | QMessageBox.Cancel, QMessageBox.Yes) != QMessageBox.Yes:
                return
            preflight = (False, False)
        else:
            preflight = self.confirm_asset_preflight(root, extractor)
        if preflight is None: return
        close_game, preserve = preflight
        phase_percent = {"preserve": 10, "mappings": 15, "restore": 30,
                         "extract": 40, "bundle": 80, "complete": 100}
        def event(value):
            message = value.get("message", str(value)); phase = value.get("phase", "extract")
            self.operation_progress_signal.emit(message, phase_percent.get(phase, -1))
        def action():
            if close_game:
                self.operation_progress_signal.emit("Requesting Dragonwilds to close", 5)
                if not request_dragonwilds_close():
                    raise RuntimeError("Dragonwilds did not close normally. Close it manually and retry; it was not force-terminated.")
            return extractor.extract(root, output, event, preserve_existing_loader=preserve,
                                     recovery_root=self.asset_recovery_root(root),
                                     work_root=work, resume=bool(resume_status))
        self.run_async(action,
                       lambda item: self.asset_extraction_complete(item, resumed=False),
                       title=(f"Resuming asset extraction from {resume_status['stage']}…" if resume_status
                              else "Extracting Dragonwilds web assets…"), determinate=True)

    def resume_asset_extraction(self):
        root = self.choose_or_confirm_game_root()
        if not root: return
        tool_root = resource_path("asset-tools"); cache = BundleCache(self.paths["assets"])
        extractor = AssetExtractor(tool_root, cache); work = self.asset_work_root(root)
        status = extractor.resume_compatibility(work, root)
        if not status:
            QMessageBox.information(self, "Nothing to resume", "No verified Zanik extraction workspace exists for this game installation.")
            self.refresh_bundles(); return
        if not status["compatible"]:
            self.offer_stale_asset_cleanup(root, extractor, status, self.extract_assets)
            return
        output = self.paths["root"] / "extraction-output"
        def event(value):
            phase = value.get("phase", "extract")
            percent = {"extract": 35, "bundle": 75, "cleanup-warning": 98, "complete": 100}.get(phase, -1)
            self.operation_progress_signal.emit(value.get("message", str(value)), percent)
        self.run_async(
            lambda: extractor.extract(root, output, event, recovery_root=self.asset_recovery_root(root),
                                      work_root=work, resume=True),
            lambda item: self.asset_extraction_complete(item, resumed=True),
            title=f"Resuming asset extraction from {status['stage']}…", determinate=True,
        )

    def discard_asset_extraction(self):
        root = self.choose_or_confirm_game_root()
        if not root: return
        tool_root = resource_path("asset-tools"); extractor = AssetExtractor(tool_root, BundleCache(self.paths["assets"]))
        work = self.asset_work_root(root); status = extractor.resumable(work)
        if not status:
            self.refresh_bundles(); return
        if QMessageBox.question(self, "Discard generated work",
                                f"Discard the verified Zanik extraction workspace at stage {status['stage']}?\n\nValidated cached bundles are not removed.",
                                QMessageBox.Yes | QMessageBox.No, QMessageBox.No) != QMessageBox.Yes:
            return
        self.run_async(lambda: extractor.discard(work),
                       lambda _: (self.refresh_bundles(), self.log("Discarded the incomplete asset extraction workspace.")),
                       title="Discarding generated work…")

    def offer_stale_asset_cleanup(self, root: Path, extractor: AssetExtractor, status: dict, retry):
        labels = {
            "gameExecutableSha256": "Dragonwilds was updated",
            "recipeSha256": "the extraction recipe changed",
            "toolLockSha256": "the packaged extraction tools changed",
        }
        reasons = ", ".join(labels.get(name, name) for name in status.get("mismatches", []))
        box = QMessageBox(self); box.setWindowTitle("Saved extraction is out of date")
        box.setText("The saved extraction cannot be safely resumed.")
        box.setInformativeText(
            f"Reason: {reasons}. Zanik can safely remove only its saved workspace and start a fresh extraction. "
            "Validated cached bundles are not removed.")
        discard = box.addButton("Discard stale work and start fresh", QMessageBox.AcceptRole)
        cancel = box.addButton("Cancel", QMessageBox.RejectRole)
        discard.setToolTip("Remove only this marked, incompatible Zanik workspace, then begin a fresh extraction.")
        cancel.setToolTip("Keep the saved workspace and do not start extraction.")
        box.setDefaultButton(discard); box.exec()
        if box.clickedButton() is not discard:
            return
        work = self.asset_work_root(root)
        self.run_async(
            lambda: extractor.discard(work),
            lambda _: (self.log("Discarded stale extraction work after a game or tool update."),
                       self.refresh_bundles(), retry()),
            title="Preparing a fresh asset extraction…",
        )

    def asset_extraction_complete(self, item, resumed: bool = False):
        self.refresh_bundles()
        self.log(f"{'Resumed and validated' if resumed else 'Validated'} asset bundle: {item.path}")
        profile = self.asset_target_box.currentData()
        if not profile:
            QMessageBox.information(self, "Asset extraction complete",
                                    f"The validated asset bundle is ready at:\n{item.path}")
            return
        box = QMessageBox(self); box.setWindowTitle("Asset extraction complete")
        box.setText("The website asset bundle was generated and validated successfully.")
        box.setInformativeText(
            f"Upload and activate it now on:\n\n{profile.name}\n{profile.panel_origin}\nServer {profile.server_id}\n\n"
            "The target remains explicit, and a running game server will restart after activation.")
        upload = box.addButton("Upload and activate now", QMessageBox.AcceptRole)
        later = box.addButton("Later", QMessageBox.RejectRole)
        upload.setToolTip("Upload this newly validated bundle to the displayed server and activate it atomically.")
        later.setToolTip("Keep the validated bundle in the local cache without changing a server.")
        box.setDefaultButton(upload); box.exec()
        if box.clickedButton() is upload:
            self.upload_assets(profile=profile, bundle=item.path, confirmed=True)

    def upload_assets(self, profile: ServerProfile | None = None, bundle: Path | None = None,
                      confirmed: bool = False):
        profile = profile or self.asset_target_box.currentData()
        value = bundle or self.bundle_box.currentData()
        if not profile or not value: return
        bundle = Path(value)
        if not confirmed and QMessageBox.question(self, "Confirm asset target",
                                f"Upload and activate {bundle.name} on:\n\n{profile.name}\n{profile.panel_origin}\nServer {profile.server_id}?\n\n"
                                "If the game server is running, Zanik Manager will restart it after activation so native catalog tracking loads the new assets.",
                                QMessageBox.Yes | QMessageBox.No, QMessageBox.No) != QMessageBox.Yes: return
        def progress(current, total, path):
            percent = 15 + int(current * 75 / max(total, 1))
            self.operation_progress_signal.emit(f"Uploading and verifying {path}", percent)
        def action():
            provider = self.provider(profile)
            result = AssetDeployer(provider).activate(
                profile.id, profile.server_id, profile.server_root, bundle, progress,
                lambda message: self.operation_progress_signal.emit(message, 93))
            if result.get("restartRequested"):
                self.operation_progress_signal.emit("Waiting for the restarted server and verifying the Zanik launcher", 96)
                result["launcherStatus"] = provider.verify_launcher_after_restart(
                    profile.server_id, profile.server_root,
                    previous_uptime=result.get("restartPreviousUptime"))
            return result
        self.run_async(action,
                       lambda result: self.asset_activation_complete(profile, result),
                       title="Uploading and activating web assets…", determinate=True)

    def asset_activation_complete(self, profile: ServerProfile, result: dict):
        self.log(f"Activated {result['fileCount']} asset files on {profile.name}")
        launcher = result.get("launcherStatus", {})
        if launcher.get("applicable") and not launcher.get("healthy"):
            self.offer_launcher_repair(profile, launcher)
            return
        QMessageBox.information(
            self, "Assets activated",
            f"Activated {result['fileCount']} validated asset files on {profile.name}. "
            + ("The server restarted, and the replacement launcher was verified."
               if result.get("restartRequested") and launcher.get("applicable") else
               "The running game server was restarted to load recipe and quest tracking."
               if result.get("restartRequested") else
               "The catalogs will load the next time the game server starts."))

    def open_asset_folder(self):
        value = self.bundle_box.currentData()
        path = Path(value).parent if value else self.paths["assets"]
        QDesktopServices.openUrl(QUrl.fromLocalFile(str(path)))

    def config_model(self):
        return ConfigModel(resource_path("config", "zanik-config.schema.json"))

    def reset_config_editor(self):
        self.config_text.clear()
        for key, value in self.config_model().defaults().items():
            field = self.config_fields[key]
            if isinstance(field, QCheckBox): field.setChecked(bool(value))
            elif isinstance(field, QSpinBox): field.setValue(int(value))
            else: field.setText(str(value))

    def load_startup_config(self, profile_id: str):
        if self.threads:
            QTimer.singleShot(500, lambda: self.load_startup_config(profile_id))
            return
        profile = self.active()
        if profile and profile.id == profile_id and self.current_config_profile_id != profile_id:
            self.load_config(profile, automatic=True)

    def load_config(self, profile: ServerProfile | None = None, automatic: bool = False):
        profile = profile or self.active()
        if not profile: return
        def action():
            return self.provider(profile, background=automatic).read_configuration(
                profile.server_id, profile.server_root)
        def done(result):
            if not self.active() or self.active().id != profile.id:
                self.log(f"Configuration loaded for {profile.name}, but the active server changed before it could be displayed.")
                return
            if result.get("error"):
                if automatic:
                    self.log(f"Configuration was not auto-loaded for {profile.name}: {result['error']}")
                else:
                    QMessageBox.information(self, "Configuration unavailable", result["error"])
                return
            if not result.get("exists"):
                message = (f"Zanik is not installed on {profile.name}; there is no server configuration to load yet. "
                           "Install Zanik first, then use the Configuration tab to change its settings.")
                self.reset_config_editor()
                if automatic: self.log(message)
                else: QMessageBox.information(self, "Zanik is not installed", message)
                return
            data = result["data"]
            try:
                values = self.config_model().parse(data.decode())
            except Exception as error:
                if automatic:
                    self.log(f"Configuration from {profile.name} could not be displayed: {error}")
                    return
                QMessageBox.critical(self, "Invalid server configuration", str(error)); return
            self.current_config_hash = content_hash(data); self.current_config_profile_id = profile.id
            self.config_text.setPlainText(data.decode())
            for key,value in values.items():
                field=self.config_fields[key]
                if isinstance(field,QCheckBox): field.setChecked(bool(value))
                elif isinstance(field,QSpinBox): field.setValue(int(value))
                else: field.setText(str(value))
            self.log(f"Loaded configuration from {profile.name}.")
        if automatic:
            def background_done(result, error):
                if error:
                    self.log(f"Configuration was not auto-loaded for {profile.name}: {error}")
                else:
                    done(result)
            self.run_background(f"config-load-{profile.id}", action, background_done)
        else:
            self.run_async(action, done, title=f"Loading configuration from {profile.name}…")

    def save_config(self):
        profile = self.active()
        if not profile or not self.current_config_hash or self.current_config_profile_id != profile.id:
            QMessageBox.information(self, "Load configuration first",
                                    "Load the selected server's configuration before saving changes.")
            return
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
            self.current_config_profile_id = profile.id
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
    update_handshake = "--update-handshake" in sys.argv
    application = QApplication(sys.argv); application.setApplicationName("Zanik Manager")
    window = MainWindow(prompt_first_run=not smoke_test and not update_handshake); window.show()
    if update_handshake:
        try:
            handshake = Path(sys.argv[sys.argv.index("--update-handshake") + 1])
            QTimer.singleShot(500, lambda: (write_startup_handshake(handshake, window.manager_version),
                                            prune_updater_backups(distribution_root())))
        except (IndexError, ValueError):
            pass
    if smoke_test:
        QTimer.singleShot(1000 if update_handshake else 250, application.quit)
    return application.exec()


if __name__ == "__main__":
    raise SystemExit(main())
