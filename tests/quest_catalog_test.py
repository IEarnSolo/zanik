import json
import sys
from pathlib import Path
root=Path(__file__).parents[1]
sys.path.insert(0,str(root/'tools'))
from analyze_quest_steps import location_update_is_destination,within_authored_area
assert location_update_is_destination({'Revealed':True})
assert not location_update_is_destination({'Revealed':False})
assert not location_update_is_destination({})
test_area={'world':{'x':100.0,'y':100.0},'radius':25.0}
assert within_authored_area({'world':{'x':115.0,'y':110.0}},[test_area])
assert not within_authored_area({'world':{'x':200.0,'y':200.0}},[test_area])
catalog=json.loads((root/'web/quest-catalog.json').read_text(encoding='utf-8'))
quests=catalog['quests']
assert len(quests)>=20
assert len({q['id'] for q in quests})==len(quests)
assert {q['type'] for q in quests}=={'main','side'}
assert not any(any(word in q['name'].lower() for word in ('ftue','craft dreamstone','craft gatekeeper','item pack','journal quest')) for q in quests)
for quest in quests:
    assert quest['name'] and quest['region'] and quest['starter'] and quest['icon'].startswith('/assets/quests/')
    if quest['map'] is not None: assert 0<=quest['map']['x']<=1 and 0<=quest['map']['y']<=1
runtime=(root/'web/quest-runtime-catalog.tsv').read_text(encoding='utf-8').splitlines()
assert f'count={len(quests)}' in runtime[0] and catalog['fingerprint'] in runtime[0]
assert len([line for line in runtime if line and not line.startswith('#')])==len(quests)
assert {line.split('\t',1)[0] for line in runtime if line and not line.startswith('#')}=={q['id'] for q in quests}
page=(root/'src/web_page.h').read_text(encoding='utf-8')
for token in ('>Quests</summary>','/api/player-quests','/api/quests/catalog','questRegion','questSort','questHideCompleted','zanik-quests-hide-completed','setInterval(refreshQuests, 2000)'): assert token in page
steps=json.loads((root/'web/quest-step-locations.json').read_text(encoding='utf-8'))
assert steps['schemaVersion']==2
assert steps['catalogFingerprint']==catalog['fingerprint']
assert len(steps['quests'])==len(quests)
all_steps=[step for quest in steps['quests'] for step in quest['steps']]
assert len(all_steps)==230 and all(step['text'] for step in all_steps)
assert all(step['available'] or step['destinationMode'] in ('dynamic','unavailable') for step in all_steps)
assert all(isinstance(step.get('semanticWarnings'),list) and isinstance(step.get('reviewedRelationship'),bool) for step in all_steps)
analysis=json.loads((root/'dist/quest-step-analysis.json').read_text(encoding='utf-8'))
assert analysis['summary']['objectives']==232 and analysis['summary']['excludedTechnicalObjectives']==2
overrides=json.loads((root/'deploy/quest-step-overrides.json').read_text(encoding='utf-8'))
assert overrides['schemaVersion']==2 and len(overrides['relationships'])>=20
assert not any(any(field in target for field in ('map','world','x','y','z')) for row in overrides['relationships'] for target in row.get('targets',[]))
assert all(row.get('note') for row in overrides['relationships'] if row.get('allowPartialTargets'))
dragon_override=next(row for row in overrides['relationships'] if row['questId']=='yrRZnkmro-IvH_OmZ3f-5A' and row['objectiveKey']=='Locate cathan in the vault')
assert dragon_override['targets']==[{'kind':'dungeon_entrance','reference':'VertentisKara'}]
npcs=json.loads((root/'web/npc-catalog.json').read_text(encoding='utf-8'))
npc_counts={kind['id']:kind['count'] for kind in npcs['types']}
for row in overrides['relationships']:
    for target in row.get('targets',[]):
        if target.get('kind')!='npc' or npc_counts.get(target.get('reference'),0)<=1: continue
        assert target.get('actorType') or target.get('opaqueId') or target.get('stateKey') or target.get('allowMultiple'), (row['questId'],row['objectiveKey'],target)
        if target.get('allowMultiple'): assert row['destinationMode']=='multiple'
