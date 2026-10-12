"""Build the public quest-objective destination catalog from cooked assets.

Only game-authored relationships are publishable: exact QuestLocation links, or
quest-scoped GameplayObjectRegistry landmarks named by an objective/location
operation.  Wiki text is useful to review ordering, but never supplies coords.
"""
from __future__ import annotations
import json,math,re
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
CONTENT=ROOT/'dist/recipe-unlock-all-json-current'
CATALOG=json.loads((ROOT/'web/quest-catalog.json').read_text(encoding='utf-8'))
NPC=json.loads((ROOT/'web/npc-catalog.json').read_text(encoding='utf-8'))
OUTPUT=ROOT/'dist/quest-step-analysis.json'
PUBLIC=ROOT/'web/quest-step-locations.json'
COVERAGE=ROOT/'docs/quest-step-location-coverage.md'
OVERRIDES=ROOT/'deploy/quest-step-overrides.json'
NPC_IDENTITIES=ROOT/'deploy/npc-identities.tsv'

def txt(v):
    return (v or {}).get('LocalizedString') or (v or {}).get('SourceString') or '' if isinstance(v,dict) else ''

def ref_name(v):
    if not isinstance(v,dict):return ''
    name=v.get('ObjectName','')
    quoted=re.search(r"'([^']+)'",name)
    return (quoted.group(1) if quoted else name).split(':')[-1]

def slug(v):
    return re.sub(r'[^a-z0-9]+','-',v.lower()).strip('-')

def natural_key(v):
    return [int(part) if part.isdigit() else part.lower() for part in re.split(r'(\d+)',v)]

def words(v):
    stop={'the','and','for','from','with','into','near','return','speak','find','look','bring','visit','search','objective','quest','none','location','target','npc','area','step'}
    out=[]
    expanded=re.sub(r'(?<=[a-z0-9])(?=[A-Z])',' ',v or '')
    for word in re.findall(r'[a-z0-9]+',expanded.lower()):
        if word.endswith('s') and len(word)>4:word=word[:-1]
        if len(word)>2 and word not in stop:out.append(word)
    return set(out)

def location(x,y,z=None):
    mx=(float(x)+11075)/420000;my=(float(y)+117685)/420000
    if not (0<=mx<=1 and 0<=my<=1):return None
    return {'map':{'x':round(mx,8),'y':round(my,8)},'world':{'x':x,'y':y,'z':z}}

def location_update_is_destination(properties):
    return properties.get('Revealed',False) is True

def within_authored_area(marker,areas):
    return any(math.hypot(float(marker['world']['x'])-float(area['world']['x']),float(marker['world']['y'])-float(area['world']['y']))<=area['radius'] for area in areas)

def quest_scope_tokens(path,internal,public):
    values=[internal,public.get('name',''),path.parent.name,path.parent.parent.name]
    prefixes=('quest','data','main','side','sq','fh','dr','ums')
    generic={'quest','quests','gameplay','content','mainquest','sidequest','questdata'}
    result=set()
    for value in values:
        compact=re.sub(r'[^a-z0-9]','',str(value).lower())
        for prefix in prefixes:
            if compact.startswith(prefix) and len(compact)-len(prefix)>=6:compact=compact[len(prefix):]
        if len(compact)>=6 and compact not in generic:result.add(compact)
    return result

def scoped_landmark(table_id,tokens):
    table=re.sub(r'[^a-z0-9]','',(table_id or '').lower())
    return any(token in table or table in token for token in tokens if len(token)>=6)

def table_base(value):
    return (value or '').split('.',1)[0]

def point_in_polygon(x,y,polygon):
    inside=False;j=len(polygon)-1
    for i,(xi,yi) in enumerate(polygon):
        xj,yj=polygon[j]
        if (yi>y)!=(yj>y) and x<(xj-xi)*(y-yi)/(yj-yi)+xi:inside=not inside
        j=i
    return inside

def actor_location(actor,records):
    name=str(actor.get('Name',''))
    for component in records:
        props=component.get('Properties') or {};outer=str((component.get('Outer') or {}).get('ObjectName',''))
        if name and name in outer and isinstance(props.get('RelativeLocation'),dict):return props['RelativeLocation']
    return None

