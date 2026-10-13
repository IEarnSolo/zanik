"""Generate the public/runtime quest catalogs from cooked QuestData exports."""
from __future__ import annotations
import argparse,hashlib,json,re,sys
from pathlib import Path
try:
    from asset_filesystem import read_json
except ModuleNotFoundError:  # Imported as tools.* by repository tests.
    from tools.asset_filesystem import read_json

REGIONS={'Brynmoor':'Brynmoor','Ghornfell':'Ghornfell','Fellhollow':'Fellhollow','DowdunReach':'Dowdun Reach','UmbralSands':'Umbral Sands','ScornedWilderness':'Scorned Wilderness'}
EXCLUDE=('ftue','test','debug','example','template')

def text(value):
    if isinstance(value,dict): return value.get('LocalizedString') or value.get('SourceString') or ''
    return value if isinstance(value,str) else ''
def strings(value):
    out=[]
    if isinstance(value,dict):
        candidate=text(value)
        if candidate: out.append(candidate)
        else:
            for child in value.values(): out+=strings(child)
    elif isinstance(value,list):
        for child in value: out+=strings(child)
    return list(dict.fromkeys(x for x in out if x))
def region(value,path):
    key=str(value or '').split('::')[-1]
    if key in REGIONS:return REGIONS[key]
    low=path.lower()
    for token,label in [('umbralsands','Umbral Sands'),('ums_quests','Umbral Sands'),('scornedwilderness','Scorned Wilderness'),('dowdun','Dowdun Reach'),('fh_quests','Fellhollow'),('fellhollow','Fellhollow'),('ghornfell','Ghornfell')]:
        if token in low:return label
    return 'Brynmoor'

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--source',type=Path,required=True)
    parser.add_argument('--public',type=Path,required=True)
    parser.add_argument('--runtime',type=Path,required=True)
    parser.add_argument('--overrides',type=Path,required=True)
    args=parser.parse_args()
    overrides=json.loads(args.overrides.read_text(encoding='utf-8')) if args.overrides.exists() else {}
    quests=[]
    for path in args.source.rglob('*.json'):
        if 'quest' not in str(path).lower():continue
        try:data=read_json(path)
        except (OSError,UnicodeError,json.JSONDecodeError) as error:
            raise RuntimeError(f"cannot read exported JSON {path.name}: {error}") from error
        for export in data if isinstance(data,list) else []:
            if export.get('Type')!='QuestData':continue
            p=export.get('Properties') or {};qid=p.get('PersistenceID');name=text(p.get('QuestName'))
            marker=(str(path)+' '+export.get('Name','')+' '+name).lower()
            if not qid or not name or any(word in marker for word in EXCLUDE):continue
            objectives=[]
            for item in p.get('ObjectiveTexts') or []:
                value=text((item or {}).get('Value'))
                if value and value not in objectives:objectives.append(value)
            override=overrides.get(qid,{})
            entry={'id':qid,'name':name,'internal':export.get('Name',''),'type':'main' if p.get('bIsMainQuest') else 'side','region':override.get('region') or region(p.get('QuestRegion'),str(path)),'subregion':override.get('subregion',''),'starter':override.get('starter','Unknown'),'description':text(p.get('QuestDescription')),'objectives':objectives,'requirements':override.get('requirements',strings(p.get('QuestRequirements'))),'rewards':override.get('rewards',strings(p.get('QuestRewards'))),'icon':'/assets/quests/main-quest.svg' if p.get('bIsMainQuest') else '/assets/quests/side-quest.svg','map':override.get('map')}
            quests.append(entry)
    by_id={}
    for quest in quests:
        prior=by_id.get(quest['id'])
        if prior and prior['internal']!=quest['internal']:raise SystemExit(f"duplicate persistence id {quest['id']}")
        by_id[quest['id']]=quest
    quests=sorted(by_id.values(),key=lambda q:(q['region'],q['type']!='main',q['name']))
    if not quests:raise SystemExit('no playable QuestData assets found')
    canonical=json.dumps(quests,ensure_ascii=False,separators=(',',':'),sort_keys=True).encode()
    fingerprint=hashlib.sha256(canonical).hexdigest()
    public_quests=[]
    for quest in quests:
        quest=dict(quest);quest.pop('internal');public_quests.append(quest)
    args.public.parent.mkdir(parents=True,exist_ok=True)
    args.public.write_text(json.dumps({'schemaVersion':1,'fingerprint':fingerprint,'quests':public_quests},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    lines=[f'# schema=1 fingerprint={fingerprint} count={len(quests)}']+[f"{q['id']}\t{q['name']}\t{q['internal']}" for q in quests]
    args.runtime.write_text('\n'.join(lines)+'\n',encoding='utf-8')
    print(f'generated {len(quests)} playable quests ({fingerprint})')
if __name__=='__main__':main()
