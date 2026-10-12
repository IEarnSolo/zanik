import json, pathlib

ROOT=pathlib.Path(__file__).resolve().parents[1]
catalog=json.loads((ROOT/'web/npc-catalog.json').read_text())
assert catalog['nodes'] and catalog['types']
assert len({n['id'] for n in catalog['nodes']})==len(catalog['nodes'])
assert all(0<=n['x']<=1 and 0<=n['y']<=1 for n in catalog['nodes'])
assert all(t['name'] and t['count']>0 for t in catalog['types'])
assert all(not t.get('icon') or t['icon'].startswith('/assets/npcs/') for t in catalog['types'])
assert sum(t['count'] for t in catalog['types'] if t.get('icon')) >= 2200
assert sum(t['count'] for t in catalog['types'])==len(catalog['nodes'])
assert any(t['count']>1 for t in catalog['types'])
assert any(t['name']=='Chicken' for t in catalog['types'])
assert any('Wise Old Man' in t['name'] for t in catalog['types'])
required_enemy_terms=('garou','green dragon','blue dragon','red dragon','lesser demon')
names='\n'.join(t['name'].lower() for t in catalog['types'])
assert all(term in names for term in required_enemy_terms)
for black_knight in ('black knight warrior','black knight guardian','elite black knight','black knight sniper','black knight titan','sergeant blister','houndmaster rexiton'):
    assert black_knight in names
assert all(t.get('icon') for t in catalog['types'] if t['name'].lower() in ('black knight warrior','black knight guardian','elite black knight','black knight sniper','black knight titan','sergeant blister','houndmaster rexiton'))
exclusions=(ROOT/'deploy/npc-exclusions.tsv').read_text().lower()
assert 'blackdragon' in exclusions and 'outside-playable-map' in exclusions
assert not any('Post Box' in t['name'] or 'Library Book' in t['name'] for t in catalog['types'])
assert len(catalog['contentFingerprint'])==64
assert catalog['version']=='npcs-'+catalog['contentFingerprint'][:12]
suspect=next(n for n in catalog['nodes'] if n['id']=='ea7fc9b47cb4e8132555')
assert 'respawnGameSeconds' not in suspect