def dungeon_entrances():
    result={}
    maps=CONTENT/'RSDragonwilds'
    for path in maps.rglob('*.json'):
        if 'Maps' not in path.parts:continue
        try:raw=path.read_text(encoding='utf-8')
        except Exception:continue
        if 'DungeonEntrance_' not in raw:continue
        try:records=json.loads(raw)
        except Exception:continue
        if not isinstance(records,list):continue
        for actor in records:
            label=str(actor.get('ActorLabel',''));match=re.match(r'DungeonEntrance_([A-Za-z0-9_]+)',label)
            if not match:continue
            where=actor_location(actor,records)
            if not where:continue
            loc=location(where['X'],where['Y'],where.get('Z'))
            if not loc:continue
            internal=match.group(1);display=txt((actor.get('Properties') or {}).get('DisplayName')) or re.sub(r'(?<=[a-z0-9])(?=[A-Z])',' ',internal.replace('_',' '))
            result[internal]={'name':display+' entrance','interiorTarget':display,'kind':'dungeon_entrance',**loc,'source':str(path.relative_to(ROOT)).replace('\\','/'),'actor':label}
        # Newer interiors use ordinary teleport targets rather than the older
        # DungeonEntrance naming convention. These aliases come from exact
        # authored target/actor identities and their overworld transforms.
        aliases=(
            ('FightCaves','Fight Caves',lambda a:'fightcaveentrance' in str((a.get('Properties') or {}).get('TargetName','')).lower()),
            ('RunecraftingGuild','Runecrafting Guild',lambda a:'institute of magic & runecrafting' in str((a.get('Properties') or {}).get('TargetName','')).lower()),
            ('KalphiteHive','Kalphite Hive',lambda a:'kalphitequeen_arena_1_entry' in str(a.get('ActorLabel','')).lower()),
            ('DragonkinVault','Dragonkin Vault',lambda a:str(a.get('Type',''))=='BP_DK_VautEntrance_Locked_C'),
            ('SilverthornKeep','Silverthorn Keep',lambda a:'silverthorn fortress' in str((a.get('Properties') or {}).get('TargetName','')).lower()),
        )
        for internal,display,predicate in aliases:
            actor=next((a for a in records if predicate(a)),None)
            if not actor:continue
            where=actor_location(actor,records)
            if not where:continue
            loc=location(where['X'],where['Y'],where.get('Z'))
            if not loc:continue
            result[internal]={'name':display+' entrance','interiorTarget':display,'kind':'dungeon_entrance',**loc,'source':str(path.relative_to(ROOT)).replace('\\','/'),'actor':actor.get('ActorLabel') or actor.get('Name')}
    return result

