#!/usr/bin/env python3
"""Offline integration tests for the GitLab pipeline monitor."""

import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import threading
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer


ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "tools" / "gitlab_pipeline.py"
SHA = "a" * 40


class Handler(BaseHTTPRequestHandler):
    failed = False

    def log_message(self, _format, *_args):
        pass

    def do_GET(self):
        if self.path.startswith("/api/v4/projects/dragonwilds%2Fzanik/pipelines?"):
            body = [{"id": 42, "status": "running", "ref": "main", "sha": SHA,
                     "web_url": "https://gitlab.example/pipelines/42"}]
            self.respond(body)
        elif self.path == "/api/v4/projects/dragonwilds%2Fzanik/pipelines/42":
            status = "failed" if self.failed else "success"
            self.respond({"id": 42, "status": status, "ref": "main", "sha": SHA,
                          "web_url": "https://gitlab.example/pipelines/42"})
        elif self.path.startswith("/api/v4/projects/dragonwilds%2Fzanik/pipelines/42/jobs?"):
            status = "failed" if self.failed else "success"
            self.respond([{"id": 99, "status": status, "stage": "test",
                           "name": "test-core", "duration": 1.25}])
        elif self.path == "/api/v4/projects/dragonwilds%2Fzanik/jobs/99/trace":
            payload = b"compile output\nintentional failure\n"
            self.send_response(200)
            self.send_header("Content-Type", "text/plain")
            self.send_header("Content-Length", str(len(payload)))
            self.end_headers()
            self.wfile.write(payload)
        else:
            self.send_error(404)

    def respond(self, value):
        assert self.headers.get("PRIVATE-TOKEN") == "test-token"
        payload = json.dumps(value).encode()
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(payload)))
        self.end_headers()
        self.wfile.write(payload)


server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
thread = threading.Thread(target=server.serve_forever, daemon=True)
thread.start()

try:
    with tempfile.TemporaryDirectory(prefix="zanik-gitlab-monitor-") as temporary:
        env_file = Path(temporary) / ".env.gitlab"
        env_file.write_text(
            f"GITLAB_BASE_URL=http://127.0.0.1:{server.server_port}\n"
            "GITLAB_PROJECT=dragonwilds/zanik\n"
            "GITLAB_TOKEN=test-token\n",
            encoding="utf-8",
        )
        environment = os.environ.copy()
        for key in ("GITLAB_BASE_URL", "GITLAB_PROJECT", "GITLAB_TOKEN"):
            environment.pop(key, None)
        command = [sys.executable, str(SCRIPT), "watch", "--env-file", str(env_file),
                   "--sha", SHA, "--interval", "0.01", "--timeout", "2"]

        success = subprocess.run(command, cwd=ROOT, env=environment, text=True,
                                 capture_output=True, check=False)
        assert success.returncode == 0, success.stderr
        assert "pipeline=42 status=success" in success.stdout
        assert "test-core" in success.stdout
        assert "test-token" not in success.stdout + success.stderr

        Handler.failed = True
        failure = subprocess.run(command, cwd=ROOT, env=environment, text=True,
                                 capture_output=True, check=False)
        assert failure.returncode == 1, failure.stderr
        assert "intentional failure" in failure.stdout
        assert "test-token" not in failure.stdout + failure.stderr
finally:
    server.shutdown()
    server.server_close()
    thread.join(timeout=2)

print("GitLab pipeline monitor: ok")
