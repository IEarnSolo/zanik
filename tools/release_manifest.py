#!/usr/bin/env python3
import argparse, hashlib, json
from pathlib import Path

def sha(path):
    h=hashlib.sha256()
    with path.open('rb') as f:
        for block in iter(lambda:f.read(1024*1024),b''): h.update(block)
    return h.hexdigest()

p=argparse.ArgumentParser(); p.add_argument('--directory',required=True); p.add_argument('--version',required=True); p.add_argument('--output',required=True); a=p.parse_args()
root=Path(a.directory); output=Path(a.output).resolve()
files=[]
for path in sorted(root.glob('zanik-*')):
    if path.is_file() and path.resolve()!=output:
        files.append({'name':path.name,'size':path.stat().st_size,'sha256':sha(path)})
output.write_text(json.dumps({'schemaVersion':1,'version':a.version,'files':files},indent=2,sort_keys=True)+'\n',encoding='utf-8')
