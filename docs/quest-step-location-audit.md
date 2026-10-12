# Quest step location confidence audit

Catalog fingerprint: `5b89d5a15671b69c53aa4d7d521eea68681474d957302aedf36c87a47f72f050`

This report lists every public quest objective state and explains how each map marker was determined. Coordinates are normalized map coordinates extracted from the current cooked game build. Wiki guides may inform manual relationship review, but never supply coordinates.

- 41 quests; 230 objective states; 679 marker uses
- Confidence: 212 High, 18 Medium, 0 Low
- 105 exact shared-coordinate groups across 170 objective states

Confidence is an evidence-quality and route-precision assessment, not a claim of certainty. High means a direct/exact/reviewed current-build relationship. Medium generally means text-scoped matching, inherited counter-state evidence, an authored area, or several possible destinations. Low means the available relationship is weak enough to require further reverse engineering.

## Every quest step

### Dragon Slayer

Main quest · Brynmoor / Wise Old Man's Sanctuary

#### 1. Construct a base of operations in Bramblemead village. Build a shelter and claim a bed.

- Objective key: `Claim a bed`
- Destination mode: `area`
- Confidence: **High (91/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. The game authors an area rather than one exact interaction point.
- Display note: Build and claim a bed anywhere valid in Bramblemead village; this marker identifies the authored village objective area, not a required bed position.
- Reviewed relationship: **yes**
- Cooked location operations: `WiseOldMan` (revealed, `QF_DragonSlayer_Beat1.json` / `QFT_UpdateQuestLocation_C_2`)
- Marker 1: **BMCastle** (`area`) — map `0.17187582, 0.66053476`; world `61112.844, 159739.6, 20160.379`
  - Determination: reviewed objective relationship to extracted game quest marker 'BMCastle': The objective is completed by player construction in Bramblemead; Quest_DragonSlayer owns the BMCastle area marker.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 2. Speak to the Wise Old Man in Bramblemead village.

- Objective key: `Return to the Wise old man after claimed bed`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: `WiseOldMan` (hidden/removed, `QF_DragonSlayer_Beat1.json` / `QFT_UpdateQuestLocation_C_0`); `WiseOldMan` (hidden/removed, `QF_DragonSlayer_Beat1.json` / `QFT_UpdateQuestLocation_C_3`); `WiseOldManObjectiveDS` (revealed, `QF_DragonSlayer_Beat1.json` / `QFT_UpdateQuestLocation_C_4`); `WiseOldManObjectiveDS` (revealed, `QF_DragonSlayer_Beat1.json` / `QFT_UpdateQuestLocation_C_7`)
- Marker 1: **Wise Old Man — Bramblemead** (`location`) — map `0.1419211, 0.70314926`; world `48531.863, 177637.69, 20160.379`; shared-coordinate group [S9](#s9)
  - Determination: reviewed objective relationship to extracted game quest marker 'WiseOldManObjectiveDS': Both states explicitly return to the Wise Old Man in Bramblemead; WiseOldManObjectiveDS is the exact Dragon Slayer interaction target, while WiseOldMan is its colocated actor/area duplicate.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 3. Hunt a deer and bring one of its antlers to the Wise Old Man.

- Objective key: `Find and hunt a deer`
- Destination mode: `multiple`
- Confidence: **Medium (88/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains. The state exposes a large candidate set, reducing route precision.
- Display note: Hunt any standard deer for an antler, then return it to the Wise Old Man. All current standard-deer spawns remain visible because the objective does not require one specific animal.
- Reviewed relationship: **yes**
- Cooked location operations: `BMCastle` (hidden/removed, `QF_DragonSlayer_Beat1.json` / `QFT_UpdateQuestLocation_C_1`); `WiseOldMan` (revealed, `QF_DragonSlayer_Beat1.json` / `QFT_UpdateQuestLocation_C_5`); `WiseOldManObjectiveDS` (hidden/removed, `QF_DragonSlayer_Beat1.json` / `QFT_UpdateQuestLocation_C_8`)
- Marker 1: **Deer spawn** (`npc`) — map `0.0731059, 0.48677352`; world `19629.479, 86759.88, 7170.054`
  - Determination: reviewed objective relationship to extracted game npc 'deer': The walkthrough confirms the antler is obtained from ordinary deer in Bramblemead. Current-build deer spawns provide the resource locations and the exact quest-owned Wise Old Man target provides the hand-in.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Deer spawn** (`npc`) — map `0.23303045, 0.49228607`; world `86797.79, 89075.15, 205.083`
  - Determination: reviewed objective relationship to extracted game npc 'deer': The walkthrough confirms the antler is obtained from ordinary deer in Bramblemead. Current-build deer spawns provide the resource locations and the exact quest-owned Wise Old Man target provides the hand-in.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Deer spawn** (`npc`) — map `0.36586652, 0.48646445`; world `142588.94, 86630.07, 851.187`
  - Determination: reviewed objective relationship to extracted game npc 'deer': The walkthrough confirms the antler is obtained from ordinary deer in Bramblemead. Current-build deer spawns provide the resource locations and the exact quest-owned Wise Old Man target provides the hand-in.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **Deer spawn** (`npc`) — map `0.11710176, 0.45077933`; world `38107.74, 71642.32, 7494.713`
  - Determination: reviewed objective relationship to extracted game npc 'deer': The walkthrough confirms the antler is obtained from ordinary deer in Bramblemead. Current-build deer spawns provide the resource locations and the exact quest-owned Wise Old Man target provides the hand-in.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 5: **Deer spawn** (`npc`) — map `0.30493857, 0.5130543`; world `116999.2, 97797.805, -162.352`
  - Determination: reviewed objective relationship to extracted game npc 'deer': The walkthrough confirms the antler is obtained from ordinary deer in Bramblemead. Current-build deer spawns provide the resource locations and the exact quest-owned Wise Old Man target provides the hand-in.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 6: **Deer spawn** (`npc`) — map `0.33792658, 0.44765305`; world `130854.164, 70329.28, 1016.909`
  - Determination: reviewed objective relationship to extracted game npc 'deer': The walkthrough confirms the antler is obtained from ordinary deer in Bramblemead. Current-build deer spawns provide the resource locations and the exact quest-owned Wise Old Man target provides the hand-in.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 7: **Deer spawn** (`npc`) — map `0.19293174, 0.48802064`; world `69956.33, 87283.67, -977.95`
  - Determination: reviewed objective relationship to extracted game npc 'deer': The walkthrough confirms the antler is obtained from ordinary deer in Bramblemead. Current-build deer spawns provide the resource locations and the exact quest-owned Wise Old Man target provides the hand-in.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 8: **Deer spawn** (`npc`) — map `0.26700476, 0.57529775`; world `101067, 123940.055, 536.371`
  - Determination: reviewed objective relationship to extracted game npc 'deer': The walkthrough confirms the antler is obtained from ordinary deer in Bramblemead. Current-build deer spawns provide the resource locations and the exact quest-owned Wise Old Man target provides the hand-in.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 9: **Deer spawn** (`npc`) — map `0.10599199, 0.5459584`; world `33441.637, 111617.53, 6511.644`
  - Determination: reviewed objective relationship to extracted game npc 'deer': The walkthrough confirms the antler is obtained from ordinary deer in Bramblemead. Current-build deer spawns provide the resource locations and the exact quest-owned Wise Old Man target provides the hand-in.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 10: **Deer spawn** (`npc`) — map `0.07359225, 0.48760417`; world `19833.744, 87108.75, 7087.831`
  - Determination: reviewed objective relationship to extracted game npc 'deer': The walkthrough confirms the antler is obtained from ordinary deer in Bramblemead. Current-build deer spawns provide the resource locations and the exact quest-owned Wise Old Man target provides the hand-in.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 11: **Deer spawn** (`npc`) — map `0.10550564, 0.54512777`; world `33237.37, 111268.664, 6480.833`
  - Determination: reviewed objective relationship to extracted game npc 'deer': The walkthrough confirms the antler is obtained from ordinary deer in Bramblemead. Current-build deer spawns provide the resource locations and the exact quest-owned Wise Old Man target provides the hand-in.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 12: **Deer spawn** (`npc`) — map `0.11661541, 0.44994869`; world `37903.473, 71293.45, 7565.385`
  - Determination: reviewed objective relationship to extracted game npc 'deer': The walkthrough confirms the antler is obtained from ordinary deer in Bramblemead. Current-build deer spawns provide the resource locations and the exact quest-owned Wise Old Man target provides the hand-in.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 13: **Deer spawn** (`npc`) — map `0.23539707, 0.49326521`; world `87791.77, 89486.39, 218.996`
  - Determination: reviewed objective relationship to extracted game npc 'deer': The walkthrough confirms the antler is obtained from ordinary deer in Bramblemead. Current-build deer spawns provide the resource locations and the exact quest-owned Wise Old Man target provides the hand-in.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 14: **Deer spawn** (`npc`) — map `0.19109971, 0.48974093`; world `69186.88, 88006.19, -1083.899`
  - Determination: reviewed objective relationship to extracted game npc 'deer': The walkthrough confirms the antler is obtained from ordinary deer in Bramblemead. Current-build deer spawns provide the resource locations and the exact quest-owned Wise Old Man target provides the hand-in.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 15: **Deer spawn** (`npc`) — map `0.33566098, 0.50450897`; world `129902.61, 94208.766, 459.15`
  - Determination: reviewed objective relationship to extracted game npc 'deer': The walkthrough confirms the antler is obtained from ordinary deer in Bramblemead. Current-build deer spawns provide the resource locations and the exact quest-owned Wise Old Man target provides the hand-in.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 16: **Deer spawn** (`npc`) — map `0.33790729, 0.44913921`; world `130846.06, 70953.47, 998.278`
  - Determination: reviewed objective relationship to extracted game npc 'deer': The walkthrough confirms the antler is obtained from ordinary deer in Bramblemead. Current-build deer spawns provide the resource locations and the exact quest-owned Wise Old Man target provides the hand-in.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 17: **Deer spawn** (`npc`) — map `0.30380643, 0.51334801`; world `116523.7, 97921.164, -125.036`
  - Determination: reviewed objective relationship to extracted game npc 'deer': The walkthrough confirms the antler is obtained from ordinary deer in Bramblemead. Current-build deer spawns provide the resource locations and the exact quest-owned Wise Old Man target provides the hand-in.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 18: **Deer spawn** (`npc`) — map `0.36859333, 0.47926129`; world `143734.2, 83604.74, 1087.601`
  - Determination: reviewed objective relationship to extracted game npc 'deer': The walkthrough confirms the antler is obtained from ordinary deer in Bramblemead. Current-build deer spawns provide the resource locations and the exact quest-owned Wise Old Man target provides the hand-in.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 19: **Deer spawn** (`npc`) — map `0.31429679, 0.42652475`; world `120929.65, 61455.395, 671.061`
  - Determination: reviewed objective relationship to extracted game npc 'deer': The walkthrough confirms the antler is obtained from ordinary deer in Bramblemead. Current-build deer spawns provide the resource locations and the exact quest-owned Wise Old Man target provides the hand-in.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 20: **Deer spawn** (`npc`) — map `0.33675252, 0.50414545`; world `130361.06, 94056.09, 442.885`
  - Determination: reviewed objective relationship to extracted game npc 'deer': The walkthrough confirms the antler is obtained from ordinary deer in Bramblemead. Current-build deer spawns provide the resource locations and the exact quest-owned Wise Old Man target provides the hand-in.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 21: **Wise Old Man — antler hand-in** (`location`) — map `0.1419211, 0.70314926`; world `48531.863, 177637.69, 20160.379`; shared-coordinate group [S9](#s9)
  - Determination: reviewed objective relationship to extracted game quest marker 'WiseOldManObjectiveDS': The walkthrough confirms the antler is obtained from ordinary deer in Bramblemead. Current-build deer spawns provide the resource locations and the exact quest-owned Wise Old Man target provides the hand-in.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 4. Siege the goblin castle and look for Cathan.

- Objective key: `Investigate the castle`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: `BMCastle` (revealed, `QF_DragonSlayer_Beat2.json` / `QFT_UpdateQuestLocation_C_3`)
- Marker 1: **Cathan's Journal — Keep of Blue Flames** (`world_actor`) — map `0.17258183, 0.66351771`; world `61409.367, 160992.44, 4280.248`
  - Determination: reviewed objective relationship to extracted game world actor 'BP_LoreItem_CathanJournal_Castle_DS2': The castle objective advances by reading Cathan's authored Castle journal. The current world contains one exact BP_LoreItem_CathanJournal_Castle_DS2 interaction inside the broad BMCastle quest area.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/0XA776XVNVKKX0CRW24QM5MM1.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 5. Investigate the blue-flame door in the swamp. Find out what Cathan is up to.

- Objective key: `Locate cathan in the vault`
- Destination mode: `fixed`
- Confidence: **High (97/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Display note: The objective continues inside Vertentis Kara; the world-map marker shows its Whispering Swamp entrance.
- Reviewed relationship: **yes**
- Cooked location operations: `BMSwamp` (revealed, `QF_DragonSlayer_Beat2.json` / `QFT_UpdateQuestLocation_C_4`)
- Marker 1: **Vertentis Kara entrance** (`dungeon_entrance`) — map `0.2710176, 0.62499512`; world `102752.39, 144812.95, -3763.527`
  - Determination: reviewed objective relationship to extracted game dungeon entrance 'VertentisKara': The Beat 2 objective is directly preceded by the authored BMSwamp quest-location reveal, and the extracted DungeonEntrance_VertentisKara actor is the blue-flame vault entrance inside that authored area.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 97/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 6. Speak to the Wise Old Man in Bramblemead village.

- Objective key: `Return to the Wise old man`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: `WiseOldMan` (hidden/removed, `QF_DragonSlayer_Beat2.json` / `QFT_UpdateQuestLocation_C_10`); `WiseOldManObjectiveDS` (revealed, `QF_DragonSlayer_Beat2.json` / `QFT_UpdateQuestLocation_C_18`)
- Marker 1: **Wise Old Man — Bramblemead** (`location`) — map `0.1419211, 0.70314926`; world `48531.863, 177637.69, 20160.379`; shared-coordinate group [S9](#s9)
  - Determination: reviewed objective relationship to extracted game quest marker 'WiseOldManObjectiveDS': Both states explicitly return to the Wise Old Man in Bramblemead; WiseOldManObjectiveDS is the exact Dragon Slayer interaction target, while WiseOldMan is its colocated actor/area duplicate.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 7. Gear yourself up and push forward into Ghornfell in search of Cathan.

- Objective key: `Venture into Ghornfell`
- Destination mode: `area`
- Confidence: **High (91/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. The game authors an area rather than one exact interaction point.
- Display note: Cross into the authored Ghornfell search area; the earlier Wise Old Man marker is no longer an active destination.
- Reviewed relationship: **yes**
- Cooked location operations: `WiseOldMan` (revealed, `QF_DragonSlayer_Beat2.json` / `QFT_UpdateQuestLocation_C_0`); `WiseOldManObjectiveDS` (hidden/removed, `QF_DragonSlayer_Beat2.json` / `QFT_UpdateQuestLocation_C_1`); `GhornfellArea` (revealed, `QF_DragonSlayer_Beat2.json` / `QFT_UpdateQuestLocation_C_21`)
- Marker 1: **Ghornfell approach** (`area`) — map `0.21914457, 0.53694864`; world `80965.72, 107833.43, 8960.0`
  - Determination: reviewed objective relationship to extracted game quest marker 'GhornfellArea': The compiled objective reveals GhornfellArea after the Bramblemead conversation. WiseOldMan is retained in the flow only as the preceding phase's marker and is explicitly hidden during this transition.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 8. Investigate Velgar's tower in search of Cathan.

- Objective key: `Look for evidence of Cathan at the tower`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: Read Cathan's Ghornfell journal at Velgar's Rise and follow the local Cathan evidence. Both interactions belong to the same tower approach rather than the later boss arena.
- Reviewed relationship: **yes**
- Cooked location operations: `GhornfellArea` (hidden/removed, `QF_DragonSlayer_Beat3.json` / `QFT_UpdateQuestLocation_C_2`); `DragonsLairCathanArea` (revealed, `QF_DragonSlayer_Beat3.json` / `QFT_UpdateQuestLocation_C_5`)
- Marker 1: **Cathan's Journal — Velgar's Rise** (`world_actor`) — map `0.21252243, 0.60001145`; world `78184.42, 134319.81, -1343.6356`
  - Determination: reviewed objective relationship to extracted game world actor 'BP_LoreItemDS5': The current world contains the unique BP_LoreItemDS5 actor bound to JOURNAL_Know_CathanJournal_Velgarslair inside the authored DragonsLairCathanArea. The nearby Dragon Slayer Cathan placement is independently distinguished from the Dowdun-gate and Fellhollow Cathan copies.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/2WT8WE21YKCZECW0NQ9UL9OC1.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Cathan — Velgar's Rise** (`npc`) — map `0.26253019, 0.53228543`; world `99187.68, 105874.88, -358.524`
  - Determination: reviewed objective relationship to extracted game npc 'ghost-of-cathan' placement 'cfbfee9192c1bd79b266': The current world contains the unique BP_LoreItemDS5 actor bound to JOURNAL_Know_CathanJournal_Velgarslair inside the authored DragonsLairCathanArea. The nearby Dragon Slayer Cathan placement is independently distinguished from the Dowdun-gate and Fellhollow Cathan copies.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 9. Find the ingredients and craft the Challenge Horn. Check the recipe in your journal for more information.

- Objective key: `Craft the summon horn`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: Mine a Dragon Tooth in the Stormtouched Highlands, chop Bloodwood Sap in the Bloodblight Swamp, supply five Bronze Bars, and craft the Challenge Horn at a Blacksmith's Bench. Ordinary bronze and player-built bench sources are dynamic.
- Reviewed relationship: **yes**
- Cooked location operations: `BloodblightRuin` (hidden/removed, `QF_DragonSlayer_Beat3.json` / `QFT_UpdateQuestLocation_C_0`); `HighlandsArea` (hidden/removed, `QF_DragonSlayer_Beat3.json` / `QFT_UpdateQuestLocation_C_1`); `HighlandsArea` (revealed, `QF_DragonSlayer_Beat3.json` / `QFT_UpdateQuestLocation_C_6`); `DragonsLairCathanArea` (hidden/removed, `QF_DragonSlayer_Beat3.json` / `QFT_UpdateQuestLocation_C_7`); `BloodblightRuin` (revealed, `QF_DragonSlayer_Beat3.json` / `QFT_UpdateQuestLocation_C_8`)
- Marker 1: **Dragon Tooth deposit** (`world_actor`) — map `0.0786296, 0.50863629`; world `21949.434, 95942.24, 7694.165`
  - Determination: reviewed objective relationship to extracted game world actor type 'BP_MiningRock_Dragon_Tooth': Current-build world actors provide every fixed Dragon Tooth and Bloodwood Sap resource placement. The quest-authored HighlandsArea and BloodblightRuin candidates agree with the walkthrough's dragon-skull and southern-castle locations; Velgar's Rise is a later use destination and is excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/DPZ8T2XAY4KVII15X3G77PFW1.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Dragon Tooth deposit** (`world_actor`) — map `0.07903691, 0.50865234`; world `22120.502, 95948.984, 7739.1475`
  - Determination: reviewed objective relationship to extracted game world actor type 'BP_MiningRock_Dragon_Tooth': Current-build world actors provide every fixed Dragon Tooth and Bloodwood Sap resource placement. The quest-authored HighlandsArea and BloodblightRuin candidates agree with the walkthrough's dragon-skull and southern-castle locations; Velgar's Rise is a later use destination and is excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/DPZ8T2XAY4KVII15X3G77PFW1.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Dragon Tooth deposit** (`world_actor`) — map `0.07967442, 0.50865242`; world `22388.256, 95949.016, 7842.0347`
  - Determination: reviewed objective relationship to extracted game world actor type 'BP_MiningRock_Dragon_Tooth': Current-build world actors provide every fixed Dragon Tooth and Bloodwood Sap resource placement. The quest-authored HighlandsArea and BloodblightRuin candidates agree with the walkthrough's dragon-skull and southern-castle locations; Velgar's Rise is a later use destination and is excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/DPZ8T2XAY4KVII15X3G77PFW1.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **Dragon Tooth deposit** (`world_actor`) — map `0.08006774, 0.50884543`; world `22553.451, 96030.08, 7850.0776`
  - Determination: reviewed objective relationship to extracted game world actor type 'BP_MiningRock_Dragon_Tooth': Current-build world actors provide every fixed Dragon Tooth and Bloodwood Sap resource placement. The quest-authored HighlandsArea and BloodblightRuin candidates agree with the walkthrough's dragon-skull and southern-castle locations; Velgar's Rise is a later use destination and is excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/DPZ8T2XAY4KVII15X3G77PFW1.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 5: **Dragon Tooth deposit** (`world_actor`) — map `0.08044668, 0.50913633`; world `22712.604, 96152.26, 7812.5474`
  - Determination: reviewed objective relationship to extracted game world actor type 'BP_MiningRock_Dragon_Tooth': Current-build world actors provide every fixed Dragon Tooth and Bloodwood Sap resource placement. The quest-authored HighlandsArea and BloodblightRuin candidates agree with the walkthrough's dragon-skull and southern-castle locations; Velgar's Rise is a later use destination and is excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/DPZ8T2XAY4KVII15X3G77PFW1.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 6: **Dragon Tooth deposit** (`world_actor`) — map `0.0793443, 0.50862252`; world `22249.607, 95936.46, 7800.6235`
  - Determination: reviewed objective relationship to extracted game world actor type 'BP_MiningRock_Dragon_Tooth': Current-build world actors provide every fixed Dragon Tooth and Bloodwood Sap resource placement. The quest-authored HighlandsArea and BloodblightRuin candidates agree with the walkthrough's dragon-skull and southern-castle locations; Velgar's Rise is a later use destination and is excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/DPZ8T2XAY4KVII15X3G77PFW1.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 7: **Bloodwood Sap — southern Bloodblight ruins** (`area`) — map `0.31438014, 0.6708684`; world `120964.66, 164079.73, 8960.0`
  - Determination: reviewed objective relationship to extracted game quest marker 'BloodblightRuin': Current-build world actors provide every fixed Dragon Tooth and Bloodwood Sap resource placement. The quest-authored HighlandsArea and BloodblightRuin candidates agree with the walkthrough's dragon-skull and southern-castle locations; Velgar's Rise is a later use destination and is excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 10. Summon and defeat Velgar.

- Objective key: `Challenge Velgar`
- Destination mode: `fixed`
- Confidence: **High (97/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Display note: Velgar is challenged inside the tower encounter; the overworld map marks the entrance to Velgar's Rise.
- Reviewed relationship: **yes**
- Cooked location operations: `HighlandsArea` (hidden/removed, `QF_DragonSlayer_Beat4.json` / `QFT_UpdateQuestLocation_C_4`); `DragonsLairChallengeVelgar` (revealed, `QF_DragonSlayer_Beat4.json` / `QFT_UpdateQuestLocation_C_9`)
- Marker 1: **Velgar's Rise entrance** (`dungeon_entrance`) — map `0.26167576, 0.51888457`; world `98828.82, 100246.52, -302.75412`
  - Determination: reviewed objective relationship to extracted game dungeon entrance 'DragonsLair': The objective's DragonsLairChallengeVelgar target is an interior encounter location. The extracted DungeonEntrance_DragonsLair actor is the corresponding overworld entrance and avoids publishing an interior transform.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/6WRF7EVOUH1XK9E22EO1HQEB3.json`
  - Marker confidence: 97/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 11. Bring Velgar's head to the Wise Old Man outside Velgar's Lair.

- Objective key: `Return to the Wise old man after killing velgar`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: `WiseOldManObjective_Lair` (revealed, `QF_DragonSlayer_Beat5.json` / `QFT_UpdateQuestLocation_C_5`)
- Marker 1: **Wise Old Man — outside Velgar's Lair** (`location`) — map `0.26277381, 0.53260714`; world `99290.0, 106010.0, 20160.379`
  - Determination: reviewed objective relationship to extracted game quest marker 'WiseOldManObjective_Lair': The objective explicitly places the Wise Old Man outside Velgar's Lair and the compiled flow directly links WiseOldManObjective_Lair to this state.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

### Goblin Diplomacy

Side quest · Brynmoor / Eastern goblin camp

#### 1. Investigate the ruins to the northeast and the guild to the south of Zanik.

- Objective key: `Objective1`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: Both valid first-investigation destinations are shown; the player may visit them in either order.
- Reviewed relationship: **yes**
- Cooked location operations: `ZanikGoblinDeplomacy` (revealed, `QF_GoblinDiplomacy.json` / `QFT_UpdateQuestLocation_C_5`)
- Marker 1: **Outlook Ruins reliquary** (`world_actor`) — map `0.21939348, 0.64779114`; world `81070.26, 154387.28, 1750.3807`; shared-coordinate group [S14](#s14)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_NPC_ReliquaryCastle': The objective names the ruins northeast of Zanik and the guild to her south. The compiled quest branches enter ReliquaryCliffRuins and ReliquaryGuild, whose exact current-build placements are BP_NPC_ReliquaryCastle and BP_NPC_ReliquaryGuild. The temporarily revealed Zanik marker identifies the quest giver, not either requested destination.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/1NOO68QCN6J5Y6H314339OJOB.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Adventurers' Guild reliquary** (`world_actor`) — map `0.2182855, 0.69625933`; world `80604.91, 174743.92, 209.09344`; shared-coordinate group [S13](#s13)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_NPC_ReliquaryGuild': The objective names the ruins northeast of Zanik and the guild to her south. The compiled quest branches enter ReliquaryCliffRuins and ReliquaryGuild, whose exact current-build placements are BP_NPC_ReliquaryCastle and BP_NPC_ReliquaryGuild. The temporarily revealed Zanik marker identifies the quest giver, not either requested destination.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/1NOO68QCN6J5Y6H314339OJOB.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 2. Investigate the ruin northeast of Zanik.

- Objective key: `Objective2a`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Outlook Ruins reliquary** (`world_actor`) — map `0.21939348, 0.64779114`; world `81070.26, 154387.28, 1750.3807`; shared-coordinate group [S14](#s14)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_NPC_ReliquaryCastle': The cliff-ruins branch enters QuestFlow.Entry.GoblinDiplomacy.ReliquaryCliffRuins; the placed quest actor is BP_NPC_ReliquaryCastle.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/1NOO68QCN6J5Y6H314339OJOB.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 3. Investigate the guild south of Zanik.

- Objective key: `Objective2b`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Adventurers' Guild reliquary** (`world_actor`) — map `0.2182855, 0.69625933`; world `80604.91, 174743.92, 209.09344`; shared-coordinate group [S13](#s13)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_NPC_ReliquaryGuild': The guild branch enters QuestFlow.Entry.GoblinDiplomacy.ReliquaryGuild; the placed quest actor is BP_NPC_ReliquaryGuild.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/1NOO68QCN6J5Y6H314339OJOB.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 4. Search for the other amulet piece in an area filled with mud and stagnant swampwater.

- Objective key: `Objective3`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **BP_SpawnPoint_MagicBeast_Quest_GD** (`world_actor`) — map `0.2103115, 0.62998679`; world `77255.83, 146909.45, -2955.9407`
  - Determination: reviewed objective relationship to extracted game world actor 'BP_SpawnPoint_MagicBeast_Quest_GD': After finding the emptied reliquary, the amulet is recovered from the quest-only corrupted beast at the authored Goblin Diplomacy druid encounter.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/2WT8WE21YKCZECW0NQ9UL9OC1.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 5. Bring both amulets to Zanik.

- Objective key: `Objective4`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **ZanikGoblinDeplomacy** (`npc`) — map `0.20321014, 0.6563285`; world `74273.26, 157972.97, 20160.379`; shared-coordinate group [S12](#s12)
  - Determination: reviewed objective relationship to extracted game quest marker 'ZanikGoblinDeplomacy': The objective explicitly returns both amulets to Zanik; this quest owns the ZanikGoblinDeplomacy marker.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 6. Look for the other amulet at either the ruins northeast of Zanik, or the guild to the south.

- Objective key: `Objective5`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: Either reliquary can be investigated first; both authored destinations are shown.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Outlook Ruins reliquary** (`world_actor`) — map `0.21939348, 0.64779114`; world `81070.26, 154387.28, 1750.3807`; shared-coordinate group [S14](#s14)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_NPC_ReliquaryCastle': This objective explicitly permits either the cliff ruins or guild branch, whose exact placed quest actors are the two reliquaries.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/1NOO68QCN6J5Y6H314339OJOB.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Adventurers' Guild reliquary** (`world_actor`) — map `0.2182855, 0.69625933`; world `80604.91, 174743.92, 209.09344`; shared-coordinate group [S13](#s13)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_NPC_ReliquaryGuild': This objective explicitly permits either the cliff ruins or guild branch, whose exact placed quest actors are the two reliquaries.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/1NOO68QCN6J5Y6H314339OJOB.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 7. Find someone that can identify the Bandosian amulet.

- Objective key: `Objective6`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: `ZanikGoblinDeplomacy` (revealed, `QF_GoblinDiplomacy.json` / `QFT_UpdateQuestLocation_C_2`); `ZanikGoblinDeplomacy` (revealed, `QF_GoblinDiplomacy.json` / `QFT_UpdateQuestLocation_C_4`)
- Marker 1: **Zanik (amulet identification)** (`npc`) — map `0.20321014, 0.6563285`; world `74273.26, 157972.97, 20160.379`; shared-coordinate group [S12](#s12)
  - Determination: reviewed objective relationship to extracted game quest marker 'ZanikGoblinDeplomacy': The walkthrough resolves the unidentified Bandosian amulet by returning to Zanik, and the compiled flow reveals this quest's ZanikGoblinDeplomacy location for the state. This is an exact quest-owned marker, not a name-based NPC inference.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

### Highlighting the Problem

Side quest · Brynmoor

#### 1. Investigate the area for clues about the statue.

- Objective key: `Investigate for clues about the statue`
- Destination mode: `fixed`
- Confidence: **High (97/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Display note: The password statue is inside Thishepen Kara; per the dungeon-marker policy, the public world map points to the vault entrance.
- Reviewed relationship: **yes**
- Cooked location operations: `MagicGuild_HtP` (revealed, `QF_HighlightingTheProblem.json` / `QFT_UpdateQuestLocation_C_1`)
- Marker 1: **Thishepen Kara password-statue entrance** (`dungeon_entrance`) — map `0.09024376, 0.63956152`; world `26827.38, 150930.84, -455.4454`
  - Determination: reviewed objective relationship to extracted game dungeon entrance 'ThishepenKara': Multiple walkthroughs identify the required clue as the password Pthentorok, learned from the blue statue inside Thishepen Kara. The current build contains the exact DungeonEntrance_ThishepenKara placement; the broad MagicGuild_HtP area does not represent this interior destination.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 97/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 2. Search the Runecrafting Guild for a statue.

- Objective key: `Search the runecrafting guild for wizard statue`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: `MagicGuild_HtP` (revealed, `QF_HighlightingTheProblem.json` / `QFT_UpdateQuestLocation_C_2`)
- Marker 1: **Runecrafting Guild wizard statue** (`world_actor`) — map `0.07216621, 0.63033771`; world `19234.809, 147056.84, 3456.3066`; shared-coordinate group [S2](#s2)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_NPC_HighLightingTheProblem_Statue': After learning Pthentorok in Thishepen Kara, the player must enter it at the central wizard statue outside the Runecrafting Guild. The current world contains one quest-specific BP_NPC_HighLightingTheProblem_Statue placement.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/8D7AJV1MRZXIGQU4PDFKUTQRT.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 3. Find a way to charge the  mysterious orb.

- Objective key: `Charge the orb`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: All three tower pedestals must be activated; the markers represent the exact interaction actors rather than the shared guild-area center.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Mysterious Orb charging pedestal** (`world_actor`) — map `0.07141488, 0.6352039`; world `18919.25, 149100.64, 4579.4277`
  - Determination: reviewed objective relationship to extracted game world actor type 'BP_NPC_HighLightingTheProbem_Pedestal': The walkthrough and compiled quest flow require three pedestal interactions around the guild. The current world has exactly three placements of the quest-specific BP_NPC_HighLightingTheProbem_Pedestal actor, so each actionable tower can be mapped independently.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/8D7AJV1MRZXIGQU4PDFKUTQRT.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Mysterious Orb charging pedestal** (`world_actor`) — map `0.0738261, 0.62620257`; world `19931.96, 145320.08, 4579.4277`
  - Determination: reviewed objective relationship to extracted game world actor type 'BP_NPC_HighLightingTheProbem_Pedestal': The walkthrough and compiled quest flow require three pedestal interactions around the guild. The current world has exactly three placements of the quest-specific BP_NPC_HighLightingTheProbem_Pedestal actor, so each actionable tower can be mapped independently.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/8D7AJV1MRZXIGQU4PDFKUTQRT.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Mysterious Orb charging pedestal** (`world_actor`) — map `0.08283744, 0.62859207`; world `23716.723, 146323.67, 4579.4277`
  - Determination: reviewed objective relationship to extracted game world actor type 'BP_NPC_HighLightingTheProbem_Pedestal': The walkthrough and compiled quest flow require three pedestal interactions around the guild. The current world has exactly three placements of the quest-specific BP_NPC_HighLightingTheProbem_Pedestal actor, so each actionable tower can be mapped independently.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/8D7AJV1MRZXIGQU4PDFKUTQRT.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 4. Return to the wizard statue.

- Objective key: `Return to statue`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Runecrafting Guild wizard statue (charged-orb return)** (`world_actor`) — map `0.07216621, 0.63033771`; world `19234.809, 147056.84, 3456.3066`; shared-coordinate group [S2](#s2)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_NPC_HighLightingTheProblem_Statue': The charged orb is returned to the same central wizard statue that issued it. The exact current-build quest actor is BP_NPC_HighLightingTheProblem_Statue, which is more precise than the broad MagicGuild_HtP area marker.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/8D7AJV1MRZXIGQU4PDFKUTQRT.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

### Restless Ghosts

Side quest · Brynmoor / Ghostly well

#### 1. Search for a purpose for the Amulet of Ghostspeak.

- Objective key: `Found ghost amulet`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Display note: Warren is the first spirit beside the ruined village, southwest of the well; he appears only at night.
- Reviewed relationship: **yes**
- Cooked location operations: `Well_RG` (revealed, `QF_RestlessGhosts.json` / `QFT_UpdateQuestLocation_C_1`)
- Marker 1: **Warren** (`location`) — map `0.11618128, 0.6959686`; world `37721.137, 174621.81, -4140.7485`; shared-coordinate group [S5](#s5)
  - Determination: reviewed objective relationship to extracted game registry landmark '689EC4DF-4A18B80D-220E4EB3-ED4ED5AE': The guide sequence identifies Warren as the interaction that establishes the amulet's purpose. The extracted Restless Spirit registry placement southwest of the authored Well_RG marker is Warren's placed quest actor.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 2. Find the restless spirits and help lay them to rest. They only appear at night. 

- Objective key: `Put ghosts to rest`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: Speak to Warren, Lazina, Finn, and Kay at night, then remember all four at the Remembrance Shrine. The objective key does not expose which names the character has already learned, so every still-possible interaction is shown.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Warren** (`location`) — map `0.11618128, 0.6959686`; world `37721.137, 174621.81, -4140.7485`; shared-coordinate group [S5](#s5)
  - Determination: reviewed objective relationship to extracted game registry landmark '689EC4DF-4A18B80D-220E4EB3-ED4ED5AE': The quest flow contains four named remembrance choices and uses QuestFlow.Participant.Memorial_RG for the final interactions. Extracted world data contains exactly four BP_NPC_RestlessGhost_Quest placements and one BP_NPC_RestlessGhostMemorial placement, matching the guide's four spirits plus final shrine sequence.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Lazina** (`location`) — map `0.07187008, 0.69324252`; world `19110.434, 173476.86, -1134.6252`
  - Determination: reviewed objective relationship to extracted game registry landmark '9734735E-46C37F16-BDD72EB5-983FAEF2': The quest flow contains four named remembrance choices and uses QuestFlow.Participant.Memorial_RG for the final interactions. Extracted world data contains exactly four BP_NPC_RestlessGhost_Quest placements and one BP_NPC_RestlessGhostMemorial placement, matching the guide's four spirits plus final shrine sequence.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Finn** (`location`) — map `0.17867055, 0.72491681`; world `63966.63, 186780.06, -3476.7625`
  - Determination: reviewed objective relationship to extracted game registry landmark '07D13459-4DCF3A28-257A81BF-FD5DF0A7': The quest flow contains four named remembrance choices and uses QuestFlow.Participant.Memorial_RG for the final interactions. Extracted world data contains exactly four BP_NPC_RestlessGhost_Quest placements and one BP_NPC_RestlessGhostMemorial placement, matching the guide's four spirits plus final shrine sequence.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **Kay** (`location`) — map `0.15491171, 0.62916521`; world `53987.918, 146564.39, -1143.6705`
  - Determination: reviewed objective relationship to extracted game registry landmark '6F4B9D60-494C735B-61CD6695-47C1B657': The quest flow contains four named remembrance choices and uses QuestFlow.Participant.Memorial_RG for the final interactions. Extracted world data contains exactly four BP_NPC_RestlessGhost_Quest placements and one BP_NPC_RestlessGhostMemorial placement, matching the guide's four spirits plus final shrine sequence.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 5: **Remembrance Shrine** (`world_actor`) — map `0.03226708, 0.7277684`; world `2477.1724, 187977.73, -3039.709`
  - Determination: reviewed objective relationship to extracted game world actor 'BP_NPC_RestlessGhostMemorial': The quest flow contains four named remembrance choices and uses QuestFlow.Participant.Memorial_RG for the final interactions. Extracted world data contains exactly four BP_NPC_RestlessGhost_Quest placements and one BP_NPC_RestlessGhostMemorial placement, matching the guide's four spirits plus final shrine sequence.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/5DE2Q7722I32URAIIDX90NY5T.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

### Scarecrow Scavenging

Side quest · Brynmoor

### The Great Body Robbery

Side quest · Brynmoor

#### 1. Dig up body parts from graves around the Bloodblight Swamps. 0/3

- Objective key: `BodyParts.0`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: Dig up the Human torso, Garou Arms, and Goblin Legs at the three quest-specific buried coffins. Collection order is unrestricted, so all three remain visible.
- Reviewed relationship: **yes**
- Cooked location operations: `Fenkenstrain` (revealed, `QF_GreatBodyRobbery.json` / `QFT_UpdateQuestLocation_C_0`)
- Marker 1: **Human torso grave** (`world_actor`) — map `0.33492895, 0.62215262`; world `129595.16, 143619.1, -2026.2`; shared-coordinate group [S22](#s22)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_TheGreatBodyRobbery_BuriedCoffin3': Three placed BP_TheGreatBodyRobbery_BuriedCoffin actors belong to Quest_GreatBodyRobbery. Their Coffin01/02/03 entry tags and booleans lead respectively to the torso, arms, and legs grant branches in the cooked quest flow.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/2EZDVI8Q8B7A19UOIUYL7QVFF.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Garou Arms grave** (`world_actor`) — map `0.32324324, 0.60940838`; world `124687.16, 138266.52, -1049.139`; shared-coordinate group [S16](#s16)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_TheGreatBodyRobbery_BuriedCoffin2': Three placed BP_TheGreatBodyRobbery_BuriedCoffin actors belong to Quest_GreatBodyRobbery. Their Coffin01/02/03 entry tags and booleans lead respectively to the torso, arms, and legs grant branches in the cooked quest flow.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/9QL50M61Q5E51FDLOM5UBYZII.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Goblin Legs grave** (`world_actor`) — map `0.3385789, 0.61351933`; world `131128.14, 139993.12, -1506.1326`; shared-coordinate group [S24](#s24)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_TheGreatBodyRobbery_BuriedCoffin': Three placed BP_TheGreatBodyRobbery_BuriedCoffin actors belong to Quest_GreatBodyRobbery. Their Coffin01/02/03 entry tags and booleans lead respectively to the torso, arms, and legs grant branches in the cooked quest flow.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/2EZDVI8Q8B7A19UOIUYL7QVFF.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 2. Speak to Fenkenstrain

- Objective key: `Objective2`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Doctor Fenkenstrain** (`npc`) — map `0.32433942, 0.65421714`; world `125147.555, 157086.2, 964.1517`; shared-coordinate group [S18](#s18)
  - Determination: reviewed objective relationship to extracted game quest marker 'Fenkenstrain': Each state explicitly talks to or returns ingredients to Doctor Fenkenstrain; the quest owns one stable Fenkenstrain location marker while his staged actor forms share that laboratory destination.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 3. Speak to Fenkenstrain

- Objective key: `Objective3`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Doctor Fenkenstrain** (`npc`) — map `0.32433942, 0.65421714`; world `125147.555, 157086.2, 964.1517`; shared-coordinate group [S18](#s18)
  - Determination: reviewed objective relationship to extracted game quest marker 'Fenkenstrain': Each state explicitly talks to or returns ingredients to Doctor Fenkenstrain; the quest owns one stable Fenkenstrain location marker while his staged actor forms share that laboratory destination.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 4. Bring Dr Fenkenstrain: 1 Weak Antipoison, 1 Weak Focused Attack Potion and 1 Weak Lumberjack Potion.

- Objective key: `Objective4`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Doctor Fenkenstrain** (`npc`) — map `0.32433942, 0.65421714`; world `125147.555, 157086.2, 964.1517`; shared-coordinate group [S18](#s18)
  - Determination: reviewed objective relationship to extracted game quest marker 'Fenkenstrain': Each state explicitly talks to or returns ingredients to Doctor Fenkenstrain; the quest owns one stable Fenkenstrain location marker while his staged actor forms share that laboratory destination.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 5. Collect lightning in a bottle and bring it to Dr Fenkenstrain.

- Objective key: `Objective5`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: Carry the empty vial and get struck by lightning. Stormtouched Highlands is the reliable destination, while a thunderstorm can work in nearly any region; no exact strike coordinate exists. Once filled, bring the bottle to Doctor Fenkenstrain at the marked laboratory.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Doctor Fenkenstrain — lightning bottle hand-in** (`npc`) — map `0.32433942, 0.65421714`; world `125147.555, 157086.2, 964.1517`; shared-coordinate group [S18](#s18)
  - Determination: reviewed objective relationship to extracted game quest marker 'Fenkenstrain': The cooked quest requires the empty vial to become Lightning-in-a-bottle after a lightning strike, so no fixed collection point can truthfully be published. The same objective explicitly includes the hand-in, and Fenkenstrain is the quest-owned exact laboratory marker used by its surrounding conversation states.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 6. Perform an autopsy of the body on the examination table in Doctor Fenkenstrain's tower. 

- Objective key: `Objecrive3.5`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Examination Table** (`location`) — map `0.32377924, 0.65373062`; world `124912.28, 156881.86, 910.1021`; shared-coordinate group [S17](#s17)
  - Determination: reviewed objective relationship to extracted game registry landmark 'B583AF8E-4A86E359-FA1FDF88-8EA67879': The autopsy is performed on the exact quest-authored Examination Table registry identity, not on one of the several Fenkenstrain stage actors nearby.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 7. Speak to Fenkenstrain

- Objective key: `Objective6`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Doctor Fenkenstrain** (`npc`) — map `0.32433942, 0.65421714`; world `125147.555, 157086.2, 964.1517`; shared-coordinate group [S18](#s18)
  - Determination: reviewed objective relationship to extracted game quest marker 'Fenkenstrain': Each state explicitly talks to or returns ingredients to Doctor Fenkenstrain; the quest owns one stable Fenkenstrain location marker while his staged actor forms share that laboratory destination.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 8. Put Doctor Fenkenstrain's head on his new body on the examination table. 

- Objective key: `Objective7`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Examination Table** (`location`) — map `0.32377924, 0.65373062`; world `124912.28, 156881.86, 910.1021`; shared-coordinate group [S17](#s17)
  - Determination: reviewed objective relationship to extracted game registry landmark 'B583AF8E-4A86E359-FA1FDF88-8EA67879': The head is attached at the exact quest-authored Examination Table registry identity.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 9. Put the Lightning-in-a-bottle into the machine in Doctor Fenkenstrain's lab. 

- Objective key: `Objective8`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Strange Machine** (`location`) — map `0.3231366, 0.6536969`; world `124642.37, 156867.7, 981.2823`
  - Determination: reviewed objective relationship to extracted game registry landmark 'AFA8DDF4-402C43D9-030F73B9-872E4367': The bottled lightning is inserted into the quest-authored Strange Machine registry identity in Fenkenstrain's laboratory.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 10. Speak to Fenkenstrain

- Objective key: `Objective9`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Doctor Fenkenstrain** (`npc`) — map `0.32433942, 0.65421714`; world `125147.555, 157086.2, 964.1517`; shared-coordinate group [S18](#s18)
  - Determination: reviewed objective relationship to extracted game quest marker 'Fenkenstrain': Each state explicitly talks to or returns ingredients to Doctor Fenkenstrain; the quest owns one stable Fenkenstrain location marker while his staged actor forms share that laboratory destination.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 11. Dig up body parts from graves around the Bloodblight Swamps. 1/3

- Objective key: `BodyParts.1`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: One part has been collected, but the aggregate counter does not identify which one. All three authored graves remain possible destinations.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Human torso grave** (`world_actor`) — map `0.33492895, 0.62215262`; world `129595.16, 143619.1, -2026.2`; shared-coordinate group [S22](#s22)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_TheGreatBodyRobbery_BuriedCoffin3': The objective exposes only a count. The same three Coffin01/02/03 actors remain the complete game-authored target set and the current snapshot does not expose their individual booleans.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/2EZDVI8Q8B7A19UOIUYL7QVFF.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Garou Arms grave** (`world_actor`) — map `0.32324324, 0.60940838`; world `124687.16, 138266.52, -1049.139`; shared-coordinate group [S16](#s16)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_TheGreatBodyRobbery_BuriedCoffin2': The objective exposes only a count. The same three Coffin01/02/03 actors remain the complete game-authored target set and the current snapshot does not expose their individual booleans.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/9QL50M61Q5E51FDLOM5UBYZII.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Goblin Legs grave** (`world_actor`) — map `0.3385789, 0.61351933`; world `131128.14, 139993.12, -1506.1326`; shared-coordinate group [S24](#s24)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_TheGreatBodyRobbery_BuriedCoffin': The objective exposes only a count. The same three Coffin01/02/03 actors remain the complete game-authored target set and the current snapshot does not expose their individual booleans.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/2EZDVI8Q8B7A19UOIUYL7QVFF.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 12. Dig up body parts from graves around the Bloodblight Swamps. 2/3

- Objective key: `BodyParts.2`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: Two parts have been collected, but the aggregate counter does not identify the remaining grave. All three authored graves remain possible destinations.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Human torso grave** (`world_actor`) — map `0.33492895, 0.62215262`; world `129595.16, 143619.1, -2026.2`; shared-coordinate group [S22](#s22)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_TheGreatBodyRobbery_BuriedCoffin3': The objective exposes only a count. The same three Coffin01/02/03 actors remain the complete game-authored target set and the current snapshot does not expose their individual booleans.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/2EZDVI8Q8B7A19UOIUYL7QVFF.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Garou Arms grave** (`world_actor`) — map `0.32324324, 0.60940838`; world `124687.16, 138266.52, -1049.139`; shared-coordinate group [S16](#s16)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_TheGreatBodyRobbery_BuriedCoffin2': The objective exposes only a count. The same three Coffin01/02/03 actors remain the complete game-authored target set and the current snapshot does not expose their individual booleans.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/9QL50M61Q5E51FDLOM5UBYZII.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Goblin Legs grave** (`world_actor`) — map `0.3385789, 0.61351933`; world `131128.14, 139993.12, -1506.1326`; shared-coordinate group [S24](#s24)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_TheGreatBodyRobbery_BuriedCoffin': The objective exposes only a count. The same three Coffin01/02/03 actors remain the complete game-authored target set and the current snapshot does not expose their individual booleans.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/2EZDVI8Q8B7A19UOIUYL7QVFF.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 13. Place the body parts on the examination table in Fenkenstrain's tower.

- Objective key: `AddPartsToTable`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Examination Table** (`location`) — map `0.32377924, 0.65373062`; world `124912.28, 156881.86, 910.1021`; shared-coordinate group [S17](#s17)
  - Determination: reviewed objective relationship to extracted game registry landmark 'B583AF8E-4A86E359-FA1FDF88-8EA67879': The current-build registry identity B583AF8E-4A86E359-FA1FDF88-8EA67879 is the quest-authored Examination Table in Fenkenstrain's laboratory.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

### Black Knight's Fortress

Main quest · Dowdun Reach

#### 1. Speak to Cathan at the eastern gates in Fellhollow

- Objective key: `DR_Main_Objective_0`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: `DR_MQ_Cathan` (revealed, `QF_Death_PostQuestDialogue.json` / `QFT_UpdateQuestLocation_C_0`); `DR_MQ_Death` (hidden/removed, `QF_Death_PostQuestDialogue.json` / `QFT_UpdateQuestLocation_C_2`); `DR_MQ_Cathan` (revealed, `QF_DR_MainQuest_Entry.json` / `QFT_UpdateQuestLocation_C_1`)
- Marker 1: **Cathan — Silverthorn Keep eastern gate** (`location`) — map `0.52816138, 0.27321513`; world `210752.78, -2934.6438, -5213.449`
  - Determination: reviewed objective relationship to extracted game quest marker 'DR_MQ_Cathan': After Death, the walkthrough sends the player to Cathan at Silverthorn Keep's eastern gatehouse. The compiled flow repeatedly names the single quest-owned DR_MQ_Cathan marker; repeated references are one physical destination.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 2. Speak to the Wise Old Man In Dowdun Reach

- Objective key: `DR_Main_Objective_1`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: `DR_MQ_OldMan_Target` (revealed, `QF_DR_MainQuest_Entry.json` / `QFT_UpdateQuestLocation_C_0`); `DR_MQ_OldMan_Target` (revealed, `QF_DR_Main_Cathan.json` / `QFT_UpdateQuestLocation_C_0`)
- Marker 1: **Wise Old Man — Dowdun Reach** (`location`) — map `0.55591667, 0.35863095`; world `222410.0, 32940.0, -5419.941`
  - Determination: reviewed objective relationship to extracted game quest marker 'DR_MQ_OldMan_Target': The objective names the Wise Old Man in Dowdun Reach and the quest flow exposes DR_MQ_OldMan_Target as its exact interaction target; DR_MQ_OldMan is a colocated area/phase duplicate.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 3. Speak to Doric in Dowdun Reach

- Objective key: `DR_Main_Objective_2`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: `DR_MQ_Doric` (revealed, `QF_DR_MainQuest_OldMan.json` / `QFT_UpdateQuestLocation_C_0`)
- Marker 1: **Doric — The Nexus** (`location`) — map `0.56936667, 0.41005238`; world `228059.0, 54537.0, -1170.0`; shared-coordinate group [S60](#s60)
  - Determination: reviewed objective relationship to extracted game quest marker 'DR_MQ_Doric': The route from the Courtyard continues to Doric at The Nexus. DR_MQ_Doric is this quest's exact marker at the Dowdun Doric placement; global Doric registry copies in other regions must not be published.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 4. Investigate the Dreaming Stone

- Objective key: `DR_Main_Objective_3`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: `DR_MQ_DreamingStone` (revealed, `QF_DR_MainQuest_Doric.json` / `QFT_UpdateQuestLocation_C_0`); `DR_MQ_Doric` (hidden/removed, `QF_DR_MainQuest_Doric.json` / `QFT_UpdateQuestLocation_C_1`); `DR_MQ_OldMan_Target` (hidden/removed, `QF_DR_MainQuest_Doric.json` / `QFT_UpdateQuestLocation_C_2`)
- Marker 1: **Dreaming Stone** (`location`) — map `0.5703, 0.40994048`; world `228451.0, 54490.0, -1140.0`; shared-coordinate group [S61](#s61)
  - Determination: reviewed objective relationship to extracted game quest marker 'DR_MQ_DreamingStone': QF_DR_MainQuest_Doric sets this objective and immediately reveals DR_MQ_DreamingStone; the shards are not revealed until the subsequent fragment sequence.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 5. Collect the Shards of the Dreaming Stone around Dowdun Reach: 0/13

- Objective key: `DR_Main_Objective_4`
- Destination mode: `multiple`
- Confidence: **Medium (88/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains. The state exposes a large candidate set, reducing route precision.
- Display note: All thirteen authored shard locations are shown because the aggregate count does not identify which individual shards remain.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **DR_MQ_Fragment_1** (`area`) — map `0.6237154, 0.32018209`; world `250885.47, 16791.479, 1790.0`; shared-coordinate group [S70](#s70)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': QF_DR_MainQuest_Dreamstone independently reveals DR_MQ_Fragment_1 through DR_MQ_Fragment_13 and gates them with per-fragment quest booleans.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **DR_MQ_Fragment_2** (`area`) — map `0.59098333, 0.33170476`; world `237138.0, 21631.0, -4474.0`; shared-coordinate group [S65](#s65)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': QF_DR_MainQuest_Dreamstone independently reveals DR_MQ_Fragment_1 through DR_MQ_Fragment_13 and gates them with per-fragment quest booleans.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **DR_MQ_Fragment_3** (`area`) — map `0.50478571, 0.37952143`; world `200935.0, 41714.0, -3770.0`; shared-coordinate group [S40](#s40)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': QF_DR_MainQuest_Dreamstone independently reveals DR_MQ_Fragment_1 through DR_MQ_Fragment_13 and gates them with per-fragment quest booleans.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **DR_MQ_Fragment_4** (`area`) — map `0.52945476, 0.4240119`; world `211296.0, 60400.0, -2727.0`; shared-coordinate group [S48](#s48)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': QF_DR_MainQuest_Dreamstone independently reveals DR_MQ_Fragment_1 through DR_MQ_Fragment_13 and gates them with per-fragment quest booleans.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 5: **DR_MQ_Fragment_5** (`area`) — map `0.55027143, 0.37528333`; world `220039.0, 39934.0, -11350.0`; shared-coordinate group [S54](#s54)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': QF_DR_MainQuest_Dreamstone independently reveals DR_MQ_Fragment_1 through DR_MQ_Fragment_13 and gates them with per-fragment quest booleans.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 6: **DR_MQ_Fragment_6** (`area`) — map `0.56269762, 0.36642857`; world `225258.0, 36215.0, -16913.0`; shared-coordinate group [S58](#s58)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': QF_DR_MainQuest_Dreamstone independently reveals DR_MQ_Fragment_1 through DR_MQ_Fragment_13 and gates them with per-fragment quest booleans.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 7: **DR_MQ_Fragment_7** (`area`) — map `0.51314174, 0.44434374`; world `204444.53, 68939.37, 373.6807`; shared-coordinate group [S43](#s43)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': QF_DR_MainQuest_Dreamstone independently reveals DR_MQ_Fragment_1 through DR_MQ_Fragment_13 and gates them with per-fragment quest booleans.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 8: **DR_MQ_Fragment_8** (`area`) — map `0.53901429, 0.38740476`; world `215311.0, 45025.0, -7420.0`; shared-coordinate group [S52](#s52)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': QF_DR_MainQuest_Dreamstone independently reveals DR_MQ_Fragment_1 through DR_MQ_Fragment_13 and gates them with per-fragment quest booleans.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 9: **DR_MQ_Fragment_9** (`area`) — map `0.62325, 0.40284524`; world `250690.0, 51510.0, -870.0`; shared-coordinate group [S69](#s69)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': QF_DR_MainQuest_Dreamstone independently reveals DR_MQ_Fragment_1 through DR_MQ_Fragment_13 and gates them with per-fragment quest booleans.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 10: **DR_MQ_Fragment_10** (`area`) — map `0.6469119, 0.36865952`; world `260628.0, 37152.0, -370.0`; shared-coordinate group [S77](#s77)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': QF_DR_MainQuest_Dreamstone independently reveals DR_MQ_Fragment_1 through DR_MQ_Fragment_13 and gates them with per-fragment quest booleans.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 11: **DR_MQ_Fragment_11** (`area`) — map `0.50978095, 0.40071429`; world `203033.0, 50615.0, 250.0`; shared-coordinate group [S42](#s42)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': QF_DR_MainQuest_Dreamstone independently reveals DR_MQ_Fragment_1 through DR_MQ_Fragment_13 and gates them with per-fragment quest booleans.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 12: **DR_MQ_Fragment_12** (`area`) — map `0.50505952, 0.44700476`; world `201050.0, 70057.0, 4854.0`; shared-coordinate group [S41](#s41)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': QF_DR_MainQuest_Dreamstone independently reveals DR_MQ_Fragment_1 through DR_MQ_Fragment_13 and gates them with per-fragment quest booleans.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 13: **DR_MQ_Fragment_13** (`area`) — map `0.5760119, 0.33353571`; world `230850.0, 22400.0, -87.0`; shared-coordinate group [S63](#s63)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': QF_DR_MainQuest_Dreamstone independently reveals DR_MQ_Fragment_1 through DR_MQ_Fragment_13 and gates them with per-fragment quest booleans.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 6. Defeat the Gatekeeper

- Objective key: `DR_Main_Objective_5`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: `DR_MQ_DreamingStone` (hidden/removed, `QF_DR_MainQuest_Dreamstone.json` / `QFT_UpdateQuestLocation_C_15`); `DR_GateKeeper` (revealed, `QF_DR_MainQuest_Dreamstone.json` / `QFT_UpdateQuestLocation_C_4`)
- Marker 1: **Black Knight Titan arena** (`location`) — map `0.5864619, 0.4681619`; world `235239.0, 78943.0, 7451.3457`
  - Determination: reviewed objective relationship to extracted game quest marker 'DR_GateKeeper': Restoring the Dreaming Stone opens the route to The Bastion and the Black Knight Titan encounter. The objective has a direct parent relationship to the unique quest-owned DR_GateKeeper marker at the arena.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 7. Return to the Dreaming Stone

- Objective key: `DR_Main_Objective_6`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: `DR_MQ_DreamingStone` (revealed, `QF_DR_MainQuest_Gatekeeper.json` / `QFT_UpdateQuestLocation_C_0`); `DR_MQ_DreamingStone` (revealed, `QF_DR_MainQuest_Gatekeeper.json` / `QFT_UpdateQuestLocation_C_1`); `DR_MQ_Fragment_8` (hidden/removed, `QF_DR_MainQuest_Gatekeeper.json` / `QFT_UpdateQuestLocation_C_10`); `DR_MQ_Fragment_9` (hidden/removed, `QF_DR_MainQuest_Gatekeeper.json` / `QFT_UpdateQuestLocation_C_11`); `DR_MQ_Fragment_10` (hidden/removed, `QF_DR_MainQuest_Gatekeeper.json` / `QFT_UpdateQuestLocation_C_12`); `DR_MQ_Fragment_11` (hidden/removed, `QF_DR_MainQuest_Gatekeeper.json` / `QFT_UpdateQuestLocation_C_13`); `DR_MQ_Fragment_12` (hidden/removed, `QF_DR_MainQuest_Gatekeeper.json` / `QFT_UpdateQuestLocation_C_14`); `DR_MQ_DreamingStone` (revealed, `QF_DR_MainQuest_Gatekeeper.json` / `QFT_UpdateQuestLocation_C_15`); `DR_MQ_Fragment_1` (hidden/removed, `QF_DR_MainQuest_Gatekeeper.json` / `QFT_UpdateQuestLocation_C_16`); `DR_MQ_Fragment_2` (hidden/removed, `QF_DR_MainQuest_Gatekeeper.json` / `QFT_UpdateQuestLocation_C_17`); `DR_MQ_Fragment_13` (hidden/removed, `QF_DR_MainQuest_Gatekeeper.json` / `QFT_UpdateQuestLocation_C_18`); `DR_MQ_Doric` (hidden/removed, `QF_DR_MainQuest_Gatekeeper.json` / `QFT_UpdateQuestLocation_C_19`); `DR_MQ_DreamingStone` (revealed, `QF_DR_MainQuest_Gatekeeper.json` / `QFT_UpdateQuestLocation_C_2`); `DR_MQ_Death` (hidden/removed, `QF_DR_MainQuest_Gatekeeper.json` / `QFT_UpdateQuestLocation_C_20`); `DR_MQ_OldMan_Target` (hidden/removed, `QF_DR_MainQuest_Gatekeeper.json` / `QFT_UpdateQuestLocation_C_21`); `DR_MQ_OldMan_Target` (hidden/removed, `QF_DR_MainQuest_Gatekeeper.json` / `QFT_UpdateQuestLocation_C_23`); `DR_GateKeeper` (hidden/removed, `QF_DR_MainQuest_Gatekeeper.json` / `QFT_UpdateQuestLocation_C_3`); `DR_GateKeeper` (hidden/removed, `QF_DR_MainQuest_Gatekeeper.json` / `QFT_UpdateQuestLocation_C_4`); `DR_MQ_Fragment_3` (hidden/removed, `QF_DR_MainQuest_Gatekeeper.json` / `QFT_UpdateQuestLocation_C_5`); `DR_MQ_Fragment_4` (hidden/removed, `QF_DR_MainQuest_Gatekeeper.json` / `QFT_UpdateQuestLocation_C_6`); `DR_MQ_Fragment_5` (hidden/removed, `QF_DR_MainQuest_Gatekeeper.json` / `QFT_UpdateQuestLocation_C_7`); `DR_MQ_Fragment_6` (hidden/removed, `QF_DR_MainQuest_Gatekeeper.json` / `QFT_UpdateQuestLocation_C_8`); `DR_MQ_Fragment_7` (hidden/removed, `QF_DR_MainQuest_Gatekeeper.json` / `QFT_UpdateQuestLocation_C_9`); `DR_MQ_DreamingStone` (revealed, `QF_UmS_MQ_Beat1.json` / `QFT_UpdateQuestLocation_C_4`)
- Marker 1: **Avisk — restored Dreaming Stone** (`location`) — map `0.5703, 0.40994048`; world `228451.0, 54490.0, -1140.0`; shared-coordinate group [S61](#s61)
  - Determination: reviewed objective relationship to extracted game quest marker 'DR_MQ_DreamingStone': The objective explicitly returns to the restored Dreaming Stone/Avisk after the Gatekeeper sequence; its authored DR_MQ_DreamingStone location is the destination.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 8. Speak To Death

- Objective key: `DR_Main_Objective_-1`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: `DR_MQ_Death` (revealed, `QF_DR_Main_ProvideQuestOnStart.json` / `QFT_UpdateQuestLocation_C_2`)
- Marker 1: **Death — Forgotten Temple** (`location`) — map `0.33020277, 0.3646539`; world `127610.164, 35469.64, -2585.1736`
  - Determination: reviewed objective relationship to extracted game quest marker 'DR_MQ_Death': This opening state follows the Imaru hand-in and explicitly requires Death. Quest_BlackKnightsFortress owns the exact DR_MQ_Death marker at Death's Forgotten Temple placement; the colocated NPC and duplicate flow discoveries are evidence for the same destination, not additional markers.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 9. Collect the Shards of the Dreaming Stone around Dowdun Reach: 1/13

- Objective key: `DR_Main_Objective_4_1`
- Destination mode: `multiple`
- Confidence: **Medium (88/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains. The state exposes a large candidate set, reducing route precision.
- Display note: The count alone does not reveal which shard was collected; all thirteen possible locations remain visible.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **DR_MQ_Fragment_1** (`area`) — map `0.6237154, 0.32018209`; world `250885.47, 16791.479, 1790.0`; shared-coordinate group [S70](#s70)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **DR_MQ_Fragment_2** (`area`) — map `0.59098333, 0.33170476`; world `237138.0, 21631.0, -4474.0`; shared-coordinate group [S65](#s65)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **DR_MQ_Fragment_3** (`area`) — map `0.50478571, 0.37952143`; world `200935.0, 41714.0, -3770.0`; shared-coordinate group [S40](#s40)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **DR_MQ_Fragment_4** (`area`) — map `0.52945476, 0.4240119`; world `211296.0, 60400.0, -2727.0`; shared-coordinate group [S48](#s48)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 5: **DR_MQ_Fragment_5** (`area`) — map `0.55027143, 0.37528333`; world `220039.0, 39934.0, -11350.0`; shared-coordinate group [S54](#s54)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 6: **DR_MQ_Fragment_6** (`area`) — map `0.56269762, 0.36642857`; world `225258.0, 36215.0, -16913.0`; shared-coordinate group [S58](#s58)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 7: **DR_MQ_Fragment_7** (`area`) — map `0.51314174, 0.44434374`; world `204444.53, 68939.37, 373.6807`; shared-coordinate group [S43](#s43)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 8: **DR_MQ_Fragment_8** (`area`) — map `0.53901429, 0.38740476`; world `215311.0, 45025.0, -7420.0`; shared-coordinate group [S52](#s52)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 9: **DR_MQ_Fragment_9** (`area`) — map `0.62325, 0.40284524`; world `250690.0, 51510.0, -870.0`; shared-coordinate group [S69](#s69)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 10: **DR_MQ_Fragment_10** (`area`) — map `0.6469119, 0.36865952`; world `260628.0, 37152.0, -370.0`; shared-coordinate group [S77](#s77)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 11: **DR_MQ_Fragment_11** (`area`) — map `0.50978095, 0.40071429`; world `203033.0, 50615.0, 250.0`; shared-coordinate group [S42](#s42)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 12: **DR_MQ_Fragment_12** (`area`) — map `0.50505952, 0.44700476`; world `201050.0, 70057.0, 4854.0`; shared-coordinate group [S41](#s41)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 13: **DR_MQ_Fragment_13** (`area`) — map `0.5760119, 0.33353571`; world `230850.0, 22400.0, -87.0`; shared-coordinate group [S63](#s63)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 10. Collect the Shards of the Dreaming Stone around Dowdun Reach: 2/13

- Objective key: `DR_Main_Objective_4_2`
- Destination mode: `multiple`
- Confidence: **Medium (88/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains. The state exposes a large candidate set, reducing route precision.
- Display note: The count alone does not reveal which shards were collected; all thirteen possible locations remain visible.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **DR_MQ_Fragment_1** (`area`) — map `0.6237154, 0.32018209`; world `250885.47, 16791.479, 1790.0`; shared-coordinate group [S70](#s70)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **DR_MQ_Fragment_2** (`area`) — map `0.59098333, 0.33170476`; world `237138.0, 21631.0, -4474.0`; shared-coordinate group [S65](#s65)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **DR_MQ_Fragment_3** (`area`) — map `0.50478571, 0.37952143`; world `200935.0, 41714.0, -3770.0`; shared-coordinate group [S40](#s40)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **DR_MQ_Fragment_4** (`area`) — map `0.52945476, 0.4240119`; world `211296.0, 60400.0, -2727.0`; shared-coordinate group [S48](#s48)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 5: **DR_MQ_Fragment_5** (`area`) — map `0.55027143, 0.37528333`; world `220039.0, 39934.0, -11350.0`; shared-coordinate group [S54](#s54)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 6: **DR_MQ_Fragment_6** (`area`) — map `0.56269762, 0.36642857`; world `225258.0, 36215.0, -16913.0`; shared-coordinate group [S58](#s58)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 7: **DR_MQ_Fragment_7** (`area`) — map `0.51314174, 0.44434374`; world `204444.53, 68939.37, 373.6807`; shared-coordinate group [S43](#s43)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 8: **DR_MQ_Fragment_8** (`area`) — map `0.53901429, 0.38740476`; world `215311.0, 45025.0, -7420.0`; shared-coordinate group [S52](#s52)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 9: **DR_MQ_Fragment_9** (`area`) — map `0.62325, 0.40284524`; world `250690.0, 51510.0, -870.0`; shared-coordinate group [S69](#s69)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 10: **DR_MQ_Fragment_10** (`area`) — map `0.6469119, 0.36865952`; world `260628.0, 37152.0, -370.0`; shared-coordinate group [S77](#s77)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 11: **DR_MQ_Fragment_11** (`area`) — map `0.50978095, 0.40071429`; world `203033.0, 50615.0, 250.0`; shared-coordinate group [S42](#s42)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 12: **DR_MQ_Fragment_12** (`area`) — map `0.50505952, 0.44700476`; world `201050.0, 70057.0, 4854.0`; shared-coordinate group [S41](#s41)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 13: **DR_MQ_Fragment_13** (`area`) — map `0.5760119, 0.33353571`; world `230850.0, 22400.0, -87.0`; shared-coordinate group [S63](#s63)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 11. Collect the Shards of the Dreaming Stone around Dowdun Reach: 3/13

- Objective key: `DR_Main_Objective_4_3`
- Destination mode: `multiple`
- Confidence: **Medium (88/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains. The state exposes a large candidate set, reducing route precision.
- Display note: The count alone does not identify the collected shards; all authored possibilities are shown.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **DR_MQ_Fragment_1** (`area`) — map `0.6237154, 0.32018209`; world `250885.47, 16791.479, 1790.0`; shared-coordinate group [S70](#s70)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **DR_MQ_Fragment_2** (`area`) — map `0.59098333, 0.33170476`; world `237138.0, 21631.0, -4474.0`; shared-coordinate group [S65](#s65)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **DR_MQ_Fragment_3** (`area`) — map `0.50478571, 0.37952143`; world `200935.0, 41714.0, -3770.0`; shared-coordinate group [S40](#s40)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **DR_MQ_Fragment_4** (`area`) — map `0.52945476, 0.4240119`; world `211296.0, 60400.0, -2727.0`; shared-coordinate group [S48](#s48)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 5: **DR_MQ_Fragment_5** (`area`) — map `0.55027143, 0.37528333`; world `220039.0, 39934.0, -11350.0`; shared-coordinate group [S54](#s54)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 6: **DR_MQ_Fragment_6** (`area`) — map `0.56269762, 0.36642857`; world `225258.0, 36215.0, -16913.0`; shared-coordinate group [S58](#s58)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 7: **DR_MQ_Fragment_7** (`area`) — map `0.51314174, 0.44434374`; world `204444.53, 68939.37, 373.6807`; shared-coordinate group [S43](#s43)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 8: **DR_MQ_Fragment_8** (`area`) — map `0.53901429, 0.38740476`; world `215311.0, 45025.0, -7420.0`; shared-coordinate group [S52](#s52)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 9: **DR_MQ_Fragment_9** (`area`) — map `0.62325, 0.40284524`; world `250690.0, 51510.0, -870.0`; shared-coordinate group [S69](#s69)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 10: **DR_MQ_Fragment_10** (`area`) — map `0.6469119, 0.36865952`; world `260628.0, 37152.0, -370.0`; shared-coordinate group [S77](#s77)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 11: **DR_MQ_Fragment_11** (`area`) — map `0.50978095, 0.40071429`; world `203033.0, 50615.0, 250.0`; shared-coordinate group [S42](#s42)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 12: **DR_MQ_Fragment_12** (`area`) — map `0.50505952, 0.44700476`; world `201050.0, 70057.0, 4854.0`; shared-coordinate group [S41](#s41)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 13: **DR_MQ_Fragment_13** (`area`) — map `0.5760119, 0.33353571`; world `230850.0, 22400.0, -87.0`; shared-coordinate group [S63](#s63)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 12. Collect the Shards of the Dreaming Stone around Dowdun Reach: 4/13

- Objective key: `DR_Main_Objective_4_4`
- Destination mode: `multiple`
- Confidence: **Medium (88/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains. The state exposes a large candidate set, reducing route precision.
- Display note: The count alone does not identify the collected shards; all authored possibilities are shown.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **DR_MQ_Fragment_1** (`area`) — map `0.6237154, 0.32018209`; world `250885.47, 16791.479, 1790.0`; shared-coordinate group [S70](#s70)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **DR_MQ_Fragment_2** (`area`) — map `0.59098333, 0.33170476`; world `237138.0, 21631.0, -4474.0`; shared-coordinate group [S65](#s65)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **DR_MQ_Fragment_3** (`area`) — map `0.50478571, 0.37952143`; world `200935.0, 41714.0, -3770.0`; shared-coordinate group [S40](#s40)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **DR_MQ_Fragment_4** (`area`) — map `0.52945476, 0.4240119`; world `211296.0, 60400.0, -2727.0`; shared-coordinate group [S48](#s48)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 5: **DR_MQ_Fragment_5** (`area`) — map `0.55027143, 0.37528333`; world `220039.0, 39934.0, -11350.0`; shared-coordinate group [S54](#s54)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 6: **DR_MQ_Fragment_6** (`area`) — map `0.56269762, 0.36642857`; world `225258.0, 36215.0, -16913.0`; shared-coordinate group [S58](#s58)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 7: **DR_MQ_Fragment_7** (`area`) — map `0.51314174, 0.44434374`; world `204444.53, 68939.37, 373.6807`; shared-coordinate group [S43](#s43)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 8: **DR_MQ_Fragment_8** (`area`) — map `0.53901429, 0.38740476`; world `215311.0, 45025.0, -7420.0`; shared-coordinate group [S52](#s52)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 9: **DR_MQ_Fragment_9** (`area`) — map `0.62325, 0.40284524`; world `250690.0, 51510.0, -870.0`; shared-coordinate group [S69](#s69)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 10: **DR_MQ_Fragment_10** (`area`) — map `0.6469119, 0.36865952`; world `260628.0, 37152.0, -370.0`; shared-coordinate group [S77](#s77)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 11: **DR_MQ_Fragment_11** (`area`) — map `0.50978095, 0.40071429`; world `203033.0, 50615.0, 250.0`; shared-coordinate group [S42](#s42)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 12: **DR_MQ_Fragment_12** (`area`) — map `0.50505952, 0.44700476`; world `201050.0, 70057.0, 4854.0`; shared-coordinate group [S41](#s41)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 13: **DR_MQ_Fragment_13** (`area`) — map `0.5760119, 0.33353571`; world `230850.0, 22400.0, -87.0`; shared-coordinate group [S63](#s63)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 13. Collect the Shards of the Dreaming Stone around Dowdun Reach: 5/13

- Objective key: `DR_Main_Objective_4_5`
- Destination mode: `multiple`
- Confidence: **Medium (88/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains. The state exposes a large candidate set, reducing route precision.
- Display note: The count alone does not identify the collected shards; all authored possibilities are shown.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **DR_MQ_Fragment_1** (`area`) — map `0.6237154, 0.32018209`; world `250885.47, 16791.479, 1790.0`; shared-coordinate group [S70](#s70)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **DR_MQ_Fragment_2** (`area`) — map `0.59098333, 0.33170476`; world `237138.0, 21631.0, -4474.0`; shared-coordinate group [S65](#s65)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **DR_MQ_Fragment_3** (`area`) — map `0.50478571, 0.37952143`; world `200935.0, 41714.0, -3770.0`; shared-coordinate group [S40](#s40)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **DR_MQ_Fragment_4** (`area`) — map `0.52945476, 0.4240119`; world `211296.0, 60400.0, -2727.0`; shared-coordinate group [S48](#s48)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 5: **DR_MQ_Fragment_5** (`area`) — map `0.55027143, 0.37528333`; world `220039.0, 39934.0, -11350.0`; shared-coordinate group [S54](#s54)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 6: **DR_MQ_Fragment_6** (`area`) — map `0.56269762, 0.36642857`; world `225258.0, 36215.0, -16913.0`; shared-coordinate group [S58](#s58)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 7: **DR_MQ_Fragment_7** (`area`) — map `0.51314174, 0.44434374`; world `204444.53, 68939.37, 373.6807`; shared-coordinate group [S43](#s43)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 8: **DR_MQ_Fragment_8** (`area`) — map `0.53901429, 0.38740476`; world `215311.0, 45025.0, -7420.0`; shared-coordinate group [S52](#s52)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 9: **DR_MQ_Fragment_9** (`area`) — map `0.62325, 0.40284524`; world `250690.0, 51510.0, -870.0`; shared-coordinate group [S69](#s69)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 10: **DR_MQ_Fragment_10** (`area`) — map `0.6469119, 0.36865952`; world `260628.0, 37152.0, -370.0`; shared-coordinate group [S77](#s77)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 11: **DR_MQ_Fragment_11** (`area`) — map `0.50978095, 0.40071429`; world `203033.0, 50615.0, 250.0`; shared-coordinate group [S42](#s42)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 12: **DR_MQ_Fragment_12** (`area`) — map `0.50505952, 0.44700476`; world `201050.0, 70057.0, 4854.0`; shared-coordinate group [S41](#s41)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 13: **DR_MQ_Fragment_13** (`area`) — map `0.5760119, 0.33353571`; world `230850.0, 22400.0, -87.0`; shared-coordinate group [S63](#s63)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 14. Collect the Shards of the Dreaming Stone around Dowdun Reach: 6/13

- Objective key: `DR_Main_Objective_4_6`
- Destination mode: `multiple`
- Confidence: **Medium (88/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains. The state exposes a large candidate set, reducing route precision.
- Display note: The count alone does not identify the collected shards; all authored possibilities are shown.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **DR_MQ_Fragment_1** (`area`) — map `0.6237154, 0.32018209`; world `250885.47, 16791.479, 1790.0`; shared-coordinate group [S70](#s70)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **DR_MQ_Fragment_2** (`area`) — map `0.59098333, 0.33170476`; world `237138.0, 21631.0, -4474.0`; shared-coordinate group [S65](#s65)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **DR_MQ_Fragment_3** (`area`) — map `0.50478571, 0.37952143`; world `200935.0, 41714.0, -3770.0`; shared-coordinate group [S40](#s40)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **DR_MQ_Fragment_4** (`area`) — map `0.52945476, 0.4240119`; world `211296.0, 60400.0, -2727.0`; shared-coordinate group [S48](#s48)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 5: **DR_MQ_Fragment_5** (`area`) — map `0.55027143, 0.37528333`; world `220039.0, 39934.0, -11350.0`; shared-coordinate group [S54](#s54)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 6: **DR_MQ_Fragment_6** (`area`) — map `0.56269762, 0.36642857`; world `225258.0, 36215.0, -16913.0`; shared-coordinate group [S58](#s58)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 7: **DR_MQ_Fragment_7** (`area`) — map `0.51314174, 0.44434374`; world `204444.53, 68939.37, 373.6807`; shared-coordinate group [S43](#s43)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 8: **DR_MQ_Fragment_8** (`area`) — map `0.53901429, 0.38740476`; world `215311.0, 45025.0, -7420.0`; shared-coordinate group [S52](#s52)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 9: **DR_MQ_Fragment_9** (`area`) — map `0.62325, 0.40284524`; world `250690.0, 51510.0, -870.0`; shared-coordinate group [S69](#s69)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 10: **DR_MQ_Fragment_10** (`area`) — map `0.6469119, 0.36865952`; world `260628.0, 37152.0, -370.0`; shared-coordinate group [S77](#s77)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 11: **DR_MQ_Fragment_11** (`area`) — map `0.50978095, 0.40071429`; world `203033.0, 50615.0, 250.0`; shared-coordinate group [S42](#s42)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 12: **DR_MQ_Fragment_12** (`area`) — map `0.50505952, 0.44700476`; world `201050.0, 70057.0, 4854.0`; shared-coordinate group [S41](#s41)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 13: **DR_MQ_Fragment_13** (`area`) — map `0.5760119, 0.33353571`; world `230850.0, 22400.0, -87.0`; shared-coordinate group [S63](#s63)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 15. Collect the Shards of the Dreaming Stone around Dowdun Reach: 7/13

- Objective key: `DR_Main_Objective_4_7`
- Destination mode: `multiple`
- Confidence: **Medium (88/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains. The state exposes a large candidate set, reducing route precision.
- Display note: The count alone does not identify the collected shards; all authored possibilities are shown.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **DR_MQ_Fragment_1** (`area`) — map `0.6237154, 0.32018209`; world `250885.47, 16791.479, 1790.0`; shared-coordinate group [S70](#s70)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **DR_MQ_Fragment_2** (`area`) — map `0.59098333, 0.33170476`; world `237138.0, 21631.0, -4474.0`; shared-coordinate group [S65](#s65)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **DR_MQ_Fragment_3** (`area`) — map `0.50478571, 0.37952143`; world `200935.0, 41714.0, -3770.0`; shared-coordinate group [S40](#s40)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **DR_MQ_Fragment_4** (`area`) — map `0.52945476, 0.4240119`; world `211296.0, 60400.0, -2727.0`; shared-coordinate group [S48](#s48)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 5: **DR_MQ_Fragment_5** (`area`) — map `0.55027143, 0.37528333`; world `220039.0, 39934.0, -11350.0`; shared-coordinate group [S54](#s54)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 6: **DR_MQ_Fragment_6** (`area`) — map `0.56269762, 0.36642857`; world `225258.0, 36215.0, -16913.0`; shared-coordinate group [S58](#s58)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 7: **DR_MQ_Fragment_7** (`area`) — map `0.51314174, 0.44434374`; world `204444.53, 68939.37, 373.6807`; shared-coordinate group [S43](#s43)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 8: **DR_MQ_Fragment_8** (`area`) — map `0.53901429, 0.38740476`; world `215311.0, 45025.0, -7420.0`; shared-coordinate group [S52](#s52)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 9: **DR_MQ_Fragment_9** (`area`) — map `0.62325, 0.40284524`; world `250690.0, 51510.0, -870.0`; shared-coordinate group [S69](#s69)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 10: **DR_MQ_Fragment_10** (`area`) — map `0.6469119, 0.36865952`; world `260628.0, 37152.0, -370.0`; shared-coordinate group [S77](#s77)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 11: **DR_MQ_Fragment_11** (`area`) — map `0.50978095, 0.40071429`; world `203033.0, 50615.0, 250.0`; shared-coordinate group [S42](#s42)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 12: **DR_MQ_Fragment_12** (`area`) — map `0.50505952, 0.44700476`; world `201050.0, 70057.0, 4854.0`; shared-coordinate group [S41](#s41)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 13: **DR_MQ_Fragment_13** (`area`) — map `0.5760119, 0.33353571`; world `230850.0, 22400.0, -87.0`; shared-coordinate group [S63](#s63)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 16. Collect the Shards of the Dreaming Stone around Dowdun Reach: 8/13

- Objective key: `DR_Main_Objective_4_8`
- Destination mode: `multiple`
- Confidence: **Medium (88/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains. The state exposes a large candidate set, reducing route precision.
- Display note: The count alone does not identify the collected shards; all authored possibilities are shown.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **DR_MQ_Fragment_1** (`area`) — map `0.6237154, 0.32018209`; world `250885.47, 16791.479, 1790.0`; shared-coordinate group [S70](#s70)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **DR_MQ_Fragment_2** (`area`) — map `0.59098333, 0.33170476`; world `237138.0, 21631.0, -4474.0`; shared-coordinate group [S65](#s65)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **DR_MQ_Fragment_3** (`area`) — map `0.50478571, 0.37952143`; world `200935.0, 41714.0, -3770.0`; shared-coordinate group [S40](#s40)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **DR_MQ_Fragment_4** (`area`) — map `0.52945476, 0.4240119`; world `211296.0, 60400.0, -2727.0`; shared-coordinate group [S48](#s48)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 5: **DR_MQ_Fragment_5** (`area`) — map `0.55027143, 0.37528333`; world `220039.0, 39934.0, -11350.0`; shared-coordinate group [S54](#s54)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 6: **DR_MQ_Fragment_6** (`area`) — map `0.56269762, 0.36642857`; world `225258.0, 36215.0, -16913.0`; shared-coordinate group [S58](#s58)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 7: **DR_MQ_Fragment_7** (`area`) — map `0.51314174, 0.44434374`; world `204444.53, 68939.37, 373.6807`; shared-coordinate group [S43](#s43)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 8: **DR_MQ_Fragment_8** (`area`) — map `0.53901429, 0.38740476`; world `215311.0, 45025.0, -7420.0`; shared-coordinate group [S52](#s52)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 9: **DR_MQ_Fragment_9** (`area`) — map `0.62325, 0.40284524`; world `250690.0, 51510.0, -870.0`; shared-coordinate group [S69](#s69)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 10: **DR_MQ_Fragment_10** (`area`) — map `0.6469119, 0.36865952`; world `260628.0, 37152.0, -370.0`; shared-coordinate group [S77](#s77)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 11: **DR_MQ_Fragment_11** (`area`) — map `0.50978095, 0.40071429`; world `203033.0, 50615.0, 250.0`; shared-coordinate group [S42](#s42)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 12: **DR_MQ_Fragment_12** (`area`) — map `0.50505952, 0.44700476`; world `201050.0, 70057.0, 4854.0`; shared-coordinate group [S41](#s41)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 13: **DR_MQ_Fragment_13** (`area`) — map `0.5760119, 0.33353571`; world `230850.0, 22400.0, -87.0`; shared-coordinate group [S63](#s63)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 17. Collect the Shards of the Dreaming Stone around Dowdun Reach: 9/13

- Objective key: `DR_Main_Objective_4_9`
- Destination mode: `multiple`
- Confidence: **Medium (88/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains. The state exposes a large candidate set, reducing route precision.
- Display note: The count alone does not identify the collected shards; all authored possibilities are shown.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **DR_MQ_Fragment_1** (`area`) — map `0.6237154, 0.32018209`; world `250885.47, 16791.479, 1790.0`; shared-coordinate group [S70](#s70)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **DR_MQ_Fragment_2** (`area`) — map `0.59098333, 0.33170476`; world `237138.0, 21631.0, -4474.0`; shared-coordinate group [S65](#s65)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **DR_MQ_Fragment_3** (`area`) — map `0.50478571, 0.37952143`; world `200935.0, 41714.0, -3770.0`; shared-coordinate group [S40](#s40)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **DR_MQ_Fragment_4** (`area`) — map `0.52945476, 0.4240119`; world `211296.0, 60400.0, -2727.0`; shared-coordinate group [S48](#s48)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 5: **DR_MQ_Fragment_5** (`area`) — map `0.55027143, 0.37528333`; world `220039.0, 39934.0, -11350.0`; shared-coordinate group [S54](#s54)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 6: **DR_MQ_Fragment_6** (`area`) — map `0.56269762, 0.36642857`; world `225258.0, 36215.0, -16913.0`; shared-coordinate group [S58](#s58)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 7: **DR_MQ_Fragment_7** (`area`) — map `0.51314174, 0.44434374`; world `204444.53, 68939.37, 373.6807`; shared-coordinate group [S43](#s43)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 8: **DR_MQ_Fragment_8** (`area`) — map `0.53901429, 0.38740476`; world `215311.0, 45025.0, -7420.0`; shared-coordinate group [S52](#s52)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 9: **DR_MQ_Fragment_9** (`area`) — map `0.62325, 0.40284524`; world `250690.0, 51510.0, -870.0`; shared-coordinate group [S69](#s69)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 10: **DR_MQ_Fragment_10** (`area`) — map `0.6469119, 0.36865952`; world `260628.0, 37152.0, -370.0`; shared-coordinate group [S77](#s77)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 11: **DR_MQ_Fragment_11** (`area`) — map `0.50978095, 0.40071429`; world `203033.0, 50615.0, 250.0`; shared-coordinate group [S42](#s42)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 12: **DR_MQ_Fragment_12** (`area`) — map `0.50505952, 0.44700476`; world `201050.0, 70057.0, 4854.0`; shared-coordinate group [S41](#s41)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 13: **DR_MQ_Fragment_13** (`area`) — map `0.5760119, 0.33353571`; world `230850.0, 22400.0, -87.0`; shared-coordinate group [S63](#s63)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 18. Collect the Shards of the Dreaming Stone around Dowdun Reach: 10/13

- Objective key: `DR_Main_Objective_4_10`
- Destination mode: `multiple`
- Confidence: **Medium (88/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains. The state exposes a large candidate set, reducing route precision.
- Display note: The count alone does not identify the collected shards; all authored possibilities are shown.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **DR_MQ_Fragment_1** (`area`) — map `0.6237154, 0.32018209`; world `250885.47, 16791.479, 1790.0`; shared-coordinate group [S70](#s70)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **DR_MQ_Fragment_2** (`area`) — map `0.59098333, 0.33170476`; world `237138.0, 21631.0, -4474.0`; shared-coordinate group [S65](#s65)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **DR_MQ_Fragment_3** (`area`) — map `0.50478571, 0.37952143`; world `200935.0, 41714.0, -3770.0`; shared-coordinate group [S40](#s40)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **DR_MQ_Fragment_4** (`area`) — map `0.52945476, 0.4240119`; world `211296.0, 60400.0, -2727.0`; shared-coordinate group [S48](#s48)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 5: **DR_MQ_Fragment_5** (`area`) — map `0.55027143, 0.37528333`; world `220039.0, 39934.0, -11350.0`; shared-coordinate group [S54](#s54)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 6: **DR_MQ_Fragment_6** (`area`) — map `0.56269762, 0.36642857`; world `225258.0, 36215.0, -16913.0`; shared-coordinate group [S58](#s58)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 7: **DR_MQ_Fragment_7** (`area`) — map `0.51314174, 0.44434374`; world `204444.53, 68939.37, 373.6807`; shared-coordinate group [S43](#s43)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 8: **DR_MQ_Fragment_8** (`area`) — map `0.53901429, 0.38740476`; world `215311.0, 45025.0, -7420.0`; shared-coordinate group [S52](#s52)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 9: **DR_MQ_Fragment_9** (`area`) — map `0.62325, 0.40284524`; world `250690.0, 51510.0, -870.0`; shared-coordinate group [S69](#s69)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 10: **DR_MQ_Fragment_10** (`area`) — map `0.6469119, 0.36865952`; world `260628.0, 37152.0, -370.0`; shared-coordinate group [S77](#s77)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 11: **DR_MQ_Fragment_11** (`area`) — map `0.50978095, 0.40071429`; world `203033.0, 50615.0, 250.0`; shared-coordinate group [S42](#s42)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 12: **DR_MQ_Fragment_12** (`area`) — map `0.50505952, 0.44700476`; world `201050.0, 70057.0, 4854.0`; shared-coordinate group [S41](#s41)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 13: **DR_MQ_Fragment_13** (`area`) — map `0.5760119, 0.33353571`; world `230850.0, 22400.0, -87.0`; shared-coordinate group [S63](#s63)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 19. Collect the Shards of the Dreaming Stone around Dowdun Reach: 11/13

- Objective key: `DR_Main_Objective_4_11`
- Destination mode: `multiple`
- Confidence: **Medium (88/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains. The state exposes a large candidate set, reducing route precision.
- Display note: The count alone does not identify the collected shards; all authored possibilities are shown.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **DR_MQ_Fragment_1** (`area`) — map `0.6237154, 0.32018209`; world `250885.47, 16791.479, 1790.0`; shared-coordinate group [S70](#s70)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **DR_MQ_Fragment_2** (`area`) — map `0.59098333, 0.33170476`; world `237138.0, 21631.0, -4474.0`; shared-coordinate group [S65](#s65)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **DR_MQ_Fragment_3** (`area`) — map `0.50478571, 0.37952143`; world `200935.0, 41714.0, -3770.0`; shared-coordinate group [S40](#s40)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **DR_MQ_Fragment_4** (`area`) — map `0.52945476, 0.4240119`; world `211296.0, 60400.0, -2727.0`; shared-coordinate group [S48](#s48)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 5: **DR_MQ_Fragment_5** (`area`) — map `0.55027143, 0.37528333`; world `220039.0, 39934.0, -11350.0`; shared-coordinate group [S54](#s54)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 6: **DR_MQ_Fragment_6** (`area`) — map `0.56269762, 0.36642857`; world `225258.0, 36215.0, -16913.0`; shared-coordinate group [S58](#s58)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 7: **DR_MQ_Fragment_7** (`area`) — map `0.51314174, 0.44434374`; world `204444.53, 68939.37, 373.6807`; shared-coordinate group [S43](#s43)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 8: **DR_MQ_Fragment_8** (`area`) — map `0.53901429, 0.38740476`; world `215311.0, 45025.0, -7420.0`; shared-coordinate group [S52](#s52)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 9: **DR_MQ_Fragment_9** (`area`) — map `0.62325, 0.40284524`; world `250690.0, 51510.0, -870.0`; shared-coordinate group [S69](#s69)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 10: **DR_MQ_Fragment_10** (`area`) — map `0.6469119, 0.36865952`; world `260628.0, 37152.0, -370.0`; shared-coordinate group [S77](#s77)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 11: **DR_MQ_Fragment_11** (`area`) — map `0.50978095, 0.40071429`; world `203033.0, 50615.0, 250.0`; shared-coordinate group [S42](#s42)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 12: **DR_MQ_Fragment_12** (`area`) — map `0.50505952, 0.44700476`; world `201050.0, 70057.0, 4854.0`; shared-coordinate group [S41](#s41)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 13: **DR_MQ_Fragment_13** (`area`) — map `0.5760119, 0.33353571`; world `230850.0, 22400.0, -87.0`; shared-coordinate group [S63](#s63)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 20. Collect the Shards of the Dreaming Stone around Dowdun Reach: 12/13

- Objective key: `DR_Main_Objective_4_12`
- Destination mode: `multiple`
- Confidence: **Medium (88/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains. The state exposes a large candidate set, reducing route precision.
- Display note: One shard remains, but the aggregate count does not identify which one; all thirteen authored possibilities are shown.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **DR_MQ_Fragment_1** (`area`) — map `0.6237154, 0.32018209`; world `250885.47, 16791.479, 1790.0`; shared-coordinate group [S70](#s70)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **DR_MQ_Fragment_2** (`area`) — map `0.59098333, 0.33170476`; world `237138.0, 21631.0, -4474.0`; shared-coordinate group [S65](#s65)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **DR_MQ_Fragment_3** (`area`) — map `0.50478571, 0.37952143`; world `200935.0, 41714.0, -3770.0`; shared-coordinate group [S40](#s40)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **DR_MQ_Fragment_4** (`area`) — map `0.52945476, 0.4240119`; world `211296.0, 60400.0, -2727.0`; shared-coordinate group [S48](#s48)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 5: **DR_MQ_Fragment_5** (`area`) — map `0.55027143, 0.37528333`; world `220039.0, 39934.0, -11350.0`; shared-coordinate group [S54](#s54)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 6: **DR_MQ_Fragment_6** (`area`) — map `0.56269762, 0.36642857`; world `225258.0, 36215.0, -16913.0`; shared-coordinate group [S58](#s58)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 7: **DR_MQ_Fragment_7** (`area`) — map `0.51314174, 0.44434374`; world `204444.53, 68939.37, 373.6807`; shared-coordinate group [S43](#s43)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 8: **DR_MQ_Fragment_8** (`area`) — map `0.53901429, 0.38740476`; world `215311.0, 45025.0, -7420.0`; shared-coordinate group [S52](#s52)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 9: **DR_MQ_Fragment_9** (`area`) — map `0.62325, 0.40284524`; world `250690.0, 51510.0, -870.0`; shared-coordinate group [S69](#s69)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 10: **DR_MQ_Fragment_10** (`area`) — map `0.6469119, 0.36865952`; world `260628.0, 37152.0, -370.0`; shared-coordinate group [S77](#s77)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 11: **DR_MQ_Fragment_11** (`area`) — map `0.50978095, 0.40071429`; world `203033.0, 50615.0, 250.0`; shared-coordinate group [S42](#s42)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 12: **DR_MQ_Fragment_12** (`area`) — map `0.50505952, 0.44700476`; world `201050.0, 70057.0, 4854.0`; shared-coordinate group [S41](#s41)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 13: **DR_MQ_Fragment_13** (`area`) — map `0.5760119, 0.33353571`; world `230850.0, 22400.0, -87.0`; shared-coordinate group [S63](#s63)
  - Determination: reviewed objective relationship to extracted game quest marker prefix 'DR_MQ_Fragment_': The fragment flow tracks thirteen independent booleans, while this public objective exposes only an aggregate count.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 21. Return to the Dreaming Stone

- Objective key: `DR_Main_Objective_4_13`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: `DR_MQ_DreamingStone` (revealed, `QF_DR_MainQuest_Fragments.json` / `QFT_UpdateQuestLocation_C_15`)
- Marker 1: **Dreaming Stone** (`location`) — map `0.5703, 0.40994048`; world `228451.0, 54490.0, -1140.0`; shared-coordinate group [S61](#s61)
  - Determination: reviewed objective relationship to extracted game quest marker 'DR_MQ_DreamingStone': At 13/13 the collection is complete and QF_DR_MainQuest_Fragments reveals DR_MQ_DreamingStone for the hand-in; fragment markers are no longer destinations.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 22. Speak to Doric about the shards around Dowdun Reach

- Objective key: `DR_Main_Objective_4_-1`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: `DR_MQ_OldMan_Target` (hidden/removed, `QF_DR_MainQuest_Fragments.json` / `QFT_UpdateQuestLocation_C_13`); `DR_MQ_Doric` (revealed, `QF_DR_MainQuest_Fragments.json` / `QFT_UpdateQuestLocation_C_14`)
- Marker 1: **Doric — shard briefing at The Nexus** (`location`) — map `0.56936667, 0.41005238`; world `228059.0, 54537.0, -1170.0`; shared-coordinate group [S60](#s60)
  - Determination: reviewed objective relationship to extracted game quest marker 'DR_MQ_Doric': This pre-collection state explicitly asks Doric about the shards. The current quest's DR_MQ_Doric marker is colocated with the Dowdun Doric actor at The Nexus and excludes his unrelated global placements.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

### Biohazard

Side quest · Dowdun Reach

#### 1. Speak to Zanik about the Corrupted Essence you got from a strange raven.

- Objective key: `DR_SQ3_EntryPickup`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Display note: Zanik is underground in the cell beneath The Approach; the surface projection is the exact quest-owned Zanik marker.
- Reviewed relationship: **yes**
- Cooked location operations: `DR_SQ3_Zanik` (revealed, `QF_DR_SQ3_Nevermore.json` / `QFT_UpdateQuestLocation_C_4`); `DR_SQ3_Zanik` (revealed, `QF_DR_SQ3_Nevermore_Ravens.json` / `QFT_UpdateQuestLocation_C_0`); `DR_SQ3_Zanik` (revealed, `QF_DR_SQ3_Nevermore_Ravens.json` / `QFT_UpdateQuestLocation_C_30`); `DR_SQ3_Zanik` (revealed, `QF_DR_SQ3_Nevermore_Ravens.json` / `QFT_UpdateQuestLocation_C_35`); `DR_SQ3_Zanik` (revealed, `QF_DR_SQ3_Nevermore_Ravens.json` / `QFT_UpdateQuestLocation_C_37`); `DR_SQ3_Zanik` (revealed, `QF_DR_SQ3_Nevermore_Ravens.json` / `QFT_UpdateQuestLocation_C_39`); `DR_SQ3_Zanik` (revealed, `QF_DR_SQ3_Nevermore_Ravens.json` / `QFT_UpdateQuestLocation_C_41`); `DR_SQ3_Zanik` (revealed, `QF_DR_SQ3_Nevermore_Ravens.json` / `QFT_UpdateQuestLocation_C_43`); `DR_SQ3_Zanik` (revealed, `QF_DR_SQ3_Nevermore_Ravens.json` / `QFT_UpdateQuestLocation_C_45`); `DR_SQ3_Zanik` (revealed, `QF_DR_SQ3_Nevermore_Ravens.json` / `QFT_UpdateQuestLocation_C_47`); `DR_SQ3_Zanik` (revealed, `QF_DR_SQ3_Nevermore_Ravens.json` / `QFT_UpdateQuestLocation_C_49`)
- Marker 1: **Zanik — cell beneath The Approach** (`npc`) — map `0.55128119, 0.34590352`; world `220463.1, 27594.48, -7423.706`; shared-coordinate group [S55](#s55)
  - Determination: reviewed objective relationship to extracted game quest marker 'DR_SQ3_Zanik': Picking up a Corrupted Essence Sample starts this entry path, but the objective explicitly requires speaking to Zanik. DR_SQ3_Zanik is Biohazard's exact Dowdun marker and prevents unrelated Zanik phase copies from being published.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 2. Gather 10 biological samples from strange ravens in Dowdun Reach, and bring them to Zanik.

- Objective key: `DR_SQ3_HuntRavens`
- Destination mode: `multiple`
- Confidence: **Medium (88/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains. The state exposes a large candidate set, reducing route precision.
- Display note: All ten current-build corrupted-raven spawn points are possible sample sources; Zanik beneath The Approach is the hand-in.
- Reviewed relationship: **yes**
- Cooked location operations: `DR_SQ3_Zanik` (revealed, `QF_DR_SQ3_Nevermore.json` / `QFT_UpdateQuestLocation_C_5`); `DR_SQ3_Zanik` (revealed, `QF_DR_SQ3_Nevermore.json` / `QFT_UpdateQuestLocation_C_6`)
- Marker 1: **Corrupted Raven spawn** (`npc`) — map `0.55410588, 0.35320685`; world `221649.47, 30661.877, -4402.102`
  - Determination: reviewed objective relationship to extracted game npc 'magical-raven' actor type 'BP_SpawnPoint_MagicalRaven': BP_SpawnPoint_MagicalRaven uses the quest-specific Magical Raven character/data and the current world contains exactly ten placements. The quest-owned DR_SQ3_Zanik marker is the return point.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Corrupted Raven spawn** (`npc`) — map `0.53646281, 0.39438952`; world `214239.38, 47958.598, -2638.337`
  - Determination: reviewed objective relationship to extracted game npc 'magical-raven' actor type 'BP_SpawnPoint_MagicalRaven': BP_SpawnPoint_MagicalRaven uses the quest-specific Magical Raven character/data and the current world contains exactly ten placements. The quest-owned DR_SQ3_Zanik marker is the return point.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Corrupted Raven spawn** (`npc`) — map `0.46621593, 0.4114359`; world `184735.69, 55118.08, 3132.773`
  - Determination: reviewed objective relationship to extracted game npc 'magical-raven' actor type 'BP_SpawnPoint_MagicalRaven': BP_SpawnPoint_MagicalRaven uses the quest-specific Magical Raven character/data and the current world contains exactly ten placements. The quest-owned DR_SQ3_Zanik marker is the return point.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **Corrupted Raven spawn** (`npc`) — map `0.63133802, 0.3184271`; world `254086.97, 16054.383, 3012.303`
  - Determination: reviewed objective relationship to extracted game npc 'magical-raven' actor type 'BP_SpawnPoint_MagicalRaven': BP_SpawnPoint_MagicalRaven uses the quest-specific Magical Raven character/data and the current world contains exactly ten placements. The quest-owned DR_SQ3_Zanik marker is the return point.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 5: **Corrupted Raven spawn** (`npc`) — map `0.61502419, 0.40229404`; world `247235.16, 51278.496, 1817.135`
  - Determination: reviewed objective relationship to extracted game npc 'magical-raven' actor type 'BP_SpawnPoint_MagicalRaven': BP_SpawnPoint_MagicalRaven uses the quest-specific Magical Raven character/data and the current world contains exactly ten placements. The quest-owned DR_SQ3_Zanik marker is the return point.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 6: **Corrupted Raven spawn** (`npc`) — map `0.56930833, 0.35271847`; world `228034.5, 30456.758, -1751.56`
  - Determination: reviewed objective relationship to extracted game npc 'magical-raven' actor type 'BP_SpawnPoint_MagicalRaven': BP_SpawnPoint_MagicalRaven uses the quest-specific Magical Raven character/data and the current world contains exactly ten placements. The quest-owned DR_SQ3_Zanik marker is the return point.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 7: **Corrupted Raven spawn** (`npc`) — map `0.54101774, 0.42902264`; world `216152.45, 62504.508, -1439.161`
  - Determination: reviewed objective relationship to extracted game npc 'magical-raven' actor type 'BP_SpawnPoint_MagicalRaven': BP_SpawnPoint_MagicalRaven uses the quest-specific Magical Raven character/data and the current world contains exactly ten placements. The quest-owned DR_SQ3_Zanik marker is the return point.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 8: **Corrupted Raven spawn** (`npc`) — map `0.55064702, 0.40696956`; world `220196.75, 53242.215, -551.825`
  - Determination: reviewed objective relationship to extracted game npc 'magical-raven' actor type 'BP_SpawnPoint_MagicalRaven': BP_SpawnPoint_MagicalRaven uses the quest-specific Magical Raven character/data and the current world contains exactly ten placements. The quest-owned DR_SQ3_Zanik marker is the return point.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 9: **Corrupted Raven spawn** (`npc`) — map `0.60883021, 0.34577763`; world `244633.69, 27541.605, 618.283`
  - Determination: reviewed objective relationship to extracted game npc 'magical-raven' actor type 'BP_SpawnPoint_MagicalRaven': BP_SpawnPoint_MagicalRaven uses the quest-specific Magical Raven character/data and the current world contains exactly ten placements. The quest-owned DR_SQ3_Zanik marker is the return point.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 10: **Corrupted Raven spawn** (`npc`) — map `0.50557343, 0.40249547`; world `201265.84, 51363.098, 1597.753`
  - Determination: reviewed objective relationship to extracted game npc 'magical-raven' actor type 'BP_SpawnPoint_MagicalRaven': BP_SpawnPoint_MagicalRaven uses the quest-specific Magical Raven character/data and the current world contains exactly ten placements. The quest-owned DR_SQ3_Zanik marker is the return point.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 11: **Zanik (sample hand-in)** (`npc`) — map `0.55128119, 0.34590352`; world `220463.1, 27594.48, -7423.706`; shared-coordinate group [S55](#s55)
  - Determination: reviewed objective relationship to extracted game quest marker 'DR_SQ3_Zanik': BP_SpawnPoint_MagicalRaven uses the quest-specific Magical Raven character/data and the current world contains exactly ten placements. The quest-owned DR_SQ3_Zanik marker is the return point.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 3. Bring Zanik the ingredients. Dragons Blood. A Twilight Lily and some Unholy Water.

- Objective key: `DR_SQ3_FindIngredients`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: Dragon Blood may instead be obtained from Dragon Wolves in Fellhollow. The map shows the four current Dowdun Blue Dragon placements to avoid flooding it with every renewable Dragon Wolf spawn, plus both fixed pickups and Zanik's hand-in.
- Reviewed relationship: **yes**
- Cooked location operations: `DR_SQ3_HolyFont` (revealed, `QF_DR_SQ3_Nevermore.json` / `QFT_UpdateQuestLocation_C_2`); `DR_SQ3_MidnightLilly` (revealed, `QF_DR_SQ3_Nevermore.json` / `QFT_UpdateQuestLocation_C_7`)
- Marker 1: **Twilight Lily — The Lake** (`location`) — map `0.55322619, 0.38122619`; world `221280.0, 42430.0, -11215.0`
  - Determination: reviewed objective relationship to extracted game registry landmark 'A3D9F580-489D131D-5092F8A4-42ECD820': The guide and cooked objective require Dragon Blood, Twilight Lily, and Unholy Water. The current world registry provides exact identities for the lily at The Lake and water behind The Cathedral; the NPC catalog contains four Blue Dragons in Dowdun Reach, and DR_SQ3_Zanik is the required delivery point.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Unholy Water — behind The Cathedral** (`location`) — map `0.52225714, 0.42470476`; world `208273.0, 60691.0, -2835.0`
  - Determination: reviewed objective relationship to extracted game registry landmark '26470032-49E96542-0CAF80AB-54320698': The guide and cooked objective require Dragon Blood, Twilight Lily, and Unholy Water. The current world registry provides exact identities for the lily at The Lake and water behind The Cathedral; the NPC catalog contains four Blue Dragons in Dowdun Reach, and DR_SQ3_Zanik is the required delivery point.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Blue Dragon — Dragon Blood source** (`npc`) — map `0.48651964, 0.39195163`; world `193263.25, 46934.684, 1710.893`
  - Determination: reviewed objective relationship to extracted game npc 'blue-dragon': The guide and cooked objective require Dragon Blood, Twilight Lily, and Unholy Water. The current world registry provides exact identities for the lily at The Lake and water behind The Cathedral; the NPC catalog contains four Blue Dragons in Dowdun Reach, and DR_SQ3_Zanik is the required delivery point.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **Blue Dragon — Dragon Blood source** (`npc`) — map `0.49684186, 0.44843949`; world `197598.58, 70659.586, 4429.321`
  - Determination: reviewed objective relationship to extracted game npc 'blue-dragon': The guide and cooked objective require Dragon Blood, Twilight Lily, and Unholy Water. The current world registry provides exact identities for the lily at The Lake and water behind The Cathedral; the NPC catalog contains four Blue Dragons in Dowdun Reach, and DR_SQ3_Zanik is the required delivery point.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 5: **Blue Dragon — Dragon Blood source** (`npc`) — map `0.62199631, 0.31893352`; world `250163.45, 16267.077, 1794.011`
  - Determination: reviewed objective relationship to extracted game npc 'blue-dragon': The guide and cooked objective require Dragon Blood, Twilight Lily, and Unholy Water. The current world registry provides exact identities for the lily at The Lake and water behind The Cathedral; the NPC catalog contains four Blue Dragons in Dowdun Reach, and DR_SQ3_Zanik is the required delivery point.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 6: **Blue Dragon — Dragon Blood source** (`npc`) — map `0.59762605, 0.31446825`; world `239927.94, 14391.664, -2631.7`
  - Determination: reviewed objective relationship to extracted game npc 'blue-dragon': The guide and cooked objective require Dragon Blood, Twilight Lily, and Unholy Water. The current world registry provides exact identities for the lily at The Lake and water behind The Cathedral; the NPC catalog contains four Blue Dragons in Dowdun Reach, and DR_SQ3_Zanik is the required delivery point.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 7: **Zanik — ingredient hand-in** (`npc`) — map `0.55128119, 0.34590352`; world `220463.1, 27594.48, -7423.706`; shared-coordinate group [S55](#s55)
  - Determination: reviewed objective relationship to extracted game quest marker 'DR_SQ3_Zanik': The guide and cooked objective require Dragon Blood, Twilight Lily, and Unholy Water. The current world registry provides exact identities for the lily at The Lake and water behind The Cathedral; the NPC catalog contains four Blue Dragons in Dowdun Reach, and DR_SQ3_Zanik is the required delivery point.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 4. Find the source of the cthonian infection, somewhere high up in Dowdun Reach.

- Objective key: `DR_SQ3_LocateSource`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Putrilius — Grand Hall roof** (`npc`) — map `0.61099262, 0.39926579`; world `245541.9, 50006.633, 3572.673`
  - Determination: reviewed objective relationship to extracted game npc 'pusilineous': Zanik's dialogue identifies the central intelligence as Putrilius/Pusilineous; the world contains one BP_NPC_Pusilineous quest placement high in Dowdun Reach.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 5. Return to Zanik and explain the situation.

- Objective key: `DR_SQ3_FinalBeat`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Zanik — final report** (`npc`) — map `0.55128119, 0.34590352`; world `220463.1, 27594.48, -7423.706`; shared-coordinate group [S55](#s55)
  - Determination: reviewed objective relationship to extracted game quest marker 'DR_SQ3_Zanik': The objective explicitly returns to Zanik; Quest_SQ3_Nevermore owns DR_SQ3_Zanik.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

### Wanted!

Side quest · Dowdun Reach

#### 1. Bring the Journal to Vannaka

- Objective key: `DR_SQ2_EntryJournal`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: `DR_SQ2_Vannaka` (revealed, `QF_DR_SQ2_AnimalLover.json` / `QFT_UpdateQuestLocation_C_13`)
- Marker 1: **Vannaka — Wanted! journal hand-in** (`npc`) — map `0.52709762, 0.36161905`; world `210306.0, 34195.0, -4532.0`; shared-coordinate group [S46](#s46)
  - Determination: reviewed objective relationship to extracted game quest marker 'DR_SQ2_Vannaka': This alternate entry state occurs after a journal is found before accepting the quest and explicitly requires bringing it to Vannaka. DR_SQ2_Vannaka is the quest-owned Courtyard marker and excludes Vannaka's other regional phase copies.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 2. Journal Pages Collected 0/3

- Objective key: `DR_SQ2_FindPages_0`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: The three journals can be collected in any order; these are the exact post-fight journal interactions, not approximate trainer spawn points.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Rat Master's Journal — Menagerie** (`world_actor`) — map `0.50024662, 0.4335252`; world `199028.58, 64395.582, 1491.29`; shared-coordinate group [S38](#s38)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_Journal_DR_RatMaster_Switchable': Quest booleans name the Rat, Hound, and Demon journals. The current world contains three switchable journal actors whose entry tags and localized display names correspond exactly to those booleans and guide locations.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/98CEFJS02ZFIXMI882V29OMSX.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Hound Master's Journal — Garrison kennels** (`world_actor`) — map `0.57380952, 0.34228571`; world `229925.0, 26075.0, -5825.0`; shared-coordinate group [S62](#s62)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_Journal_DR_Houndmaster_Switchable': Quest booleans name the Rat, Hound, and Demon journals. The current world contains three switchable journal actors whose entry tags and localized display names correspond exactly to those booleans and guide locations.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/EBDQ6YBM9RGDNL030IUWM2WAR.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Summoner's Journal — Cathedral crypt** (`world_actor`) — map `0.53705131, 0.40394001`; world `214486.55, 51969.805, -11412.8545`; shared-coordinate group [S50](#s50)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_Journal__DR_Demonmaster_Switchable': Quest booleans name the Rat, Hound, and Demon journals. The current world contains three switchable journal actors whose entry tags and localized display names correspond exactly to those booleans and guide locations.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/7UP1QMSO8MJYPWHH62UEDGUCA.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 3. Journal Pages Collected 1/3

- Objective key: `DR_SQ2_FindPages_1`
- Destination mode: `multiple`
- Confidence: **Medium (85/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: One journal is already collected, but the snapshot does not expose which one; all three possible remaining locations are shown.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Rat Master's Journal — Menagerie** (`world_actor`) — map `0.50024662, 0.4335252`; world `199028.58, 64395.582, 1491.29`; shared-coordinate group [S38](#s38)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_Journal_DR_RatMaster_Switchable': Journal collection order is free and the current progress counter alone cannot identify which exact journal interaction remains.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/98CEFJS02ZFIXMI882V29OMSX.json`
  - Marker confidence: 88/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Hound Master's Journal — Garrison kennels** (`world_actor`) — map `0.57380952, 0.34228571`; world `229925.0, 26075.0, -5825.0`; shared-coordinate group [S62](#s62)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_Journal_DR_Houndmaster_Switchable': Journal collection order is free and the current progress counter alone cannot identify which exact journal interaction remains.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/EBDQ6YBM9RGDNL030IUWM2WAR.json`
  - Marker confidence: 88/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Summoner's Journal — Cathedral crypt** (`world_actor`) — map `0.53705131, 0.40394001`; world `214486.55, 51969.805, -11412.8545`; shared-coordinate group [S50](#s50)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_Journal__DR_Demonmaster_Switchable': Journal collection order is free and the current progress counter alone cannot identify which exact journal interaction remains.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/7UP1QMSO8MJYPWHH62UEDGUCA.json`
  - Marker confidence: 88/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 4. Journal Pages Collected 2/3

- Objective key: `DR_SQ2_FindPages_2`
- Destination mode: `multiple`
- Confidence: **Medium (85/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: Two journals are already collected, but the snapshot does not expose which trainer remains; all possible locations are shown.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Rat Master's Journal — Menagerie** (`world_actor`) — map `0.50024662, 0.4335252`; world `199028.58, 64395.582, 1491.29`; shared-coordinate group [S38](#s38)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_Journal_DR_RatMaster_Switchable': Journal collection order is free and the current progress counter alone cannot identify the final journal interaction.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/98CEFJS02ZFIXMI882V29OMSX.json`
  - Marker confidence: 88/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Hound Master's Journal — Garrison kennels** (`world_actor`) — map `0.57380952, 0.34228571`; world `229925.0, 26075.0, -5825.0`; shared-coordinate group [S62](#s62)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_Journal_DR_Houndmaster_Switchable': Journal collection order is free and the current progress counter alone cannot identify the final journal interaction.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/EBDQ6YBM9RGDNL030IUWM2WAR.json`
  - Marker confidence: 88/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Summoner's Journal — Cathedral crypt** (`world_actor`) — map `0.53705131, 0.40394001`; world `214486.55, 51969.805, -11412.8545`; shared-coordinate group [S50](#s50)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_Journal__DR_Demonmaster_Switchable': Journal collection order is free and the current progress counter alone cannot identify the final journal interaction.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/7UP1QMSO8MJYPWHH62UEDGUCA.json`
  - Marker confidence: 88/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 5. Journal Pages Collected 3/3
Return to Vannaka

- Objective key: `DR_SQ2_ReturnToVannaka`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: `DR_SQ2_Vannaka` (revealed, `QF_DR_SQ2_AnimalLover.json` / `QFT_UpdateQuestLocation_C_12`)
- Marker 1: **Vannaka (Wanted! hand-in)** (`npc`) — map `0.52709762, 0.36161905`; world `210306.0, 34195.0, -4532.0`; shared-coordinate group [S46](#s46)
  - Determination: reviewed objective relationship to extracted game quest marker 'DR_SQ2_Vannaka': The 3/3 objective explicitly says Return to Vannaka, and its compiled QFT_SetQuestObjective directly reveals the quest-owned DR_SQ2_Vannaka marker.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

### What's Theirs is Mine

Side quest · Dowdun Reach

#### 1. Look for signs of the mugger-mage in the Cathedral in Dowdun Keep.

- Objective key: `DR_SQ1_Temple`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: `Doric_SQ1_NPC` (revealed, `QF_DR_SQ1_ThinkingWithPortals.json` / `QFT_UpdateQuestLocation_C_0`)
- Marker 1: **Scrawled Notes — Cathedral upper ledge** (`location`) — map `0.53481719, 0.40165669`; world `213548.22, 51010.81, -1336.4332`
  - Determination: reviewed objective relationship to extracted game registry landmark '1E6B02E6-471FCF0A-7DD0B08A-072FE962': The Cathedral branch advances by reading the Scrawled Notes on its upper ledge. The current world registry contains the exact quest-owned note identity at the Cathedral; Doric is the quest giver and must not be carried into this search state.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 2. Look for signs of the mugger-mage in the Garrisons of Dowdun Keep.

- Objective key: `DR_SQ1_Garrison`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Scrawled Notes — Garrison nodestone** (`location`) — map `0.57704033, 0.33264679`; world `231281.94, 22026.652, -3033.6943`
  - Determination: reviewed objective relationship to extracted game registry landmark '843B8CD4-4E5E1A28-684CC6A5-8AE67163': The Garrison branch advances by reading the schematics beside its nodestone. The registry identity is the exact placed Scrawled Notes interaction and is more precise than the Nexus portal previously used as a proxy.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 3. Look for signs of the mugger-mage in the underground Demon Gate of Dowdun Keep.

- Objective key: `DR_SQ1_Aquifer`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Dead Zamorak Mage — Demon Gate** (`location`) — map `0.5593621, 0.3636678`; world `223857.08, 35055.477, -17193.598`
  - Determination: reviewed objective relationship to extracted game registry landmark '40E5E9B9-4B732365-3BB67AAE-8B01D1EB': The Demon Gate branch advances by searching the dead Zamorak mage beyond the two shardbound demons. The registry exposes that exact quest-owned body, allowing the marker to replace the broader Aquifer portal proxy.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 4. Speak to Doric in the Dowdun Reach Nexus.

- Objective key: `DR_SQ1_ReturnToDoric`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Doric — The Nexus** (`npc`) — map `0.5693746, 0.41005833`; world `228062.33, 54539.5, -994.7146`
  - Determination: reviewed objective relationship to extracted game quest marker 'Doric_SQ1_NPC': The objective explicitly returns to Doric; this quest owns Doric_SQ1_NPC.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

### Withering Heights

Main quest · Fellhollow / Death's Domain

#### 1. Speak to the Wise Old Man near Ashien's Watch.

- Objective key: `TalkToWomNEGF`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: `WomGFExitTarget` (revealed, `QF_DragonSlayer_Beat5.json` / `QFT_UpdateQuestLocation_C_0`); `WomGFExitTarget` (revealed, `QF_WiseOldMan.json` / `QFT_UpdateQuestLocation_C_0`); `WomGFExitTarget` (revealed, `QF_DeathQuest_Beat1.json` / `QFT_UpdateQuestLocation_C_5`); `WomGFExitTarget` (revealed, `QF_ProvideQuestOnStart.json` / `QFT_UpdateQuestLocation_C_0`)
- Marker 1: **Wise Old Man — Ashien's Watch** (`location`) — map `0.32243304, 0.42154587`; world `124346.875, 59364.266, 20160.379`
  - Determination: reviewed objective relationship to extracted game quest marker 'WomGFExitTarget': The opening objective and four current-build flow operations agree on WomGFExitTarget beside Ashien's Watch. Duplicate operations represent the same authored interaction, not four destinations.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 2. Unlock the strange door.

- Objective key: `UnlockGate`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: `WomGFExitTarget` (hidden/removed, `QF_DeathQuest_Beat1.json` / `QFT_UpdateQuestLocation_C_0`); `EnterDeathsDomainTarget` (revealed, `QF_DeathQuest_Beat1.json` / `QFT_UpdateQuestLocation_C_1`)
- Marker 1: **Ominous Door — Forgotten Temple entrance** (`location`) — map `0.34244367, 0.4021815`; world `132751.34, 51231.23, 20160.379`
  - Determination: reviewed objective relationship to extracted game quest marker 'EnterDeathsDomainTarget': After the Wise Old Man conversation, the flow hides WomGFExitTarget and reveals EnterDeathsDomainTarget at the Ominous Door. Death is beyond the door and is not the unlocking interaction.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 3. Speak to the temple's inhabitant.

- Objective key: `SearchTheStructure`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Display note: Death is inside the Forgotten Temple; this marker is the exact authored temple destination on the world map.
- Reviewed relationship: **yes**
- Cooked location operations: `EnterDeathsDomainTarget` (hidden/removed, `QF_DeathQuest_Beat1.json` / `QFT_UpdateQuestLocation_C_3`); `DeathsTempleArea` (revealed, `QF_DeathQuest_Beat1.json` / `QFT_UpdateQuestLocation_C_4`)
- Marker 1: **Death — Forgotten Temple** (`area`) — map `0.33398188, 0.36679069`; world `129197.39, 36367.09, 20160.379`; shared-coordinate group [S21](#s21)
  - Determination: reviewed objective relationship to extracted game quest marker 'DeathsTempleArea': The flow hides the entrance and reveals DeathsTempleArea for the search state, while the following objective explicitly speaks to Death at that same temple destination.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 4. Speak to Death.

- Objective key: `SpeakToMysteriousFigure`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Display note: Death is inside the Forgotten Temple; this marker is the exact authored temple destination on the world map.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Death — Forgotten Temple** (`area`) — map `0.33398188, 0.36679069`; world `129197.39, 36367.09, 20160.379`; shared-coordinate group [S21](#s21)
  - Determination: reviewed objective relationship to extracted game quest marker 'DeathsTempleArea': The flow hides the entrance and reveals DeathsTempleArea for the search state, while the following objective explicitly speaks to Death at that same temple destination.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 5. Speak to Postie Pete.

- Objective key: `SpeakToPostiePete`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: `DeathsTempleArea` (hidden/removed, `QF_DeathQuest_Beat1.json` / `QFT_UpdateQuestLocation_C_6`); `NPCDeath` (revealed, `QF_DeathQuest_Beat1.json` / `QFT_UpdateQuestLocation_C_7`); `PostiePeteTarget` (revealed, `QF_DeathQuest_Beat1.json` / `QFT_UpdateQuestLocation_C_8`)
- Marker 1: **Postie Pete — Forgotten Temple** (`location`) — map `0.34258817, 0.33918205`; world `132812.03, 24771.459, -4715.578`; shared-coordinate group [S25](#s25)
  - Determination: reviewed objective relationship to extracted game registry landmark '354D8FA4-4054FF1B-FE04DA81-EEF1DFFD': This story beat occurs beside the Forgotten Temple exit. Of the two current Postie Pete registry identities, 354D8FA4 is colocated with the quest-owned PostiePeteTarget; the other is the later Dowdun Reach placement.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 6. Enter Fellhollow.

- Objective key: `GoThroughDoorToFH`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: `EnterFHArea` (revealed, `QF_PostiePete.json` / `QFT_UpdateQuestLocation_C_2`); `PostiePeteTarget` (hidden/removed, `QF_PostiePete.json` / `QFT_UpdateQuestLocation_C_8`)
- Marker 1: **Fellhollow crossing** (`area`) — map `0.33844531, 0.29857729`; world `131072.03, 7717.4634, 20160.379`
  - Determination: reviewed objective relationship to extracted game quest marker 'EnterFHArea': The Postie Pete state is complete when the flow hides his target and reveals EnterFHArea at the wither-water crossing into Fellhollow.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 7. Remove the Soul Rift at a Purification Pool.

- Objective key: `FindACleansingPool`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: `EnterFHArea` (hidden/removed, `QF_DeathQuest_Beat2.json` / `QFT_UpdateQuestLocation_C_3`); `CleansingPoolTarget` (revealed, `QF_DeathQuest_Beat2.json` / `QFT_UpdateQuestLocation_C_4`); `PostiePeteTarget` (hidden/removed, `QF_DeathQuest_Beat2.json` / `QFT_UpdateQuestLocation_C_9`)
- Marker 1: **Purification Pool — Fellhollow entry** (`location`) — map `0.33524136, 0.25936008`; world `129726.37, -8753.765, -1119.6908`
  - Determination: reviewed objective relationship to extracted game quest marker 'CleansingPoolTarget': On entering Fellhollow, the flow hides EnterFHArea and reveals CleansingPoolTarget. This is the exact first Purification Pool used to remove Soul Rift.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 8. Find Cathan and look into what he was investigating.

- Objective key: `InvestigateStandingStones`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: Speak to Cathan on the Bleakfields cliff, then examine all three nearby standing stones to reveal the three curse-fragment routes.
- Reviewed relationship: **yes**
- Cooked location operations: `StandingStonesArea` (revealed, `QF_DeathQuest_Beat3.json` / `QFT_UpdateQuestLocation_C_1`); `CleansingPoolTarget` (hidden/removed, `QF_DeathQuest_Beat3.json` / `QFT_UpdateQuestLocation_C_5`)
- Marker 1: **Cathan — Bleakfields cliff** (`npc`) — map `0.35830719, 0.23536819`; world `139414.02, -18830.361, -6075.971`
  - Determination: reviewed objective relationship to extracted game npc 'ghost-of-cathan' placement '3f56e42c6827459a3d9b': The nearest Ghost of Cathan placement is only 760 world units from the authored StandingStonesArea, and the current registry contains exactly three Standing Stone identities inside that same area. Quest flow and guide sequencing require Cathan followed by all three clues.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Standing Stone — first curse clue** (`location`) — map `0.3545681, 0.23334665`; world `137843.6, -19679.408, -6049.2114`; shared-coordinate group [S26](#s26)
  - Determination: reviewed objective relationship to extracted game registry landmark '727378D6-40266062-E871B38D-1F069AA6': The nearest Ghost of Cathan placement is only 760 world units from the authored StandingStonesArea, and the current registry contains exactly three Standing Stone identities inside that same area. Quest flow and guide sequencing require Cathan followed by all three clues.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Standing Stone — second curse clue** (`location`) — map `0.36072019, 0.23483984`; world `140427.48, -19052.268, -5998.4795`; shared-coordinate group [S28](#s28)
  - Determination: reviewed objective relationship to extracted game registry landmark '3509DF90-4B8870A6-F647E082-800F2459': The nearest Ghost of Cathan placement is only 760 world units from the authored StandingStonesArea, and the current registry contains exactly three Standing Stone identities inside that same area. Quest flow and guide sequencing require Cathan followed by all three clues.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **Standing Stone — third curse clue** (`location`) — map `0.35814121, 0.2327137`; world `139344.31, -19945.248, -6061.522`; shared-coordinate group [S27](#s27)
  - Determination: reviewed objective relationship to extracted game registry landmark '80C5DC0C-4E6152BC-DEFD5395-B174BA49': The nearest Ghost of Cathan placement is only 760 world units from the authored StandingStonesArea, and the current registry contains exactly three Standing Stone identities inside that same area. Quest flow and guide sequencing require Cathan followed by all three clues.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 9. Get location of curse fragments 0/3.

- Objective key: `ReachedStandingStones`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: Read all three nearby stones; they reveal the village, waterfall, and fortress fragment routes.
- Reviewed relationship: **yes**
- Cooked location operations: `StandingStonesArea` (hidden/removed, `QF_DeathQuest_Beat4.json` / `QFT_UpdateQuestLocation_C_4`); `StandStoneVillage` (revealed, `QF_DeathQuest_Beat4.json` / `QFT_UpdateQuestLocation_C_5`); `StandStoneWaterfall` (revealed, `QF_DeathQuest_Beat4.json` / `QFT_UpdateQuestLocation_C_6`); `StandStoneFortress` (revealed, `QF_DeathQuest_Beat4.json` / `QFT_UpdateQuestLocation_C_7`)
- Marker 1: **Standing Stone — village clue** (`location`) — map `0.3545681, 0.23334665`; world `137843.6, -19679.408, -6049.2114`; shared-coordinate group [S26](#s26)
  - Determination: reviewed objective relationship to extracted game registry landmark '727378D6-40266062-E871B38D-1F069AA6': The current flow reveals exactly StandStoneVillage, StandStoneWaterfall, and StandStoneFortress for the 0/3 state. Their duplicate candidates are repeated flow references to the same three authored locations.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Standing Stone — waterfall clue** (`location`) — map `0.36072019, 0.23483984`; world `140427.48, -19052.268, -5998.4795`; shared-coordinate group [S28](#s28)
  - Determination: reviewed objective relationship to extracted game registry landmark '3509DF90-4B8870A6-F647E082-800F2459': The current flow reveals exactly StandStoneVillage, StandStoneWaterfall, and StandStoneFortress for the 0/3 state. Their duplicate candidates are repeated flow references to the same three authored locations.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Standing Stone — fortress clue** (`location`) — map `0.35814121, 0.2327137`; world `139344.31, -19945.248, -6061.522`; shared-coordinate group [S27](#s27)
  - Determination: reviewed objective relationship to extracted game registry landmark '80C5DC0C-4E6152BC-DEFD5395-B174BA49': The current flow reveals exactly StandStoneVillage, StandStoneWaterfall, and StandStoneFortress for the 0/3 state. Their duplicate candidates are repeated flow references to the same three authored locations.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 10. Get location of curse fragments 1/3.

- Objective key: `InteractedWithStone1`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: The aggregate count does not reveal which standing stones have been read, so all three exact clues remain visible.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Standing Stone — village clue** (`location`) — map `0.3545681, 0.23334665`; world `137843.6, -19679.408, -6049.2114`; shared-coordinate group [S26](#s26)
  - Determination: reviewed objective relationship to extracted game registry landmark '727378D6-40266062-E871B38D-1F069AA6': These are progressed counter states of the same three-stone interaction established by the preceding objective; the current registry contains exactly those three stones in the authored area.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Standing Stone — waterfall clue** (`location`) — map `0.36072019, 0.23483984`; world `140427.48, -19052.268, -5998.4795`; shared-coordinate group [S28](#s28)
  - Determination: reviewed objective relationship to extracted game registry landmark '3509DF90-4B8870A6-F647E082-800F2459': These are progressed counter states of the same three-stone interaction established by the preceding objective; the current registry contains exactly those three stones in the authored area.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Standing Stone — fortress clue** (`location`) — map `0.35814121, 0.2327137`; world `139344.31, -19945.248, -6061.522`; shared-coordinate group [S27](#s27)
  - Determination: reviewed objective relationship to extracted game registry landmark '80C5DC0C-4E6152BC-DEFD5395-B174BA49': These are progressed counter states of the same three-stone interaction established by the preceding objective; the current registry contains exactly those three stones in the authored area.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 11. Get location of curse fragments 2/3.

- Objective key: `InteractedWithStone2`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: The aggregate count does not reveal which standing stones have been read, so all three exact clues remain visible.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Standing Stone — village clue** (`location`) — map `0.3545681, 0.23334665`; world `137843.6, -19679.408, -6049.2114`; shared-coordinate group [S26](#s26)
  - Determination: reviewed objective relationship to extracted game registry landmark '727378D6-40266062-E871B38D-1F069AA6': These are progressed counter states of the same three-stone interaction established by the preceding objective; the current registry contains exactly those three stones in the authored area.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Standing Stone — waterfall clue** (`location`) — map `0.36072019, 0.23483984`; world `140427.48, -19052.268, -5998.4795`; shared-coordinate group [S28](#s28)
  - Determination: reviewed objective relationship to extracted game registry landmark '3509DF90-4B8870A6-F647E082-800F2459': These are progressed counter states of the same three-stone interaction established by the preceding objective; the current registry contains exactly those three stones in the authored area.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Standing Stone — fortress clue** (`location`) — map `0.35814121, 0.2327137`; world `139344.31, -19945.248, -6061.522`; shared-coordinate group [S27](#s27)
  - Determination: reviewed objective relationship to extracted game registry landmark '80C5DC0C-4E6152BC-DEFD5395-B174BA49': These are progressed counter states of the same three-stone interaction established by the preceding objective; the current registry contains exactly those three stones in the authored area.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 12. Gather three curse fragments from the locations described in the standing stones.

- Objective key: `BreakTheThreeSeals`
- Destination mode: `multiple`
- Confidence: **High (94/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: The fragments can be collected in flexible order and the aggregate objective does not identify which remain. All three destinations stay visible: Emberwood Village, Vekchenven Kara beneath the waterfall, and Silverthorn Keep.
- Reviewed relationship: **yes**
- Cooked location operations: `VillageArea` (revealed, `QF_DeathQuest_Beat4.json` / `QFT_UpdateQuestLocation_C_11`); `FortressArea` (revealed, `QF_DeathQuest_Beat4.json` / `QFT_UpdateQuestLocation_C_12`); `FallArea` (revealed, `QF_DeathQuest_Beat4.json` / `QFT_UpdateQuestLocation_C_13`)
- Marker 1: **Curse Fragment of Grief — Emberwood Village** (`area`) — map `0.17002493, 0.14765352`; world `60335.47, -55670.52, 20160.379`; shared-coordinate group [S11](#s11)
  - Determination: reviewed objective relationship to extracted game quest marker 'VillageArea': The three standing-stone clues resolve to grief in Emberwood Village, rage inside Vekchenven Kara at the Lake of Lost Souls waterfall, and despair inside Silverthorn Keep. Dungeon objectives use their authored overworld entrances.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 97/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Curse Fragment of Rage — Vekchenven Kara entrance** (`dungeon_entrance`) — map `0.36963329, 0.14279896`; world `144170.98, -57709.438, -11778.527`; shared-coordinate group [S30](#s30)
  - Determination: reviewed objective relationship to extracted game dungeon entrance 'VekchenvenKara': The three standing-stone clues resolve to grief in Emberwood Village, rage inside Vekchenven Kara at the Lake of Lost Souls waterfall, and despair inside Silverthorn Keep. Dungeon objectives use their authored overworld entrances.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 97/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Curse Fragment of Despair — Silverthorn Keep entrance** (`dungeon_entrance`) — map `0.55772786, 0.24067574`; world `223170.7, -16601.19, -4996.3745`; shared-coordinate group [S56](#s56)
  - Determination: reviewed objective relationship to extracted game dungeon entrance 'SilverthornKeep': The three standing-stone clues resolve to grief in Emberwood Village, rage inside Vekchenven Kara at the Lake of Lost Souls waterfall, and despair inside Silverthorn Keep. Dungeon objectives use their authored overworld entrances.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 97/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 13. Gather two more curse fragments from the locations described in the standing stones.

- Objective key: `BreakRemainingtwoSeals`
- Destination mode: `multiple`
- Confidence: **High (94/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: The fragments can be collected in flexible order and the aggregate objective does not identify which remain. All three destinations stay visible: Emberwood Village, Vekchenven Kara beneath the waterfall, and Silverthorn Keep.
- Reviewed relationship: **yes**
- Cooked location operations: `VillageArea` (hidden/removed, `QF_DeathQuest_Beat4.json` / `QFT_UpdateQuestLocation_C_30`); `FortressArea` (hidden/removed, `QF_DeathQuest_Beat4.json` / `QFT_UpdateQuestLocation_C_32`); `FallArea` (hidden/removed, `QF_DeathQuest_Beat4.json` / `QFT_UpdateQuestLocation_C_35`)
- Marker 1: **Curse Fragment of Grief — Emberwood Village** (`area`) — map `0.17002493, 0.14765352`; world `60335.47, -55670.52, 20160.379`; shared-coordinate group [S11](#s11)
  - Determination: reviewed objective relationship to extracted game quest marker 'VillageArea': The three standing-stone clues resolve to grief in Emberwood Village, rage inside Vekchenven Kara at the Lake of Lost Souls waterfall, and despair inside Silverthorn Keep. Dungeon objectives use their authored overworld entrances.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 97/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Curse Fragment of Rage — Vekchenven Kara entrance** (`dungeon_entrance`) — map `0.36963329, 0.14279896`; world `144170.98, -57709.438, -11778.527`; shared-coordinate group [S30](#s30)
  - Determination: reviewed objective relationship to extracted game dungeon entrance 'VekchenvenKara': The three standing-stone clues resolve to grief in Emberwood Village, rage inside Vekchenven Kara at the Lake of Lost Souls waterfall, and despair inside Silverthorn Keep. Dungeon objectives use their authored overworld entrances.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 97/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Curse Fragment of Despair — Silverthorn Keep entrance** (`dungeon_entrance`) — map `0.55772786, 0.24067574`; world `223170.7, -16601.19, -4996.3745`; shared-coordinate group [S56](#s56)
  - Determination: reviewed objective relationship to extracted game dungeon entrance 'SilverthornKeep': The three standing-stone clues resolve to grief in Emberwood Village, rage inside Vekchenven Kara at the Lake of Lost Souls waterfall, and despair inside Silverthorn Keep. Dungeon objectives use their authored overworld entrances.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 97/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 14. Find the last curse fragment from the locations described in the standing stones.

- Objective key: `BreakTheFinalSeal`
- Destination mode: `multiple`
- Confidence: **High (94/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: The fragments can be collected in flexible order and the aggregate objective does not identify which remain. All three destinations stay visible: Emberwood Village, Vekchenven Kara beneath the waterfall, and Silverthorn Keep.
- Reviewed relationship: **yes**
- Cooked location operations: `VillageArea` (hidden/removed, `QF_DeathQuest_Beat4.json` / `QFT_UpdateQuestLocation_C_29`); `FortressArea` (hidden/removed, `QF_DeathQuest_Beat4.json` / `QFT_UpdateQuestLocation_C_33`); `FallArea` (hidden/removed, `QF_DeathQuest_Beat4.json` / `QFT_UpdateQuestLocation_C_36`)
- Marker 1: **Curse Fragment of Grief — Emberwood Village** (`area`) — map `0.17002493, 0.14765352`; world `60335.47, -55670.52, 20160.379`; shared-coordinate group [S11](#s11)
  - Determination: reviewed objective relationship to extracted game quest marker 'VillageArea': The three standing-stone clues resolve to grief in Emberwood Village, rage inside Vekchenven Kara at the Lake of Lost Souls waterfall, and despair inside Silverthorn Keep. Dungeon objectives use their authored overworld entrances.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 97/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Curse Fragment of Rage — Vekchenven Kara entrance** (`dungeon_entrance`) — map `0.36963329, 0.14279896`; world `144170.98, -57709.438, -11778.527`; shared-coordinate group [S30](#s30)
  - Determination: reviewed objective relationship to extracted game dungeon entrance 'VekchenvenKara': The three standing-stone clues resolve to grief in Emberwood Village, rage inside Vekchenven Kara at the Lake of Lost Souls waterfall, and despair inside Silverthorn Keep. Dungeon objectives use their authored overworld entrances.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 97/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Curse Fragment of Despair — Silverthorn Keep entrance** (`dungeon_entrance`) — map `0.55772786, 0.24067574`; world `223170.7, -16601.19, -4996.3745`; shared-coordinate group [S56](#s56)
  - Determination: reviewed objective relationship to extracted game dungeon entrance 'SilverthornKeep': The three standing-stone clues resolve to grief in Emberwood Village, rage inside Vekchenven Kara at the Lake of Lost Souls waterfall, and despair inside Silverthorn Keep. Dungeon objectives use their authored overworld entrances.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 97/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 15. Enter the Shattered Spire and defeat Imaru.

- Objective key: `ChallangeImaru`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: `SlayImaruTarget` (revealed, `QF_DeathQuest_Beat4.json` / `QFT_UpdateQuestLocation_C_0`); `SlayImaruTarget` (revealed, `QF_DeathQuest_Beat4.json` / `QFT_UpdateQuestLocation_C_1`); `SlayImaruTarget` (revealed, `QF_DeathQuest_Beat4.json` / `QFT_UpdateQuestLocation_C_3`); `VillageArea` (hidden/removed, `QF_DeathQuest_Beat4.json` / `QFT_UpdateQuestLocation_C_31`); `FortressArea` (hidden/removed, `QF_DeathQuest_Beat4.json` / `QFT_UpdateQuestLocation_C_34`); `FallArea` (hidden/removed, `QF_DeathQuest_Beat4.json` / `QFT_UpdateQuestLocation_C_37`)
- Marker 1: **Imaru — Shattered Spire** (`location`) — map `0.34013119, 0.14219912`; world `131780.1, -57961.37, 20160.379`
  - Determination: reviewed objective relationship to extracted game quest marker 'SlayImaruTarget': Once all fragments are collected, the flow reveals SlayImaruTarget at the Shattered Spire and hides all three fragment areas. Silverthorn Keep is a stale inferred dungeon candidate and is not part of this objective.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 16. Bring Imaru's head to Death.

- Objective key: `ReturnToDeath`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: `DeathTempleTarget` (revealed, `QF_DeathQuest_Beat5.json` / `QFT_UpdateQuestLocation_C_0`); `SlayImaruTarget` (hidden/removed, `QF_DeathQuest_Beat5.json` / `QFT_UpdateQuestLocation_C_1`); `NPCDeath` (hidden/removed, `QF_DeathQuest_Beat5.json` / `QFT_UpdateQuestLocation_C_3`)
- Marker 1: **Death — Forgotten Temple** (`location`) — map `0.3291706, 0.36396822`; world `127176.65, 35181.652, 20160.379`
  - Determination: reviewed objective relationship to extracted game quest marker 'DeathTempleTarget': After Imaru is defeated, the flow hides SlayImaruTarget and reveals DeathTempleTarget for the head hand-in. Death's nearby actor representations are duplicates of this authored destination.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

### Doric's Quest

Side quest · Fellhollow

#### 1. Craft an Undead Ranger's Bow and bring it to Doric in Fellhollow. Doric believes the recipe is in a vault somewhere.

- Objective key: `FindSkeletonShortbow`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: The Undead Ranger's Bow recipe is inside Skekven Kara; the overworld map marks the vault entrance. Craft the bow, then return it to Doric.
- Reviewed relationship: **yes**
- Cooked location operations: `DoricNPC` (revealed, `QF_FH_CollectorsCalling.json` / `QFT_UpdateQuestLocation_C_3`); `DoricNPC` (revealed, `QF_FH_CollectorsCalling.json` / `QFT_UpdateQuestLocation_C_4`)
- Marker 1: **Undead Ranger's Bow — Skekven Kara entrance** (`dungeon_entrance`) — map `0.49727614, 0.17935909`; world `197780.98, -42354.184, -12428.552`; shared-coordinate group [S36](#s36)
  - Determination: reviewed objective relationship to extracted game dungeon entrance 'SkekvenKara': Independent walkthroughs identify the Dragonkin Effigy inside Skekven Kara as the fixed recipe source. The current build supplies the exact Skekven Kara entrance and this quest's DoricNPC hand-in marker; unrelated global Doric phase copies are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/6TBAA5Q9QN8OQMFB8O52E5TBI.json`
  - Marker confidence: 97/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Doric — bow hand-in** (`npc`) — map `0.48839286, 0.17934524`; world `194050.0, -42360.0, -12610.0`; shared-coordinate group [S35](#s35)
  - Determination: reviewed objective relationship to extracted game quest marker 'DoricNPC': Independent walkthroughs identify the Dragonkin Effigy inside Skekven Kara as the fixed recipe source. The current build supplies the exact Skekven Kara entrance and this quest's DoricNPC hand-in marker; unrelated global Doric phase copies are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 2. Craft a Shadow Sword and bring it to Doric in Fellhollow. Doric believes a zogre has the recipe.

- Objective key: `FindShadowSword`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: The Shadow Sword recipe is a rare Zogre drop. Current standard Fellhollow Zogre spawns are shown together with Doric's hand-in location.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Fellhollow Zogre spawn** (`npc`) — map `0.37631767, 0.12607005`; world `146978.42, -64735.58, -7339.342`
  - Determination: reviewed objective relationship to extracted game npc 'zogre': The walkthrough identifies Zogres in Fellhollow as the recipe source, including the Vekchenven Kara boss; the standard extracted Zogre placements cover the overworld hunt while the separate vault entrance is included for the interior boss route.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Fellhollow Zogre spawn** (`npc`) — map `0.20735412, 0.17061354`; world `76013.73, -46027.312, -6609.108`
  - Determination: reviewed objective relationship to extracted game npc 'zogre': The walkthrough identifies Zogres in Fellhollow as the recipe source, including the Vekchenven Kara boss; the standard extracted Zogre placements cover the overworld hunt while the separate vault entrance is included for the interior boss route.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Zogre boss — Vekchenven Kara entrance** (`dungeon_entrance`) — map `0.36963329, 0.14279896`; world `144170.98, -57709.438, -11778.527`; shared-coordinate group [S30](#s30)
  - Determination: reviewed objective relationship to extracted game dungeon entrance 'VekchenvenKara': The walkthrough identifies Zogres in Fellhollow as the recipe source, including the Vekchenven Kara boss; the standard extracted Zogre placements cover the overworld hunt while the separate vault entrance is included for the interior boss route.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 97/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **Doric — sword hand-in** (`npc`) — map `0.48839286, 0.17934524`; world `194050.0, -42360.0, -12610.0`; shared-coordinate group [S35](#s35)
  - Determination: reviewed objective relationship to extracted game quest marker 'DoricNPC': The walkthrough identifies Zogres in Fellhollow as the recipe source, including the Vekchenven Kara boss; the standard extracted Zogre placements cover the overworld hunt while the separate vault entrance is included for the interior boss route.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 3. Craft a Necromancer's Staff and bring it to Doric in Fellhollow. Doric believes the recipe is in a vault somewhere.

- Objective key: `FindNecromancerStaff`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: The Necromancer's Staff recipe is inside Vekchenven Kara; the overworld map marks its entrance. Craft the staff, then return it to Doric.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Necromancer's Staff — Vekchenven Kara entrance** (`dungeon_entrance`) — map `0.36963329, 0.14279896`; world `144170.98, -57709.438, -11778.527`; shared-coordinate group [S30](#s30)
  - Determination: reviewed objective relationship to extracted game dungeon entrance 'VekchenvenKara': The walkthrough identifies the fixed dragonkin-effigy recipe in Vekchenven Kara. The current world supplies that vault entrance and the quest-owned Doric hand-in marker.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 97/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Doric — staff hand-in** (`npc`) — map `0.48839286, 0.17934524`; world `194050.0, -42360.0, -12610.0`; shared-coordinate group [S35](#s35)
  - Determination: reviewed objective relationship to extracted game quest marker 'DoricNPC': The walkthrough identifies the fixed dragonkin-effigy recipe in Vekchenven Kara. The current world supplies that vault entrance and the quest-owned Doric hand-in marker.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 4. Speak to Doric in Fellhollow.

- Objective key: `SpeakToDoric`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Doric — Fellhollow** (`npc`) — map `0.48839286, 0.17934524`; world `194050.0, -42360.0, -12610.0`; shared-coordinate group [S35](#s35)
  - Determination: reviewed objective relationship to extracted game quest marker 'DoricNPC': The objective explicitly names Doric in Fellhollow and DoricNPC is the exact quest-owned interaction target.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

### Even More Restless Ghosts

Side quest · Fellhollow

#### 1. Find ghosts and complete their unfinished business. (0/3)

- Objective key: `FindGhosts0/3`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: The three spirits can be completed in any order. The sketchbook marker is the authored retrieval interaction for the paint-spattered spirit; return to that spirit afterward.
- Reviewed relationship: **yes**
- Cooked location operations: `ZanikNPC` (revealed, `QF_FH_GhostQuest.json` / `QFT_UpdateQuestLocation_C_0`); `ZanikNPC` (revealed, `QF_FH_GhostQuest.json` / `QFT_UpdateQuestLocation_C_1`); `ZanikNPC` (revealed, `QF_FH_GhostQuest.json` / `QFT_UpdateQuestLocation_C_2`); `ZanikNPC` (revealed, `QF_FH_GhostQuest.json` / `QFT_UpdateQuestLocation_C_3`)
- Marker 1: **Windswept Ghost** (`location`) — map `0.1530686, 0.12794853`; world `53213.81, -63946.617, -5322.3325`; shared-coordinate group [S10](#s10)
  - Determination: reviewed objective relationship to extracted game registry landmark '22DE00B8-4CB15EF4-F101619F-E8B85D27': The three extracted Ghost landmarks align with the Emberwood house, Hope's Fall gate, and Fellhollow paint-spattered-spirit locations. The current world also places the quest-specific BP_NPC_FHGhostQuest_Sketchbook actor used by the third spirit's flow.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Reminiscing Ghost** (`location`) — map `0.33686819, 0.10300426`; world `130409.64, -74423.21, -6088.3696`; shared-coordinate group [S23](#s23)
  - Determination: reviewed objective relationship to extracted game registry landmark '8C8F2DE3-4365929A-14A90B8F-3484376E': The three extracted Ghost landmarks align with the Emberwood house, Hope's Fall gate, and Fellhollow paint-spattered-spirit locations. The current world also places the quest-specific BP_NPC_FHGhostQuest_Sketchbook actor used by the third spirit's flow.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Paint-Spattered Ghost** (`location`) — map `0.46599245, 0.10918731`; world `184641.83, -71826.33, -11668.955`; shared-coordinate group [S32](#s32)
  - Determination: reviewed objective relationship to extracted game registry landmark 'EB81888F-401988D1-26F7B1AD-CB140DF0': The three extracted Ghost landmarks align with the Emberwood house, Hope's Fall gate, and Fellhollow paint-spattered-spirit locations. The current world also places the quest-specific BP_NPC_FHGhostQuest_Sketchbook actor used by the third spirit's flow.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **Dust-covered Sketchbook** (`world_actor`) — map `0.51465619, 0.08953525`; world `205080.6, -80080.195, -9830.05`; shared-coordinate group [S44](#s44)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_NPC_FHGhostQuest_Sketchbook': The three extracted Ghost landmarks align with the Emberwood house, Hope's Fall gate, and Fellhollow paint-spattered-spirit locations. The current world also places the quest-specific BP_NPC_FHGhostQuest_Sketchbook actor used by the third spirit's flow.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/495PCPB53E80MXZ4BTLDO890O.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 2. Find ghosts and complete their unfinished business. (1/3)

- Objective key: `FindGhosts1/3`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: The counter does not identify which spirit is complete, so all three possible spirit locations and the quest-specific sketchbook retrieval remain visible.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Windswept Ghost** (`location`) — map `0.1530686, 0.12794853`; world `53213.81, -63946.617, -5322.3325`; shared-coordinate group [S10](#s10)
  - Determination: reviewed objective relationship to extracted game registry landmark '22DE00B8-4CB15EF4-F101619F-E8B85D27': The quest exposes only an aggregate counter while the cooked flow tracks the three spirit branches independently.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Reminiscing Ghost** (`location`) — map `0.33686819, 0.10300426`; world `130409.64, -74423.21, -6088.3696`; shared-coordinate group [S23](#s23)
  - Determination: reviewed objective relationship to extracted game registry landmark '8C8F2DE3-4365929A-14A90B8F-3484376E': The quest exposes only an aggregate counter while the cooked flow tracks the three spirit branches independently.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Paint-Spattered Ghost** (`location`) — map `0.46599245, 0.10918731`; world `184641.83, -71826.33, -11668.955`; shared-coordinate group [S32](#s32)
  - Determination: reviewed objective relationship to extracted game registry landmark 'EB81888F-401988D1-26F7B1AD-CB140DF0': The quest exposes only an aggregate counter while the cooked flow tracks the three spirit branches independently.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **Dust-covered Sketchbook** (`world_actor`) — map `0.51465619, 0.08953525`; world `205080.6, -80080.195, -9830.05`; shared-coordinate group [S44](#s44)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_NPC_FHGhostQuest_Sketchbook': The quest exposes only an aggregate counter while the cooked flow tracks the three spirit branches independently.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/495PCPB53E80MXZ4BTLDO890O.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 3. Find ghosts and complete their unfinished business. (2/3)

- Objective key: `FindGhosts2/3`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: One spirit remains, but the aggregate counter does not identify it; all possible spirit locations and the sketchbook retrieval are shown.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Windswept Ghost** (`location`) — map `0.1530686, 0.12794853`; world `53213.81, -63946.617, -5322.3325`; shared-coordinate group [S10](#s10)
  - Determination: reviewed objective relationship to extracted game registry landmark '22DE00B8-4CB15EF4-F101619F-E8B85D27': The quest exposes only an aggregate counter while the cooked flow tracks the three spirit branches independently.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Reminiscing Ghost** (`location`) — map `0.33686819, 0.10300426`; world `130409.64, -74423.21, -6088.3696`; shared-coordinate group [S23](#s23)
  - Determination: reviewed objective relationship to extracted game registry landmark '8C8F2DE3-4365929A-14A90B8F-3484376E': The quest exposes only an aggregate counter while the cooked flow tracks the three spirit branches independently.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Paint-Spattered Ghost** (`location`) — map `0.46599245, 0.10918731`; world `184641.83, -71826.33, -11668.955`; shared-coordinate group [S32](#s32)
  - Determination: reviewed objective relationship to extracted game registry landmark 'EB81888F-401988D1-26F7B1AD-CB140DF0': The quest exposes only an aggregate counter while the cooked flow tracks the three spirit branches independently.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **Dust-covered Sketchbook** (`world_actor`) — map `0.51465619, 0.08953525`; world `205080.6, -80080.195, -9830.05`; shared-coordinate group [S44](#s44)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_NPC_FHGhostQuest_Sketchbook': The quest exposes only an aggregate counter while the cooked flow tracks the three spirit branches independently.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/495PCPB53E80MXZ4BTLDO890O.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 4. Return to Zanik.

- Objective key: `BackToZanik`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Zanik** (`npc`) — map `0.39156857, 0.22961113`; world `153383.8, -21248.326, -1119.6909`; shared-coordinate group [S31](#s31)
  - Determination: reviewed objective relationship to extracted game quest marker 'ZanikNPC': The objective explicitly returns to Zanik and ZanikNPC is the quest-owned hand-in marker for this story state.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 5. Tell someone about these spirits. 

- Objective key: `BackToZanikNotSpokenToZanik`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **ZanikNPC** (`npc`) — map `0.39156857, 0.22961113`; world `153383.8, -21248.326, -1119.6909`; shared-coordinate group [S31](#s31)
  - Determination: reviewed objective relationship to extracted game quest marker 'ZanikNPC': The flow state is the pre-dialogue return to Zanik and the quest owns ZanikNPC.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

### Letters For The Dead

Side quest · Fellhollow

#### 1. Find and deliver letters for Postie Pete. 0/3

- Objective key: `ObjectiveOne`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: First recover Letter 1 from the Suspicious Skeleton, then deliver it to the postbox north-west of Vannaka's camp.
- Reviewed relationship: **yes**
- Cooked location operations: `PostiePeteTarget` (revealed, `QF_FH_LettersForTheDead.json` / `QFT_UpdateQuestLocation_C_0`)
- Marker 1: **Suspicious Skeleton (Letter 1)** (`world_actor`) — map `0.34873412, 0.29254132`; world `135393.33, 5182.354, -3575.9456`
  - Determination: reviewed objective relationship to extracted game world actor 'BP_NPC_Quest_FunnySkeleton': The cooked flow gives ITEM_quest_FH_LettersForTheDead_Letter_1 from the Suspicious Skeleton and sets TookLetter1. The matching placed postbox carries QuestFlow.Entry.FH_LettersForTheDead.PostBox1.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/58S1H0LFS081MPH4I8XX6PTN2.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Postbox 1 (Letter 1 delivery)** (`world_actor`) — map `0.30254174, 0.19173955`; world `115992.53, -37154.39, -7316.9395`
  - Determination: reviewed objective relationship to extracted game world actor 'BP_NPC_QuestPostBox_01': The cooked flow gives ITEM_quest_FH_LettersForTheDead_Letter_1 from the Suspicious Skeleton and sets TookLetter1. The matching placed postbox carries QuestFlow.Entry.FH_LettersForTheDead.PostBox1.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/DQ0NQ8F8YT8V5G4L0PMRLQZMN.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 2. Find and deliver letters for Postie Pete. 1/3

- Objective key: `ObjectiveTwo`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: Follow the spectral-leaf clue to the hanging letter south-east of Cathan's standing stones, then deliver it to PostBox2.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Spectral hanging letter (Letter 2)** (`world_actor`) — map `0.38257819, 0.24320312`; world `149607.84, -15539.69, -7168.662`
  - Determination: reviewed objective relationship to extracted game world actor 'BP_NPC_Quest_Letter': The placed Missing Letter actor enters QuestFlow.Entry.FH_LettersForTheDead.Letter1 and the flow awards Letter 2. The matching placed postbox carries QuestFlow.Entry.FH_LettersForTheDead.PostBox2.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/BPBH3XFDRKSPIRPPXM5GKTS0O.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Postbox 2 (Letter 2 delivery)** (`world_actor`) — map `0.35646757, 0.24327857`; world `138641.38, -15508.0, -6918.1396`
  - Determination: reviewed objective relationship to extracted game world actor 'BP_NPC_QuestPostBox_02': The placed Missing Letter actor enters QuestFlow.Entry.FH_LettersForTheDead.Letter1 and the flow awards Letter 2. The matching placed postbox carries QuestFlow.Entry.FH_LettersForTheDead.PostBox2.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/BPBH3XFDRKSPIRPPXM5GKTS0O.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 3. Find and deliver letters for Postie Pete. 2/3

- Objective key: `ObjectiveThree`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: Recover the final letter from Shaun and deliver it to PostBox3 outside Imaru's tower.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Shaun (Letter 3)** (`npc`) — map `0.31369269, 0.22304814`; world `120675.93, -24004.78, -7073.801`
  - Determination: reviewed objective relationship to extracted game npc 'shaun': Shaun is the uniquely placed BP_NPC_Quest_GhostSheep actor and his flow awards Letter 3. The matching placed postbox carries QuestFlow.Entry.FH_LettersForTheDead.PostBox3.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Postbox 3 (Letter 3 delivery)** (`world_actor`) — map `0.34478881, 0.13895244`; world `133736.3, -59324.977, -6853.7065`
  - Determination: reviewed objective relationship to extracted game world actor 'BP_NPC_QuestPostBox_03': Shaun is the uniquely placed BP_NPC_Quest_GhostSheep actor and his flow awards Letter 3. The matching placed postbox carries QuestFlow.Entry.FH_LettersForTheDead.PostBox3.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/CA7QEQ4QSQI81LC2TFJPKH91Q.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 4. Return to Postie Pete.

- Objective key: `ObjectiveReturn`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Postie Pete (Fellhollow)** (`npc`) — map `0.34258817, 0.33918205`; world `132812.03, 24771.459, -4715.578`; shared-coordinate group [S25](#s25)
  - Determination: reviewed objective relationship to extracted game npc 'postie-pete' placement 'd63ae2ad16b258bf984f': The quest begins and ends with the Fellhollow Postie Pete, whose placement is distinct from the Dowdun actor.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 5. Visit Postie Pete near the entrance to Fellhollow to begin searching for lost letters.

- Objective key: `ObjectiveZero`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Postie Pete (Fellhollow)** (`npc`) — map `0.34258817, 0.33918205`; world `132812.03, 24771.459, -4715.578`; shared-coordinate group [S25](#s25)
  - Determination: reviewed objective relationship to extracted game npc 'postie-pete' placement 'd63ae2ad16b258bf984f': The objective names Postie Pete near Fellhollow. The Fellhollow BP_NPC_PostiePete placement is distinct from the Dowdun Postie Pete actor.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

### Mirror Mirror

Side quest · Fellhollow

#### 1. Gather dragon blood, spectral silver thread and a ritual spellbook and bring them back to the mirror.

- Objective key: `FindSilverThread`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: Collect the spellbook from Emberwood Village, have Edna make the spectral silver thread, and obtain dragon blood from the nearby green dragon or any valid dragon wolf. Return all three reagents to the mirror.
- Reviewed relationship: **yes**
- Cooked location operations: `Magic Mirror` (revealed, `QF_FH_MirrorMirror.json` / `QFT_UpdateQuestLocation_C_1`); `Fellhollow Candle` (revealed, `QF_FH_MirrorMirror.json` / `QFT_UpdateQuestLocation_C_14`); `Bloodblight Candle` (revealed, `QF_FH_MirrorMirror.json` / `QFT_UpdateQuestLocation_C_15`); `Bramblemead Candle` (revealed, `QF_FH_MirrorMirror.json` / `QFT_UpdateQuestLocation_C_16`); `Fractured Candle` (revealed, `QF_FH_MirrorMirror.json` / `QFT_UpdateQuestLocation_C_17`); `Storm Candle` (revealed, `QF_FH_MirrorMirror.json` / `QFT_UpdateQuestLocation_C_18`); `Whispering Candle` (revealed, `QF_FH_MirrorMirror.json` / `QFT_UpdateQuestLocation_C_19`); `Magic Mirror` (revealed, `QF_FH_MirrorMirror.json` / `QFT_UpdateQuestLocation_C_9`)
- Marker 1: **Bedraggled Spellbook — Emberwood windmill** (`location`) — map `0.13952983, 0.13493128`; world `47527.53, -61013.863, -4881.821`
  - Determination: reviewed objective relationship to extracted game registry landmark 'AC5081C9-4B94236F-7ADE1F89-5459FBDB': The current quest registry exposes an exact Bedraggled Spellbook and mirror identity, while the NPC catalog contains one Edna quest actor and a green dragon only 9,106 world units from the mirror. Cooked dialogue identifies Edna as the thread crafter and permits dragon blood from either the nearby green dragon or dragon wolves.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Edna — spectral silver thread** (`npc`) — map `0.23185556, 0.1329634`; world `86304.336, -61840.37, -6654.637`
  - Determination: reviewed objective relationship to extracted game npc 'edna': The current quest registry exposes an exact Bedraggled Spellbook and mirror identity, while the NPC catalog contains one Edna quest actor and a green dragon only 9,106 world units from the mirror. Cooked dialogue identifies Edna as the thread crafter and permits dragon blood from either the nearby green dragon or dragon wolves.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Nearby Green Dragon — dragon blood** (`npc`) — map `0.11433493, 0.18158857`; world `36945.67, -41417.8, -6828.652`
  - Determination: reviewed objective relationship to extracted game npc 'green-dragon' placement '3b26ef1bc5f72aa2c841': The current quest registry exposes an exact Bedraggled Spellbook and mirror identity, while the NPC catalog contains one Edna quest actor and a green dragon only 9,106 world units from the mirror. Cooked dialogue identifies Edna as the thread crafter and permits dragon blood from either the nearby green dragon or dragon wolves.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **Magic Mirror — reagent hand-in** (`location`) — map `0.09267786, 0.18263629`; world `27849.7, -40977.76, -4970.749`; shared-coordinate group [S4](#s4)
  - Determination: reviewed objective relationship to extracted game registry landmark '7976CEBB-422FB482-F55B18A1-432B54A2': The current quest registry exposes an exact Bedraggled Spellbook and mirror identity, while the NPC catalog contains one Edna quest actor and a green dragon only 9,106 world units from the mirror. Cooked dialogue identifies Edna as the thread crafter and permits dragon blood from either the nearby green dragon or dragon wolves.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 2. Rekindle the memorial candles across Ashenfall.

- Objective key: `LightCandles`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: Light all six memorial candles: beside the mirror in Fellhollow; at the Fractured Plains lake; on the Stormtouched Highlands cliff; west of the Bloodblight Swamp Garou camp; beside the giant tree in Whispering Swamp; and on the Bramblemead clifftop south of Crasorak Kara.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Memorial Candle — Fellhollow mirror** (`location`) — map `0.09257863, 0.18298232`; world `27808.025, -40832.426, -4842.6646`
  - Determination: reviewed objective relationship to extracted game registry landmark '420BCC62-4A08DDD7-C39126A1-8D433769': The current world registry contains exactly six Candle identities owned by the Mirror Mirror string table. Each exact placement falls within its correspondingly named authored candle search area, and the guide descriptions independently distinguish the same six physical contexts.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Memorial Candle — Fractured Plains lake** (`location`) — map `0.29461008, 0.56881386`; world `112661.234, 121216.82, 258.02164`
  - Determination: reviewed objective relationship to extracted game registry landmark 'E4094A70-43E6966C-FA63549D-B9AC8B47': The current world registry contains exactly six Candle identities owned by the Mirror Mirror string table. Each exact placement falls within its correspondingly named authored candle search area, and the guide descriptions independently distinguish the same six physical contexts.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Memorial Candle — Stormtouched cliff** (`location`) — map `0.12348098, 0.44025948`; world `40787.01, 67223.98, 6136.3003`
  - Determination: reviewed objective relationship to extracted game registry landmark '43F5A15B-41F9C823-95EC228F-79246B19': The current world registry contains exactly six Candle identities owned by the Mirror Mirror string table. Each exact placement falls within its correspondingly named authored candle search area, and the guide descriptions independently distinguish the same six physical contexts.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **Memorial Candle — Bloodblight Swamp** (`location`) — map `0.29047496, 0.64431876`; world `110924.484, 152928.88, -2004.3497`
  - Determination: reviewed objective relationship to extracted game registry landmark '58A8D2F4-4F968512-D4B05C99-D14A27D1': The current world registry contains exactly six Candle identities owned by the Mirror Mirror string table. Each exact placement falls within its correspondingly named authored candle search area, and the guide descriptions independently distinguish the same six physical contexts.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 5: **Memorial Candle — Whispering Swamp tree** (`location`) — map `0.23381994, 0.63039405`; world `87129.375, 147080.5, -3620.3445`
  - Determination: reviewed objective relationship to extracted game registry landmark '0B63B5D7-4A7AC806-5D4EA99C-38CDA14C': The current world registry contains exactly six Candle identities owned by the Mirror Mirror string table. Each exact placement falls within its correspondingly named authored candle search area, and the guide descriptions independently distinguish the same six physical contexts.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 6: **Memorial Candle — Bramblemead clifftop** (`location`) — map `0.11017511, 0.74882738`; world `35198.547, 196822.5, 1500.4291`
  - Determination: reviewed objective relationship to extracted game registry landmark 'E6E3E03E-40259DC9-DAB0D6B7-96FE9125': The current world registry contains exactly six Candle identities owned by the Mirror Mirror string table. Each exact placement falls within its correspondingly named authored candle search area, and the guide descriptions independently distinguish the same six physical contexts.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 3. Speak to the mirror.

- Objective key: `ReturnToNPC`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Magic Mirror** (`location`) — map `0.09267786, 0.18263629`; world `27849.7, -40977.76, -4970.749`; shared-coordinate group [S4](#s4)
  - Determination: reviewed objective relationship to extracted game registry landmark '7976CEBB-422FB482-F55B18A1-432B54A2': The objective explicitly says to speak to the mirror; the registry contains one exact quest-owned Magic Mirror identity at the Fellhollow relay.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

### Seeking Salvation

Side quest · Fellhollow

#### 1. Access the vault and find the hidden secret within. 

- Objective key: `FindRecipe`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: Activate the four dragonkin seal statues around Doric's ruin, then enter Skekven Kara. The dwarven writing is inside, so the overworld map uses the vault entrance for that final interaction.
- Reviewed relationship: **yes**
- Cooked location operations: `DoricNPC` (revealed, `QF_FH_WitherWard.json` / `QFT_UpdateQuestLocation_C_0`)
- Marker 1: **Dragonkin seal statue 1** (`world_actor`) — map `0.48531005, 0.186395`; world `192755.22, -39399.1, -12447.557`
  - Determination: reviewed objective relationship to extracted game world actor 'BP_NPC_Vault_WitherWard_Statue_Puzzle1': The current world contains four separately placed BP_NPC_Vault_WitherWard_Statue_Puzzle actors surrounding the Skekven Kara entrance. The quest flow keeps them under FindRecipe, and the interior BP_NPC_Vault_WitherWard_Statue advances the dwarven-writing state; per the dungeon policy its public marker is the exact Skekven Kara entrance.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/6TBAA5Q9QN8OQMFB8O52E5TBI.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Dragonkin seal statue 2** (`world_actor`) — map `0.4848934, 0.1673144`; world `192580.23, -47412.953, -11358.888`
  - Determination: reviewed objective relationship to extracted game world actor 'BP_NPC_Vault_WitherWard_Statue_Puzzle2': The current world contains four separately placed BP_NPC_Vault_WitherWard_Statue_Puzzle actors surrounding the Skekven Kara entrance. The quest flow keeps them under FindRecipe, and the interior BP_NPC_Vault_WitherWard_Statue advances the dwarven-writing state; per the dungeon policy its public marker is the exact Skekven Kara entrance.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/6TBAA5Q9QN8OQMFB8O52E5TBI.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Dragonkin seal statue 3** (`world_actor`) — map `0.46167921, 0.19109721`; world `182830.27, -37424.17, -11504.246`
  - Determination: reviewed objective relationship to extracted game world actor 'BP_NPC_Vault_WitherWard_Statue_Puzzle3': The current world contains four separately placed BP_NPC_Vault_WitherWard_Statue_Puzzle actors surrounding the Skekven Kara entrance. The quest flow keeps them under FindRecipe, and the interior BP_NPC_Vault_WitherWard_Statue advances the dwarven-writing state; per the dungeon policy its public marker is the exact Skekven Kara entrance.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/6TBAA5Q9QN8OQMFB8O52E5TBI.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **Dragonkin seal statue 4** (`world_actor`) — map `0.48376271, 0.18067227`; world `192105.34, -41802.645, -10001.705`
  - Determination: reviewed objective relationship to extracted game world actor 'BP_NPC_Vault_WitherWard_Statue_Puzzle4': The current world contains four separately placed BP_NPC_Vault_WitherWard_Statue_Puzzle actors surrounding the Skekven Kara entrance. The quest flow keeps them under FindRecipe, and the interior BP_NPC_Vault_WitherWard_Statue advances the dwarven-writing state; per the dungeon policy its public marker is the exact Skekven Kara entrance.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/6TBAA5Q9QN8OQMFB8O52E5TBI.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 5: **Hidden dwarven writing — Skekven Kara entrance** (`dungeon_entrance`) — map `0.49727614, 0.17935909`; world `197780.98, -42354.184, -12428.552`; shared-coordinate group [S36](#s36)
  - Determination: reviewed objective relationship to extracted game dungeon entrance 'SkekvenKara': The current world contains four separately placed BP_NPC_Vault_WitherWard_Statue_Puzzle actors surrounding the Skekven Kara entrance. The quest flow keeps them under FindRecipe, and the interior BP_NPC_Vault_WitherWard_Statue advances the dwarven-writing state; per the dungeon policy its public marker is the exact Skekven Kara entrance.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/6TBAA5Q9QN8OQMFB8O52E5TBI.json`
  - Marker confidence: 97/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 2. Bring the dwarven writing to Doric. 

- Objective key: `ReturnToDoric`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Doric — Skekven Kara ruins** (`npc`) — map `0.4883775, 0.17938842`; world `194043.55, -42341.863, -1119.6909`; shared-coordinate group [S34](#s34)
  - Determination: reviewed objective relationship to extracted game quest marker 'DoricNPC': The objective explicitly returns the dwarven writing to Doric and DoricNPC is the quest-owned interaction target.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 3. Find someone in Fellhollow that can translate dwarvish text. 

- Objective key: `LookForDoric`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: `DoricNPC` (revealed, `QF_FH_WitherWard.json` / `QFT_UpdateQuestLocation_C_1`)
- Marker 1: **Doric — Skekven Kara ruins** (`npc`) — map `0.4883775, 0.17938842`; world `194043.55, -42341.863, -1119.6909`; shared-coordinate group [S34](#s34)
  - Determination: reviewed objective relationship to extracted game quest marker 'DoricNPC': The quest flow reveals DoricNPC for this state and the quest owns one exact Fellhollow Doric marker beside Skekven Kara. Generic name matching previously added Doric placements from unrelated regions.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

### The Wild Hunt

Side quest · Fellhollow

#### 1. Hunt down Nature Spirits near Fellhollow's willow groves and bring three of their hearts to Vannaka. 

- Objective key: `HuntBeast`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: The current build has five authored Nature Spirit spawn points. Hunt any three, then bring the Primordial Hearts to Vannaka.
- Reviewed relationship: **yes**
- Cooked location operations: `VannakaNPC` (revealed, `QF_FH_WitherBeast.json` / `QFT_UpdateQuestLocation_C_2`); `VannakaNPC` (revealed, `QF_FH_WitherBeast.json` / `QFT_UpdateQuestLocation_C_5`)
- Marker 1: **Nature Spirit — willow grove 1** (`world_actor`) — map `0.21950719, 0.25348068`; world `81118.02, -11223.115, -4993.6387`
  - Determination: reviewed objective relationship to extracted game world actor 'BP_SpawnPoint_WitherBeast': Five exact BP_SpawnPoint_WitherBeast actors are placed in the current world at willow groves. The objective accepts three hearts rather than prescribing three particular spirits, so every authored source is a valid destination and the quest-owned VannakaNPC marker is the hand-in.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/7HFZQX4NHQ0JCGSGY212UX6HF.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Nature Spirit — willow grove 2** (`world_actor`) — map `0.21818748, 0.13678684`; world `80563.74, -60234.527, -7002.16`
  - Determination: reviewed objective relationship to extracted game world actor 'BP_SpawnPoint_WitherBeast2': Five exact BP_SpawnPoint_WitherBeast actors are placed in the current world at willow groves. The objective accepts three hearts rather than prescribing three particular spirits, so every authored source is a valid destination and the quest-owned VannakaNPC marker is the hand-in.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/6OEJG9H7TJAHJFSWMID5EQ2BW.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Nature Spirit — willow grove 3** (`world_actor`) — map `0.09225951, 0.16788183`; world `27673.996, -47174.63, -6475.705`
  - Determination: reviewed objective relationship to extracted game world actor 'BP_SpawnPoint_WitherBeast3': Five exact BP_SpawnPoint_WitherBeast actors are placed in the current world at willow groves. The objective accepts three hearts rather than prescribing three particular spirits, so every authored source is a valid destination and the quest-owned VannakaNPC marker is the hand-in.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/6BGT67PHR1YE1829CV1W4YPLE.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **Nature Spirit — willow grove 4** (`world_actor`) — map `0.5174195, 0.17488821`; world `206241.19, -44231.95, -7337.9077`
  - Determination: reviewed objective relationship to extracted game world actor 'BP_SpawnPoint_WitherBeast4': Five exact BP_SpawnPoint_WitherBeast actors are placed in the current world at willow groves. The objective accepts three hearts rather than prescribing three particular spirits, so every authored source is a valid destination and the quest-owned VannakaNPC marker is the hand-in.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/1AEP5DGD1XXTXYOCR4YCRLQJO.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 5: **Nature Spirit — willow grove 5** (`world_actor`) — map `0.33156436, 0.08820701`; world `128182.03, -80638.055, -6305.8745`
  - Determination: reviewed objective relationship to extracted game world actor 'BP_SpawnPoint_WitherBeast5': Five exact BP_SpawnPoint_WitherBeast actors are placed in the current world at willow groves. The objective accepts three hearts rather than prescribing three particular spirits, so every authored source is a valid destination and the quest-owned VannakaNPC marker is the hand-in.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/8Z4BG0SXA1QGWOEU5G6EVQWII.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 6: **Vannaka — Primordial Heart hand-in** (`npc`) — map `0.32752136, 0.20833522`; world `126483.97, -30184.209, -1119.6909`; shared-coordinate group [S19](#s19)
  - Determination: reviewed objective relationship to extracted game quest marker 'VannakaNPC': Five exact BP_SpawnPoint_WitherBeast actors are placed in the current world at willow groves. The objective accepts three hearts rather than prescribing three particular spirits, so every authored source is a valid destination and the quest-owned VannakaNPC marker is the hand-in.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 2. Bring the hearts to Vannaka. 

- Objective key: `BringHeart`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Vannaka** (`npc`) — map `0.32752136, 0.20833522`; world `126483.97, -30184.209, -1119.6909`; shared-coordinate group [S19](#s19)
  - Determination: reviewed objective relationship to extracted game quest marker 'VannakaNPC': The objective explicitly brings the hearts to Vannaka and VannakaNPC is this quest's exact hand-in target.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 3. Bring the Primordial Heart to someone with an interest in the wild hunt. 

- Objective key: `BringHeartNotStarted`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: `VannakaNPC` (hidden/removed, `QF_FH_WitherBeast.json` / `QFT_UpdateQuestLocation_C_4`)
- Marker 1: **VannakaNPC** (`npc`) — map `0.32752136, 0.20833522`; world `126483.97, -30184.209, -1119.6909`; shared-coordinate group [S19](#s19)
  - Determination: reviewed objective relationship to extracted game quest marker 'VannakaNPC': The objective explicitly asks for someone interested in the Wild Hunt; the quest flow identifies VannakaNPC and the quest owns one exact VannakaNPC marker.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

### Things That Go Boom In The Night

Side quest · Fellhollow

#### 1. Gather 10 spectral chinchompa skulls and bring them to Vannaka in Fellhollow.

- Objective key: `FindSpectralChinchompaSkulls`
- Destination mode: `multiple`
- Confidence: **Medium (88/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains. The state exposes a large candidate set, reducing route precision.
- Display note: The current build contains 26 authored Spectral Chinchompa spawn points across Fellhollow. Gather ten skulls from any available spawns, then return to Vannaka.
- Reviewed relationship: **yes**
- Cooked location operations: `VannakaNPC` (revealed, `QF_FH_NightoftheSpectralBoom.json` / `QFT_UpdateQuestLocation_C_3`)
- Marker 1: **Spectral Chinchompa spawn** (`npc`) — map `0.56239429, 0.21364524`; world `225130.6, -27953.998, -5488.577`
  - Determination: reviewed objective relationship to extracted game npc 'spectral-chinchompa' actor type 'BP_SpawnPoint_SpectralChinchompa': The objective is a roaming enemy collection task rather than a single Vannaka interaction. Every published hunt point resolves from a placed BP_SpawnPoint_SpectralChinchompa actor in the current world; Vannaka remains only as the hand-in marker.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Spectral Chinchompa spawn** (`npc`) — map `0.59275405, 0.23020838`; world `237881.7, -20997.479, -957.608`
  - Determination: reviewed objective relationship to extracted game npc 'spectral-chinchompa' actor type 'BP_SpawnPoint_SpectralChinchompa': The objective is a roaming enemy collection task rather than a single Vannaka interaction. Every published hunt point resolves from a placed BP_SpawnPoint_SpectralChinchompa actor in the current world; Vannaka remains only as the hand-in marker.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Spectral Chinchompa spawn** (`npc`) — map `0.15891986, 0.12694176`; world `55671.34, -64369.46, -5523.296`
  - Determination: reviewed objective relationship to extracted game npc 'spectral-chinchompa' actor type 'BP_SpawnPoint_SpectralChinchompa': The objective is a roaming enemy collection task rather than a single Vannaka interaction. Every published hunt point resolves from a placed BP_SpawnPoint_SpectralChinchompa actor in the current world; Vannaka remains only as the hand-in marker.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **Spectral Chinchompa spawn** (`npc`) — map `0.51934067, 0.2560013`; world `207048.08, -10164.453, -8666.424`
  - Determination: reviewed objective relationship to extracted game npc 'spectral-chinchompa' actor type 'BP_SpawnPoint_SpectralChinchompa': The objective is a roaming enemy collection task rather than a single Vannaka interaction. Every published hunt point resolves from a placed BP_SpawnPoint_SpectralChinchompa actor in the current world; Vannaka remains only as the hand-in marker.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 5: **Spectral Chinchompa spawn** (`npc`) — map `0.49460557, 0.08607143`; world `196659.34, -81535, -6444.689`
  - Determination: reviewed objective relationship to extracted game npc 'spectral-chinchompa' actor type 'BP_SpawnPoint_SpectralChinchompa': The objective is a roaming enemy collection task rather than a single Vannaka interaction. Every published hunt point resolves from a placed BP_SpawnPoint_SpectralChinchompa actor in the current world; Vannaka remains only as the hand-in marker.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 6: **Spectral Chinchompa spawn** (`npc`) — map `0.44000821, 0.1859196`; world `173728.45, -39598.77, -10719.327`
  - Determination: reviewed objective relationship to extracted game npc 'spectral-chinchompa' actor type 'BP_SpawnPoint_SpectralChinchompa': The objective is a roaming enemy collection task rather than a single Vannaka interaction. Every published hunt point resolves from a placed BP_SpawnPoint_SpectralChinchompa actor in the current world; Vannaka remains only as the hand-in marker.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 7: **Spectral Chinchompa spawn** (`npc`) — map `0.32480748, 0.08862224`; world `125344.14, -80463.66, -5681.367`
  - Determination: reviewed objective relationship to extracted game npc 'spectral-chinchompa' actor type 'BP_SpawnPoint_SpectralChinchompa': The objective is a roaming enemy collection task rather than a single Vannaka interaction. Every published hunt point resolves from a placed BP_SpawnPoint_SpectralChinchompa actor in the current world; Vannaka remains only as the hand-in marker.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 8: **Spectral Chinchompa spawn** (`npc`) — map `0.46107838, 0.19103531`; world `182577.92, -37450.168, -11703.361`
  - Determination: reviewed objective relationship to extracted game npc 'spectral-chinchompa' actor type 'BP_SpawnPoint_SpectralChinchompa': The objective is a roaming enemy collection task rather than a single Vannaka interaction. Every published hunt point resolves from a placed BP_SpawnPoint_SpectralChinchompa actor in the current world; Vannaka remains only as the hand-in marker.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 9: **Spectral Chinchompa spawn** (`npc`) — map `0.49609945, 0.08734457`; world `197286.77, -81000.28, -6476.668`
  - Determination: reviewed objective relationship to extracted game npc 'spectral-chinchompa' actor type 'BP_SpawnPoint_SpectralChinchompa': The objective is a roaming enemy collection task rather than a single Vannaka interaction. Every published hunt point resolves from a placed BP_SpawnPoint_SpectralChinchompa actor in the current world; Vannaka remains only as the hand-in marker.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 10: **Spectral Chinchompa spawn** (`npc`) — map `0.56367595, 0.21284986`; world `225668.9, -28288.057, -5495.377`
  - Determination: reviewed objective relationship to extracted game npc 'spectral-chinchompa' actor type 'BP_SpawnPoint_SpectralChinchompa': The objective is a roaming enemy collection task rather than a single Vannaka interaction. Every published hunt point resolves from a placed BP_SpawnPoint_SpectralChinchompa actor in the current world; Vannaka remains only as the hand-in marker.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 11: **Spectral Chinchompa spawn** (`npc`) — map `0.21270251, 0.20950979`; world `78260.055, -29690.89, -6830.654`
  - Determination: reviewed objective relationship to extracted game npc 'spectral-chinchompa' actor type 'BP_SpawnPoint_SpectralChinchompa': The objective is a roaming enemy collection task rather than a single Vannaka interaction. Every published hunt point resolves from a placed BP_SpawnPoint_SpectralChinchompa actor in the current world; Vannaka remains only as the hand-in marker.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 12: **Spectral Chinchompa spawn** (`npc`) — map `0.56300014, 0.21419999`; world `225385.06, -27721.006, -5450.488`
  - Determination: reviewed objective relationship to extracted game npc 'spectral-chinchompa' actor type 'BP_SpawnPoint_SpectralChinchompa': The objective is a roaming enemy collection task rather than a single Vannaka interaction. Every published hunt point resolves from a placed BP_SpawnPoint_SpectralChinchompa actor in the current world; Vannaka remains only as the hand-in marker.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 13: **Spectral Chinchompa spawn** (`npc`) — map `0.46194052, 0.19185437`; world `182940.02, -37106.164, -11731.048`
  - Determination: reviewed objective relationship to extracted game npc 'spectral-chinchompa' actor type 'BP_SpawnPoint_SpectralChinchompa': The objective is a roaming enemy collection task rather than a single Vannaka interaction. Every published hunt point resolves from a placed BP_SpawnPoint_SpectralChinchompa actor in the current world; Vannaka remains only as the hand-in marker.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 14: **Spectral Chinchompa spawn** (`npc`) — map `0.32625575, 0.08799418`; world `125952.414, -80727.445, -5851.969`
  - Determination: reviewed objective relationship to extracted game npc 'spectral-chinchompa' actor type 'BP_SpawnPoint_SpectralChinchompa': The objective is a roaming enemy collection task rather than a single Vannaka interaction. Every published hunt point resolves from a placed BP_SpawnPoint_SpectralChinchompa actor in the current world; Vannaka remains only as the hand-in marker.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 15: **Spectral Chinchompa spawn** (`npc`) — map `0.32573926, 0.08862224`; world `125735.49, -80463.66, -5783.373`
  - Determination: reviewed objective relationship to extracted game npc 'spectral-chinchompa' actor type 'BP_SpawnPoint_SpectralChinchompa': The objective is a roaming enemy collection task rather than a single Vannaka interaction. Every published hunt point resolves from a placed BP_SpawnPoint_SpectralChinchompa actor in the current world; Vannaka remains only as the hand-in marker.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 16: **Spectral Chinchompa spawn** (`npc`) — map `0.46242421, 0.19096505`; world `183143.17, -37479.68, -11703.361`
  - Determination: reviewed objective relationship to extracted game npc 'spectral-chinchompa' actor type 'BP_SpawnPoint_SpectralChinchompa': The objective is a roaming enemy collection task rather than a single Vannaka interaction. Every published hunt point resolves from a placed BP_SpawnPoint_SpectralChinchompa actor in the current world; Vannaka remains only as the hand-in marker.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 17: **Spectral Chinchompa spawn** (`npc`) — map `0.56210383, 0.21275967`; world `225008.61, -28325.938, -5538.514`
  - Determination: reviewed objective relationship to extracted game npc 'spectral-chinchompa' actor type 'BP_SpawnPoint_SpectralChinchompa': The objective is a roaming enemy collection task rather than a single Vannaka interaction. Every published hunt point resolves from a placed BP_SpawnPoint_SpectralChinchompa actor in the current world; Vannaka remains only as the hand-in marker.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 18: **Spectral Chinchompa spawn** (`npc`) — map `0.43999736, 0.18680659`; world `173723.89, -39226.234, -10719.905`
  - Determination: reviewed objective relationship to extracted game npc 'spectral-chinchompa' actor type 'BP_SpawnPoint_SpectralChinchompa': The objective is a roaming enemy collection task rather than a single Vannaka interaction. Every published hunt point resolves from a placed BP_SpawnPoint_SpectralChinchompa actor in the current world; Vannaka remains only as the hand-in marker.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 19: **Spectral Chinchompa spawn** (`npc`) — map `0.51771157, 0.25555868`; world `206363.86, -10350.354, -8666.424`
  - Determination: reviewed objective relationship to extracted game npc 'spectral-chinchompa' actor type 'BP_SpawnPoint_SpectralChinchompa': The objective is a roaming enemy collection task rather than a single Vannaka interaction. Every published hunt point resolves from a placed BP_SpawnPoint_SpectralChinchompa actor in the current world; Vannaka remains only as the hand-in marker.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 20: **Spectral Chinchompa spawn** (`npc`) — map `0.49547214, 0.08605305`; world `197023.3, -81542.72, -6413.629`
  - Determination: reviewed objective relationship to extracted game npc 'spectral-chinchompa' actor type 'BP_SpawnPoint_SpectralChinchompa': The objective is a roaming enemy collection task rather than a single Vannaka interaction. Every published hunt point resolves from a placed BP_SpawnPoint_SpectralChinchompa actor in the current world; Vannaka remains only as the hand-in marker.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 21: **Spectral Chinchompa spawn** (`npc`) — map `0.59385852, 0.22912724`; world `238345.58, -21451.56, -888.776`
  - Determination: reviewed objective relationship to extracted game npc 'spectral-chinchompa' actor type 'BP_SpawnPoint_SpectralChinchompa': The objective is a roaming enemy collection task rather than a single Vannaka interaction. Every published hunt point resolves from a placed BP_SpawnPoint_SpectralChinchompa actor in the current world; Vannaka remains only as the hand-in marker.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 22: **Spectral Chinchompa spawn** (`npc`) — map `0.21268048, 0.21008961`; world `78250.8, -29447.363, -6806.011`
  - Determination: reviewed objective relationship to extracted game npc 'spectral-chinchompa' actor type 'BP_SpawnPoint_SpectralChinchompa': The objective is a roaming enemy collection task rather than a single Vannaka interaction. Every published hunt point resolves from a placed BP_SpawnPoint_SpectralChinchompa actor in the current world; Vannaka remains only as the hand-in marker.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 23: **Spectral Chinchompa spawn** (`npc`) — map `0.15864817, 0.12750871`; world `55557.23, -64131.34, -5537.247`
  - Determination: reviewed objective relationship to extracted game npc 'spectral-chinchompa' actor type 'BP_SpawnPoint_SpectralChinchompa': The objective is a roaming enemy collection task rather than a single Vannaka interaction. Every published hunt point resolves from a placed BP_SpawnPoint_SpectralChinchompa actor in the current world; Vannaka remains only as the hand-in marker.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 24: **Spectral Chinchompa spawn** (`npc`) — map `0.46186238, 0.19045903`; world `182907.2, -37692.207, -11685.662`
  - Determination: reviewed objective relationship to extracted game npc 'spectral-chinchompa' actor type 'BP_SpawnPoint_SpectralChinchompa': The objective is a roaming enemy collection task rather than a single Vannaka interaction. Every published hunt point resolves from a placed BP_SpawnPoint_SpectralChinchompa actor in the current world; Vannaka remains only as the hand-in marker.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 25: **Spectral Chinchompa spawn** (`npc`) — map `0.32529264, 0.08713921`; world `125547.91, -81086.53, -5732.211`
  - Determination: reviewed objective relationship to extracted game npc 'spectral-chinchompa' actor type 'BP_SpawnPoint_SpectralChinchompa': The objective is a roaming enemy collection task rather than a single Vannaka interaction. Every published hunt point resolves from a placed BP_SpawnPoint_SpectralChinchompa actor in the current world; Vannaka remains only as the hand-in marker.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 26: **Spectral Chinchompa spawn** (`npc`) — map `0.51788843, 0.25640273`; world `206438.14, -9995.853, -8666.425`
  - Determination: reviewed objective relationship to extracted game npc 'spectral-chinchompa' actor type 'BP_SpawnPoint_SpectralChinchompa': The objective is a roaming enemy collection task rather than a single Vannaka interaction. Every published hunt point resolves from a placed BP_SpawnPoint_SpectralChinchompa actor in the current world; Vannaka remains only as the hand-in marker.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 27: **Vannaka** (`location`) — map `0.32753919, 0.20839084`; world `126491.46, -30160.848, -7408.736`; shared-coordinate group [S20](#s20)
  - Determination: reviewed objective relationship to extracted game registry landmark '4E97E2C0-40F420D4-82DBC3B5-F25FA4D1': The objective is a roaming enemy collection task rather than a single Vannaka interaction. Every published hunt point resolves from a placed BP_SpawnPoint_SpectralChinchompa actor in the current world; Vannaka remains only as the hand-in marker.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 2. Speak to Vannaka in Fellhollow.

- Objective key: `SpeaktoVannaka`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Vannaka** (`location`) — map `0.32753919, 0.20839084`; world `126491.46, -30160.848, -7408.736`; shared-coordinate group [S20](#s20)
  - Determination: reviewed objective relationship to extracted game registry landmark '4E97E2C0-40F420D4-82DBC3B5-F25FA4D1': The objective explicitly starts at the uniquely resolved Fellhollow Vannaka placement.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

### What Remains is Written

Side quest · Fellhollow

#### 1. Find five of Abraxus’ missing books in eastern Fellhollow and return them to him.

- Objective key: `FindBooks`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: Collect the five individually placed books in eastern Fellhollow, then return them to Abraxus at Coalridge Pass.
- Reviewed relationship: **yes**
- Cooked location operations: `Abraxus The Eternal` (revealed, `QF_FH_WhatRemainsIsWritten.json` / `QFT_UpdateQuestLocation_C_3`)
- Marker 1: **Aetheric Fundamentals, a Primordial Primer** (`world_actor`) — map `0.55930888, 0.16139381`; world `223834.73, -49899.598, 7251.867`; shared-coordinate group [S57](#s57)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_NPC_FH_WhatRemains_Book1': The current-build quest actors BP_NPC_FH_WhatRemains_Book1 through Book5 expose the five exact book display names, TookBook1..5 quest booleans, WhatRemains participant/entry tags, and unique authored world placements. The objective also explicitly includes the return to the Coalridge Abraxus actor.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/1AEP5DGD1XXTXYOCR4YCRLQJO.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **The Pride of the Avernic** (`world_actor`) — map `0.60055826, 0.20257285`; world `241159.47, -32604.402, 4833.055`; shared-coordinate group [S66](#s66)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_NPC_FH_WhatRemains_Book2': The current-build quest actors BP_NPC_FH_WhatRemains_Book1 through Book5 expose the five exact book display names, TookBook1..5 quest booleans, WhatRemains participant/entry tags, and unique authored world placements. The objective also explicitly includes the return to the Coalridge Abraxus actor.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/924LPY5237PHSF5H3RRV2IEVR.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **The Wrath of Baba Potterington** (`world_actor`) — map `0.5311059, 0.18667376`; world `211989.48, -39282.02, -6234.3335`; shared-coordinate group [S49](#s49)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_NPC_FH_WhatRemains_Book3': The current-build quest actors BP_NPC_FH_WhatRemains_Book1 through Book5 expose the five exact book display names, TookBook1..5 quest booleans, WhatRemains participant/entry tags, and unique authored world placements. The objective also explicitly includes the return to the Coalridge Abraxus actor.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/1AEP5DGD1XXTXYOCR4YCRLQJO.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **The Importance of Chaos** (`world_actor`) — map `0.52307481, 0.2416812`; world `208616.42, -16178.898, -8448.804`; shared-coordinate group [S45](#s45)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_NPC_FH_WhatRemains_Book4': The current-build quest actors BP_NPC_FH_WhatRemains_Book1 through Book5 expose the five exact book display names, TookBook1..5 quest booleans, WhatRemains participant/entry tags, and unique authored world placements. The objective also explicitly includes the return to the Coalridge Abraxus actor.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/0HL76YBH79TQVOKNTQMMX3US6.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 5: **Leonard The Bound** (`world_actor`) — map `0.47397869, 0.22572224`; world `187996.05, -22881.66, -9124.527`; shared-coordinate group [S33](#s33)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_NPC_FH_WhatRemains_Book5': The current-build quest actors BP_NPC_FH_WhatRemains_Book1 through Book5 expose the five exact book display names, TookBook1..5 quest booleans, WhatRemains participant/entry tags, and unique authored world placements. The objective also explicitly includes the return to the Coalridge Abraxus actor.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/7EOARR1YMXQR7YC28236IWN0P.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 6: **Abraxus The Eternal** (`npc`) — map `0.50379445, 0.10259045`; world `200518.67, -74597.01, -10339.829`; shared-coordinate group [S39](#s39)
  - Determination: reviewed objective relationship to extracted game npc 'abraxus-the-eternal' actor type 'BP_NPC_FH_Abraxus_Ghost': The current-build quest actors BP_NPC_FH_WhatRemains_Book1 through Book5 expose the five exact book display names, TookBook1..5 quest booleans, WhatRemains participant/entry tags, and unique authored world placements. The objective also explicitly includes the return to the Coalridge Abraxus actor.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 2. Bring the books to Abraxus.

- Objective key: `FoundBooks`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Display note: Bring the five books to Abraxus at Coalridge Pass.
- Reviewed relationship: **yes**
- Cooked location operations: `Abraxus The Eternal` (revealed, `QF_FH_WhatRemainsIsWritten.json` / `QFT_UpdateQuestLocation_C_5`)
- Marker 1: **Abraxus The Eternal** (`npc`) — map `0.50379445, 0.10259045`; world `200518.67, -74597.01, -10339.829`; shared-coordinate group [S39](#s39)
  - Determination: reviewed objective relationship to extracted game npc 'abraxus-the-eternal' actor type 'BP_NPC_FH_Abraxus_Ghost': This is the book hand-in state. BP_NPC_FH_Abraxus_Ghost is the quest's Coalridge actor; the same-named BP_NPC_FH_Abraxus_Dowdun placement belongs to another quest state.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 3. Defeat Abraxus' enemies to retrieve the scrolls.

- Objective key: `FindScrolls`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: Recover Binding of Ib, Coclear Malificarum, and Eventide at their three authored interaction points. The markers identify the scroll pickups associated with Blightscale, Ulgo, and Razlem, not merely the enemies' patrol positions.
- Reviewed relationship: **yes**
- Cooked location operations: `Abraxus The Eternal` (revealed, `QF_FH_WhatRemainsIsWritten.json` / `QFT_UpdateQuestLocation_C_0`); `Abraxus The Eternal` (revealed, `QF_FH_WhatRemainsIsWritten.json` / `QFT_UpdateQuestLocation_C_1`)
- Marker 1: **The Binding of Ib** (`world_actor`) — map `0.58781593, 0.2348789`; world `235807.69, -19035.86, -4675.3384`; shared-coordinate group [S64](#s64)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_NPC_FH_WhatRemains_Scroll1': The current-build quest actors BP_NPC_FH_WhatRemains_Scroll1 through Scroll3 expose the exact scroll names, TookScroll1..3 quest booleans, WhatRemains participant/entry tags, and unique authored placements.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/AH1KHVYOKGH2CP54XNL3JAWTA.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Coclear Malificarum** (`world_actor`) — map `0.5443506, 0.12216518`; world `217552.25, -66375.625, -9330.706`; shared-coordinate group [S53](#s53)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_NPC_FH_WhatRemains_Scroll2': The current-build quest actors BP_NPC_FH_WhatRemains_Scroll1 through Scroll3 expose the exact scroll names, TookScroll1..3 quest booleans, WhatRemains participant/entry tags, and unique authored placements.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/0YCY7RWU9XXVKIWCMAOVZ0FNA.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Eventide - A New Spawn** (`world_actor`) — map `0.56451488, 0.2759552`; world `226021.25, -1783.8168, -5564.1494`; shared-coordinate group [S59](#s59)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_NPC_FH_WhatRemains_Scroll3': The current-build quest actors BP_NPC_FH_WhatRemains_Scroll1 through Scroll3 expose the exact scroll names, TookScroll1..3 quest booleans, WhatRemains participant/entry tags, and unique authored placements.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/0HL76YBH79TQVOKNTQMMX3US6.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 4. Bring the scrolls to Abraxus.

- Objective key: `FoundScrolls`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Display note: Bring the three scrolls to Abraxus at Coalridge Pass.
- Reviewed relationship: **yes**
- Cooked location operations: `Abraxus The Eternal` (revealed, `QF_FH_WhatRemainsIsWritten.json` / `QFT_UpdateQuestLocation_C_4`)
- Marker 1: **Abraxus The Eternal** (`npc`) — map `0.50379445, 0.10259045`; world `200518.67, -74597.01, -10339.829`; shared-coordinate group [S39](#s39)
  - Determination: reviewed objective relationship to extracted game npc 'abraxus-the-eternal' actor type 'BP_NPC_FH_Abraxus_Ghost': This is the scroll hand-in state. BP_NPC_FH_Abraxus_Ghost is the exact Coalridge quest actor and excludes the unrelated same-named Dowdun placement.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 5. Find five of Abraxus’ missing books in eastern Fellhollow and return them to him.

- Objective key: `FindBooksAndReturn`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: Collect any missing books at their five authored interaction points and return the complete set to Abraxus at Coalridge Pass.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Aetheric Fundamentals, a Primordial Primer** (`world_actor`) — map `0.55930888, 0.16139381`; world `223834.73, -49899.598, 7251.867`; shared-coordinate group [S57](#s57)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_NPC_FH_WhatRemains_Book1': This counter/combined state has the same five-book action and return requirement. The exact current-build WhatRemains book actors provide the collection positions; the exact Coalridge Abraxus actor provides the hand-in position.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/1AEP5DGD1XXTXYOCR4YCRLQJO.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **The Pride of the Avernic** (`world_actor`) — map `0.60055826, 0.20257285`; world `241159.47, -32604.402, 4833.055`; shared-coordinate group [S66](#s66)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_NPC_FH_WhatRemains_Book2': This counter/combined state has the same five-book action and return requirement. The exact current-build WhatRemains book actors provide the collection positions; the exact Coalridge Abraxus actor provides the hand-in position.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/924LPY5237PHSF5H3RRV2IEVR.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **The Wrath of Baba Potterington** (`world_actor`) — map `0.5311059, 0.18667376`; world `211989.48, -39282.02, -6234.3335`; shared-coordinate group [S49](#s49)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_NPC_FH_WhatRemains_Book3': This counter/combined state has the same five-book action and return requirement. The exact current-build WhatRemains book actors provide the collection positions; the exact Coalridge Abraxus actor provides the hand-in position.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/1AEP5DGD1XXTXYOCR4YCRLQJO.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **The Importance of Chaos** (`world_actor`) — map `0.52307481, 0.2416812`; world `208616.42, -16178.898, -8448.804`; shared-coordinate group [S45](#s45)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_NPC_FH_WhatRemains_Book4': This counter/combined state has the same five-book action and return requirement. The exact current-build WhatRemains book actors provide the collection positions; the exact Coalridge Abraxus actor provides the hand-in position.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/0HL76YBH79TQVOKNTQMMX3US6.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 5: **Leonard The Bound** (`world_actor`) — map `0.47397869, 0.22572224`; world `187996.05, -22881.66, -9124.527`; shared-coordinate group [S33](#s33)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_NPC_FH_WhatRemains_Book5': This counter/combined state has the same five-book action and return requirement. The exact current-build WhatRemains book actors provide the collection positions; the exact Coalridge Abraxus actor provides the hand-in position.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/7EOARR1YMXQR7YC28236IWN0P.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 6: **Abraxus The Eternal** (`npc`) — map `0.50379445, 0.10259045`; world `200518.67, -74597.01, -10339.829`; shared-coordinate group [S39](#s39)
  - Determination: reviewed objective relationship to extracted game npc 'abraxus-the-eternal' actor type 'BP_NPC_FH_Abraxus_Ghost': This counter/combined state has the same five-book action and return requirement. The exact current-build WhatRemains book actors provide the collection positions; the exact Coalridge Abraxus actor provides the hand-in position.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 6. Defeat Abraxus' enemies to retrieve the scrolls. Then return them to Abraxus.

- Objective key: `FindScrollsAndReturn`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: Recover any missing scrolls at their three authored interaction points, then return them to Abraxus at Coalridge Pass.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **The Binding of Ib** (`world_actor`) — map `0.58781593, 0.2348789`; world `235807.69, -19035.86, -4675.3384`; shared-coordinate group [S64](#s64)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_NPC_FH_WhatRemains_Scroll1': This combined state retains the three current-build WhatRemains scroll actors and the exact Coalridge Abraxus hand-in actor.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/AH1KHVYOKGH2CP54XNL3JAWTA.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Coclear Malificarum** (`world_actor`) — map `0.5443506, 0.12216518`; world `217552.25, -66375.625, -9330.706`; shared-coordinate group [S53](#s53)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_NPC_FH_WhatRemains_Scroll2': This combined state retains the three current-build WhatRemains scroll actors and the exact Coalridge Abraxus hand-in actor.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/0YCY7RWU9XXVKIWCMAOVZ0FNA.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Eventide - A New Spawn** (`world_actor`) — map `0.56451488, 0.2759552`; world `226021.25, -1783.8168, -5564.1494`; shared-coordinate group [S59](#s59)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_NPC_FH_WhatRemains_Scroll3': This combined state retains the three current-build WhatRemains scroll actors and the exact Coalridge Abraxus hand-in actor.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/0HL76YBH79TQVOKNTQMMX3US6.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **Abraxus The Eternal** (`npc`) — map `0.50379445, 0.10259045`; world `200518.67, -74597.01, -10339.829`; shared-coordinate group [S39](#s39)
  - Determination: reviewed objective relationship to extracted game npc 'abraxus-the-eternal' actor type 'BP_NPC_FH_Abraxus_Ghost': This combined state retains the three current-build WhatRemains scroll actors and the exact Coalridge Abraxus hand-in actor.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 7. Find the owner of the sinister text. 

- Objective key: `ObjectiveZero`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Display note: This objective points to Abraxus at Coalridge Pass; the separate Dowdun quest-state actor is not part of this objective.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Abraxus The Eternal** (`npc`) — map `0.50379445, 0.10259045`; world `200518.67, -74597.01, -10339.829`; shared-coordinate group [S39](#s39)
  - Determination: reviewed objective relationship to extracted game npc 'abraxus-the-eternal' actor type 'BP_NPC_FH_Abraxus_Ghost': The quest flow identifies Abraxus as the owner. The authored Coalridge placement is BP_NPC_FH_Abraxus_Ghost; BP_NPC_FH_Abraxus_Dowdun belongs to a different quest state.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

### A Melody Remembered

Side quest · Ghornfell

#### 1. The jewellery box requires a specific ring.

- Objective key: `Objective1`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Display note: Bring an Inspiring Ring of Runecrafting to this box.
- Reviewed relationship: **yes**
- Cooked location operations: `JewelleryBox` (revealed, `QF_AMelodyRemebered.json` / `QFT_UpdateQuestLocation_C_0`)
- Marker 1: **Jewellery Box — Inspiring Ring of Runecrafting** (`location`) — map `0.06214134, 0.47796524`; world `15024.364, 83060.4, 12166.037`; shared-coordinate group [S1](#s1)
  - Determination: reviewed objective relationship to extracted game registry landmark '7743312F-4E17D80F-A455BE9C-0F55C7F7': The quest flow reveals JewelleryBox and the current registry contains one quest-owned Jewellery Box identity at exactly that target. All auto-discovered candidates were duplicate representations of the same interaction.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 2. The jewellery box requires a specific amulet.

- Objective key: `Objective2`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Display note: Bring the standard Amulet of Magic, not the Inspiring variant.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Jewellery Box — Amulet of Magic** (`location`) — map `0.06214134, 0.47796524`; world `15024.364, 83060.4, 12166.037`; shared-coordinate group [S1](#s1)
  - Determination: reviewed objective relationship to extracted game registry landmark '7743312F-4E17D80F-A455BE9C-0F55C7F7': The current world registry contains one quest-owned Jewellery Box identity matching the objective; the additional JewelleryBox candidate is its colocated authored target.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

### A Room with a Garou

Side quest · Ghornfell

#### 1. Build a home around Caratacus the Elder.

- Objective key: `None`
- Destination mode: `area`
- Confidence: **High (91/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. The game authors an area rather than one exact interaction point.
- Display note: Build the home around Caratacus; the marker identifies the exact elder placement, not a mandatory building snap point.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Caratacus building site** (`location`) — map `0.17249994, 0.49355983`; world `61374.973, 89610.13, 319.77213`
  - Determination: reviewed objective relationship to extracted game registry landmark '57F6B9DA-420523CE-2261DB92-E304B9DC': The objective is player construction around the uniquely registered Caratacus the Elder placement.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

### Animal Magnetism

Side quest · Ghornfell

#### 1. Read the Advert-Inna-Bottle next to Manktongue

- Objective key: `None`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Advert-Inna-Bottle** (`location`) — map `0.13067089, 0.56809457`; world `43806.773, 120914.72, 2262.2192`
  - Determination: reviewed objective relationship to extracted game registry landmark '32BD943C-48CB0710-47A20DBF-E46A2612': The objective explicitly reads the uniquely identified Advert-Inna-Bottle interaction next to Manktongue; Manktongue himself is not the progression interaction for this state.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 2. Discuss the advert with Manktongue

- Objective key: `Objective2`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Manktongue** (`npc`) — map `0.1303523, 0.56805707`; world `43672.965, 120898.97, 2242.93`; shared-coordinate group [S7](#s7)
  - Determination: reviewed objective relationship to extracted game npc 'manktongue': After reading the advert, the authored dialogue and objective both return to the single extracted Manktongue NPC placement.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 3. Gather an undead chicken, magnetised iron bar and undead twigs, and bring them to Manktongue

- Objective key: `Objective3`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: Collect the undead chicken and an undead twig bundle. Pulpknuckle magnetises an iron bar for five law runes at Manktongue's camp; return the components to Manktongue there.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Undead Chicken** (`location`) — map `0.12338868, 0.47697571`; world `40748.246, 82644.8, 6536.584`
  - Determination: reviewed objective relationship to extracted game registry landmark '20A409F9-407F5A30-9262E180-5A922918': The quest dialogue explicitly places the chicken due north, the twig bundle at a glowing Bloodblight tree, and Pulpknuckle upstairs at the quest-giver camp. The current world provides exact quest-specific chicken, twig, and BP_NPC_Goblin_Smith interactions; Manktongue remains the separate component hand-in.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Undead Twig Bundle** (`world_actor`) — map `0.33278226, 0.6349426`; world `128693.55, 148990.89, -2122.4712`
  - Determination: reviewed objective relationship to extracted game world actor 'BP_NPC_Quest_UndeadTwigBundle': The quest dialogue explicitly places the chicken due north, the twig bundle at a glowing Bloodblight tree, and Pulpknuckle upstairs at the quest-giver camp. The current world provides exact quest-specific chicken, twig, and BP_NPC_Goblin_Smith interactions; Manktongue remains the separate component hand-in.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/2EZDVI8Q8B7A19UOIUYL7QVFF.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Pulpknuckle — magnetise iron bar** (`world_actor`) — map `0.12908925, 0.56645505`; world `43142.484, 120226.12, 2552.4922`
  - Determination: reviewed objective relationship to extracted game world actor 'BP_NPC_Goblin_Smith': The quest dialogue explicitly places the chicken due north, the twig bundle at a glowing Bloodblight tree, and Pulpknuckle upstairs at the quest-giver camp. The current world provides exact quest-specific chicken, twig, and BP_NPC_Goblin_Smith interactions; Manktongue remains the separate component hand-in.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/7JMU10RBHE3GDHPS9WC0APS9A.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **Manktongue — component hand-in** (`npc`) — map `0.1303523, 0.56805707`; world `43672.965, 120898.97, 2242.93`; shared-coordinate group [S7](#s7)
  - Determination: reviewed objective relationship to extracted game npc 'manktongue': The quest dialogue explicitly places the chicken due north, the twig bundle at a glowing Bloodblight tree, and Pulpknuckle upstairs at the quest-giver camp. The current world provides exact quest-specific chicken, twig, and BP_NPC_Goblin_Smith interactions; Manktongue remains the separate component hand-in.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 4. Return to Manktongue

- Objective key: `Objective4`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Manktongue** (`npc`) — map `0.1303523, 0.56805707`; world `43672.965, 120898.97, 2242.93`; shared-coordinate group [S7](#s7)
  - Determination: reviewed objective relationship to extracted game npc 'manktongue': The final objective explicitly returns to the single extracted Manktongue placement.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

### Cook's Assistant

Side quest · Ghornfell

#### 1. Gather 4 eggs, 6 wheat and a bucket of milk, and bring them all to Slopfinger.

- Objective key: `Objective1`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: The Dairy Cow is the fixed milk interaction and Slopfinger is the hand-in. Eggs may come from chicken or terrorbird eggs, and wheat is grown at any valid farming patch, so those renewable sources have no single truthful marker.
- Reviewed relationship: **yes**
- Cooked location operations: `GoblinChef` (revealed, `QF_CooksAssistant.json` / `QFT_UpdateQuestLocation_C_0`)
- Marker 1: **Dairy Cow (Bucket of Milk)** (`world_actor`) — map `0.1336978, 0.56902714`; world `45078.074, 121306.4, 2114.211`
  - Determination: reviewed objective relationship to extracted game world actor 'BP_NPC_Quest_Cow': Cooked Slopfinger dialogue explicitly permits chicken or terrorbird eggs, describes farming wheat, and identifies the nearby Dairy Cow. BP_NPC_Quest_Cow and Slopfinger each have one current-build placement.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/7JMU10RBHE3GDHPS9WC0APS9A.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Slopfinger (ingredient hand-in)** (`npc`) — map `0.1289099, 0.56793729`; world `43067.16, 120848.66, 2242.929`; shared-coordinate group [S6](#s6)
  - Determination: reviewed objective relationship to extracted game npc 'slopfinger': Cooked Slopfinger dialogue explicitly permits chicken or terrorbird eggs, describes farming wheat, and identifies the nearby Dairy Cow. BP_NPC_Quest_Cow and Slopfinger each have one current-build placement.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 2. Bake a cake using the stove next to Slopfinger.

- Objective key: `Objective2`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Goblin Stove** (`world_actor`) — map `0.12943368, 0.56835481`; world `43287.145, 121024.02, 2274.5664`
  - Determination: reviewed objective relationship to extracted game world actor 'BP_NPC_Quest_Stove': The objective explicitly names the stove beside Slopfinger; the current world has one placed BP_NPC_Quest_Stove whose dialogue bakes the Glorious Cake.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/7JMU10RBHE3GDHPS9WC0APS9A.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 3. Bring the signed cake to Manktongue.

- Objective key: `Objective3`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: `Manktongue` (revealed, `QF_AnimalMagnetism.json` / `QFT_UpdateQuestLocation_C_3`)
- Marker 1: **Manktongue — signed cake delivery** (`npc`) — map `0.1303523, 0.56805707`; world `43672.965, 120898.97, 2242.93`; shared-coordinate group [S7](#s7)
  - Determination: reviewed objective relationship to extracted game npc 'manktongue': The signed-cake state advances only through Manktongue's dialogue. The NPC catalog contains one current Manktongue placement at the camp.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 4. Show Slopfinger the freshly cooked cake.

- Objective key: `Objective4`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Slopfinger — have the cake signed** (`npc`) — map `0.1289099, 0.56793729`; world `43067.16, 120848.66, 2242.929`; shared-coordinate group [S6](#s6)
  - Determination: reviewed objective relationship to extracted game npc 'slopfinger': The freshly baked cake must be shown to Slopfinger so he can sign it; the current world contains one Slopfinger placement.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 5. Give Slopfinger the good news.

- Objective key: `Objective5`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Slopfinger — report the good news** (`npc`) — map `0.1289099, 0.56793729`; world `43067.16, 120848.66, 2242.929`; shared-coordinate group [S6](#s6)
  - Determination: reviewed objective relationship to extracted game npc 'slopfinger': After Manktongue accepts the cake, the final conversation and reward occur at the same unique Slopfinger actor.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

### Dog Days

Side quest · Ghornfell / Garou shrine

#### 1. Search Garou camps beneath giant wyrm bones in the Highlands for information on Garou ritual practices.

- Objective key: `Objective1`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **BP_LoreItem_DogDays_DD1** (`world_actor`) — map `0.08822407, 0.51816455`; world `25979.111, 99944.11, 7927.8887`; shared-coordinate group [S3](#s3)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_LoreItem_DogDays_DD1': The ritual-practices clue is the uniquely placed BP_LoreItem_DogDays_DD1, whose close callback enters QuestFlow.Entry.DogDays.LoreEntry.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/7JEDEDOZ4FBAM8W7QKL4Y9A1K.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 2. Challenge a Garou king in their Highland stronghold and claim their token.

- Objective key: `Objective2`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Garou King** (`npc`) — map `0.0700363, 0.45215378`; world `18340.248, 72219.586, 14889.971`
  - Determination: reviewed objective relationship to extracted game npc 'garou-king': The objective requires challenging the uniquely placed Garou King, resolved from the NPC registry.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 3. Search garou camps beneath the giant wyrm bones in the Highlands for information on the King's Token.

- Objective key: `Objective3`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **BP_LoreItem_DogDays_DD1** (`world_actor`) — map `0.08822407, 0.51816455`; world `25979.111, 99944.11, 7927.8887`; shared-coordinate group [S3](#s3)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_LoreItem_DogDays_DD1': This alternate ordering state still requires the Dog Days ritual lore entry after obtaining the King's Token first.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/7JEDEDOZ4FBAM8W7QKL4Y9A1K.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 4. Complete the ritual of purification on the King's Token at the ancient shrine where "a heart once beat".

- Objective key: `Objective4`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Lougrim's Shrine** (`world_actor`) — map `0.12855635, 0.46532619`; world `42918.668, 77752.0, 8071.2827`
  - Determination: reviewed objective relationship to extracted game world actor 'BP_NPC_Lougrim_Shrine': The objective is completed through QuestFlow.Participant.LougrimShrine; the world contains one exact BP_NPC_Lougrim_Shrine actor.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/7JEDEDOZ4FBAM8W7QKL4Y9A1K.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 5. Bring the Purified King's Token to Vannaka in the Ghornfell Highlands.

- Objective key: `Objective5`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Vannaka — Ghornfell Highlands** (`npc`) — map `0.13132323, 0.50114205`; world `44080.758, 92794.66, 20160.379`; shared-coordinate group [S8](#s8)
  - Determination: reviewed objective relationship to extracted game quest marker 'Vannaka_DogDays': The objective explicitly names Vannaka in Ghornfell and Vannaka_DogDays is the quest-owned purified-token hand-in target.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 6. Search the Ghornfell Highlands for a friendly face with an interest in Garou rituals.

- Objective key: `Objective6`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Vannaka_DogDays** (`npc`) — map `0.13132323, 0.50114205`; world `44080.758, 92794.66, 20160.379`; shared-coordinate group [S8](#s8)
  - Determination: reviewed objective relationship to extracted game quest marker 'Vannaka_DogDays': The quest dialogue identifies the friendly ritual expert as Vannaka and Quest_DogDays owns Vannaka_DogDays.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

### Granite Mauled

Side quest · Ghornfell

#### 1. Investigate the secrets hidden below Ashien's Watch.

- Objective key: `Objective1`
- Destination mode: `fixed`
- Confidence: **High (97/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Display note: The objective is inside the Skeklac vault; the world-map marker shows the Skeklac Kara entrance.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Skeklac Kara entrance** (`dungeon_entrance`) — map `0.35517076, 0.46523564`; world `138096.72, 77713.97, 4292.902`
  - Determination: reviewed objective relationship to extracted game dungeon entrance 'SkeklacKara': The Granite Mauled interior contains BP_LootChest_Prefab_Tier3_Vault_FinalChest_Skeklac, proving the vault identity; its extracted overworld entrance is SkeklacKara.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 97/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 2. Find someone near Ashien's Watch that can translate ancient dwarvish. 

- Objective key: `Objective2`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Doric_GM** (`npc`) — map `0.3649939, 0.5006833`; world `142222.44, 92601.984, 20160.379`; shared-coordinate group [S29](#s29)
  - Determination: reviewed objective relationship to extracted game quest marker 'Doric_GM': The quest's translator objective resolves to Doric and Quest_GraniteMauled owns Doric_GM.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 3. Bring the blueprint to Doric in Ashien's Watch.

- Objective key: `Objective3`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Doric — Ashien's Watch** (`npc`) — map `0.3649939, 0.5006833`; world `142222.44, 92601.984, 20160.379`; shared-coordinate group [S29](#s29)
  - Determination: reviewed objective relationship to extracted game quest marker 'Doric_GM': The objective explicitly brings the blueprint to Doric in Ashien's Watch and Doric_GM is the quest-owned hand-in target for this state.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

### Heartstrings

Side quest · Ghornfell

#### 1. Follow the memorial path, look for blue trees heading toward the east of Ghornfell and the memorial markers.

- Objective key: `follow the blue-leafed trees`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: All seven authored memorials are shown because this single aggregate objective remains active for the entire memorial path.
- Reviewed relationship: **yes**
- Cooked location operations: `Mesa_HS` (revealed, `QF_Heartstrings.json` / `QFT_UpdateQuestLocation_C_0`)
- Marker 1: **Heartstrings memorial** (`world_actor`) — map `0.29159174, 0.54991545`; world `111393.53, 113279.49, 301.55322`
  - Determination: reviewed objective relationship to extracted game world actor type 'BP_NPC_Hearstrings_Memorial': The cooked world contains exactly seven placed BP_NPC_Hearstrings_Memorial actors. This matches the seven sequential memorial interactions described by the quest guide; using the exact actor set is more precise than the broad Mesa_HS quest marker.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/41AEJS42NF6O5YS3CIMTI8OH4.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Heartstrings memorial** (`world_actor`) — map `0.32083108, 0.55774529`; world `123674.055, 116568.02, 780.919`
  - Determination: reviewed objective relationship to extracted game world actor type 'BP_NPC_Hearstrings_Memorial': The cooked world contains exactly seven placed BP_NPC_Hearstrings_Memorial actors. This matches the seven sequential memorial interactions described by the quest guide; using the exact actor set is more precise than the broad Mesa_HS quest marker.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/41AEJS42NF6O5YS3CIMTI8OH4.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Heartstrings memorial** (`world_actor`) — map `0.218661, 0.55048431`; world `80762.62, 113518.41, 6639.368`
  - Determination: reviewed objective relationship to extracted game world actor type 'BP_NPC_Hearstrings_Memorial': The cooked world contains exactly seven placed BP_NPC_Hearstrings_Memorial actors. This matches the seven sequential memorial interactions described by the quest guide; using the exact actor set is more precise than the broad Mesa_HS quest marker.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/76OD2K5FB931W3KXC1O0OXLZD.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **Heartstrings memorial** (`world_actor`) — map `0.24785214, 0.54994912`; world `93022.9, 113293.63, 1050.2893`
  - Determination: reviewed objective relationship to extracted game world actor type 'BP_NPC_Hearstrings_Memorial': The cooked world contains exactly seven placed BP_NPC_Hearstrings_Memorial actors. This matches the seven sequential memorial interactions described by the quest guide; using the exact actor set is more precise than the broad Mesa_HS quest marker.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/76OD2K5FB931W3KXC1O0OXLZD.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 5: **Heartstrings memorial** (`world_actor`) — map `0.33625774, 0.5182321`; world `130153.25, 99972.48, 3411.6091`
  - Determination: reviewed objective relationship to extracted game world actor type 'BP_NPC_Hearstrings_Memorial': The cooked world contains exactly seven placed BP_NPC_Hearstrings_Memorial actors. This matches the seven sequential memorial interactions described by the quest guide; using the exact actor set is more precise than the broad Mesa_HS quest marker.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/7SN3IF8J89HOIEGH9RZYPOQ0P.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 6: **Heartstrings memorial** (`world_actor`) — map `0.36059126, 0.57600743`; world `140373.33, 124238.12, 55.607292`
  - Determination: reviewed objective relationship to extracted game world actor type 'BP_NPC_Hearstrings_Memorial': The cooked world contains exactly seven placed BP_NPC_Hearstrings_Memorial actors. This matches the seven sequential memorial interactions described by the quest guide; using the exact actor set is more precise than the broad Mesa_HS quest marker.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/D6JR3YHGPAG0IT72Z4JP6XIVE.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 7: **Heartstrings memorial** (`world_actor`) — map `0.37053524, 0.54174583`; world `144549.8, 109848.25, 2355.496`
  - Determination: reviewed objective relationship to extracted game world actor type 'BP_NPC_Hearstrings_Memorial': The cooked world contains exactly seven placed BP_NPC_Hearstrings_Memorial actors. This matches the seven sequential memorial interactions described by the quest guide; using the exact actor set is more precise than the broad Mesa_HS quest marker.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/D6JR3YHGPAG0IT72Z4JP6XIVE.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

### Regicide

Main quest · Scorned Wilderness

#### 1. Defeat Kuldra

- Objective key: `DefeatKuldra`
- Destination mode: `fixed`
- Confidence: **High (97/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Display note: Kuldra is in the Silverthorn interior; the world-map marker shows the authored entrance.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **SW_Door** (`location`) — map `0.25361976, 0.42259874`; world `95445.3, 59806.47, 3657.5676`; shared-coordinate group [S15](#s15)
  - Determination: reviewed objective relationship to extracted game quest marker 'SW_Door': The Regicide flow enters the Silverthorn encounter through SW_Door; interior coordinates are intentionally replaced by the overworld entrance marker.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 97/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 2. Bring Kuldra's head to Zilyana. 

- Objective key: `BringHead`
- Destination mode: `fixed`
- Confidence: **High (97/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Display note: Zilyana is inside Silverthorn Keep; the world-map marker shows the authored entrance.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **SW_Door** (`location`) — map `0.25361976, 0.42259874`; world `95445.3, 59806.47, 3657.5676`; shared-coordinate group [S15](#s15)
  - Determination: reviewed objective relationship to extracted game quest marker 'SW_Door': The target NPC placement is outside world-map bounds inside the keep; SW_Door is the quest-owned overworld entrance.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 97/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 3. Gain access to the Scorned Wilderness. 

- Objective key: `AccessSW`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: `SW_Door` (revealed, `QF_SW_MainQuest.json` / `QFT_UpdateQuestLocation_C_0`)
- Marker 1: **Dragon Gate — Scorned Wilderness access** (`location`) — map `0.25361976, 0.42259874`; world `95445.3, 59806.47, 3657.5676`; shared-coordinate group [S15](#s15)
  - Determination: reviewed objective relationship to extracted game quest marker 'SW_Door': The current Regicide flow reveals exactly SW_Door for gaining access to the Scorned Wilderness, and the world contains one matching quest-owned Dragon Gate location.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 4. Speak to Zilyana. 

- Objective key: `SpeakZilyana`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Display note: Zilyana is inside Silverthorn Keep; the world-map marker shows its overworld entrance.
- Reviewed relationship: **yes**
- Cooked location operations: `SW_Door` (hidden/removed, `QF_SW_MainQuest.json` / `QFT_UpdateQuestLocation_C_4`); `ZilyanaNPC` (revealed, `QF_SW_MainQuest.json` / `QFT_UpdateQuestLocation_C_5`)
- Marker 1: **SW_Door** (`location`) — map `0.25361976, 0.42259874`; world `95445.3, 59806.47, 3657.5676`; shared-coordinate group [S15](#s15)
  - Determination: reviewed objective relationship to extracted game quest marker 'SW_Door': The objective reveals the interior ZilyanaNPC marker, which has no publishable overworld transform; the quest-owned SW_Door marker is the extracted entrance used for this interior.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 5. Once you are fully prepared, speak to Zilyana. 

- Objective key: `ReturnToZilyana`
- Destination mode: `fixed`
- Confidence: **High (97/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Display note: Zilyana is inside Silverthorn Keep; the world-map marker shows the authored entrance.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **SW_Door** (`location`) — map `0.25361976, 0.42259874`; world `95445.3, 59806.47, 3657.5676`; shared-coordinate group [S15](#s15)
  - Determination: reviewed objective relationship to extracted game quest marker 'SW_Door': The target NPC placement is outside world-map bounds inside the keep; SW_Door is the quest-owned overworld entrance.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 97/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

### Icthlarin's Little Helper

Main quest · Umbral Sands

#### 1. Speak to the Alcarrid Elder in the Moon Garou settlement to the south

- Objective key: `FindGarou`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Ancamairon — Alcarrid Elder** (`location`) — map `0.62743857, 0.54721181`; world `252449.2, 112143.96, 3231.601`; shared-coordinate group [S75](#s75)
  - Determination: reviewed objective relationship to extracted game quest marker 'CampLeader_A_Target': The objective names the Alcarrid Elder, and the cooked world provides CampLeader_A_Target at the active Ancamairon interaction. This avoids the two phase-specific Ancamairon registry copies and unrelated Uzzer/Manafem elders.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 2. Investigate the nearby area for something that concerns the Moon Garou

- Objective key: `InvestigateGarouProblem`
- Destination mode: `area`
- Confidence: **High (91/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. The game authors an area rather than one exact interaction point.
- Display note: Search within the authored Alcarrid camp vicinity. No individual progression actor for this legacy objective state can be proven from the current cooked flow.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Alcarrid camp investigation area** (`area`) — map `0.62376876, 0.55132352`; world `250907.88, 113870.88, 3361.1914`; shared-coordinate group [S71](#s71)
  - Determination: reviewed objective relationship to extracted game quest marker 'GarouCamp_A_Area': This state follows the instruction to find the Alcarrid settlement, so the exact quest-owned GarouCamp_A_Area is the narrowest proven location. Community-map associations with Uzzer stone slabs and Manafem dead scouts are parent-quest aggregation and are deliberately excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 3. Return to Shangruuh, the Alcarrid Envoy in the passage from Dowdun Reach

- Objective key: `ReturnToFirstGarou`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Shangruuh — Dowdun Reach passage** (`location`) — map `0.59532274, 0.48127207`; world `238960.55, 84449.27, 5874.241`
  - Determination: reviewed objective relationship to extracted game registry landmark '10014319-45A53113-A476CD9E-1A6C7636': The objective specifies Shangruuh in the passage from Dowdun Reach. Of the two current placements, 10014319 is beside the Bastion/Umbral Sands passage; 2DBE7A7B is the later Uzzer-camp copy used by Mark Day.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 4. Travel to Moon Garou camps, find their elders, and earn their trust to gain their Mirror Lenses

- Objective key: `FindThreeKeys`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: Visit the three clan elders and complete their camp tasks. The Alcarrid interaction is the authored leader target; the other two markers are the exact active Manafem and Uzzer elder placements.
- Reviewed relationship: **yes**
- Cooked location operations: `TribeC.Camp` (revealed, `QF_UmS_MQ_Beat3.json` / `QFT_UpdateQuestLocation_C_10`); `Icthlarin.Mirror` (revealed, `QF_UmS_MQ_Beat3.json` / `QFT_UpdateQuestLocation_C_11`); `FirstGarou.CampNPC` (hidden/removed, `QF_UmS_MQ_Beat3.json` / `QFT_UpdateQuestLocation_C_12`); `FirstGarou.CampNPC` (hidden/removed, `QF_UmS_MQ_Beat3.json` / `QFT_UpdateQuestLocation_C_4`); `CampLeader_A_Target` (hidden/removed, `QF_UmS_MQ_Beat3.json` / `QFT_UpdateQuestLocation_C_5`); `Icthlarin.Mirror` (revealed, `QF_UmS_MQ_Beat3.json` / `QFT_UpdateQuestLocation_C_8`); `TribeB.Camp` (revealed, `QF_UmS_MQ_Beat3.json` / `QFT_UpdateQuestLocation_C_9`)
- Marker 1: **Ancamairon — Alcarrid Elder** (`location`) — map `0.62743857, 0.54721181`; world `252449.2, 112143.96, 3231.601`; shared-coordinate group [S75](#s75)
  - Determination: reviewed objective relationship to extracted game quest marker 'CampLeader_A_Target': The quest and walkthrough require lenses from Alcarrid, Manafem, and Uzzer. Cooked camp locations and elder placements distinguish the active camp copies from phase duplicates and unrelated mirror actors.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Dúshlánach — Manafem Elder** (`location`) — map `0.5293431, 0.7113979`; world `211249.1, 181102.12, -1099.4247`; shared-coordinate group [S47](#s47)
  - Determination: reviewed objective relationship to extracted game registry landmark 'E82A10BB-4F0B9238-2D9DA6AA-46AD12F5': The quest and walkthrough require lenses from Alcarrid, Manafem, and Uzzer. Cooked camp locations and elder placements distinguish the active camp copies from phase duplicates and unrelated mirror actors.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Gaimbín — Uzzer Elder** (`location`) — map `0.87553995, 0.56231179`; world `356651.78, 118485.95, 2137.6094`; shared-coordinate group [S98](#s98)
  - Determination: reviewed objective relationship to extracted game registry landmark '4CD11574-43227F44-A9FB52BA-94B5237B': The quest and walkthrough require lenses from Alcarrid, Manafem, and Uzzer. Cooked camp locations and elder placements distinguish the active camp copies from phase duplicates and unrelated mirror actors.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 5. Enter the Fight Caves and defeat Fuzan

- Objective key: `FightFuzan`
- Destination mode: `fixed`
- Confidence: **High (97/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Display note: Fuzan is inside the Fight Caves activity; the world map therefore marks its overworld entrance.
- Reviewed relationship: **yes**
- Cooked location operations: `FightFuzan` (revealed, `QF_UmS_MQ_Beat5.json` / `QFT_UpdateQuestLocation_C_42`)
- Marker 1: **Fight Caves entrance** (`dungeon_entrance`) — map `0.78498719, 0.83871283`; world `318619.62, 234574.39, -2342.94`; shared-coordinate group [S90](#s90)
  - Determination: reviewed objective relationship to extracted game dungeon entrance 'FightCaves': Both objective states explicitly require entering and completing the Fight Caves. Interior encounter coordinates are not published on the overworld map.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 97/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 6. Bring Fuzan's head to Icthlarin at the Mirror Relay

- Objective key: `ReturnToIcthlarin`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: `FightFuzan` (hidden/removed, `QF_UmS_MQ_Beat5.json` / `QFT_UpdateQuestLocation_C_2`); `Icthlarin_Mirror_Target` (revealed, `QF_UmS_MQ_Beat5.json` / `QFT_UpdateQuestLocation_C_21`); `Icthlarin.Mirror` (hidden/removed, `QF_UmS_MQ_Beat5.json` / `QFT_UpdateQuestLocation_C_9`)
- Marker 1: **Icthlarin — Mirror Relay** (`location`) — map `0.62650555, 0.54707266`; world `252057.33, 112085.516, 3260.089`; shared-coordinate group [S74](#s74)
  - Determination: reviewed objective relationship to extracted game quest marker 'Icthlarin_Mirror_Target': Both objectives explicitly name Icthlarin at the Mirror Relay; Icthlarin_Mirror_Target is the exact quest-owned conversation target for that relay phase.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 7. Gain the trust of the Alcarrid Camp

- Objective key: `FirstGarouEarnReputation.NoReputation`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: Complete the three Alcarrid trust tasks in any available order. Reputation alone does not identify which task remains, so all three authored task-giver targets stay visible.
- Reviewed relationship: **yes**
- Cooked location operations: `FirstGarou.CampNPC` (revealed, `QF_UmS_MQ_Beat3.json` / `QFT_UpdateQuestLocation_C_1`); `TribeA.Taskgiver1` (revealed, `QF_UmS_MQ_Beat3.json` / `QFT_UpdateQuestLocation_C_13`); `TribeA.Taskgiver2` (revealed, `QF_UmS_MQ_Beat3.json` / `QFT_UpdateQuestLocation_C_14`); `TribeA.Taskgiver3` (revealed, `QF_UmS_MQ_Beat3.json` / `QFT_UpdateQuestLocation_C_15`); `CampLeader_A_Target` (hidden/removed, `QF_UmS_MQ_Beat3.json` / `QFT_UpdateQuestLocation_C_6`)
- Marker 1: **Valas — The Mirror Crack'd** (`location`) — map `0.60923162, 0.55673445`; world `244802.28, 116143.47, 3257.1028`; shared-coordinate group [S67](#s67)
  - Determination: reviewed objective relationship to extracted game quest marker 'TribeA.Taskgiver1': The cooked reputation flow exposes three independently named Alcarrid task-giver locations. The public quest state provides only the reputation tier, not per-task completion flags.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Domri — Rogue Trader** (`location`) — map `0.61753333, 0.54792393`; world `248289.0, 112443.05, 2686.7534`; shared-coordinate group [S68](#s68)
  - Determination: reviewed objective relationship to extracted game quest marker 'TribeA.Taskgiver2': The cooked reputation flow exposes three independently named Alcarrid task-giver locations. The public quest state provides only the reputation tier, not per-task completion flags.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Lagra — Rag and Bone Garou** (`location`) — map `0.6262666, 0.55770167`; world `251956.97, 116549.7, 3058.5261`; shared-coordinate group [S73](#s73)
  - Determination: reviewed objective relationship to extracted game quest marker 'TribeA.Taskgiver3': The cooked reputation flow exposes three independently named Alcarrid task-giver locations. The public quest state provides only the reputation tier, not per-task completion flags.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 8. Gain the trust of the Alcarrid Camp

- Objective key: `FirstGarouEarnReputation.LowReputation`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: Complete the three Alcarrid trust tasks in any available order. Reputation alone does not identify which task remains, so all three authored task-giver targets stay visible.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Valas — The Mirror Crack'd** (`location`) — map `0.60923162, 0.55673445`; world `244802.28, 116143.47, 3257.1028`; shared-coordinate group [S67](#s67)
  - Determination: reviewed objective relationship to extracted game quest marker 'TribeA.Taskgiver1': The cooked reputation flow exposes three independently named Alcarrid task-giver locations. The public quest state provides only the reputation tier, not per-task completion flags.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Domri — Rogue Trader** (`location`) — map `0.61753333, 0.54792393`; world `248289.0, 112443.05, 2686.7534`; shared-coordinate group [S68](#s68)
  - Determination: reviewed objective relationship to extracted game quest marker 'TribeA.Taskgiver2': The cooked reputation flow exposes three independently named Alcarrid task-giver locations. The public quest state provides only the reputation tier, not per-task completion flags.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Lagra — Rag and Bone Garou** (`location`) — map `0.6262666, 0.55770167`; world `251956.97, 116549.7, 3058.5261`; shared-coordinate group [S73](#s73)
  - Determination: reviewed objective relationship to extracted game quest marker 'TribeA.Taskgiver3': The cooked reputation flow exposes three independently named Alcarrid task-giver locations. The public quest state provides only the reputation tier, not per-task completion flags.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 9. Gain the trust of the Alcarrid Camp

- Objective key: `FirstGarouEarnReputation.MediumReputation`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: Complete the three Alcarrid trust tasks in any available order. Reputation alone does not identify which task remains, so all three authored task-giver targets stay visible.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Valas — The Mirror Crack'd** (`location`) — map `0.60923162, 0.55673445`; world `244802.28, 116143.47, 3257.1028`; shared-coordinate group [S67](#s67)
  - Determination: reviewed objective relationship to extracted game quest marker 'TribeA.Taskgiver1': The cooked reputation flow exposes three independently named Alcarrid task-giver locations. The public quest state provides only the reputation tier, not per-task completion flags.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Domri — Rogue Trader** (`location`) — map `0.61753333, 0.54792393`; world `248289.0, 112443.05, 2686.7534`; shared-coordinate group [S68](#s68)
  - Determination: reviewed objective relationship to extracted game quest marker 'TribeA.Taskgiver2': The cooked reputation flow exposes three independently named Alcarrid task-giver locations. The public quest state provides only the reputation tier, not per-task completion flags.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Lagra — Rag and Bone Garou** (`location`) — map `0.6262666, 0.55770167`; world `251956.97, 116549.7, 3058.5261`; shared-coordinate group [S73](#s73)
  - Determination: reviewed objective relationship to extracted game quest marker 'TribeA.Taskgiver3': The cooked reputation flow exposes three independently named Alcarrid task-giver locations. The public quest state provides only the reputation tier, not per-task completion flags.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 10. Gain the trust of the Alcarrid Camp

- Objective key: `FirstGarouEarnReputation.HighReputation`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: Complete the three Alcarrid trust tasks in any available order. Reputation alone does not identify which task remains, so all three authored task-giver targets stay visible.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Valas — The Mirror Crack'd** (`location`) — map `0.60923162, 0.55673445`; world `244802.28, 116143.47, 3257.1028`; shared-coordinate group [S67](#s67)
  - Determination: reviewed objective relationship to extracted game quest marker 'TribeA.Taskgiver1': The cooked reputation flow exposes three independently named Alcarrid task-giver locations. The public quest state provides only the reputation tier, not per-task completion flags.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Domri — Rogue Trader** (`location`) — map `0.61753333, 0.54792393`; world `248289.0, 112443.05, 2686.7534`; shared-coordinate group [S68](#s68)
  - Determination: reviewed objective relationship to extracted game quest marker 'TribeA.Taskgiver2': The cooked reputation flow exposes three independently named Alcarrid task-giver locations. The public quest state provides only the reputation tier, not per-task completion flags.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Lagra — Rag and Bone Garou** (`location`) — map `0.6262666, 0.55770167`; world `251956.97, 116549.7, 3058.5261`; shared-coordinate group [S73](#s73)
  - Determination: reviewed objective relationship to extracted game quest marker 'TribeA.Taskgiver3': The cooked reputation flow exposes three independently named Alcarrid task-giver locations. The public quest state provides only the reputation tier, not per-task completion flags.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 11. You have completed the tribe's tasks. Return to the Alcarrid Elder

- Objective key: `FirstGarouEarnReputation.MaxReputation`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: `FirstGarou.CampNPC` (hidden/removed, `QF_UmS_MQ_Beat3.json` / `QFT_UpdateQuestLocation_C_2`); `CampLeader_A_Target` (revealed, `QF_UmS_MQ_Beat3.json` / `QFT_UpdateQuestLocation_C_3`); `FirstGarou.CampNPC` (hidden/removed, `QF_UmS_TribeA_Task2_RogueTrader.json` / `QFT_UpdateQuestLocation_C_10`); `CampLeader_A_Target` (revealed, `QF_UmS_TribeA_Task2_RogueTrader.json` / `QFT_UpdateQuestLocation_C_11`)
- Marker 1: **Ancamairon — Alcarrid Elder** (`location`) — map `0.62743857, 0.54721181`; world `252449.2, 112143.96, 3231.601`; shared-coordinate group [S75](#s75)
  - Determination: reviewed objective relationship to extracted game quest marker 'CampLeader_A_Target': This state explicitly says the Alcarrid tasks are complete and directs the player back to the elder; CampLeader_A_Target is the cooked Ancamairon interaction for that hand-in.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 12. Speak to Icthlarin at the Mirror Relay

- Objective key: `SpeakToIcthlarin`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: `Icthlarin_Mirror_Target` (revealed, `QF_UmS_MQ_Beat2.json` / `QFT_UpdateQuestLocation_C_11`); `GarouCamp_A_Area` (hidden/removed, `QF_UmS_MQ_Beat2.json` / `QFT_UpdateQuestLocation_C_12`); `Bastion_DoorToUmS_Target` (hidden/removed, `QF_UmS_MQ_Beat2.json` / `QFT_UpdateQuestLocation_C_5`); `FirstGarouInCave_Target` (hidden/removed, `QF_UmS_MQ_Beat2.json` / `QFT_UpdateQuestLocation_C_6`); `Icthlarin_Mirror_Target` (revealed, `QF_UmS_MQ_Beat4.json` / `QFT_UpdateQuestLocation_C_21`); `Icthlarin.Mirror` (hidden/removed, `QF_UmS_MQ_Beat4.json` / `QFT_UpdateQuestLocation_C_9`)
- Marker 1: **Icthlarin — Mirror Relay** (`location`) — map `0.62650555, 0.54707266`; world `252057.33, 112085.516, 3260.089`; shared-coordinate group [S74](#s74)
  - Determination: reviewed objective relationship to extracted game quest marker 'Icthlarin_Mirror_Target': Both objectives explicitly name Icthlarin at the Mirror Relay; Icthlarin_Mirror_Target is the exact quest-owned conversation target for that relay phase.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 13. Speak to the Alcarrid Elder and complete tasks to earn their trust

- Objective key: `SpeakToGarouElder`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: `GarouCamp_A_Area` (hidden/removed, `QF_UmS_MQ_Icthlarin.json` / `QFT_UpdateQuestLocation_C_12`); `CampLeader_A_Target` (revealed, `QF_UmS_MQ_Icthlarin.json` / `QFT_UpdateQuestLocation_C_6`); `Icthlarin_Mirror_Target` (hidden/removed, `QF_UmS_MQ_Icthlarin.json` / `QFT_UpdateQuestLocation_C_8`)
- Marker 1: **Ancamairon — Alcarrid Elder** (`location`) — map `0.62743857, 0.54721181`; world `252449.2, 112143.96, 3231.601`; shared-coordinate group [S75](#s75)
  - Determination: reviewed objective relationship to extracted game quest marker 'CampLeader_A_Target': The objective explicitly names the Alcarrid Elder and the compiled Icthlarin flow reveals CampLeader_A_Target for this beat.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 14. Place the three Mirror Lenses into the Mirror Relay

- Objective key: `RepairTheMirrors`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: Place the three clan lenses into the three separate sockets at the Alcarrid Mirror Relay.
- Reviewed relationship: **yes**
- Cooked location operations: `UmS_TribeA_Mirror_1_Target` (revealed, `QF_UmS_MQ_Icthlarin.json` / `QFT_UpdateQuestLocation_C_53`); `Icthlarin_Mirror_Target` (hidden/removed, `QF_UmS_MQ_Icthlarin.json` / `QFT_UpdateQuestLocation_C_54`); `UmS_TribeA_Mirror_2_Target` (revealed, `QF_UmS_MQ_Icthlarin.json` / `QFT_UpdateQuestLocation_C_55`); `UmS_TribeA_Mirror_3_Target` (revealed, `QF_UmS_MQ_Icthlarin.json` / `QFT_UpdateQuestLocation_C_56`); `Icthlarin.Mirror` (revealed, `QF_UmS_MQ_Icthlarin.json` / `QFT_UpdateQuestLocation_C_57`)
- Marker 1: **Mirror Relay — lens socket 1** (`location`) — map `0.62676105, 0.54833883`; world `252164.64, 112617.31, 3062.5989`
  - Determination: reviewed objective relationship to extracted game quest marker 'UmS_TribeA_Mirror_1_Target': The compiled quest flow reveals three independently authored mirror targets for this objective; their placements correspond to the three physical sockets rather than the nearby Icthlarin conversation marker.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Mirror Relay — lens socket 2** (`location`) — map `0.62776336, 0.54833369`; world `252585.61, 112615.15, 3059.5735`
  - Determination: reviewed objective relationship to extracted game quest marker 'UmS_TribeA_Mirror_2_Target': The compiled quest flow reveals three independently authored mirror targets for this objective; their placements correspond to the three physical sockets rather than the nearby Icthlarin conversation marker.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Mirror Relay — lens socket 3** (`location`) — map `0.62625588, 0.54920488`; world `251952.47, 112981.05, 3059.7952`
  - Determination: reviewed objective relationship to extracted game quest marker 'UmS_TribeA_Mirror_3_Target': The compiled quest flow reveals three independently authored mirror targets for this objective; their placements correspond to the three physical sockets rather than the nearby Icthlarin conversation marker.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 15. Journey through the Fight Caves and slay Fuzan

- Objective key: `SlayFuzan`
- Destination mode: `fixed`
- Confidence: **High (97/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Display note: Fuzan is inside the Fight Caves activity; the world map therefore marks its overworld entrance.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Fight Caves entrance** (`dungeon_entrance`) — map `0.78498719, 0.83871283`; world `318619.62, 234574.39, -2342.94`; shared-coordinate group [S90](#s90)
  - Determination: reviewed objective relationship to extracted game dungeon entrance 'FightCaves': Both objective states explicitly require entering and completing the Fight Caves. Interior encounter coordinates are not published on the overworld map.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 97/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 16. Look for signs of life in the desert

- Objective key: `LookForLife`
- Destination mode: `area`
- Confidence: **High (91/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. The game authors an area rather than one exact interaction point.
- Reviewed relationship: **yes**
- Cooked location operations: `Bastion_DoorToUmS_Target` (hidden/removed, `QF_UmS_MQ_Beat1.json` / `QFT_UpdateQuestLocation_C_13`); `GarouCamp_A_Area` (revealed, `QF_UmS_MQ_Beat1.json` / `QFT_UpdateQuestLocation_C_14`)
- Marker 1: **Signs of life — Alcarrid camp search area** (`area`) — map `0.62376876, 0.55132352`; world `250907.88, 113870.88, 3361.1914`; shared-coordinate group [S71](#s71)
  - Determination: reviewed objective relationship to extracted game quest marker 'GarouCamp_A_Area': Immediately after entry, the flow hides Bastion_DoorToUmS_Target and reveals only GarouCamp_A_Area. The guide sequence confirms the player searches for life toward the first Moon Garou camp rather than returning to the Bastion door.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 17. Enter the Umbral Sands

- Objective key: `EnterUmS`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: `Bastion_DoorToUmS_Target` (revealed, `QF_UmS_MQ_Beat1.json` / `QFT_UpdateQuestLocation_C_0`)
- Marker 1: **Door to Umbral Sands — Bastion** (`location`) — map `0.58914193, 0.47370564`; world `236364.61, 81271.37, 7391.934`
  - Determination: reviewed objective relationship to extracted game quest marker 'Bastion_DoorToUmS_Target': The first objective reveals exactly Bastion_DoorToUmS_Target at the door inside the Bastion boss room. It is the authored transition into Umbral Sands.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 18. Complete tasks for the final camp and gain their Mirror Lens

- Objective key: `FindLastKey`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: The saved objective does not identify whether Manafem or Uzzer is the player's final unfinished clan, so both exact elder locations remain visible.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Dúshlánach — Manafem Elder** (`location`) — map `0.5293431, 0.7113979`; world `211249.1, 181102.12, -1099.4247`; shared-coordinate group [S47](#s47)
  - Determination: reviewed objective relationship to extracted game registry landmark 'E82A10BB-4F0B9238-2D9DA6AA-46AD12F5': Alcarrid is completed before this late aggregate state. The final outstanding lens may be earned from either Manafem or Uzzer, and no per-clan completion identity is exposed by this objective key.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Gaimbín — Uzzer Elder** (`location`) — map `0.87553995, 0.56231179`; world `356651.78, 118485.95, 2137.6094`; shared-coordinate group [S98](#s98)
  - Determination: reviewed objective relationship to extracted game registry landmark '4CD11574-43227F44-A9FB52BA-94B5237B': Alcarrid is completed before this late aggregate state. The final outstanding lens may be earned from either Manafem or Uzzer, and no per-clan completion identity is exposed by this objective key.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

### Beast in the Sand

Side quest · Umbral Sands

#### 1. Track down the Dearg Due through the sandstorm

- Objective key: `FollowBeast`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: The public objective key is reused while the Dearg Due moves through the sandstorm, so all four authored trail stages are shown as the route.
- Reviewed relationship: **yes**
- Cooked location operations: `TribeC.Task1.Mirage1` (revealed, `QF_UmS_MainQuest_TribeC_Task1_MirageBeast.json` / `QFT_UpdateQuestLocation_C_1`); `TribeC.Taskgiver1` (hidden/removed, `QF_UmS_MainQuest_TribeC_Task1_MirageBeast.json` / `QFT_UpdateQuestLocation_C_8`); `TribeC_Taskgiver1_NPC` (revealed, `QF_UmS_MainQuest_TribeC_Task1_MirageBeast.json` / `QFT_UpdateQuestLocation_C_9`)
- Marker 1: **Dearg Due trail — first sighting** (`area`) — map `0.50931405, 0.81768888`; world `202836.9, 225744.33, 1882.3102`
  - Determination: reviewed objective relationship to extracted game quest marker 'TribeC.Task1.Mirage1': The compiled quest flow advances and hides the four quest-owned Mirage1 through Mirage4 locations in sequence. The quest guide independently confirms three howl stops followed by Efina's hiding place. The task-giver marker exposed later in the same flow is a return-state target and is excluded here.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Dearg Due trail — second sighting** (`area`) — map `0.51626071, 0.86190607`; world `205754.5, 244315.55, 1309.7556`
  - Determination: reviewed objective relationship to extracted game quest marker 'TribeC.Task1.Mirage2': The compiled quest flow advances and hides the four quest-owned Mirage1 through Mirage4 locations in sequence. The quest guide independently confirms three howl stops followed by Efina's hiding place. The task-giver marker exposed later in the same flow is a return-state target and is excluded here.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Dearg Due trail — third sighting** (`area`) — map `0.56005707, 0.86169595`; world `224148.97, 244227.3, 997.8026`
  - Determination: reviewed objective relationship to extracted game quest marker 'TribeC.Task1.Mirage3': The compiled quest flow advances and hides the four quest-owned Mirage1 through Mirage4 locations in sequence. The quest guide independently confirms three howl stops followed by Efina's hiding place. The task-giver marker exposed later in the same flow is a return-state target and is excluded here.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **Efina's hiding place** (`area`) — map `0.58590019, 0.87198817`; world `235003.08, 248550.03, -28.8816`
  - Determination: reviewed objective relationship to extracted game quest marker 'TribeC.Task1.Mirage4': The compiled quest flow advances and hides the four quest-owned Mirage1 through Mirage4 locations in sequence. The quest guide independently confirms three howl stops followed by Efina's hiding place. The task-giver marker exposed later in the same flow is a return-state target and is excluded here.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 2. Give Efina - The Lost some clean water

- Objective key: `GiveWater`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Efina — The Lost** (`location`) — map `0.58589895, 0.87198638`; world `235002.56, 248549.28, -112.959946`
  - Determination: reviewed objective relationship to extracted game registry landmark '2736394F-4613C7C2-566C60B3-DFA40E46': The objective explicitly names Efina, and the world registry contains one exact Efina - The Lost NPC placement at the final trail destination.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 3. Return to Manathep - The Bereaved

- Objective key: `Return`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: `TribeC.Task1.Mirage3` (hidden/removed, `QF_UmS_MainQuest_TribeC_Task1_MirageBeast.json` / `QFT_UpdateQuestLocation_C_10`); `TribeC.Task1.Mirage4` (hidden/removed, `QF_UmS_MainQuest_TribeC_Task1_MirageBeast.json` / `QFT_UpdateQuestLocation_C_11`)
- Marker 1: **Manathep — The Bereaved** (`location`) — map `0.53277295, 0.71605795`; world `212689.64, 183059.34, -1509.8046`
  - Determination: reviewed objective relationship to extracted game registry landmark '66CC10A1-43D25A90-6B505FAB-74A2DDC2': The objective explicitly names Manathep, and the world registry contains one exact Manathep - The Bereaved NPC placement in the Manafem camp.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

### Brink of Extinction

Side quest · Umbral Sands

#### 1. Use the KotHaar numbering book to get past the KotHaar Bouncer

- Objective key: `Objective5`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **KotHaar Bouncer** (`location`) — map `0.68482388, 0.72591202`; world `276551.03, 187198.05, -2320.4062`
  - Determination: reviewed objective relationship to extracted game registry landmark 'A13E380A-459F8CF7-13BBA1A7-927DDAD3': The objective explicitly requires using the numbering book at the KotHaar Bouncer, and the registry contains one exact quest-owned Bouncer identity; the other candidate is its colocated quest target.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 2. Bring 1 cactus steak and 3 raw beltfish back to Zi in Umbral Sands

- Objective key: `Objective1`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: `Zi` (revealed, `QF_BrinkOfExtinction.json` / `QFT_UpdateQuestLocation_C_1`)
- Marker 1: **Zi — food hand-in** (`npc`) — map `0.75473438, 0.74288267`; world `305913.44, 194325.72, -2830.78`
  - Determination: reviewed objective relationship to extracted game quest marker 'Zi': The objective is an item hand-in to Zi and the compiled flow reveals the quest-owned Zi marker. Unrelated NPCs previously introduced by loose name matching are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 3. Touch 4 pedestals in any order in THE GAUNTLET OF FIRE - 0/4

- Objective key: `Objective2`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: Touch the four Burning Spire pedestals in any order. The 0/4 counter does not identify which pedestal will be used first, so all four valid interactions remain visible.
- Reviewed relationship: **yes**
- Cooked location operations: `UmS_PedestalRadius` (revealed, `QF_BrinkOfExtinction.json` / `QFT_UpdateQuestLocation_C_5`)
- Marker 1: **Fire Pedestal — north of Sinking Pyramid** (`location`) — map `0.73979762, 0.70717857`; world `299640.0, 179330.0, -1758.9911`; shared-coordinate group [S83](#s83)
  - Determination: reviewed objective relationship to extracted game registry landmark '6427174B-49675037-E713B9AB-3C3C119D': The Brink of Extinction string table owns exactly four Pedestal registry identities inside the authored UmS_PedestalRadius. Their current-build coordinates match the guide's north, east, south, and west positions around the Sinking Pyramid.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Fire Pedestal — east rock formation** (`location`) — map `0.75844048, 0.73013095`; world `307470.0, 188970.0, -1199.2651`; shared-coordinate group [S87](#s87)
  - Determination: reviewed objective relationship to extracted game registry landmark '4A50CA20-4FF14D60-F08C37AA-81320C2B': The Brink of Extinction string table owns exactly four Pedestal registry identities inside the authored UmS_PedestalRadius. Their current-build coordinates match the guide's north, east, south, and west positions around the Sinking Pyramid.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Fire Pedestal — south of Sinking Pyramid** (`location`) — map `0.74103824, 0.75162964`; world `300161.06, 197999.45, -3040.3394`; shared-coordinate group [S84](#s84)
  - Determination: reviewed objective relationship to extracted game registry landmark 'CF2EE897-45637E57-6824AEA3-26B04CCB': The Brink of Extinction string table owns exactly four Pedestal registry identities inside the authored UmS_PedestalRadius. Their current-build coordinates match the guide's north, east, south, and west positions around the Sinking Pyramid.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **Fire Pedestal — west of Sinking Pyramid** (`location`) — map `0.71284524, 0.72415476`; world `288320.0, 186460.0, -1395.8845`; shared-coordinate group [S80](#s80)
  - Determination: reviewed objective relationship to extracted game registry landmark '27A8F58C-4E95785F-89FA3792-4946CB91': The Brink of Extinction string table owns exactly four Pedestal registry identities inside the authored UmS_PedestalRadius. Their current-build coordinates match the guide's north, east, south, and west positions around the Sinking Pyramid.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 4. Touch 4 pedestals in any order in THE GAUNTLET OF FIRE - 1/4

- Objective key: `Objective2.1`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: One pedestal is complete, but the quest counter does not expose its identity. All four possible remaining interactions stay visible rather than guessing which marker to hide.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Fire Pedestal — north of Sinking Pyramid** (`location`) — map `0.73979762, 0.70717857`; world `299640.0, 179330.0, -1758.9911`; shared-coordinate group [S83](#s83)
  - Determination: reviewed objective relationship to extracted game registry landmark '6427174B-49675037-E713B9AB-3C3C119D': This is the 1/4 counter state of the same freely ordered four-pedestal objective; no per-pedestal completion booleans are available in the public snapshot.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Fire Pedestal — east rock formation** (`location`) — map `0.75844048, 0.73013095`; world `307470.0, 188970.0, -1199.2651`; shared-coordinate group [S87](#s87)
  - Determination: reviewed objective relationship to extracted game registry landmark '4A50CA20-4FF14D60-F08C37AA-81320C2B': This is the 1/4 counter state of the same freely ordered four-pedestal objective; no per-pedestal completion booleans are available in the public snapshot.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Fire Pedestal — south of Sinking Pyramid** (`location`) — map `0.74103824, 0.75162964`; world `300161.06, 197999.45, -3040.3394`; shared-coordinate group [S84](#s84)
  - Determination: reviewed objective relationship to extracted game registry landmark 'CF2EE897-45637E57-6824AEA3-26B04CCB': This is the 1/4 counter state of the same freely ordered four-pedestal objective; no per-pedestal completion booleans are available in the public snapshot.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **Fire Pedestal — west of Sinking Pyramid** (`location`) — map `0.71284524, 0.72415476`; world `288320.0, 186460.0, -1395.8845`; shared-coordinate group [S80](#s80)
  - Determination: reviewed objective relationship to extracted game registry landmark '27A8F58C-4E95785F-89FA3792-4946CB91': This is the 1/4 counter state of the same freely ordered four-pedestal objective; no per-pedestal completion booleans are available in the public snapshot.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 5. Touch 4 pedestals in any order in THE GAUNTLET OF FIRE - 2/4

- Objective key: `Objective2.2`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: Two pedestals are complete, but the quest counter does not expose their identities. All four possible remaining interactions stay visible rather than guessing which markers to hide.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Fire Pedestal — north of Sinking Pyramid** (`location`) — map `0.73979762, 0.70717857`; world `299640.0, 179330.0, -1758.9911`; shared-coordinate group [S83](#s83)
  - Determination: reviewed objective relationship to extracted game registry landmark '6427174B-49675037-E713B9AB-3C3C119D': This is the 2/4 counter state of the same freely ordered four-pedestal objective; no per-pedestal completion booleans are available in the public snapshot.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Fire Pedestal — east rock formation** (`location`) — map `0.75844048, 0.73013095`; world `307470.0, 188970.0, -1199.2651`; shared-coordinate group [S87](#s87)
  - Determination: reviewed objective relationship to extracted game registry landmark '4A50CA20-4FF14D60-F08C37AA-81320C2B': This is the 2/4 counter state of the same freely ordered four-pedestal objective; no per-pedestal completion booleans are available in the public snapshot.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Fire Pedestal — south of Sinking Pyramid** (`location`) — map `0.74103824, 0.75162964`; world `300161.06, 197999.45, -3040.3394`; shared-coordinate group [S84](#s84)
  - Determination: reviewed objective relationship to extracted game registry landmark 'CF2EE897-45637E57-6824AEA3-26B04CCB': This is the 2/4 counter state of the same freely ordered four-pedestal objective; no per-pedestal completion booleans are available in the public snapshot.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **Fire Pedestal — west of Sinking Pyramid** (`location`) — map `0.71284524, 0.72415476`; world `288320.0, 186460.0, -1395.8845`; shared-coordinate group [S80](#s80)
  - Determination: reviewed objective relationship to extracted game registry landmark '27A8F58C-4E95785F-89FA3792-4946CB91': This is the 2/4 counter state of the same freely ordered four-pedestal objective; no per-pedestal completion booleans are available in the public snapshot.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 6. Return to Zi in the Burning Spire region of the Umbral Sands

- Objective key: `Objective3`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: `UmS_PedestalRadius` (hidden/removed, `QF_BrinkOfExtinction.json` / `QFT_UpdateQuestLocation_C_6`)
- Marker 1: **Zi** (`npc`) — map `0.75520133, 0.742969`; world `306109.56, 194361.98, -2869.65`
  - Determination: reviewed objective relationship to extracted game npc 'zi': The objective explicitly returns to Zi in the Burning Spire and the extracted NPC registry contains one uniquely placed Zi.
  - Source: `web/npc-catalog.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 7. Touch 4 pedestals in any order in THE GAUNTLET OF FIRE - 3/4

- Objective key: `Objective2.3`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: Three pedestals are complete, but the quest counter does not expose which one remains. All four possible interactions stay visible rather than selecting an unsupported final marker.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Fire Pedestal — north of Sinking Pyramid** (`location`) — map `0.73979762, 0.70717857`; world `299640.0, 179330.0, -1758.9911`; shared-coordinate group [S83](#s83)
  - Determination: reviewed objective relationship to extracted game registry landmark '6427174B-49675037-E713B9AB-3C3C119D': This is the 3/4 counter state of the same freely ordered four-pedestal objective; no per-pedestal completion booleans are available in the public snapshot.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Fire Pedestal — east rock formation** (`location`) — map `0.75844048, 0.73013095`; world `307470.0, 188970.0, -1199.2651`; shared-coordinate group [S87](#s87)
  - Determination: reviewed objective relationship to extracted game registry landmark '4A50CA20-4FF14D60-F08C37AA-81320C2B': This is the 3/4 counter state of the same freely ordered four-pedestal objective; no per-pedestal completion booleans are available in the public snapshot.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Fire Pedestal — south of Sinking Pyramid** (`location`) — map `0.74103824, 0.75162964`; world `300161.06, 197999.45, -3040.3394`; shared-coordinate group [S84](#s84)
  - Determination: reviewed objective relationship to extracted game registry landmark 'CF2EE897-45637E57-6824AEA3-26B04CCB': This is the 3/4 counter state of the same freely ordered four-pedestal objective; no per-pedestal completion booleans are available in the public snapshot.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **Fire Pedestal — west of Sinking Pyramid** (`location`) — map `0.71284524, 0.72415476`; world `288320.0, 186460.0, -1395.8845`; shared-coordinate group [S80](#s80)
  - Determination: reviewed objective relationship to extracted game registry landmark '27A8F58C-4E95785F-89FA3792-4946CB91': This is the 3/4 counter state of the same freely ordered four-pedestal objective; no per-pedestal completion booleans are available in the public snapshot.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 8. Use the password to enter the Cave of Screams, within the Burning Spire region

- Objective key: `Objective4`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Display note: The bouncer stands at the Cave of Screams entrance and controls access; the marker identifies that exterior interaction.
- Reviewed relationship: **yes**
- Cooked location operations: `Bouncer` (revealed, `QF_BrinkOfExtinction.json` / `QFT_UpdateQuestLocation_C_0`); `Zi` (hidden/removed, `QF_BrinkOfExtinction.json` / `QFT_UpdateQuestLocation_C_17`); `Bouncer` (hidden/removed, `QF_BrinkOfExtinction.json` / `QFT_UpdateQuestLocation_C_2`); `CaveofScreamsDoor` (revealed, `QF_BrinkOfExtinction.json` / `QFT_UpdateQuestLocation_C_3`); `UmS_PedestalRadius` (hidden/removed, `QF_BrinkOfExtinction.json` / `QFT_UpdateQuestLocation_C_7`)
- Marker 1: **KotHaar Bouncer — Cave of Screams entrance** (`npc`) — map `0.68487595, 0.72604536`; world `276572.9, 187254.05, -2312.7302`; shared-coordinate group [S79](#s79)
  - Determination: reviewed objective relationship to extracted game quest marker 'Bouncer': The compiled flow reveals the quest-owned Bouncer marker for the password stage, and the guide confirms the password is spoken to the bouncer outside the cave.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 9. Return to the KotHaar Bouncer outside of the Cave of Screams

- Objective key: `Objective6`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: `Bouncer` (revealed, `QF_BrinkOfExtinction.json` / `QFT_UpdateQuestLocation_C_4`); `CaveofScreamsDoor` (hidden/removed, `QF_BrinkOfExtinction.json` / `QFT_UpdateQuestLocation_C_8`)
- Marker 1: **KotHaar Bouncer — final return** (`npc`) — map `0.68487595, 0.72604536`; world `276572.9, 187254.05, -2312.7302`; shared-coordinate group [S79](#s79)
  - Determination: reviewed objective relationship to extracted game quest marker 'Bouncer': The final objective explicitly returns to the KotHaar Bouncer and the flow re-reveals the same exact quest-owned Bouncer marker after the cave encounter.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

### Contact!

Side quest · Umbral Sands

#### 1. Find the Vault Guardian Prime in the Kalphite Hive

- Objective key: `Objective1`
- Destination mode: `fixed`
- Confidence: **High (97/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Display note: Vault Guardian Prime is inside the Kalphite Hive; the world map therefore marks the dungeon entrance.
- Reviewed relationship: **yes**
- Cooked location operations: `VaultGuardianSearchArea` (revealed, `QF_Contact.json` / `QFT_UpdateQuestLocation_C_1`)
- Marker 1: **Vault Guardian Prime — Kalphite Hive entrance** (`dungeon_entrance`) — map `0.77502262, 0.34840311`; world `314434.5, 28644.307, 1852.626`; shared-coordinate group [S89](#s89)
  - Determination: reviewed objective relationship to extracted game dungeon entrance 'KalphiteHive': The placed BP_NPC_Quest_Contact_VaultGuardian_Leader actor proves the NPC is inside the hive cave, and the current world independently exposes the Kalphite Hive overworld entrance.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 97/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 2. Find 3 missing vault guardians and recover their Crystal Helix fragments

- Objective key: `Objective2`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: The damaged guardian is reached from the Kalphite Hive entrance. The dead guardian is among the kalphite eggs, and the buried guardian must be dug up at the ruined building.
- Reviewed relationship: **yes**
- Cooked location operations: `VaultGuardianSearchArea` (hidden/removed, `QF_Contact.json` / `QFT_UpdateQuestLocation_C_10`); `VaultGuardianPrime` (revealed, `QF_Contact.json` / `QFT_UpdateQuestLocation_C_2`); `UmS_Contact_Region` (revealed, `QF_Contact.json` / `QFT_UpdateQuestLocation_C_4`)
- Marker 1: **Damaged guardian — Kalphite Hive entrance** (`dungeon_entrance`) — map `0.77502262, 0.34840311`; world `314434.5, 28644.307, 1852.626`; shared-coordinate group [S89](#s89)
  - Determination: reviewed objective relationship to extracted game dungeon entrance 'KalphiteHive': The current world contains three semantically distinct quest actors: BP_NPC_Quest_Contact_DamagedVaultGuardian_UmS inside the Prime cave, BP_NPC_Quest_Contact_DeadVaultGuardian_UmS outside by the eggs, and BP_Contact_Buried_VaultGuardian at the ruins. The walkthrough independently confirms those three interactions and their roles.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 97/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Dead Vault Guardian — kalphite eggs** (`world_actor`) — map `0.80232581, 0.35629501`; world `325901.84, 31958.906, 1823.8793`; shared-coordinate group [S93](#s93)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_NPC_Quest_Contact_DeadVaultGuardian_UmS': The current world contains three semantically distinct quest actors: BP_NPC_Quest_Contact_DamagedVaultGuardian_UmS inside the Prime cave, BP_NPC_Quest_Contact_DeadVaultGuardian_UmS outside by the eggs, and BP_Contact_Buried_VaultGuardian at the ruins. The walkthrough independently confirms those three interactions and their roles.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/8AG0WHIXYVB5RO3QNZSY49AU8.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Buried Vault Guardian — ruined building** (`world_actor`) — map `0.77151976, 0.38517525`; world `312963.3, 44088.605, 2849.3892`; shared-coordinate group [S88](#s88)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_Contact_Buried_VaultGuardian': The current world contains three semantically distinct quest actors: BP_NPC_Quest_Contact_DamagedVaultGuardian_UmS inside the Prime cave, BP_NPC_Quest_Contact_DeadVaultGuardian_UmS outside by the eggs, and BP_Contact_Buried_VaultGuardian at the ruins. The walkthrough independently confirms those three interactions and their roles.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/8AG0WHIXYVB5RO3QNZSY49AU8.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 3. Use the Crystal Helix to pass through the magical barrier in the Kalphite Hive

- Objective key: `Objective4`
- Destination mode: `fixed`
- Confidence: **High (97/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Display note: The objective interaction is inside the Kalphite Hive; the world map marks the dungeon entrance.
- Reviewed relationship: **yes**
- Cooked location operations: `VaultGuardianSearchArea` (hidden/removed, `QF_Contact.json` / `QFT_UpdateQuestLocation_C_11`); `UmS_Contact_Region` (hidden/removed, `QF_Contact.json` / `QFT_UpdateQuestLocation_C_7`); `VaultGuardianPrime` (hidden/removed, `QF_Contact.json` / `QFT_UpdateQuestLocation_C_9`)
- Marker 1: **Kalphite Hive entrance** (`dungeon_entrance`) — map `0.77502262, 0.34840311`; world `314434.5, 28644.307, 1852.626`; shared-coordinate group [S89](#s89)
  - Determination: reviewed objective relationship to extracted game dungeon entrance 'KalphiteHive': Vault Guardian Prime, the magical barrier, and the Shortcut Chamber crystals are all interior Kalphite Hive interactions. Publishing interior transforms on the overworld map would be misleading.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 97/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 4. Talk to Zanik in the Shortcut Chamber within the Kalphite Hive

- Objective key: `Objective5`
- Destination mode: `fixed`
- Confidence: **High (97/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Display note: Zanik is inside the Kalphite Hive Shortcut Chamber; the world map shows the Kalphite Hive entrance.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Zanik — Kalphite Hive entrance** (`dungeon_entrance`) — map `0.77502262, 0.34840311`; world `314434.5, 28644.307, 1852.626`; shared-coordinate group [S89](#s89)
  - Determination: reviewed objective relationship to extracted game dungeon entrance 'KalphiteHive': Both objectives explicitly locate Zanik in the interior Shortcut Chamber. The compiled flow and guide place the conversation beyond the Prime guardian's magical barrier, so the verified overworld Kalphite Hive entrance is the only truthful world-map destination.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 97/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 5. Follow the footsteps in the sand to find who made them

- Objective key: `Objective2.1`
- Destination mode: `area`
- Confidence: **High (91/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. The game authors an area rather than one exact interaction point.
- Display note: Follow the trail within the authored Vault Guardian search area.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **VaultGuardianSearchArea** (`area`) — map `0.82114471, 0.37418317`; world `333805.78, 39471.93, 1245.5339`
  - Determination: reviewed objective relationship to extracted game quest marker 'VaultGuardianSearchArea': QF_Contact updates VaultGuardianSearchArea directly below Objective2.1; the quest owns the matching placed search-area marker.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 6. Break the four crystals in the Shortcut Chamber of the Kalphite Hive

- Objective key: `Objective6`
- Destination mode: `fixed`
- Confidence: **High (97/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Display note: The objective interaction is inside the Kalphite Hive; the world map marks the dungeon entrance.
- Reviewed relationship: **yes**
- Cooked location operations: `UmS_AviskCrystals` (revealed, `QF_Contact.json` / `QFT_UpdateQuestLocation_C_0`)
- Marker 1: **Kalphite Hive entrance** (`dungeon_entrance`) — map `0.77502262, 0.34840311`; world `314434.5, 28644.307, 1852.626`; shared-coordinate group [S89](#s89)
  - Determination: reviewed objective relationship to extracted game dungeon entrance 'KalphiteHive': Vault Guardian Prime, the magical barrier, and the Shortcut Chamber crystals are all interior Kalphite Hive interactions. Publishing interior transforms on the overworld map would be misleading.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 97/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 7. Return to the Vault Guardian Prime

- Objective key: `Objective3`
- Destination mode: `fixed`
- Confidence: **High (97/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Display note: The objective interaction is inside the Kalphite Hive; the world map marks the dungeon entrance.
- Reviewed relationship: **yes**
- Cooked location operations: `VaultGuardianLeader` (revealed, `QF_Contact.json` / `QFT_UpdateQuestLocation_C_5`)
- Marker 1: **Kalphite Hive entrance** (`dungeon_entrance`) — map `0.77502262, 0.34840311`; world `314434.5, 28644.307, 1852.626`; shared-coordinate group [S89](#s89)
  - Determination: reviewed objective relationship to extracted game dungeon entrance 'KalphiteHive': Vault Guardian Prime, the magical barrier, and the Shortcut Chamber crystals are all interior Kalphite Hive interactions. Publishing interior transforms on the overworld map would be misleading.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 97/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 8. Return to Zanik in the Shortcut Chamber within the Kalphite Hive

- Objective key: `Objective7`
- Destination mode: `fixed`
- Confidence: **High (97/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Display note: Zanik is inside the Kalphite Hive Shortcut Chamber; the world map shows the Kalphite Hive entrance.
- Reviewed relationship: **yes**
- Cooked location operations: `UmS_AviskCrystals` (hidden/removed, `QF_Contact.json` / `QFT_UpdateQuestLocation_C_8`)
- Marker 1: **Zanik — Kalphite Hive entrance** (`dungeon_entrance`) — map `0.77502262, 0.34840311`; world `314434.5, 28644.307, 1852.626`; shared-coordinate group [S89](#s89)
  - Determination: reviewed objective relationship to extracted game dungeon entrance 'KalphiteHive': Both objectives explicitly locate Zanik in the interior Shortcut Chamber. The compiled flow and guide place the conversation beyond the Prime guardian's magical barrier, so the verified overworld Kalphite Hive entrance is the only truthful world-map destination.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 97/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 9. Find Crystal Helix fragments. 1/3

- Objective key: `CrystalShard.1`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: The aggregate fragment count does not identify which guardian source remains, so all three proven sources stay visible.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Damaged guardian — Kalphite Hive entrance** (`dungeon_entrance`) — map `0.77502262, 0.34840311`; world `314434.5, 28644.307, 1852.626`; shared-coordinate group [S89](#s89)
  - Determination: reviewed objective relationship to extracted game dungeon entrance 'KalphiteHive': The Crystal Helix fragments come from the damaged guardian inside the Hive, the dead guardian by the eggs, and the buried guardian at the ruins; these are the same three independently placed interactions established by the preceding search state.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 97/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Dead Vault Guardian — kalphite eggs** (`world_actor`) — map `0.80232581, 0.35629501`; world `325901.84, 31958.906, 1823.8793`; shared-coordinate group [S93](#s93)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_NPC_Quest_Contact_DeadVaultGuardian_UmS': The Crystal Helix fragments come from the damaged guardian inside the Hive, the dead guardian by the eggs, and the buried guardian at the ruins; these are the same three independently placed interactions established by the preceding search state.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/8AG0WHIXYVB5RO3QNZSY49AU8.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Buried Vault Guardian — ruined building** (`world_actor`) — map `0.77151976, 0.38517525`; world `312963.3, 44088.605, 2849.3892`; shared-coordinate group [S88](#s88)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_Contact_Buried_VaultGuardian': The Crystal Helix fragments come from the damaged guardian inside the Hive, the dead guardian by the eggs, and the buried guardian at the ruins; these are the same three independently placed interactions established by the preceding search state.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/8AG0WHIXYVB5RO3QNZSY49AU8.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 10. Find Crystal Helix fragments. 2/3

- Objective key: `CrystalShard.2`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: The aggregate fragment count does not identify which guardian source remains, so all three proven sources stay visible.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Damaged guardian — Kalphite Hive entrance** (`dungeon_entrance`) — map `0.77502262, 0.34840311`; world `314434.5, 28644.307, 1852.626`; shared-coordinate group [S89](#s89)
  - Determination: reviewed objective relationship to extracted game dungeon entrance 'KalphiteHive': The Crystal Helix fragments come from the damaged guardian inside the Hive, the dead guardian by the eggs, and the buried guardian at the ruins; these are the same three independently placed interactions established by the preceding search state.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 97/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Dead Vault Guardian — kalphite eggs** (`world_actor`) — map `0.80232581, 0.35629501`; world `325901.84, 31958.906, 1823.8793`; shared-coordinate group [S93](#s93)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_NPC_Quest_Contact_DeadVaultGuardian_UmS': The Crystal Helix fragments come from the damaged guardian inside the Hive, the dead guardian by the eggs, and the buried guardian at the ruins; these are the same three independently placed interactions established by the preceding search state.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/8AG0WHIXYVB5RO3QNZSY49AU8.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Buried Vault Guardian — ruined building** (`world_actor`) — map `0.77151976, 0.38517525`; world `312963.3, 44088.605, 2849.3892`; shared-coordinate group [S88](#s88)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_Contact_Buried_VaultGuardian': The Crystal Helix fragments come from the damaged guardian inside the Hive, the dead guardian by the eggs, and the buried guardian at the ruins; these are the same three independently placed interactions established by the preceding search state.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/8AG0WHIXYVB5RO3QNZSY49AU8.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

### Mapping the Sands I

Side quest · Umbral Sands

#### 1. Climb tall locations around the Alcarrid Camp and find their Moon Sprites

- Objective key: `Objective1`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: Interact with the three Survey I Moon Sprites on tall locations around Alcarrid Camp. Counter-only progress does not identify which freely ordered sprites were already found, so every verified target remains visible.
- Reviewed relationship: **yes**
- Cooked location operations: `MoonSpriteShamanNPC1` (revealed, `QF_UMS_MappingTheSands2.json` / `QFT_UpdateQuestLocation_C_11`)
- Marker 1: **Survey I Moon Sprite 1 — east of Alcarrid Camp** (`location`) — map `0.66969538, 0.54106729`; world `270197.06, 109563.26, 3353.6294`; shared-coordinate group [S78](#s78)
  - Determination: reviewed objective relationship to extracted game registry landmark 'DA7CE8F2-493C74D1-E29B67AC-21F0AB64': The current world contains exactly three non-b-series survey actors, BP_NPC_UmS_MoonSprite1, 2, and 3. Their positions match three registry identities; the colocated BP_NPC_UmS_MoonSpriteCamp is Anamcara's camp sprite and is deliberately excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Survey I Moon Sprite 2 — southeast height** (`location`) — map `0.73286376, 0.66181952`; world `296727.78, 160279.2, 2907.0208`; shared-coordinate group [S81](#s81)
  - Determination: reviewed objective relationship to extracted game registry landmark '24DAD7DB-47DAF91B-DF1DDEAA-CDE9C121': The current world contains exactly three non-b-series survey actors, BP_NPC_UmS_MoonSprite1, 2, and 3. Their positions match three registry identities; the colocated BP_NPC_UmS_MoonSpriteCamp is Anamcara's camp sprite and is deliberately excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Survey I Moon Sprite 3 — southwest height** (`location`) — map `0.53863281, 0.61067286`; world `215150.78, 138797.6, 4607.744`; shared-coordinate group [S51](#s51)
  - Determination: reviewed objective relationship to extracted game registry landmark '44DACD8B-48066352-860C6397-2221B764': The current world contains exactly three non-b-series survey actors, BP_NPC_UmS_MoonSprite1, 2, and 3. Their positions match three registry identities; the colocated BP_NPC_UmS_MoonSpriteCamp is Anamcara's camp sprite and is deliberately excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 2. Return to the Moon Sprite Shaman in the Alcarrid Camp

- Objective key: `Objective2`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: `MoonSpriteShamanNPC1` (hidden/removed, `QF_UMS_MappingTheSands.json` / `QFT_UpdateQuestLocation_C_4`); `MoonSpriteShamanTarget1` (revealed, `QF_UMS_MappingTheSands.json` / `QFT_UpdateQuestLocation_C_5`); `MoonSpriteShamanNPC2` (hidden/removed, `QF_UMS_MappingTheSands2.json` / `QFT_UpdateQuestLocation_C_12`); `MoonSpriteShamanTarget2` (revealed, `QF_UMS_MappingTheSands2.json` / `QFT_UpdateQuestLocation_C_13`)
- Marker 1: **Anamcara — Alcarrid Camp** (`location`) — map `0.64234155, 0.54190862`; world `258708.45, 109916.62, 2523.7878`; shared-coordinate group [S76](#s76)
  - Determination: reviewed objective relationship to extracted game quest marker 'MoonSpriteShamanTarget1': The objective explicitly returns to Anamcara in Alcarrid Camp; MoonSpriteShamanTarget1 is the exact quest interaction marker, while MoonSpriteShamanNPC1 is its colocated actor marker.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 3. Climb tall locations near Alcarrid Camp and find their Moon Sprites (0/3)

- Objective key: `0/3`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: Interact with the three Survey I Moon Sprites on tall locations around Alcarrid Camp. Counter-only progress does not identify which freely ordered sprites were already found, so every verified target remains visible.
- Reviewed relationship: **yes**
- Cooked location operations: `MoonSpriteShamanNPC1` (revealed, `QF_UMS_MappingTheSands.json` / `QFT_UpdateQuestLocation_C_0`)
- Marker 1: **Survey I Moon Sprite 1 — east of Alcarrid Camp** (`location`) — map `0.66969538, 0.54106729`; world `270197.06, 109563.26, 3353.6294`; shared-coordinate group [S78](#s78)
  - Determination: reviewed objective relationship to extracted game registry landmark 'DA7CE8F2-493C74D1-E29B67AC-21F0AB64': The current world contains exactly three non-b-series survey actors, BP_NPC_UmS_MoonSprite1, 2, and 3. Their positions match three registry identities; the colocated BP_NPC_UmS_MoonSpriteCamp is Anamcara's camp sprite and is deliberately excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Survey I Moon Sprite 2 — southeast height** (`location`) — map `0.73286376, 0.66181952`; world `296727.78, 160279.2, 2907.0208`; shared-coordinate group [S81](#s81)
  - Determination: reviewed objective relationship to extracted game registry landmark '24DAD7DB-47DAF91B-DF1DDEAA-CDE9C121': The current world contains exactly three non-b-series survey actors, BP_NPC_UmS_MoonSprite1, 2, and 3. Their positions match three registry identities; the colocated BP_NPC_UmS_MoonSpriteCamp is Anamcara's camp sprite and is deliberately excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Survey I Moon Sprite 3 — southwest height** (`location`) — map `0.53863281, 0.61067286`; world `215150.78, 138797.6, 4607.744`; shared-coordinate group [S51](#s51)
  - Determination: reviewed objective relationship to extracted game registry landmark '44DACD8B-48066352-860C6397-2221B764': The current world contains exactly three non-b-series survey actors, BP_NPC_UmS_MoonSprite1, 2, and 3. Their positions match three registry identities; the colocated BP_NPC_UmS_MoonSpriteCamp is Anamcara's camp sprite and is deliberately excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 4. Climb tall locations near Alcarrid Camp and find their Moon Sprites (1/3)

- Objective key: `1/3`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: Interact with the three Survey I Moon Sprites on tall locations around Alcarrid Camp. Counter-only progress does not identify which freely ordered sprites were already found, so every verified target remains visible.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Survey I Moon Sprite 1 — east of Alcarrid Camp** (`location`) — map `0.66969538, 0.54106729`; world `270197.06, 109563.26, 3353.6294`; shared-coordinate group [S78](#s78)
  - Determination: reviewed objective relationship to extracted game registry landmark 'DA7CE8F2-493C74D1-E29B67AC-21F0AB64': The current world contains exactly three non-b-series survey actors, BP_NPC_UmS_MoonSprite1, 2, and 3. Their positions match three registry identities; the colocated BP_NPC_UmS_MoonSpriteCamp is Anamcara's camp sprite and is deliberately excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Survey I Moon Sprite 2 — southeast height** (`location`) — map `0.73286376, 0.66181952`; world `296727.78, 160279.2, 2907.0208`; shared-coordinate group [S81](#s81)
  - Determination: reviewed objective relationship to extracted game registry landmark '24DAD7DB-47DAF91B-DF1DDEAA-CDE9C121': The current world contains exactly three non-b-series survey actors, BP_NPC_UmS_MoonSprite1, 2, and 3. Their positions match three registry identities; the colocated BP_NPC_UmS_MoonSpriteCamp is Anamcara's camp sprite and is deliberately excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Survey I Moon Sprite 3 — southwest height** (`location`) — map `0.53863281, 0.61067286`; world `215150.78, 138797.6, 4607.744`; shared-coordinate group [S51](#s51)
  - Determination: reviewed objective relationship to extracted game registry landmark '44DACD8B-48066352-860C6397-2221B764': The current world contains exactly three non-b-series survey actors, BP_NPC_UmS_MoonSprite1, 2, and 3. Their positions match three registry identities; the colocated BP_NPC_UmS_MoonSpriteCamp is Anamcara's camp sprite and is deliberately excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 5. Climb tall locations near Alcarrid Camp and find their Moon Sprites (2/3)

- Objective key: `2/3`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: Interact with the three Survey I Moon Sprites on tall locations around Alcarrid Camp. Counter-only progress does not identify which freely ordered sprites were already found, so every verified target remains visible.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Survey I Moon Sprite 1 — east of Alcarrid Camp** (`location`) — map `0.66969538, 0.54106729`; world `270197.06, 109563.26, 3353.6294`; shared-coordinate group [S78](#s78)
  - Determination: reviewed objective relationship to extracted game registry landmark 'DA7CE8F2-493C74D1-E29B67AC-21F0AB64': The current world contains exactly three non-b-series survey actors, BP_NPC_UmS_MoonSprite1, 2, and 3. Their positions match three registry identities; the colocated BP_NPC_UmS_MoonSpriteCamp is Anamcara's camp sprite and is deliberately excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Survey I Moon Sprite 2 — southeast height** (`location`) — map `0.73286376, 0.66181952`; world `296727.78, 160279.2, 2907.0208`; shared-coordinate group [S81](#s81)
  - Determination: reviewed objective relationship to extracted game registry landmark '24DAD7DB-47DAF91B-DF1DDEAA-CDE9C121': The current world contains exactly three non-b-series survey actors, BP_NPC_UmS_MoonSprite1, 2, and 3. Their positions match three registry identities; the colocated BP_NPC_UmS_MoonSpriteCamp is Anamcara's camp sprite and is deliberately excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Survey I Moon Sprite 3 — southwest height** (`location`) — map `0.53863281, 0.61067286`; world `215150.78, 138797.6, 4607.744`; shared-coordinate group [S51](#s51)
  - Determination: reviewed objective relationship to extracted game registry landmark '44DACD8B-48066352-860C6397-2221B764': The current world contains exactly three non-b-series survey actors, BP_NPC_UmS_MoonSprite1, 2, and 3. Their positions match three registry identities; the colocated BP_NPC_UmS_MoonSpriteCamp is Anamcara's camp sprite and is deliberately excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 6. Climb tall locations near Alcarrid Camp and find their Moon Sprites (3/3)

- Objective key: `3/3`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: Interact with the three Survey I Moon Sprites on tall locations around Alcarrid Camp. Counter-only progress does not identify which freely ordered sprites were already found, so every verified target remains visible.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Survey I Moon Sprite 1 — east of Alcarrid Camp** (`location`) — map `0.66969538, 0.54106729`; world `270197.06, 109563.26, 3353.6294`; shared-coordinate group [S78](#s78)
  - Determination: reviewed objective relationship to extracted game registry landmark 'DA7CE8F2-493C74D1-E29B67AC-21F0AB64': The current world contains exactly three non-b-series survey actors, BP_NPC_UmS_MoonSprite1, 2, and 3. Their positions match three registry identities; the colocated BP_NPC_UmS_MoonSpriteCamp is Anamcara's camp sprite and is deliberately excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Survey I Moon Sprite 2 — southeast height** (`location`) — map `0.73286376, 0.66181952`; world `296727.78, 160279.2, 2907.0208`; shared-coordinate group [S81](#s81)
  - Determination: reviewed objective relationship to extracted game registry landmark '24DAD7DB-47DAF91B-DF1DDEAA-CDE9C121': The current world contains exactly three non-b-series survey actors, BP_NPC_UmS_MoonSprite1, 2, and 3. Their positions match three registry identities; the colocated BP_NPC_UmS_MoonSpriteCamp is Anamcara's camp sprite and is deliberately excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Survey I Moon Sprite 3 — southwest height** (`location`) — map `0.53863281, 0.61067286`; world `215150.78, 138797.6, 4607.744`; shared-coordinate group [S51](#s51)
  - Determination: reviewed objective relationship to extracted game registry landmark '44DACD8B-48066352-860C6397-2221B764': The current world contains exactly three non-b-series survey actors, BP_NPC_UmS_MoonSprite1, 2, and 3. Their positions match three registry identities; the colocated BP_NPC_UmS_MoonSpriteCamp is Anamcara's camp sprite and is deliberately excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

### Mapping the Sands II

Side quest · Umbral Sands

#### 1. Climb tall locations around Umbral Sands and find their Moon Sprites

- Objective key: `Objective1`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: Interact with all eight Survey II Moon Sprites across Umbral Sands. Counter-only progress does not identify which freely ordered sprites were already found, so every verified target remains visible.
- Reviewed relationship: **yes**
- Cooked location operations: `MoonSpriteShamanNPC1` (revealed, `QF_UMS_MappingTheSands2.json` / `QFT_UpdateQuestLocation_C_11`)
- Marker 1: **Survey II Moon Sprite 1** (`location`) — map `0.73562805, 0.72213433`; world `297888.78, 185611.42, -2042.9752`; shared-coordinate group [S82](#s82)
  - Determination: reviewed objective relationship to extracted game registry landmark 'BCDC2CE7-40567BB6-0055BBB9-8E0DB2AE': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Survey II Moon Sprite 2** (`location`) — map `0.62457957, 0.77285014`; world `251248.42, 206912.06, 2108.581`; shared-coordinate group [S72](#s72)
  - Determination: reviewed objective relationship to extracted game registry landmark 'E13773E6-46782333-AB396AB7-35507903': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Survey II Moon Sprite 3** (`location`) — map `0.79303564, 0.77793467`; world `321999.97, 209047.56, -2292.4695`; shared-coordinate group [S92](#s92)
  - Determination: reviewed objective relationship to extracted game registry landmark '0EC1F4EA-4447FE69-5A2E17B7-8C7C1D97': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **Survey II Moon Sprite 4** (`location`) — map `0.74908162, 0.70706245`; world `303539.28, 179281.23, -1406.9464`; shared-coordinate group [S86](#s86)
  - Determination: reviewed objective relationship to extracted game registry landmark 'D05DC1FC-4BC445AE-398275A7-772A4AC1': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 5: **Survey II Moon Sprite 5** (`location`) — map `0.899914, 0.47171469`; world `366888.88, 80435.17, 4160.434`; shared-coordinate group [S100](#s100)
  - Determination: reviewed objective relationship to extracted game registry landmark 'B4368D72-4DC36E39-76CDDC84-003E54A5': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 6: **Survey II Moon Sprite 6** (`location`) — map `0.91382405, 0.72215038`; world `372731.1, 185618.16, 5314.2046`; shared-coordinate group [S103](#s103)
  - Determination: reviewed objective relationship to extracted game registry landmark '2890F86A-4F1E8EFA-126CBEA7-E46E02AC': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 7: **Survey II Moon Sprite 7** (`location`) — map `0.74848571, 0.43437237`; world `303289.0, 64751.395, 2930.9355`; shared-coordinate group [S85](#s85)
  - Determination: reviewed objective relationship to extracted game registry landmark 'AF47C155-4515C0B4-D8A82B8D-1203F227': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 8: **Survey II Moon Sprite 9** (`location`) — map `0.78657895, 0.73351493`; world `319288.16, 190391.27, 4439.0166`; shared-coordinate group [S91](#s91)
  - Determination: reviewed objective relationship to extracted game registry landmark 'C0CBAB63-43897DDA-F0B99FAC-04091E16': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 2. Return to the Moon Sprite Shaman in the Alcarrid Camp

- Objective key: `Objective2`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: `MoonSpriteShamanNPC2` (hidden/removed, `QF_UMS_MappingTheSands2.json` / `QFT_UpdateQuestLocation_C_12`); `MoonSpriteShamanTarget2` (revealed, `QF_UMS_MappingTheSands2.json` / `QFT_UpdateQuestLocation_C_13`)
- Marker 1: **Anamcara — Alcarrid Camp** (`location`) — map `0.64234155, 0.54190862`; world `258708.45, 109916.62, 2523.7878`; shared-coordinate group [S76](#s76)
  - Determination: reviewed objective relationship to extracted game quest marker 'MoonSpriteShamanNPC2': The objective explicitly returns to the Moon Sprite Shaman and the second quest owns one exact MoonSpriteShamanNPC2 destination at Anamcara's Alcarrid camp.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 3. Climb tall locations around Umbral Sands and find their Moon Sprites (0/8)

- Objective key: `0/8`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: Interact with all eight Survey II Moon Sprites across Umbral Sands. Counter-only progress does not identify which freely ordered sprites were already found, so every verified target remains visible.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Survey II Moon Sprite 1** (`location`) — map `0.73562805, 0.72213433`; world `297888.78, 185611.42, -2042.9752`; shared-coordinate group [S82](#s82)
  - Determination: reviewed objective relationship to extracted game registry landmark 'BCDC2CE7-40567BB6-0055BBB9-8E0DB2AE': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Survey II Moon Sprite 2** (`location`) — map `0.62457957, 0.77285014`; world `251248.42, 206912.06, 2108.581`; shared-coordinate group [S72](#s72)
  - Determination: reviewed objective relationship to extracted game registry landmark 'E13773E6-46782333-AB396AB7-35507903': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Survey II Moon Sprite 3** (`location`) — map `0.79303564, 0.77793467`; world `321999.97, 209047.56, -2292.4695`; shared-coordinate group [S92](#s92)
  - Determination: reviewed objective relationship to extracted game registry landmark '0EC1F4EA-4447FE69-5A2E17B7-8C7C1D97': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **Survey II Moon Sprite 4** (`location`) — map `0.74908162, 0.70706245`; world `303539.28, 179281.23, -1406.9464`; shared-coordinate group [S86](#s86)
  - Determination: reviewed objective relationship to extracted game registry landmark 'D05DC1FC-4BC445AE-398275A7-772A4AC1': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 5: **Survey II Moon Sprite 5** (`location`) — map `0.899914, 0.47171469`; world `366888.88, 80435.17, 4160.434`; shared-coordinate group [S100](#s100)
  - Determination: reviewed objective relationship to extracted game registry landmark 'B4368D72-4DC36E39-76CDDC84-003E54A5': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 6: **Survey II Moon Sprite 6** (`location`) — map `0.91382405, 0.72215038`; world `372731.1, 185618.16, 5314.2046`; shared-coordinate group [S103](#s103)
  - Determination: reviewed objective relationship to extracted game registry landmark '2890F86A-4F1E8EFA-126CBEA7-E46E02AC': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 7: **Survey II Moon Sprite 7** (`location`) — map `0.74848571, 0.43437237`; world `303289.0, 64751.395, 2930.9355`; shared-coordinate group [S85](#s85)
  - Determination: reviewed objective relationship to extracted game registry landmark 'AF47C155-4515C0B4-D8A82B8D-1203F227': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 8: **Survey II Moon Sprite 9** (`location`) — map `0.78657895, 0.73351493`; world `319288.16, 190391.27, 4439.0166`; shared-coordinate group [S91](#s91)
  - Determination: reviewed objective relationship to extracted game registry landmark 'C0CBAB63-43897DDA-F0B99FAC-04091E16': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 4. Climb tall locations around Umbral Sands and find their Moon Sprites (1/8)

- Objective key: `1/8`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: Interact with all eight Survey II Moon Sprites across Umbral Sands. Counter-only progress does not identify which freely ordered sprites were already found, so every verified target remains visible.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Survey II Moon Sprite 1** (`location`) — map `0.73562805, 0.72213433`; world `297888.78, 185611.42, -2042.9752`; shared-coordinate group [S82](#s82)
  - Determination: reviewed objective relationship to extracted game registry landmark 'BCDC2CE7-40567BB6-0055BBB9-8E0DB2AE': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Survey II Moon Sprite 2** (`location`) — map `0.62457957, 0.77285014`; world `251248.42, 206912.06, 2108.581`; shared-coordinate group [S72](#s72)
  - Determination: reviewed objective relationship to extracted game registry landmark 'E13773E6-46782333-AB396AB7-35507903': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Survey II Moon Sprite 3** (`location`) — map `0.79303564, 0.77793467`; world `321999.97, 209047.56, -2292.4695`; shared-coordinate group [S92](#s92)
  - Determination: reviewed objective relationship to extracted game registry landmark '0EC1F4EA-4447FE69-5A2E17B7-8C7C1D97': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **Survey II Moon Sprite 4** (`location`) — map `0.74908162, 0.70706245`; world `303539.28, 179281.23, -1406.9464`; shared-coordinate group [S86](#s86)
  - Determination: reviewed objective relationship to extracted game registry landmark 'D05DC1FC-4BC445AE-398275A7-772A4AC1': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 5: **Survey II Moon Sprite 5** (`location`) — map `0.899914, 0.47171469`; world `366888.88, 80435.17, 4160.434`; shared-coordinate group [S100](#s100)
  - Determination: reviewed objective relationship to extracted game registry landmark 'B4368D72-4DC36E39-76CDDC84-003E54A5': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 6: **Survey II Moon Sprite 6** (`location`) — map `0.91382405, 0.72215038`; world `372731.1, 185618.16, 5314.2046`; shared-coordinate group [S103](#s103)
  - Determination: reviewed objective relationship to extracted game registry landmark '2890F86A-4F1E8EFA-126CBEA7-E46E02AC': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 7: **Survey II Moon Sprite 7** (`location`) — map `0.74848571, 0.43437237`; world `303289.0, 64751.395, 2930.9355`; shared-coordinate group [S85](#s85)
  - Determination: reviewed objective relationship to extracted game registry landmark 'AF47C155-4515C0B4-D8A82B8D-1203F227': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 8: **Survey II Moon Sprite 9** (`location`) — map `0.78657895, 0.73351493`; world `319288.16, 190391.27, 4439.0166`; shared-coordinate group [S91](#s91)
  - Determination: reviewed objective relationship to extracted game registry landmark 'C0CBAB63-43897DDA-F0B99FAC-04091E16': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 5. Climb tall locations around Umbral Sands and find their Moon Sprites (2/8)

- Objective key: `2/8`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: Interact with all eight Survey II Moon Sprites across Umbral Sands. Counter-only progress does not identify which freely ordered sprites were already found, so every verified target remains visible.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Survey II Moon Sprite 1** (`location`) — map `0.73562805, 0.72213433`; world `297888.78, 185611.42, -2042.9752`; shared-coordinate group [S82](#s82)
  - Determination: reviewed objective relationship to extracted game registry landmark 'BCDC2CE7-40567BB6-0055BBB9-8E0DB2AE': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Survey II Moon Sprite 2** (`location`) — map `0.62457957, 0.77285014`; world `251248.42, 206912.06, 2108.581`; shared-coordinate group [S72](#s72)
  - Determination: reviewed objective relationship to extracted game registry landmark 'E13773E6-46782333-AB396AB7-35507903': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Survey II Moon Sprite 3** (`location`) — map `0.79303564, 0.77793467`; world `321999.97, 209047.56, -2292.4695`; shared-coordinate group [S92](#s92)
  - Determination: reviewed objective relationship to extracted game registry landmark '0EC1F4EA-4447FE69-5A2E17B7-8C7C1D97': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **Survey II Moon Sprite 4** (`location`) — map `0.74908162, 0.70706245`; world `303539.28, 179281.23, -1406.9464`; shared-coordinate group [S86](#s86)
  - Determination: reviewed objective relationship to extracted game registry landmark 'D05DC1FC-4BC445AE-398275A7-772A4AC1': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 5: **Survey II Moon Sprite 5** (`location`) — map `0.899914, 0.47171469`; world `366888.88, 80435.17, 4160.434`; shared-coordinate group [S100](#s100)
  - Determination: reviewed objective relationship to extracted game registry landmark 'B4368D72-4DC36E39-76CDDC84-003E54A5': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 6: **Survey II Moon Sprite 6** (`location`) — map `0.91382405, 0.72215038`; world `372731.1, 185618.16, 5314.2046`; shared-coordinate group [S103](#s103)
  - Determination: reviewed objective relationship to extracted game registry landmark '2890F86A-4F1E8EFA-126CBEA7-E46E02AC': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 7: **Survey II Moon Sprite 7** (`location`) — map `0.74848571, 0.43437237`; world `303289.0, 64751.395, 2930.9355`; shared-coordinate group [S85](#s85)
  - Determination: reviewed objective relationship to extracted game registry landmark 'AF47C155-4515C0B4-D8A82B8D-1203F227': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 8: **Survey II Moon Sprite 9** (`location`) — map `0.78657895, 0.73351493`; world `319288.16, 190391.27, 4439.0166`; shared-coordinate group [S91](#s91)
  - Determination: reviewed objective relationship to extracted game registry landmark 'C0CBAB63-43897DDA-F0B99FAC-04091E16': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 6. Climb tall locations around Umbral Sands and find their Moon Sprites (3/8)

- Objective key: `3/8`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: Interact with all eight Survey II Moon Sprites across Umbral Sands. Counter-only progress does not identify which freely ordered sprites were already found, so every verified target remains visible.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Survey II Moon Sprite 1** (`location`) — map `0.73562805, 0.72213433`; world `297888.78, 185611.42, -2042.9752`; shared-coordinate group [S82](#s82)
  - Determination: reviewed objective relationship to extracted game registry landmark 'BCDC2CE7-40567BB6-0055BBB9-8E0DB2AE': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Survey II Moon Sprite 2** (`location`) — map `0.62457957, 0.77285014`; world `251248.42, 206912.06, 2108.581`; shared-coordinate group [S72](#s72)
  - Determination: reviewed objective relationship to extracted game registry landmark 'E13773E6-46782333-AB396AB7-35507903': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Survey II Moon Sprite 3** (`location`) — map `0.79303564, 0.77793467`; world `321999.97, 209047.56, -2292.4695`; shared-coordinate group [S92](#s92)
  - Determination: reviewed objective relationship to extracted game registry landmark '0EC1F4EA-4447FE69-5A2E17B7-8C7C1D97': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **Survey II Moon Sprite 4** (`location`) — map `0.74908162, 0.70706245`; world `303539.28, 179281.23, -1406.9464`; shared-coordinate group [S86](#s86)
  - Determination: reviewed objective relationship to extracted game registry landmark 'D05DC1FC-4BC445AE-398275A7-772A4AC1': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 5: **Survey II Moon Sprite 5** (`location`) — map `0.899914, 0.47171469`; world `366888.88, 80435.17, 4160.434`; shared-coordinate group [S100](#s100)
  - Determination: reviewed objective relationship to extracted game registry landmark 'B4368D72-4DC36E39-76CDDC84-003E54A5': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 6: **Survey II Moon Sprite 6** (`location`) — map `0.91382405, 0.72215038`; world `372731.1, 185618.16, 5314.2046`; shared-coordinate group [S103](#s103)
  - Determination: reviewed objective relationship to extracted game registry landmark '2890F86A-4F1E8EFA-126CBEA7-E46E02AC': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 7: **Survey II Moon Sprite 7** (`location`) — map `0.74848571, 0.43437237`; world `303289.0, 64751.395, 2930.9355`; shared-coordinate group [S85](#s85)
  - Determination: reviewed objective relationship to extracted game registry landmark 'AF47C155-4515C0B4-D8A82B8D-1203F227': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 8: **Survey II Moon Sprite 9** (`location`) — map `0.78657895, 0.73351493`; world `319288.16, 190391.27, 4439.0166`; shared-coordinate group [S91](#s91)
  - Determination: reviewed objective relationship to extracted game registry landmark 'C0CBAB63-43897DDA-F0B99FAC-04091E16': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 7. Climb tall locations around Umbral Sands and find their Moon Sprites (4/8)

- Objective key: `4/8`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: Interact with all eight Survey II Moon Sprites across Umbral Sands. Counter-only progress does not identify which freely ordered sprites were already found, so every verified target remains visible.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Survey II Moon Sprite 1** (`location`) — map `0.73562805, 0.72213433`; world `297888.78, 185611.42, -2042.9752`; shared-coordinate group [S82](#s82)
  - Determination: reviewed objective relationship to extracted game registry landmark 'BCDC2CE7-40567BB6-0055BBB9-8E0DB2AE': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Survey II Moon Sprite 2** (`location`) — map `0.62457957, 0.77285014`; world `251248.42, 206912.06, 2108.581`; shared-coordinate group [S72](#s72)
  - Determination: reviewed objective relationship to extracted game registry landmark 'E13773E6-46782333-AB396AB7-35507903': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Survey II Moon Sprite 3** (`location`) — map `0.79303564, 0.77793467`; world `321999.97, 209047.56, -2292.4695`; shared-coordinate group [S92](#s92)
  - Determination: reviewed objective relationship to extracted game registry landmark '0EC1F4EA-4447FE69-5A2E17B7-8C7C1D97': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **Survey II Moon Sprite 4** (`location`) — map `0.74908162, 0.70706245`; world `303539.28, 179281.23, -1406.9464`; shared-coordinate group [S86](#s86)
  - Determination: reviewed objective relationship to extracted game registry landmark 'D05DC1FC-4BC445AE-398275A7-772A4AC1': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 5: **Survey II Moon Sprite 5** (`location`) — map `0.899914, 0.47171469`; world `366888.88, 80435.17, 4160.434`; shared-coordinate group [S100](#s100)
  - Determination: reviewed objective relationship to extracted game registry landmark 'B4368D72-4DC36E39-76CDDC84-003E54A5': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 6: **Survey II Moon Sprite 6** (`location`) — map `0.91382405, 0.72215038`; world `372731.1, 185618.16, 5314.2046`; shared-coordinate group [S103](#s103)
  - Determination: reviewed objective relationship to extracted game registry landmark '2890F86A-4F1E8EFA-126CBEA7-E46E02AC': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 7: **Survey II Moon Sprite 7** (`location`) — map `0.74848571, 0.43437237`; world `303289.0, 64751.395, 2930.9355`; shared-coordinate group [S85](#s85)
  - Determination: reviewed objective relationship to extracted game registry landmark 'AF47C155-4515C0B4-D8A82B8D-1203F227': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 8: **Survey II Moon Sprite 9** (`location`) — map `0.78657895, 0.73351493`; world `319288.16, 190391.27, 4439.0166`; shared-coordinate group [S91](#s91)
  - Determination: reviewed objective relationship to extracted game registry landmark 'C0CBAB63-43897DDA-F0B99FAC-04091E16': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 8. Climb tall locations around Umbral Sands and find their Moon Sprites (5/8)

- Objective key: `5/8`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: Interact with all eight Survey II Moon Sprites across Umbral Sands. Counter-only progress does not identify which freely ordered sprites were already found, so every verified target remains visible.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Survey II Moon Sprite 1** (`location`) — map `0.73562805, 0.72213433`; world `297888.78, 185611.42, -2042.9752`; shared-coordinate group [S82](#s82)
  - Determination: reviewed objective relationship to extracted game registry landmark 'BCDC2CE7-40567BB6-0055BBB9-8E0DB2AE': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Survey II Moon Sprite 2** (`location`) — map `0.62457957, 0.77285014`; world `251248.42, 206912.06, 2108.581`; shared-coordinate group [S72](#s72)
  - Determination: reviewed objective relationship to extracted game registry landmark 'E13773E6-46782333-AB396AB7-35507903': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Survey II Moon Sprite 3** (`location`) — map `0.79303564, 0.77793467`; world `321999.97, 209047.56, -2292.4695`; shared-coordinate group [S92](#s92)
  - Determination: reviewed objective relationship to extracted game registry landmark '0EC1F4EA-4447FE69-5A2E17B7-8C7C1D97': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **Survey II Moon Sprite 4** (`location`) — map `0.74908162, 0.70706245`; world `303539.28, 179281.23, -1406.9464`; shared-coordinate group [S86](#s86)
  - Determination: reviewed objective relationship to extracted game registry landmark 'D05DC1FC-4BC445AE-398275A7-772A4AC1': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 5: **Survey II Moon Sprite 5** (`location`) — map `0.899914, 0.47171469`; world `366888.88, 80435.17, 4160.434`; shared-coordinate group [S100](#s100)
  - Determination: reviewed objective relationship to extracted game registry landmark 'B4368D72-4DC36E39-76CDDC84-003E54A5': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 6: **Survey II Moon Sprite 6** (`location`) — map `0.91382405, 0.72215038`; world `372731.1, 185618.16, 5314.2046`; shared-coordinate group [S103](#s103)
  - Determination: reviewed objective relationship to extracted game registry landmark '2890F86A-4F1E8EFA-126CBEA7-E46E02AC': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 7: **Survey II Moon Sprite 7** (`location`) — map `0.74848571, 0.43437237`; world `303289.0, 64751.395, 2930.9355`; shared-coordinate group [S85](#s85)
  - Determination: reviewed objective relationship to extracted game registry landmark 'AF47C155-4515C0B4-D8A82B8D-1203F227': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 8: **Survey II Moon Sprite 9** (`location`) — map `0.78657895, 0.73351493`; world `319288.16, 190391.27, 4439.0166`; shared-coordinate group [S91](#s91)
  - Determination: reviewed objective relationship to extracted game registry landmark 'C0CBAB63-43897DDA-F0B99FAC-04091E16': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 9. Climb tall locations around Umbral Sands and find their Moon Sprites (6/8)

- Objective key: `6/8`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: Interact with all eight Survey II Moon Sprites across Umbral Sands. Counter-only progress does not identify which freely ordered sprites were already found, so every verified target remains visible.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Survey II Moon Sprite 1** (`location`) — map `0.73562805, 0.72213433`; world `297888.78, 185611.42, -2042.9752`; shared-coordinate group [S82](#s82)
  - Determination: reviewed objective relationship to extracted game registry landmark 'BCDC2CE7-40567BB6-0055BBB9-8E0DB2AE': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Survey II Moon Sprite 2** (`location`) — map `0.62457957, 0.77285014`; world `251248.42, 206912.06, 2108.581`; shared-coordinate group [S72](#s72)
  - Determination: reviewed objective relationship to extracted game registry landmark 'E13773E6-46782333-AB396AB7-35507903': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Survey II Moon Sprite 3** (`location`) — map `0.79303564, 0.77793467`; world `321999.97, 209047.56, -2292.4695`; shared-coordinate group [S92](#s92)
  - Determination: reviewed objective relationship to extracted game registry landmark '0EC1F4EA-4447FE69-5A2E17B7-8C7C1D97': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **Survey II Moon Sprite 4** (`location`) — map `0.74908162, 0.70706245`; world `303539.28, 179281.23, -1406.9464`; shared-coordinate group [S86](#s86)
  - Determination: reviewed objective relationship to extracted game registry landmark 'D05DC1FC-4BC445AE-398275A7-772A4AC1': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 5: **Survey II Moon Sprite 5** (`location`) — map `0.899914, 0.47171469`; world `366888.88, 80435.17, 4160.434`; shared-coordinate group [S100](#s100)
  - Determination: reviewed objective relationship to extracted game registry landmark 'B4368D72-4DC36E39-76CDDC84-003E54A5': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 6: **Survey II Moon Sprite 6** (`location`) — map `0.91382405, 0.72215038`; world `372731.1, 185618.16, 5314.2046`; shared-coordinate group [S103](#s103)
  - Determination: reviewed objective relationship to extracted game registry landmark '2890F86A-4F1E8EFA-126CBEA7-E46E02AC': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 7: **Survey II Moon Sprite 7** (`location`) — map `0.74848571, 0.43437237`; world `303289.0, 64751.395, 2930.9355`; shared-coordinate group [S85](#s85)
  - Determination: reviewed objective relationship to extracted game registry landmark 'AF47C155-4515C0B4-D8A82B8D-1203F227': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 8: **Survey II Moon Sprite 9** (`location`) — map `0.78657895, 0.73351493`; world `319288.16, 190391.27, 4439.0166`; shared-coordinate group [S91](#s91)
  - Determination: reviewed objective relationship to extracted game registry landmark 'C0CBAB63-43897DDA-F0B99FAC-04091E16': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 10. Climb tall locations around Umbral Sands and find their Moon Sprites (7/8)

- Objective key: `7/8`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: Interact with all eight Survey II Moon Sprites across Umbral Sands. Counter-only progress does not identify which freely ordered sprites were already found, so every verified target remains visible.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Survey II Moon Sprite 1** (`location`) — map `0.73562805, 0.72213433`; world `297888.78, 185611.42, -2042.9752`; shared-coordinate group [S82](#s82)
  - Determination: reviewed objective relationship to extracted game registry landmark 'BCDC2CE7-40567BB6-0055BBB9-8E0DB2AE': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Survey II Moon Sprite 2** (`location`) — map `0.62457957, 0.77285014`; world `251248.42, 206912.06, 2108.581`; shared-coordinate group [S72](#s72)
  - Determination: reviewed objective relationship to extracted game registry landmark 'E13773E6-46782333-AB396AB7-35507903': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Survey II Moon Sprite 3** (`location`) — map `0.79303564, 0.77793467`; world `321999.97, 209047.56, -2292.4695`; shared-coordinate group [S92](#s92)
  - Determination: reviewed objective relationship to extracted game registry landmark '0EC1F4EA-4447FE69-5A2E17B7-8C7C1D97': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **Survey II Moon Sprite 4** (`location`) — map `0.74908162, 0.70706245`; world `303539.28, 179281.23, -1406.9464`; shared-coordinate group [S86](#s86)
  - Determination: reviewed objective relationship to extracted game registry landmark 'D05DC1FC-4BC445AE-398275A7-772A4AC1': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 5: **Survey II Moon Sprite 5** (`location`) — map `0.899914, 0.47171469`; world `366888.88, 80435.17, 4160.434`; shared-coordinate group [S100](#s100)
  - Determination: reviewed objective relationship to extracted game registry landmark 'B4368D72-4DC36E39-76CDDC84-003E54A5': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 6: **Survey II Moon Sprite 6** (`location`) — map `0.91382405, 0.72215038`; world `372731.1, 185618.16, 5314.2046`; shared-coordinate group [S103](#s103)
  - Determination: reviewed objective relationship to extracted game registry landmark '2890F86A-4F1E8EFA-126CBEA7-E46E02AC': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 7: **Survey II Moon Sprite 7** (`location`) — map `0.74848571, 0.43437237`; world `303289.0, 64751.395, 2930.9355`; shared-coordinate group [S85](#s85)
  - Determination: reviewed objective relationship to extracted game registry landmark 'AF47C155-4515C0B4-D8A82B8D-1203F227': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 8: **Survey II Moon Sprite 9** (`location`) — map `0.78657895, 0.73351493`; world `319288.16, 190391.27, 4439.0166`; shared-coordinate group [S91](#s91)
  - Determination: reviewed objective relationship to extracted game registry landmark 'C0CBAB63-43897DDA-F0B99FAC-04091E16': The current world contains exactly eight Survey II b-series actors (1b through 7b plus 9b), each matching one registry identity by exact coordinates. The three Survey I actors and Anamcara's camp sprite are excluded.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

### Mark Day

Side quest · Umbral Sands

#### 1. Convince the Uzzer councillors to abandon the Mark Day Festival. 0/3

- Objective key: `ConvinceNoble.0`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: The three councillors may be persuaded in any order. Only their three authored target areas are shown; the quest giver is a later return destination.
- Reviewed relationship: **yes**
- Cooked location operations: `TribeB.Task3.Noble2` (revealed, `QF_UmS_MainQuest_TribeB_Task3_FuzanFestival.json` / `QFT_UpdateQuestLocation_C_0`); `TribeB.Task3.Noble3` (revealed, `QF_UmS_MainQuest_TribeB_Task3_FuzanFestival.json` / `QFT_UpdateQuestLocation_C_1`); `TribeB.Task3.Noble1` (revealed, `QF_UmS_MainQuest_TribeB_Task3_FuzanFestival.json` / `QFT_UpdateQuestLocation_C_11`); `TribeB_Taskgiver3_NPC` (revealed, `QF_UmS_MainQuest_TribeB_Task3_FuzanFestival.json` / `QFT_UpdateQuestLocation_C_3`); `TribeB.Taskgiver3` (hidden/removed, `QF_UmS_MainQuest_TribeB_Task3_FuzanFestival.json` / `QFT_UpdateQuestLocation_C_7`)
- Marker 1: **Brialka — Historian Councillor** (`area`) — map `0.80333795, 0.73349548`; world `326326.94, 190383.1, 660.0`; shared-coordinate group [S94](#s94)
  - Determination: reviewed objective relationship to extracted game quest marker 'TribeB.Task3.Noble1': The compiled flow reveals Noble1, Noble2, and Noble3 for Brialka, Alabrais, and Chirot. The guide independently confirms these are the three councillors, while TribeB_Taskgiver3_NPC is Shangruuh and not a persuasion target.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Alabrais — Science Councillor** (`area`) — map `0.83684412, 0.45898287`; world `340399.53, 75087.805, 2970.0`; shared-coordinate group [S95](#s95)
  - Determination: reviewed objective relationship to extracted game quest marker 'TribeB.Task3.Noble2': The compiled flow reveals Noble1, Noble2, and Noble3 for Brialka, Alabrais, and Chirot. The guide independently confirms these are the three councillors, while TribeB_Taskgiver3_NPC is Shangruuh and not a persuasion target.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Chirot — Broker Councillor** (`area`) — map `0.86020945, 0.64445871`; world `350212.97, 152987.66, 100.0`; shared-coordinate group [S97](#s97)
  - Determination: reviewed objective relationship to extracted game quest marker 'TribeB.Task3.Noble3': The compiled flow reveals Noble1, Noble2, and Noble3 for Brialka, Alabrais, and Chirot. The guide independently confirms these are the three councillors, while TribeB_Taskgiver3_NPC is Shangruuh and not a persuasion target.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 2. Convince the Uzzer councillors to abandon the Mark Day Festival. 1/3

- Objective key: `ConvinceNoble.1`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: The counter does not identify which freely ordered councillors remain, so all three councillor targets stay visible; Shangruuh is a separate return state.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Brialka — Historian Councillor** (`area`) — map `0.80333795, 0.73349548`; world `326326.94, 190383.1, 660.0`; shared-coordinate group [S94](#s94)
  - Determination: reviewed objective relationship to extracted game quest marker 'TribeB.Task3.Noble1': The compiled Mark Day flow exposes three independent Noble targets for Brialka, Chirot, and Alabrais. TribeB_Taskgiver3_NPC is the quest giver and is not a persuasion target.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Alabrais — Science Councillor** (`area`) — map `0.83684412, 0.45898287`; world `340399.53, 75087.805, 2970.0`; shared-coordinate group [S95](#s95)
  - Determination: reviewed objective relationship to extracted game quest marker 'TribeB.Task3.Noble2': The compiled Mark Day flow exposes three independent Noble targets for Brialka, Chirot, and Alabrais. TribeB_Taskgiver3_NPC is the quest giver and is not a persuasion target.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Chirot — Broker Councillor** (`area`) — map `0.86020945, 0.64445871`; world `350212.97, 152987.66, 100.0`; shared-coordinate group [S97](#s97)
  - Determination: reviewed objective relationship to extracted game quest marker 'TribeB.Task3.Noble3': The compiled Mark Day flow exposes three independent Noble targets for Brialka, Chirot, and Alabrais. TribeB_Taskgiver3_NPC is the quest giver and is not a persuasion target.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 3. Convince the Uzzer councillors to abandon the Mark Day Festival. 2/3

- Objective key: `ConvinceNoble.2`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: The counter does not identify which freely ordered councillors remain, so all three councillor targets stay visible; Shangruuh is a separate return state.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Brialka — Historian Councillor** (`area`) — map `0.80333795, 0.73349548`; world `326326.94, 190383.1, 660.0`; shared-coordinate group [S94](#s94)
  - Determination: reviewed objective relationship to extracted game quest marker 'TribeB.Task3.Noble1': The compiled Mark Day flow exposes three independent Noble targets for Brialka, Chirot, and Alabrais. TribeB_Taskgiver3_NPC is the quest giver and is not a persuasion target.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Alabrais — Science Councillor** (`area`) — map `0.83684412, 0.45898287`; world `340399.53, 75087.805, 2970.0`; shared-coordinate group [S95](#s95)
  - Determination: reviewed objective relationship to extracted game quest marker 'TribeB.Task3.Noble2': The compiled Mark Day flow exposes three independent Noble targets for Brialka, Chirot, and Alabrais. TribeB_Taskgiver3_NPC is the quest giver and is not a persuasion target.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Chirot — Broker Councillor** (`area`) — map `0.86020945, 0.64445871`; world `350212.97, 152987.66, 100.0`; shared-coordinate group [S97](#s97)
  - Determination: reviewed objective relationship to extracted game quest marker 'TribeB.Task3.Noble3': The compiled Mark Day flow exposes three independent Noble targets for Brialka, Chirot, and Alabrais. TribeB_Taskgiver3_NPC is the quest giver and is not a persuasion target.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 4. Return to Shangruuh - Garou Envoy in the Uzzer Camp

- Objective key: `ReturnToShangruuh`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: `TribeB_Taskgiver3_NPC` (hidden/removed, `QF_UmS_MainQuest_TribeB_Task3_FuzanFestival.json` / `QFT_UpdateQuestLocation_C_8`); `TribeB_Taskgiver3_Target` (revealed, `QF_UmS_MainQuest_TribeB_Task3_FuzanFestival.json` / `QFT_UpdateQuestLocation_C_9`)
- Marker 1: **Shangruuh — Uzzer Camp** (`location`) — map `0.8771389, 0.56805424`; world `357323.34, 120897.78, 2294.431`
  - Determination: reviewed objective relationship to extracted game quest marker 'TribeB_Taskgiver3_Target': The compiled flow reveals TribeB_Taskgiver3_Target only for the final return, and the objective explicitly identifies Shangruuh in the Uzzer Camp.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

### Rag and Bone Garou

Side quest · Umbral Sands

#### 1. Bring Lagra - The Butcher 3 raw farm meat, 3 kalphite shell fragments and 3 cactus steaks

- Objective key: `GatherIngredients`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Display note: The ingredients have multiple renewable sources; this marker identifies the required hand-in to Lagra rather than pretending there is one mandatory gathering location.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Lagra — The Butcher** (`location`) — map `0.60922626, 0.55674538`; world `244800.03, 116148.06, 3248.5967`
  - Determination: reviewed objective relationship to extracted game registry landmark '685A686A-4A3B33AE-D302F8B9-E65410FF': The objective explicitly brings all three ingredient types to Lagra, and the registry contains one exact Lagra - The Butcher placement. Guides confirm raw farm meat, kalphite shell fragments, and cactus steaks can each be sourced in multiple ways or places.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

### Rogue Trader

Side quest · Umbral Sands

#### 1. Find 3 moonstones beneath gypsum deposits in rocky areas west of Alcarrid

- Objective key: `FindMoonstone`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: Mine the gypsum deposit at each of the three fixed Moonstone locations west of Alcarrid: the northern cave, the rock platforms, and the southern Ruby Vine cave.
- Reviewed relationship: **yes**
- Cooked location operations: `TribeA.Taskgiver2` (hidden/removed, `QF_UmS_TribeA_Task2_RogueTrader.json` / `QFT_UpdateQuestLocation_C_14`); `TribeA_Task2_Area_01` (revealed, `QF_UmS_TribeA_Task2_RogueTrader.json` / `QFT_UpdateQuestLocation_C_17`); `TribeA_Task2_Area_03` (revealed, `QF_UmS_TribeA_Task2_RogueTrader.json` / `QFT_UpdateQuestLocation_C_18`); `TribeA_Task2_Area_02` (revealed, `QF_UmS_TribeA_Task2_RogueTrader.json` / `QFT_UpdateQuestLocation_C_3`); `TribeA_Task2_NPC` (revealed, `QF_UmS_TribeA_Task2_RogueTrader.json` / `QFT_UpdateQuestLocation_C_5`)
- Marker 1: **Moonstone 1 — northern cave** (`location`) — map `0.54588764, 0.54482351`; world `218197.81, 111140.875, 1063.4182`
  - Determination: reviewed objective relationship to extracted game registry landmark '8AAA344F-4CC04E18-8CC04DA1-7670E0CB': The quest flow has three separately named Moonstone entry states, and the current world registry publishes exactly three FirstCamp.Task2 Moonstone identities. The three broad search areas surround those exact item placements; Domri is a separate return objective.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Moonstone 2 — rock platforms** (`location`) — map `0.54542243, 0.59460231`; world `218002.42, 132047.97, 2313.305`
  - Determination: reviewed objective relationship to extracted game registry landmark 'FE794809-49AD55B2-F8E402AC-FC5A9041': The quest flow has three separately named Moonstone entry states, and the current world registry publishes exactly three FirstCamp.Task2 Moonstone identities. The three broad search areas surround those exact item placements; Domri is a separate return objective.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Moonstone 3 — Ruby Vine cave** (`location`) — map `0.51044417, 0.64446857`; world `203311.55, 152991.8, -217.00848`
  - Determination: reviewed objective relationship to extracted game registry landmark '4E6F5F7D-431A2937-099421B6-E9A69636': The quest flow has three separately named Moonstone entry states, and the current world registry publishes exactly three FirstCamp.Task2 Moonstone identities. The three broad search areas surround those exact item placements; Domri is a separate return objective.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 2. Return to Domri - The Merchant in the Alcarrid Camp

- Objective key: `ReturnToShopkeeper`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: `TribeA_Task2_NPC` (hidden/removed, `QF_UmS_TribeA_Task2_RogueTrader.json` / `QFT_UpdateQuestLocation_C_20`); `TribeA_Task2_Target` (revealed, `QF_UmS_TribeA_Task2_RogueTrader.json` / `QFT_UpdateQuestLocation_C_21`); `TribeA.Task2` (hidden/removed, `QF_UmS_TribeA_Task2_RogueTrader.json` / `QFT_UpdateQuestLocation_C_22`)
- Marker 1: **Domri — The Merchant** (`location`) — map `0.61748895, 0.54792752`; world `248270.36, 112444.56, 2686.7534`
  - Determination: reviewed objective relationship to extracted game registry landmark '8C55BAB3-4DC37D37-300CBB96-D03D2CFB': The objective explicitly names Domri, and the current world registry contains one exact Domri - The Merchant placement at the authored TribeA_Task2 target in Alcarrid Camp.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

### Saga of the Adventurer

Side quest · Umbral Sands

#### 1. Use your Saga to find clues towards the treasure

- Objective key: `FindTheClues`
- Destination mode: `area`
- Confidence: **High (91/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. The game authors an area rather than one exact interaction point.
- Display note: Search the authored rocky-spire clue area north of Uzzer Camp. The cooked quest data exposes the search area, not separate publishable transforms for every slab.
- Reviewed relationship: **yes**
- Cooked location operations: `TribeB_Taskgiver1_NPC` (revealed, `QF_UmS_MainQuest_TribeB_Task1_SagaOfTheAdventurer.json` / `QFT_UpdateQuestLocation_C_1`); `TribeB.TaskGiver1` (hidden/removed, `QF_UmS_MainQuest_TribeB_Task1_SagaOfTheAdventurer.json` / `QFT_UpdateQuestLocation_C_17`); `TribeB.Task1_Target_1` (revealed, `QF_UmS_MainQuest_TribeB_Task1_SagaOfTheAdventurer.json` / `QFT_UpdateQuestLocation_C_34`)
- Marker 1: **Rocky-spire stone slab search area** (`area`) — map `0.85036793, 0.44673281`; world `346079.53, 69942.78, 3161.58`; shared-coordinate group [S96](#s96)
  - Determination: reviewed objective relationship to extracted game quest marker 'TribeB.Task1_Target_1': The compiled flow reveals TribeB.Task1_Target_1 for the first clue search. TribeB_Taskgiver1_NPC is Laguna's later return marker and is excluded from this search objective.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 2. Continue to find stone slabs. Your Saga has been updated!

- Objective key: `None`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: Search the two authored first-clue areas.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **TribeB.Task1_Target_1** (`area`) — map `0.85036793, 0.44673281`; world `346079.53, 69942.78, 3161.58`; shared-coordinate group [S96](#s96)
  - Determination: reviewed objective relationship to extracted game quest marker 'TribeB.Task1_Target_1': The first Saga state precedes the clue counter and the quest owns both initial clue targets.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **TribeB.Task1_Area_step2** (`area`) — map `0.90597829, 0.3785566`; world `369435.88, 41308.773, 3161.58`; shared-coordinate group [S102](#s102)
  - Determination: reviewed objective relationship to extracted game quest marker 'TribeB.Task1_Area_step2': The first Saga state precedes the clue counter and the quest owns both initial clue targets.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 3. Continue to find stone slabs. Your Saga has been updated!

- Objective key: `FindRemainingClues2/4`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **TribeB.Task1_Target_step2** (`location`) — map `0.90278936, 0.38289883`; world `368096.53, 43132.508, 3161.58`
  - Determination: reviewed objective relationship to extracted game quest marker 'TribeB.Task1_Target_step2': Quest flow ordering links the 2/4 state to the authored step-two target.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 4. Continue to find stone slabs. Your Saga has been updated!

- Objective key: `FindRemainingClues3/4`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **TribeB.Task1_Target_1** (`area`) — map `0.85036793, 0.44673281`; world `346079.53, 69942.78, 3161.58`; shared-coordinate group [S96](#s96)
  - Determination: reviewed objective relationship to extracted game quest marker 'TribeB.Task1_Target_1': The remaining clue state resolves to the quest's remaining authored clue target.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 5. Follow the final clue to the last stone slab. Your Saga has been updated! 

- Objective key: `LocateTheGateway`
- Destination mode: `area`
- Confidence: **High (91/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. The game authors an area rather than one exact interaction point.
- Display note: The final slab is approached through the authored gateway search area near the Crucible spire.
- Reviewed relationship: **yes**
- Cooked location operations: `TribeB.Task1_Area_step2` (revealed, `QF_UmS_MainQuest_TribeB_Task1_SagaOfTheAdventurer.json` / `QFT_UpdateQuestLocation_C_18`); `TribeB.Task1_Target_1` (hidden/removed, `QF_UmS_MainQuest_TribeB_Task1_SagaOfTheAdventurer.json` / `QFT_UpdateQuestLocation_C_3`)
- Marker 1: **Final slab — Crucible spire gateway** (`area`) — map `0.90597829, 0.3785566`; world `369435.88, 41308.773, 3161.58`; shared-coordinate group [S102](#s102)
  - Determination: reviewed objective relationship to extracted game quest marker 'TribeB.Task1_Area_step2': The compiled flow directly reveals TribeB.Task1_Area_step2 for this final-clue state, and the guide identifies the destination as the last ancient Garou statue at the Crucible of Humiliation spire.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 6. Return to Laguna - The Bard

- Objective key: `ReturnToBard`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: `TribeB_Taskgiver1_Target` (revealed, `QF_UmS_MainQuest_TribeB_Task1_SagaOfTheAdventurer.json` / `QFT_UpdateQuestLocation_C_30`)
- Marker 1: **TribeB_Taskgiver1_NPC** (`npc`) — map `0.87559324, 0.55619247`; world `356674.16, 115915.836, 1888.5864`
  - Determination: reviewed objective relationship to extracted game quest marker 'TribeB_Taskgiver1_NPC': The objective explicitly returns to Laguna and the quest's task-giver NPC and target share this authored transform.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

### Scaling Difficulties

Side quest · Umbral Sands

#### 1. Locate the dragon to the east and bring its heart back to Hedric - The Hunter

- Objective key: `FindDragon`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Display note: Defeat the red dragon at the authored Radiant encounter, then return its heart to Hedric.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **The Radiant — red dragon encounter** (`area`) — map `0.66890667, 0.76956979`; world `269865.8, 205534.31, 285.67657`
  - Determination: reviewed objective relationship to extracted game quest marker 'The Radiant': The quest owns an exact placed marker named The Radiant at the red-dragon encounter. The guide confirms the investigation leads farther east to that dragon; Hedric is the subsequent hand-in rather than the kill location.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 2. Search the village to the east for signs of the lost scouts

- Objective key: `SearchLocation`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: Inspect both fallen Garou scouts and the scorched wall in the eastern village.
- Reviewed relationship: **yes**
- Cooked location operations: `TribeC.Taskgiver2` (hidden/removed, `QF_UmS_MainQuest_TribeC_Task2_DragonHunt.json` / `QFT_UpdateQuestLocation_C_10`); `TribeC_Taskgiver2_NPC` (revealed, `QF_UmS_MainQuest_TribeC_Task2_DragonHunt.json` / `QFT_UpdateQuestLocation_C_11`); `TribeC.Task2` (revealed, `QF_UmS_MainQuest_TribeC_Task2_DragonHunt.json` / `QFT_UpdateQuestLocation_C_29`); `TribeC.Task2.1` (revealed, `QF_UmS_MainQuest_TribeC_Task2_DragonHunt.json` / `QFT_UpdateQuestLocation_C_4`); `TribeC.Task2.2` (revealed, `QF_UmS_MainQuest_TribeC_Task2_DragonHunt.json` / `QFT_UpdateQuestLocation_C_6`)
- Marker 1: **Lost scout — village ruins** (`world_actor`) — map `0.61891005, 0.76030514`; world `248867.22, 201643.16, 13.683839`
  - Determination: reviewed objective relationship to extracted game world actor 'BP_NPC_UmS_TribeC_Task2_DeadGarou': The current world contains three quest-specific placed interactions for this investigation: BP_NPC_UmS_TribeC_Task2_DeadGarou, BP_NPC_UmS_TribeC_Task2_DeadGarou1, and BP_NPC_UmS_TribeC_Task2_ScorchMarks. The guide independently confirms exactly two bodies and one scorched wall.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/0O9C1THR9X6QP37JLV2I8WZNA.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Lost scout — eastern village** (`world_actor`) — map `0.6243525, 0.7711795`; world `251153.05, 206210.39, 391.16766`
  - Determination: reviewed objective relationship to extracted game world actor 'BP_NPC_UmS_TribeC_Task2_DeadGarou1': The current world contains three quest-specific placed interactions for this investigation: BP_NPC_UmS_TribeC_Task2_DeadGarou, BP_NPC_UmS_TribeC_Task2_DeadGarou1, and BP_NPC_UmS_TribeC_Task2_ScorchMarks. The guide independently confirms exactly two bodies and one scorched wall.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/0O9C1THR9X6QP37JLV2I8WZNA.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Scorched wall clue** (`world_actor`) — map `0.62815319, 0.75541467`; world `252749.34, 199589.16, 731.24945`
  - Determination: reviewed objective relationship to extracted game world actor 'BP_NPC_UmS_TribeC_Task2_ScorchMarks': The current world contains three quest-specific placed interactions for this investigation: BP_NPC_UmS_TribeC_Task2_DeadGarou, BP_NPC_UmS_TribeC_Task2_DeadGarou1, and BP_NPC_UmS_TribeC_Task2_ScorchMarks. The guide independently confirms exactly two bodies and one scorched wall.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/8GICSE579KWCEZPPIVQHJM6NL.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 3. Return the dragon heart to Hedric - The Hunter

- Objective key: `ReturnToGarou`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: `The Radiant` (hidden/removed, `QF_UmS_MainQuest_TribeC_Task2_DragonHunt.json` / `QFT_UpdateQuestLocation_C_3`); `TribeC_Taskgiver2_Target` (revealed, `QF_UmS_MainQuest_TribeC_Task2_DragonHunt.json` / `QFT_UpdateQuestLocation_C_8`); `TribeC_Taskgiver2_NPC` (hidden/removed, `QF_UmS_MainQuest_TribeC_Task2_DragonHunt.json` / `QFT_UpdateQuestLocation_C_9`)
- Marker 1: **Hedric — The Hunter** (`location`) — map `0.52985964, 0.71722038`; world `211466.05, 183547.56, -1447.6926`
  - Determination: reviewed objective relationship to extracted game quest marker 'TribeC_Taskgiver2_Target': The flow reveals TribeC_Taskgiver2_Target after the Radiant encounter and the objective explicitly returns the heart to Hedric.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

### Song of the Ancients

Side quest · Umbral Sands

#### 1. Transcribe lyrics written on stone slabs within the Dragonkin Vault. 0/3

- Objective key: `FindStatues.0`
- Destination mode: `fixed`
- Confidence: **High (97/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Display note: All three slabs are inside the Dragonkin Vault, so the overworld map marks the vault entrance throughout the counter states.
- Reviewed relationship: **yes**
- Cooked location operations: `TribeC.Task3.Statue2` (revealed, `QF_UmS_MainQuest_TribeC_Task3_SongOfTheAncients.json` / `QFT_UpdateQuestLocation_C_0`); `TribeC.Task3.Statue3` (revealed, `QF_UmS_MainQuest_TribeC_Task3_SongOfTheAncients.json` / `QFT_UpdateQuestLocation_C_1`); `TribeC.Task3.Statue5` (revealed, `QF_UmS_MainQuest_TribeC_Task3_SongOfTheAncients.json` / `QFT_UpdateQuestLocation_C_16`); `TribeC.Task3.Statue4` (revealed, `QF_UmS_MainQuest_TribeC_Task3_SongOfTheAncients.json` / `QFT_UpdateQuestLocation_C_2`); `TribeC.Task3.Statue1` (revealed, `QF_UmS_MainQuest_TribeC_Task3_SongOfTheAncients.json` / `QFT_UpdateQuestLocation_C_31`); `TribeC.Taskgiver3` (hidden/removed, `QF_UmS_MainQuest_TribeC_Task3_SongOfTheAncients.json` / `QFT_UpdateQuestLocation_C_7`); `TribeC_Taskgiver3_NPC` (revealed, `QF_UmS_MainQuest_TribeC_Task3_SongOfTheAncients.json` / `QFT_UpdateQuestLocation_C_8`)
- Marker 1: **Dragonkin Vault entrance** (`dungeon_entrance`) — map `0.49764014, 0.17931121`; world `197933.86, -42374.29, -12370.809`; shared-coordinate group [S37](#s37)
  - Determination: reviewed objective relationship to extracted game dungeon entrance 'DragonkinVault': The walkthrough and cooked flow place all three lyric slabs inside one dungeon. Interior slab coordinates and the Manafem quest giver are not valid overworld destinations while transcribing them.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/6TBAA5Q9QN8OQMFB8O52E5TBI.json`
  - Marker confidence: 97/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 2. Transcribe lyrics written on stone slabs within the Dragonkin Vault. 1/3

- Objective key: `FindStatues.1`
- Destination mode: `fixed`
- Confidence: **High (97/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Display note: All three slabs are inside the Dragonkin Vault, so the overworld map marks the vault entrance throughout the counter states.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Dragonkin Vault entrance** (`dungeon_entrance`) — map `0.49764014, 0.17931121`; world `197933.86, -42374.29, -12370.809`; shared-coordinate group [S37](#s37)
  - Determination: reviewed objective relationship to extracted game dungeon entrance 'DragonkinVault': The walkthrough and cooked flow place all three lyric slabs inside one dungeon. Interior slab coordinates and the Manafem quest giver are not valid overworld destinations while transcribing them.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/6TBAA5Q9QN8OQMFB8O52E5TBI.json`
  - Marker confidence: 97/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 3. Transcribe lyrics written on stone slabs within the Dragonkin Vault. 2/3

- Objective key: `FindStatues.2`
- Destination mode: `fixed`
- Confidence: **High (97/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Display note: All three slabs are inside the Dragonkin Vault, so the overworld map marks the vault entrance throughout the counter states.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Dragonkin Vault entrance** (`dungeon_entrance`) — map `0.49764014, 0.17931121`; world `197933.86, -42374.29, -12370.809`; shared-coordinate group [S37](#s37)
  - Determination: reviewed objective relationship to extracted game dungeon entrance 'DragonkinVault': The walkthrough and cooked flow place all three lyric slabs inside one dungeon. Interior slab coordinates and the Manafem quest giver are not valid overworld destinations while transcribing them.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/6TBAA5Q9QN8OQMFB8O52E5TBI.json`
  - Marker confidence: 97/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 4. Return to Laguna - The Bard in the Manafem Camp. 

- Objective key: `Return`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: `TribeC.Task3.Statue1` (hidden/removed, `QF_UmS_MainQuest_TribeC_Task3_SongOfTheAncients.json` / `QFT_UpdateQuestLocation_C_27`)
- Marker 1: **TribeC_Taskgiver3_NPC** (`npc`) — map `0.52509417, 0.71318955`; world `209464.55, 181854.61, -1764.069`
  - Determination: reviewed objective relationship to extracted game quest marker 'TribeC_Taskgiver3_NPC': The objective explicitly returns to Laguna in the Manafem Camp; TribeC_Taskgiver3_NPC is the quest-owned marker at Laguna's extracted Manafem placement.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

### The Dig Site

Side quest · Umbral Sands

#### 1. Dig up Forbidden Scrolls in the garou ruins. 0/4

- Objective key: `DigScroll.0`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: The counter does not identify which scroll sites have already been dug, so all four exact sites remain visible for every 0/4 through 3/4 state.
- Reviewed relationship: **yes**
- Cooked location operations: `TribeB.Taskgiver2` (hidden/removed, `QF_UmS_MainQuest_TribeB_Task2_ArchaeologistsHope.json` / `QFT_UpdateQuestLocation_C_0`); `TribeB.Task2` (revealed, `QF_UmS_MainQuest_TribeB_Task2_ArchaeologistsHope.json` / `QFT_UpdateQuestLocation_C_19`); `TribeB_Taskgiver2_NPC` (revealed, `QF_UmS_MainQuest_TribeB_Task2_ArchaeologistsHope.json` / `QFT_UpdateQuestLocation_C_4`)
- Marker 1: **Forbidden Scroll dig site — western ruin** (`world_actor`) — map `0.91585029, 0.72473936`; world `373582.12, 186705.53, 3655.8909`; shared-coordinate group [S104](#s104)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_UmS_Mainquest_TribeB_Task2_ArchaeologyDigSite': The current world contains exactly four placed BP_UmS_Mainquest_TribeB_Task2_ArchaeologyDigSite actors. Their count and placement in four separate Garou ruins match the compiled four-scroll objective and the guide's four specific ruins.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/BV5MN3MY0YU8TSOYLHS5JMG5Y.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Forbidden Scroll dig site — lower western ruin** (`world_actor`) — map `0.89182314, 0.70796986`; world `363490.72, 179662.34, 1723.108`; shared-coordinate group [S99](#s99)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_UmS_Mainquest_TribeB_Task2_ArchaeologyDigSite2': The current world contains exactly four placed BP_UmS_Mainquest_TribeB_Task2_ArchaeologyDigSite actors. Their count and placement in four separate Garou ruins match the compiled four-scroll objective and the guide's four specific ruins.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/BV5MN3MY0YU8TSOYLHS5JMG5Y.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Forbidden Scroll dig site — southern ruin** (`world_actor`) — map `0.90001971, 0.68942286`; world `366933.28, 171872.6, 1572.0302`; shared-coordinate group [S101](#s101)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_UmS_Mainquest_TribeB_Task2_ArchaeologyDigSite3': The current world contains exactly four placed BP_UmS_Mainquest_TribeB_Task2_ArchaeologyDigSite actors. Their count and placement in four separate Garou ruins match the compiled four-scroll objective and the guide's four specific ruins.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/C70QSGAMV0KG66GO6HBR0RRCA.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **Forbidden Scroll dig site — eastern ruin** (`world_actor`) — map `0.92301562, 0.69816271`; world `376591.56, 175543.34, 3076.5308`; shared-coordinate group [S105](#s105)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_UmS_Mainquest_TribeB_Task2_ArchaeologyDigSite4': The current world contains exactly four placed BP_UmS_Mainquest_TribeB_Task2_ArchaeologyDigSite actors. Their count and placement in four separate Garou ruins match the compiled four-scroll objective and the guide's four specific ruins.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/C70QSGAMV0KG66GO6HBR0RRCA.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 2. Dig up Forbidden Scrolls in the garou ruins. 1/4

- Objective key: `DigScroll.1`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: The counter does not identify which scroll sites have already been dug, so all four exact sites remain visible for every 0/4 through 3/4 state.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Forbidden Scroll dig site — western ruin** (`world_actor`) — map `0.91585029, 0.72473936`; world `373582.12, 186705.53, 3655.8909`; shared-coordinate group [S104](#s104)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_UmS_Mainquest_TribeB_Task2_ArchaeologyDigSite': The current world contains exactly four placed BP_UmS_Mainquest_TribeB_Task2_ArchaeologyDigSite actors. Their count and placement in four separate Garou ruins match the compiled four-scroll objective and the guide's four specific ruins.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/BV5MN3MY0YU8TSOYLHS5JMG5Y.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Forbidden Scroll dig site — lower western ruin** (`world_actor`) — map `0.89182314, 0.70796986`; world `363490.72, 179662.34, 1723.108`; shared-coordinate group [S99](#s99)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_UmS_Mainquest_TribeB_Task2_ArchaeologyDigSite2': The current world contains exactly four placed BP_UmS_Mainquest_TribeB_Task2_ArchaeologyDigSite actors. Their count and placement in four separate Garou ruins match the compiled four-scroll objective and the guide's four specific ruins.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/BV5MN3MY0YU8TSOYLHS5JMG5Y.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Forbidden Scroll dig site — southern ruin** (`world_actor`) — map `0.90001971, 0.68942286`; world `366933.28, 171872.6, 1572.0302`; shared-coordinate group [S101](#s101)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_UmS_Mainquest_TribeB_Task2_ArchaeologyDigSite3': The current world contains exactly four placed BP_UmS_Mainquest_TribeB_Task2_ArchaeologyDigSite actors. Their count and placement in four separate Garou ruins match the compiled four-scroll objective and the guide's four specific ruins.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/C70QSGAMV0KG66GO6HBR0RRCA.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **Forbidden Scroll dig site — eastern ruin** (`world_actor`) — map `0.92301562, 0.69816271`; world `376591.56, 175543.34, 3076.5308`; shared-coordinate group [S105](#s105)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_UmS_Mainquest_TribeB_Task2_ArchaeologyDigSite4': The current world contains exactly four placed BP_UmS_Mainquest_TribeB_Task2_ArchaeologyDigSite actors. Their count and placement in four separate Garou ruins match the compiled four-scroll objective and the guide's four specific ruins.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/C70QSGAMV0KG66GO6HBR0RRCA.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 3. Dig up Forbidden Scrolls in the garou ruins. 2/4

- Objective key: `DigScroll.2`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: The counter does not identify which scroll sites have already been dug, so all four exact sites remain visible for every 0/4 through 3/4 state.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Forbidden Scroll dig site — western ruin** (`world_actor`) — map `0.91585029, 0.72473936`; world `373582.12, 186705.53, 3655.8909`; shared-coordinate group [S104](#s104)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_UmS_Mainquest_TribeB_Task2_ArchaeologyDigSite': The current world contains exactly four placed BP_UmS_Mainquest_TribeB_Task2_ArchaeologyDigSite actors. Their count and placement in four separate Garou ruins match the compiled four-scroll objective and the guide's four specific ruins.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/BV5MN3MY0YU8TSOYLHS5JMG5Y.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Forbidden Scroll dig site — lower western ruin** (`world_actor`) — map `0.89182314, 0.70796986`; world `363490.72, 179662.34, 1723.108`; shared-coordinate group [S99](#s99)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_UmS_Mainquest_TribeB_Task2_ArchaeologyDigSite2': The current world contains exactly four placed BP_UmS_Mainquest_TribeB_Task2_ArchaeologyDigSite actors. Their count and placement in four separate Garou ruins match the compiled four-scroll objective and the guide's four specific ruins.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/BV5MN3MY0YU8TSOYLHS5JMG5Y.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Forbidden Scroll dig site — southern ruin** (`world_actor`) — map `0.90001971, 0.68942286`; world `366933.28, 171872.6, 1572.0302`; shared-coordinate group [S101](#s101)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_UmS_Mainquest_TribeB_Task2_ArchaeologyDigSite3': The current world contains exactly four placed BP_UmS_Mainquest_TribeB_Task2_ArchaeologyDigSite actors. Their count and placement in four separate Garou ruins match the compiled four-scroll objective and the guide's four specific ruins.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/C70QSGAMV0KG66GO6HBR0RRCA.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **Forbidden Scroll dig site — eastern ruin** (`world_actor`) — map `0.92301562, 0.69816271`; world `376591.56, 175543.34, 3076.5308`; shared-coordinate group [S105](#s105)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_UmS_Mainquest_TribeB_Task2_ArchaeologyDigSite4': The current world contains exactly four placed BP_UmS_Mainquest_TribeB_Task2_ArchaeologyDigSite actors. Their count and placement in four separate Garou ruins match the compiled four-scroll objective and the guide's four specific ruins.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/C70QSGAMV0KG66GO6HBR0RRCA.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 4. Dig up Forbidden Scrolls in the garou ruins. 3/4

- Objective key: `DigScroll.3`
- Destination mode: `multiple`
- Confidence: **High (93/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data. Several destinations are valid or runtime progress cannot identify which one remains.
- Display note: The counter does not identify which scroll sites have already been dug, so all four exact sites remain visible for every 0/4 through 3/4 state.
- Reviewed relationship: **yes**
- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.
- Marker 1: **Forbidden Scroll dig site — western ruin** (`world_actor`) — map `0.91585029, 0.72473936`; world `373582.12, 186705.53, 3655.8909`; shared-coordinate group [S104](#s104)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_UmS_Mainquest_TribeB_Task2_ArchaeologyDigSite': The current world contains exactly four placed BP_UmS_Mainquest_TribeB_Task2_ArchaeologyDigSite actors. Their count and placement in four separate Garou ruins match the compiled four-scroll objective and the guide's four specific ruins.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/BV5MN3MY0YU8TSOYLHS5JMG5Y.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 2: **Forbidden Scroll dig site — lower western ruin** (`world_actor`) — map `0.89182314, 0.70796986`; world `363490.72, 179662.34, 1723.108`; shared-coordinate group [S99](#s99)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_UmS_Mainquest_TribeB_Task2_ArchaeologyDigSite2': The current world contains exactly four placed BP_UmS_Mainquest_TribeB_Task2_ArchaeologyDigSite actors. Their count and placement in four separate Garou ruins match the compiled four-scroll objective and the guide's four specific ruins.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/BV5MN3MY0YU8TSOYLHS5JMG5Y.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 3: **Forbidden Scroll dig site — southern ruin** (`world_actor`) — map `0.90001971, 0.68942286`; world `366933.28, 171872.6, 1572.0302`; shared-coordinate group [S101](#s101)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_UmS_Mainquest_TribeB_Task2_ArchaeologyDigSite3': The current world contains exactly four placed BP_UmS_Mainquest_TribeB_Task2_ArchaeologyDigSite actors. Their count and placement in four separate Garou ruins match the compiled four-scroll objective and the guide's four specific ruins.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/C70QSGAMV0KG66GO6HBR0RRCA.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Marker 4: **Forbidden Scroll dig site — eastern ruin** (`world_actor`) — map `0.92301562, 0.69816271`; world `376591.56, 175543.34, 3076.5308`; shared-coordinate group [S105](#s105)
  - Determination: reviewed objective relationship to extracted game world actor 'BP_UmS_Mainquest_TribeB_Task2_ArchaeologyDigSite4': The current world contains exactly four placed BP_UmS_Mainquest_TribeB_Task2_ArchaeologyDigSite actors. Their count and placement in four separate Garou ruins match the compiled four-scroll objective and the guide's four specific ruins.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World/_Generated_/C70QSGAMV0KG66GO6HBR0RRCA.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

#### 5. Return to Lehran - The Archaeologist in the Uzzer Camp

- Objective key: `ReturnToArchaeologist`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Reviewed relationship: **yes**
- Cooked location operations: `TribeB.Task2` (hidden/removed, `QF_UmS_MainQuest_TribeB_Task2_ArchaeologistsHope.json` / `QFT_UpdateQuestLocation_C_3`); `TribeB_Taskgiver2_NPC` (hidden/removed, `QF_UmS_MainQuest_TribeB_Task2_ArchaeologistsHope.json` / `QFT_UpdateQuestLocation_C_6`); `TribeB_Taskgiver2_Target` (revealed, `QF_UmS_MainQuest_TribeB_Task2_ArchaeologistsHope.json` / `QFT_UpdateQuestLocation_C_7`)
- Marker 1: **Lehran — The Archaeologist** (`location`) — map `0.88071876, 0.56459781`; world `358826.88, 119446.08, 2304.1367`
  - Determination: reviewed objective relationship to extracted game quest marker 'TribeB_Taskgiver2_Target': The flow reveals TribeB_Taskgiver2_Target after all four scrolls and the objective explicitly returns to Lehran in Uzzer Camp.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

### The Mirror Crack'd

Side quest · Umbral Sands

#### 1. Bring Valas - The Blacksmith 3 adamant bars

- Objective key: `BringBars`
- Destination mode: `fixed`
- Confidence: **High (96/100)**
- Assessment: Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.
- Display note: Adamant ore and bars can be sourced outside the suggested search area; the required progression interaction is the hand-in to Valas.
- Reviewed relationship: **yes**
- Cooked location operations: `TribeA.Taskgiver3` (hidden/removed, `QF_UmS_TribeA_Task3_TheMirrorCracked.json` / `QFT_UpdateQuestLocation_C_15`); `TribeA.Task3` (revealed, `QF_UmS_TribeA_Task3_TheMirrorCracked.json` / `QFT_UpdateQuestLocation_C_25`); `TribeA_Task3_NPC` (revealed, `QF_UmS_TribeA_Task3_TheMirrorCracked.json` / `QFT_UpdateQuestLocation_C_6`)
- Marker 1: **Valas — The Blacksmith** (`location`) — map `0.62622714, 0.55769507`; world `251940.4, 116546.93, 3035.5615`
  - Determination: reviewed objective relationship to extracted game registry landmark 'F8250830-405F796E-EC7E0581-8790FA54': The objective explicitly brings three adamant bars to Valas and the current world registry contains one exact Valas - The Blacksmith placement. The broad TribeA.Task3 area is optional sourcing guidance, not a required destination.
  - Source: `dist/recipe-unlock-all-json-current/RSDragonwilds/Content/Maps/World/L_World.json`
  - Marker confidence: 96/100 — Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.

## Exact shared map coordinates

These groups use numerically identical normalized `x` and `y` values. Most are intentional: counter variants, repeated return objectives, an NPC and its colocated quest target, or several states within the same dungeon.

### S1

Map coordinate: `0.06214134, 0.47796524` · 2 distinct objective states

- **A Melody Remembered** — `Objective1`: The jewellery box requires a specific ring. — marker `Jewellery Box — Inspiring Ring of Runecrafting`
- **A Melody Remembered** — `Objective2`: The jewellery box requires a specific amulet. — marker `Jewellery Box — Amulet of Magic`

### S2

Map coordinate: `0.07216621, 0.63033771` · 2 distinct objective states

- **Highlighting the Problem** — `Search the runecrafting guild for wizard statue`: Search the Runecrafting Guild for a statue. — marker `Runecrafting Guild wizard statue`
- **Highlighting the Problem** — `Return to statue`: Return to the wizard statue. — marker `Runecrafting Guild wizard statue (charged-orb return)`

### S3

Map coordinate: `0.08822407, 0.51816455` · 2 distinct objective states

- **Dog Days** — `Objective1`: Search Garou camps beneath giant wyrm bones in the Highlands for information on Garou ritual practices. — marker `BP_LoreItem_DogDays_DD1`
- **Dog Days** — `Objective3`: Search garou camps beneath the giant wyrm bones in the Highlands for information on the King's Token. — marker `BP_LoreItem_DogDays_DD1`

### S4

Map coordinate: `0.09267786, 0.18263629` · 2 distinct objective states

- **Mirror Mirror** — `FindSilverThread`: Gather dragon blood, spectral silver thread and a ritual spellbook and bring them back to the mirror. — marker `Magic Mirror — reagent hand-in`
- **Mirror Mirror** — `ReturnToNPC`: Speak to the mirror. — marker `Magic Mirror`

### S5

Map coordinate: `0.11618128, 0.6959686` · 2 distinct objective states

- **Restless Ghosts** — `Found ghost amulet`: Search for a purpose for the Amulet of Ghostspeak. — marker `Warren`
- **Restless Ghosts** — `Put ghosts to rest`: Find the restless spirits and help lay them to rest. They only appear at night.  — marker `Warren`

### S6

Map coordinate: `0.1289099, 0.56793729` · 3 distinct objective states

- **Cook's Assistant** — `Objective1`: Gather 4 eggs, 6 wheat and a bucket of milk, and bring them all to Slopfinger. — marker `Slopfinger (ingredient hand-in)`
- **Cook's Assistant** — `Objective4`: Show Slopfinger the freshly cooked cake. — marker `Slopfinger — have the cake signed`
- **Cook's Assistant** — `Objective5`: Give Slopfinger the good news. — marker `Slopfinger — report the good news`

### S7

Map coordinate: `0.1303523, 0.56805707` · 4 distinct objective states

- **Animal Magnetism** — `Objective2`: Discuss the advert with Manktongue — marker `Manktongue`
- **Animal Magnetism** — `Objective3`: Gather an undead chicken, magnetised iron bar and undead twigs, and bring them to Manktongue — marker `Manktongue — component hand-in`
- **Animal Magnetism** — `Objective4`: Return to Manktongue — marker `Manktongue`
- **Cook's Assistant** — `Objective3`: Bring the signed cake to Manktongue. — marker `Manktongue — signed cake delivery`

### S8

Map coordinate: `0.13132323, 0.50114205` · 2 distinct objective states

- **Dog Days** — `Objective5`: Bring the Purified King's Token to Vannaka in the Ghornfell Highlands. — marker `Vannaka — Ghornfell Highlands`
- **Dog Days** — `Objective6`: Search the Ghornfell Highlands for a friendly face with an interest in Garou rituals. — marker `Vannaka_DogDays`

### S9

Map coordinate: `0.1419211, 0.70314926` · 3 distinct objective states

- **Dragon Slayer** — `Return to the Wise old man after claimed bed`: Speak to the Wise Old Man in Bramblemead village. — marker `Wise Old Man — Bramblemead`
- **Dragon Slayer** — `Find and hunt a deer`: Hunt a deer and bring one of its antlers to the Wise Old Man. — marker `Wise Old Man — antler hand-in`
- **Dragon Slayer** — `Return to the Wise old man`: Speak to the Wise Old Man in Bramblemead village. — marker `Wise Old Man — Bramblemead`

### S10

Map coordinate: `0.1530686, 0.12794853` · 3 distinct objective states

- **Even More Restless Ghosts** — `FindGhosts0/3`: Find ghosts and complete their unfinished business. (0/3) — marker `Windswept Ghost`
- **Even More Restless Ghosts** — `FindGhosts1/3`: Find ghosts and complete their unfinished business. (1/3) — marker `Windswept Ghost`
- **Even More Restless Ghosts** — `FindGhosts2/3`: Find ghosts and complete their unfinished business. (2/3) — marker `Windswept Ghost`

### S11

Map coordinate: `0.17002493, 0.14765352` · 3 distinct objective states

- **Withering Heights** — `BreakTheThreeSeals`: Gather three curse fragments from the locations described in the standing stones. — marker `Curse Fragment of Grief — Emberwood Village`
- **Withering Heights** — `BreakRemainingtwoSeals`: Gather two more curse fragments from the locations described in the standing stones. — marker `Curse Fragment of Grief — Emberwood Village`
- **Withering Heights** — `BreakTheFinalSeal`: Find the last curse fragment from the locations described in the standing stones. — marker `Curse Fragment of Grief — Emberwood Village`

### S12

Map coordinate: `0.20321014, 0.6563285` · 2 distinct objective states

- **Goblin Diplomacy** — `Objective4`: Bring both amulets to Zanik. — marker `ZanikGoblinDeplomacy`
- **Goblin Diplomacy** — `Objective6`: Find someone that can identify the Bandosian amulet. — marker `Zanik (amulet identification)`

### S13

Map coordinate: `0.2182855, 0.69625933` · 3 distinct objective states

- **Goblin Diplomacy** — `Objective1`: Investigate the ruins to the northeast and the guild to the south of Zanik. — marker `Adventurers' Guild reliquary`
- **Goblin Diplomacy** — `Objective2b`: Investigate the guild south of Zanik. — marker `Adventurers' Guild reliquary`
- **Goblin Diplomacy** — `Objective5`: Look for the other amulet at either the ruins northeast of Zanik, or the guild to the south. — marker `Adventurers' Guild reliquary`

### S14

Map coordinate: `0.21939348, 0.64779114` · 3 distinct objective states

- **Goblin Diplomacy** — `Objective1`: Investigate the ruins to the northeast and the guild to the south of Zanik. — marker `Outlook Ruins reliquary`
- **Goblin Diplomacy** — `Objective2a`: Investigate the ruin northeast of Zanik. — marker `Outlook Ruins reliquary`
- **Goblin Diplomacy** — `Objective5`: Look for the other amulet at either the ruins northeast of Zanik, or the guild to the south. — marker `Outlook Ruins reliquary`

### S15

Map coordinate: `0.25361976, 0.42259874` · 5 distinct objective states

- **Regicide** — `DefeatKuldra`: Defeat Kuldra — marker `SW_Door`
- **Regicide** — `BringHead`: Bring Kuldra's head to Zilyana.  — marker `SW_Door`
- **Regicide** — `AccessSW`: Gain access to the Scorned Wilderness.  — marker `Dragon Gate — Scorned Wilderness access`
- **Regicide** — `SpeakZilyana`: Speak to Zilyana.  — marker `SW_Door`
- **Regicide** — `ReturnToZilyana`: Once you are fully prepared, speak to Zilyana.  — marker `SW_Door`

### S16

Map coordinate: `0.32324324, 0.60940838` · 3 distinct objective states

- **The Great Body Robbery** — `BodyParts.0`: Dig up body parts from graves around the Bloodblight Swamps. 0/3 — marker `Garou Arms grave`
- **The Great Body Robbery** — `BodyParts.1`: Dig up body parts from graves around the Bloodblight Swamps. 1/3 — marker `Garou Arms grave`
- **The Great Body Robbery** — `BodyParts.2`: Dig up body parts from graves around the Bloodblight Swamps. 2/3 — marker `Garou Arms grave`

### S17

Map coordinate: `0.32377924, 0.65373062` · 3 distinct objective states

- **The Great Body Robbery** — `Objecrive3.5`: Perform an autopsy of the body on the examination table in Doctor Fenkenstrain's tower.  — marker `Examination Table`
- **The Great Body Robbery** — `Objective7`: Put Doctor Fenkenstrain's head on his new body on the examination table.  — marker `Examination Table`
- **The Great Body Robbery** — `AddPartsToTable`: Place the body parts on the examination table in Fenkenstrain's tower. — marker `Examination Table`

### S18

Map coordinate: `0.32433942, 0.65421714` · 6 distinct objective states

- **The Great Body Robbery** — `Objective2`: Speak to Fenkenstrain — marker `Doctor Fenkenstrain`
- **The Great Body Robbery** — `Objective3`: Speak to Fenkenstrain — marker `Doctor Fenkenstrain`
- **The Great Body Robbery** — `Objective4`: Bring Dr Fenkenstrain: 1 Weak Antipoison, 1 Weak Focused Attack Potion and 1 Weak Lumberjack Potion. — marker `Doctor Fenkenstrain`
- **The Great Body Robbery** — `Objective5`: Collect lightning in a bottle and bring it to Dr Fenkenstrain. — marker `Doctor Fenkenstrain — lightning bottle hand-in`
- **The Great Body Robbery** — `Objective6`: Speak to Fenkenstrain — marker `Doctor Fenkenstrain`
- **The Great Body Robbery** — `Objective9`: Speak to Fenkenstrain — marker `Doctor Fenkenstrain`

### S19

Map coordinate: `0.32752136, 0.20833522` · 3 distinct objective states

- **The Wild Hunt** — `HuntBeast`: Hunt down Nature Spirits near Fellhollow's willow groves and bring three of their hearts to Vannaka.  — marker `Vannaka — Primordial Heart hand-in`
- **The Wild Hunt** — `BringHeart`: Bring the hearts to Vannaka.  — marker `Vannaka`
- **The Wild Hunt** — `BringHeartNotStarted`: Bring the Primordial Heart to someone with an interest in the wild hunt.  — marker `VannakaNPC`

### S20

Map coordinate: `0.32753919, 0.20839084` · 2 distinct objective states

- **Things That Go Boom In The Night** — `FindSpectralChinchompaSkulls`: Gather 10 spectral chinchompa skulls and bring them to Vannaka in Fellhollow. — marker `Vannaka`
- **Things That Go Boom In The Night** — `SpeaktoVannaka`: Speak to Vannaka in Fellhollow. — marker `Vannaka`

### S21

Map coordinate: `0.33398188, 0.36679069` · 2 distinct objective states

- **Withering Heights** — `SearchTheStructure`: Speak to the temple's inhabitant. — marker `Death — Forgotten Temple`
- **Withering Heights** — `SpeakToMysteriousFigure`: Speak to Death. — marker `Death — Forgotten Temple`

### S22

Map coordinate: `0.33492895, 0.62215262` · 3 distinct objective states

- **The Great Body Robbery** — `BodyParts.0`: Dig up body parts from graves around the Bloodblight Swamps. 0/3 — marker `Human torso grave`
- **The Great Body Robbery** — `BodyParts.1`: Dig up body parts from graves around the Bloodblight Swamps. 1/3 — marker `Human torso grave`
- **The Great Body Robbery** — `BodyParts.2`: Dig up body parts from graves around the Bloodblight Swamps. 2/3 — marker `Human torso grave`

### S23

Map coordinate: `0.33686819, 0.10300426` · 3 distinct objective states

- **Even More Restless Ghosts** — `FindGhosts0/3`: Find ghosts and complete their unfinished business. (0/3) — marker `Reminiscing Ghost`
- **Even More Restless Ghosts** — `FindGhosts1/3`: Find ghosts and complete their unfinished business. (1/3) — marker `Reminiscing Ghost`
- **Even More Restless Ghosts** — `FindGhosts2/3`: Find ghosts and complete their unfinished business. (2/3) — marker `Reminiscing Ghost`

### S24

Map coordinate: `0.3385789, 0.61351933` · 3 distinct objective states

- **The Great Body Robbery** — `BodyParts.0`: Dig up body parts from graves around the Bloodblight Swamps. 0/3 — marker `Goblin Legs grave`
- **The Great Body Robbery** — `BodyParts.1`: Dig up body parts from graves around the Bloodblight Swamps. 1/3 — marker `Goblin Legs grave`
- **The Great Body Robbery** — `BodyParts.2`: Dig up body parts from graves around the Bloodblight Swamps. 2/3 — marker `Goblin Legs grave`

### S25

Map coordinate: `0.34258817, 0.33918205` · 3 distinct objective states

- **Withering Heights** — `SpeakToPostiePete`: Speak to Postie Pete. — marker `Postie Pete — Forgotten Temple`
- **Letters For The Dead** — `ObjectiveReturn`: Return to Postie Pete. — marker `Postie Pete (Fellhollow)`
- **Letters For The Dead** — `ObjectiveZero`: Visit Postie Pete near the entrance to Fellhollow to begin searching for lost letters. — marker `Postie Pete (Fellhollow)`

### S26

Map coordinate: `0.3545681, 0.23334665` · 4 distinct objective states

- **Withering Heights** — `InvestigateStandingStones`: Find Cathan and look into what he was investigating. — marker `Standing Stone — first curse clue`
- **Withering Heights** — `ReachedStandingStones`: Get location of curse fragments 0/3. — marker `Standing Stone — village clue`
- **Withering Heights** — `InteractedWithStone1`: Get location of curse fragments 1/3. — marker `Standing Stone — village clue`
- **Withering Heights** — `InteractedWithStone2`: Get location of curse fragments 2/3. — marker `Standing Stone — village clue`

### S27

Map coordinate: `0.35814121, 0.2327137` · 4 distinct objective states

- **Withering Heights** — `InvestigateStandingStones`: Find Cathan and look into what he was investigating. — marker `Standing Stone — third curse clue`
- **Withering Heights** — `ReachedStandingStones`: Get location of curse fragments 0/3. — marker `Standing Stone — fortress clue`
- **Withering Heights** — `InteractedWithStone1`: Get location of curse fragments 1/3. — marker `Standing Stone — fortress clue`
- **Withering Heights** — `InteractedWithStone2`: Get location of curse fragments 2/3. — marker `Standing Stone — fortress clue`

### S28

Map coordinate: `0.36072019, 0.23483984` · 4 distinct objective states

- **Withering Heights** — `InvestigateStandingStones`: Find Cathan and look into what he was investigating. — marker `Standing Stone — second curse clue`
- **Withering Heights** — `ReachedStandingStones`: Get location of curse fragments 0/3. — marker `Standing Stone — waterfall clue`
- **Withering Heights** — `InteractedWithStone1`: Get location of curse fragments 1/3. — marker `Standing Stone — waterfall clue`
- **Withering Heights** — `InteractedWithStone2`: Get location of curse fragments 2/3. — marker `Standing Stone — waterfall clue`

### S29

Map coordinate: `0.3649939, 0.5006833` · 2 distinct objective states

- **Granite Mauled** — `Objective2`: Find someone near Ashien's Watch that can translate ancient dwarvish.  — marker `Doric_GM`
- **Granite Mauled** — `Objective3`: Bring the blueprint to Doric in Ashien's Watch. — marker `Doric — Ashien's Watch`

### S30

Map coordinate: `0.36963329, 0.14279896` · 5 distinct objective states

- **Withering Heights** — `BreakTheThreeSeals`: Gather three curse fragments from the locations described in the standing stones. — marker `Curse Fragment of Rage — Vekchenven Kara entrance`
- **Withering Heights** — `BreakRemainingtwoSeals`: Gather two more curse fragments from the locations described in the standing stones. — marker `Curse Fragment of Rage — Vekchenven Kara entrance`
- **Withering Heights** — `BreakTheFinalSeal`: Find the last curse fragment from the locations described in the standing stones. — marker `Curse Fragment of Rage — Vekchenven Kara entrance`
- **Doric's Quest** — `FindShadowSword`: Craft a Shadow Sword and bring it to Doric in Fellhollow. Doric believes a zogre has the recipe. — marker `Zogre boss — Vekchenven Kara entrance`
- **Doric's Quest** — `FindNecromancerStaff`: Craft a Necromancer's Staff and bring it to Doric in Fellhollow. Doric believes the recipe is in a vault somewhere. — marker `Necromancer's Staff — Vekchenven Kara entrance`

### S31

Map coordinate: `0.39156857, 0.22961113` · 2 distinct objective states

- **Even More Restless Ghosts** — `BackToZanik`: Return to Zanik. — marker `Zanik`
- **Even More Restless Ghosts** — `BackToZanikNotSpokenToZanik`: Tell someone about these spirits.  — marker `ZanikNPC`

### S32

Map coordinate: `0.46599245, 0.10918731` · 3 distinct objective states

- **Even More Restless Ghosts** — `FindGhosts0/3`: Find ghosts and complete their unfinished business. (0/3) — marker `Paint-Spattered Ghost`
- **Even More Restless Ghosts** — `FindGhosts1/3`: Find ghosts and complete their unfinished business. (1/3) — marker `Paint-Spattered Ghost`
- **Even More Restless Ghosts** — `FindGhosts2/3`: Find ghosts and complete their unfinished business. (2/3) — marker `Paint-Spattered Ghost`

### S33

Map coordinate: `0.47397869, 0.22572224` · 2 distinct objective states

- **What Remains is Written** — `FindBooks`: Find five of Abraxus’ missing books in eastern Fellhollow and return them to him. — marker `Leonard The Bound`
- **What Remains is Written** — `FindBooksAndReturn`: Find five of Abraxus’ missing books in eastern Fellhollow and return them to him. — marker `Leonard The Bound`

### S34

Map coordinate: `0.4883775, 0.17938842` · 2 distinct objective states

- **Seeking Salvation** — `ReturnToDoric`: Bring the dwarven writing to Doric.  — marker `Doric — Skekven Kara ruins`
- **Seeking Salvation** — `LookForDoric`: Find someone in Fellhollow that can translate dwarvish text.  — marker `Doric — Skekven Kara ruins`

### S35

Map coordinate: `0.48839286, 0.17934524` · 4 distinct objective states

- **Doric's Quest** — `FindSkeletonShortbow`: Craft an Undead Ranger's Bow and bring it to Doric in Fellhollow. Doric believes the recipe is in a vault somewhere. — marker `Doric — bow hand-in`
- **Doric's Quest** — `FindShadowSword`: Craft a Shadow Sword and bring it to Doric in Fellhollow. Doric believes a zogre has the recipe. — marker `Doric — sword hand-in`
- **Doric's Quest** — `FindNecromancerStaff`: Craft a Necromancer's Staff and bring it to Doric in Fellhollow. Doric believes the recipe is in a vault somewhere. — marker `Doric — staff hand-in`
- **Doric's Quest** — `SpeakToDoric`: Speak to Doric in Fellhollow. — marker `Doric — Fellhollow`

### S36

Map coordinate: `0.49727614, 0.17935909` · 2 distinct objective states

- **Doric's Quest** — `FindSkeletonShortbow`: Craft an Undead Ranger's Bow and bring it to Doric in Fellhollow. Doric believes the recipe is in a vault somewhere. — marker `Undead Ranger's Bow — Skekven Kara entrance`
- **Seeking Salvation** — `FindRecipe`: Access the vault and find the hidden secret within.  — marker `Hidden dwarven writing — Skekven Kara entrance`

### S37

Map coordinate: `0.49764014, 0.17931121` · 3 distinct objective states

- **Song of the Ancients** — `FindStatues.0`: Transcribe lyrics written on stone slabs within the Dragonkin Vault. 0/3 — marker `Dragonkin Vault entrance`
- **Song of the Ancients** — `FindStatues.1`: Transcribe lyrics written on stone slabs within the Dragonkin Vault. 1/3 — marker `Dragonkin Vault entrance`
- **Song of the Ancients** — `FindStatues.2`: Transcribe lyrics written on stone slabs within the Dragonkin Vault. 2/3 — marker `Dragonkin Vault entrance`

### S38

Map coordinate: `0.50024662, 0.4335252` · 3 distinct objective states

- **Wanted!** — `DR_SQ2_FindPages_0`: Journal Pages Collected 0/3 — marker `Rat Master's Journal — Menagerie`
- **Wanted!** — `DR_SQ2_FindPages_1`: Journal Pages Collected 1/3 — marker `Rat Master's Journal — Menagerie`
- **Wanted!** — `DR_SQ2_FindPages_2`: Journal Pages Collected 2/3 — marker `Rat Master's Journal — Menagerie`

### S39

Map coordinate: `0.50379445, 0.10259045` · 6 distinct objective states

- **What Remains is Written** — `FindBooks`: Find five of Abraxus’ missing books in eastern Fellhollow and return them to him. — marker `Abraxus The Eternal`
- **What Remains is Written** — `FoundBooks`: Bring the books to Abraxus. — marker `Abraxus The Eternal`
- **What Remains is Written** — `FoundScrolls`: Bring the scrolls to Abraxus. — marker `Abraxus The Eternal`
- **What Remains is Written** — `FindBooksAndReturn`: Find five of Abraxus’ missing books in eastern Fellhollow and return them to him. — marker `Abraxus The Eternal`
- **What Remains is Written** — `FindScrollsAndReturn`: Defeat Abraxus' enemies to retrieve the scrolls. Then return them to Abraxus. — marker `Abraxus The Eternal`
- **What Remains is Written** — `ObjectiveZero`: Find the owner of the sinister text.  — marker `Abraxus The Eternal`

### S40

Map coordinate: `0.50478571, 0.37952143` · 13 distinct objective states

- **Black Knight's Fortress** — `DR_Main_Objective_4`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 0/13 — marker `DR_MQ_Fragment_3`
- **Black Knight's Fortress** — `DR_Main_Objective_4_1`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 1/13 — marker `DR_MQ_Fragment_3`
- **Black Knight's Fortress** — `DR_Main_Objective_4_2`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 2/13 — marker `DR_MQ_Fragment_3`
- **Black Knight's Fortress** — `DR_Main_Objective_4_3`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 3/13 — marker `DR_MQ_Fragment_3`
- **Black Knight's Fortress** — `DR_Main_Objective_4_4`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 4/13 — marker `DR_MQ_Fragment_3`
- **Black Knight's Fortress** — `DR_Main_Objective_4_5`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 5/13 — marker `DR_MQ_Fragment_3`
- **Black Knight's Fortress** — `DR_Main_Objective_4_6`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 6/13 — marker `DR_MQ_Fragment_3`
- **Black Knight's Fortress** — `DR_Main_Objective_4_7`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 7/13 — marker `DR_MQ_Fragment_3`
- **Black Knight's Fortress** — `DR_Main_Objective_4_8`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 8/13 — marker `DR_MQ_Fragment_3`
- **Black Knight's Fortress** — `DR_Main_Objective_4_9`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 9/13 — marker `DR_MQ_Fragment_3`
- **Black Knight's Fortress** — `DR_Main_Objective_4_10`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 10/13 — marker `DR_MQ_Fragment_3`
- **Black Knight's Fortress** — `DR_Main_Objective_4_11`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 11/13 — marker `DR_MQ_Fragment_3`
- **Black Knight's Fortress** — `DR_Main_Objective_4_12`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 12/13 — marker `DR_MQ_Fragment_3`

### S41

Map coordinate: `0.50505952, 0.44700476` · 13 distinct objective states

- **Black Knight's Fortress** — `DR_Main_Objective_4`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 0/13 — marker `DR_MQ_Fragment_12`
- **Black Knight's Fortress** — `DR_Main_Objective_4_1`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 1/13 — marker `DR_MQ_Fragment_12`
- **Black Knight's Fortress** — `DR_Main_Objective_4_2`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 2/13 — marker `DR_MQ_Fragment_12`
- **Black Knight's Fortress** — `DR_Main_Objective_4_3`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 3/13 — marker `DR_MQ_Fragment_12`
- **Black Knight's Fortress** — `DR_Main_Objective_4_4`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 4/13 — marker `DR_MQ_Fragment_12`
- **Black Knight's Fortress** — `DR_Main_Objective_4_5`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 5/13 — marker `DR_MQ_Fragment_12`
- **Black Knight's Fortress** — `DR_Main_Objective_4_6`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 6/13 — marker `DR_MQ_Fragment_12`
- **Black Knight's Fortress** — `DR_Main_Objective_4_7`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 7/13 — marker `DR_MQ_Fragment_12`
- **Black Knight's Fortress** — `DR_Main_Objective_4_8`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 8/13 — marker `DR_MQ_Fragment_12`
- **Black Knight's Fortress** — `DR_Main_Objective_4_9`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 9/13 — marker `DR_MQ_Fragment_12`
- **Black Knight's Fortress** — `DR_Main_Objective_4_10`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 10/13 — marker `DR_MQ_Fragment_12`
- **Black Knight's Fortress** — `DR_Main_Objective_4_11`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 11/13 — marker `DR_MQ_Fragment_12`
- **Black Knight's Fortress** — `DR_Main_Objective_4_12`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 12/13 — marker `DR_MQ_Fragment_12`

### S42

Map coordinate: `0.50978095, 0.40071429` · 13 distinct objective states

- **Black Knight's Fortress** — `DR_Main_Objective_4`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 0/13 — marker `DR_MQ_Fragment_11`
- **Black Knight's Fortress** — `DR_Main_Objective_4_1`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 1/13 — marker `DR_MQ_Fragment_11`
- **Black Knight's Fortress** — `DR_Main_Objective_4_2`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 2/13 — marker `DR_MQ_Fragment_11`
- **Black Knight's Fortress** — `DR_Main_Objective_4_3`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 3/13 — marker `DR_MQ_Fragment_11`
- **Black Knight's Fortress** — `DR_Main_Objective_4_4`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 4/13 — marker `DR_MQ_Fragment_11`
- **Black Knight's Fortress** — `DR_Main_Objective_4_5`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 5/13 — marker `DR_MQ_Fragment_11`
- **Black Knight's Fortress** — `DR_Main_Objective_4_6`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 6/13 — marker `DR_MQ_Fragment_11`
- **Black Knight's Fortress** — `DR_Main_Objective_4_7`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 7/13 — marker `DR_MQ_Fragment_11`
- **Black Knight's Fortress** — `DR_Main_Objective_4_8`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 8/13 — marker `DR_MQ_Fragment_11`
- **Black Knight's Fortress** — `DR_Main_Objective_4_9`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 9/13 — marker `DR_MQ_Fragment_11`
- **Black Knight's Fortress** — `DR_Main_Objective_4_10`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 10/13 — marker `DR_MQ_Fragment_11`
- **Black Knight's Fortress** — `DR_Main_Objective_4_11`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 11/13 — marker `DR_MQ_Fragment_11`
- **Black Knight's Fortress** — `DR_Main_Objective_4_12`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 12/13 — marker `DR_MQ_Fragment_11`

### S43

Map coordinate: `0.51314174, 0.44434374` · 13 distinct objective states

- **Black Knight's Fortress** — `DR_Main_Objective_4`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 0/13 — marker `DR_MQ_Fragment_7`
- **Black Knight's Fortress** — `DR_Main_Objective_4_1`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 1/13 — marker `DR_MQ_Fragment_7`
- **Black Knight's Fortress** — `DR_Main_Objective_4_2`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 2/13 — marker `DR_MQ_Fragment_7`
- **Black Knight's Fortress** — `DR_Main_Objective_4_3`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 3/13 — marker `DR_MQ_Fragment_7`
- **Black Knight's Fortress** — `DR_Main_Objective_4_4`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 4/13 — marker `DR_MQ_Fragment_7`
- **Black Knight's Fortress** — `DR_Main_Objective_4_5`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 5/13 — marker `DR_MQ_Fragment_7`
- **Black Knight's Fortress** — `DR_Main_Objective_4_6`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 6/13 — marker `DR_MQ_Fragment_7`
- **Black Knight's Fortress** — `DR_Main_Objective_4_7`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 7/13 — marker `DR_MQ_Fragment_7`
- **Black Knight's Fortress** — `DR_Main_Objective_4_8`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 8/13 — marker `DR_MQ_Fragment_7`
- **Black Knight's Fortress** — `DR_Main_Objective_4_9`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 9/13 — marker `DR_MQ_Fragment_7`
- **Black Knight's Fortress** — `DR_Main_Objective_4_10`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 10/13 — marker `DR_MQ_Fragment_7`
- **Black Knight's Fortress** — `DR_Main_Objective_4_11`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 11/13 — marker `DR_MQ_Fragment_7`
- **Black Knight's Fortress** — `DR_Main_Objective_4_12`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 12/13 — marker `DR_MQ_Fragment_7`

### S44

Map coordinate: `0.51465619, 0.08953525` · 3 distinct objective states

- **Even More Restless Ghosts** — `FindGhosts0/3`: Find ghosts and complete their unfinished business. (0/3) — marker `Dust-covered Sketchbook`
- **Even More Restless Ghosts** — `FindGhosts1/3`: Find ghosts and complete their unfinished business. (1/3) — marker `Dust-covered Sketchbook`
- **Even More Restless Ghosts** — `FindGhosts2/3`: Find ghosts and complete their unfinished business. (2/3) — marker `Dust-covered Sketchbook`

### S45

Map coordinate: `0.52307481, 0.2416812` · 2 distinct objective states

- **What Remains is Written** — `FindBooks`: Find five of Abraxus’ missing books in eastern Fellhollow and return them to him. — marker `The Importance of Chaos`
- **What Remains is Written** — `FindBooksAndReturn`: Find five of Abraxus’ missing books in eastern Fellhollow and return them to him. — marker `The Importance of Chaos`

### S46

Map coordinate: `0.52709762, 0.36161905` · 2 distinct objective states

- **Wanted!** — `DR_SQ2_EntryJournal`: Bring the Journal to Vannaka — marker `Vannaka — Wanted! journal hand-in`
- **Wanted!** — `DR_SQ2_ReturnToVannaka`: Journal Pages Collected 3/3
Return to Vannaka — marker `Vannaka (Wanted! hand-in)`

### S47

Map coordinate: `0.5293431, 0.7113979` · 2 distinct objective states

- **Icthlarin's Little Helper** — `FindThreeKeys`: Travel to Moon Garou camps, find their elders, and earn their trust to gain their Mirror Lenses — marker `Dúshlánach — Manafem Elder`
- **Icthlarin's Little Helper** — `FindLastKey`: Complete tasks for the final camp and gain their Mirror Lens — marker `Dúshlánach — Manafem Elder`

### S48

Map coordinate: `0.52945476, 0.4240119` · 13 distinct objective states

- **Black Knight's Fortress** — `DR_Main_Objective_4`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 0/13 — marker `DR_MQ_Fragment_4`
- **Black Knight's Fortress** — `DR_Main_Objective_4_1`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 1/13 — marker `DR_MQ_Fragment_4`
- **Black Knight's Fortress** — `DR_Main_Objective_4_2`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 2/13 — marker `DR_MQ_Fragment_4`
- **Black Knight's Fortress** — `DR_Main_Objective_4_3`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 3/13 — marker `DR_MQ_Fragment_4`
- **Black Knight's Fortress** — `DR_Main_Objective_4_4`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 4/13 — marker `DR_MQ_Fragment_4`
- **Black Knight's Fortress** — `DR_Main_Objective_4_5`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 5/13 — marker `DR_MQ_Fragment_4`
- **Black Knight's Fortress** — `DR_Main_Objective_4_6`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 6/13 — marker `DR_MQ_Fragment_4`
- **Black Knight's Fortress** — `DR_Main_Objective_4_7`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 7/13 — marker `DR_MQ_Fragment_4`
- **Black Knight's Fortress** — `DR_Main_Objective_4_8`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 8/13 — marker `DR_MQ_Fragment_4`
- **Black Knight's Fortress** — `DR_Main_Objective_4_9`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 9/13 — marker `DR_MQ_Fragment_4`
- **Black Knight's Fortress** — `DR_Main_Objective_4_10`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 10/13 — marker `DR_MQ_Fragment_4`
- **Black Knight's Fortress** — `DR_Main_Objective_4_11`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 11/13 — marker `DR_MQ_Fragment_4`
- **Black Knight's Fortress** — `DR_Main_Objective_4_12`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 12/13 — marker `DR_MQ_Fragment_4`

### S49

Map coordinate: `0.5311059, 0.18667376` · 2 distinct objective states

- **What Remains is Written** — `FindBooks`: Find five of Abraxus’ missing books in eastern Fellhollow and return them to him. — marker `The Wrath of Baba Potterington`
- **What Remains is Written** — `FindBooksAndReturn`: Find five of Abraxus’ missing books in eastern Fellhollow and return them to him. — marker `The Wrath of Baba Potterington`

### S50

Map coordinate: `0.53705131, 0.40394001` · 3 distinct objective states

- **Wanted!** — `DR_SQ2_FindPages_0`: Journal Pages Collected 0/3 — marker `Summoner's Journal — Cathedral crypt`
- **Wanted!** — `DR_SQ2_FindPages_1`: Journal Pages Collected 1/3 — marker `Summoner's Journal — Cathedral crypt`
- **Wanted!** — `DR_SQ2_FindPages_2`: Journal Pages Collected 2/3 — marker `Summoner's Journal — Cathedral crypt`

### S51

Map coordinate: `0.53863281, 0.61067286` · 5 distinct objective states

- **Mapping the Sands I** — `Objective1`: Climb tall locations around the Alcarrid Camp and find their Moon Sprites — marker `Survey I Moon Sprite 3 — southwest height`
- **Mapping the Sands I** — `0/3`: Climb tall locations near Alcarrid Camp and find their Moon Sprites (0/3) — marker `Survey I Moon Sprite 3 — southwest height`
- **Mapping the Sands I** — `1/3`: Climb tall locations near Alcarrid Camp and find their Moon Sprites (1/3) — marker `Survey I Moon Sprite 3 — southwest height`
- **Mapping the Sands I** — `2/3`: Climb tall locations near Alcarrid Camp and find their Moon Sprites (2/3) — marker `Survey I Moon Sprite 3 — southwest height`
- **Mapping the Sands I** — `3/3`: Climb tall locations near Alcarrid Camp and find their Moon Sprites (3/3) — marker `Survey I Moon Sprite 3 — southwest height`

### S52

Map coordinate: `0.53901429, 0.38740476` · 13 distinct objective states

- **Black Knight's Fortress** — `DR_Main_Objective_4`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 0/13 — marker `DR_MQ_Fragment_8`
- **Black Knight's Fortress** — `DR_Main_Objective_4_1`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 1/13 — marker `DR_MQ_Fragment_8`
- **Black Knight's Fortress** — `DR_Main_Objective_4_2`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 2/13 — marker `DR_MQ_Fragment_8`
- **Black Knight's Fortress** — `DR_Main_Objective_4_3`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 3/13 — marker `DR_MQ_Fragment_8`
- **Black Knight's Fortress** — `DR_Main_Objective_4_4`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 4/13 — marker `DR_MQ_Fragment_8`
- **Black Knight's Fortress** — `DR_Main_Objective_4_5`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 5/13 — marker `DR_MQ_Fragment_8`
- **Black Knight's Fortress** — `DR_Main_Objective_4_6`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 6/13 — marker `DR_MQ_Fragment_8`
- **Black Knight's Fortress** — `DR_Main_Objective_4_7`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 7/13 — marker `DR_MQ_Fragment_8`
- **Black Knight's Fortress** — `DR_Main_Objective_4_8`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 8/13 — marker `DR_MQ_Fragment_8`
- **Black Knight's Fortress** — `DR_Main_Objective_4_9`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 9/13 — marker `DR_MQ_Fragment_8`
- **Black Knight's Fortress** — `DR_Main_Objective_4_10`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 10/13 — marker `DR_MQ_Fragment_8`
- **Black Knight's Fortress** — `DR_Main_Objective_4_11`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 11/13 — marker `DR_MQ_Fragment_8`
- **Black Knight's Fortress** — `DR_Main_Objective_4_12`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 12/13 — marker `DR_MQ_Fragment_8`

### S53

Map coordinate: `0.5443506, 0.12216518` · 2 distinct objective states

- **What Remains is Written** — `FindScrolls`: Defeat Abraxus' enemies to retrieve the scrolls. — marker `Coclear Malificarum`
- **What Remains is Written** — `FindScrollsAndReturn`: Defeat Abraxus' enemies to retrieve the scrolls. Then return them to Abraxus. — marker `Coclear Malificarum`

### S54

Map coordinate: `0.55027143, 0.37528333` · 13 distinct objective states

- **Black Knight's Fortress** — `DR_Main_Objective_4`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 0/13 — marker `DR_MQ_Fragment_5`
- **Black Knight's Fortress** — `DR_Main_Objective_4_1`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 1/13 — marker `DR_MQ_Fragment_5`
- **Black Knight's Fortress** — `DR_Main_Objective_4_2`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 2/13 — marker `DR_MQ_Fragment_5`
- **Black Knight's Fortress** — `DR_Main_Objective_4_3`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 3/13 — marker `DR_MQ_Fragment_5`
- **Black Knight's Fortress** — `DR_Main_Objective_4_4`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 4/13 — marker `DR_MQ_Fragment_5`
- **Black Knight's Fortress** — `DR_Main_Objective_4_5`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 5/13 — marker `DR_MQ_Fragment_5`
- **Black Knight's Fortress** — `DR_Main_Objective_4_6`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 6/13 — marker `DR_MQ_Fragment_5`
- **Black Knight's Fortress** — `DR_Main_Objective_4_7`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 7/13 — marker `DR_MQ_Fragment_5`
- **Black Knight's Fortress** — `DR_Main_Objective_4_8`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 8/13 — marker `DR_MQ_Fragment_5`
- **Black Knight's Fortress** — `DR_Main_Objective_4_9`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 9/13 — marker `DR_MQ_Fragment_5`
- **Black Knight's Fortress** — `DR_Main_Objective_4_10`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 10/13 — marker `DR_MQ_Fragment_5`
- **Black Knight's Fortress** — `DR_Main_Objective_4_11`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 11/13 — marker `DR_MQ_Fragment_5`
- **Black Knight's Fortress** — `DR_Main_Objective_4_12`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 12/13 — marker `DR_MQ_Fragment_5`

### S55

Map coordinate: `0.55128119, 0.34590352` · 4 distinct objective states

- **Biohazard** — `DR_SQ3_EntryPickup`: Speak to Zanik about the Corrupted Essence you got from a strange raven. — marker `Zanik — cell beneath The Approach`
- **Biohazard** — `DR_SQ3_HuntRavens`: Gather 10 biological samples from strange ravens in Dowdun Reach, and bring them to Zanik. — marker `Zanik (sample hand-in)`
- **Biohazard** — `DR_SQ3_FindIngredients`: Bring Zanik the ingredients. Dragons Blood. A Twilight Lily and some Unholy Water. — marker `Zanik — ingredient hand-in`
- **Biohazard** — `DR_SQ3_FinalBeat`: Return to Zanik and explain the situation. — marker `Zanik — final report`

### S56

Map coordinate: `0.55772786, 0.24067574` · 3 distinct objective states

- **Withering Heights** — `BreakTheThreeSeals`: Gather three curse fragments from the locations described in the standing stones. — marker `Curse Fragment of Despair — Silverthorn Keep entrance`
- **Withering Heights** — `BreakRemainingtwoSeals`: Gather two more curse fragments from the locations described in the standing stones. — marker `Curse Fragment of Despair — Silverthorn Keep entrance`
- **Withering Heights** — `BreakTheFinalSeal`: Find the last curse fragment from the locations described in the standing stones. — marker `Curse Fragment of Despair — Silverthorn Keep entrance`

### S57

Map coordinate: `0.55930888, 0.16139381` · 2 distinct objective states

- **What Remains is Written** — `FindBooks`: Find five of Abraxus’ missing books in eastern Fellhollow and return them to him. — marker `Aetheric Fundamentals, a Primordial Primer`
- **What Remains is Written** — `FindBooksAndReturn`: Find five of Abraxus’ missing books in eastern Fellhollow and return them to him. — marker `Aetheric Fundamentals, a Primordial Primer`

### S58

Map coordinate: `0.56269762, 0.36642857` · 13 distinct objective states

- **Black Knight's Fortress** — `DR_Main_Objective_4`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 0/13 — marker `DR_MQ_Fragment_6`
- **Black Knight's Fortress** — `DR_Main_Objective_4_1`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 1/13 — marker `DR_MQ_Fragment_6`
- **Black Knight's Fortress** — `DR_Main_Objective_4_2`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 2/13 — marker `DR_MQ_Fragment_6`
- **Black Knight's Fortress** — `DR_Main_Objective_4_3`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 3/13 — marker `DR_MQ_Fragment_6`
- **Black Knight's Fortress** — `DR_Main_Objective_4_4`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 4/13 — marker `DR_MQ_Fragment_6`
- **Black Knight's Fortress** — `DR_Main_Objective_4_5`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 5/13 — marker `DR_MQ_Fragment_6`
- **Black Knight's Fortress** — `DR_Main_Objective_4_6`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 6/13 — marker `DR_MQ_Fragment_6`
- **Black Knight's Fortress** — `DR_Main_Objective_4_7`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 7/13 — marker `DR_MQ_Fragment_6`
- **Black Knight's Fortress** — `DR_Main_Objective_4_8`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 8/13 — marker `DR_MQ_Fragment_6`
- **Black Knight's Fortress** — `DR_Main_Objective_4_9`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 9/13 — marker `DR_MQ_Fragment_6`
- **Black Knight's Fortress** — `DR_Main_Objective_4_10`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 10/13 — marker `DR_MQ_Fragment_6`
- **Black Knight's Fortress** — `DR_Main_Objective_4_11`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 11/13 — marker `DR_MQ_Fragment_6`
- **Black Knight's Fortress** — `DR_Main_Objective_4_12`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 12/13 — marker `DR_MQ_Fragment_6`

### S59

Map coordinate: `0.56451488, 0.2759552` · 2 distinct objective states

- **What Remains is Written** — `FindScrolls`: Defeat Abraxus' enemies to retrieve the scrolls. — marker `Eventide - A New Spawn`
- **What Remains is Written** — `FindScrollsAndReturn`: Defeat Abraxus' enemies to retrieve the scrolls. Then return them to Abraxus. — marker `Eventide - A New Spawn`

### S60

Map coordinate: `0.56936667, 0.41005238` · 2 distinct objective states

- **Black Knight's Fortress** — `DR_Main_Objective_2`: Speak to Doric in Dowdun Reach — marker `Doric — The Nexus`
- **Black Knight's Fortress** — `DR_Main_Objective_4_-1`: Speak to Doric about the shards around Dowdun Reach — marker `Doric — shard briefing at The Nexus`

### S61

Map coordinate: `0.5703, 0.40994048` · 3 distinct objective states

- **Black Knight's Fortress** — `DR_Main_Objective_3`: Investigate the Dreaming Stone — marker `Dreaming Stone`
- **Black Knight's Fortress** — `DR_Main_Objective_6`: Return to the Dreaming Stone — marker `Avisk — restored Dreaming Stone`
- **Black Knight's Fortress** — `DR_Main_Objective_4_13`: Return to the Dreaming Stone — marker `Dreaming Stone`

### S62

Map coordinate: `0.57380952, 0.34228571` · 3 distinct objective states

- **Wanted!** — `DR_SQ2_FindPages_0`: Journal Pages Collected 0/3 — marker `Hound Master's Journal — Garrison kennels`
- **Wanted!** — `DR_SQ2_FindPages_1`: Journal Pages Collected 1/3 — marker `Hound Master's Journal — Garrison kennels`
- **Wanted!** — `DR_SQ2_FindPages_2`: Journal Pages Collected 2/3 — marker `Hound Master's Journal — Garrison kennels`

### S63

Map coordinate: `0.5760119, 0.33353571` · 13 distinct objective states

- **Black Knight's Fortress** — `DR_Main_Objective_4`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 0/13 — marker `DR_MQ_Fragment_13`
- **Black Knight's Fortress** — `DR_Main_Objective_4_1`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 1/13 — marker `DR_MQ_Fragment_13`
- **Black Knight's Fortress** — `DR_Main_Objective_4_2`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 2/13 — marker `DR_MQ_Fragment_13`
- **Black Knight's Fortress** — `DR_Main_Objective_4_3`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 3/13 — marker `DR_MQ_Fragment_13`
- **Black Knight's Fortress** — `DR_Main_Objective_4_4`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 4/13 — marker `DR_MQ_Fragment_13`
- **Black Knight's Fortress** — `DR_Main_Objective_4_5`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 5/13 — marker `DR_MQ_Fragment_13`
- **Black Knight's Fortress** — `DR_Main_Objective_4_6`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 6/13 — marker `DR_MQ_Fragment_13`
- **Black Knight's Fortress** — `DR_Main_Objective_4_7`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 7/13 — marker `DR_MQ_Fragment_13`
- **Black Knight's Fortress** — `DR_Main_Objective_4_8`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 8/13 — marker `DR_MQ_Fragment_13`
- **Black Knight's Fortress** — `DR_Main_Objective_4_9`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 9/13 — marker `DR_MQ_Fragment_13`
- **Black Knight's Fortress** — `DR_Main_Objective_4_10`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 10/13 — marker `DR_MQ_Fragment_13`
- **Black Knight's Fortress** — `DR_Main_Objective_4_11`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 11/13 — marker `DR_MQ_Fragment_13`
- **Black Knight's Fortress** — `DR_Main_Objective_4_12`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 12/13 — marker `DR_MQ_Fragment_13`

### S64

Map coordinate: `0.58781593, 0.2348789` · 2 distinct objective states

- **What Remains is Written** — `FindScrolls`: Defeat Abraxus' enemies to retrieve the scrolls. — marker `The Binding of Ib`
- **What Remains is Written** — `FindScrollsAndReturn`: Defeat Abraxus' enemies to retrieve the scrolls. Then return them to Abraxus. — marker `The Binding of Ib`

### S65

Map coordinate: `0.59098333, 0.33170476` · 13 distinct objective states

- **Black Knight's Fortress** — `DR_Main_Objective_4`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 0/13 — marker `DR_MQ_Fragment_2`
- **Black Knight's Fortress** — `DR_Main_Objective_4_1`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 1/13 — marker `DR_MQ_Fragment_2`
- **Black Knight's Fortress** — `DR_Main_Objective_4_2`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 2/13 — marker `DR_MQ_Fragment_2`
- **Black Knight's Fortress** — `DR_Main_Objective_4_3`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 3/13 — marker `DR_MQ_Fragment_2`
- **Black Knight's Fortress** — `DR_Main_Objective_4_4`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 4/13 — marker `DR_MQ_Fragment_2`
- **Black Knight's Fortress** — `DR_Main_Objective_4_5`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 5/13 — marker `DR_MQ_Fragment_2`
- **Black Knight's Fortress** — `DR_Main_Objective_4_6`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 6/13 — marker `DR_MQ_Fragment_2`
- **Black Knight's Fortress** — `DR_Main_Objective_4_7`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 7/13 — marker `DR_MQ_Fragment_2`
- **Black Knight's Fortress** — `DR_Main_Objective_4_8`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 8/13 — marker `DR_MQ_Fragment_2`
- **Black Knight's Fortress** — `DR_Main_Objective_4_9`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 9/13 — marker `DR_MQ_Fragment_2`
- **Black Knight's Fortress** — `DR_Main_Objective_4_10`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 10/13 — marker `DR_MQ_Fragment_2`
- **Black Knight's Fortress** — `DR_Main_Objective_4_11`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 11/13 — marker `DR_MQ_Fragment_2`
- **Black Knight's Fortress** — `DR_Main_Objective_4_12`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 12/13 — marker `DR_MQ_Fragment_2`

### S66

Map coordinate: `0.60055826, 0.20257285` · 2 distinct objective states

- **What Remains is Written** — `FindBooks`: Find five of Abraxus’ missing books in eastern Fellhollow and return them to him. — marker `The Pride of the Avernic`
- **What Remains is Written** — `FindBooksAndReturn`: Find five of Abraxus’ missing books in eastern Fellhollow and return them to him. — marker `The Pride of the Avernic`

### S67

Map coordinate: `0.60923162, 0.55673445` · 4 distinct objective states

- **Icthlarin's Little Helper** — `FirstGarouEarnReputation.NoReputation`: Gain the trust of the Alcarrid Camp — marker `Valas — The Mirror Crack'd`
- **Icthlarin's Little Helper** — `FirstGarouEarnReputation.LowReputation`: Gain the trust of the Alcarrid Camp — marker `Valas — The Mirror Crack'd`
- **Icthlarin's Little Helper** — `FirstGarouEarnReputation.MediumReputation`: Gain the trust of the Alcarrid Camp — marker `Valas — The Mirror Crack'd`
- **Icthlarin's Little Helper** — `FirstGarouEarnReputation.HighReputation`: Gain the trust of the Alcarrid Camp — marker `Valas — The Mirror Crack'd`

### S68

Map coordinate: `0.61753333, 0.54792393` · 4 distinct objective states

- **Icthlarin's Little Helper** — `FirstGarouEarnReputation.NoReputation`: Gain the trust of the Alcarrid Camp — marker `Domri — Rogue Trader`
- **Icthlarin's Little Helper** — `FirstGarouEarnReputation.LowReputation`: Gain the trust of the Alcarrid Camp — marker `Domri — Rogue Trader`
- **Icthlarin's Little Helper** — `FirstGarouEarnReputation.MediumReputation`: Gain the trust of the Alcarrid Camp — marker `Domri — Rogue Trader`
- **Icthlarin's Little Helper** — `FirstGarouEarnReputation.HighReputation`: Gain the trust of the Alcarrid Camp — marker `Domri — Rogue Trader`

### S69

Map coordinate: `0.62325, 0.40284524` · 13 distinct objective states

- **Black Knight's Fortress** — `DR_Main_Objective_4`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 0/13 — marker `DR_MQ_Fragment_9`
- **Black Knight's Fortress** — `DR_Main_Objective_4_1`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 1/13 — marker `DR_MQ_Fragment_9`
- **Black Knight's Fortress** — `DR_Main_Objective_4_2`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 2/13 — marker `DR_MQ_Fragment_9`
- **Black Knight's Fortress** — `DR_Main_Objective_4_3`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 3/13 — marker `DR_MQ_Fragment_9`
- **Black Knight's Fortress** — `DR_Main_Objective_4_4`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 4/13 — marker `DR_MQ_Fragment_9`
- **Black Knight's Fortress** — `DR_Main_Objective_4_5`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 5/13 — marker `DR_MQ_Fragment_9`
- **Black Knight's Fortress** — `DR_Main_Objective_4_6`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 6/13 — marker `DR_MQ_Fragment_9`
- **Black Knight's Fortress** — `DR_Main_Objective_4_7`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 7/13 — marker `DR_MQ_Fragment_9`
- **Black Knight's Fortress** — `DR_Main_Objective_4_8`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 8/13 — marker `DR_MQ_Fragment_9`
- **Black Knight's Fortress** — `DR_Main_Objective_4_9`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 9/13 — marker `DR_MQ_Fragment_9`
- **Black Knight's Fortress** — `DR_Main_Objective_4_10`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 10/13 — marker `DR_MQ_Fragment_9`
- **Black Knight's Fortress** — `DR_Main_Objective_4_11`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 11/13 — marker `DR_MQ_Fragment_9`
- **Black Knight's Fortress** — `DR_Main_Objective_4_12`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 12/13 — marker `DR_MQ_Fragment_9`

### S70

Map coordinate: `0.6237154, 0.32018209` · 13 distinct objective states

- **Black Knight's Fortress** — `DR_Main_Objective_4`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 0/13 — marker `DR_MQ_Fragment_1`
- **Black Knight's Fortress** — `DR_Main_Objective_4_1`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 1/13 — marker `DR_MQ_Fragment_1`
- **Black Knight's Fortress** — `DR_Main_Objective_4_2`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 2/13 — marker `DR_MQ_Fragment_1`
- **Black Knight's Fortress** — `DR_Main_Objective_4_3`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 3/13 — marker `DR_MQ_Fragment_1`
- **Black Knight's Fortress** — `DR_Main_Objective_4_4`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 4/13 — marker `DR_MQ_Fragment_1`
- **Black Knight's Fortress** — `DR_Main_Objective_4_5`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 5/13 — marker `DR_MQ_Fragment_1`
- **Black Knight's Fortress** — `DR_Main_Objective_4_6`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 6/13 — marker `DR_MQ_Fragment_1`
- **Black Knight's Fortress** — `DR_Main_Objective_4_7`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 7/13 — marker `DR_MQ_Fragment_1`
- **Black Knight's Fortress** — `DR_Main_Objective_4_8`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 8/13 — marker `DR_MQ_Fragment_1`
- **Black Knight's Fortress** — `DR_Main_Objective_4_9`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 9/13 — marker `DR_MQ_Fragment_1`
- **Black Knight's Fortress** — `DR_Main_Objective_4_10`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 10/13 — marker `DR_MQ_Fragment_1`
- **Black Knight's Fortress** — `DR_Main_Objective_4_11`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 11/13 — marker `DR_MQ_Fragment_1`
- **Black Knight's Fortress** — `DR_Main_Objective_4_12`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 12/13 — marker `DR_MQ_Fragment_1`

### S71

Map coordinate: `0.62376876, 0.55132352` · 2 distinct objective states

- **Icthlarin's Little Helper** — `InvestigateGarouProblem`: Investigate the nearby area for something that concerns the Moon Garou — marker `Alcarrid camp investigation area`
- **Icthlarin's Little Helper** — `LookForLife`: Look for signs of life in the desert — marker `Signs of life — Alcarrid camp search area`

### S72

Map coordinate: `0.62457957, 0.77285014` · 9 distinct objective states

- **Mapping the Sands II** — `Objective1`: Climb tall locations around Umbral Sands and find their Moon Sprites — marker `Survey II Moon Sprite 2`
- **Mapping the Sands II** — `0/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (0/8) — marker `Survey II Moon Sprite 2`
- **Mapping the Sands II** — `1/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (1/8) — marker `Survey II Moon Sprite 2`
- **Mapping the Sands II** — `2/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (2/8) — marker `Survey II Moon Sprite 2`
- **Mapping the Sands II** — `3/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (3/8) — marker `Survey II Moon Sprite 2`
- **Mapping the Sands II** — `4/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (4/8) — marker `Survey II Moon Sprite 2`
- **Mapping the Sands II** — `5/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (5/8) — marker `Survey II Moon Sprite 2`
- **Mapping the Sands II** — `6/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (6/8) — marker `Survey II Moon Sprite 2`
- **Mapping the Sands II** — `7/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (7/8) — marker `Survey II Moon Sprite 2`

### S73

Map coordinate: `0.6262666, 0.55770167` · 4 distinct objective states

- **Icthlarin's Little Helper** — `FirstGarouEarnReputation.NoReputation`: Gain the trust of the Alcarrid Camp — marker `Lagra — Rag and Bone Garou`
- **Icthlarin's Little Helper** — `FirstGarouEarnReputation.LowReputation`: Gain the trust of the Alcarrid Camp — marker `Lagra — Rag and Bone Garou`
- **Icthlarin's Little Helper** — `FirstGarouEarnReputation.MediumReputation`: Gain the trust of the Alcarrid Camp — marker `Lagra — Rag and Bone Garou`
- **Icthlarin's Little Helper** — `FirstGarouEarnReputation.HighReputation`: Gain the trust of the Alcarrid Camp — marker `Lagra — Rag and Bone Garou`

### S74

Map coordinate: `0.62650555, 0.54707266` · 2 distinct objective states

- **Icthlarin's Little Helper** — `ReturnToIcthlarin`: Bring Fuzan's head to Icthlarin at the Mirror Relay — marker `Icthlarin — Mirror Relay`
- **Icthlarin's Little Helper** — `SpeakToIcthlarin`: Speak to Icthlarin at the Mirror Relay — marker `Icthlarin — Mirror Relay`

### S75

Map coordinate: `0.62743857, 0.54721181` · 4 distinct objective states

- **Icthlarin's Little Helper** — `FindGarou`: Speak to the Alcarrid Elder in the Moon Garou settlement to the south — marker `Ancamairon — Alcarrid Elder`
- **Icthlarin's Little Helper** — `FindThreeKeys`: Travel to Moon Garou camps, find their elders, and earn their trust to gain their Mirror Lenses — marker `Ancamairon — Alcarrid Elder`
- **Icthlarin's Little Helper** — `FirstGarouEarnReputation.MaxReputation`: You have completed the tribe's tasks. Return to the Alcarrid Elder — marker `Ancamairon — Alcarrid Elder`
- **Icthlarin's Little Helper** — `SpeakToGarouElder`: Speak to the Alcarrid Elder and complete tasks to earn their trust — marker `Ancamairon — Alcarrid Elder`

### S76

Map coordinate: `0.64234155, 0.54190862` · 2 distinct objective states

- **Mapping the Sands I** — `Objective2`: Return to the Moon Sprite Shaman in the Alcarrid Camp — marker `Anamcara — Alcarrid Camp`
- **Mapping the Sands II** — `Objective2`: Return to the Moon Sprite Shaman in the Alcarrid Camp — marker `Anamcara — Alcarrid Camp`

### S77

Map coordinate: `0.6469119, 0.36865952` · 13 distinct objective states

- **Black Knight's Fortress** — `DR_Main_Objective_4`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 0/13 — marker `DR_MQ_Fragment_10`
- **Black Knight's Fortress** — `DR_Main_Objective_4_1`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 1/13 — marker `DR_MQ_Fragment_10`
- **Black Knight's Fortress** — `DR_Main_Objective_4_2`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 2/13 — marker `DR_MQ_Fragment_10`
- **Black Knight's Fortress** — `DR_Main_Objective_4_3`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 3/13 — marker `DR_MQ_Fragment_10`
- **Black Knight's Fortress** — `DR_Main_Objective_4_4`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 4/13 — marker `DR_MQ_Fragment_10`
- **Black Knight's Fortress** — `DR_Main_Objective_4_5`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 5/13 — marker `DR_MQ_Fragment_10`
- **Black Knight's Fortress** — `DR_Main_Objective_4_6`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 6/13 — marker `DR_MQ_Fragment_10`
- **Black Knight's Fortress** — `DR_Main_Objective_4_7`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 7/13 — marker `DR_MQ_Fragment_10`
- **Black Knight's Fortress** — `DR_Main_Objective_4_8`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 8/13 — marker `DR_MQ_Fragment_10`
- **Black Knight's Fortress** — `DR_Main_Objective_4_9`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 9/13 — marker `DR_MQ_Fragment_10`
- **Black Knight's Fortress** — `DR_Main_Objective_4_10`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 10/13 — marker `DR_MQ_Fragment_10`
- **Black Knight's Fortress** — `DR_Main_Objective_4_11`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 11/13 — marker `DR_MQ_Fragment_10`
- **Black Knight's Fortress** — `DR_Main_Objective_4_12`: Collect the Shards of the Dreaming Stone around Dowdun Reach: 12/13 — marker `DR_MQ_Fragment_10`

### S78

Map coordinate: `0.66969538, 0.54106729` · 5 distinct objective states

- **Mapping the Sands I** — `Objective1`: Climb tall locations around the Alcarrid Camp and find their Moon Sprites — marker `Survey I Moon Sprite 1 — east of Alcarrid Camp`
- **Mapping the Sands I** — `0/3`: Climb tall locations near Alcarrid Camp and find their Moon Sprites (0/3) — marker `Survey I Moon Sprite 1 — east of Alcarrid Camp`
- **Mapping the Sands I** — `1/3`: Climb tall locations near Alcarrid Camp and find their Moon Sprites (1/3) — marker `Survey I Moon Sprite 1 — east of Alcarrid Camp`
- **Mapping the Sands I** — `2/3`: Climb tall locations near Alcarrid Camp and find their Moon Sprites (2/3) — marker `Survey I Moon Sprite 1 — east of Alcarrid Camp`
- **Mapping the Sands I** — `3/3`: Climb tall locations near Alcarrid Camp and find their Moon Sprites (3/3) — marker `Survey I Moon Sprite 1 — east of Alcarrid Camp`

### S79

Map coordinate: `0.68487595, 0.72604536` · 2 distinct objective states

- **Brink of Extinction** — `Objective4`: Use the password to enter the Cave of Screams, within the Burning Spire region — marker `KotHaar Bouncer — Cave of Screams entrance`
- **Brink of Extinction** — `Objective6`: Return to the KotHaar Bouncer outside of the Cave of Screams — marker `KotHaar Bouncer — final return`

### S80

Map coordinate: `0.71284524, 0.72415476` · 4 distinct objective states

- **Brink of Extinction** — `Objective2`: Touch 4 pedestals in any order in THE GAUNTLET OF FIRE - 0/4 — marker `Fire Pedestal — west of Sinking Pyramid`
- **Brink of Extinction** — `Objective2.1`: Touch 4 pedestals in any order in THE GAUNTLET OF FIRE - 1/4 — marker `Fire Pedestal — west of Sinking Pyramid`
- **Brink of Extinction** — `Objective2.2`: Touch 4 pedestals in any order in THE GAUNTLET OF FIRE - 2/4 — marker `Fire Pedestal — west of Sinking Pyramid`
- **Brink of Extinction** — `Objective2.3`: Touch 4 pedestals in any order in THE GAUNTLET OF FIRE - 3/4 — marker `Fire Pedestal — west of Sinking Pyramid`

### S81

Map coordinate: `0.73286376, 0.66181952` · 5 distinct objective states

- **Mapping the Sands I** — `Objective1`: Climb tall locations around the Alcarrid Camp and find their Moon Sprites — marker `Survey I Moon Sprite 2 — southeast height`
- **Mapping the Sands I** — `0/3`: Climb tall locations near Alcarrid Camp and find their Moon Sprites (0/3) — marker `Survey I Moon Sprite 2 — southeast height`
- **Mapping the Sands I** — `1/3`: Climb tall locations near Alcarrid Camp and find their Moon Sprites (1/3) — marker `Survey I Moon Sprite 2 — southeast height`
- **Mapping the Sands I** — `2/3`: Climb tall locations near Alcarrid Camp and find their Moon Sprites (2/3) — marker `Survey I Moon Sprite 2 — southeast height`
- **Mapping the Sands I** — `3/3`: Climb tall locations near Alcarrid Camp and find their Moon Sprites (3/3) — marker `Survey I Moon Sprite 2 — southeast height`

### S82

Map coordinate: `0.73562805, 0.72213433` · 9 distinct objective states

- **Mapping the Sands II** — `Objective1`: Climb tall locations around Umbral Sands and find their Moon Sprites — marker `Survey II Moon Sprite 1`
- **Mapping the Sands II** — `0/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (0/8) — marker `Survey II Moon Sprite 1`
- **Mapping the Sands II** — `1/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (1/8) — marker `Survey II Moon Sprite 1`
- **Mapping the Sands II** — `2/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (2/8) — marker `Survey II Moon Sprite 1`
- **Mapping the Sands II** — `3/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (3/8) — marker `Survey II Moon Sprite 1`
- **Mapping the Sands II** — `4/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (4/8) — marker `Survey II Moon Sprite 1`
- **Mapping the Sands II** — `5/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (5/8) — marker `Survey II Moon Sprite 1`
- **Mapping the Sands II** — `6/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (6/8) — marker `Survey II Moon Sprite 1`
- **Mapping the Sands II** — `7/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (7/8) — marker `Survey II Moon Sprite 1`

### S83

Map coordinate: `0.73979762, 0.70717857` · 4 distinct objective states

- **Brink of Extinction** — `Objective2`: Touch 4 pedestals in any order in THE GAUNTLET OF FIRE - 0/4 — marker `Fire Pedestal — north of Sinking Pyramid`
- **Brink of Extinction** — `Objective2.1`: Touch 4 pedestals in any order in THE GAUNTLET OF FIRE - 1/4 — marker `Fire Pedestal — north of Sinking Pyramid`
- **Brink of Extinction** — `Objective2.2`: Touch 4 pedestals in any order in THE GAUNTLET OF FIRE - 2/4 — marker `Fire Pedestal — north of Sinking Pyramid`
- **Brink of Extinction** — `Objective2.3`: Touch 4 pedestals in any order in THE GAUNTLET OF FIRE - 3/4 — marker `Fire Pedestal — north of Sinking Pyramid`

### S84

Map coordinate: `0.74103824, 0.75162964` · 4 distinct objective states

- **Brink of Extinction** — `Objective2`: Touch 4 pedestals in any order in THE GAUNTLET OF FIRE - 0/4 — marker `Fire Pedestal — south of Sinking Pyramid`
- **Brink of Extinction** — `Objective2.1`: Touch 4 pedestals in any order in THE GAUNTLET OF FIRE - 1/4 — marker `Fire Pedestal — south of Sinking Pyramid`
- **Brink of Extinction** — `Objective2.2`: Touch 4 pedestals in any order in THE GAUNTLET OF FIRE - 2/4 — marker `Fire Pedestal — south of Sinking Pyramid`
- **Brink of Extinction** — `Objective2.3`: Touch 4 pedestals in any order in THE GAUNTLET OF FIRE - 3/4 — marker `Fire Pedestal — south of Sinking Pyramid`

### S85

Map coordinate: `0.74848571, 0.43437237` · 9 distinct objective states

- **Mapping the Sands II** — `Objective1`: Climb tall locations around Umbral Sands and find their Moon Sprites — marker `Survey II Moon Sprite 7`
- **Mapping the Sands II** — `0/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (0/8) — marker `Survey II Moon Sprite 7`
- **Mapping the Sands II** — `1/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (1/8) — marker `Survey II Moon Sprite 7`
- **Mapping the Sands II** — `2/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (2/8) — marker `Survey II Moon Sprite 7`
- **Mapping the Sands II** — `3/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (3/8) — marker `Survey II Moon Sprite 7`
- **Mapping the Sands II** — `4/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (4/8) — marker `Survey II Moon Sprite 7`
- **Mapping the Sands II** — `5/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (5/8) — marker `Survey II Moon Sprite 7`
- **Mapping the Sands II** — `6/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (6/8) — marker `Survey II Moon Sprite 7`
- **Mapping the Sands II** — `7/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (7/8) — marker `Survey II Moon Sprite 7`

### S86

Map coordinate: `0.74908162, 0.70706245` · 9 distinct objective states

- **Mapping the Sands II** — `Objective1`: Climb tall locations around Umbral Sands and find their Moon Sprites — marker `Survey II Moon Sprite 4`
- **Mapping the Sands II** — `0/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (0/8) — marker `Survey II Moon Sprite 4`
- **Mapping the Sands II** — `1/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (1/8) — marker `Survey II Moon Sprite 4`
- **Mapping the Sands II** — `2/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (2/8) — marker `Survey II Moon Sprite 4`
- **Mapping the Sands II** — `3/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (3/8) — marker `Survey II Moon Sprite 4`
- **Mapping the Sands II** — `4/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (4/8) — marker `Survey II Moon Sprite 4`
- **Mapping the Sands II** — `5/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (5/8) — marker `Survey II Moon Sprite 4`
- **Mapping the Sands II** — `6/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (6/8) — marker `Survey II Moon Sprite 4`
- **Mapping the Sands II** — `7/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (7/8) — marker `Survey II Moon Sprite 4`

### S87

Map coordinate: `0.75844048, 0.73013095` · 4 distinct objective states

- **Brink of Extinction** — `Objective2`: Touch 4 pedestals in any order in THE GAUNTLET OF FIRE - 0/4 — marker `Fire Pedestal — east rock formation`
- **Brink of Extinction** — `Objective2.1`: Touch 4 pedestals in any order in THE GAUNTLET OF FIRE - 1/4 — marker `Fire Pedestal — east rock formation`
- **Brink of Extinction** — `Objective2.2`: Touch 4 pedestals in any order in THE GAUNTLET OF FIRE - 2/4 — marker `Fire Pedestal — east rock formation`
- **Brink of Extinction** — `Objective2.3`: Touch 4 pedestals in any order in THE GAUNTLET OF FIRE - 3/4 — marker `Fire Pedestal — east rock formation`

### S88

Map coordinate: `0.77151976, 0.38517525` · 3 distinct objective states

- **Contact!** — `Objective2`: Find 3 missing vault guardians and recover their Crystal Helix fragments — marker `Buried Vault Guardian — ruined building`
- **Contact!** — `CrystalShard.1`: Find Crystal Helix fragments. 1/3 — marker `Buried Vault Guardian — ruined building`
- **Contact!** — `CrystalShard.2`: Find Crystal Helix fragments. 2/3 — marker `Buried Vault Guardian — ruined building`

### S89

Map coordinate: `0.77502262, 0.34840311` · 9 distinct objective states

- **Contact!** — `Objective1`: Find the Vault Guardian Prime in the Kalphite Hive — marker `Vault Guardian Prime — Kalphite Hive entrance`
- **Contact!** — `Objective2`: Find 3 missing vault guardians and recover their Crystal Helix fragments — marker `Damaged guardian — Kalphite Hive entrance`
- **Contact!** — `Objective4`: Use the Crystal Helix to pass through the magical barrier in the Kalphite Hive — marker `Kalphite Hive entrance`
- **Contact!** — `Objective5`: Talk to Zanik in the Shortcut Chamber within the Kalphite Hive — marker `Zanik — Kalphite Hive entrance`
- **Contact!** — `Objective6`: Break the four crystals in the Shortcut Chamber of the Kalphite Hive — marker `Kalphite Hive entrance`
- **Contact!** — `Objective3`: Return to the Vault Guardian Prime — marker `Kalphite Hive entrance`
- **Contact!** — `Objective7`: Return to Zanik in the Shortcut Chamber within the Kalphite Hive — marker `Zanik — Kalphite Hive entrance`
- **Contact!** — `CrystalShard.1`: Find Crystal Helix fragments. 1/3 — marker `Damaged guardian — Kalphite Hive entrance`
- **Contact!** — `CrystalShard.2`: Find Crystal Helix fragments. 2/3 — marker `Damaged guardian — Kalphite Hive entrance`

### S90

Map coordinate: `0.78498719, 0.83871283` · 2 distinct objective states

- **Icthlarin's Little Helper** — `FightFuzan`: Enter the Fight Caves and defeat Fuzan — marker `Fight Caves entrance`
- **Icthlarin's Little Helper** — `SlayFuzan`: Journey through the Fight Caves and slay Fuzan — marker `Fight Caves entrance`

### S91

Map coordinate: `0.78657895, 0.73351493` · 9 distinct objective states

- **Mapping the Sands II** — `Objective1`: Climb tall locations around Umbral Sands and find their Moon Sprites — marker `Survey II Moon Sprite 9`
- **Mapping the Sands II** — `0/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (0/8) — marker `Survey II Moon Sprite 9`
- **Mapping the Sands II** — `1/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (1/8) — marker `Survey II Moon Sprite 9`
- **Mapping the Sands II** — `2/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (2/8) — marker `Survey II Moon Sprite 9`
- **Mapping the Sands II** — `3/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (3/8) — marker `Survey II Moon Sprite 9`
- **Mapping the Sands II** — `4/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (4/8) — marker `Survey II Moon Sprite 9`
- **Mapping the Sands II** — `5/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (5/8) — marker `Survey II Moon Sprite 9`
- **Mapping the Sands II** — `6/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (6/8) — marker `Survey II Moon Sprite 9`
- **Mapping the Sands II** — `7/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (7/8) — marker `Survey II Moon Sprite 9`

### S92

Map coordinate: `0.79303564, 0.77793467` · 9 distinct objective states

- **Mapping the Sands II** — `Objective1`: Climb tall locations around Umbral Sands and find their Moon Sprites — marker `Survey II Moon Sprite 3`
- **Mapping the Sands II** — `0/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (0/8) — marker `Survey II Moon Sprite 3`
- **Mapping the Sands II** — `1/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (1/8) — marker `Survey II Moon Sprite 3`
- **Mapping the Sands II** — `2/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (2/8) — marker `Survey II Moon Sprite 3`
- **Mapping the Sands II** — `3/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (3/8) — marker `Survey II Moon Sprite 3`
- **Mapping the Sands II** — `4/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (4/8) — marker `Survey II Moon Sprite 3`
- **Mapping the Sands II** — `5/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (5/8) — marker `Survey II Moon Sprite 3`
- **Mapping the Sands II** — `6/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (6/8) — marker `Survey II Moon Sprite 3`
- **Mapping the Sands II** — `7/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (7/8) — marker `Survey II Moon Sprite 3`

### S93

Map coordinate: `0.80232581, 0.35629501` · 3 distinct objective states

- **Contact!** — `Objective2`: Find 3 missing vault guardians and recover their Crystal Helix fragments — marker `Dead Vault Guardian — kalphite eggs`
- **Contact!** — `CrystalShard.1`: Find Crystal Helix fragments. 1/3 — marker `Dead Vault Guardian — kalphite eggs`
- **Contact!** — `CrystalShard.2`: Find Crystal Helix fragments. 2/3 — marker `Dead Vault Guardian — kalphite eggs`

### S94

Map coordinate: `0.80333795, 0.73349548` · 3 distinct objective states

- **Mark Day** — `ConvinceNoble.0`: Convince the Uzzer councillors to abandon the Mark Day Festival. 0/3 — marker `Brialka — Historian Councillor`
- **Mark Day** — `ConvinceNoble.1`: Convince the Uzzer councillors to abandon the Mark Day Festival. 1/3 — marker `Brialka — Historian Councillor`
- **Mark Day** — `ConvinceNoble.2`: Convince the Uzzer councillors to abandon the Mark Day Festival. 2/3 — marker `Brialka — Historian Councillor`

### S95

Map coordinate: `0.83684412, 0.45898287` · 3 distinct objective states

- **Mark Day** — `ConvinceNoble.0`: Convince the Uzzer councillors to abandon the Mark Day Festival. 0/3 — marker `Alabrais — Science Councillor`
- **Mark Day** — `ConvinceNoble.1`: Convince the Uzzer councillors to abandon the Mark Day Festival. 1/3 — marker `Alabrais — Science Councillor`
- **Mark Day** — `ConvinceNoble.2`: Convince the Uzzer councillors to abandon the Mark Day Festival. 2/3 — marker `Alabrais — Science Councillor`

### S96

Map coordinate: `0.85036793, 0.44673281` · 3 distinct objective states

- **Saga of the Adventurer** — `FindTheClues`: Use your Saga to find clues towards the treasure — marker `Rocky-spire stone slab search area`
- **Saga of the Adventurer** — `None`: Continue to find stone slabs. Your Saga has been updated! — marker `TribeB.Task1_Target_1`
- **Saga of the Adventurer** — `FindRemainingClues3/4`: Continue to find stone slabs. Your Saga has been updated! — marker `TribeB.Task1_Target_1`

### S97

Map coordinate: `0.86020945, 0.64445871` · 3 distinct objective states

- **Mark Day** — `ConvinceNoble.0`: Convince the Uzzer councillors to abandon the Mark Day Festival. 0/3 — marker `Chirot — Broker Councillor`
- **Mark Day** — `ConvinceNoble.1`: Convince the Uzzer councillors to abandon the Mark Day Festival. 1/3 — marker `Chirot — Broker Councillor`
- **Mark Day** — `ConvinceNoble.2`: Convince the Uzzer councillors to abandon the Mark Day Festival. 2/3 — marker `Chirot — Broker Councillor`

### S98

Map coordinate: `0.87553995, 0.56231179` · 2 distinct objective states

- **Icthlarin's Little Helper** — `FindThreeKeys`: Travel to Moon Garou camps, find their elders, and earn their trust to gain their Mirror Lenses — marker `Gaimbín — Uzzer Elder`
- **Icthlarin's Little Helper** — `FindLastKey`: Complete tasks for the final camp and gain their Mirror Lens — marker `Gaimbín — Uzzer Elder`

### S99

Map coordinate: `0.89182314, 0.70796986` · 4 distinct objective states

- **The Dig Site** — `DigScroll.0`: Dig up Forbidden Scrolls in the garou ruins. 0/4 — marker `Forbidden Scroll dig site — lower western ruin`
- **The Dig Site** — `DigScroll.1`: Dig up Forbidden Scrolls in the garou ruins. 1/4 — marker `Forbidden Scroll dig site — lower western ruin`
- **The Dig Site** — `DigScroll.2`: Dig up Forbidden Scrolls in the garou ruins. 2/4 — marker `Forbidden Scroll dig site — lower western ruin`
- **The Dig Site** — `DigScroll.3`: Dig up Forbidden Scrolls in the garou ruins. 3/4 — marker `Forbidden Scroll dig site — lower western ruin`

### S100

Map coordinate: `0.899914, 0.47171469` · 9 distinct objective states

- **Mapping the Sands II** — `Objective1`: Climb tall locations around Umbral Sands and find their Moon Sprites — marker `Survey II Moon Sprite 5`
- **Mapping the Sands II** — `0/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (0/8) — marker `Survey II Moon Sprite 5`
- **Mapping the Sands II** — `1/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (1/8) — marker `Survey II Moon Sprite 5`
- **Mapping the Sands II** — `2/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (2/8) — marker `Survey II Moon Sprite 5`
- **Mapping the Sands II** — `3/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (3/8) — marker `Survey II Moon Sprite 5`
- **Mapping the Sands II** — `4/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (4/8) — marker `Survey II Moon Sprite 5`
- **Mapping the Sands II** — `5/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (5/8) — marker `Survey II Moon Sprite 5`
- **Mapping the Sands II** — `6/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (6/8) — marker `Survey II Moon Sprite 5`
- **Mapping the Sands II** — `7/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (7/8) — marker `Survey II Moon Sprite 5`

### S101

Map coordinate: `0.90001971, 0.68942286` · 4 distinct objective states

- **The Dig Site** — `DigScroll.0`: Dig up Forbidden Scrolls in the garou ruins. 0/4 — marker `Forbidden Scroll dig site — southern ruin`
- **The Dig Site** — `DigScroll.1`: Dig up Forbidden Scrolls in the garou ruins. 1/4 — marker `Forbidden Scroll dig site — southern ruin`
- **The Dig Site** — `DigScroll.2`: Dig up Forbidden Scrolls in the garou ruins. 2/4 — marker `Forbidden Scroll dig site — southern ruin`
- **The Dig Site** — `DigScroll.3`: Dig up Forbidden Scrolls in the garou ruins. 3/4 — marker `Forbidden Scroll dig site — southern ruin`

### S102

Map coordinate: `0.90597829, 0.3785566` · 2 distinct objective states

- **Saga of the Adventurer** — `None`: Continue to find stone slabs. Your Saga has been updated! — marker `TribeB.Task1_Area_step2`
- **Saga of the Adventurer** — `LocateTheGateway`: Follow the final clue to the last stone slab. Your Saga has been updated!  — marker `Final slab — Crucible spire gateway`

### S103

Map coordinate: `0.91382405, 0.72215038` · 9 distinct objective states

- **Mapping the Sands II** — `Objective1`: Climb tall locations around Umbral Sands and find their Moon Sprites — marker `Survey II Moon Sprite 6`
- **Mapping the Sands II** — `0/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (0/8) — marker `Survey II Moon Sprite 6`
- **Mapping the Sands II** — `1/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (1/8) — marker `Survey II Moon Sprite 6`
- **Mapping the Sands II** — `2/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (2/8) — marker `Survey II Moon Sprite 6`
- **Mapping the Sands II** — `3/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (3/8) — marker `Survey II Moon Sprite 6`
- **Mapping the Sands II** — `4/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (4/8) — marker `Survey II Moon Sprite 6`
- **Mapping the Sands II** — `5/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (5/8) — marker `Survey II Moon Sprite 6`
- **Mapping the Sands II** — `6/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (6/8) — marker `Survey II Moon Sprite 6`
- **Mapping the Sands II** — `7/8`: Climb tall locations around Umbral Sands and find their Moon Sprites (7/8) — marker `Survey II Moon Sprite 6`

### S104

Map coordinate: `0.91585029, 0.72473936` · 4 distinct objective states

- **The Dig Site** — `DigScroll.0`: Dig up Forbidden Scrolls in the garou ruins. 0/4 — marker `Forbidden Scroll dig site — western ruin`
- **The Dig Site** — `DigScroll.1`: Dig up Forbidden Scrolls in the garou ruins. 1/4 — marker `Forbidden Scroll dig site — western ruin`
- **The Dig Site** — `DigScroll.2`: Dig up Forbidden Scrolls in the garou ruins. 2/4 — marker `Forbidden Scroll dig site — western ruin`
- **The Dig Site** — `DigScroll.3`: Dig up Forbidden Scrolls in the garou ruins. 3/4 — marker `Forbidden Scroll dig site — western ruin`

### S105

Map coordinate: `0.92301562, 0.69816271` · 4 distinct objective states

- **The Dig Site** — `DigScroll.0`: Dig up Forbidden Scrolls in the garou ruins. 0/4 — marker `Forbidden Scroll dig site — eastern ruin`
- **The Dig Site** — `DigScroll.1`: Dig up Forbidden Scrolls in the garou ruins. 1/4 — marker `Forbidden Scroll dig site — eastern ruin`
- **The Dig Site** — `DigScroll.2`: Dig up Forbidden Scrolls in the garou ruins. 2/4 — marker `Forbidden Scroll dig site — eastern ruin`
- **The Dig Site** — `DigScroll.3`: Dig up Forbidden Scrolls in the garou ruins. 3/4 — marker `Forbidden Scroll dig site — eastern ruin`