abraxus=next(step for quest in steps['quests'] if quest['id']=='WXRGL036pJRhAU-BWTZ_9g' for step in quest['steps'] if step['key']=='ObjectiveZero')
assert abraxus['destinationMode']=='fixed' and len(abraxus['markers'])==1
assert abraxus['markers'][0]['name']=='Abraxus The Eternal'
assert abraxus['markers'][0]['map']=={'x':0.50379445,'y':0.10259045}
what_remains={step['key']:step for quest in steps['quests'] if quest['id']=='WXRGL036pJRhAU-BWTZ_9g' for step in quest['steps']}
book_names={
    'Aetheric Fundamentals, a Primordial Primer',
    'The Pride of the Avernic',
    'The Wrath of Baba Potterington',
    'The Importance of Chaos',
    'Leonard The Bound',
}
scroll_names={'The Binding of Ib','Coclear Malificarum','Eventide - A New Spawn'}
for key in ('FindBooks','FindBooksAndReturn'):
    assert {marker['name'] for marker in what_remains[key]['markers'] if marker['role']=='collect'}==book_names
    assert sum(marker['role']=='return' and marker['map']=={'x':0.50379445,'y':0.10259045} for marker in what_remains[key]['markers'])==1
for key in ('FindScrolls','FindScrollsAndReturn'):
    assert {marker['name'] for marker in what_remains[key]['markers'] if marker['role']=='collect'}==scroll_names
assert len(what_remains['FindScrolls']['markers'])==3
for key in ('FoundBooks','FoundScrolls'):
    assert what_remains[key]['destinationMode']=='fixed' and len(what_remains[key]['markers'])==1
    assert what_remains[key]['markers'][0]['map']=={'x':0.50379445,'y':0.10259045}
assert not any(marker['map']=={'x':0.50900629,'y':0.40049381} for step in what_remains.values() for marker in step['markers'])
assert all(marker.get('actorType','').startswith('BP_NPC_FH_WhatRemains_') for key in ('FindBooks','FindBooksAndReturn','FindScrolls','FindScrollsAndReturn') for marker in what_remains[key]['markers'] if marker['role']=='collect')
assert all(step['reviewedRelationship'] and not step['semanticWarnings'] for step in what_remains.values())
restless={step['key']:step for quest in steps['quests'] if quest['id']=='eKV5vkDqZH89aGWiY_T7GQ' for step in quest['steps']}
assert [marker['name'] for marker in restless['Found ghost amulet']['markers']]==['Warren']
assert {marker['name'] for marker in restless['Put ghosts to rest']['markers']}=={'Warren','Lazina','Finn','Kay','Remembrance Shrine'}
assert sum(marker['role']=='finish' for marker in restless['Put ghosts to rest']['markers'])==1
black_knights={step['key']:step for quest in steps['quests'] if quest['id']=='3MpWS0mD8A6yFVmjVAA2Qg' for step in quest['steps']}
for key in ['DR_Main_Objective_4']+[f'DR_Main_Objective_4_{index}' for index in range(1,13)]:
    assert len(black_knights[key]['markers'])==13
    assert {marker['name'] for marker in black_knights[key]['markers']}=={f'DR_MQ_Fragment_{index}' for index in range(1,14)}
    assert all(marker['role']=='collect' for marker in black_knights[key]['markers'])
for key in ('DR_Main_Objective_3','DR_Main_Objective_4_13'):
    assert len(black_knights[key]['markers'])==1 and black_knights[key]['markers'][0]['name']=='Dreaming Stone'
assert len(black_knights['DR_Main_Objective_6']['markers'])==1
assert black_knights['DR_Main_Objective_6']['markers'][0]['name']=='Avisk — restored Dreaming Stone'
assert black_knights['DR_Main_Objective_3']['markers'][0]['role']=='investigate'
assert all(black_knights[key]['markers'][0]['role']=='return' for key in ('DR_Main_Objective_6','DR_Main_Objective_4_13'))
more_ghosts={step['key']:step for quest in steps['quests'] if quest['id']=='4QQuNkR6spTCXC6zj6VqxQ' for step in quest['steps']}
for key in ('FindGhosts0/3','FindGhosts1/3','FindGhosts2/3'):
    assert {marker['name'] for marker in more_ghosts[key]['markers']}=={'Windswept Ghost','Reminiscing Ghost','Paint-Spattered Ghost','Dust-covered Sketchbook'}
    assert not any(marker['name']=='ZanikNPC' for marker in more_ghosts[key]['markers'])
