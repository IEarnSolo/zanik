#!/usr/bin/env python3
import importlib.util, json, os, subprocess, sys, tempfile, threading
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def module(name, path):
    spec = importlib.util.spec_from_file_location(name, ROOT / path)
    value = importlib.util.module_from_spec(spec); spec.loader.exec_module(value); return value

contract = module("release_contract", "tools/release_contract.py")
gitlab = module("publish_gitlab_package", "tools/publish_gitlab_package.py")
sys.path.insert(0, str(ROOT / "tools"))
gitlab_notes = module("update_gitlab_release_notes", "tools/update_gitlab_release_notes.py")

current_version = contract.read_version(ROOT / "VERSION")
contract.validate(f"v{current_version}", current_version)
assert "Zanik Manager" in contract.release_notes(ROOT / "CHANGELOG.md", current_version)
package_script = (ROOT / "scripts/package-release.sh").read_text()
release_validator = (ROOT / "tools/validate_release.py").read_text()
ci = (ROOT / ".gitlab-ci.yml").read_text()
release_guide = (ROOT / "docs/releasing.md").read_text()
assert "python-embed-windows-x86_64" in package_script
assert "unreal-asset-scout-windows-x86_64" in package_script
assert "ue4ss-windows-x86_64" in package_script
assert "zanik-manager-windows-x86_64-$VERSION.zip" in package_script
assert "zanik-asset-tools-linux-x86_64-$VERSION.tar.gz" in package_script
assert "prohibited_archive_path(name)" in release_validator
assert "--require-annotated-tag" in ci and "--require-ref refs/remotes/origin/release-main" in ci
assert "origin +main:refs/remotes/origin/release-main" in ci
assert "expire_in: 7 days" in ci and "Branch artifacts expire after 7 days." in release_guide
for tag in (current_version, "v0.1", "v1.2.3-rc1", "v9.9.9"):
    try:
        contract.validate(tag, current_version); raise AssertionError(tag)
    except ValueError: pass

with tempfile.TemporaryDirectory() as temporary:
    repository = Path(temporary)
    subprocess.run(["git", "init", "-q"], cwd=repository, check=True)
    subprocess.run(["git", "config", "user.name", "Release Test"], cwd=repository, check=True)
    subprocess.run(["git", "config", "user.email", "release-test@example.invalid"], cwd=repository, check=True)
    (repository / "candidate").write_text("one\n")
    subprocess.run(["git", "add", "candidate"], cwd=repository, check=True)
    subprocess.run(["git", "commit", "-q", "-m", "candidate"], cwd=repository, check=True)
    commit = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=repository, text=True).strip()
    subprocess.run(["git", "tag", "-a", "v1.2.3", "-m", "Zanik 1.2.3"], cwd=repository, check=True)
    contract.require_annotated_tag(repository, "v1.2.3")
    contract.require_ref_contains(repository, "HEAD", commit)
    subprocess.run(["git", "tag", "v1.2.4"], cwd=repository, check=True)
    try:
        contract.require_annotated_tag(repository, "v1.2.4"); raise AssertionError("lightweight tag accepted")
    except ValueError: pass
    (repository / "candidate").write_text("two\n")
    subprocess.run(["git", "commit", "-qam", "later"], cwd=repository, check=True)
    later = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=repository, text=True).strip()
    contract.require_ref_contains(repository, "HEAD", commit)
    try:
        contract.require_ref_contains(repository, "v1.2.3", later); raise AssertionError("uncontained commit accepted")
    except ValueError: pass


class FakeGitLabReleaseClient:
    def __init__(self, release=None):
        self.release = release
        self.calls = []

    def request(self, method, path, body=None):
        self.calls.append((method, path, body))
        if method == "GET":
            if self.release is None: return 404, b"{}"
            return 200, json.dumps(self.release).encode()
        if method == "PUT":
            payload = json.loads(body)
            self.release = {**self.release, **payload}
            return 200, json.dumps(self.release).encode()
        raise AssertionError((method, path))


gitlab_release = {"tag_name": "v1.2.3", "name": "Zanik 1.2.3",
                  "description": "Old notes", "_links": {"self": "https://gitlab.example/release"}}
gitlab_client = FakeGitLabReleaseClient(gitlab_release)
gitlab_note_result = gitlab_notes.update_release_notes(
    gitlab_client, "v1.2.3", "1.2.3", "Correct notes\n")
