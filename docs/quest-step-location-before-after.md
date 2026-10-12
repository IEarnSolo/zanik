# Quest step marker before/after audit

Generated after correcting bidirectional objective/location attribution and excluding `Revealed: false` location removals from destination candidates.

## 2026-10-10 What Remains is Written deep review

This review found that the earlier analyzer treated the generic `Abraxus The
Eternal` quest-location operation as the complete destination set. It also
resolved that name to two distinct actors. The corrected relationships use the
eight quest-specific interaction actors and the exact Coalridge Abraxus actor.

| Objective key | Before | After |
|---|---|---|
| `ObjectiveZero` | Coalridge Abraxus | Coalridge Abraxus (unchanged; exact actor identity retained) |
| `FindBooks` | Coalridge Abraxus; unrelated Dowdun Abraxus | Five authored book interaction points; Coalridge Abraxus return point |
| `FoundBooks` | Coalridge Abraxus; unrelated Dowdun Abraxus | Coalridge Abraxus only |
| `FindBooksAndReturn` | Coalridge Abraxus; unrelated Dowdun Abraxus | Five authored book interaction points; Coalridge Abraxus return point |
| `FindScrolls` | Coalridge Abraxus; unrelated Dowdun Abraxus | Binding of Ib, Coclear Malificarum, and Eventide authored pickup points |
| `FoundScrolls` | Coalridge Abraxus; unrelated Dowdun Abraxus | Coalridge Abraxus only |
| `FindScrollsAndReturn` | Coalridge Abraxus; unrelated Dowdun Abraxus | Three authored scroll pickup points; Coalridge Abraxus return point |

Net result: the incorrect Dowdun coordinate `0.50900629, 0.40049381` was
removed from six states, eight previously omitted action locations were added,
and combined states now distinguish collection markers from their return
marker. Exact current-build coordinates and evidence are listed in
`quest-step-location-audit.md`.

## 2026-10-10 The Great Body Robbery deep review

| Objective state | Before | After |
|---|---|---|
| `BodyParts.0`, `.1`, `.2` | Fenkenstrain quest marker plus three nearby Fenkenstrain stage representations | Human torso, Garou Arms, and Goblin Legs grave actors |
| `AddPartsToTable` | Fenkenstrain representations plus Examination Table | Examination Table only |
| `Objecrive3.5` | Fenkenstrain representations plus Examination Table | Examination Table only |
| `Objective7` | Fenkenstrain representations plus Examination Table | Examination Table only |
| `Objective8` | Fenkenstrain representations plus Strange Machine | Strange Machine only |
| `Objective5` | Fenkenstrain representations | Dynamic/no fixed marker; lightning strike location is not fixed |

This removed 24 misleading marker uses, added nine grave marker uses across the
three counter states, and replaced one false fixed destination with an explicit
dynamic-location state.

## 2026-10-10 Restless Ghosts deep review

| Objective state | Before | After |
|---|---|---|
| `Found ghost amulet` | Crumbling Well plus all four anonymous Restless Spirit placements | Warren only, named and marked as a talk interaction |
| `Put ghosts to rest` | Four anonymous Restless Spirit placements | Warren, Lazina, Finn, and Kay talk points plus the Remembrance Shrine finish point |

This removes a stale start interaction, gives each placed spirit its reviewed
identity, and adds the previously omitted final location. The shrine coordinate
comes from `BP_NPC_RestlessGhostMemorial` in the current world, not a guide map.

## 2026-10-10 Black Knight's Fortress shard deep review

| Objective state | Before | After |
|---|---|---|
| `DR_Main_Objective_3` | Dreaming Stone mixed with nine shard registry landmarks | Dreaming Stone only |
| `DR_Main_Objective_4`, `_4_1` through `_4_12` | Dreaming Stone plus only nine named shard registry landmarks | All thirteen authored `DR_MQ_Fragment_1..13` markers |
| `DR_Main_Objective_6` | Dreaming Stone mixed with shard markers | Dreaming Stone only |
| `DR_Main_Objective_4_13` | All fragment markers plus unrelated quest-stage markers | Dreaming Stone only |

This fixes both omission and phase contamination. Four locations absent from
the generic registry-name join are recovered from the quest's authored marker
family, while investigation and return phases no longer inherit collection
locations.

## 2026-10-10 additional whole-quest reviews