animal={step['key']:step for quest in steps['quests'] if quest['id']=='0WS3LUBsaPUDeuiPT_akhQ' for step in quest['steps']}
assert [marker['name'] for marker in animal['None']['markers']]==['Advert-Inna-Bottle']
assert [marker['name'] for marker in animal['Objective2']['markers']]==['Manktongue']
assert {marker['name'] for marker in animal['Objective3']['markers']}=={
    'Undead Chicken','Undead Twig Bundle','Pulpknuckle — magnetise iron bar','Manktongue — component hand-in'}
assert [marker['name'] for marker in animal['Objective4']['markers']]==['Manktongue']
heartstrings={step['key']:step for quest in steps['quests'] if quest['id']=='xsgOzUS-M2iCLDe-XTDq3w' for step in quest['steps']}
assert len(heartstrings['follow the blue-leafed trees']['markers'])==7
assert all(marker['actorType']=='BP_NPC_Hearstrings_Memorial' for marker in heartstrings['follow the blue-leafed trees']['markers'])
beast={step['key']:step for quest in steps['quests'] if quest['id']=='0cB0D0QD_L4nrpu6CxZEsw' for step in quest['steps']}
assert [marker['name'] for marker in beast['FollowBeast']['markers']]==[
    'Dearg Due trail — first sighting','Dearg Due trail — second sighting',
    'Dearg Due trail — third sighting',"Efina's hiding place"]
assert [marker['name'] for marker in beast['GiveWater']['markers']]==['Efina — The Lost']
assert [marker['name'] for marker in beast['Return']['markers']]==['Manathep — The Bereaved']
mark_day={step['key']:step for quest in steps['quests'] if quest['id']=='nIvEyEv-kvSwgqOLc5VopQ' for step in quest['steps']}
assert {marker['name'] for marker in mark_day['ConvinceNoble.0']['markers']}=={
    'Brialka — Historian Councillor','Alabrais — Science Councillor','Chirot — Broker Councillor'}
assert [marker['name'] for marker in mark_day['ReturnToShangruuh']['markers']]==['Shangruuh — Uzzer Camp']
scaling={step['key']:step for quest in steps['quests'] if quest['id']=='Bnon4kzm_EdvZz-bma5HBg' for step in quest['steps']}
assert {marker['name'] for marker in scaling['SearchLocation']['markers']}=={
    'Lost scout — village ruins','Lost scout — eastern village','Scorched wall clue'}
assert [marker['name'] for marker in scaling['FindDragon']['markers']]==['The Radiant — red dragon encounter']
dig_site={step['key']:step for quest in steps['quests'] if quest['id']=='8Rb0t0upslwKHXWYGnTKgg' for step in quest['steps']}
for key in ('DigScroll.0','DigScroll.1','DigScroll.2','DigScroll.3'):
    assert len(dig_site[key]['markers'])==4
    assert all(marker['actorType']=='BP_UmS_Mainquest_TribeB_Task2_ArchaeologyDigSite' for marker in dig_site[key]['markers'])
boom={step['key']:step for quest in steps['quests'] if quest['id']=='wwEK50EPipTecO20yTquSQ' for step in quest['steps']}
boom_hunt=boom['FindSpectralChinchompaSkulls']['markers']
assert sum(marker['name']=='Spectral Chinchompa spawn' and marker['role']=='hunt' for marker in boom_hunt)==26
assert sum(marker['name']=='Vannaka' and marker['role']=='return' for marker in boom_hunt)==1
letters={step['key']:step for quest in steps['quests'] if quest['id']=='4kbEAUJrs6mebhGkzFQxmA' for step in quest['steps']}
assert [marker['name'] for marker in letters['ObjectiveZero']['markers']]==['Postie Pete (Fellhollow)']
assert {(marker['name'],marker['role']) for marker in letters['ObjectiveOne']['markers']}=={
    ('Suspicious Skeleton (Letter 1)','collect'),('Postbox 1 (Letter 1 delivery)','deliver')}
