#!/usr/bin/env python3
import argparse, hashlib, json, re, tarfile, tempfile, zipfile
from pathlib import Path

BANNED_NAMES={'.env','.env.gitlab','admins.txt','console-admin.key','pterodactyl_api_key.txt','AGENTS.MD'}
BANNED_SUFFIXES={'.pak','.utoc','.ucas','.usmap','.sav','.db','.pem','.p12','.pfx'}
BANNED_CONTENT=(b'PTERODACTYL_API_KEY=',b'GITLAB_TOKEN=',b'-----BEGIN PRIVATE KEY-----')
REQUIRED={'bin/libzanik.so','bin/zanik-compat-analyzer','bin/zanik-server','scripts/install.sh','scripts/update.sh','scripts/uninstall.sh','scripts/doctor.sh','scripts/init-assets.sh','config/zanik.conf.example','compatibility/compatibility.tsv','LICENSE','README.md','CHANGELOG.md','docs/releasing.md'}
EXECUTABLE={'bin/libzanik.so','bin/zanik-compat-analyzer','bin/zanik-server','scripts/install.sh','scripts/update.sh','scripts/uninstall.sh','scripts/doctor.sh','scripts/init-assets.sh'}
WINDOWS_ASSET_REQUIRED={'init-assets.ps1','runtime/python.exe','runtime/LICENSE.txt','third_party/uas/uas.exe',
    'third_party/uas/LICENSE.txt','third_party/ue4ss/dwmapi.dll','third_party/ue4ss/ue4ss/UE4SS.dll',
    'third_party/ue4ss/ue4ss/LICENSE','asset-tools/extraction-recipes.json','tools/zanik_assets.py',
    'tools/windows_mapping.py','tools/windows_asset_pipeline.py','tools/generate_web_assets.py',
    'tools/generate_world_catalogs.py','deploy/quest-overrides.json','deploy/quest-step-overrides.json'}

def clean(name): return name.lstrip('./').replace('\\','/')
def names(path):
    if path.name.endswith(('.tar.gz','.tar.xz')):
        with tarfile.open(path) as a: return {clean(x.name) for x in a.getmembers() if x.isfile()}
    with zipfile.ZipFile(path) as a: return {clean(x.filename) for x in a.infolist() if not x.is_dir()}
def inspect(path):
    entries=names(path)
    for name in entries:
        parts=Path(name).parts
        if '..' in parts or any(part in BANNED_NAMES for part in parts) or Path(name).suffix.lower() in BANNED_SUFFIXES:
            raise SystemExit(f'prohibited archive entry in {path.name}: {name}')
    if path.name.startswith('zanik-server-'):
        missing=REQUIRED-entries
        if missing: raise SystemExit(f'{path.name} missing: {sorted(missing)}')
    if path.name.startswith('zanik-asset-tools-windows-'):
        missing=WINDOWS_ASSET_REQUIRED-entries
        if missing: raise SystemExit(f'{path.name} missing portable extractor files: {sorted(missing)}')
    if path.name.endswith(('.tar.gz','.tar.xz')):
        with tarfile.open(path) as archive:
            for member in archive.getmembers():
                name=clean(member.name)
                if name in EXECUTABLE and member.mode & 0o111 == 0: raise SystemExit(f'not executable in {path.name}: {name}')
                if member.isfile() and member.size <= 1024*1024:
                    data=archive.extractfile(member).read()
                    if any(marker in data for marker in BANNED_CONTENT): raise SystemExit(f'credential marker in {path.name}: {name}')
    else:
        with zipfile.ZipFile(path) as archive:
            for member in archive.infolist():
                name=clean(member.filename); mode=(member.external_attr >> 16) & 0o777
                if path.name.startswith('zanik-server-') and name in EXECUTABLE and mode & 0o111 == 0: raise SystemExit(f'not executable in {path.name}: {name}')
                if not member.is_dir() and member.file_size <= 1024*1024:
                    data=archive.read(member)
                    if any(marker in data for marker in BANNED_CONTENT): raise SystemExit(f'credential marker in {path.name}: {name}')
    return entries

p=argparse.ArgumentParser(); p.add_argument('--directory',required=True); p.add_argument('--tag'); p.add_argument('--commit'); a=p.parse_args(); root=Path(a.directory)
archives=sorted([*root.glob('zanik-server-*'),*root.glob('zanik-asset-tools-*')])
archives=[p for p in archives if p.suffix in {'.zip','.gz','.xz'}]
if len(archives)!=4: raise SystemExit(f'expected four release archives, found {len(archives)}')
for archive in archives: inspect(archive)
manifest=json.loads((root/'release-manifest.json').read_text())
if manifest.get('schemaVersion') != 2: raise SystemExit('release manifest schema must be 2')
if a.tag is not None and manifest.get('tag') != a.tag: raise SystemExit('release manifest tag mismatch')
if a.commit is not None and manifest.get('commit') != a.commit: raise SystemExit('release manifest commit mismatch')
if not isinstance(manifest.get('sourceDateEpoch'),int) or not manifest.get('buildTimestamp'): raise SystemExit('release manifest provenance is incomplete')
for record in manifest['files']:
    path=root/record['name']; actual=hashlib.sha256(path.read_bytes()).hexdigest()
    if actual!=record['sha256'] or path.stat().st_size!=record['size']: raise SystemExit(f'manifest mismatch: {path.name}')
print(f'validated {len(archives)} archives and {len(manifest["files"])} manifest records')
