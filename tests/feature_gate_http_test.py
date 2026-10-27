#!/usr/bin/env python3
import json, os, pathlib, socket, subprocess, tempfile, time, urllib.error, urllib.request

ROOT=pathlib.Path(__file__).resolve().parents[1]
with socket.socket() as probe:
    probe.bind(('127.0.0.1',0)); port=probe.getsockname()[1]
with tempfile.TemporaryDirectory(prefix='zanik-gates-') as name:
    env=os.environ.copy(); env.update({
        'LD_PRELOAD':str(ROOT/'dist/libdwstatus.so'),'DW_STATUS_PORT':str(port),
        'DW_STATUS_DATA':str(pathlib.Path(name)/'players.db'),'DW_WEB_ROOT':name,
        'DW_STATUS_TEST_HOST':'1','ZANIK_CONFIG_VALID':'1',
        'ZANIK_FEATURE_PLAYER_STATUS':'0','ZANIK_FEATURE_KILL_TRACKING':'0',
        'ZANIK_FEATURE_MAP':'0','ZANIK_FEATURE_RESOURCE_TRACKING':'0',
        'ZANIK_FEATURE_NPC_TRACKING':'0','ZANIK_FEATURE_PLAYER_RECIPE_TRACKING':'0',
        'ZANIK_FEATURE_PLAYER_QUEST_TRACKING':'0','ZANIK_FEATURE_ADMIN_READ':'0',
    })
    process=subprocess.Popen(['/bin/sleep','20'],env=env,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
    base=f'http://127.0.0.1:{port}'
    try:
        for _ in range(50):
            try:
                with urllib.request.urlopen(base+'/api/update-status',timeout=2) as response: update=json.loads(response.read())
                break
            except (OSError, urllib.error.URLError): time.sleep(.1)
        else: raise AssertionError('listener did not start')
        assert all(update['featureStates'][key]=='disabled' for key in ('playerStatus','killTracking','map','resourceTracking','npcTracking','playerRecipeTracking','playerQuestTracking','adminRead'))
        for path,status in (('/',404),('/map',404),('/kills',404),('/api/players',503),('/api/kills',503),('/api/skills',503),('/api/skill-leaderboard?skill=Mining',503),('/api/map/resources/catalog',503),('/api/map/npcs/catalog',503),('/api/quests/catalog',503)):
            try: urllib.request.urlopen(base+path,timeout=2); raise AssertionError(path)
            except urllib.error.HTTPError as error:
                assert error.code==status,(path,error.code)
                if status==503: assert json.loads(error.read())['error']=='Feature disabled by server operator'
    finally:
        process.terminate(); _,stderr=process.communicate(timeout=5)
        text=stderr.decode(errors='replace')
        assert 'hook installed' not in text.lower()
        assert 'tracking initialized' not in text.lower()
print('feature gate HTTP tests passed')