| Quest | Objective state | Before | After |
|---|---|---|---|
| Even More Restless Ghosts | `FindGhosts0/3`, `1/3`, `2/3` | Three anonymous Ghost markers plus stale Zanik marker | Named Windswept, Reminiscing, and Paint-Spattered ghosts plus Dust-covered Sketchbook |
| Animal Magnetism | `None` | Manktongue and Advert mixed together | Advert-Inna-Bottle only |
| Animal Magnetism | `Objective2` | Manktongue and Advert mixed together | Manktongue only |
| Animal Magnetism | `Objective3` | Duplicate Manktongue points and Undead Chicken only | Undead Chicken, Undead Twig Bundle, and Manktongue/Pulpknuckle camp |
| Animal Magnetism | `Objective4` | Generic Manktongue marker | Exact unique Manktongue placement |
| Things That Go Boom In The Night | `FindSpectralChinchompaSkulls` | Vannaka only | 26 authored Spectral Chinchompa spawn points plus Vannaka return |
| Letters For The Dead | `ObjectiveZero` | Missing Letter | Fellhollow Postie Pete only |
| Letters For The Dead | `ObjectiveOne` (0/3) | Both Postie Pete placements plus a generic Missing Letter | Suspicious Skeleton (Letter 1) plus PostBox1 |
| Letters For The Dead | `ObjectiveTwo` (1/3) | Both Postie Pete placements plus the same generic Missing Letter | Spectral hanging letter (Letter 2) plus PostBox2 |
| Letters For The Dead | `ObjectiveThree` (2/3) | Both Postie Pete placements plus the same generic Missing Letter | Shaun (Letter 3) plus PostBox3 |
| Letters For The Dead | `ObjectiveReturn` | Both Fellhollow and Dowdun Postie Pete placements | Fellhollow Postie Pete only |
| Biohazard | `DR_SQ3_HuntRavens` | Quest Zanik plus five unrelated Zanik placements; no raven sources | Ten authored Corrupted Raven spawns plus quest Zanik hand-in |
| Wanted! | `DR_SQ2_ReturnToVannaka` (3/3) | Quest marker plus four unrelated Vannaka placements | Quest-owned Dowdun Reach Vannaka hand-in only |
| Cook's Assistant | `Objective1` | Duplicate Slopfinger/GoblinChef markers; no ingredient interaction | Exact Dairy Cow plus Slopfinger hand-in; dynamic egg and farming sources documented |
| Cook's Assistant | `Objective2` | Slopfinger approximation | Exact placed Goblin Stove interaction |
| Cook's Assistant | `Objective3` | Unreviewed/automatic candidates | Exact Manktongue signed-cake delivery |
| Cook's Assistant | `Objective4` | Unreviewed/automatic candidates | Exact Slopfinger cake-signing interaction |
| Cook's Assistant | `Objective5` | Unreviewed/automatic candidates | Exact Slopfinger completion report |
| Heartstrings | `follow the blue-leafed trees` | One broad `Mesa_HS` area marker | All seven exact placed Heartstrings memorial actors |
| Beast in the Sand | `FollowBeast` | First broad mirage area plus a later task-giver return marker | Exact four-stage Mirage1–Mirage4 trail; unrelated return marker removed |
| Beast in the Sand | `GiveWater` | Unreviewed exact-name inference | Reviewed exact Efina world-registry placement |
| Beast in the Sand | `Return` | Unreviewed exact-name inference | Reviewed exact Manathep world-registry placement |
| Brink of Extinction | `Objective1` | Zi marker plus six unrelated Blazing Black Knights and a quest proxy | Exact quest-owned Zi food hand-in marker only |
| Brink of Extinction | `Objective4` | Duplicate bouncer candidates inferred from the full flow | One exact quest-owned KotHaar Bouncer/Cave entrance marker |
| Brink of Extinction | `Objective6` | Three duplicate bouncer candidates | One exact quest-owned KotHaar Bouncer final-return marker |
| Contact! | `Objective5`, `Objective7` | Automatic dungeon-name inference | Reviewed Kalphite Hive entrance substitution for both interior Zanik conversations |
| Mark Day | `ConvinceNoble.0` | Eight markers: duplicated councillors plus unrelated Shangruuh task-giver targets | Exactly three authored councillor areas |
| Mark Day | `ReturnToShangruuh` | Unreviewed flow candidate | Exact quest-owned Shangruuh final-return marker |
| Rag and Bone Garou | `GatherIngredients` | Unreviewed exact-name inference | Reviewed exact Lagra hand-in; renewable ingredient sources documented |
| Saga of the Adventurer | `FindTheClues` | Laguna task-giver plus a broad clue area | Authored rocky-spire clue area only; unrelated Laguna marker removed |
| Saga of the Adventurer | `LocateTheGateway` | Unreviewed direct area candidate | Reviewed final gateway/Crucible-spire search area |
| Scaling Difficulties | `SearchLocation` | Hedric task-giver plus three broad/invalid quest areas | Exact two dead-scout actors and exact scorched-wall actor |
| Scaling Difficulties | `FindDragon` | Hedric hand-in only | Exact quest-owned `The Radiant` red-dragon encounter |
| Scaling Difficulties | `ReturnToGarou` | Duplicate Hedric quest-marker and registry points | One exact quest-owned Hedric return marker |
| The Dig Site | `DigScroll.0`–`DigScroll.3` | One broad Garou-ruins search area; initial state also included duplicate area and Lehran | Four exact placed archaeology dig-site actors for every counter state |
| The Dig Site | `ReturnToArchaeologist` | Near-duplicate quest-marker and registry locations | One exact quest-owned Lehran return marker |
| The Mirror Crack'd | `BringBars` | Broad optional mining area plus duplicate Valas proxy and registry points | One exact Valas hand-in marker |
| Rogue Trader | `FindMoonstone` | Three broad search-area centres, stale Domri return marker, and three ambiguous Moonstone markers | Exactly three current-build Moonstone registry identities, labelled northern cave, rock platforms, and Ruby Vine cave |
| Rogue Trader | `ReturnToShopkeeper` | Duplicate quest target and Domri marker | Exact Domri registry identity only |
| Contact! | `Objective1` | Broad search area, ordinary guardians, Prime, and entrance mixed together | Kalphite Hive entrance only, because Vault Guardian Prime is inside |
| Contact! | `Objective2` | Prime, broad region, unrelated Avisk marker, and two ambiguously named guardians | Hive entrance for the interior damaged guardian, plus exact dead-guardian and buried-guardian actor placements |
| Mirror Mirror | `FindSilverThread` | Mirror, all six future candles, five broad candle areas, eleven unrelated rams, and the spellbook | Exact spellbook, Edna, nearby green-dragon source, and mirror hand-in |
| Mirror Mirror | `LightCandles` | Six identically named Candle markers without geographic distinction | The same six exact registry identities labelled by their verified region and physical context |
| Withering Heights | `SpeakToPostiePete` | Death, a broad Death area, quest target, and both regional Postie Pete placements | Exact Forgotten Temple Postie Pete only |
| Withering Heights | `InvestigateStandingStones` | Broad standing-stones area plus three identically named stones; Cathan omitted | Exact nearby Cathan plus the three individually labelled standing-stone clues |
| Brink of Extinction | `Objective2` through `Objective2.3` | Repeated broad area and duplicated anonymous Pedestal candidates at every counter state | Exactly four current-build pedestal identities, consistently labelled north, east, south, and west |
| Mapping the Sands I | `Objective1`, `0/3` through `3/3` | All twelve Moon Sprite registry identities, including Survey II and the camp sprite | Exactly the three non-b-series Survey I actors matched to their registry positions |
| Mapping the Sands II | `Objective1`, `0/8` through `7/8` | All twelve Moon Sprite registry identities, including Survey I and the camp sprite | Exactly the eight b-series Survey II actors matched to their registry positions |
| Icthlarin's Little Helper | `FindGarou` | Two Uzzer elder copies selected by broad “Garou” matching; Alcarrid Elder omitted | Exact quest-owned Alcarrid leader target (Ancamairon) |
| Icthlarin's Little Helper | `InvestigateGarouProblem` | Uzzer and Manafem elders; community maps also aggregate unrelated later slabs and scouts | Exact quest-owned Alcarrid camp investigation area only; no fabricated individual interaction |
| Icthlarin's Little Helper | `ReturnToFirstGarou` | Both Shangruuh phase copies | Exact Shangruuh copy beside the Dowdun/Bastion passage |
| Icthlarin's Little Helper | `FindThreeKeys` | Mirror sockets, five Icthlarin copies, duplicated camps, phase copies, and unrelated actors | Exact active elders for Alcarrid, Manafem, and Uzzer |
| Icthlarin's Little Helper | four incomplete Alcarrid reputation states | Repeated elders, Shangruuh copies, and duplicated task locations | Exactly the three authored Alcarrid task-giver targets (Valas, Domri, and Lagra) |
| Icthlarin's Little Helper | completed Alcarrid reputation / elder states | Elders and envoys from all three clans and phases | Exact quest-owned Ancamairon leader target |
| Icthlarin's Little Helper | `RepairTheMirrors` | Mirror sockets mixed with five unrelated Icthlarin copies and registry duplicates | Exactly the three authored Mirror Relay lens sockets |
| Icthlarin's Little Helper | `FindLastKey` | Three Alcarrid mirror objects | Exact Manafem and Uzzer elders, since the aggregate save state does not identify which final clan remains |
| Icthlarin's Little Helper | Fuzan fight states | Interior fight marker mixed with its entrance | Fight Caves overworld entrance only |
| Contact! | barrier, shortcut-crystal, and Prime-return states | Interior markers mixed with the Hive entrance and stale Avisk candidates | Kalphite Hive overworld entrance only |
| Contact! | progressed Crystal Helix counters | Generic Avisk area/crystal candidates | Exact damaged-, dead-, and buried-guardian sources; interior guardian represented by the Hive entrance |
| Withering Heights | progressed standing-stone counters | Three anonymous quest markers | Three exact registry stones labelled by village, waterfall, and fortress clue |
| Withering Heights | all curse-fragment counter states | Broad areas, stale Standing Stones area, and later Silverthorn-only collapse | Emberwood Village plus Vekchenven Kara and Silverthorn Keep entrances retained for every aggregate counter state |
| Doric's Quest | Necromancer's Staff | Doric only | Vekchenven Kara entrance for the fixed effigy recipe plus Doric hand-in |
| Doric's Quest | Shadow Sword | Doric only | Current Fellhollow Zogre spawns, Vekchenven Kara boss entrance, and Doric hand-in |
| Dragon Slayer | Challenge Horn | Broad regions and the later Velgar entrance | All extracted Dragon Tooth deposits plus the authored southern Bloodblight Sap ruins; dynamic bronze/bench sources documented |
| Song of the Ancients | three lyric counters | Interior slab, task giver, and vault entrance mixed together | Dragonkin Vault overworld entrance only |
| The Dig Site | progressed scroll counters | Ruins search area mixed with the later task-giver hand-in | Authored Forbidden Scroll ruins area only |

