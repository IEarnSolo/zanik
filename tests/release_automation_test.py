#!/usr/bin/env python3
import importlib.util, json, os, subprocess, tempfile, threading
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def module(name, path):
    spec = importlib.util.spec_from_file_location(name, ROOT / path)
    value = importlib.util.module_from_spec(spec); spec.loader.exec_module(value); return value

contract = module("release_contract", "tools/release_contract.py")
gitlab = module("publish_gitlab_package", "tools/publish_gitlab_package.py")

assert contract.read_version(ROOT / "VERSION") == "0.1.0"
contract.validate("v0.1.0", "0.1.0")
assert "Linux x86-64" in contract.release_notes(ROOT / "CHANGELOG.md", "0.1.0")
for tag in ("0.1.0", "v0.1", "v1.2.3-rc1", "v0.1.1"):
    try:
        contract.validate(tag, "0.1.0"); raise AssertionError(tag)
    except ValueError: pass

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
        self.curse.append(data); self.__class__.next_id += 1
        body = json.dumps({"id": self.next_id}).encode()
        self.send_response(200); self.send_header("Content-Length", str(len(body))); self.end_headers(); self.wfile.write(body)

server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
threading.Thread(target=server.serve_forever, daemon=True).start()
base = f"http://127.0.0.1:{server.server_port}"
with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary); version = "0.1.0"
    names = [f"zanik-server-linux-x86_64-{version}.zip", f"zanik-server-linux-x86_64-{version}.tar.gz", f"zanik-asset-tools-linux-x86_64-{version}.tar.xz", f"zanik-asset-tools-windows-x86_64-{version}.zip"]
    for index, name in enumerate(names): (root / name).write_bytes(f"archive-{index}".encode())
    (root / "SHA256SUMS").write_text("checksums\n"); (root / "release-manifest.json").write_text("{}\n")
    assert len(gitlab.publish(root, version, base + "/api/v4", "355", "job-token")) == 6
    assert len(Handler.store) == 6
    gitlab.publish(root, version, base + "/api/v4", "355", "job-token")
    (root / names[0]).write_bytes(b"different")
    try: gitlab.publish(root, version, base + "/api/v4", "355", "job-token"); raise AssertionError("conflict accepted")
    except RuntimeError as error: assert "differs" in str(error)
    (root / names[0]).write_bytes(b"archive-0")
    report = root / "curseforge-report.json"; env = os.environ.copy(); env["CURSEFORGE_API_TOKEN"] = "test-secret-token"
    command = ["python3", str(ROOT / "tools/curseforge_upload.py"), "--directory", str(root), "--version", version, "--tag", "v0.1.0", "--changelog", str(ROOT / "CHANGELOG.md"), "--report", str(report), "--project-id", "123", "--base-url", base]
    subprocess.run(command + ["--dry-run"], check=True, env=env)
    assert json.loads(report.read_text())["status"] == "validated"
    subprocess.run(command, check=True, env=env)
    result = json.loads(report.read_text())
    assert result["status"] == "submitted" and len(Handler.curse) == 3
    assert "test-secret-token" not in report.read_text()
server.shutdown(); server.server_close()
print("release automation tests passed")
