import hashlib, json, os, stat, subprocess, tempfile, zipfile
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
TOOL=ROOT/'tools/zanik_assets.py'
REQUIRED=('buildings/catalog.json','buildings/catalog.tsv','npc-catalog.json','plan-items.tsv','quest-catalog.json','quest-runtime-catalog.tsv','quest-step-locations.json','recipe-unlocks.json','recipe-runtime-catalog.tsv','resource-catalog.json','resource-identities.tsv','npc-identities.tsv')

with tempfile.TemporaryDirectory() as name:
    root=Path(name); game=root/'game'; generated=game/'zanik-generated-web'; destination=root/'assets'
    paks=game/'RSDragonwilds/Content/Paks'; paks.mkdir(parents=True)
    (paks/'RSDragonwilds-LinuxServer.utoc').write_bytes(b'container')
    for item in REQUIRED:
        path=generated/item; path.parent.mkdir(parents=True,exist_ok=True)
        path.write_text('# schema=1\tid\n' if path.suffix=='.tsv' else '{}\n')
    first=root/'one.zip'; second=root/'two.zip'
    env=dict(os.environ, SOURCE_DATE_EPOCH='1')
    subprocess.run(['python3',str(TOOL),'build','--game-root',str(game),'--generated-root',str(generated),'--output',str(first)],check=True,env=env)
    subprocess.run(['python3',str(TOOL),'build','--game-root',str(game),'--generated-root',str(generated),'--output',str(second)],check=True,env=env)
    assert hashlib.sha256(first.read_bytes()).digest()==hashlib.sha256(second.read_bytes()).digest()
    subprocess.run(['python3',str(TOOL),'validate','--generated-root',str(generated)],check=True)
    subprocess.run(['python3',str(TOOL),'validate','--bundle',str(first)],check=True)
    subprocess.run(['python3',str(TOOL),'install-bundle','--bundle',str(first),'--destination',str(destination)],check=True)
    current=destination/'current'; manifest=json.loads((current/'asset-manifest.json').read_text())
    assert manifest['sourceKind']=='linux-dedicated-server' and not manifest['missingRequired']
    previous=current.resolve()
    bad=root/'bad.zip'
    with zipfile.ZipFile(bad,'w') as z: z.writestr('../escape','bad')
    failed=subprocess.run(['python3',str(TOOL),'install','--bundle',str(bad),'--destination',str(destination)],capture_output=True)
    assert failed.returncode and current.resolve()==previous and not (root/'escape').exists()
    converter=root/'converter.sh'
    converter.write_text('#!/bin/sh\nset -eu\nout=\nwhile [ $# -gt 0 ]; do case "$1" in --output) out=$2; shift 2;; *) shift;; esac; done\nmkdir -p "$out"\ncp -a "'+str(generated)+'/." "$out/"\n')
    converter.chmod(converter.stat().st_mode | stat.S_IXUSR)
    extracted=root/'extracted'
    subprocess.run(['python3',str(TOOL),'extract','--game-root',str(game),'--output',str(extracted),'--source-kind','server','--tool-path',str(converter)],check=True)
    assert (extracted/'recipe-unlocks.json').is_file()
    no_tool=subprocess.run(['python3',str(TOOL),'extract','--game-root',str(game),'--output',str(root/'none')],capture_output=True)
    assert no_tool.returncode==3
print('asset tool tests passed')