assert {(marker['name'],marker['role']) for marker in letters['ObjectiveTwo']['markers']}=={
    ('Spectral hanging letter (Letter 2)','collect'),('Postbox 2 (Letter 2 delivery)','deliver')}
assert {(marker['name'],marker['role']) for marker in letters['ObjectiveThree']['markers']}=={
    ('Shaun (Letter 3)','collect'),('Postbox 3 (Letter 3 delivery)','deliver')}
assert [marker['name'] for marker in letters['ObjectiveReturn']['markers']]==['Postie Pete (Fellhollow)']
assert all(letters[key]['reviewedRelationship'] and not letters[key]['semanticWarnings'] for key in letters)
biohazard={step['key']:step for quest in steps['quests'] if quest['id']=='2kXJ3kWFS38kZfmPnvUFXw' for step in quest['steps']}
biohazard_hunt=biohazard['DR_SQ3_HuntRavens']['markers']
assert sum(marker['name']=='Corrupted Raven spawn' and marker['role']=='hunt' for marker in biohazard_hunt)==10
assert sum(marker['name']=='Zanik (sample hand-in)' and marker['role']=='return' for marker in biohazard_hunt)==1
assert biohazard['DR_SQ3_HuntRavens']['reviewedRelationship'] and not biohazard['DR_SQ3_HuntRavens']['semanticWarnings']
wanted={step['key']:step for quest in steps['quests'] if quest['id']=='9Kod2UgSkm8YrJCwMgmAdQ' for step in quest['steps']}
assert [(marker['name'],marker['role']) for marker in wanted['DR_SQ2_ReturnToVannaka']['markers']]==[('Vannaka (Wanted! hand-in)','return')]
assert wanted['DR_SQ2_ReturnToVannaka']['reviewedRelationship'] and not wanted['DR_SQ2_ReturnToVannaka']['semanticWarnings']
cooks={step['key']:step for quest in steps['quests'] if quest['id']=='cZlfrUaPSsi4VfeVc6iIEw' for step in quest['steps']}
assert {(marker['name'],marker['role']) for marker in cooks['Objective1']['markers']}=={
    ('Dairy Cow (Bucket of Milk)','collect'),('Slopfinger (ingredient hand-in)','return')}
assert [(marker['name'],marker['role']) for marker in cooks['Objective2']['markers']]==[('Goblin Stove','interact')]
assert all(cooks[key]['reviewedRelationship'] and not cooks[key]['semanticWarnings'] for key in ('Objective1','Objective2'))
rogue={step['key']:step for quest in steps['quests'] if quest['id']=='d6s4dUyoCduLy7eRc2KndQ' for step in quest['steps']}
assert [(marker['name'],marker['role']) for marker in rogue['FindMoonstone']['markers']]==[
    ('Moonstone 1 — northern cave','collect'),('Moonstone 2 — rock platforms','collect'),('Moonstone 3 — Ruby Vine cave','collect')]
assert [(marker['name'],marker['role']) for marker in rogue['ReturnToShopkeeper']['markers']]==[('Domri — The Merchant','return')]
assert all(rogue[key]['reviewedRelationship'] and not rogue[key]['semanticWarnings'] for key in rogue)
contact={step['key']:step for quest in steps['quests'] if quest['id']=='A3DOkkGZxXUHc82rPKHznA' for step in quest['steps']}
assert [(marker['name'],marker['role']) for marker in contact['Objective1']['markers']]==[('Vault Guardian Prime — Kalphite Hive entrance','enter')]
assert {(marker['name'],marker['role']) for marker in contact['Objective2']['markers']}=={
    ('Damaged guardian — Kalphite Hive entrance','collect'),
    ('Dead Vault Guardian — kalphite eggs','investigate'),
    ('Buried Vault Guardian — ruined building','dig-and-collect')}
assert all(contact[key]['reviewedRelationship'] and not contact[key]['semanticWarnings'] for key in ('Objective1','Objective2'))
mirror={step['key']:step for quest in steps['quests'] if quest['id']=='KLBpt02O2AA_kaa_A8Av9w' for step in quest['steps']}
assert {(marker['name'],marker['role']) for marker in mirror['FindSilverThread']['markers']}=={
    ('Bedraggled Spellbook — Emberwood windmill','collect'),
    ('Edna — spectral silver thread','craft'),
    ('Nearby Green Dragon — dragon blood','hunt'),
    ('Magic Mirror — reagent hand-in','return')}
