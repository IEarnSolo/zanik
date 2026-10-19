from __future__ import annotations

import json
import http.client
import mimetypes
import time
import urllib.error
import urllib.parse
import urllib.request
import uuid
from dataclasses import dataclass
from typing import Callable


class PterodactylError(RuntimeError):
    def __init__(self, message: str, status: int | None = None):
        super().__init__(message)
        self.status = status


@dataclass
class Response:
    status: int
    body: bytes
    headers: dict[str, str]

    def json(self):
        return json.loads(self.body or b"{}")


Transport = Callable[[str, str, dict[str, str], bytes | None, float], Response]
CONTAINER_ROOT = "/home/container"
USER_AGENT = "Zanik-Manager/0.2.15"


def api_file_path(path: str) -> str:
    """Translate the host-visible server root to Pterodactyl's virtual file root."""
    normalized = path.replace("\\", "/").rstrip("/") or "/"
    if normalized == CONTAINER_ROOT:
        return "/"
    if normalized.startswith(CONTAINER_ROOT + "/"):
        return normalized[len(CONTAINER_ROOT):]
    return normalized


def urllib_transport(method: str, url: str, headers: dict[str, str], body: bytes | None, timeout: float) -> Response:
    request = urllib.request.Request(url, method=method, headers=headers, data=body)
    try:
        with urllib.request.urlopen(request, timeout=timeout) as reply:
            return Response(reply.status, reply.read(), dict(reply.headers))
    except urllib.error.HTTPError as error:
        return Response(error.code, error.read(4096), dict(error.headers))
    except urllib.error.URLError as error:
        raise PterodactylError(f"Could not reach the panel: {error.reason}") from error


def direct_transport(method: str, url: str, headers: dict[str, str], body: bytes | None, timeout: float) -> Response:
    """Send a signed node request without urllib redirect or proxy rewriting."""
    parsed = urllib.parse.urlsplit(url)
    connection_type = http.client.HTTPSConnection if parsed.scheme == "https" else http.client.HTTPConnection
    connection = connection_type(parsed.hostname, parsed.port, timeout=timeout)
    target = urllib.parse.urlunsplit(("", "", parsed.path or "/", parsed.query, ""))
    request_headers = dict(headers)
    if body is not None:
        request_headers["Content-Length"] = str(len(body))
    try:
        connection.request(method, target, body=body, headers=request_headers)
        reply = connection.getresponse()
        return Response(reply.status, reply.read(4096), dict(reply.headers))
    except OSError as error:
        raise PterodactylError(f"Could not reach the server node: {error}") from error
    finally:
        connection.close()


