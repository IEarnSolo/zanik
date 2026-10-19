#!/usr/bin/env python3
import json, pathlib, sys, tempfile, unittest
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from manager.zanik_manager.journal import JournalStore
from manager.zanik_manager.lifecycle import PterodactylProvider
from manager.zanik_manager.pterodactyl import PterodactylClient, Response

class StartupSafetyTests(unittest.TestCase):
    def test_startup_variable_is_normalized_without_other_values(self):
        def transport(method,url,headers,body,timeout):
            return Response(200,json.dumps({"data":[
                {"attributes":{"env_variable":"SECRET","server_value":"do-not-return","is_editable":True}},
                {"attributes":{"env_variable":"AUTO_UPDATE","server_value":"1","default_value":"1","is_editable":True,"rules":"required|numeric"}}]}).encode(),{})
        value=PterodactylClient("https://panel", "token", transport).startup_variable("server","AUTO_UPDATE")
        self.assertEqual(value,{"key":"AUTO_UPDATE","value":"1","default":"1","editable":True,"rules":"required|numeric"})

    def test_replacement_enforcement_writes_and_reads_back(self):
        class Client:
            def __init__(self): self.value="1"
            def startup_variable(self,_server,key): return {"key":key,"value":self.value,"editable":True}
            def set_startup_variable(self,_server,key,value): self.value=value
        with tempfile.TemporaryDirectory() as temporary:
            client=Client(); provider=PterodactylProvider(client,JournalStore(pathlib.Path(temporary)))
            state=provider._enforce_replacement_auto_update("dev")
            self.assertEqual(state["previousValue"],"1"); self.assertEqual(client.value,"0")

    def test_replacement_rejects_missing_or_read_only_variable(self):
        for value in (None,{"key":"AUTO_UPDATE","value":"1","editable":False}):
            class Client:
                def startup_variable(self,_server,_key): return value
            with tempfile.TemporaryDirectory() as temporary:
                provider=PterodactylProvider(Client(),JournalStore(pathlib.Path(temporary)))
                with self.assertRaises(RuntimeError): provider._enforce_replacement_auto_update("dev")

    def test_replacement_rejects_a_write_that_does_not_stick(self):
        class Client:
            def startup_variable(self, _server, key):
                return {"key": key, "value": "1", "editable": True}
            def set_startup_variable(self, _server, _key, _value):
                pass
        with tempfile.TemporaryDirectory() as temporary:
            provider=PterodactylProvider(Client(),JournalStore(pathlib.Path(temporary)))
            with self.assertRaisesRegex(RuntimeError, "could not set and verify AUTO_UPDATE=0"):
                provider._enforce_replacement_auto_update("dev")

    def test_uninstall_restore_does_not_overwrite_an_operator_change(self):
        class Client:
            def __init__(self, value): self.value=value; self.writes=[]
            def startup_variable(self, _server, key):
                return {"key": key, "value": self.value, "editable": True}
            def set_startup_variable(self, _server, key, value):
                self.writes.append((key,value)); self.value=value
        metadata={"managedByZanik":True,"managedValue":"0","previousValue":"1"}
        with tempfile.TemporaryDirectory() as temporary:
            changed=Client("1")
            provider=PterodactylProvider(changed,JournalStore(pathlib.Path(temporary)))
            self.assertFalse(provider._restore_managed_auto_update("dev",metadata))
            self.assertEqual(changed.writes,[])
            managed=Client("0")
            provider=PterodactylProvider(managed,JournalStore(pathlib.Path(temporary)))
            self.assertTrue(provider._restore_managed_auto_update("dev",metadata))
            self.assertEqual(managed.writes,[("AUTO_UPDATE","1")])

if __name__=="__main__": unittest.main()
