import hashlib, json, os, subprocess, tempfile, zipfile
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
TOOL=ROOT/'tools/zanik_assets.py'
REQUIRED=('buildings/catalog.json','buildings/catalog.tsv','npc-catalog.json','plan-items.tsv','quest-catalog.json','quest-runtime-catalog.tsv','quest-step-locations.json','recipe-unlocks.json','recipe-runtime-catalog.tsv')

with tempfile.TemporaryDirectory() as name:
    root=Path(name); game=root/'game'; generated=game/'zanik-generated-web'; destination=root/'assets'
    paks=game/'RSDragonwilds/Content/Paks'; paks.mkdir(parents=True)
    (paks/'RSDragonwilds-LinuxServer.utoc').write_bytes(b'container')
    for item in REQUIRED:
        path=generated/item; path.parent.mkdir(parents=True,exist_ok=True); path.write_text('{}\n')
    first=root/'one.zip'; second=root/'two.zip'
    env=dict(os.environ, SOURCE_DATE_EPOCH='1')
    subprocess.run(['python3',str(TOOL),'build','--game-root',str(game),'--generated-root',str(generated),'--output',str(first)],check=True,env=env)
    subprocess.run(['python3',str(TOOL),'build','--game-root',str(game),'--generated-root',str(generated),'--output',str(second)],check=True,env=env)
    assert hashlib.sha256(first.read_bytes()).digest()==hashlib.sha256(second.read_bytes()).digest()
    subprocess.run(['python3',str(TOOL),'install','--bundle',str(first),'--destination',str(destination)],check=True)
    current=destination/'current'; manifest=json.loads((current/'asset-manifest.json').read_text())
    assert manifest['sourceKind']=='linux-dedicated-server' and not manifest['missingRequired']
    previous=current.resolve()
    bad=root/'bad.zip'
    with zipfile.ZipFile(bad,'w') as z: z.writestr('../escape','bad')
    failed=subprocess.run(['python3',str(TOOL),'install','--bundle',str(bad),'--destination',str(destination)],capture_output=True)
    assert failed.returncode and current.resolve()==previous and not (root/'escape').exists()
print('asset tool tests passed')