def main():
    entrances=dungeon_entrances()
    overrides=json.loads(OVERRIDES.read_text(encoding='utf-8')) if OVERRIDES.exists() else {'schemaVersion':1,'relationships':[]}
    if overrides.get('schemaVersion')!=2:raise ValueError('unsupported quest-step override schema')
    npc_types={t['id']:t for t in NPC['types']}
    npc_nodes={}
    for n in NPC['nodes']:npc_nodes.setdefault(n['type'],[]).append(n)
    npc_identity={}
    for line in NPC_IDENTITIES.read_text(encoding='utf-8').splitlines():
        if not line or line.startswith('#'):continue
        fields=line.split('\t')
        if len(fields)>=5:npc_identity[fields[1]]={'stateKey':fields[0],'type':fields[2],'actorType':fields[3],'aiClass':fields[4]}
    for nodes in npc_nodes.values():
        for node in nodes:node.update(npc_identity.get(node.get('id'),{}))
    quest_assets={};authored_markers=[];landmarks=[];actor_placements={};actor_metadata={};regions=[]
    flow_files=[]
    for path in CONTENT.rglob('*.json'):
        is_quest='quest' in str(path).lower()
        is_world_map='Maps' in path.parts and 'World' in path.parts
        is_actor_definition=path.stem.startswith('BP_') and ('NPCs' in path.parts or 'Interactables' in path.parts)
        if not is_quest and not is_world_map and not is_actor_definition:continue
        try:raw=path.read_text(encoding='utf-8')
        except Exception:continue
        if not is_quest and not is_world_map and not is_actor_definition:continue
        if is_world_map and not is_quest and not any(token in raw for token in ('"RelativeLocation"','"GameplayObjectRegistry"','"BP_MapRegion_C"')):continue
        try:data=json.loads(raw)
        except Exception:continue
        if not isinstance(data,list):continue
        # Quest pickups and bespoke interactables are frequently implemented as
        # NPC-derived blueprints instead of BP_QuestLocation actors. Preserve
        # their authored display/quest identity so reviewed objective links can
        # publish the interaction point without degrading it to a class name.
        blueprint_type=path.stem
        default_object=next((item for item in data if item.get('Name')==f'Default__{blueprint_type}_C'),None)
        participant=next((item for item in data if item.get('Type')=='DomConversationParticipant'),None)
        if default_object:
            default_props=default_object.get('Properties') or {};visibility=default_props.get('NPCVisibilitySettings') or {}
            participant_props=(participant or {}).get('Properties') or {}
            display=txt(default_props.get('DisplayName')) or str(participant_props.get('DisplayName') or '')
            quest_ref=ref_name(visibility.get('QuestData'))
            participant_tag=str((participant_props.get('ParticipantTag') or {}).get('TagName',''))
            entry_tag=str((participant_props.get('EntryPointTag') or {}).get('TagName',''))
            if display or quest_ref or participant_tag or entry_tag:
                actor_metadata[blueprint_type]={
                    'displayName':display,'quest':quest_ref,
                    'questBool':visibility.get('VisibilityQuestBoolName',''),
                    'participantTag':participant_tag,'entryTag':entry_tag,
                    'definitionSource':str(path.relative_to(ROOT)).replace('\\','/'),
                }
        for item in data:
            if item.get('Type')=='QuestData':quest_assets[item.get('Name','')]=(path,item)
            props=item.get('Properties') or {};root=props.get('RootComponent') or {};root_match=re.search(r'\.(\d+)$',root.get('ObjectPath',''))
            component=data[int(root_match.group(1))] if root_match and int(root_match.group(1))<len(data) else {}
            where=(component.get('Properties') or {}).get('RelativeLocation')
            actor_type=str(item.get('Type','')).removesuffix('_C')
            if actor_type.startswith('BP_') and where:
                loc=location(where.get('X',1e20),where.get('Y',1e20),where.get('Z'))
                if loc:
                    actor_label=item.get('ActorLabel') or item.get('Name')
                    actor_placements.setdefault(actor_type,[]).append({'name':actor_label,'actor':actor_label,'kind':'world_actor',**loc,'source':str(path.relative_to(ROOT)).replace('\\','/'),'actorType':actor_type})
            if item.get('Type')=='BP_MapRegion_C':
                points=(((component.get('Properties') or {}).get('SplineCurves') or {}).get('position') or {}).get('Points') or []
                polygon=[(float(p['OutVal']['X']),float(p['OutVal']['Y'])) for p in points if isinstance(p.get('OutVal'),dict)]
                if len(polygon)>=3:
                    name=txt(props.get('RegionName'));tag=str((props.get('MapRegionTag') or {}).get('TagName',''))
                    aliases=words(name)|({'fellhollow'} if tag=='Location.Region.Fellhollow' else set())
                    regions.append({'name':name,'words':aliases,'polygon':polygon})
            if props.get('Quest') and props.get('QuestLocationName') and where:
                quest_name=ref_name(props.get('Quest'))
                if where and quest_name and props.get('QuestLocationName'):
                    loc=location(where['X'],where['Y'],where.get('Z'))
                    if loc:authored_markers.append({'quest':quest_name,'name':props['QuestLocationName'],'kind':str(props.get('Type','EQuestLocationType::Location')).split('::')[-1].lower(),'radius':float(props.get('Radius') or 0),**loc,'source':str(path.relative_to(ROOT)).replace('\\','/'),'actor':item.get('Name')})
            if item.get('Type')=='GameplayObjectRegistry':
                for row in (item.get('Properties') or {}).get('Landmarks') or []:
                    where=row.get('Location') or {};label=row.get('UserVisibleDescription') or {};loc=location(where.get('X',1e20),where.get('Y',1e20),where.get('Z'))
                    if not loc:continue
                    landmarks.append({'name':txt(label),'key':label.get('Key',''),'table':label.get('TableId',''),'category':str(row.get('Category','')).split('::')[-1].lower(),'tag':(row.get('GameplayTag') or {}).get('TagName',''),**loc,'source':str(path.relative_to(ROOT)).replace('\\','/'),'identifier':row.get('Identifier','')})
        if is_quest and any(str(x.get('Type','')).startswith(('QFT_','QFR_')) for x in data):flow_files.append((path,data))
    for actor_type,placements in actor_placements.items():
        metadata=actor_metadata.get(actor_type)
        if not metadata:continue
        for placement in placements:
            if metadata.get('displayName'):placement['name']=metadata['displayName']
            placement['questActor']={key:value for key,value in metadata.items() if value}
    for marker in landmarks:
        x=marker['world']['x'];y=marker['world']['y'];marker['regionWords']=sorted(set().union(*(r['words'] for r in regions if point_in_polygon(x,y,r['polygon']))))
    authored_by_identity={(m['quest'],m['name']):m for m in authored_markers}
    override_by_step={}
    for row in overrides.get('relationships',[]):
        keys=row.get('objectiveKeys') or [row.get('objectiveKey')]
        if row.get('objectiveKeys') and row.get('objectiveKey'):raise ValueError('override relationship may define objectiveKey or objectiveKeys, not both')
        if not isinstance(keys,list) or not keys or any(not isinstance(key,str) or not key for key in keys):raise ValueError(f'invalid override objective keys: {keys}')
        for key in keys:
            identity=(row.get('questId'),key)
            if not all(identity) or identity in override_by_step:raise ValueError(f'invalid or duplicate override relationship: {identity}')
            for target in row.get('targets',[]):
                if target.get('kind')=='npc' and target.get('allowMultiple') and row.get('destinationMode')!='multiple':raise ValueError(f'NPC allowMultiple requires destinationMode multiple: {identity}')
            override_by_step[identity]={**row,'objectiveKey':key}
    reports=[]
    for public in CATALOG['quests']:
        internal=next((line.split('\t')[2] for line in (ROOT/'web/quest-runtime-catalog.tsv').read_text(encoding='utf-8').splitlines() if line and not line.startswith('#') and line.split('\t')[0]==public['id']), '')
        asset=quest_assets.get(internal)
        if not asset:continue
        quest_path,quest=asset;p=quest.get('Properties') or {};scope=quest_scope_tokens(quest_path,internal,public)
        objective_tables={table_base((row.get('Value') or {}).get('TableId')) for row in p.get('ObjectiveTexts') or []}
        objective_tables.discard('')
        quest_landmarks=[row for row in landmarks if table_base(row['table']) in objective_tables or scoped_landmark(row['table'],scope)]
        objectives=[]
        for index,row in enumerate(p.get('ObjectiveTexts') or []):
            value=row.get('Value') or {};key=row.get('Key') or 'None';objectives.append({'index':index,'key':key,'text':txt(value),'table':value.get('TableId',''),'flowReferenced':False,'locationOperations':[],'candidates':[]})
        relevant=[]
        for path,data in flow_files:
            encoded=json.dumps(data,separators=(',',':'))
            if internal in encoded:relevant.append((path,data))
        for path,data in relevant:
            by_name={x.get('Name'):x for x in data}
            objective_nodes={x.get('Name'):(x.get('Properties') or {}).get('TextName','None') for x in data if x.get('Type')=='QFT_SetQuestObjective_C' and internal in json.dumps(x.get('Properties') or {})}
            direct_objective_parent_options={}
            for node_name,objective_key in objective_nodes.items():
                parent=ref_name((by_name.get(node_name,{}).get('Properties') or {}).get('ParentNode'))
                if parent:
                    direct_objective_parent_options.setdefault(parent,set()).add(objective_key)
            direct_objective_parents={parent:next(iter(keys)) for parent,keys in direct_objective_parent_options.items() if len(keys)==1}
            for objective_key in objective_nodes.values():
                objective=next((o for o in objectives if o['key']==objective_key),None)
                if objective:objective['flowReferenced']=True
            def owning_objective(name):
                seen=set()
                for _ in range(64):
                    if not name or name in seen:return None
                    if name in objective_nodes:return objective_nodes[name]
                    seen.add(name);name=ref_name((by_name.get(name,{}).get('Properties') or {}).get('ParentNode'))
                return None
            for node in data:
                if node.get('Type')!='QFT_UpdateQuestLocation_C':continue
                props=node.get('Properties') or {};parent=ref_name(props.get('ParentNode'))
                # Cooked quest graphs use both arrangements: an objective can
                # precede its location update, or it can name the final location
                # update as its ParentNode. The latter is an exact direct link
                # and must be checked before walking farther upstream.
                direct_objective_key=direct_objective_parents.get(node.get('Name'))
                objective_key=direct_objective_key or owning_objective(parent)
                if not objective_key:continue
                target=props.get('Quest Location Name','');op={'name':target,'revealed':location_update_is_destination(props),'source':str(path.relative_to(ROOT)).replace('\\','/'),'node':node.get('Name')}
                objective=next((o for o in objectives if o['key']==objective_key),None)
                if not objective:continue
                objective['locationOperations'].append(op)
                # An unrevealed update removes or keeps hidden a marker from a
                # prior phase. It is useful flow evidence, but it is not the
                # destination of the objective being established.
                if not op['revealed']:continue
                for marker in authored_markers:
                    if marker['quest']==internal and marker['name']==target:
                        objective['candidates'].append({**marker,'directObjectiveLink':bool(direct_objective_key),'evidence':('exact direct objective-parent QuestLocationName match to authored BP_QuestLocation' if direct_objective_key else 'exact quest ID and QuestLocationName match between quest-flow operation and authored BP_QuestLocation')})
                target_slug=slug(target)
                matches=[]
                for type_id,t in npc_types.items():
                    if target_slug and (target_slug==type_id or target_slug in type_id or type_id in target_slug):
                        for n in npc_nodes.get(type_id,[]):matches.append({'kind':'npc','type':type_id,'name':t['name'],'map':{'x':n['x'],'y':n['y']},'world':{'x':n.get('worldX'),'y':n.get('worldY'),'z':n.get('worldZ')},'evidence':'authored quest-location name matched an extracted NPC placement'})
                objective['candidates'].extend(matches)
                target_words=words(target)
                for marker in quest_landmarks:
                    label_words=words(marker['name'])|words(marker['key'])
                    if target_words and label_words and (target_words<=label_words or label_words<=target_words):
                        objective['candidates'].append({**marker,'kind':marker['category'] or 'landmark','evidence':'exact quest-scoped world-registry landmark named by authored quest-location operation'})
            # A node below an objective can reference a dungeon actor, exit,
            # level, or encounter. Match that stable identity to the separately
            # placed overworld DungeonEntrance actor with the same suffix.
            for node in data:
                objective_key=owning_objective(node.get('Name')) or owning_objective(ref_name((node.get('Properties') or {}).get('ParentNode')))
                if not objective_key:continue
                encoded_properties=json.dumps(node.get('Properties') or {},separators=(',',':'))
                encoded_node=re.sub(r'[^a-z0-9]','',encoded_properties.lower())
                for dungeon,entrance in entrances.items():
                    if re.sub(r'[^a-z0-9]','',dungeon.lower()) not in encoded_node:continue
                    objective=next((o for o in objectives if o['key']==objective_key),None)
                    if objective:objective['candidates'].append({**entrance,'evidence':'objective flow subtree references dungeon identity matched to authored overworld entrance'})
                referenced_types={value.removesuffix('_C') for value in re.findall(r'BP_[A-Za-z0-9_]+(?:_C)?',encoded_properties)}
                for actor_type in referenced_types:
                    placements=actor_placements.get(actor_type,[])
                    if not placements or len(placements)>32:continue
                    objective=next((o for o in objectives if o['key']==objective_key),None)
                    if objective:
                        objective['candidates'].extend({**placement,'evidence':'objective flow subtree references exact placed actor class'} for placement in placements)
        # Some objectives directly name their world object without a separate
        # UpdateQuestLocation node.  The registry's string-table path proves the
        # quest ownership; requiring two meaningful shared words (or one unique
        # long proper name) prevents broad words such as "door" from matching.
        for objective in objectives:
            objective_words=words(objective['text'])
            named=[]
            for dungeon,entrance in entrances.items():
                dungeon_words=words(entrance['interiorTarget']);shared=objective_words&dungeon_words
                if dungeon_words and dungeon_words<=objective_words:
                    named.append((len(shared)+20,1.0,{**entrance,'evidence':'authored objective text names dungeon with an authored overworld entrance'}))
            for marker in authored_markers:
                if marker['quest']!=internal:continue
                marker_words=words(marker['name']);shared=objective_words&marker_words
                if marker_words and (marker_words<=objective_words or len(shared)>=2 or any(len(word)>=7 for word in shared)):
                    named.append((len(shared)+10,len(shared)/max(1,len(marker_words)),{**marker,'evidence':'quest-owned BP_QuestLocation explicitly named by objective text'}))
            exact_table=table_base(objective.get('table'))
            objective_landmarks=[m for m in quest_landmarks if not exact_table or table_base(m['table'])==exact_table]
            for marker in objective_landmarks:
                label_words=words(marker['name'])|words(marker['key'])
                shared=objective_words&label_words
                strong=len(shared)>=2 or any(len(word)>=(6 if exact_table else 7) for word in shared)
                if strong:
                    named.append((len(shared),len(shared)/max(1,len(label_words)),marker))
            # Named NPC/object destinations often use a shared global string
            # table rather than the quest's table. Accept only a complete,
            # explicit authored display-name match from the objective text.
            global_groups={}
            for marker in landmarks:
                name_words=words(marker['name'])
                if not name_words:continue
                global_groups.setdefault(tuple(sorted(name_words)),[]).append(marker)
            for signature,markers_for_name in global_groups.items():
                name_words=set(signature)
                if len(markers_for_name)>12 or not name_words<=objective_words:continue
                if len(name_words)<2 and max(map(len,name_words))<5:continue
                selected=markers_for_name
                if len(selected)>1:
                    regional=[m for m in selected if set(m.get('regionWords',[]))&objective_words]
                    if regional:selected=regional
                    elif not re.search(r'\d+\s*(?:/\s*\d+)?',objective['text']):continue
                for marker in selected:
                    named.append((len(name_words)+5,1.0,{**marker,'kind':marker['category'] or 'landmark','evidence':'authored objective explicitly names exact world-registry landmark'}))
            # Dialogue/return objectives name one destination alongside context
            # about the prior targets. Prefer the most specifically named actor.
            if re.match(r'^\s*(return|speak|talk|tell|bring|give|show)\b',objective['text'],re.I) and named:
                best=max((score,coverage) for score,coverage,_ in named)
                named=[row for row in named if row[:2]==best]
            for _,_,marker in named:
                if 'evidence' in marker:objective['candidates'].append(marker)
                else:objective['candidates'].append({**marker,'kind':marker['category'] or 'landmark','evidence':'quest-scoped world-registry landmark explicitly named by objective text'})
        # Counter variants are separate authored objective keys but point at the
        # same target set (for example 0/13 through 12/13). Share only between
        # objectives whose normalized game-authored text has the same word set.
        families={}
        for objective in objectives:families.setdefault(tuple(sorted(words(objective['text']))),[]).append(objective)
        for signature,family in families.items():
            if not signature or len(family)<2:continue
            proven=[]
            for objective in family:
                proven.extend(c for c in objective['candidates'] if c.get('evidence','').startswith(('exact quest ID','exact direct','exact quest-scoped','quest-scoped','quest-owned','objective flow','authored objective')))
            if not proven:continue
            for objective in family:
                for candidate in proven:
                    objective['candidates'].append({**candidate,'evidence':'same authored objective-text family as a game-proven destination'})
        # Reviewed overrides establish only the relationship between an
        # objective and an extracted game identity. They never contain or
        # manufacture coordinates. Missing identities therefore fail closed
        # after a game update instead of leaving a stale marker on the map.
        for objective in objectives:
            step_identity=(public['id'],objective['key'])
            override=override_by_step.get(step_identity)
            if not override:continue
            objective['override']={
                'destinationMode':override.get('destinationMode'),
                'note':override.get('note',''),
                'reason':override.get('reason',''),
                'allowPartialTargets':bool(override.get('allowPartialTargets')),
            }
            override_candidates=[];unresolved=[]
            for target in override.get('targets',[]):
                kind=target.get('kind');reference=target.get('reference','');resolved=[]
                if kind=='quest_marker':
                    marker=authored_by_identity.get((internal,reference))
                    if marker:resolved=[marker]
                elif kind=='quest_marker_prefix':
                    # Counter objectives often reveal a family of independently
                    # authored targets (for example all collectible fragments).
                    # Resolve the family from this build instead of copying a
                    # coordinate list into the reviewed override.
                    resolved=[marker for (owner,name),marker in authored_by_identity.items() if owner==internal and name.startswith(reference)]
                    resolved.sort(key=lambda marker:natural_key(marker.get('name','')))
                elif kind=='dungeon_entrance':
                    marker=entrances.get(reference)
                    if marker:resolved=[marker]
                elif kind=='npc':
                    npc_type=npc_types.get(reference)
                    if npc_type:
                        nodes=npc_nodes.get(reference,[])
                        if target.get('actorType'):nodes=[n for n in nodes if n.get('actorType')==target['actorType']]
                        if target.get('opaqueId'):nodes=[n for n in nodes if n.get('id')==target['opaqueId']]
                        if len(nodes)>1 and not target.get('allowMultiple'):raise ValueError(f"ambiguous NPC override {step_identity}: '{reference}' resolved {len(nodes)} placements; specify actorType/opaqueId or explicitly allowMultiple")
                        resolved=[{'kind':'npc','type':reference,'actorType':n.get('actorType'),'opaqueId':n.get('id'),'name':npc_type['name'],'map':{'x':n['x'],'y':n['y']},'world':{'x':n.get('worldX'),'y':n.get('worldY'),'z':n.get('worldZ')},'source':'web/npc-catalog.json'} for n in nodes]
                elif kind=='world_actor':
                    resolved=[placement for placements in actor_placements.values() for placement in placements if placement.get('actor')==reference or placement.get('name')==reference]
                elif kind=='world_actor_type':
                    resolved=list(actor_placements.get(reference,[]))
                elif kind=='registry_landmark':
                    resolved=[marker for marker in landmarks if marker.get('identifier')==reference]
                if not resolved:unresolved.append(f'{kind}:{reference}')
                for marker in resolved:
                    qualifier=f" actor type '{target.get('actorType')}'" if target.get('actorType') else f" placement '{target.get('opaqueId')}'" if target.get('opaqueId') else ''
                    resolved_marker={**marker}
                    if target.get('displayName'):resolved_marker['name']=target['displayName']
                    override_candidates.append({**resolved_marker,'role':target.get('role','destination'),'evidence':f"reviewed objective relationship to extracted game {kind.replace('_',' ')} '{reference}'{qualifier}: {override.get('reason','')}"})
            if unresolved:raise ValueError(f'unresolved override targets for {step_identity}: {unresolved}')
            # A precise reviewed target may refine a broad game-authored area,
            # but it may not contradict it. This catches cross-region/cross-beat
            # mistakes such as mapping the Whispering Swamp objective to
            # Velgar's Rise while still allowing its exact vault entrance.
            direct_areas=[c for c in objective['candidates'] if c.get('directObjectiveLink') and c.get('kind')=='area' and c.get('radius',0)>0]
            for marker in override_candidates:
                if direct_areas and not within_authored_area(marker,direct_areas):
                    raise ValueError(f"override target '{marker.get('name')}' for {step_identity} falls outside its directly authored quest area")
            if 'targets' in override:
                # Reviewed targets are authoritative refinements. Retaining
                # heuristic candidates beside them makes a fixed destination
                # silently publish unrelated markers.
                objective['candidates']=override_candidates
        reports.append({'id':public['id'],'name':public['name'],'internal':internal,'objectives':objectives})
    payload={'schemaVersion':2,'catalogFingerprint':CATALOG['fingerprint'],'quests':reports,'summary':{'quests':len(reports),'objectives':sum(len(q['objectives']) for q in reports),'excludedTechnicalObjectives':sum(not o['text'] and not o['flowReferenced'] for q in reports for o in q['objectives']),'authoredWorldMarkers':len(authored_markers),'worldRegistryLandmarks':len(landmarks),'reviewedRelationships':len(override_by_step),'objectivesWithAuthoredLocation':sum(bool(o['locationOperations']) for q in reports for o in q['objectives']),'objectivesWithPlacementCandidate':sum(bool(o['candidates']) for q in reports for o in q['objectives'])}}
    OUTPUT.write_text(json.dumps(payload,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    public_quests=[]
    for quest in reports:
        steps=[]
        for objective in quest['objectives']:
            if not objective['text'] and not objective['flowReferenced']:continue
            markers=[];seen=set()
            for candidate in objective['candidates']:
                if not candidate.get('evidence','').startswith(('exact quest ID','exact direct','exact quest-scoped','quest-scoped','quest-owned','objective flow','authored objective','same authored','reviewed objective')):continue
                key=(candidate['map']['x'],candidate['map']['y'],candidate.get('name',''))
                if key in seen:continue
                seen.add(key);markers.append({'name':candidate.get('name',''),'kind':candidate.get('kind','location'),'role':candidate.get('role','destination'),'map':candidate['map'],'world':candidate.get('world'),'source':candidate.get('source'),'evidence':candidate.get('evidence'),'interiorTarget':candidate.get('interiorTarget'),'actorType':candidate.get('actorType'),'questActor':candidate.get('questActor')})
            override=objective.get('override') or {};mode=override.get('destinationMode') or ('multiple' if len(markers)>1 else 'fixed' if markers else 'unavailable')
            semantic_warnings=[]
            by_name={}
            for marker in markers:by_name.setdefault(slug(marker.get('name','')),[]).append(marker)
            ambiguous_names=[group[0].get('name','') for name,group in by_name.items() if name and len({(m['map']['x'],m['map']['y']) for m in group})>1]
            if ambiguous_names and not override:
                semantic_warnings.append('Same display name resolves to distinct world placements without a reviewed identity: '+', '.join(sorted(set(ambiguous_names))))
            # Require the complete imperative verb. Without the trailing word
            # boundary, a completed counter such as "Collected 3/3 — Return"
            # is misread as a request to collect three more targets.
            count_match=re.search(r'\b(?:find|collect|gather|recover|retrieve|activate|defeat|visit)\b\D{0,32}(\d+)\b',objective['text'],re.I)
            if count_match and mode!='dynamic' and not override.get('allowPartialTargets'):
                expected=int(count_match.group(1));action_markers=[m for m in markers if m.get('role')!='return']
                if expected>1 and len(action_markers)<expected:
                    semantic_warnings.append(f'Objective text requests {expected} targets but only {len(action_markers)} non-return marker(s) are published.')
            steps.append({'key':objective['key'],'text':objective['text'],'destinationMode':mode,'markers':markers,'available':bool(markers),'reviewedRelationship':bool(override),'semanticWarnings':semantic_warnings,'destinationNote':override.get('note',''),'unavailableReason':'' if markers else ('This objective is location-dependent but has no single authored destination.' if mode=='dynamic' else 'No unambiguous authored world placement is currently linked to this objective.')})
        public_quests.append({'id':quest['id'],'steps':steps})
    PUBLIC.write_text(json.dumps({'schemaVersion':2,'catalogFingerprint':CATALOG['fingerprint'],'quests':public_quests},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    playable_objectives=sum(len(q['steps']) for q in public_quests)
    lines=['# Quest step location coverage','',f"Catalog fingerprint: `{CATALOG['fingerprint']}`",'',f"This report covers {payload['summary']['quests']} quests and {playable_objectives} reachable player objectives. {payload['summary']['excludedTechnicalObjectives']} blank objective-table records that are never referenced by compiled quest flow remain in the internal analysis but are excluded from the public catalog. A mapped objective has an exact quest-location link, a quest-scoped world-registry landmark, or a dungeon identity linked to its authored overworld entrance. Wiki coordinates are never accepted.",'',f"The current build contains {len(entrances)} verified overworld dungeon entrances. Interior steps use those entrances and retain the interior target name as marker metadata.",'','| Quest | Objectives | Mapped | Missing |','|---|---:|---:|---:|']
    for quest in public_quests:
        mapped=sum(s['available'] for s in quest['steps']);lines.append(f"| {next(q['name'] for q in reports if q['id']==quest['id']).replace('|','\\|')} | {len(quest['steps'])} | {mapped} | {len(quest['steps'])-mapped} |")
    lines+=['','## Missing objective evidence','']
    for quest in public_quests:
        missing=[s for s in quest['steps'] if not s['available']]
        if not missing:continue
        name=next(q['name'] for q in reports if q['id']==quest['id']);lines+=['',f'### {name}','']
        for step in missing:lines.append(f"- `{step['key']}` — {step['text'] or '[missing localized text in the game asset]'}")
    lines+=['','## Safety behavior','','- Active objectives use only exact authored markers from this dataset.','- An active objective without a verified marker displays no destination marker. It never falls back to the quest start point.','- Unstarted and completed quests may continue to show the separately verified quest-start location.','- Interior objectives require an independently verified overworld entrance before they can be marked on the world map.','- The runtime joins progress to this catalog by current objective text; mismatches fail closed.']
    COVERAGE.write_text('\n'.join(lines)+'\n',encoding='utf-8')
    print(json.dumps(payload['summary']))

if __name__=='__main__':main()
