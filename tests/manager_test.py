#!/usr/bin/env python3
import json
import sys
import tempfile
import unittest
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from manager.zanik_manager.assets import AssetDeployer
from manager.zanik_manager.cache import BundleCache
from manager.zanik_manager.config import ConfigError, ConfigModel, content_hash
from manager.zanik_manager.journal import JournalStore, OperationJournal
from manager.zanik_manager.lifecycle import (PterodactylProvider, ReleasePayload, is_exact_legacy_launcher,
                                             safe_archive_name, validate_server_root)
from manager.zanik_manager.profiles import ProfileStore, ServerProfile, is_local_origin, normalize_origin
from manager.zanik_manager.pterodactyl import PterodactylClient, PterodactylError, Response, api_file_path
from manager.zanik_manager.security import credential_target, redact
from manager.zanik_manager.support import create_support_bundle
from tools.prune_pyside_runtime import KEEP_FILES, prune

class ManagerTests(unittest.TestCase):
    def test_origin_and_http_policy_helpers(self):
        self.assertEqual(normalize_origin("EXAMPLE.com/"), "https://example.com")
        self.assertEqual(normalize_origin("http://localhost:8080/"), "http://localhost:8080")
        self.assertTrue(is_local_origin("http://127.0.0.1"))
        self.assertFalse(is_local_origin("http://panel.example.com"))
        with self.assertRaises(ValueError): normalize_origin("ftp://example.com")

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
        self.assertFalse(values["FEATURE_ADMIN_TELEPORT"])
        self.assertEqual(model.parse(model.render(values)), values)
        with self.assertRaises(ConfigError): model.parse("UNKNOWN=true\n")
        with self.assertRaises(ConfigError): model.parse("STATUS_PORT=99999\n")
        with self.assertRaises(ConfigError): model.parse("FEATURE_MAP=perhaps\n")

    def test_journal_persistence_and_incomplete_filter(self):
        with tempfile.TemporaryDirectory() as temporary:
            store = JournalStore(Path(temporary)); journal = OperationJournal.create("profile", "install")
            store.save(journal); self.assertEqual(store.incomplete("profile")[0].operation_id, journal.operation_id)
            journal.state = "complete"; store.save(journal); self.assertEqual(store.incomplete("profile"), [])

    def test_cache_keeps_two_unpinned_bundles(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary); cache = BundleCache(root / "cache")
            for index in range(3):
                bundle = root / f"asset-{index}.zip"; bundle.write_bytes(str(index).encode())
                cache.add(bundle, "game", {"gameBuild": str(index), "generatedAt": f"2026-01-0{index+1}"})
            self.assertEqual(len(cache.list("game")), 2)

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

    def test_release_payload_manifest_and_traversal(self):
        with tempfile.TemporaryDirectory() as temporary:
            archive = Path(temporary) / "server.zip"
            required = ["bin/libzanik.so", "bin/zanik-server", "bin/zanik-compat-analyzer", "scripts/compat-launcher.sh",
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

    def test_remote_asset_verification_uses_large_file_download_route(self):
        class Client:
            def download_file(self, server, path): return b"asset-bytes"
            def read_file(self, server, path): raise AssertionError("text endpoint must not verify assets")
        class Provider:
            client = Client()
        with tempfile.TemporaryDirectory() as temporary:
            tree=Path(temporary); (tree/"world-map.jpg").write_bytes(b"asset-bytes")
            progress=[]
            AssetDeployer(Provider())._verify_remote_tree("dev","/stage",tree,
                                                          lambda current,total,path:progress.append((current,total,path)))
            self.assertEqual(progress,[(1,1,"world-map.jpg")])

    def test_pyside_pruner_keeps_only_declared_runtime(self):
        with tempfile.TemporaryDirectory() as temporary:
            root=Path(temporary)
            for name in KEEP_FILES|{"PySide6/Qt6Qml.dll","PySide6/designer.exe"}:
                path=root/name; path.parent.mkdir(parents=True,exist_ok=True); path.write_bytes(b"x")
            prune(root)
            self.assertFalse((root/"PySide6/Qt6Qml.dll").exists())
            self.assertFalse((root/"PySide6/designer.exe").exists())
            self.assertTrue(all((root/name).is_file() for name in KEEP_FILES))

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
                     "scripts/compat-launcher.sh": b"#!/bin/sh\n# ZANIK_COMPAT_LAUNCHER=1\n", "config/zanik.conf.example": b"FEATURE_MAP=true\n",
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