assert [marker['name'] for marker in mirror['LightCandles']['markers']]==[
    'Memorial Candle — Fellhollow mirror','Memorial Candle — Fractured Plains lake',
    'Memorial Candle — Stormtouched cliff','Memorial Candle — Bloodblight Swamp',
    'Memorial Candle — Whispering Swamp tree','Memorial Candle — Bramblemead clifftop']
assert all(mirror[key]['reviewedRelationship'] and not mirror[key]['semanticWarnings'] for key in ('FindSilverThread','LightCandles'))
withering={step['key']:step for quest in steps['quests'] if quest['id']=='uftGB0es-CGrvXyPoxW31w' for step in quest['steps']}
assert [(marker['name'],marker['role']) for marker in withering['SpeakToPostiePete']['markers']]==[('Postie Pete — Forgotten Temple','talk')]
assert [(marker['name'],marker['role']) for marker in withering['InvestigateStandingStones']['markers']]==[
    ('Cathan — Bleakfields cliff','talk'),('Standing Stone — first curse clue','investigate'),
    ('Standing Stone — second curse clue','investigate'),('Standing Stone — third curse clue','investigate')]
assert all(withering[key]['reviewedRelationship'] and not withering[key]['semanticWarnings'] for key in ('SpeakToPostiePete','InvestigateStandingStones'))
brink={step['key']:step for quest in steps['quests'] if quest['id']=='sQ91eUA55bcQx0OQ0m59sg' for step in quest['steps']}
brink_pedestals=['Fire Pedestal — north of Sinking Pyramid','Fire Pedestal — east rock formation','Fire Pedestal — south of Sinking Pyramid','Fire Pedestal — west of Sinking Pyramid']
for key in ('Objective2','Objective2.1','Objective2.2','Objective2.3'):
    assert [marker['name'] for marker in brink[key]['markers']]==brink_pedestals
    assert brink[key]['reviewedRelationship'] and not brink[key]['semanticWarnings']
mapping_i={step['key']:step for quest in steps['quests'] if quest['id']=='NjeUO0TBeyQGzSCmu2b3bA' for step in quest['steps']}
mapping_i_names=['Survey I Moon Sprite 1 — east of Alcarrid Camp','Survey I Moon Sprite 2 — southeast height','Survey I Moon Sprite 3 — southwest height']
for key in ('Objective1','0/3','1/3','2/3','3/3'):
    assert [marker['name'] for marker in mapping_i[key]['markers']]==mapping_i_names
    assert mapping_i[key]['reviewedRelationship'] and not mapping_i[key]['semanticWarnings']
mapping_ii={step['key']:step for quest in steps['quests'] if quest['id']=='rI4VkUT_8837ayKcLgxWwQ' for step in quest['steps']}
mapping_ii_names=[f'Survey II Moon Sprite {number}' for number in (1,2,3,4,5,6,7,9)]
for key in ('Objective1','0/8','1/8','2/8','3/8','4/8','5/8','6/8','7/8'):
    assert [marker['name'] for marker in mapping_ii[key]['markers']]==mapping_ii_names
    assert mapping_ii[key]['reviewedRelationship'] and not mapping_ii[key]['semanticWarnings']
icthlarin={step['key']:step for quest in steps['quests'] if quest['id']=='kPtTW0exXrqw2BqELR_bFA' for step in quest['steps']}
assert [(marker['name'],marker['role']) for marker in icthlarin['FindGarou']['markers']]==[('Ancamairon — Alcarrid Elder','talk')]
assert [(marker['name'],marker['role']) for marker in icthlarin['InvestigateGarouProblem']['markers']]==[('Alcarrid camp investigation area','investigate')]
assert [(marker['name'],marker['role']) for marker in icthlarin['ReturnToFirstGarou']['markers']]==[('Shangruuh — Dowdun Reach passage','return')]
assert [marker['name'] for marker in icthlarin['FindThreeKeys']['markers']]==[
    'Ancamairon — Alcarrid Elder','Dúshlánach — Manafem Elder','Gaimbín — Uzzer Elder']
