#!/usr/bin/env python3
import importlib.util
import json
import subprocess
import sys
import tempfile
from pathlib import Path
from urllib.parse import parse_qs, urlsplit

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("publish_github_release", ROOT / "tools/publish_github_release.py")
github = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(github)
NOTES_SPEC = importlib.util.spec_from_file_location(
    "update_github_release_notes", ROOT / "tools/update_github_release_notes.py")
notes_updater = importlib.util.module_from_spec(NOTES_SPEC)
sys.path.insert(0, str(ROOT / "tools"))
NOTES_SPEC.loader.exec_module(notes_updater)


def fixture(root: Path, version: str = "1.2.3") -> list[Path]:
    names = [
        f"zanik-server-linux-x86_64-{version}.tar.gz",
        f"zanik-server-linux-x86_64-{version}.zip",
        f"zanik-asset-tools-linux-x86_64-{version}.tar.gz",
        f"zanik-asset-tools-windows-x86_64-{version}.zip",
        f"zanik-manager-windows-x86_64-{version}.zip",
    ]
    for index, name in enumerate(names):
        (root / name).write_bytes(f"file-{index}".encode())
    (root / "SHA256SUMS").write_text("checksums\n", encoding="utf-8")
    (root / "release-manifest.json").write_text("{}\n", encoding="utf-8")
    (root / "release-notes.md").write_text("Release notes\n", encoding="utf-8")
    return github.release_files(root, version)


class FakeClient:
    def __init__(self, *, commit="abc123", release=None, tag_status=200):
        self.commit = commit
        self.release = release
        self.tag_status = tag_status
        self.assets = []
        self.calls = []
        self.upload_results = []
        self.create_result = 201
        self.next_id = 80

    def release_value(self):
        return self.release or {
            "id": self.next_id,
            "tag_name": "v1.2.3",
            "html_url": "https://github.example/release/v1.2.3",
            "upload_url": "https://uploads.example/repos/IEarnSolo/zanik/releases/80/assets{?name,label}",
        }

    def request(self, method, path, *, body=None, content_type="application/json"):
        self.calls.append((method, path, body, content_type))
        if "/commits/" in path:
            return self.tag_status, {}, json.dumps({"sha": self.commit}).encode()
        if "/releases/tags/" in path:
            if self.release is None:
                return 404, {}, b"{}"
            return 200, {}, json.dumps(self.release).encode()
        if method == "POST" and path.endswith("/releases"):
            self.release = self.release_value()
            if self.create_result == "timeout-after-create":
                raise TimeoutError("ambiguous release creation")
            return 201, {}, json.dumps(self.release).encode()
        if method == "PATCH" and "/releases/" in path:
            return 200, {}, json.dumps(self.release_value()).encode()
        if method == "GET" and "/assets?" in path:
            return 200, {}, json.dumps(self.assets).encode()
        if method == "POST" and "uploads.example" in path:
            name = parse_qs(urlsplit(path).query)["name"][0]
            source_value = next(call[2] for call in reversed(self.calls) if call[0] == "POST" and call[1] == path)
            source = source_value.read_bytes() if isinstance(source_value, Path) else source_value
            result = self.upload_results.pop(0) if self.upload_results else 201
            if result == "timeout-after-upload":
                self.assets.append({"name": name, "digest": "sha256:" + __import__("hashlib").sha256(source).hexdigest()})
                raise TimeoutError("secret response should never be shown")
            if result == 201:
                self.assets.append({"name": name, "digest": "sha256:" + __import__("hashlib").sha256(source).hexdigest()})
            return result, ({"retry-after": "0"} if result == 429 else {}), b"sensitive upstream body"
        raise AssertionError((method, path))