class PterodactylClient:
    def __init__(self, origin: str, token: str, transport: Transport = urllib_transport,
                 retry_seconds: float = 60.0, cancelled: Callable[[], bool] = lambda: False,
                 status: Callable[[str], None] = lambda _message: None):
        self.origin = origin.rstrip("/")
        self.token = token
        self.transport = transport
        self.retry_seconds = retry_seconds
        self.cancelled = cancelled
        self.status = status

    def request(self, method: str, path: str, *, query: dict | None = None, data=None,
                raw: bytes | None = None, timeout: float = 120.0, retry_522: bool = True) -> Response:
        url = self.origin + path
        if query:
            url += "?" + urllib.parse.urlencode(query)
        headers = {"Accept": "application/json", "Authorization": f"Bearer {self.token}",
                   "User-Agent": USER_AGENT}
        body = raw
        if data is not None:
            body = json.dumps(data, separators=(",", ":")).encode()
            headers["Content-Type"] = "application/json"
        elif raw is not None:
            headers["Content-Type"] = "application/octet-stream"
        while True:
            response = self.transport(method, url, headers, body, timeout)
            if response.status != 522 or not retry_522:
                break
            if self.cancelled():
                raise PterodactylError("Operation cancelled while waiting for the panel", 522)
            self.status("The panel is temporarily unavailable (HTTP 522). Retrying in 60 seconds…")
            time.sleep(self.retry_seconds)
        if response.status < 200 or response.status >= 300:
            detail = ""
            try:
                errors = response.json().get("errors", [])
                detail = errors[0].get("detail", "") if errors else ""
            except (ValueError, KeyError, TypeError):
                pass
            raise PterodactylError(detail or f"Panel returned HTTP {response.status}", response.status)
        return response

    def account(self) -> dict:
        return self.request("GET", "/api/client/account").json().get("attributes", {})

    def servers(self) -> list[dict]:
        result=[]
        for row in self.request("GET", "/api/client").json().get("data", []):
            attributes=dict(row.get("attributes", row))
            relationships=row.get("relationships", attributes.get("relationships", {}))
            permission_data=relationships.get("permissions", {}).get("data", {}) if isinstance(relationships, dict) else {}
            permission_attributes=permission_data.get("attributes", {}) if isinstance(permission_data, dict) else {}
            attributes["_permissions"]=permission_attributes.get("permissions", attributes.get("permissions", []))
            result.append(attributes)
        return result

    def server(self, server_id: str) -> dict:
        return self.request("GET", f"/api/client/servers/{server_id}").json().get("attributes", {})

    def resources(self, server_id: str) -> dict:
        return self.request("GET", f"/api/client/servers/{server_id}/resources").json().get("attributes", {})

    def startup(self, server_id: str) -> dict:
        return self.request("GET", f"/api/client/servers/{server_id}/startup").json()

    def startup_variable(self, server_id: str, key: str) -> dict | None:
        """Return one normalized startup variable without exposing unrelated values."""
        for row in self.startup(server_id).get("data", []):
            item = row.get("attributes", row)
            if item.get("env_variable") == key:
                return {
                    "key": key,
                    "value": str(item.get("server_value", "")),
                    "default": str(item.get("default_value", "")),
                    "editable": bool(item.get("is_editable", False)),
                    "rules": str(item.get("rules", "")),
                }
        return None

    def wrapper_adapter(self, server_id: str) -> dict | None:
        startup = self.startup(server_id)
        command = str(startup.get("meta", {}).get("startup_command", ""))
        for row in startup.get("data", []):
            item = row.get("attributes", row)
            key = item.get("env_variable", "")
            if key not in {"SERVER_BINARY", "EXECUTABLE", "BINARY"} or not item.get("is_editable", False):
                continue
            if f"{{{{{key}}}}}" in command or f"${{{key}}}" in command or f"${key}" in command:
                return {"key": key, "value": "zanik/bin/zanik-server", "previous": item.get("server_value", ""),
                        "startupCommand": command}
        return None

    def set_startup_variable(self, server_id: str, key: str, value: str) -> dict:
        return self.request("PUT", f"/api/client/servers/{server_id}/startup/variable",
                            data={"key": key, "value": value}).json()

    def power(self, server_id: str, signal: str) -> None:
        if signal not in {"start", "stop", "restart", "kill"}:
            raise ValueError("Invalid power signal")
        self.request("POST", f"/api/client/servers/{server_id}/power", data={"signal": signal})

    def command(self, server_id: str, command: str) -> None:
        self.request("POST", f"/api/client/servers/{server_id}/command", data={"command": command})

    def list_files(self, server_id: str, directory: str = "/") -> list[dict]:
        return [row.get("attributes", row) for row in self.request("GET", f"/api/client/servers/{server_id}/files/list",
                                                                    query={"directory": api_file_path(directory)}).json().get("data", [])]

    def read_file(self, server_id: str, path: str) -> bytes:
        try:
            return self.request("GET", f"/api/client/servers/{server_id}/files/contents",
                                query={"file": api_file_path(path)}).body
        except PterodactylError as error:
            if error.status not in {500, 502, 503, 504}:
                raise
            return self.download_file(server_id, path)

    def download_file(self, server_id: str, path: str) -> bytes:
        details = self.request("GET", f"/api/client/servers/{server_id}/files/download",
                               query={"file": api_file_path(path)}).json().get("attributes", {})
        url = details.get("url")
        if not url:
            raise PterodactylError("Panel did not return a file download URL")
        response = self.transport("GET", url, {"Accept": "application/octet-stream",
                                                "User-Agent": USER_AGENT}, None, 300.0)
        if response.status < 200 or response.status >= 300:
            raise PterodactylError(f"File download failed with HTTP {response.status}", response.status)
        return response.body

    def write_file(self, server_id: str, path: str, content: bytes) -> None:
        self.request("POST", f"/api/client/servers/{server_id}/files/write", query={"file": api_file_path(path)}, raw=content)

    def create_folder(self, server_id: str, root: str, name: str) -> None:
        self.request("POST", f"/api/client/servers/{server_id}/files/create-folder", data={"root": api_file_path(root), "name": name})

    def rename(self, server_id: str, root: str, pairs: list[tuple[str, str]]) -> None:
        self.request("PUT", f"/api/client/servers/{server_id}/files/rename",
                     data={"root": api_file_path(root), "files": [{"from": source, "to": target} for source, target in pairs]})

    def delete(self, server_id: str, root: str, files: list[str]) -> None:
        self.request("POST", f"/api/client/servers/{server_id}/files/delete", data={"root": api_file_path(root), "files": files})

    def chmod(self, server_id: str, root: str, files: list[tuple[str, int]]) -> None:
        self.request("POST", f"/api/client/servers/{server_id}/files/chmod",
                     data={"root": api_file_path(root),
                           "files": [{"file": path, "mode": format(mode, "o")} for path, mode in files]})

    def decompress(self, server_id: str, root: str, file: str) -> None:
        self.request("POST", f"/api/client/servers/{server_id}/files/decompress", data={"root": api_file_path(root), "file": file})

    def upload(self, server_id: str, directory: str, filename: str, content: bytes) -> None:
        details = self.request("GET", f"/api/client/servers/{server_id}/files/upload").json().get("attributes", {})
        url = details.get("url")
        if not url:
            raise PterodactylError("Panel did not return a file upload URL")
        boundary = "zanik-" + uuid.uuid4().hex
        content_type = mimetypes.guess_type(filename)[0] or "application/octet-stream"
        prefix = (f"--{boundary}\r\nContent-Disposition: form-data; name=\"files\"; filename=\"{filename}\"\r\n"
                  f"Content-Type: {content_type}\r\n\r\n").encode()
        suffix = f"\r\n--{boundary}--\r\n".encode()
        separator = "&" if "?" in url else "?"
        upload_transport = direct_transport if self.transport is urllib_transport else self.transport
        response = upload_transport("POST", url + separator + urllib.parse.urlencode({"directory": api_file_path(directory)}),
                                    {"Accept": "*/*", "Content-Type": f"multipart/form-data; boundary={boundary}",
                                     "User-Agent": USER_AGENT},
                                    prefix + content + suffix, 300.0)
        if response.status < 200 or response.status >= 300:
            raise PterodactylError(f"File upload failed with HTTP {response.status}", response.status)
