#!/usr/bin/env python3
import hashlib
import json
import pathlib
import sys
import tempfile
import unittest
from unittest import mock

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))

from manager.zanik_manager.journal import JournalStore
from manager.zanik_manager.lifecycle import LAUNCHER_MARKER, PterodactylProvider


ROOT = "/srv/dragonwilds"
GAME = ROOT + "/RSDragonwilds/Binaries/Linux/RSDragonwildsServer-Linux-Shipping"


class FakeClient:
    def __init__(self, *, corrupt_launcher=False, stale_result=False):
        self.launcher = b"#!/bin/sh\n# ZANIK_COMPAT_LAUNCHER=1\n"
        self.corrupt_launcher = corrupt_launcher
        self.stale_result = stale_result
        self.state = "offline"
        self.files = {
            ROOT + "/zanik/install-state.json": json.dumps({
                "mode": "replace-executable", "version": "1.2.3"
            }).encode(),
            GAME: self.launcher,
            GAME + ".zanik-original": b"\x7fELFofficial",
            ROOT + "/zanik/scripts/update-game.sh": b"#!/bin/sh\n# ZANIK_GAME_UPDATER=1\n",
        }
        self.deleted = []

    def server(self, _server):
        return {"server_owner": True}

    def startup_variable(self, _server, key):
        return {"key": key, "value": "0", "editable": True}

    def resources(self, _server):
        return {"current_state": self.state}

    def power(self, _server, signal):
        if signal == "stop":
            self.state = "offline"
            return
        if signal != "start":
            raise AssertionError(signal)
        if b"ZANIK_GAME_UPDATER=1" in self.files[GAME]:
            request = json.loads(self.files[ROOT + "/zanik/state/game-update-request.json"])
            launcher = self.files[ROOT + "/zanik/state/game-update-launcher"]
            self.files[GAME] = b"corrupt" if self.corrupt_launcher else launcher
            self.files[ROOT + "/zanik/state/game-update-result.json"] = json.dumps({
                "schemaVersion": 2,
                "operationId": "stale-operation" if self.stale_result else request["operationId"],
                "status": "complete",
                "installedBuild": request["expectedBuild"],
                "officialSha256": "a" * 64,
                "launcherSha256": request["launcherSha256"],
                "saveFingerprintBefore": "save-hash",
                "saveFingerprintAfter": "save-hash",
            }).encode()
            self.state = "offline"
        else:
            self.state = "running"

    def read_file(self, _server, path):
        if path not in self.files:
            raise RuntimeError("missing " + path)
        return self.files[path]

    def write_file(self, _server, path, content):
        self.files[path] = content

    def upload(self, _server, directory, name, content):
        self.files[directory.rstrip("/") + "/" + name] = content

    def chmod(self, *_args):
        pass

    def delete(self, _server, directory, names):
        for name in names:
            path = directory.rstrip("/") + "/" + name
            self.deleted.append(path)
            self.files.pop(path, None)


class Provider(PterodactylProvider):
    def _mkdir(self, _server, _path):
        pass

    def _entry(self, _server, path):
        if path not in self.client.files:
            return None
        size = 2 * 1024 * 1024 if path.endswith(".zanik-original") else len(self.client.files[path])
        return {"is_file": True, "size": size}

    def _exists(self, _server, path):
        return path in self.client.files

    def _finish_backup(self, _profile, _action, server, _root, backup, family,
                       operation_id, state):
        self._write_backup_manifest(server, backup, family, operation_id, state)


class ReplacementLifecycleTests(unittest.TestCase):
    def provider(self, temporary, **client_options):
        return Provider(FakeClient(**client_options), JournalStore(pathlib.Path(temporary) / "journals"))

    def test_operation_bound_update_verifies_launcher_and_cleans_transients(self):
        with tempfile.TemporaryDirectory() as temporary:
            provider = self.provider(temporary)
            result = provider.update_game("profile", "dev", ROOT, "200")
            self.assertEqual(result["installedBuild"], "200")
            self.assertEqual(provider.client.state, "running")
            self.assertEqual(provider.client.files[GAME], provider.client.launcher)
            self.assertEqual(hashlib.sha256(provider.client.files[GAME]).hexdigest(),
                             hashlib.sha256(provider.client.launcher).hexdigest())
            for name in ("game-update-launcher", "game-update-request.json", "game-update-result.json"):
                self.assertNotIn(ROOT + "/zanik/state/" + name, provider.client.files)
            state = json.loads(provider.client.files[ROOT + "/zanik/install-state.json"])
            self.assertEqual(state["gameBuild"], "200")
            self.assertEqual(state["officialSha256"], "a" * 64)

    def test_mismatched_result_is_not_accepted_and_recovery_files_remain(self):
        with tempfile.TemporaryDirectory() as temporary:
            provider = self.provider(temporary, stale_result=True)
            with mock.patch("manager.zanik_manager.lifecycle.time.monotonic", side_effect=[0, 1000]):
                with self.assertRaisesRegex(RuntimeError, "did not produce a result"):
                    provider.update_game("profile", "dev", ROOT, "200")
            self.assertEqual(provider.client.state, "offline")
            self.assertIn(ROOT + "/zanik/state/game-update-request.json", provider.client.files)
            self.assertIn(ROOT + "/zanik/state/game-update-result.json", provider.client.files)

    def test_corrupted_restored_launcher_blocks_restart(self):
        with tempfile.TemporaryDirectory() as temporary:
            provider = self.provider(temporary, corrupt_launcher=True)
            with self.assertRaisesRegex(RuntimeError, "restored replacement launcher did not verify"):
                provider.update_game("profile", "dev", ROOT, "200")
            self.assertEqual(provider.client.state, "offline")


if __name__ == "__main__":
    unittest.main()
