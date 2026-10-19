#!/usr/bin/env python3
import json, os, pathlib, socket, subprocess, tempfile, time, urllib.error, urllib.request

ROOT=pathlib.Path(__file__).resolve().parents[1]
with socket.socket() as probe:
    probe.bind(("127.0.0.1",0)); port=probe.getsockname()[1]
with tempfile.TemporaryDirectory(prefix="zanik-maintenance-") as temporary:
    state=pathlib.Path(temporary)/"release.json"
    state.write_text(json.dumps({"schemaVersion":1,"status":"compatible-release-available",
                                 "version":"9.8.7","releaseUrl":"https://github.com/IEarnSolo/zanik/releases/tag/v9.8.7"}))
    env=os.environ.copy(); env.update(LD_PRELOAD=str(ROOT/"dist/libdwstatus.so"),DW_STATUS_PORT=str(port),
        DW_STATUS_TEST_HOST="1",ZANIK_MAINTENANCE_MODE="1",ZANIK_INSTALL_MODE="replace-executable",
        ZANIK_GAME_EXECUTABLE_SHA256="a"*64,DW_MAINTENANCE_RELEASE_STATE=str(state))
    process=subprocess.Popen(["/bin/sleep","20"],env=env,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
    base=f"http://127.0.0.1:{port}"
    try:
        for _ in range(50):
            try:
                urllib.request.urlopen(base+"/",timeout=1).close(); break
            except OSError: time.sleep(.1)
        with urllib.request.urlopen(base+"/") as response:
            page=response.read().decode(); assert "waiting for a compatibility update" in page
            assert "AUTO_UPDATE must remain 0" in page and "Update / repair Zanik" in page, page
        with urllib.request.urlopen(base+"/api/update-status") as response:
            status=json.loads(response.read()); assert status["maintenance"] is True
            assert status["nativeRuntimeLoaded"] is False and status["release"]["status"]=="compatible-release-available"
            assert "zanikVersion" in status and "gameBuild" in status and "autoUpdate" in status
        try: urllib.request.urlopen(base+"/api/players")
        except urllib.error.HTTPError as error: assert error.code==503
        else: raise AssertionError("normal APIs must be unavailable during maintenance")
    finally:
        process.terminate(); _,stderr=process.communicate(timeout=5)
    log=stderr.decode(errors="replace")
    assert "maintenance-only website listening" in log
    assert "dispatcher installed" not in log and "World resource tracking" not in log
print("maintenance-only HTTP behavior: ok")
