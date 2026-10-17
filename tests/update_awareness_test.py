#!/usr/bin/env python3
import hashlib
import io
import json
import sys
import tempfile
import unittest
import urllib.error
import zipfile
from pathlib import Path
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from manager.zanik_manager.updates import (GitHubReleaseChecker, parse_version, prune_update_staging,
                                           release_update_components, update_staging_cleanup_pending)
from manager.zanik_manager.game_updates import PublicSteamBuildChecker, installed_build


class Reply:
    def __init__(self, body: bytes, headers=None, status=200):
        self.body, self.headers, self.status = io.BytesIO(body), headers or {}, status
    def __enter__(self): return self
    def __exit__(self, *_): pass
    def read(self, size=-1): return self.body.read(size)


def manager_zip(version="0.2.15"):
    output = io.BytesIO()
    with zipfile.ZipFile(output, "w") as archive:
        for name in ("ZanikManager.exe", "ZanikManagerUpdater.exe", "runtime/pythonw.exe", "app/zanik_manager/app.py"):
            archive.writestr(name, b"binary")
        archive.writestr("VERSION", version + "\n")
        archive.writestr("payload/payloads.json", json.dumps({"version": version}))
    return output.getvalue()


class UpdateAwarenessTests(unittest.TestCase):
    def test_public_game_build_metadata_and_manifest(self):
        payload = {"data": {"4019830": {"depots": {"branches": {"public": {"buildid": "25811348"}}}}}}
        checker = PublicSteamBuildChecker(lambda _request, timeout=0: Reply(json.dumps(payload).encode()))
        self.assertEqual(checker.latest_build(), "25811348")
        self.assertEqual(installed_build(b'"AppState"\n{\n "buildid" "25811348"\n}'), "25811348")

    def test_public_game_build_metadata_fails_closed(self):
        checker = PublicSteamBuildChecker(lambda _request, timeout=0: Reply(b'{"status":"success"}'))
        with self.assertRaisesRegex(RuntimeError, "did not contain"):
            checker.latest_build()
        with self.assertRaisesRegex(RuntimeError, "no valid build"):
            installed_build(b'"AppState" {}')

    def test_strict_versions(self):
        self.assertEqual(parse_version("v1.2.3"), (1, 2, 3))
        for invalid in ("v1.2", "v01.2.3", "v1.2.3-beta", "latest"):
            with self.assertRaises(ValueError): parse_version(invalid)

    def test_release_update_components_keep_server_and_assets_independent(self):
        current_server = {"contractSha256": "a" * 64}
        current_assets = {"extractionRecipeVersion": 5, "compatibilityProfileVersion": "reviewed-5",
                          "extractionRecipeSha256": "b" * 64}
        manager_only = {"serverPayload": {"contractSha256": "a" * 64}, "assetContract": current_assets}
        self.assertEqual(release_update_components(manager_only, current_server, current_assets), ["Manager only"])
        assets = {**manager_only, "assetContract": {**current_assets, "extractionRecipeVersion": 6,
                                                     "extractionRecipeSha256": "c" * 64}}
        self.assertEqual(release_update_components(assets, current_server, current_assets), ["website assets"])
        both = {**assets, "serverPayload": {"contractSha256": "d" * 64}}
        self.assertEqual(release_update_components(both, current_server, current_assets),
                         ["Zanik server", "website assets"])

    def test_release_metadata_and_digest_are_validated(self):
        archive = manager_zip("0.2.16")
        digest = hashlib.sha256(archive).hexdigest()
        manifest = {"schemaVersion": 2, "version": "0.2.16", "tag": "v0.2.16",
                    "serverPayload": {"releaseVersion": "0.2.16", "contractSchemaVersion": 1,
                                      "contractSha256": "c" * 64},
                    "assetContract": {"extractionRecipeVersion": 6,
                                      "compatibilityProfileVersion": "reviewed-6",
                                      "extractionRecipeSha256": "d" * 64},
                    "files": [{"name": "zanik-manager-windows-x86_64-0.2.16.zip",
                               "size": len(archive), "sha256": digest}]}
        release = {"tag_name": "v0.2.16", "draft": False, "prerelease": False, "html_url": "https://example/release",
                   "assets": [
                       {"name": "zanik-manager-windows-x86_64-0.2.16.zip", "size": len(archive),
                        "digest": "sha256:" + digest, "browser_download_url": "https://example/manager"},
                       {"name": "release-manifest.json", "browser_download_url": "https://example/manifest"}]}
        def opener(request, timeout=0):
            if request.full_url.endswith("/latest"): return Reply(json.dumps(release).encode(), {"ETag": '"release"'})
            if request.full_url.endswith("/manifest"): return Reply(json.dumps(manifest).encode())
            if request.full_url.endswith("/manager"): return Reply(archive)
            raise AssertionError(request.full_url)
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary); checker = GitHubReleaseChecker(root / "updates", opener)
            result = checker.check("0.2.15", force=True)
            self.assertTrue(result["updateAvailable"])
            self.assertEqual(result["serverPayload"]["contractSha256"], "c" * 64)
            self.assertEqual(result["assetContract"]["extractionRecipeVersion"], 6)
            install = root / "Zanik Manager"; (install / "payload").mkdir(parents=True)
            (install / "ZanikManager.exe").write_bytes(b"old")
            (install / "ZanikManagerUpdater.exe").write_bytes(b"updater")
            (install / "payload" / "payloads.json").write_text('{}')
            staged = checker.stage(result, install)
            self.assertTrue(staged["ready"])
            self.assertTrue(Path(staged["staged"], "VERSION").is_file())

    def test_digest_conflict_fails_closed(self):
        manifest = {"schemaVersion": 2, "version": "0.2.16", "tag": "v0.2.16",
                    "files": [{"name": "zanik-manager-windows-x86_64-0.2.16.zip", "size": 1, "sha256": "a" * 64}]}
        release = {"tag_name": "v0.2.16", "draft": False, "prerelease": False,
                   "assets": [{"name": "zanik-manager-windows-x86_64-0.2.16.zip", "size": 1,
                               "digest": "sha256:" + "b" * 64, "browser_download_url": "https://example/manager"},
                              {"name": "release-manifest.json", "browser_download_url": "https://example/manifest"}]}
        def opener(request, timeout=0):
            return Reply(json.dumps(release if request.full_url.endswith("latest") else manifest).encode())
        with tempfile.TemporaryDirectory() as temporary:
            with self.assertRaisesRegex(RuntimeError, "digest"):
                GitHubReleaseChecker(Path(temporary), opener).check("0.2.15", force=True)

    def test_invalid_server_payload_metadata_fails_closed(self):
        archive = manager_zip("0.2.16"); digest = hashlib.sha256(archive).hexdigest()
        manifest = {"schemaVersion": 2, "version": "0.2.16", "tag": "v0.2.16",
                    "serverPayload": {"releaseVersion": "0.2.15", "contractSchemaVersion": 1,
                                      "contractSha256": "c" * 64},
                    "files": [{"name": "zanik-manager-windows-x86_64-0.2.16.zip",
                               "size": len(archive), "sha256": digest}]}
        release = {"tag_name": "v0.2.16", "draft": False, "prerelease": False,
                   "assets": [{"name": "zanik-manager-windows-x86_64-0.2.16.zip", "size": len(archive),
                               "browser_download_url": "https://example/manager"},
                              {"name": "release-manifest.json", "browser_download_url": "https://example/manifest"}]}
        def opener(request, timeout=0):
            return Reply(json.dumps(release if request.full_url.endswith("latest") else manifest).encode())
        with tempfile.TemporaryDirectory() as temporary:
            with self.assertRaisesRegex(RuntimeError, "server payload"):
                GitHubReleaseChecker(Path(temporary), opener).check("0.2.15", force=True)

    def test_invalid_asset_contract_metadata_fails_closed(self):
        archive = manager_zip("0.2.16"); digest = hashlib.sha256(archive).hexdigest()
        manifest = {"schemaVersion": 2, "version": "0.2.16", "tag": "v0.2.16",
                    "assetContract": {"extractionRecipeVersion": 6,
                                      "compatibilityProfileVersion": "reviewed-6",
                                      "extractionRecipeSha256": "not-a-digest"},
                    "files": [{"name": "zanik-manager-windows-x86_64-0.2.16.zip",
                               "size": len(archive), "sha256": digest}]}
        release = {"tag_name": "v0.2.16", "draft": False, "prerelease": False,
                   "assets": [{"name": "zanik-manager-windows-x86_64-0.2.16.zip", "size": len(archive),
                               "browser_download_url": "https://example/manager"},
                              {"name": "release-manifest.json", "browser_download_url": "https://example/manifest"}]}
        def opener(request, timeout=0):
            return Reply(json.dumps(release if request.full_url.endswith("latest") else manifest).encode())
        with tempfile.TemporaryDirectory() as temporary:
            with self.assertRaisesRegex(RuntimeError, "asset contract"):
                GitHubReleaseChecker(Path(temporary), opener).check("0.2.15", force=True)

    def test_recent_cache_is_recomputed_for_the_running_version(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary); root.mkdir(exist_ok=True)
            (root / "manager-update.json").write_text(json.dumps({
                "validated": True, "checkedAt": "2999-01-01T00:00:00Z",
                "latestVersion": "0.2.16", "currentVersion": "0.2.15", "updateAvailable": True,
            }))
            checker = GitHubReleaseChecker(root, lambda *_args, **_kwargs: self.fail("recent cache should avoid a request"))
            result = checker.check("0.2.16")
            self.assertFalse(result["updateAvailable"])
            self.assertEqual(result["currentVersion"], "0.2.16")

    def test_safe_zip_rejects_traversal(self):
        data = io.BytesIO()
        with zipfile.ZipFile(data, "w") as archive: archive.writestr("../escape", b"x")
        digest = hashlib.sha256(data.getvalue()).hexdigest()
        result = {"validated": True, "updateAvailable": True, "latestVersion": "0.2.16", "currentVersion": "0.2.15",
                  "manager": {"name": "manager.zip", "url": "https://example/manager", "size": len(data.getvalue()), "sha256": digest}}
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary); install = root / "current"; (install / "payload").mkdir(parents=True)
            (install / "ZanikManager.exe").write_bytes(b"x"); (install / "payload" / "payloads.json").write_text('{}')
            with self.assertRaisesRegex(RuntimeError, "unsafe"):
                GitHubReleaseChecker(root / "updates", lambda *_args, **_kwargs: Reply(data.getvalue())).stage(result, install)

    def test_updater_launches_outside_portable_install(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            updates = root / "LocalAppData" / "updates"
            updates.mkdir(parents=True)
            updater = updates / "ZanikManagerUpdater.exe"
            updater.write_bytes(b"updater")
            journal = updates / "self-update-journal.json"
            journal.write_text(json.dumps({
                "source": str(root / "staged"), "target": str(root / "portable"),
                "backup": str(root / "backup"), "handshake": str(updates / "handshake.json"),
            }))
            staged = {"ready": True, "updater": str(updater), "journal": str(journal)}
            with mock.patch("manager.zanik_manager.updates.subprocess.Popen") as popen:
                GitHubReleaseChecker.launch(staged)
            self.assertEqual(popen.call_args.kwargs["cwd"], str(updates.resolve()))
            self.assertTrue(popen.call_args.kwargs["close_fds"])
            self.assertEqual(Path(popen.call_args.args[0][0]), updater.resolve())

    def test_native_updater_leaves_install_directory_and_retries_rename(self):
        source = (ROOT / "manager" / "updater.c").read_text(encoding="utf-8")
        self.assertIn("SetCurrentDirectoryW(updater_path)", source)
        self.assertIn("move_with_retry(target, backup", source)
        self.assertIn('L"move-current-to-backup"', source)

    def test_completed_update_staging_is_pruned_but_unrecognized_data_is_preserved(self):
        with tempfile.TemporaryDirectory() as temporary:
            updates = Path(temporary)
            (updates / "self-update-journal.json.status").write_text(
                json.dumps({"schemaVersion": 1, "state": "complete"}), encoding="utf-8")
            for version in ("0.2.16", "0.2.32"):
                verified = updates / version / "verified"
                (verified / "payload").mkdir(parents=True)
                (verified / "ZanikManager.exe").write_bytes(b"manager")
                (verified / "VERSION").write_text(version, encoding="utf-8")
                (verified / "payload" / "payloads.json").write_text("{}", encoding="utf-8")
            unknown = updates / "operator-files"; unknown.mkdir(); (unknown / "keep.txt").write_text("keep")
            count, reclaimed = prune_update_staging(updates)
            self.assertEqual(count, 2)
            self.assertGreater(reclaimed, 0)
            self.assertTrue((unknown / "keep.txt").is_file())

    def test_update_staging_is_not_pruned_before_successful_updater_status(self):
        with tempfile.TemporaryDirectory() as temporary:
            updates = Path(temporary); verified = updates / "0.2.32" / "verified"
            (verified / "payload").mkdir(parents=True)
            (verified / "ZanikManager.exe").write_bytes(b"manager")
            (verified / "VERSION").write_text("0.2.32", encoding="utf-8")
            (verified / "payload" / "payloads.json").write_text("{}", encoding="utf-8")
            self.assertEqual(prune_update_staging(updates), (0, 0))
            self.assertTrue(verified.is_dir())
            self.assertTrue(update_staging_cleanup_pending(updates))
            (updates / "self-update-journal.json.status").write_text(json.dumps({"state": "complete"}))
            self.assertFalse(update_staging_cleanup_pending(updates))


if __name__ == "__main__": unittest.main()