alcarrid_tasks=['Valas — The Mirror Crack\'d','Domri — Rogue Trader','Lagra — Rag and Bone Garou']
for key in ('FirstGarouEarnReputation.NoReputation','FirstGarouEarnReputation.LowReputation','FirstGarouEarnReputation.MediumReputation','FirstGarouEarnReputation.HighReputation'):
    assert [marker['name'] for marker in icthlarin[key]['markers']]==alcarrid_tasks
assert [marker['name'] for marker in icthlarin['FirstGarouEarnReputation.MaxReputation']['markers']]==['Ancamairon — Alcarrid Elder']
assert [marker['name'] for marker in icthlarin['SpeakToGarouElder']['markers']]==['Ancamairon — Alcarrid Elder']
assert [marker['name'] for marker in icthlarin['RepairTheMirrors']['markers']]==[
    'Mirror Relay — lens socket 1','Mirror Relay — lens socket 2','Mirror Relay — lens socket 3']
assert [marker['name'] for marker in icthlarin['FindLastKey']['markers']]==[
    'Dúshlánach — Manafem Elder','Gaimbín — Uzzer Elder']
assert all(icthlarin[key]['reviewedRelationship'] and not icthlarin[key]['semanticWarnings'] for key in (
    'FindGarou','InvestigateGarouProblem','ReturnToFirstGarou','FindThreeKeys',
    'FirstGarouEarnReputation.NoReputation','FirstGarouEarnReputation.LowReputation',
    'FirstGarouEarnReputation.MediumReputation','FirstGarouEarnReputation.HighReputation',
    'FirstGarouEarnReputation.MaxReputation','SpeakToGarouElder','RepairTheMirrors','FindLastKey'))
assert [marker['name'] for marker in icthlarin['FightFuzan']['markers']]==['Fight Caves entrance']
assert [marker['name'] for marker in icthlarin['SlayFuzan']['markers']]==['Fight Caves entrance']
assert [marker['name'] for marker in icthlarin['ReturnToIcthlarin']['markers']]==['Icthlarin — Mirror Relay']
contact_progress={step['key']:step for quest in steps['quests'] if quest['id']=='A3DOkkGZxXUHc82rPKHznA' for step in quest['steps']}
for key in ('Objective3','Objective4','Objective6'):
    assert [marker['name'] for marker in contact_progress[key]['markers']]==['Kalphite Hive entrance']
for key in ('CrystalShard.1','CrystalShard.2'):
    assert [marker['name'] for marker in contact_progress[key]['markers']]==[
        'Damaged guardian — Kalphite Hive entrance','Dead Vault Guardian — kalphite eggs','Buried Vault Guardian — ruined building']
withering_fragments={step['key']:step for quest in steps['quests'] if quest['id']=='uftGB0es-CGrvXyPoxW31w' for step in quest['steps']}
for key in ('BreakTheThreeSeals','BreakRemainingtwoSeals','BreakTheFinalSeal'):
    assert [marker['name'] for marker in withering_fragments[key]['markers']]==[
        'Curse Fragment of Grief — Emberwood Village','Curse Fragment of Rage — Vekchenven Kara entrance',
        'Curse Fragment of Despair — Silverthorn Keep entrance']
doric={step['key']:step for quest in steps['quests'] if quest['id']=='Fv3Rt0Dn18PT1DepNzEqtQ' for step in quest['steps']}
assert [(marker['name'],marker['role']) for marker in doric['FindSkeletonShortbow']['markers']]==[
    ("Undead Ranger's Bow — Skekven Kara entrance",'enter'),('Doric — bow hand-in','return')]
