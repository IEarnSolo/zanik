#!/usr/bin/env python3
import json
import hashlib
import subprocess
import sys
import tempfile
import unittest
import zipfile
from datetime import datetime, timezone
from pathlib import Path, PurePosixPath
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from manager.zanik_manager.assets import (AssetDeployer, AssetExtractor, asset_contract_presentation, asset_setup_mode,
                                          background_creation_flags,
                                          cleanup_extraction_output, compact_completed_asset_work,
                                          dragonwilds_is_running, expected_asset_contract,
                                          game_root_id, request_dragonwilds_close,
                                          resume_activity_message)
from manager.zanik_manager.activity import format_activity_entry
from manager.zanik_manager.cache import BundleCache, format_bundle_timestamp
from manager.zanik_manager.changelog import parse_changelog
from manager.zanik_manager.config import (ConfigError, ConfigModel, content_hash, update_website_settings,
                                          website_configuration_ready)
from manager.zanik_manager.doctor_view import doctor_presentation, render_doctor_html
from manager.zanik_manager.doctor_cache import delete_doctor_cache, load_doctor_cache, save_doctor_cache
from manager.zanik_manager.files import (FileEntry, FileIndex, RemoteFileIndexer, calculate_directory_totals,
                                         delete_file_index, file_type, format_modified, format_size,
                                         load_file_index, normalize_entry, save_file_index, virtual_roots)
from manager.zanik_manager.journal import JournalStore, OperationJournal
from manager.zanik_manager.lifecycle import (PterodactylProvider, ReleasePayload, is_exact_legacy_launcher,
                                             safe_archive_name, validate_server_root)
from manager.zanik_manager.onboarding import discover_panel, is_remote_http
from manager.zanik_manager.profiles import (ProfileStore, ServerProfile, is_local_origin, move_profile,
                                             normalize_origin, remove_profile, rename_profile)
from manager.zanik_manager.preferences import PreferenceStore, preferred_profile_index
from manager.zanik_manager.payload_contract import compatibility_presentation, compatibility_summary, contract_from_entries
from manager.zanik_manager.pterodactyl import PterodactylClient, PterodactylError, Response, api_file_path
from manager.zanik_manager.security import credential_target, redact
from manager.zanik_manager.support import create_support_bundle
from tools.prune_pyside_runtime import KEEP_FILES, prune


class ChangelogTests(unittest.TestCase):
    def test_changelog_uses_compact_borderless_disclosure_rows(self):
        source = (ROOT / "manager" / "zanik_manager" / "app.py").read_text(encoding="utf-8")
        self.assertIn("self.toggle.setFlat(True)", source)
        self.assertIn("border: none; border-bottom", source)
        self.assertIn("text-align: left", source)
        self.assertIn("'▾' if expanded else '›'", source)
        self.assertNotIn("self.toggle.setArrowType", source)
        self.assertIn("setContentsMargins(10, 4, 8, 10)", source)

    def test_changelog_is_sorted_and_hides_versions_newer_than_manager(self):
        markdown = """# Changelog
## [2.0.0] - 2030-01-01
- Future
## [1.2.0] - 2026-10-17
### Added
- Current
## [1.1.9] - 2026-10-01
- Previous
"""
        releases = parse_changelog(markdown, "1.2.0")
        self.assertEqual([release.version for release in releases], ["1.2.0", "1.1.9"])
        self.assertEqual(releases[0].date, "2026-10-17")
        self.assertIn("Current", releases[0].changes)

    def test_activity_entries_timestamp_every_displayed_line(self):
        stamp = datetime(2026, 10, 17, 15, 4, 5, tzinfo=timezone.utc)
        self.assertEqual(format_activity_entry("First\nSecond", stamp),
                         "[2026-10-17 15:04:05] First\n[2026-10-17 15:04:05] Second")


def write_payload_archive(archive: Path, files: dict[str, bytes], version: str = "0.2.0") -> dict:
    with zipfile.ZipFile(archive, "w") as package:
        for name, data in files.items():
            package.writestr(name, data)
    with zipfile.ZipFile(archive) as package:
        entries = [{"path": item.filename, "size": item.file_size,
                    "sha256": hashlib.sha256(package.read(item)).hexdigest(),
                    "mode": (item.external_attr >> 16) & 0o777 or 0o644}
                   for item in package.infolist() if not item.is_dir()]
    contract = contract_from_entries(entries, version)
    with zipfile.ZipFile(archive, "a") as package:
        package.writestr("payload-contract.json", json.dumps(contract))
    return contract

