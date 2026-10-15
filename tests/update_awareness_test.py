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

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from manager.zanik_manager.updates import GitHubReleaseChecker, parse_version


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
    def test_strict_versions(self):
        self.assertEqual(parse_version("v1.2.3"), (1, 2, 3))
        for invalid in ("v1.2", "v01.2.3", "v1.2.3-beta", "latest"):
            with self.assertRaises(ValueError): parse_version(invalid)

    def test_release_metadata_and_digest_are_validated(self):
        archive = manager_zip("0.2.16")
        digest = hashlib.sha256(archive).hexdigest()
        manifest = {"schemaVersion": 2, "version": "0.2.16", "tag": "v0.2.16",
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


if __name__ == "__main__": unittest.main()