assert any(marker['interiorTarget']=='Vekchenven Kara' for marker in doric['FindNecromancerStaff']['markers'])
assert any(marker.get('actorType')=='BP_SpawnPoint_Zogre' for marker in doric['FindShadowSword']['markers'])
dragon={step['key']:step for quest in steps['quests'] if quest['id']=='yrRZnkmro-IvH_OmZ3f-5A' for step in quest['steps']}
assert any(marker.get('actorType')=='BP_MiningRock_Dragon_Tooth' for marker in dragon['Craft the summon horn']['markers'])
assert any(marker['name']=='Bloodwood Sap — southern Bloodblight ruins' for marker in dragon['Craft the summon horn']['markers'])
family_rows=[row for row in overrides['relationships'] if row.get('objectiveKeys')]
assert family_rows and all(len(row['objectiveKeys'])==len(set(row['objectiveKeys'])) for row in family_rows)
assert any(target.get('kind')=='world_actor_type' for row in overrides['relationships'] for target in row.get('targets',[]))
dragon_vault=next(step for quest in steps['quests'] if quest['id']=='yrRZnkmro-IvH_OmZ3f-5A' for step in quest['steps'] if step['key']=='Locate cathan in the vault')
assert len(dragon_vault['markers'])==1
assert dragon_vault['markers'][0]['kind']=='dungeon_entrance' and dragon_vault['markers'][0]['interiorTarget']=='Vertentis Kara'
dragon_vault_analysis=next(objective for quest in analysis['quests'] if quest['id']=='yrRZnkmro-IvH_OmZ3f-5A' for objective in quest['objectives'] if objective['key']=='Locate cathan in the vault')
assert dragon_vault_analysis['locationOperations'][0]['name']=='BMSwamp'
assert dragon_vault_analysis['locationOperations'][0]['revealed'] is True
assert not any(marker['interiorTarget']=="Velgar's Rise" for marker in dragon_vault['markers'])
for quest_id,key,expected in (
    ('rai6k0UuXT_GPEixJM0boQ','BringHeartNotStarted','VannakaNPC'),
    ('Nw3wiUaB8siRIN2bStWJmw','SpeakZilyana','SW_Door'),
    ('sQ91eUA55bcQx0OQ0m59sg','Objective3','Zi'),
    ('Xb1KUkU_wOIHvzahULr6yA','Return','TribeC_Taskgiver3_NPC'),
):
    step=next(step for quest in steps['quests'] if quest['id']==quest_id for step in quest['steps'] if step['key']==key)
    assert step['available'] and any(marker['name']==expected for marker in step['markers'])
dungeon_markers=[]
for step in all_steps:
    assert step['key']
    if not step['text']: assert not step['available']
    for marker in step['markers']:
        assert 0<=marker['map']['x']<=1 and 0<=marker['map']['y']<=1 and marker['source']
        if marker['kind']=='dungeon_entrance':
            assert marker['interiorTarget'] and marker['name'].endswith('entrance')
            dungeon_markers.append(marker)
assert len(dungeon_markers)>=15
for token in ('/api/quests/steps','questStepLocations','refreshQuestStepMaps'): assert token in page
for token in ('id="questPanel"','id="questSteps"','id="questTip"','renderQuestMarkers','focusQuestMarker','dragonwilds.map.questSteps'): assert token in page
for token in ('quest-marker-nav','quest-step-cycle','quest-tip-cycle','questStepFocusIndices','cycleQuestStepMarker','Previous quest location','Next quest location'): assert token in page
assert "note.className='quest-map-note';wrap.append(note)" in page
assert "note.className='quest-meta quest-map-note';node.append(note)" not in page
audit=json.loads((root/'dist/quest-step-location-audit.json').read_text(encoding='utf-8'))
assert audit['catalogFingerprint']==catalog['fingerprint']
assert audit['summary']['quests']==len(quests) and audit['summary']['steps']==len(all_steps)
assert len(audit['steps'])==len(all_steps)
assert all(row['markers'] or row['destinationMode'] in ('dynamic','unavailable') for row in audit['steps'])
assert sum(audit['summary']['confidence'].values())==len(all_steps)
assert audit['summary']['confidence']['Low']==0
body_robbery={step['key']:step for quest in steps['quests'] if quest['id']=='db-ciknQnYrkNHuOcQXhkA' for step in quest['steps']}
assert [(marker['name'],marker['role']) for marker in body_robbery['Objective5']['markers']]==[
    ('Doctor Fenkenstrain — lightning bottle hand-in','return-after-dynamic-collection')]
assert 'no exact strike coordinate exists' in body_robbery['Objective5']['destinationNote']
for group_index,group in enumerate(audit['sharedCoordinates'],1):
    assert group['distinctStepCount']==len({use['stepId'] for use in group['uses']})>=2
    assert all(any(marker['sharedCoordinateGroup']==group_index for marker in row['markers']) for row in audit['steps'] if row['stepId'] in {use['stepId'] for use in group['uses']})