class ManagerTests(unittest.TestCase):
    def test_configuration_controls_are_locked_for_load_and_save(self):
        source = (ROOT / "manager" / "zanik_manager" / "app.py").read_text(encoding="utf-8")
        self.assertIn("self.config_editors.setEnabled(bool(profile) and not pending)", source)
        self.assertIn("self.config_load_button.setEnabled(bool(profile) and not pending)", source)
        self.assertIn("self.config_pending_profile_id != profile_id", source)
        self.assertIn('self.set_config_pending(profile.id, f"Loading settings from {profile.name}…")', source)
        self.assertIn('self.set_config_pending(profile.id, f"Saving settings to {profile.name}…")', source)
        self.assertGreaterEqual(source.count("settled=lambda: self.clear_config_pending(profile.id)"), 2)

    def test_doctor_human_view_uses_raw_result_without_hashes(self):
        raw = {"healthy": True,
               "installation": {"version": "0.2.45", "mode": "wrapper", "officialSha256": "a" * 64},
               "launcher": {"applicable": False},
               "autoUpdate": {"available": True, "healthy": True, "value": "1"},
               "checks": {"library": True, "analyzer": True, "launcher": True, "official": True,
                          "configSha256": "b" * 64},
               "assets": {"installed": True, "compatible": True, "gameBuild": "123"},
               "gameUpdate": {"status": "current", "updateAvailable": False},
               "storageAndBackups": {"local": {"validatedBundles": {"bytes": 1024}},
                                     "remote": {"status": "healthy", "families": {"payload": [{}]}}},
               "managerDoctor": {"mode": "deep", "runAt": "now"}}
        presentation = doctor_presentation(raw)
        rendered = render_doctor_html(raw)
        self.assertEqual(presentation["overall"], "Healthy")
        self.assertEqual(presentation["mode"], "Deep")
        self.assertIn("Installation and payload", rendered)
        self.assertNotIn("a" * 64, rendered)
        self.assertNotIn("b" * 64, rendered)

    def test_doctor_human_view_handles_missing_and_degraded_results(self):
        self.assertEqual(doctor_presentation({})["overall"], "Unavailable")
        missing = doctor_presentation({"healthy": False, "checks": {"installation": "missing"},
                                       "managerDoctor": {"mode": "quick"}})
        self.assertEqual(missing["overall"], "Unavailable")
        degraded = doctor_presentation({"healthy": False, "installation": {"version": "1", "mode": "wrapper"},
                                        "checks": {}, "assets": {"installed": False}})
        self.assertEqual(degraded["overall"], "Attention required")
        self.assertTrue(any(section["title"] == "Website assets" for section in degraded["sections"]))

    def test_doctor_pending_and_current_game_are_unambiguous(self):
        pending = doctor_presentation({"doctorPending": True})
        self.assertEqual(pending["overall"], "Checking…")
        self.assertEqual(pending["overallState"], "neutral")
        current = doctor_presentation({
            "healthy": True,
            "installation": {"version": "1", "mode": "wrapper"},
            "gameUpdate": {"status": "current", "updateAvailable": False},
        })
        game = next(section for section in current["sections"]
                    if section["title"] == "Dragonwilds update state")
        self.assertEqual(game["status"], "healthy")
        self.assertIn("current and up to date", game["summary"])

    def test_doctor_separates_health_from_server_and_asset_freshness(self):
        presentation = doctor_presentation({
            "healthy": True,
            "installation": {"version": "0.2.42", "mode": "replace-executable"},
            "assets": {"installed": True, "compatible": True, "gameBuild": "25812671"},
            "managerFreshness": {
                "serverPayload": {"status": "update-available", "managerVersion": "0.2.47"},
                "websiteAssets": {"status": "out-of-date", "managerVersion": "0.2.47"},
            },
        })
        self.assertEqual(presentation["overall"], "Healthy")
        installation = next(section for section in presentation["sections"]
                            if section["title"] == "Installation and payload")
        assets = next(section for section in presentation["sections"]
                      if section["title"] == "Website assets")
        self.assertEqual(installation["status"], "attention")
        self.assertIn("operating normally", installation["summary"])
        self.assertIn("out of date with Manager 0.2.47", installation["summary"])
        self.assertEqual(assets["status"], "attention")
        self.assertIn("healthy and compatible", assets["summary"])
        self.assertIn("out of date with Manager 0.2.47", assets["summary"])

    def test_doctor_cache_is_profile_scoped_and_bom_tolerant(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            result = {"healthy": True, "managerDoctor": {"mode": "quick", "runAt": "now"}}
            save_doctor_cache(root, "profile-a", result)
            self.assertEqual(load_doctor_cache(root, "profile-a"), result)
            self.assertIsNone(load_doctor_cache(root, "profile-b"))
            cache_file = next(root.glob("*.json"))
            cache_file.write_bytes(b"\xef\xbb\xbf" + cache_file.read_bytes())
            self.assertEqual(load_doctor_cache(root, "profile-a"), result)
            delete_doctor_cache(root, "profile-a")
            self.assertIsNone(load_doctor_cache(root, "profile-a"))

    def test_file_roots_are_zanik_scoped_and_launcher_requires_verified_replacement(self):
        roots = virtual_roots("/home/container")
        self.assertEqual([root.path for root in roots], [
            "/home/container/zanik", "/home/container/zanik-data", "/home/container/zanik-backups",
            "/home/container/.zanik-manager", "/home/container/zanik-release",
            "/home/container/zanik-assets-incoming"])
        state = {"installed": True, "mode": "replace-executable", "repairRequired": False,
                 "installation": {"officialExecutable":
                                  "/home/container/RSDragonwilds/Binaries/Linux/RSDragonwildsServer-Linux-Shipping.zanik-original"}}
        replacement = virtual_roots("/home/container", state)
        self.assertEqual(replacement[-1].key, "launcher")
        self.assertNotIn(".zanik-original", replacement[-1].path)
        unsafe = {**state, "repairRequired": True}
        self.assertNotIn("launcher", [root.key for root in virtual_roots("/home/container", unsafe)])

    def test_file_metadata_normalization_excludes_links_original_and_traversal(self):
        self.assertIsNone(normalize_entry("/home/container/zanik", {"name": "../save", "is_file": True}))
        self.assertIsNone(normalize_entry("/home/container/zanik", {"name": "secret.zanik-original", "is_file": True}))
        self.assertIsNone(normalize_entry("/home/container/zanik", {"name": "linked", "is_file": False,
                                                                     "is_symlink": True}))
        entry = normalize_entry("/home/container/zanik", {"name": "VERSION", "is_file": True,
                                                           "size": 7, "modified_at": "2026-10-19T12:00:00Z"})
        self.assertEqual(entry.path, "/home/container/zanik/VERSION")
        self.assertEqual(format_size(entry.size), "7 B")
        self.assertIn("2026-10-19", format_modified(entry.modified_at))
        self.assertEqual(file_type(FileEntry("libzanik.so", "/x/libzanik.so", True, 1, "")),
                         "Linux shared library")

    def test_file_index_is_metadata_only_and_totals_all_roots(self):
        calls = []
        listings = {
            "/home/container/zanik": [
                {"name": "bin", "is_file": False, "size": 0},
                {"name": "VERSION", "is_file": True, "size": 7}],
            "/home/container/zanik/bin": [{"name": "zanik-server", "is_file": True, "size": 100}],
        }
        class Client:
            def list_files(self, _server, path):
                calls.append(path)
                if path in listings: return listings[path]
                raise RuntimeError("missing")
            def read_file(self, *_args): raise AssertionError("file contents must not be read")
            def download_file(self, *_args): raise AssertionError("files must not be downloaded")
        roots = virtual_roots("/home/container")
        index = RemoteFileIndexer(Client(), "dev", "profile", 1, roots).run()
        self.assertTrue(index.complete)
        self.assertEqual(index.file_count, 2)
        self.assertEqual(index.total_size, 107)
        self.assertEqual(index.root_totals["installation"], 107)
        self.assertIn("/home/container/zanik/bin", calls)
        self.assertFalse(any("save" in path.lower() for path in calls))

    def test_file_index_limits_are_partial_and_completed_cache_does_not_expire(self):
        class Client:
            def list_files(self, _server, path):
                return [{"name": f"file-{index}.txt", "is_file": True, "size": 1} for index in range(5)]
        root = virtual_roots("/home/container")[:1]
        index = RemoteFileIndexer(Client(), "dev", "profile", 2, root, max_entries=3).run()
        self.assertTrue(index.partial); self.assertFalse(index.complete)
        self.assertEqual(index.file_count, 3)
        fresh = FileIndex("profile", 1, root, complete=True, completed_at=100.0)
        self.assertTrue(fresh.fresh(100.0 + 365 * 24 * 60 * 60))

    def test_file_index_persists_per_profile_until_explicit_invalidation(self):
        roots = virtual_roots("/home/container")
        entry = FileEntry("VERSION", "/home/container/zanik/VERSION", True, 7,
                          "2026-10-19T12:00:00Z")
        index = FileIndex("stable-profile", 2, roots,
                          directories={"/home/container/zanik": [entry]},
                          directory_totals={"/home/container/zanik": 7},
                          root_totals={root.key: (7 if root.key == "installation" else 0) for root in roots},
                          file_count=1, directory_count=1, total_size=7, complete=True,
                          completed_at=1_799_000_000.0)
        with tempfile.TemporaryDirectory() as temporary:
            cache_root = Path(temporary)
            save_file_index(cache_root, index)
            loaded = load_file_index(cache_root, "stable-profile", "/home/container", 9)
            self.assertIsNotNone(loaded)
            self.assertEqual(loaded.generation, 9)
            self.assertEqual(loaded.directories["/home/container/zanik"][0], entry)
            self.assertEqual(loaded.total_size, 7)
            self.assertTrue(loaded.fresh(99_999_999_999.0))
            self.assertIsNone(load_file_index(cache_root, "other-profile", "/home/container", 1))
            delete_file_index(cache_root, "stable-profile")
            self.assertIsNone(load_file_index(cache_root, "stable-profile", "/home/container", 10))

    def test_folder_sizes_are_recursive_without_content_reads(self):
        directories = {
            "/home/container/zanik": [FileEntry("bin", "/home/container/zanik/bin", False, 0, ""),
                                      FileEntry("VERSION", "/home/container/zanik/VERSION", True, 7, "")],
            "/home/container/zanik/bin": [FileEntry("nested", "/home/container/zanik/bin/nested", False, 0, ""),
                                          FileEntry("launcher", "/home/container/zanik/bin/launcher", True, 100, "")],
            "/home/container/zanik/bin/nested": [FileEntry("data", "/home/container/zanik/bin/nested/data", True, 20, "")],
        }
        totals = calculate_directory_totals(directories)
        self.assertEqual(totals["/home/container/zanik/bin/nested"], 20)
        self.assertEqual(totals["/home/container/zanik/bin"], 120)
        self.assertEqual(totals["/home/container/zanik"], 127)

    def test_manager_doctor_and_files_ui_contract(self):
        source = (ROOT / "manager" / "zanik_manager" / "app.py").read_text(encoding="utf-8")
        self.assertIn('self.doctor_views.addTab(self.doctor_human, "Human-readable")', source)
        self.assertIn('self.doctor_views.addTab(self.doctor_raw, "Raw diagnostics")', source)
        self.assertIn('self.tabs.addTab(page, "Files")', source)
        self.assertIn("diagnostics = self.doctor_result", source)
        self.assertNotIn("self.doctor_output", source)
        self.assertIn('QCheckBox("Show raw server paths")', source)
        self.assertIn("QStyle.SP_DirIcon", source)
        self.assertIn("QStyle.SP_FileIcon", source)
        self.assertIn("Qt.BackButton", source)
        self.assertIn("Qt.ForwardButton", source)
        self.assertIn('self.files_last_updated = QLabel("Last updated: Not yet indexed")', source)
        self.assertIn("button.setFixedSize(40, 40)", source)

    def test_journal_retention_keeps_unresolved_and_newest_completed(self):
        with tempfile.TemporaryDirectory() as temporary:
            store = JournalStore(Path(temporary))
            old = OperationJournal.create("profile", "install"); old.created_at = "2026-10-01T00:00:00+00:00"; old.state = "complete"; store.save(old)
            newest = OperationJournal.create("profile", "install"); newest.created_at = "2026-10-02T00:00:00+00:00"; newest.state = "complete"; store.save(newest)
            failed = OperationJournal.create("profile", "game-update"); failed.state = "failed"; store.save(failed)
            rolled = OperationJournal.create("profile", "install"); rolled.state = "rolled-back"; store.save(rolled)
            result = store.prune()
            self.assertEqual(result["removed"], 2)
            self.assertTrue((store.root / f"{newest.operation_id}.json").is_file())
            self.assertTrue((store.root / f"{failed.operation_id}.json").is_file())

    def test_remote_backup_retention_protects_failures_and_requires_explicit_legacy_cleanup(self):
        parent = "/home/container/zanik-backups/manager"
        names = ["20261001T000000Z-aaaaaaaa", "20261002T000000Z-bbbbbbbb",
                 "20261003T000000Z-cccccccc", "20260901T000000Z-config",
                 "20260902T000000Z-config"]
        manifests = {
            names[0]: {"schemaVersion": 1, "owner": "Zanik Manager", "family": "payload",
                       "operationId": "old", "createdAt": "2026-10-01T00:00:00Z", "state": "complete"},
            names[1]: {"schemaVersion": 1, "owner": "Zanik Manager", "family": "payload",
                       "operationId": "new", "createdAt": "2026-10-02T00:00:00Z", "state": "complete"},
            names[2]: {"schemaVersion": 1, "owner": "Zanik Manager", "family": "payload",
                       "operationId": "failed", "createdAt": "2026-10-03T00:00:00Z", "state": "failed"},
        }
        class Client:
            def __init__(self): self.deleted=[]
            def list_files(self, _server, path):
                if path == parent: return [{"name": name, "is_file": False} for name in names]
                raise PterodactylError("missing", 404)
            def read_file(self, _server, path):
                name = PurePosixPath(path).parent.name
                if name in manifests: return json.dumps(manifests[name]).encode()
                raise PterodactylError("missing", 404)
            def delete(self, _server, root, files): self.deleted.extend(f"{root}/{name}" for name in files)
        client=Client()
        with tempfile.TemporaryDirectory() as temporary:
            provider=PterodactylProvider(client, JournalStore(Path(temporary)))
            report=provider.backup_storage_status("dev", "/home/container")
            automatic=provider._cleanup_candidates(report)
            manual=provider._cleanup_candidates(report, include_legacy=True)
        self.assertEqual([item["path"] for item in automatic], [f"{parent}/{names[0]}"])
        self.assertIn(f"{parent}/{names[3]}", [item["path"] for item in manual])
        self.assertNotIn(f"{parent}/{names[2]}", [item["path"] for item in manual])

    def test_game_updater_uses_one_manager_backup_and_one_old_executable(self):
        source = (ROOT / "scripts" / "update-game.sh").read_text(encoding="utf-8")
        self.assertIn("game-update-request.json", source)
        self.assertNotIn("official-before-update", source)
        self.assertEqual(source.count('mv -f "$original" "$backup_root/official-previous"'), 1)
        self.assertNotIn("zanik-backups/game-update/$(date", source)

    def test_completed_asset_work_compaction_preserves_incremental_inputs(self):
        with tempfile.TemporaryDirectory() as temporary:
            work = Path(temporary); pipeline = work / "pipeline"
            for name in ("json", "textures", "font", "web", "internal"):
                (pipeline / name).mkdir(parents=True)
                (pipeline / name / "data.bin").write_bytes(b"data")
            (work / "generated-web").mkdir(); (work / "generated-web" / "asset.bin").write_bytes(b"asset")
            (pipeline / "export.log").write_text("log")
            (pipeline / "export-checkpoint.json").write_text("{}")
            (work / "Mappings.usmap").write_bytes(b"mapping")
            (work / ".zanik-asset-work.json").write_text(json.dumps({"owner": "Zanik asset extractor"}))
            (work / "extraction-journal.json").write_text(json.dumps({"schemaVersion": 1, "stage": "complete"}))
            self.assertGreater(compact_completed_asset_work(work), 0)
            for name in ("json", "textures", "font"):
                self.assertTrue((pipeline / name / "data.bin").is_file())
            self.assertTrue((pipeline / "export-checkpoint.json").is_file())
            self.assertTrue((work / "Mappings.usmap").is_file())
            self.assertFalse((pipeline / "web").exists())
            self.assertFalse((pipeline / "internal").exists())
            self.assertTrue((work / "generated-web" / "asset.bin").is_file())
            self.assertFalse((pipeline / "export.log").exists())

    def test_extraction_output_cleanup_removes_only_manager_bundle_products(self):
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary)
            (output / "zanik-assets-123.zip").write_bytes(b"zip")
            parts = output / "zanik-assets-123-parts"; parts.mkdir(); (parts / "part.zip").write_bytes(b"part")
            (output / "operator-note.txt").write_text("keep")
            self.assertEqual(cleanup_extraction_output(output), 7)
            self.assertFalse((output / "zanik-assets-123.zip").exists())
            self.assertFalse(parts.exists())
            self.assertTrue((output / "operator-note.txt").is_file())

    def test_server_profile_rename_reorder_and_delete_are_local_and_deterministic(self):
        first = ServerProfile.create(name="First", panel_origin="https://panel.example", account_id="1",
                                     credential_target="one", server_id="aaa")
        second = ServerProfile.create(name="Second", panel_origin="https://panel.example", account_id="1",
                                      credential_target="one", server_id="bbb")
        profiles = rename_profile([first, second], second.id, "  Main Server  ")
        self.assertEqual([profile.name for profile in profiles], ["First", "Main Server"])
        profiles = move_profile(profiles, second.id, -1)
        self.assertEqual([profile.id for profile in profiles], [second.id, first.id])
        profiles = remove_profile(profiles, first.id)
        self.assertEqual([profile.id for profile in profiles], [second.id])
        self.assertEqual(first.name, "First")
        with self.assertRaises(ValueError):
            rename_profile(profiles, second.id, "  ")

    def test_manager_server_information_and_profile_editor_contract(self):
        source = (ROOT / "manager" / "zanik_manager" / "app.py").read_text(encoding="utf-8")
        self.assertIn('QPushButton("Edit servers")', source)
        self.assertIn('QPushButton("Open website")', source)
        self.assertIn("class EditServersDialog", source)
        self.assertIn("Removing a profile only removes it", source)
        self.assertIn('f"Website: {url}"', source)
        self.assertIn('f"Website port: {website.get(\'port\')}"', source)
        self.assertIn("website_configuration_ready", source)

    def test_guided_configuration_descriptions_use_dedicated_wrapping_labels(self):
        source = (ROOT / "manager" / "zanik_manager" / "app.py").read_text(encoding="utf-8")
        schema = json.loads((ROOT / "config" / "zanik-config.schema.json").read_text(encoding="utf-8"))
        teleport_description = schema["properties"]["FEATURE_ADMIN_TELEPORT"]["description"]
        self.assertIn("player-built lodestone", teleport_description)
        self.assertIn("fixed nodestone", teleport_description)
        self.assertIn("form.setFieldGrowthPolicy(QFormLayout.AllNonFixedFieldsGrow)", source)
        self.assertIn("form.setRowWrapPolicy(QFormLayout.WrapLongRows)", source)
        self.assertIn('field=QCheckBox(); field.setChecked(bool(spec["default"]))', source)
        self.assertIn("def config_description(text: str, dangerous: bool = False)", source)
        self.assertIn("class WrappingLabel(QLabel)", source)
        self.assertIn("self.setWordWrap(True)", source)
        self.assertIn("self.setSizePolicy(QSizePolicy.Ignored, QSizePolicy.Fixed)", source)
        self.assertIn("self.setFixedHeight(height)", source)
        self.assertNotIn("description.setMaximumWidth(420)", source)
        self.assertIn('field.setFixedWidth(20)', source)
        self.assertIn("field_layout.addWidget(field,0,Qt.AlignTop); field_layout.addWidget(description,1,Qt.AlignTop)", source)
        self.assertIn('if key=="STATUS_PORT": field.setFixedWidth(150)', source)
        self.assertIn('if key=="PUBLIC_BASE_URL": field.setFixedWidth(380)', source)
        self.assertIn('elif key=="ADMIN_COOKIE_NAME": field.setFixedWidth(240)', source)
        self.assertIn("field_row.addWidget(field); field_row.addStretch(1)", source)
        self.assertIn("form.addRow(spec[\"title\"],field_layout)", source)
        self.assertIn("scroll.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)", source)
        self.assertNotIn('QCheckBox(spec["description"])', source)

    def test_server_payload_compatibility_presentation_uses_shared_status_states_and_tooltips(self):
        current = compatibility_presentation(
            {"version": "0.2.15", "payloadCompatibility": "current", "payloadContractSha256": "a" * 64},
            "0.2.24")
        self.assertEqual(current["state"], "current")
        self.assertIn("Up to date with Manager 0.2.24", current["text"])
        self.assertIn("older version number", current["tooltip"])
        self.assertIn("No server update is required", current["tooltip"])

        available = compatibility_presentation(
            {"version": "0.2.15", "payloadCompatibility": "update-available"}, "0.2.24")
        self.assertEqual(available["state"], "available")
        self.assertIn("changed server-side Zanik files", available["tooltip"])
        self.assertIn("Update / repair Zanik", available["tooltip"])

        newer_manager = compatibility_presentation(
            {"version": "0.2.15", "payloadCompatibility": "current", "payloadContractSha256": "a" * 64},
            "0.2.24", {"updateAvailable": True, "latestVersion": "0.2.25",
                       "serverPayload": {"contractSha256": "b" * 64}})
        self.assertEqual(newer_manager["state"], "available")
        self.assertIn("Update Manager to 0.2.25", newer_manager["text"])

        source = (ROOT / "manager" / "zanik_manager" / "app.py").read_text(encoding="utf-8")
        update_at = source.index('QPushButton("Update / repair Zanik")')
        panel_at = source.index('QPushButton("Open panel")')
        website_at = source.index('QPushButton("Open website")')
        recover_at = source.index('QPushButton("Recover interrupted")')
        self.assertLess(update_at, panel_at)
        self.assertLess(panel_at, website_at)
        self.assertLess(website_at, recover_at)
        self.assertIn('colors = {"current": "#2f855a", "available": "#b7791f"', source)
        self.assertIn('QToolTip {{ color:#ffffff; }}', source)
        self.assertIn('self.server_summary.setText(profile.name)', source)
        self.assertIn('f"Panel: {profile.panel_origin}', source)

    def test_asset_contract_presentation_distinguishes_refresh_from_server_update(self):
        active = {"installed": True, "valid": True, "extractionRecipeVersion": 5,
                  "compatibilityProfileVersion": "reviewed-5", "extractionRecipeSha256": "a" * 64}
        current = asset_contract_presentation(active, "0.2.36")
        self.assertEqual(current["state"], "current")
        self.assertIn("Up to date with Manager 0.2.36", current["text"])
        latest = {"updateAvailable": True, "latestVersion": "0.2.37", "assetContract": {
            "extractionRecipeVersion": 6, "compatibilityProfileVersion": "reviewed-6",
            "extractionRecipeSha256": "b" * 64}}
        refresh = asset_contract_presentation(active, "0.2.36", latest, {
            "extractionRecipeVersion": 5, "compatibilityProfileVersion": "reviewed-5",
            "extractionRecipeSha256": "a" * 64})
        self.assertEqual(refresh["state"], "available")
        self.assertIn("Update Manager to 0.2.37, then refresh assets", refresh["text"])
        self.assertIn("updating the Zanik server payload is not required", refresh["tooltip"])
        unchanged_latest = {**latest, "assetContract": {
            "extractionRecipeVersion": 5, "compatibilityProfileVersion": "reviewed-5",
            "extractionRecipeSha256": "a" * 64}}
        missing = asset_contract_presentation({"installed": False, "valid": False}, "0.2.36",
                                              unchanged_latest, unchanged_latest["assetContract"])
        self.assertEqual(missing["text"], "Website assets · Missing")
        embedded_stale = asset_contract_presentation({**active, "valid": False}, "0.2.37")
        self.assertIn("Refresh available in Manager 0.2.37", embedded_stale["text"])

        source = (ROOT / "manager" / "zanik_manager" / "app.py").read_text(encoding="utf-8")
        self.assertIn("self.server_assets = QLabel()", source)
        self.assertIn("def check_asset_contract_state", source)
        self.assertIn("self.check_asset_contract_state(profile)", source)

    def test_server_payload_contract_ignores_release_docs_but_tracks_operational_files(self):
        base = [{"path": "bin/libzanik.so", "size": 3, "sha256": "a" * 64, "mode": 0o755},
                {"path": "scripts/install.sh", "size": 4, "sha256": "b" * 64, "mode": 0o755},
                {"path": "README.md", "size": 5, "sha256": "c" * 64, "mode": 0o644}]
        original = contract_from_entries(base, "0.2.21")
        manager_only = contract_from_entries([{**item, "sha256": "d" * 64} if item["path"] == "README.md" else item
                                              for item in base], "0.2.22")
        server_change = contract_from_entries([{**item, "sha256": "e" * 64} if item["path"] == "scripts/install.sh" else item
                                               for item in base], "0.2.22")
        self.assertEqual(original["contractSha256"], manager_only["contractSha256"])
        self.assertNotEqual(original["contractSha256"], server_change["contractSha256"])

    def test_update_controls_are_distinct_and_non_modal_by_default(self):
        source = (ROOT / "manager" / "zanik_manager" / "app.py").read_text(encoding="utf-8")
        self.assertIn('QPushButton("Install Zanik")', source)
        self.assertIn('QPushButton("Update / repair Zanik")', source)
        self.assertIn("def update_or_repair", source)
        self.assertIn('QPushButton("Update and relaunch")', source)
        self.assertIn('box.addButton("Update Manager first", QMessageBox.AcceptRole)', source)
        self.assertIn('box.addButton("Repair current version", QMessageBox.ActionRole)', source)
        self.assertIn("newer_server_payload_available(self.manager_release_status, embedded_contract)", source)
        self.assertIn('QPushButton("Check")', source)
        self.assertIn('run_background("manager-update-check"', source)
        self.assertIn('run_background("game-update-check"', source)
        self.assertIn('self.update_timer.setInterval(10 * 60 * 1000)', source)
        self.assertIn('QTimer.singleShot(250, lambda profile_id=profile.id: self.load_startup_config(profile_id))', source)
        self.assertIn('prompt_first_run=not smoke_test and not update_handshake', source)

    def test_origin_and_http_policy_helpers(self):
        self.assertEqual(normalize_origin("EXAMPLE.com/"), "https://example.com")
        self.assertEqual(normalize_origin("http://localhost:8080/"), "http://localhost:8080")
        self.assertTrue(is_local_origin("http://127.0.0.1"))
        self.assertFalse(is_local_origin("http://panel.example.com"))
        self.assertTrue(is_remote_http("http://panel.example.com"))
        self.assertFalse(is_remote_http("https://panel.example.com"))
        self.assertFalse(is_remote_http("http://localhost:8080"))
        with self.assertRaises(ValueError): normalize_origin("ftp://example.com")

    def test_panel_discovery_warns_before_remote_http_credentials_are_sent(self):
        events = []
        class Client:
            def __init__(self, origin, token): events.append(("client", origin, token))
            def account(self): return {"id": 7}
            def servers(self): return [{"identifier": "dev"}]
        cancelled = discover_panel("http://panel.example.com", "secret",
                                   lambda: events.append(("warning",)) or False, Client)
        self.assertIsNone(cancelled)
        self.assertEqual(events, [("warning",)])
        connected = discover_panel("http://panel.example.com", "secret",
                                   lambda: events.append(("warning",)) or True, Client)
        self.assertTrue(connected.allow_insecure_http)
        self.assertEqual(events[-2][0], "warning")
        self.assertEqual(events[-1][0], "client")

    def test_https_and_local_http_discovery_do_not_warn(self):
        class Client:
            def __init__(self, origin, token): self.origin = origin
            def account(self): return {"id": 1}
            def servers(self): return [{"identifier": "dev"}]
        def unexpected(): raise AssertionError("secure and loopback origins must not warn")
        self.assertFalse(discover_panel("https://panel.example.com", "secret", unexpected, Client).allow_insecure_http)
        self.assertFalse(discover_panel("http://127.0.0.1:8080", "secret", unexpected, Client).allow_insecure_http)

    def test_manager_dialog_contract_hides_checkbox_and_invalidates_edits(self):
        source = (ROOT / "manager" / "zanik_manager" / "app.py").read_text(encoding="utf-8")
        self.assertNotIn("self.http_ack", source)
        self.assertIn("Continue with HTTP", source)
        self.assertIn("self.origin.textChanged.connect(self.invalidate_connection)", source)
        self.assertIn("self.token.textChanged.connect(self.invalidate_connection)", source)
        self.assertIn("This panel connection uses HTTP; API traffic is not encrypted.", source)

    def test_manager_long_operations_are_modal_and_report_asset_progress(self):
        source = (ROOT / "manager" / "zanik_manager" / "app.py").read_text(encoding="utf-8")
        self.assertIn("class OperationProgressDialog(QDialog)", source)
        self.assertIn("self.setFixedSize(660, 260)", source)
        self.assertIn("self.task = QProgressBar()", source)
        self.assertIn("Current step active for", source)
        self.assertIn("self.setWindowFlag(Qt.WindowCloseButtonHint, False)", source)
        self.assertIn("Qt.ApplicationModal", source)
        self.assertIn("Wait for the current operation to finish", source)
        self.assertIn('phase_percent = {"preserve": 10, "mappings": 15, "restore": 30,', source)
        self.assertIn("Close game and continue", source)
        self.assertIn("Set up website assets now", source)
        self.assertIn("Asset extraction can take several minutes", source)
        self.assertIn("Configure the Zanik website", source)
        self.assertIn("Skip for now", source)
        self.assertIn("not the Dragonwilds game port", source)
        self.assertIn("self.asset_target_box", source)
        self.assertIn("Confirm asset target", source)
        self.assertIn("def provider(self, profile:", source)
        self.assertIn("Discard stale work and start fresh", source)
        self.assertIn("resume_compatibility(work, root)", source)
        self.assertIn("Upload and activate now", source)
        self.assertIn("verify_launcher_after_restart", source)
        self.assertIn("Repair launcher now", source)
        self.assertIn("load_startup_config", source)
        self.assertIn("format_bundle_timestamp", source)
        self.assertIn("button_help", source)

    def test_dragonwilds_process_preflight(self):
        class Result:
            stdout = "1\n"
        calls = []
        def running(command, **kwargs):
            calls.append((command, kwargs)); return Result()
        self.assertTrue(dragonwilds_is_running(running, "nt"))
        self.assertEqual(calls[0][0][0], "powershell.exe")
        self.assertEqual(calls[0][1]["creationflags"], background_creation_flags("nt"))
        self.assertFalse(dragonwilds_is_running(running, "posix"))

    def test_background_helpers_hide_their_windows_console(self):
        with mock.patch.object(subprocess, "CREATE_NO_WINDOW", 0x08000000, create=True):
            self.assertEqual(background_creation_flags("nt"), 0x08000000)
            self.assertEqual(background_creation_flags("posix"), 0)

    def test_graceful_close_never_terminates_process(self):
        calls = []
        class Result:
            stdout = "0\n"
        def runner(command, **kwargs):
            calls.append(command); return Result()
        self.assertTrue(request_dragonwilds_close(runner, timeout=1, platform="nt"))
        rendered = " ".join(calls[0])
        self.assertIn("CloseMainWindow", rendered)
        self.assertNotIn("Stop-Process", rendered)

    def test_manager_paths_include_persistent_mapping_recovery(self):
        from manager.zanik_manager.paths import ensure_layout
        with tempfile.TemporaryDirectory() as temporary:
            paths = ensure_layout(Path(temporary))
            self.assertEqual(paths["mapping_recovery"], Path(temporary) / "mapping-recovery")
            self.assertTrue(paths["mapping_recovery"].is_dir())
            self.assertEqual(paths["asset_work"], Path(temporary) / "asset-work")
            self.assertTrue(paths["asset_work"].is_dir())

    def test_asset_extraction_resume_requires_verified_journal_and_marker(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary); work = root / "work"
            extractor = AssetExtractor(root / "tools", BundleCache(root / "cache"))
            self.assertIsNone(extractor.resumable(work))
            work.mkdir()
            (work / "extraction-journal.json").write_text(json.dumps({
                "schemaVersion": 1, "stage": "generated", "updatedAt": "2026-10-13T00:00:00Z"
            }))
            self.assertIsNone(extractor.resumable(work))
            (work / ".zanik-asset-work.json").write_text(json.dumps({
                "schemaVersion": 1, "owner": "someone else"
            }))
            self.assertIsNone(extractor.resumable(work))
            (work / ".zanik-asset-work.json").write_text(json.dumps({
                "schemaVersion": 1, "owner": "Zanik asset extractor"
            }))
            self.assertEqual(extractor.resumable(work)["stage"], "generated")
            (work / ".zanik-asset-work.json").write_text(json.dumps({
                "schemaVersion": 1, "owner": "Zanik asset extractor"
            }), encoding="utf-8-sig")
            (work / "extraction-journal.json").write_text(json.dumps({
                "schemaVersion": 1, "stage": "mappings", "updatedAt": "2026-10-13T00:00:00Z"
            }), encoding="utf-8-sig")
            self.assertEqual(extractor.resumable(work)["stage"], "mappings")

    def test_incomplete_resume_rejects_game_updates_and_complete_cache_updates_incrementally(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary); tools = root / "asset-tools"; work = root / "work"
            game = root / "game"
            executable = game / "RSDragonwilds/Binaries/Win64/RSDragonwilds-Win64-Shipping.exe"
            executable.parent.mkdir(parents=True); executable.write_bytes(b"game-v1")
            (tools / "asset-tools").mkdir(parents=True)
            (tools / "asset-tools/extraction-recipes.json").write_bytes(b"recipe")
            (tools / "asset-tools/tools.lock.json").write_bytes(b"lock")
            extractor = AssetExtractor(tools, BundleCache(root / "cache"))
            work.mkdir(); (work / ".zanik-asset-work.json").write_text(json.dumps({
                "schemaVersion": 1, "owner": "Zanik asset extractor"
            }))
            fingerprints = extractor.current_fingerprints(game)
            (work / "extraction-journal.json").write_text(json.dumps({
                "schemaVersion": 1, "stage": "mappings", "fingerprints": fingerprints
            }))
            self.assertTrue(extractor.resume_compatibility(work, game)["compatible"])
            manifest = {"sourceExecutable": {"sha256": fingerprints["gameExecutableSha256"]}}
            bundle = root / "bundle.zip"
            with zipfile.ZipFile(bundle, "w") as archive:
                archive.writestr("asset-manifest.json", json.dumps(manifest))
            self.assertTrue(extractor.bundle_compatible_with_game(bundle, game))
            executable.write_bytes(b"game-v2")
            status = extractor.resume_compatibility(work, game)
            self.assertFalse(status["compatible"])
            self.assertEqual(status["mismatches"], ["gameExecutableSha256"])
            self.assertFalse(extractor.bundle_compatible_with_game(bundle, game))
            journal = json.loads((work / "extraction-journal.json").read_text())
            journal["stage"] = "complete"
            (work / "extraction-journal.json").write_text(json.dumps(journal))
            status = extractor.resume_compatibility(work, game)
            self.assertTrue(status["compatible"])
            self.assertTrue(status["incremental"])
            self.assertTrue(status["mappingRequired"])

            contract = {"extractionRecipeVersion": 5,
                        "compatibilityProfileVersion": "reviewed-5",
                        "extractionRecipeSha256": "a" * 64}
            manifest.update({"extractionRecipeVersion": 5,
                             "extractionRecipeSha256": "a" * 64,
                             "compatibilityProfile": {"version": "reviewed-5"}})
            with zipfile.ZipFile(bundle, "w") as archive:
                archive.writestr("asset-manifest.json", json.dumps(manifest))
            executable.write_bytes(b"game-v1")
            self.assertTrue(extractor.bundle_compatible_with_game(bundle, game, contract=contract))
            self.assertFalse(extractor.bundle_compatible_with_game(
                bundle, game, contract={**contract, "extractionRecipeVersion": 6}))
            self.assertFalse(extractor.bundle_compatible_with_game(
                bundle, game, contract={**contract, "extractionRecipeSha256": "b" * 64}))
            (tools / "asset-tools/tools.lock.json").write_text("changed-tools")
            status = extractor.resume_compatibility(work, game)
            self.assertFalse(status["compatible"])
            self.assertEqual(status["unsafeMismatches"], ["toolLockSha256"])

    def test_manager_exposes_resume_and_safe_discard_controls(self):
        source = (ROOT / "manager" / "zanik_manager" / "app.py").read_text(encoding="utf-8")
        self.assertIn("Resume extraction", source)
        self.assertIn("Discard generated work", source)
        self.assertIn("def resume_asset_extraction", source)
        self.assertIn("def discard_asset_extraction", source)
        self.assertIn('status.get("stage") == "created"', source)
        self.assertIn('self.resume_extract_button.setText("Update incrementally")', source)
        self.assertIn("incremental=incremental", source)
        self.assertIn("refresh_asset_controls", source)
        self.assertIn("preserve_existing_loader=preserve", source)
        assets_source = (ROOT / "manager" / "zanik_manager" / "assets.py").read_text(encoding="utf-8")
        self.assertIn('encoding="utf-8-sig"', assets_source)
        self.assertIn("UnrealAssetScout", assets_source)
        self.assertIn("UE4SS", assets_source)
        self.assertIn("game_build", assets_source)
        self.assertIn('command.append("-Incremental")', assets_source)
        self.assertIn('command += ["-SkipBundleSet", "-SkipPostBuildValidation"]', assets_source)
        self.assertIn("the full bundle was not uploaded", assets_source)
        self.assertNotIn("using the full-bundle fallback", assets_source)
        script_source = (ROOT / "scripts" / "init-assets.ps1").read_text(encoding="utf-8")
        self.assertIn("[switch]$Incremental", script_source)
        self.assertIn("only changed, added, or removed game packages", script_source)
        self.assertIn("preserveExistingLoader", script_source)
        self.assertNotIn("cleanup-work --work-root $Work", script_source)
        lifecycle_source = (ROOT / "manager" / "zanik_manager" / "lifecycle.py").read_text(encoding="utf-8")
        self.assertIn('"buildingIconSources": manifest.get("buildingIconSources", {})', lifecycle_source)
        self.assertIn('"recipeRouteCoverage": manifest.get("recipeRouteCoverage", {})', lifecycle_source)

    def test_guided_asset_setup_prefers_cached_then_resumable_work(self):
        status = {"stage": "mappings", "updatedAt": "2026-10-13T00:00:00Z"}
        self.assertEqual(asset_setup_mode(True, status), "cached")
        self.assertEqual(asset_setup_mode(False, status), "resume")
        self.assertEqual(asset_setup_mode(False, {**status, "incremental": True}), "incremental")
        self.assertEqual(asset_setup_mode(False, None), "new")
        source = (ROOT / "manager" / "zanik_manager" / "app.py").read_text(encoding="utf-8")
        self.assertIn("resume_status = None if use_cached else extractor.resume_compatibility(work, root)", source)
        self.assertIn('resume=(mode == "resume")', source)
        self.assertIn('incremental=(mode == "incremental")', source)
        self.assertIn('resume_activity_message(resume_status["stage"])', source)
        self.assertIn("Dragonwilds will not reopen", resume_activity_message("mappings"))
        self.assertIn("will continue with catalog and artwork export", resume_activity_message("mappings"))
        self.assertIn("completed UnrealAssetScout exports will not run again",
                      resume_activity_message("exports"))

    def test_manual_extraction_reuses_an_exact_cached_bundle_before_incremental_work(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary); tools = root / "tools"; game = root / "game"
            executable = game / "RSDragonwilds/Binaries/Win64/RSDragonwilds-Win64-Shipping.exe"
            executable.parent.mkdir(parents=True); executable.write_bytes(b"same-game")
            recipe_path = tools / "asset-tools/extraction-recipes.json"
            recipe_path.parent.mkdir(parents=True)
            recipe_path.write_text(json.dumps({"schemaVersion": 5,
                                               "catalogProfile": {"version": "reviewed-5"}}))
            (tools / "asset-tools/tools.lock.json").write_text("tools")
            contract = expected_asset_contract(tools)
            fingerprints = AssetExtractor(tools, BundleCache(root / "cache")).current_fingerprints(game)
            manifest = {"gameBuild": "123", "generatedAt": "2026-10-17T00:00:00Z",
                        "sourceExecutable": {"sha256": fingerprints["gameExecutableSha256"]},
                        "extractionRecipeVersion": contract["extractionRecipeVersion"],
                        "extractionRecipeSha256": contract["extractionRecipeSha256"],
                        "compatibilityProfile": {"version": contract["compatibilityProfileVersion"]}}
            bundle = root / "bundle.zip"
            with zipfile.ZipFile(bundle, "w") as archive:
                archive.writestr("asset-manifest.json", json.dumps(manifest))
            cache = BundleCache(root / "cache"); expected = cache.add(bundle, game_root_id(game), manifest)
            extractor = AssetExtractor(tools, cache)
            self.assertEqual(extractor.compatible_cached_bundles(game, contract), [expected])
            executable.write_bytes(b"changed-game")
            self.assertEqual(extractor.compatible_cached_bundles(game, contract), [])

        source = (ROOT / "manager" / "zanik_manager" / "app.py").read_text(encoding="utf-8")
        cache_check = source.index("cached = extractor.compatible_cached_bundles(root, contract)",
                                   source.index("def extract_assets"))
        incremental_check = source.index("resume_status = extractor.resume_compatibility(work, root)",
                                         source.index("def extract_assets"))
        self.assertLess(cache_check, incremental_check)
        self.assertIn("No extraction is required", source)
        self.assertIn("Run extraction anyway", source)

    def test_incremental_baseline_uses_retained_recipe_not_new_journal(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary); tools = root / "tools"; game = root / "game"; work = root / "work"
            executable = game / "RSDragonwilds/Binaries/Win64/RSDragonwilds-Win64-Shipping.exe"
            executable.parent.mkdir(parents=True); executable.write_bytes(b"same-game")
            (tools / "asset-tools").mkdir(parents=True)
            (tools / "asset-tools/extraction-recipes.json").write_text("new-recipe")
            (tools / "asset-tools/tools.lock.json").write_text("tools")
            cache = BundleCache(root / "cache"); extractor = AssetExtractor(tools, cache)
            executable_sha = extractor.current_fingerprints(game)["gameExecutableSha256"]
            old_sha, new_sha = "a" * 64, "b" * 64
            manifest = {"schemaVersion": 2, "compatibility": "compatible",
                        "extractionStage": "validated-and-packaged", "gameBuild": "1",
                        "generatedAt": "2026-10-20T00:00:00Z",
                        "extractionRecipeSha256": old_sha,
                        "sourceExecutable": {"sha256": executable_sha}}
            bundle = root / "old.zip"
            with zipfile.ZipFile(bundle, "w") as archive:
                archive.writestr("asset-manifest.json", json.dumps(manifest))
            cached = cache.add(bundle, game_root_id(game), manifest)
            (work / "pipeline").mkdir(parents=True)
            (work / "pipeline/incremental-state.json").write_text(json.dumps({
                "schemaVersion": 1, "recipeSha256": old_sha,
                "fingerprints": {"recipeSha256": old_sha, "gameExecutableSha256": executable_sha}}))
            (work / "extraction-journal.json").write_text(json.dumps({
                "schemaVersion": 1, "stage": "created",
                "fingerprints": {"recipeSha256": new_sha, "gameExecutableSha256": executable_sha},
                "previousFingerprints": {"recipeSha256": old_sha, "gameExecutableSha256": executable_sha}}))
            self.assertEqual(extractor.incremental_baseline_bundle(game, work), cached.path)

    def test_profile_store_has_only_credential_reference(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "profiles.json"
            profile = ServerProfile.create(name="Dev", panel_origin="https://panel.example", account_id="1",
                                           credential_target=credential_target("https://panel.example", "1"), server_id="abc")
            ProfileStore(path).save([profile])
            raw = path.read_text()
            self.assertNotIn("api_key", raw.lower())
            self.assertEqual(ProfileStore(path).load()[0], profile)

    def test_config_schema_matches_example_and_rejects_bad_values(self):
        model = ConfigModel(ROOT / "config/zanik-config.schema.json")
        values = model.parse((ROOT / "config/zanik.conf.example").read_text())
        self.assertTrue(values["ZANIK_ENABLED"])
        self.assertFalse(values["FEATURE_ADMIN_TELEPORT"])
        self.assertEqual(model.parse(model.render(values)), values)
        with self.assertRaises(ConfigError): model.parse("UNKNOWN=true\n")
        with self.assertRaises(ConfigError): model.parse("STATUS_PORT=99999\n")
        with self.assertRaises(ConfigError): model.parse("FEATURE_MAP=perhaps\n")

    def test_post_install_website_settings_preserve_other_configuration(self):
        model = ConfigModel(ROOT / "config/zanik-config.schema.json")
        original = (ROOT / "config/zanik.conf.example").read_text()
        rendered = update_website_settings(model, original, 30123, "https://play.example.test:30123")
        values = model.parse(rendered)
        self.assertEqual(values["STATUS_PORT"], 30123)
        self.assertEqual(values["PUBLIC_BASE_URL"], "https://play.example.test:30123")
        self.assertTrue(values["FEATURE_MAP"])
        self.assertFalse(values["FEATURE_QOL_AUTO_COMPOST"])

    def test_website_configuration_requires_non_loopback_public_url(self):
        self.assertFalse(website_configuration_ready({"STATUS_PORT": 29105, "PUBLIC_BASE_URL": "http://127.0.0.1:29105"}))
        self.assertFalse(website_configuration_ready({"STATUS_PORT": 29105, "PUBLIC_BASE_URL": "http://localhost:29105"}))
        self.assertTrue(website_configuration_ready({"STATUS_PORT": 29105, "PUBLIC_BASE_URL": "https://game.example:29105"}))

    def test_preferences_remember_last_profile_and_ignore_invalid_state(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "preferences.json"; store = PreferenceStore(path)
            self.assertEqual(store.load()["lastProfileId"], "")
            store.save_last_profile("profile-two")
            self.assertEqual(store.load()["lastProfileId"], "profile-two")
            path.write_text("not-json", encoding="utf-8")
            self.assertEqual(store.load()["lastProfileId"], "")
            profiles = [ServerProfile.create(name="one", panel_origin="https://panel", account_id="a",
                                             credential_target="one", server_id="one"),
                        ServerProfile.create(name="two", panel_origin="https://panel", account_id="a",
                                             credential_target="two", server_id="two")]
            self.assertEqual(preferred_profile_index(profiles, profiles[1].id), 1)
            self.assertEqual(preferred_profile_index(profiles, "deleted-profile"), 0)

    def test_packaged_asset_contract_uses_recipe_and_reviewed_profile(self):
        contract = expected_asset_contract(ROOT / "asset-tools")
        recipe = json.loads((ROOT / "asset-tools/extraction-recipes.json").read_text())
        history = json.loads((ROOT / "asset-tools/extraction-recipe-history.json").read_text())
        recipe12 = next(entry for entry in history["recipes"]
                        if entry["recipe"]["schemaVersion"] == 12)
        self.assertEqual(contract["extractionRecipeVersion"], recipe["schemaVersion"])
        self.assertEqual(contract["compatibilityProfileVersion"], recipe["catalogProfile"]["version"])
        self.assertEqual(contract["extractionRecipeSha256"], hashlib.sha256(
            (ROOT / "asset-tools/extraction-recipes.json").read_bytes()).hexdigest())
        self.assertEqual(contract["extractionRecipeVersion"], 13)
        self.assertNotEqual(contract["extractionRecipeSha256"], recipe12["sha256"])
        stale = asset_contract_presentation({
            "installed": True,
            "valid": False,
            "extractionRecipeVersion": 12,
            "compatibilityProfileVersion": recipe12["recipe"]["catalogProfile"]["version"],
            "extractionRecipeSha256": recipe12["sha256"],
        }, "0.2.65")
        self.assertEqual(stale["state"], "available")
        self.assertIn("Refresh available", stale["text"])
        self.assertIn("world-map.jpg", contract["criticalFiles"])

    def test_journal_persistence_and_incomplete_filter(self):
        with tempfile.TemporaryDirectory() as temporary:
            store = JournalStore(Path(temporary)); journal = OperationJournal.create("profile", "install")
            store.save(journal); self.assertEqual(store.incomplete("profile")[0].operation_id, journal.operation_id)
            journal.state = "complete"; store.save(journal); self.assertEqual(store.incomplete("profile"), [])

    def test_incomplete_journals_ignore_malformed_and_superseded_records(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary); store = JournalStore(root)
            active = OperationJournal.create("profile", "install"); store.save(active)
            superseded = OperationJournal.create("profile", "activate-assets")
            superseded.state = "superseded"; store.save(superseded)
            (root / "malformed.json").write_text("not-json", encoding="utf-8")

            self.assertEqual([item.operation_id for item in store.incomplete("profile")],
                             [active.operation_id])

    def test_recovery_only_applies_recorded_rollback_actions(self):
        source = (ROOT / "manager" / "zanik_manager" / "app.py").read_text(encoding="utf-8")
        self.assertIn('if not journal.rollback:', source)
        self.assertIn('"Restore safe state"', source)
        self.assertIn("It will not guess the original inputs", source)
        self.assertNotIn('self.install() if retry and journal.action=="install"', source)

    def test_newer_success_archives_only_superseded_failed_journal(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary); store = JournalStore(root)
            failed = OperationJournal.create("profile", "install")
            failed.created_at = "2026-10-12T23:40:15+00:00"; failed.state = "failed"
            interrupted = OperationJournal.create("profile", "install")
            interrupted.created_at = "2026-10-13T00:00:00+00:00"; interrupted.state = "activating"
            complete = OperationJournal.create("profile", "install")
            complete.created_at = "2026-10-14T05:36:26+00:00"; complete.state = "complete"
            for journal in (failed, interrupted, complete): store.save(journal)

            incomplete = store.incomplete("profile")

            self.assertEqual([item.operation_id for item in incomplete], [interrupted.operation_id])
            self.assertFalse((root / f"{failed.operation_id}.json").exists())
            self.assertTrue((root / "archive" / f"{failed.operation_id}.json").exists())

    def test_newer_success_does_not_archive_newer_failure_or_other_action(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary); store = JournalStore(root)
            complete = OperationJournal.create("profile", "install")
            complete.created_at = "2026-10-14T05:36:26+00:00"; complete.state = "complete"
            newer_failure = OperationJournal.create("profile", "install")
            newer_failure.created_at = "2026-10-15T05:36:26+00:00"; newer_failure.state = "failed"
            asset_failure = OperationJournal.create("profile", "activate-assets")
            asset_failure.created_at = "2026-10-12T05:36:26+00:00"; asset_failure.state = "failed"
            for journal in (complete, newer_failure, asset_failure): store.save(journal)

            incomplete = store.incomplete("profile")

            self.assertEqual({item.operation_id for item in incomplete},
                             {newer_failure.operation_id, asset_failure.operation_id})
            self.assertFalse((root / "archive").exists())

    def test_cache_keeps_two_unpinned_bundles(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary); cache = BundleCache(root / "cache")
            for index in range(3):
                bundle = root / f"asset-{index}.zip"; bundle.write_bytes(str(index).encode())
                cache.add(bundle, "game", {"gameBuild": str(index), "generatedAt": f"2026-01-0{index+1}"})
            self.assertEqual(len(cache.list("game")), 2)

    def test_bundle_timestamp_is_human_readable_and_stable_for_unknown_values(self):
        label = format_bundle_timestamp("2026-10-14T23:45:00Z")
        self.assertIn("2026-10-14", label)
        self.assertNotIn("T23:45:00Z", label)
        self.assertEqual(format_bundle_timestamp(""), "time unknown")
        self.assertEqual(format_bundle_timestamp("not-a-time"), "not-a-time")

    def test_redaction_and_support_bundle(self):
        token = "ptlc_supersecret"
        self.assertNotIn(token, redact(f"Authorization: Bearer {token}"))
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary); log = root / "manager.log"; log.write_text(f"token={token}\n")
            output = create_support_bundle(root / "support.zip", {"ok": True}, [], [log], (token,))
            with zipfile.ZipFile(output) as archive:
                self.assertNotIn(token, archive.read("logs/manager.log").decode())

    def test_pterodactyl_routes_and_522_retry(self):
        calls = []
        def transport(method, url, headers, body, timeout):
            calls.append((method, url, headers, body))
            if len(calls) == 1: return Response(522, b"", {})
            return Response(200, b'{"data":[{"attributes":{"identifier":"dev"}}]}', {})
        client = PterodactylClient("https://panel.example", "secret-token", transport, retry_seconds=0)
        self.assertEqual(client.servers()[0]["identifier"], "dev")
        self.assertEqual(len(calls), 2)
        self.assertEqual(calls[-1][2]["Authorization"], "Bearer secret-token")

    def test_pterodactyl_errors_do_not_include_response_body(self):
        client = PterodactylClient("https://panel.example", "secret", lambda *args: Response(500, b'token=leak', {}))
        with self.assertRaises(PterodactylError) as raised: client.servers()
        self.assertNotIn("leak", str(raised.exception))

    def test_file_contents_node_failure_uses_signed_download_fallback(self):
        calls = []
        def transport(method, url, headers, body, timeout):
            calls.append((method, url, headers))
            if "/files/contents" in url:
                return Response(500, b'{"errors":[{"detail":"node unavailable"}]}', {})
            if "/files/download" in url:
                return Response(200, b'{"attributes":{"url":"https://node.example/config"}}', {})
            if url == "https://node.example/config":
                return Response(200, b"FEATURE_MAP=true\n", {})
            raise AssertionError(url)
        content = PterodactylClient("https://panel.example", "secret", transport).read_file(
            "dev", "/home/container/zanik/config/zanik.conf")
        self.assertEqual(content, b"FEATURE_MAP=true\n")
        self.assertEqual(len(calls), 3)
        self.assertNotIn("Authorization", calls[-1][2])

    def test_file_contents_permission_failure_does_not_fallback(self):
        calls = []
        def transport(*args):
            calls.append(args); return Response(403, b"{}", {})
        with self.assertRaises(PterodactylError):
            PterodactylClient("https://panel.example", "secret", transport).read_file(
                "dev", "/home/container/zanik/config/zanik.conf")
        self.assertEqual(len(calls), 1)

    def test_file_contents_editor_size_limit_uses_signed_download_fallback(self):
        calls = []
        def transport(method, url, headers, body, timeout):
            calls.append((method, url, headers))
            if "/files/contents" in url:
                return Response(400, b'{"errors":[{"detail":"The file you are attempting to open is too large to view in the file editor."}]}', {})
            if "/files/download" in url:
                return Response(200, b'{"attributes":{"url":"https://node.example/library"}}', {})
            if url == "https://node.example/library":
                return Response(200, b"\x7fELFlarge", {})
            raise AssertionError(url)
        content = PterodactylClient("https://panel.example", "secret", transport).read_file(
            "dev", "/home/container/zanik/bin/libzanik.so")
        self.assertEqual(content, b"\x7fELFlarge")
        self.assertEqual(len(calls), 3)
        self.assertNotIn("Authorization", calls[-1][2])

    def test_pterodactyl_translates_container_paths_to_virtual_root(self):
        calls = []
        def transport(method, url, headers, body, timeout):
            calls.append((method, url, body))
            return Response(200, b'{"data":[]}', {})
        client = PterodactylClient("https://panel.example", "secret", transport)
        client.list_files("dev", "/home/container")
        client.read_file("dev", "/home/container/zanik/install-state.json")
        client.create_folder("dev", "/home/container/zanik", "assets")
        client.rename("dev", "/home/container", [("zanik-new", "zanik")])
        self.assertIn("directory=%2F", calls[0][1])
        self.assertIn("file=%2Fzanik%2Finstall-state.json", calls[1][1])
        self.assertEqual(json.loads(calls[2][2]), {"root": "/zanik", "name": "assets"})
        self.assertEqual(json.loads(calls[3][2])["root"], "/")
        self.assertEqual(api_file_path("/home/container/zanik"), "/zanik")

    def test_pterodactyl_large_file_download_uses_signed_url(self):
        calls = []
        def transport(method, url, headers, body, timeout):
            calls.append((method, url, headers))
            if "/files/download" in url:
                return Response(200, b'{"attributes":{"url":"https://node.example/signed"}}', {})
            return Response(200, b"\x7fELFlarge", {})
        client = PterodactylClient("https://panel.example", "secret", transport)
        data = client.download_file("dev", "/home/container/game")
        self.assertEqual(data, b"\x7fELFlarge")
        self.assertIn("file=%2Fgame", calls[0][1])
        self.assertEqual(calls[1][1], "https://node.example/signed")
        self.assertNotIn("Authorization", calls[1][2])

    def test_pterodactyl_formats_unix_modes_as_octal(self):
        calls = []
        def transport(method, url, headers, body, timeout):
            calls.append(json.loads(body)); return Response(204, b"", {})
        PterodactylClient("https://panel.example", "secret", transport).chmod(
            "dev", "/home/container/zanik/bin", [("zanik-server", 0o755), ("state", 0o600)])
        self.assertEqual(calls[0]["files"], [{"file":"zanik-server","mode":"755"},
                                               {"file":"state","mode":"600"}])

    def test_idempotent_directory_creation_retries_gateway_failure(self):
        class Client:
            retry_seconds = 0
            cancelled = staticmethod(lambda: False)
            def __init__(self): self.calls = []
            def create_folder(self, server, root, name):
                self.calls.append((root, name))
                if len(self.calls) == 1: raise PterodactylError("gateway", 504)
        client = Client()
        with tempfile.TemporaryDirectory() as temporary:
            PterodactylProvider(client, JournalStore(Path(temporary)))._mkdir("dev", "/home/container/zanik")
        self.assertEqual(client.calls[:2], [("/", "zanik"), ("/", "zanik")])

    def test_missing_directory_reported_as_wings_500_is_not_retried_as_an_outage(self):
        calls = []
        class Client:
            retry_seconds = 0
            cancelled = staticmethod(lambda: False)
            def list_files(self, server_id, directory):
                calls.append(directory)
                if directory == "/home/container/zanik":
                    raise PterodactylError("node file error", 500)
                if directory == "/home/container":
                    return [{"name": "RSDragonwilds", "is_file": False}]
                raise AssertionError(directory)
        with tempfile.TemporaryDirectory() as temporary:
            provider = PterodactylProvider(Client(), JournalStore(Path(temporary)))
            self.assertIsNone(provider._entry("dev", "/home/container/zanik/install-state.json"))
        self.assertEqual(calls, ["/home/container/zanik", "/home/container"])

    def test_lightweight_installation_status_distinguishes_installed_and_absent(self):
        class Client:
            def __init__(self, installed): self.installed = installed
            def list_files(self, _server, directory):
                if not self.installed: return []
                if directory == "/home/container/zanik":
                    return [{"name": "config", "is_file": False}, {"name": "VERSION", "is_file": True},
                            {"name": "install-state.json", "is_file": True}]
                if directory == "/home/container/zanik/config":
                    return [{"name": "zanik.conf", "is_file": True}]
                return []
            def read_file(self, _server, path):
                if path.endswith("zanik.conf"): return b"ZANIK_ENABLED=true\n"
                if path.endswith("install-state.json"):
                    return json.dumps({"version": "0.2.21", "mode": "wrapper"}).encode()
                if path.endswith("VERSION"): return b"0.2.21\n"
                raise PterodactylError("missing", 404)
        with tempfile.TemporaryDirectory() as temporary:
            journals = JournalStore(Path(temporary))
            installed = PterodactylProvider(Client(True), journals).installation_status("dev", "/home/container")
            absent = PterodactylProvider(Client(False), journals).installation_status("dev", "/home/container")
        self.assertTrue(installed["installed"]); self.assertEqual(installed["mode"], "wrapper")
        self.assertFalse(absent["installed"])

    def test_installation_status_and_summary_use_payload_contract_not_release_version(self):
        contract = "a" * 64
        critical = {"bin/libzanik.so": "b" * 64, "bin/zanik-compat-analyzer": "c" * 64,
                    "bin/zanik-server": "d" * 64}
        install = {"version": "0.2.21", "mode": "replace-executable", "librarySha256": critical["bin/libzanik.so"],
                   "analyzerSha256": critical["bin/zanik-compat-analyzer"], "launcherSha256": critical["bin/zanik-server"]}
        class Client:
            def list_files(self, _server, directory):
                return ([{"name": "VERSION", "is_file": True}, {"name": "install-state.json", "is_file": True}]
                        if directory.endswith("/zanik") else [])
            def read_file(self, _server, path):
                return json.dumps(install).encode() if path.endswith("install-state.json") else b"0.2.21\n"
        legacy = {"0.2.21": {"contractSchemaVersion": 1, "contractSha256": contract,
                              "criticalHashes": critical}}
        with tempfile.TemporaryDirectory() as temporary:
            result = PterodactylProvider(Client(), JournalStore(Path(temporary))).installation_status(
                "dev", "/home/container", {"schemaVersion": 1, "contractSha256": contract}, legacy)
        self.assertEqual(result["payloadCompatibility"], "current")
        self.assertTrue(result["legacyContractRecognized"])
        self.assertEqual(compatibility_summary(result, "0.2.22"),
                         "Zanik 0.2.21 · Up to date with Manager 0.2.22")
        latest = {"updateAvailable": True, "latestVersion": "0.2.23",
                  "serverPayload": {"contractSha256": "e" * 64}}
        self.assertEqual(compatibility_summary(result, "0.2.22", latest),
                         "Zanik 0.2.21 · Update Manager to 0.2.23, then update Zanik")

    def test_asset_contract_status_checks_metadata_and_critical_hashes(self):
        payloads = {"world-map.jpg": b"map", "quest-catalog.json": b"{}"}
        entries = [{"path": name, "sha256": content_hash(data), "size": len(data)}
                   for name, data in payloads.items()]
        manifest = {"compatibility": "compatible", "extractionStage": "validated-and-packaged",
                    "missingRequired": [], "extractionRecipeVersion": 5,
                    "extractionRecipeSha256": "a" * 64,
                    "compatibilityProfile": {"version": "reviewed-5"},
                    "generatedFileCount": len(entries), "files": entries}
        class Client:
            def read_file(self, _server, path):
                if path.endswith("asset-manifest.json"): return json.dumps(manifest).encode()
                raise PterodactylError("missing", 404)
            def download_file(self, _server, path): return payloads[path.rsplit("/", 1)[-1]]
        contract = {"extractionRecipeVersion": 5, "compatibilityProfileVersion": "reviewed-5",
                    "extractionRecipeSha256": "a" * 64,
                    "criticalFiles": sorted(payloads)}
        with tempfile.TemporaryDirectory() as temporary:
            provider = PterodactylProvider(Client(), JournalStore(Path(temporary)))
            valid = provider.asset_contract_status("dev", "/home/container", contract)
            stale = provider.asset_contract_status("dev", "/home/container", {**contract, "extractionRecipeVersion": 6})
            changed_rules = provider.asset_contract_status(
                "dev", "/home/container", {**contract, "extractionRecipeSha256": "b" * 64})
            missing = provider.asset_contract_status(
                "dev", "/home/container", {**contract, "criticalFiles": contract["criticalFiles"] + ["recipe-unlocks.json"]})
        self.assertTrue(valid["valid"])
        self.assertFalse(stale["valid"])
        self.assertTrue(any("recipe version" in reason for reason in stale["reasons"]))
        self.assertFalse(changed_rules["valid"])
        self.assertTrue(any("different extraction rules" in reason for reason in changed_rules["reasons"]))
        self.assertFalse(missing["valid"])
        self.assertTrue(any("recipe-unlocks.json" in reason for reason in missing["reasons"]))

    def test_release_payload_manifest_and_traversal(self):
        with tempfile.TemporaryDirectory() as temporary:
            archive = Path(temporary) / "server.zip"
            required = ["bin/libzanik.so", "bin/zanik-server", "bin/zanik-compat-analyzer", "scripts/compat-launcher.sh",
                        "scripts/verify-assets.sh", "scripts/check-game-update.sh", "scripts/check-compatible-release.sh",
                        "scripts/update-game.sh", "scripts/apply-asset-delta.sh",
                        "config/zanik.conf.example", "compatibility/compatibility.tsv", "VERSION"]
            files = {name: b"0.2.0" if name == "VERSION" else b"payload" for name in required}
            write_payload_archive(archive, files)
            payload = ReleasePayload(archive)
            self.assertEqual(len(payload.manifest()["files"]), len(required) + 1)
            self.assertEqual(payload.manifest()["payloadContractSha256"], payload.contract["contractSha256"])
            with self.assertRaises(ValueError): safe_archive_name("../secret")

    def test_server_root_and_exact_legacy_launcher(self):
        root = "/home/container"
        launcher = f'#!/bin/sh\nexec /bin/sh {root}/zanik/zanik-bootstrap.sh "$@"\n'.encode()
        self.assertEqual(validate_server_root(root), root)
        self.assertTrue(is_exact_legacy_launcher(launcher, root))
        self.assertTrue(is_exact_legacy_launcher(launcher.replace(b"\n", b"\r\n"), root))
        self.assertFalse(is_exact_legacy_launcher(launcher + b"echo injected\n", root))
        self.assertFalse(is_exact_legacy_launcher(launcher.replace(b"/home/container", b"/tmp/other"), root))
        for unsafe in ("home/container", "/", "/home", "/home/container/../other"):
            with self.assertRaises(ValueError):
                validate_server_root(unsafe)

    def test_asset_upload_parts_are_indexed_and_bounded(self):
        with tempfile.TemporaryDirectory() as temporary:
            root=Path(temporary); bundle=root/"bundle.zip"
            with zipfile.ZipFile(bundle,"w",compression=zipfile.ZIP_STORED) as archive:
                for name, data in (("asset-manifest.json", b"{}"),
                                   ("items/a.png", __import__('os').urandom(300)),
                                   ("items/b.png", __import__('os').urandom(300))):
                    archive.writestr(zipfile.ZipInfo(name, date_time=(2026, 10, 17, 4, 20, 0)), data)
            parts,index=AssetDeployer._make_parts(bundle,root/"parts",limit=500)
            self.assertGreaterEqual(len(parts),2)
            self.assertEqual(index["bundleSha256"],content_hash(bundle.read_bytes()))
            self.assertTrue(all(part.stat().st_size < 500 for part in parts))
            for part in parts:
                with zipfile.ZipFile(part) as archive:
                    self.assertEqual({item.date_time for item in archive.infolist()},
                                     {(2026, 10, 17, 4, 20, 0)})

    def test_asset_delta_plan_uploads_only_changed_and_added_files(self):
        base = {"files": [{"path": "same.png", "sha256": "a" * 64},
                           {"path": "changed.png", "sha256": "b" * 64},
                           {"path": "removed.png", "sha256": "c" * 64}]}
        target = {"files": [{"path": "same.png", "sha256": "a" * 64},
                             {"path": "changed.png", "sha256": "d" * 64},
                             {"path": "added.png", "sha256": "e" * 64}]}
        changed, removed = AssetDeployer._delta_plan(base, target)
        self.assertEqual([record["path"] for record in changed], ["added.png", "changed.png"])
        self.assertEqual(removed, ["removed.png"])

    def test_asset_delta_semantics_ignore_generation_timestamp_only(self):
        files = [{"path": "same.png", "sha256": "a" * 64}]
        base = {"schemaVersion": 2, "generatedAt": "old", "extractionRecipeVersion": 6,
                "extractionRecipeSha256": "b" * 64, "compatibility": "compatible",
                "compatibilityProfile": {"version": "reviewed-6"}, "missingRequired": [], "files": files}
        target = {**base, "generatedAt": "new"}
        self.assertEqual(AssetDeployer._delta_plan(base, target), ([], []))
        self.assertEqual(AssetDeployer._manifest_contract(base), AssetDeployer._manifest_contract(target))
        changed_contract = {**target, "extractionRecipeVersion": 7}
        self.assertNotEqual(AssetDeployer._manifest_contract(base),
                            AssetDeployer._manifest_contract(changed_contract))

    def test_remote_asset_subset_verification_uses_large_file_download_route(self):
        class Client:
            def download_file(self, server, path): return b"asset-bytes"
            def read_file(self, server, path): raise AssertionError("text endpoint must not verify assets")
        class Provider:
            client = Client()
        with tempfile.TemporaryDirectory() as temporary:
            tree=Path(temporary); (tree/"world-map.jpg").write_bytes(b"asset-bytes")
            progress=[]
            AssetDeployer(Provider())._verify_remote_subset("dev", "/stage", tree, {"world-map.jpg"},
                                                            lambda current,total,path:progress.append((current,total,path)))
            self.assertEqual(progress,[(1,1,"world-map.jpg")])

    def test_deep_asset_verification_uses_one_server_report(self):
        manifest = {"files": [
            {"path": "world-map.jpg", "sha256": "a" * 64},
            {"path": "items/example.png", "sha256": "b" * 64},
        ]}
        manifest_raw = json.dumps(manifest, sort_keys=True).encode()
        canonical = "".join(f"{entry['sha256']}\t{entry['path']}\n" for entry in manifest["files"]).encode()
        class Client:
            def __init__(self): self.request_id = None; self.deleted = False; self.reads = []
            def command(self, server, command): self.request_id = command.rsplit(" ", 1)[-1]
            def read_file(self, server, path):
                self.reads.append(path)
                if path.endswith("asset-manifest.json"): return manifest_raw
                return json.dumps({"schemaVersion": 1, "requestId": self.request_id, "status": "valid",
                                   "expected": 2, "checked": 2, "missing": 0, "mismatches": 0,
                                   "aggregateSha256": content_hash(canonical),
                                   "manifestSha256": content_hash(manifest_raw)}).encode()
            def delete(self, server, root, files): self.deleted = True
        client = Client()
        with tempfile.TemporaryDirectory() as temporary:
            provider = PterodactylProvider(client, JournalStore(Path(temporary)))
            report = provider._server_asset_audit("dev", "/home/container", manifest, timeout=0.1)
        self.assertEqual(report["method"], "server-side-aggregate")
        self.assertEqual(len(client.reads), 2)
        self.assertTrue(client.deleted)

    def test_game_update_check_rejects_stale_report_and_matches_request(self):
        class Client:
            def list_files(self, _server, _path): return []
            def read_file(self, _server, path):
                if path.endswith("appmanifest_4019830.acf"):
                    return b'"AppState" { "buildid" "123" }'
                raise PterodactylError("not found", 404)
        class Checker:
            def latest_build(self): return "123"
        with tempfile.TemporaryDirectory() as temporary:
            result = PterodactylProvider(Client(), JournalStore(Path(temporary)), Checker()).check_game_update(
                "dev", "/home/container", timeout=2.5)
        self.assertTrue(result["fresh"])
        self.assertEqual(result["status"], "current")
        self.assertFalse(result["zanikInstalled"])

    def test_offline_game_update_check_uses_cached_report(self):
        class Client:
            def resources(self, _server): return {"current_state": "offline"}
            def list_files(self, _server, path):
                return [{"name": "VERSION"}] if path.endswith("/zanik") else []
            def read_file(self, _server, path):
                if path.endswith("game-update-status.json"):
                    return b'{"schemaVersion":1,"status":"update-available","updateAvailable":true}'
                if path.endswith("appmanifest_4019830.acf"):
                    return b'"AppState" { "buildid" "123" }'
                raise PterodactylError("not found", 404)
        class Checker:
            def latest_build(self): raise RuntimeError("offline")
        with tempfile.TemporaryDirectory() as temporary:
            result = PterodactylProvider(Client(), JournalStore(Path(temporary)), Checker()).check_game_update(
                "dev", "/home/container")
        self.assertFalse(result["fresh"])
        self.assertTrue(result["zanikInstalled"])

    def test_uninstalled_configuration_is_a_normal_state(self):
        class Client:
            def list_files(self, _server, _path): return []
            def read_file(self, *_args): raise AssertionError("missing configuration must not be downloaded")
        with tempfile.TemporaryDirectory() as temporary:
            result = PterodactylProvider(Client(), JournalStore(Path(temporary))).read_configuration(
                "dev", "/home/container")
        self.assertEqual(result, {"zanikInstalled": False, "zanikEnabled": None,
                                  "exists": False, "data": None})

    def test_pyside_pruner_keeps_only_declared_runtime(self):
        with tempfile.TemporaryDirectory() as temporary:
            root=Path(temporary)
            for name in KEEP_FILES|{"PySide6/Qt6Qml.dll","PySide6/designer.exe"}:
                path=root/name; path.parent.mkdir(parents=True,exist_ok=True); path.write_bytes(b"x")
            prune(root)
            self.assertFalse((root/"PySide6/Qt6Qml.dll").exists())
            self.assertFalse((root/"PySide6/designer.exe").exists())
            self.assertTrue(all((root/name).is_file() for name in KEEP_FILES))

    def test_asset_activation_restarts_only_a_running_server(self):
        class Client:
            def __init__(self, state): self.state=state; self.signals=[]
            def resources(self, _server): return {"current_state": self.state, "resources": {"uptime": 1234}}
            def power(self, _server, signal): self.signals.append(signal)
        class Provider:
            def __init__(self, state): self.client=Client(state)
        messages=[]; running=Provider("running")
        self.assertEqual(AssetDeployer(running)._restart_runtime_if_running("dev", messages.append),
                         {"previousUptime": 1234})
        self.assertEqual(running.client.signals, ["restart"])
        self.assertIn("runtime catalogs", messages[0])
        offline=Provider("offline")
        self.assertIsNone(AssetDeployer(offline)._restart_runtime_if_running("dev", messages.append))
        self.assertEqual(offline.client.signals, [])

    def test_replacement_launcher_check_detects_steam_displacement_without_downloading_elf(self):
        root = "/home/container"
        game = root + "/RSDragonwilds/Binaries/Linux/RSDragonwildsServer-Linux-Shipping"
        state = json.dumps({"mode": "replace-executable"}).encode()
        class Client:
            def __init__(self): self.size = 230_000_000; self.game_reads = 0
            def read_file(self, _server, path):
                if path.endswith("install-state.json"): return state
                if path == game:
                    self.game_reads += 1
                    return b"#!/bin/sh\n# ZANIK_COMPAT_LAUNCHER=1\n"
                raise PterodactylError("not found", 404)
            def list_files(self, _server, directory):
                self.assert_directory = directory
                return [{"name": Path(game).name, "is_file": True, "size": self.size}]
        client = Client()
        with tempfile.TemporaryDirectory() as temporary:
            provider = PterodactylProvider(client, JournalStore(Path(temporary)))
            displaced = provider.replacement_launcher_status("dev", root)
            self.assertFalse(displaced["healthy"])
            self.assertEqual(displaced["reason"], "official-executable-replaced-launcher")
            self.assertEqual(client.game_reads, 0)
            client.size = 128
            managed = provider.replacement_launcher_status("dev", root)
            self.assertTrue(managed["healthy"])
            self.assertEqual(client.game_reads, 1)

    def test_replacement_install_and_uninstall_against_synthetic_server(self):
        class FakeClient:
            def __init__(self):
                self.files = {"/home/container/RSDragonwilds/Binaries/Linux/RSDragonwildsServer-Linux-Shipping": b"\x7fELFofficial"}
                self.directories = {"/", "/home", "/home/container", "/home/container/RSDragonwilds", "/home/container/RSDragonwilds/Binaries", "/home/container/RSDragonwilds/Binaries/Linux"}
                self.state = "offline"
                self.startup_values = {"AUTO_UPDATE": "1", "SERVER_BINARY": "RSDragonwildsServer-Linux-Shipping"}
                self.uploads = []
                self.signals = []
            def server(self, _): return {"server_owner": True}
            def resources(self, _): return {"current_state": self.state}
            def power(self, _, signal):
                self.signals.append(signal); self.state = "running" if signal in {"start", "restart"} else "offline"
            def create_folder(self, _, root, name): self.directories.add(str(Path(root) / name).replace("\\", "/"))
            def upload(self, _, directory, name, content):
                path = f"{directory.rstrip('/')}/{name}"
                self.uploads.append(path); self.files[path] = content
            def write_file(self, _, path, content): self.files[path] = content
            def read_file(self, _, path):
                if path not in self.files: raise PterodactylError("not found", 404)
                return self.files[path]
            def download_file(self, server, path): return self.read_file(server, path)
            def list_files(self, _, path):
                if path not in self.directories and not any(name.startswith(path.rstrip('/') + '/') for name in self.files):
                    raise PterodactylError("not found", 404)
                prefix=path.rstrip('/')+'/'
                result=[]
                names=set()
                for name in self.files:
                    if name.startswith(prefix) and '/' not in name[len(prefix):]:
                        names.add(name[len(prefix):]); result.append({"name":name[len(prefix):],"is_file":True,
                                                                     "size":len(self.files[name]),
                                                                     "modified_at":"2026-10-26T12:00:00Z"})
                for name in self.directories:
                    if name.startswith(prefix) and name != path and '/' not in name[len(prefix):] and name[len(prefix):] not in names:
                        result.append({"name":name[len(prefix):],"is_file":False})
                return result
            def chmod(self, *args): pass
            def startup_variable(self, _server, key):
                return {"key": key, "value": self.startup_values[key], "editable": True}
            def set_startup_variable(self, _server, key, value): self.startup_values[key] = value
            def rename(self, _, root, pairs):
                for source, target in pairs:
                    source = f"{root.rstrip('/')}/{source}"; target = f"{root.rstrip('/')}/{target}"
                    if source in self.files:
                        self.files[target] = self.files.pop(source); continue
                    moved = [(path, target + path[len(source):]) for path in list(self.files) if path.startswith(source + "/")]
                    if not moved: raise PterodactylError("not found", 404)
                    for old, new in moved: self.files[new] = self.files.pop(old)
                    self.directories = {directory for directory in self.directories
                                        if directory != source and not directory.startswith(source + "/")}
                    self.directories.add(target)
            def delete(self, _, root, files):
                for item in files:
                    path=f"{root.rstrip('/')}/{item}"
                    self.files={key:value for key,value in self.files.items() if key != path and not key.startswith(path + "/")}
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary); archive = root / "server.zip"
            files = {"bin/libzanik.so": b"library", "bin/zanik-server": b"launcher", "bin/zanik-compat-analyzer": b"analyzer",
                     "scripts/compat-launcher.sh": b"#!/bin/sh\n# ZANIK_COMPAT_LAUNCHER=1\n",
                     "scripts/verify-assets.sh": b"#!/bin/sh\nexit 0\n", "scripts/check-game-update.sh": b"#!/bin/sh\nexit 0\n",
                     "scripts/check-compatible-release.sh": b"#!/bin/sh\nexit 0\n", "scripts/update-game.sh": b"#!/bin/sh\n# ZANIK_GAME_UPDATER=1\n",
                     "scripts/apply-asset-delta.sh": b"#!/bin/sh\nexit 0\n",
                     "config/zanik.conf.example": b"FEATURE_MAP=true\n",
                     "config/zanik-config.schema.json": b"{}", "compatibility/compatibility.tsv": b"sha\toffsets\n",
                     "docs/obsolete.md": b"old documentation\n", "VERSION": b"0.2.0\n"}
            write_payload_archive(archive, files)
            client=FakeClient(); provider=PterodactylProvider(client, JournalStore(root/"journals"))
            result=provider.install("profile", "dev", "/home/container", ReleasePayload(archive), "replace-executable", True)
            game="/home/container/RSDragonwilds/Binaries/Linux/RSDragonwildsServer-Linux-Shipping"
            self.assertIn(b"ZANIK_COMPAT_LAUNCHER", client.files[game])
            self.assertEqual(client.files[game+".zanik-original"], b"\x7fELFofficial")
            self.assertEqual(result["installation"]["version"], "0.2.0")
            self.assertIsNone(result["backup"])
            self.assertEqual(client.files["/home/container/zanik/compatibility.tsv"], b"sha\toffsets\n")
            original_launcher = client.files[game]
            config_path = "/home/container/zanik/config/zanik.conf"
            client.files[config_path] = b"FEATURE_MAP=false\n"
            client.uploads.clear()
            del client.files["/home/container/zanik/config/zanik-config.schema.json"]
            files["scripts/compat-launcher.sh"] = b"#!/bin/sh\n# ZANIK_COMPAT_LAUNCHER=1\n# updated\n"
            files["VERSION"] = b"0.2.1\n"
            del files["docs/obsolete.md"]
            write_payload_archive(archive, files, "0.2.1")
            update = provider.install("profile", "dev", "/home/container", ReleasePayload(archive), "replace-executable", True)
            self.assertIsNotNone(update["backup"])
            backup_manifest = json.loads(client.files[update["backup"] + "/.zanik-backup.json"])
            self.assertEqual((backup_manifest["family"], backup_manifest["state"]), ("payload", "complete"))
            self.assertEqual(update["installation"]["version"], "0.2.1")
            self.assertNotEqual(client.files[game], original_launcher)
            uploaded = {path.removeprefix("/home/container/zanik/") for path in client.uploads
                        if path.startswith("/home/container/zanik/")}
            self.assertEqual(uploaded, {"VERSION", "config/zanik-config.schema.json", "payload-contract.json",
                                        "scripts/compat-launcher.sh"})
            self.assertNotIn("bin/libzanik.so", uploaded)
            self.assertNotIn("/home/container/zanik/docs/obsolete.md", client.files)
            self.assertEqual(update["reconciliation"], {"uploaded": 4, "removed": 1,
                                                        "reused": len(files) - 3,
                                                        "serverRestarted": False})
            update_journal = provider.journals.load(provider.journals.root / f"{update['operationId']}.json")
            client.uploads.clear(); client.signals.clear(); client.state = "running"
            repair = provider.install("profile", "dev", "/home/container", ReleasePayload(archive),
                                      "replace-executable", True)
            self.assertEqual(repair["reconciliation"], {"uploaded": 0, "removed": 0,
                                                        "reused": len(files) + 1,
                                                        "serverRestarted": False})
            self.assertEqual(client.uploads, []); self.assertEqual(client.signals, [])
            self.assertIsNone(repair["backup"])
            provider.rollback(update_journal, "dev", "/home/container")
            self.assertEqual(client.files[game], original_launcher)
            self.assertEqual(client.files[config_path], b"FEATURE_MAP=false\n")
            restored = json.loads(client.files["/home/container/zanik/install-state.json"])
            self.assertEqual(restored["version"], "0.2.0")
            client.state = "offline"; client.signals.clear()
            uninstall = provider.uninstall("profile", "dev", "/home/container")
            self.assertEqual(client.files[game], b"\x7fELFofficial")
            self.assertFalse(uninstall["dataPurged"])
            self.assertTrue(uninstall["dataPreserved"])
            self.assertFalse(uninstall["startupActionRequired"])
            self.assertFalse(uninstall["serverRestarted"])
            self.assertEqual(client.signals, [])
            self.assertNotIn(game + ".zanik-original", client.files)
            self.assertNotIn("/home/container/zanik/install-state.json", client.files)
            self.assertIn("/home/container/zanik-data/console-admin.key", client.files)
            uninstall_manifest = json.loads(client.files[uninstall["backup"] + "/.zanik-backup.json"])
            self.assertEqual((uninstall_manifest["family"], uninstall_manifest["state"]),
                             ("uninstall", "complete"))

            adapter = {"key": "SERVER_BINARY", "value": "zanik/bin/zanik-server",
                       "previous": client.startup_values["SERVER_BINARY"]}
            wrapper = provider.install("profile", "dev", "/home/container", ReleasePayload(archive), "wrapper",
                                       wrapper_adapter=adapter)
            self.assertEqual(client.startup_values["SERVER_BINARY"], "zanik/bin/zanik-server")
            self.assertEqual(wrapper["installation"]["schemaVersion"], 4)
            self.assertEqual(wrapper["installation"]["startupAdapter"]["previousValue"],
                             "RSDragonwildsServer-Linux-Shipping")
            adapter["previous"] = "zanik/bin/zanik-server"
            repaired = provider.install("profile", "dev", "/home/container", ReleasePayload(archive), "wrapper",
                                        wrapper_adapter=adapter)
            self.assertEqual(repaired["installation"]["startupAdapter"]["previousValue"],
                             "RSDragonwildsServer-Linux-Shipping")
            client.state = "running"
            client.signals.clear()
            wrapper_uninstall = provider.uninstall("profile", "dev", "/home/container")
            self.assertEqual(wrapper_uninstall["startupStatus"], "restored")
            self.assertFalse(wrapper_uninstall["startupActionRequired"])
            self.assertEqual(client.startup_values["SERVER_BINARY"], "RSDragonwildsServer-Linux-Shipping")
            self.assertEqual(client.signals, ["stop", "start"])
            self.assertIn("/home/container/zanik-data/console-admin.key", client.files)

            manual = provider.install("profile", "dev", "/home/container", ReleasePayload(archive), "wrapper")
            self.assertTrue(manual["startupActionRequired"])
            client.state = "running"; client.signals.clear()
            manual_uninstall = provider.uninstall("profile", "dev", "/home/container")
            self.assertTrue(manual_uninstall["startupActionRequired"])
            self.assertFalse(manual_uninstall["serverRestarted"])
            self.assertEqual(client.signals, ["stop"])
            self.assertEqual(client.state, "offline")
            self.assertIn("/home/container/zanik-data/console-admin.key", client.files)


if __name__ == "__main__":
    unittest.main()
