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
server_payload={'releaseVersion':contract['releaseVersion'],'contractSchemaVersion':contract['schemaVersion'],
                'contractSha256':contract['contractSha256']}
output.write_text(json.dumps({'schemaVersion':2,'version':a.version,'tag':a.tag or None,'commit':a.commit or None,'buildTimestamp':timestamp,'sourceDateEpoch':a.source_date_epoch,'serverPayload':server_payload,'files':files},indent=2,sort_keys=True)+'\n',encoding='utf-8')