The new enemy and interaction coordinates all come from current-build world
actors or registry identities. Walkthroughs were used to validate semantics and
ordering, not as coordinate sources.

- Catalog: 41 quests, 230 public objective states
- Substantive step changes: 47
- Markers removed as stale or superseded: 119
- Markers added or corrected: 6
- Unavailable public objectives after review: 0

| Quest | Objective | Before → after | Removed | Added |
|---|---|---|---|---|
| Dragon Slayer | Find and hunt a deer — Hunt a deer and bring one of its antlers to the Wise Old Man. | `multiple` → `multiple` | BMCastle @ 0.17187582, 0.66053476 | — |
| Dragon Slayer | Locate cathan in the vault — Investigate the blue-flame door in the swamp. Find out what Cathan is up to. | `fixed` → `fixed` | Velgar's Rise entrance (Velgar's Rise) @ 0.26167576, 0.51888457 | Vertentis Kara entrance (Vertentis Kara) @ 0.2710176, 0.62499512 |
| Dragon Slayer | Venture into Ghornfell — Gear yourself up and push forward into Ghornfell in search of Cathan. | `multiple` → `multiple` | WiseOldManObjectiveDS @ 0.1419211, 0.70314926 | — |
| Dragon Slayer | Look for evidence of Cathan at the tower — Investigate Velgar's tower in search of Cathan. | `multiple` → `multiple` | GhornfellArea @ 0.21914457, 0.53694864 | — |
| Dragon Slayer | Craft the summon horn — Find the ingredients and craft the Challenge Horn. Check the recipe in your journal for more information. | `multiple` → `multiple` | DragonsLairCathanArea @ 0.2617246, 0.51762467 | — |
| Highlighting the Problem | Search the runecrafting guild for wizard statue — Search the Runecrafting Guild for a statue. | `fixed` → `multiple` | — | MagicGuild_HtP @ 0.07715912, 0.63187631 |
| Black Knight's Fortress | DR_Main_Objective_0 — Speak to Cathan at the eastern gates in Fellhollow | `multiple` → `fixed` | DR_MQ_Death @ 0.33020277, 0.3646539 | — |
| Black Knight's Fortress | DR_Main_Objective_3 — Investigate the Dreaming Stone | `multiple` → `multiple` | DR_MQ_Doric @ 0.56936667, 0.41005238<br>DR_MQ_OldMan_Target @ 0.55591667, 0.35863095 | — |
| Black Knight's Fortress | DR_Main_Objective_5 — Defeat the Gatekeeper | `multiple` → `fixed` | DR_MQ_DreamingStone @ 0.5703, 0.40994048<br>Dreaming Stone Shard @ 0.50504724, 0.44698186<br>Dreaming Stone @ 0.57039286, 0.40989286<br>Dreaming Stone Shard @ 0.50475998, 0.37945687<br>Dreaming Stone Shard @ 0.57652862, 0.33476183<br>Dreaming Stone Shard @ 0.56267857, 0.36642998<br>Dreaming Stone Shard @ 0.51314174, 0.44434374<br>Dreaming Stone Shard @ 0.59089929, 0.33177742<br>Dreaming Stone Shard @ 0.52945238, 0.424<br>Dreaming Stone Shard @ 0.62498102, 0.31870842<br>Dreaming Stone Shard @ 0.64690476, 0.36870238 | DR_GateKeeper @ 0.5864619, 0.4681619 |
| Black Knight's Fortress | DR_Main_Objective_6 — Return to the Dreaming Stone | `multiple` → `multiple` | DR_MQ_Fragment_8 @ 0.53901429, 0.38740476<br>DR_MQ_Fragment_9 @ 0.62325, 0.40284524<br>DR_MQ_Fragment_10 @ 0.6469119, 0.36865952<br>DR_MQ_Fragment_11 @ 0.50978095, 0.40071429<br>DR_MQ_Fragment_12 @ 0.50505952, 0.44700476<br>DR_MQ_Fragment_1 @ 0.6237154, 0.32018209<br>DR_MQ_Fragment_2 @ 0.59098333, 0.33170476<br>DR_MQ_Fragment_13 @ 0.5760119, 0.33353571<br>DR_MQ_Doric @ 0.56936667, 0.41005238<br>DR_MQ_Death @ 0.33020277, 0.3646539<br>DR_MQ_OldMan_Target @ 0.55591667, 0.35863095<br>DR_GateKeeper @ 0.5864619, 0.4681619<br>DR_MQ_Fragment_3 @ 0.50478571, 0.37952143<br>DR_MQ_Fragment_4 @ 0.52945476, 0.4240119<br>DR_MQ_Fragment_5 @ 0.55027143, 0.37528333<br>DR_MQ_Fragment_6 @ 0.56269762, 0.36642857<br>DR_MQ_Fragment_7 @ 0.51314174, 0.44434374 | — |
| Black Knight's Fortress | DR_Main_Objective_4_13 — Return to the Dreaming Stone | `multiple` → `multiple` | DR_MQ_Fragment_8 @ 0.53901429, 0.38740476<br>DR_MQ_Fragment_9 @ 0.62325, 0.40284524<br>DR_MQ_Fragment_10 @ 0.6469119, 0.36865952<br>DR_MQ_Fragment_11 @ 0.50978095, 0.40071429<br>DR_MQ_Fragment_12 @ 0.50505952, 0.44700476<br>DR_MQ_Fragment_1 @ 0.6237154, 0.32018209<br>DR_MQ_Fragment_2 @ 0.59098333, 0.33170476<br>DR_MQ_Fragment_13 @ 0.5760119, 0.33353571<br>DR_MQ_Doric @ 0.56936667, 0.41005238<br>DR_MQ_Death @ 0.33020277, 0.3646539<br>DR_MQ_OldMan_Target @ 0.55591667, 0.35863095<br>DR_GateKeeper @ 0.5864619, 0.4681619<br>DR_MQ_Fragment_3 @ 0.50478571, 0.37952143<br>DR_MQ_Fragment_4 @ 0.52945476, 0.4240119<br>DR_MQ_Fragment_5 @ 0.55027143, 0.37528333<br>DR_MQ_Fragment_6 @ 0.56269762, 0.36642857<br>DR_MQ_Fragment_7 @ 0.51314174, 0.44434374 | — |
| Black Knight's Fortress | DR_Main_Objective_4_-1 — Speak to Doric about the shards around Dowdun Reach | `multiple` → `fixed` | DR_MQ_OldMan_Target @ 0.55591667, 0.35863095 | — |
| Withering Heights | UnlockGate — Unlock the strange door. | `multiple` → `fixed` | WomGFExitTarget @ 0.32243304, 0.42154587 | — |
| Withering Heights | SearchTheStructure — Speak to the temple's inhabitant. | `multiple` → `fixed` | EnterDeathsDomainTarget @ 0.34244367, 0.4021815 | — |
| Withering Heights | SpeakToPostiePete — Speak to Postie Pete. | `multiple` → `multiple` | DeathsTempleArea @ 0.33398188, 0.36679069 | — |
| Withering Heights | GoThroughDoorToFH — Enter Fellhollow. | `multiple` → `fixed` | PostiePeteTarget @ 0.342336, 0.33922824<br>Postie Pete @ 0.55974698, 0.35898814<br>Postie Pete @ 0.34258817, 0.33918205 | — |
| Withering Heights | FindACleansingPool — Remove the Soul Rift at a Purification Pool. | `multiple` → `fixed` | EnterFHArea @ 0.33844531, 0.29857729<br>PostiePeteTarget @ 0.342336, 0.33922824<br>Postie Pete @ 0.55974698, 0.35898814<br>Postie Pete @ 0.34258817, 0.33918205 | — |
| Withering Heights | InvestigateStandingStones — Find Cathan and look into what he was investigating. | `multiple` → `multiple` | CleansingPoolTarget @ 0.33524136, 0.25936008 | — |
| Withering Heights | ReachedStandingStones — Get location of curse fragments 0/3. | `multiple` → `multiple` | StandingStonesArea @ 0.35706748, 0.23668595<br>Standing Stone @ 0.3545681, 0.23334665<br>Standing Stone @ 0.36072019, 0.23483984<br>Standing Stone @ 0.35814121, 0.2327137 | — |
| Withering Heights | InteractedWithStone1 — Get location of curse fragments 1/3. | `multiple` → `multiple` | StandingStonesArea @ 0.35706748, 0.23668595<br>Standing Stone @ 0.3545681, 0.23334665<br>Standing Stone @ 0.36072019, 0.23483984<br>Standing Stone @ 0.35814121, 0.2327137 | — |
| Withering Heights | InteractedWithStone2 — Get location of curse fragments 2/3. | `multiple` → `multiple` | StandingStonesArea @ 0.35706748, 0.23668595<br>Standing Stone @ 0.3545681, 0.23334665<br>Standing Stone @ 0.36072019, 0.23483984<br>Standing Stone @ 0.35814121, 0.2327137 | — |
| Withering Heights | BreakRemainingtwoSeals — Gather two more curse fragments from the locations described in the standing stones. | `multiple` → `multiple` | VillageArea @ 0.17002493, 0.14765352<br>FortressArea @ 0.56306436, 0.19303314<br>FallArea @ 0.41360755, 0.14799897 | — |
| Withering Heights | BreakTheFinalSeal — Find the last curse fragment from the locations described in the standing stones. | `multiple` → `multiple` | VillageArea @ 0.17002493, 0.14765352<br>FortressArea @ 0.56306436, 0.19303314<br>FallArea @ 0.41360755, 0.14799897 | — |
| Withering Heights | ChallangeImaru — Enter the Shattered Spire and defeat Imaru. | `multiple` → `multiple` | VillageArea @ 0.17002493, 0.14765352<br>FortressArea @ 0.56306436, 0.19303314<br>FallArea @ 0.41360755, 0.14799897 | — |
| Withering Heights | ReturnToDeath — Bring Imaru's head to Death. | `multiple` → `multiple` | SlayImaruTarget @ 0.34013119, 0.14219912<br>NPCDeath @ 0.32929273, 0.3641249 | — |
| Icthlarin's Little Helper | FindThreeKeys — Travel to Moon Garou camps, find their elders, and earn their trust to gain their Mirror Lenses | `multiple` → `multiple` | CampLeader_A_Target @ 0.62743857, 0.54721181 | — |
| Icthlarin's Little Helper | ReturnToIcthlarin — Bring Fuzan's head to Icthlarin at the Mirror Relay | `multiple` → `multiple` | FightFuzan @ 0.81839257, 0.88695345 | — |
| Icthlarin's Little Helper | FirstGarouEarnReputation.NoReputation — Gain the trust of the Alcarrid Camp | `multiple` → `multiple` | CampLeader_A_Target @ 0.62743857, 0.54721181 | — |
| Icthlarin's Little Helper | FirstGarouEarnReputation.LowReputation — Gain the trust of the Alcarrid Camp | `multiple` → `multiple` | CampLeader_A_Target @ 0.62743857, 0.54721181 | — |
| Icthlarin's Little Helper | FirstGarouEarnReputation.MediumReputation — Gain the trust of the Alcarrid Camp | `multiple` → `multiple` | CampLeader_A_Target @ 0.62743857, 0.54721181 | — |
| Icthlarin's Little Helper | FirstGarouEarnReputation.HighReputation — Gain the trust of the Alcarrid Camp | `multiple` → `multiple` | CampLeader_A_Target @ 0.62743857, 0.54721181 | — |
| Icthlarin's Little Helper | FirstGarouEarnReputation.MaxReputation — You have completed the tribe's tasks. Return to the Alcarrid Elder | `multiple` → `multiple` | FirstGarou.CampNPC @ 0.62743857, 0.54721181 | — |
| Icthlarin's Little Helper | SpeakToIcthlarin — Speak to Icthlarin at the Mirror Relay | `multiple` → `multiple` | GarouCamp_A_Area @ 0.62376876, 0.55132352<br>Bastion_DoorToUmS_Target @ 0.58914193, 0.47370564 | — |
| Icthlarin's Little Helper | SpeakToGarouElder — Speak to the Alcarrid Elder and complete tasks to earn their trust | `multiple` → `multiple` | GarouCamp_A_Area @ 0.62376876, 0.55132352<br>Icthlarin_Mirror_Target @ 0.62650555, 0.54707266 | — |
| Icthlarin's Little Helper | RepairTheMirrors — Place the three Mirror Lenses into the Mirror Relay | `multiple` → `multiple` | Icthlarin_Mirror_Target @ 0.62650555, 0.54707266 | — |
| Icthlarin's Little Helper | LookForLife — Look for signs of life in the desert | `multiple` → `fixed` | Bastion_DoorToUmS_Target @ 0.58914193, 0.47370564 | — |
| Beast in the Sand | Return — Return to Manathep - The Bereaved | `multiple` → `fixed` | TribeC.Task1.Mirage3 @ 0.56005707, 0.86169595 | — |
| Brink of Extinction | Objective3 — Return to Zi in the Burning Spire region of the Umbral Sands | `fixed` → `fixed` | UmS_PedestalRadius @ 0.7365049, 0.73121271 | Zi @ 0.75520133, 0.742969 |
| Brink of Extinction | Objective4 — Use the password to enter the Cave of Screams, within the Burning Spire region | `multiple` → `multiple` | Zi @ 0.75473438, 0.74288267<br>UmS_PedestalRadius @ 0.7365049, 0.73121271 | — |
| Contact! | Objective4 — Use the Crystal Helix to pass through the magical barrier in the Kalphite Hive | `multiple` → `multiple` | VaultGuardianSearchArea @ 0.82114471, 0.37418317<br>Vault Guardian @ 0.80232581, 0.35629501<br>Vault Guardian @ 0.8115281, 0.3578809<br>Vault Guardian Prime @ 0.82224476, 0.3774861<br>UmS_Contact_Region @ 0.79927493, 0.37002522<br>VaultGuardianPrime @ 0.82227731, 0.37738573 | — |
| Contact! | Objective7 — Return to Zanik in the Shortcut Chamber within the Kalphite Hive | `multiple` → `fixed` | UmS_AviskCrystals @ 0.82573476, 0.38311333<br>Avisk Crystal @ 0.80168619, 0.40219721 | — |
| Mark Day | ReturnToShangruuh — Return to Shangruuh - Garou Envoy in the Uzzer Camp | `multiple` → `fixed` | TribeB_Taskgiver3_NPC @ 0.8771389, 0.56805424 | — |
| Rogue Trader | ReturnToShopkeeper — Return to Domri - The Merchant in the Alcarrid Camp | `multiple` → `multiple` | TribeA_Task2_NPC @ 0.61753333, 0.54792393 | — |
| Saga of the Adventurer | LocateTheGateway — Follow the final clue to the last stone slab. Your Saga has been updated! | `fixed` → `fixed` | TribeB.Task1_Target_1 @ 0.85036793, 0.44673281 | TribeB.Task1_Area_step2 @ 0.90597829, 0.3785566 |
| Scaling Difficulties | ReturnToGarou — Return the dragon heart to Hedric - The Hunter | `multiple` → `multiple` | TribeC_Taskgiver2_NPC @ 0.52983583, 0.71719657 | — |
| Song of the Ancients | Return — Return to Laguna - The Bard in the Manafem Camp. | `fixed` → `fixed` | TribeC.Task3.Statue1 @ 0.5528615, 0.84484357 | TribeC_Taskgiver3_NPC @ 0.52509417, 0.71318955 |
| The Dig Site | ReturnToArchaeologist — Return to Lehran - The Archaeologist in the Uzzer Camp | `multiple` → `multiple` | TribeB_Taskgiver2_NPC @ 0.88071876, 0.56459781 | — |
| Goblin Diplomacy | Objective1 — Investigate the ruins to the northeast and the guild to the south of Zanik. | `fixed` → `multiple` | ZanikGoblinDeplomacy @ 0.20321014, 0.6563285 | Outlook Ruins reliquary @ 0.21939348, 0.64779114<br>Adventurers' Guild reliquary @ 0.2182855, 0.69625933 |
| Highlighting the Problem | Investigate for clues about the statue — Investigate the area for clues about the statue. | `area` → `fixed` | MagicGuild_HtP @ 0.07715912, 0.63187631 | Thishepen Kara entrance @ 0.09024376, 0.63956152 |
| Highlighting the Problem | Search the runecrafting guild for wizard statue — Search the Runecrafting Guild for a statue. | `multiple` → `fixed` | MagicGuild_HtP @ 0.07715912, 0.63187631<br>TeleportationTarget8 (duplicate candidates) | Runecrafting Guild wizard statue @ 0.07216621, 0.63033771 |
| Highlighting the Problem | Charge the orb — Find a way to charge the mysterious orb. | `area` → `multiple` | MagicGuild_HtP @ 0.07715912, 0.63187631 | Three exact tower charging pedestals |
| Highlighting the Problem | Return to statue — Return to the wizard statue. | `fixed` → `fixed` | MagicGuild_HtP @ 0.07715912, 0.63187631 | Runecrafting Guild wizard statue @ 0.07216621, 0.63033771 |
| Biohazard | DR_SQ3_EntryPickup — Speak to Zanik about the Corrupted Essence. | `multiple` → `fixed` | Repeated quest marker plus every global Zanik phase copy | Zanik beneath The Approach only |
| Biohazard | DR_SQ3_FindIngredients — Bring Zanik Dragon Blood, a Twilight Lily and Unholy Water. | `multiple` → `multiple` | Lily area, exact lily and exact water only | Four Dowdun Blue Dragon sources and Zanik hand-in added; exact lily and water retained |
| Wanted! | DR_SQ2_EntryJournal — Bring the Journal to Vannaka. | `multiple` → `fixed` | Every global Vannaka placement | Wanted!'s Courtyard Vannaka marker only |
| Wanted! | all 0/3–2/3 journal states | `multiple` → `multiple` | Rat Master, duplicated Houndmaster, and Summoner spawn points | Three exact switchable journal interactions |
| What's Theirs is Mine! | DR_SQ1_Temple — Look for signs in the Cathedral. | `multiple` → `fixed` | Doric quest marker and global Doric copies | Exact Cathedral Scrawled Notes interaction |
| What's Theirs is Mine! | DR_SQ1_Garrison — Look for signs in the Garrison. | `fixed` → `fixed` | Nexus-to-Garrison portal proxy | Exact Garrison schematics interaction |
| What's Theirs is Mine! | DR_SQ1_Aquifer — Look for signs in the Demon Gate. | `fixed` → `fixed` | Nexus-to-Aquifer portal proxy | Exact Dead Zamorak Mage interaction |
| Seeking Salvation | FindRecipe — Access the vault and find the hidden secret within. | `multiple` → `multiple` | Doric marker plus every globally matched Doric placement | Four exact seal-statue interactions and the Skekven Kara entrance |
| Seeking Salvation | LookForDoric — Find someone in Fellhollow that can translate dwarvish text. | `multiple` → `fixed` | Quest marker plus every globally matched Doric placement | Quest-owned Doric marker beside Skekven Kara only |
| The Wild Hunt | HuntBeast — Hunt Nature Spirits and bring three hearts to Vannaka. | `multiple` → `multiple` | Vannaka marker plus every globally matched Vannaka placement | Five exact Nature Spirit spawns and the quest-owned Vannaka hand-in |
| A Melody Remembered | Objective1 — The jewellery box requires a specific ring. | `multiple` → `fixed` | Five duplicate marker/registry representations at one coordinate | One exact Jewellery Box interaction, labelled for the Inspiring Ring of Runecrafting |
| Animal Magnetism | Objective3 — Gather three components and bring them to Manktongue. | `multiple` → `multiple` | Exact chicken and twigs, plus Manktongue used as a combined Pulpknuckle/camp proxy | Exact chicken, twigs, Pulpknuckle smith actor, and separate Manktongue hand-in |
| Doric's Quest | FindSkeletonShortbow — Craft an Undead Ranger's Bow and bring it to Doric. | `multiple` → `multiple` | Quest marker expanded into numerous duplicate Doric phase placements | Exact Skekven Kara entrance for the interior Dragonkin Effigy recipe, plus the quest-owned Doric hand-in |
| Withering Heights | opening route through the first Purification Pool | mixed/duplicated → state-bound fixed markers | Previous and next NPC/area candidates could survive across objective transitions | Exact Wise Old Man, Ominous Door, Death temple, Postie Pete, Fellhollow crossing, then entry Purification Pool marker for each respective state |
| Withering Heights | ReachedStandingStones — Get location of curse fragments 0/3. | duplicated `multiple` → exact `multiple` | Six candidates representing two copies of each of three authored stone targets | Three exact registry stones, one for each clue |
| Withering Heights | ChallangeImaru — Enter the Shattered Spire and defeat Imaru. | `multiple` → `fixed` | Repeated Imaru target plus stale Silverthorn Keep inference | Exact `SlayImaruTarget` at the Shattered Spire only |
| Withering Heights | ReturnToDeath — Bring Imaru's head to Death. | `multiple` → `fixed` | Exact hand-in target plus duplicate Death actor representations | Exact `DeathTempleTarget` only |
| Regicide | AccessSW — Gain access to the Scorned Wilderness. | `fixed` → reviewed `fixed` | Auto-resolved `SW_Door` | Explicit exact Dragon Gate `SW_Door`, verified against the quest flow and guide sequence |
| Icthlarin's Little Helper | EnterUmS → LookForLife | fixed/area → reviewed state-bound fixed/area | Correct direct candidates but no explicit transition review | Bastion door for entry, then only the Alcarrid camp search area after the flow hides the door |
| The Great Body Robbery | Objective5 — Collect lightning in a bottle and bring it to Dr Fenkenstrain. | `dynamic` with no marker → mixed dynamic/fixed | No marker, because no exact lightning-strike point exists | Dynamic-weather guidance retained; exact Fenkenstrain laboratory marker added for the explicit hand-in |

Ordering-only changes are intentionally excluded. Every removed marker came from a cooked location update whose `Revealed` value was false, or was superseded by an authoritative reviewed target. Wiki material was used only to interpret quest sequencing; coordinates remain extracted from the current game build.

