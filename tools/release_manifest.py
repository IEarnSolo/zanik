#!/usr/bin/env python3
import argparse, datetime, hashlib, json, zipfile
from pathlib import Path

def sha(path):
    h=hashlib.sha256()
    with path.open('rb') as f:
        for block in iter(lambda:f.read(1024*1024),b''): h.update(block)
    return h.hexdigest()

p=argparse.ArgumentParser(); p.add_argument('--directory',required=True); p.add_argument('--version',required=True); p.add_argument('--output',required=True)
p.add_argument('--tag',default=''); p.add_argument('--commit',default=''); p.add_argument('--source-date-epoch',type=int,required=True); a=p.parse_args()
root=Path(a.directory); output=Path(a.output).resolve()
files=[]
for path in sorted(root.glob('zanik-*')):
    if path.is_file() and path.resolve()!=output:
        files.append({'name':path.name,'size':path.stat().st_size,'sha256':sha(path)})
timestamp=datetime.datetime.fromtimestamp(a.source_date_epoch,datetime.timezone.utc).isoformat().replace('+00:00','Z')
server_zip=root/f'zanik-server-linux-x86_64-{a.version}.zip'
with zipfile.ZipFile(server_zip) as archive:
    contract=json.loads(archive.read('payload-contract.json'))
    compatibility=archive.read('compatibility/compatibility.tsv').decode('utf-8')
server_payload={'releaseVersion':contract['releaseVersion'],'contractSchemaVersion':contract['schemaVersion'],
                'contractSha256':contract['contractSha256']}
supported_hashes=sorted({line.split('\t',1)[0].lower() for line in compatibility.splitlines()
                         if line and not line.startswith('#') and len(line.split('\t',1)[0])==64})
if not supported_hashes or any(any(c not in '0123456789abcdef' for c in value) for value in supported_hashes):
    raise SystemExit('compatibility manifest has no valid supported executable hashes')
game_compatibility={'schemaVersion':1,'linuxExecutableSha256':supported_hashes}
asset_zip=root/f'zanik-asset-tools-windows-x86_64-{a.version}.zip'
with zipfile.ZipFile(asset_zip) as archive:
    recipe_bytes=archive.read('asset-tools/extraction-recipes.json')
    recipe=json.loads(recipe_bytes)
asset_contract={'extractionRecipeVersion':recipe['schemaVersion'],
                'compatibilityProfileVersion':recipe['catalogProfile']['version'],
                'extractionRecipeSha256':hashlib.sha256(recipe_bytes).hexdigest()}
output.write_text(json.dumps({'schemaVersion':2,'version':a.version,'tag':a.tag or None,'commit':a.commit or None,'buildTimestamp':timestamp,'sourceDateEpoch':a.source_date_epoch,'serverPayload':server_payload,'gameCompatibility':game_compatibility,'assetContract':asset_contract,'files':files},indent=2,sort_keys=True)+'\n',encoding='utf-8')