assert gitlab_note_result["status"] == "updated"
assert json.loads(next(call[2] for call in gitlab_client.calls if call[0] == "PUT")) == {
    "name": "Zanik 1.2.3", "description": "Correct notes\n"}
try:
    gitlab_notes.update_release_notes(FakeGitLabReleaseClient(), "v1.2.3", "1.2.3", "notes")
    raise AssertionError("missing GitLab release accepted")
except RuntimeError: pass

class Handler(BaseHTTPRequestHandler):
    store = {}; curse = []; next_id = 700
    def log_message(self, *_): pass
    def do_GET(self):
        if self.path not in self.store: self.send_response(404); self.end_headers(); return
        self.send_response(200); self.end_headers(); self.wfile.write(self.store[self.path])
    def do_PUT(self):
        self.store[self.path] = self.rfile.read(int(self.headers["Content-Length"]))
        self.send_response(201); self.end_headers(); self.wfile.write(b"{}")
    def do_POST(self):
        data = self.rfile.read(int(self.headers["Content-Length"]))
        assert self.headers.get("X-Api-Token") == "test-secret-token" and b"test-secret-token" not in data
        if b'"parentFileID"' in data:
            body = b'{"errorCode":1011,"errorMessage":"Additional file uploads are not allowed on this site. You can only upload normal files."}'
            self.send_response(400); self.send_header("Content-Length", str(len(body))); self.end_headers(); self.wfile.write(body); return
        self.curse.append(data); self.__class__.next_id += 1
        body = json.dumps({"id": self.next_id}).encode()
        self.send_response(200); self.send_header("Content-Length", str(len(body))); self.end_headers(); self.wfile.write(body)

server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
threading.Thread(target=server.serve_forever, daemon=True).start()
base = f"http://127.0.0.1:{server.server_port}"
with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary); version = "0.1.0"
    names = [f"zanik-server-linux-x86_64-{version}.zip", f"zanik-server-linux-x86_64-{version}.tar.gz", f"zanik-asset-tools-linux-x86_64-{version}.tar.gz", f"zanik-asset-tools-windows-x86_64-{version}.zip", f"zanik-manager-windows-x86_64-{version}.zip"]
    for index, name in enumerate(names): (root / name).write_bytes(f"archive-{index}".encode())
    (root / "SHA256SUMS").write_text("checksums\n"); (root / "release-manifest.json").write_text("{}\n")
    assert len(gitlab.publish(root, version, base + "/api/v4", "355", "job-token")) == 7
    assert len(Handler.store) == 7
    gitlab.publish(root, version, base + "/api/v4", "355", "job-token")
    (root / names[0]).write_bytes(b"different")
    try: gitlab.publish(root, version, base + "/api/v4", "355", "job-token"); raise AssertionError("conflict accepted")
    except RuntimeError as error: assert "differs" in str(error)
    (root / names[0]).write_bytes(b"archive-0")
    report = root / "curseforge-report.json"; env = os.environ.copy(); env["CURSEFORGE_API_TOKEN"] = "test-secret-token"
    command = [sys.executable, str(ROOT / "tools/curseforge_upload.py"), "--directory", str(root), "--version", version, "--tag", "v0.1.0", "--changelog", str(ROOT / "CHANGELOG.md"), "--report", str(report), "--project-id", "123", "--base-url", base]
    subprocess.run(command + ["--dry-run"], check=True, env=env)
    assert json.loads(report.read_text())["status"] == "validated"
    subprocess.run(command, check=True, env=env)
    result = json.loads(report.read_text())
    assert result["status"] == "submitted" and len(Handler.curse) == 4
    assert result["schemaVersion"] == 2 and result["publicationMode"] == "normal-files"
    assert all(b'"parentFileID"' not in request for request in Handler.curse)
    assert all(b'"gameVersionNames":["1.0"]' in request for request in Handler.curse)
    assert "test-secret-token" not in report.read_text()
    Handler.curse.clear()
    subprocess.run(command + ["--primary-file-id", "701", "--skip-child", "linux", "--skip-child", "manager"], check=True, env=env)
    recovered = json.loads(report.read_text())
    assert recovered["files"]["primary"]["status"] == "reused"
    assert recovered["files"]["linux"]["status"] == "skipped"
    assert recovered["files"]["windows"]["status"] == "uploaded"
    assert recovered["files"]["manager"]["status"] == "skipped"
    assert len(Handler.curse) == 1 and b'"parentFileID"' not in Handler.curse[0]
server.shutdown(); server.server_close()
print("release automation tests passed")