with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    paths = fixture(root)
    assert len(paths) == 7
    assert [path.name for path in paths][-2:] == ["SHA256SUMS", "release-manifest.json"]

    missing = FakeClient(tag_status=404)
    try:
        github.resolve_tag(missing, "IEarnSolo/zanik", "v1.2.3", "abc123", attempts=1, delay=0)
        raise AssertionError("missing tag accepted")
    except RuntimeError as error:
        assert "HTTP 404" in str(error)

    mismatch = FakeClient(commit="different")
    try:
        github.resolve_tag(mismatch, "IEarnSolo/zanik", "v1.2.3", "abc123", attempts=1, delay=0)
        raise AssertionError("mismatched tag accepted")
    except RuntimeError as error:
        assert "different commit" in str(error)

    created = FakeClient()
    result = github.publish(root, "1.2.3", "v1.2.3", "abc123", root / "release-notes.md", "IEarnSolo/zanik", created, tag_attempts=1, tag_delay=0)
    assert result["status"] == "published" and len(result["assets"]) == 7
    assert len(created.assets) == 7
    assert any(call[0] == "POST" and call[1].endswith("/releases") for call in created.calls)

    create_timeout = FakeClient()
    create_timeout.create_result = "timeout-after-create"
    recovered_release = github.ensure_release(create_timeout, "IEarnSolo/zanik", "v1.2.3", "1.2.3", "abc123", "notes")
    assert recovered_release["id"] == 80

    existing_release = created.release_value()
    existing = FakeClient(release=existing_release)
    existing.assets = [
        {"name": path.name, "digest": "sha256:" + github.sha256(path)} for path in paths
    ]
    github.publish(root, "1.2.3", "v1.2.3", "abc123", root / "release-notes.md", "IEarnSolo/zanik", existing, tag_attempts=1, tag_delay=0)
    assert any(call[0] == "PATCH" for call in existing.calls)
    assert not any(call[0] == "POST" and "uploads.example" in call[1] for call in existing.calls)

    note_client = FakeClient(release=existing_release)
    note_result = notes_updater.update_release_notes(
        note_client, "IEarnSolo/zanik", "v1.2.3", "1.2.3", "Complete notes\n")
    assert note_result["status"] == "updated"
    note_patch = next(call for call in note_client.calls if call[0] == "PATCH")
    assert json.loads(note_patch[2]) == {"name": "Zanik 1.2.3", "body": "Complete notes\n"}

    conflict = FakeClient(release=existing_release)
    conflict.assets = [{"name": paths[0].name, "digest": "sha256:different"}]
    try:
        github.upload_asset(conflict, "IEarnSolo/zanik", existing_release, paths[0], github.sha256(paths[0]), delay=0)
        raise AssertionError("conflicting asset accepted")
    except RuntimeError as error:
        assert "differs" in str(error)

    ambiguous = FakeClient(release=existing_release)
    ambiguous.upload_results = ["timeout-after-upload"]
    github.upload_asset(ambiguous, "IEarnSolo/zanik", existing_release, paths[0], github.sha256(paths[0]), delay=0)
    assert len(ambiguous.assets) == 1

    for transient in (429, 500, 503):
        retry = FakeClient(release=existing_release)
        retry.upload_results = [transient, 201]
        github.upload_asset(retry, "IEarnSolo/zanik", existing_release, paths[0], github.sha256(paths[0]), delay=0)

    for permanent in (401, 403, 404, 422):
        failed = FakeClient(release=existing_release)
        failed.upload_results = [permanent]
        try:
            github.upload_asset(failed, "IEarnSolo/zanik", existing_release, paths[0], github.sha256(paths[0]), delay=0)
            raise AssertionError(f"HTTP {permanent} accepted")
        except RuntimeError as error:
            assert "sensitive upstream body" not in str(error)

    report = root / "dry-run.json"
    subprocess.run([
        sys.executable, str(ROOT / "tools/publish_github_release.py"),
        "--directory", str(root), "--version", "1.2.3", "--tag", "v1.2.3",
        "--commit", "abc123", "--notes", str(root / "release-notes.md"),
        "--report", str(report), "--dry-run",
    ], check=True)
    dry = json.loads(report.read_text(encoding="utf-8"))
    assert dry["status"] == "validated" and len(dry["assets"]) == 7

    ci = (ROOT / ".gitlab-ci.yml").read_text(encoding="utf-8")
    assert "publish-github-release:" in ci
    assert "update-github-release-notes:" in ci
    assert "RELEASE_NOTES_TAG" in ci
    assert "name: github-production" in ci
    assert "GITHUB_RELEASE_TOKEN" not in ci

print("GitHub release tests passed")
