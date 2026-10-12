#!/usr/bin/env python3
import argparse,json,struct
from pathlib import Path
from inspect_spud_resources import Reader,chunks,parse_metadata

def extract(path):
 data=path.read_bytes(); root=Reader(data); magic,level,_=root.chunk()
 if magic!='LEVL': return []
 level.fstring(); level.take(8); meta=sats=None
 for tag,c in chunks(level):
  if tag=='META': meta=parse_metadata(c)
  elif tag=='SATS': sats=c
 if not meta or not sats:return []
 out=[]
 for tag,c in chunks(sats):
  if tag!='SPWN':continue
  cid=c.u32()
  if cid>=len(meta['classes']) or 'PlayerGravestone' not in meta['classes'][cid]:continue
  b=data[c.pos:c.end]
  mark=b.find(b'{\n\t"Version"')
  if mark<0:continue
  end=b.find(b'\0',mark)
  try: inventory=json.loads(b[mark:end].decode())
  except Exception:continue
  loc=struct.unpack_from('<ddd',b,40) if len(b)>=64 else (0,0,0)
  slots=[]
  for key,value in inventory.items():
   if not key.isdigit() or not isinstance(value,dict):continue
   slots.append({'slot':int(key),'persistenceId':value.get('ItemData',''),'quantity':int(value.get('Count',1)),'durability':value.get('Durability'),'upgrades':value.get('NumUpgradesApplied',0),'instanceGuid':value.get('GUID','')})
  out.append({'cell':path.name,'class':meta['classes'][cid],'location':loc,'maxSlotIndex':inventory.get('MaxSlotIndex',-1),'allowAdds':inventory.get('AllowAdds',False),'slots':sorted(slots,key=lambda x:x['slot'])})
 return out

def main():
 ap=argparse.ArgumentParser();ap.add_argument('directory');ap.add_argument('--items',default='dist/dw-editor-release/data/ItemID.json');ap.add_argument('-o','--output');a=ap.parse_args()
 items={x.get('PersistenceID'):x for x in json.load(open(a.items,encoding='utf-8-sig')) if x.get('PersistenceID')}
 graves=[]
 for p in Path(a.directory).rglob('*.lvl'):graves.extend(extract(p))
 for g in graves:
  for s in g['slots']:
   item=items.get(s['persistenceId'],{});s.update(name=item.get('SourceString',s['persistenceId']),maxStack=item.get('MaxStackSize',1),icon=item.get('IconFile',''))
 text=json.dumps({'graveCount':len(graves),'graves':graves},indent=2)
 if a.output:Path(a.output).write_text(text,encoding='utf-8')
 else:print(text)
if __name__=='__main__':main()
