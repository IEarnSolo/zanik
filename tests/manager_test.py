#!/usr/bin/env python3
import json
import sys
import tempfile
import unittest
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from manager.zanik_manager.assets import (AssetDeployer, AssetExtractor, asset_setup_mode,
                                          dragonwilds_is_running, request_dragonwilds_close,
                                          resume_activity_message)
from manager.zanik_manager.cache import BundleCache, format_bundle_timestamp
from manager.zanik_manager.config import ConfigError, ConfigModel, content_hash, update_website_settings
from manager.zanik_manager.journal import JournalStore, OperationJournal
from manager.zanik_manager.lifecycle import (PterodactylProvider, ReleasePayload, is_exact_legacy_launcher,
                                             safe_archive_name, validate_server_root)
from manager.zanik_manager.onboarding import discover_panel, is_remote_http
from manager.zanik_manager.profiles import ProfileStore, ServerProfile, is_local_origin, normalize_origin
from manager.zanik_manager.pterodactyl import PterodactylClient, PterodactylError, Response, api_file_path
from manager.zanik_manager.security import credential_target, redact
from manager.zanik_manager.support import create_support_bundle
from tools.prune_pyside_runtime import KEEP_FILES, prune

class ManagerTests(unittest.TestCase):
    def test_update_controls_are_distinct_and_non_modal_by_default(self):
        source = (ROOT / "manager" / "zanik_manager" / "app.py").read_text(encoding="utf-8")
        self.assertIn('"Update Zanik server"', source)
        self.assertIn('QPushButton("Update and relaunch")', source)
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
        self.assertIn("QProgressDialog", source)
        self.assertIn("Qt.ApplicationModal", source)
        self.assertIn("Wait for the current operation to finish", source)
        self.assertIn('phase_percent = {"preserve": 10, "mappings": 15, "restore": 30,', source)
        self.assertIn("Close game and continue", source)
        self.assertIn("Set up website assets now", source)
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
        self.assertFalse(dragonwilds_is_running(running, "posix"))

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

    def test_resume_and_cache_compatibility_reject_game_updates(self):
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

    def test_manager_exposes_resume_and_safe_discard_controls(self):
        source = (ROOT / "manager" / "zanik_manager" / "app.py").read_text(encoding="utf-8")
        self.assertIn("Resume extraction", source)
        self.assertIn("Discard generated work", source)
        self.assertIn("def resume_asset_extraction", source)
        self.assertIn("def discard_asset_extraction", source)
        assets_source = (ROOT / "manager" / "zanik_manager" / "assets.py").read_text(encoding="utf-8")
        self.assertIn('encoding="utf-8-sig"', assets_source)
        self.assertIn("UnrealAssetScout", assets_source)
        self.assertIn("UE4SS", assets_source)
        self.assertIn("game_build", assets_source)
        lifecycle_source = (ROOT / "manager" / "zanik_manager" / "lifecycle.py").read_text(encoding="utf-8")
        self.assertIn('"buildingIconSources": manifest.get("buildingIconSources", {})', lifecycle_source)
        self.assertIn('"recipeRouteCoverage": manifest.get("recipeRouteCoverage", {})', lifecycle_source)

    def test_guided_asset_setup_prefers_cached_then_resumable_work(self):
        status = {"stage": "mappings", "updatedAt": "2026-10-13T00:00:00Z"}
        self.assertEqual(asset_setup_mode(True, status), "cached")
        self.assertEqual(asset_setup_mode(False, status), "resume")
        self.assertEqual(asset_setup_mode(False, None), "new")
        source = (ROOT / "manager" / "zanik_manager" / "app.py").read_text(encoding="utf-8")
        self.assertIn("resume_status = None if use_cached else extractor.resume_compatibility(work, root)", source)
        self.assertIn('resume=(mode == "resume")', source)
        self.assertIn('resume_activity_message(resume_status["stage"])', source)
        self.assertIn("Dragonwilds will not reopen", resume_activity_message("mappings"))
        self.assertIn("will continue with catalog and artwork export", resume_activity_message("mappings"))
        self.assertIn("completed UnrealAssetScout exports will not run again",
                      resume_activity_message("exports"))

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

    def test_journal_persistence_and_incomplete_filter(self):
        with tempfile.TemporaryDirectory() as temporary:
            store = JournalStore(Path(temporary)); journal = OperationJournal.create("profile", "install")
            store.save(journal); self.assertEqual(store.incomplete("profile")[0].operation_id, journal.operation_id)
            journal.state = "complete"; store.save(journal); self.assertEqual(store.incomplete("profile"), [])

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

    def test_release_payload_manifest_and_traversal(self):
        with tempfile.TemporaryDirectory() as temporary:
            archive = Path(temporary) / "server.zip"
            required = ["bin/libzanik.so", "bin/zanik-server", "bin/zanik-compat-analyzer", "scripts/compat-launcher.sh",
                        "scripts/verify-assets.sh", "scripts/check-game-update.sh",
                        "config/zanik.conf.example", "compatibility/compatibility.tsv", "VERSION"]
            with zipfile.ZipFile(archive, "w") as package:
                for name in required: package.writestr(name, b"0.2.0" if name == "VERSION" else b"payload")
            payload = ReleasePayload(archive)
            self.assertEqual(len(payload.manifest()["files"]), len(required))
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
                archive.writestr("asset-manifest.json", b"{}")
                archive.writestr("items/a.png", __import__('os').urandom(300))
                archive.writestr("items/b.png", __import__('os').urandom(300))
            parts,index=AssetDeployer._make_parts(bundle,root/"parts",limit=500)
            self.assertGreaterEqual(len(parts),2)
            self.assertEqual(index["bundleSha256"],content_hash(bundle.read_bytes()))
            self.assertTrue(all(part.stat().st_size < 500 for part in parts))

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
            def server(self, _): return {"server_owner": True}
            def resources(self, _): return {"current_state": self.state}
            def power(self, _, signal): self.state = "running" if signal in {"start", "restart"} else "offline"
            def create_folder(self, _, root, name): self.directories.add(str(Path(root) / name).replace("\\", "/"))
            def upload(self, _, directory, name, content): self.files[f"{directory.rstrip('/')}/{name}"] = content
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
                        names.add(name[len(prefix):]); result.append({"name":name[len(prefix):],"is_file":True})
                for name in self.directories:
                    if name.startswith(prefix) and name != path and '/' not in name[len(prefix):] and name[len(prefix):] not in names:
                        result.append({"name":name[len(prefix):],"is_file":False})
                return result
            def chmod(self, *args): pass
            def set_startup_variable(self, *args): pass
            def rename(self, _, root, pairs):
                for source, target in pairs:
                    source = f"{root.rstrip('/')}/{source}"; target = f"{root.rstrip('/')}/{target}"
                    if source in self.files:
                        self.files[target] = self.files.pop(source); continue
                    moved = [(path, target + path[len(source):]) for path in list(self.files) if path.startswith(source + "/")]
                    if not moved: raise PterodactylError("not found", 404)
                    for old, new in moved: self.files[new] = self.files.pop(old)
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
                     "config/zanik.conf.example": b"FEATURE_MAP=true\n",
                     "config/zanik-config.schema.json": b"{}", "compatibility/compatibility.tsv": b"sha\toffsets\n", "VERSION": b"0.2.0\n"}
            with zipfile.ZipFile(archive, "w") as package:
                for name, data in files.items(): package.writestr(name, data)
            client=FakeClient(); provider=PterodactylProvider(client, JournalStore(root/"journals"))
            result=provider.install("profile", "dev", "/home/container", ReleasePayload(archive), "replace-executable", True)
            game="/home/container/RSDragonwilds/Binaries/Linux/RSDragonwildsServer-Linux-Shipping"
            self.assertIn(b"ZANIK_COMPAT_LAUNCHER", client.files[game])
            self.assertEqual(client.files[game+".zanik-original"], b"\x7fELFofficial")
            self.assertEqual(result["installation"]["version"], "0.2.0")
            self.assertEqual(client.files["/home/container/zanik/compatibility.tsv"], b"sha\toffsets\n")
            original_launcher = client.files[game]
            config_path = "/home/container/zanik/config/zanik.conf"
            client.files[config_path] = b"FEATURE_MAP=false\n"
            files["scripts/compat-launcher.sh"] = b"#!/bin/sh\n# ZANIK_COMPAT_LAUNCHER=1\n# updated\n"
            files["VERSION"] = b"0.2.1\n"
            with zipfile.ZipFile(archive, "w") as package:
                for name, data in files.items(): package.writestr(name, data)
            update = provider.install("profile", "dev", "/home/container", ReleasePayload(archive), "replace-executable", True)
            self.assertEqual(update["installation"]["version"], "0.2.1")
            self.assertNotEqual(client.files[game], original_launcher)
            journal = provider.journals.load(provider.journals.root / f"{update['operationId']}.json")
            provider.rollback(journal, "dev", "/home/container")
            self.assertEqual(client.files[game], original_launcher)
            self.assertEqual(client.files[config_path], b"FEATURE_MAP=false\n")
            restored = json.loads(client.files["/home/container/zanik/install-state.json"])
            self.assertEqual(restored["version"], "0.2.0")
            provider.uninstall("profile", "dev", "/home/container")
            self.assertEqual(client.files[game], b"\x7fELFofficial")


if __name__ == "__main__":
    unittest.main()
