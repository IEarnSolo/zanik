# Quest step location research

Quest markers must be derived from game-authored evidence. Wiki walkthroughs are
used to understand ordering and terminology, but are not accepted as coordinate
sources.

## Reviewed case study: What Remains is Written

Review date: 2026-10-10. External behavior references:

- <https://www.dragonwildsguide.wiki/walkthrough/runescape-dragonwilds-what-remains-is-written>
- <https://www.dragonwildswiki.wiki/quests/Dragonwilds-what-remains-is-written>

The walkthrough order is: discover the sinister text, find Abraxus at Coalridge
Pass, recover five books, hand them to Abraxus, recover three scrolls associated
with Blightscale, Ulgo, and Razlem, and return the scrolls. The cooked game build
independently proves the collection targets through eight bespoke interactable
NPC-derived actors. Each actor has an exact title, `TookBook1..5` or
`TookScroll1..3` visibility boolean, a `QuestFlow.Participant.FH_WhatRemains_*`
tag, a `QuestFlow.Entry.FH_WhatRemainsIsWritten.*` entry tag, and one placed
world transform.

| Actor | Authored display name | Quest state | Map coordinate |
|---|---|---|---|
| `BP_NPC_FH_WhatRemains_Book1` | Aetheric Fundamentals, a Primordial Primer | `TookBook1` | `0.55930888, 0.16139381` |
| `BP_NPC_FH_WhatRemains_Book2` | The Pride of the Avernic | `TookBook2` | `0.60055826, 0.20257285` |
| `BP_NPC_FH_WhatRemains_Book3` | The Wrath of Baba Potterington | `TookBook3` | `0.53110590, 0.18667376` |
| `BP_NPC_FH_WhatRemains_Book4` | The Importance of Chaos | `TookBook4` | `0.52307481, 0.24168120` |
| `BP_NPC_FH_WhatRemains_Book5` | Leonard The Bound | `TookBook5` | `0.47397869, 0.22572224` |
| `BP_NPC_FH_WhatRemains_Scroll1` | The Binding of Ib | `TookScroll1` | `0.58781593, 0.23487890` |
| `BP_NPC_FH_WhatRemains_Scroll2` | Coclear Malificarum | `TookScroll2` | `0.54435060, 0.12216518` |
| `BP_NPC_FH_WhatRemains_Scroll3` | Eventide - A New Spawn | `TookScroll3` | `0.56451488, 0.27595520` |

The flow graph publishes `Abraxus The Eternal` as a generic quest location for
the book and scroll phases, but this is not a complete description of their
action targets. A display-name fallback also found both the Coalridge
`BP_NPC_FH_Abraxus_Ghost` and unrelated Dowdun
`BP_NPC_FH_Abraxus_Dowdun` placements. The original catalog therefore showed
two Abraxus markers for every phase and omitted all eight collectible actors.

The corrected mapping is:

- `ObjectiveZero`: Coalridge Abraxus only.
- `FindBooks`: five book interactions plus Coalridge Abraxus because the text
  explicitly includes the return.
- `FoundBooks`: Coalridge Abraxus only.
- `FindBooksAndReturn`: the same five books plus Coalridge Abraxus.
- `FindScrolls`: the three authored scroll interactions, not generic enemy
  patrol positions.
- `FoundScrolls`: Coalridge Abraxus only.
- `FindScrollsAndReturn`: the same three scrolls plus Coalridge Abraxus.

The reviewed relationships contain actor identities only. Coordinates and
display names are re-resolved from the extracted current build, so a removed,
renamed, duplicated, or unplaced actor fails catalog generation instead of
retaining stale coordinates. The public marker also records whether the point
is a collection or return action.

## Reviewed case study: The Great Body Robbery

Review date: 2026-10-10. Behavior reference:
<https://www.dragonwildsguide.wiki/walkthrough/runescape-dragonwilds-the-great-body-robbery>.

The three grave objectives previously inherited Fenkenstrain's generic quest
location and showed no graves. The current build instead contains exactly three
placed `BP_TheGreatBodyRobbery_BuriedCoffin` actors. Their placed properties
identify `Quest_GreatBodyRobbery`, distinct entry tags, and distinct blocking
booleans. Following those entry points through `QF_GreatBodyRobbery` proves:

| Entry/bool | Granted part | Actor label | Map coordinate |
|---|---|---|---|
| `Coffin01` | Human torso | `BP_TheGreatBodyRobbery_BuriedCoffin3` | `0.334929, 0.62215262` |
| `Coffin02` | Garou Arms | `BP_TheGreatBodyRobbery_BuriedCoffin2` | `0.32324324, 0.60940838` |
| `Coffin03` | Goblin Legs | `BP_TheGreatBodyRobbery_BuriedCoffin` | `0.33857890, 0.61351933` |

All three locations remain visible for the 0/3, 1/3, and 2/3 counter states
because the public runtime snapshot currently exposes only the aggregate
objective text, not the three individual coffin booleans. This is deliberately
described as a possible-target set rather than claiming that every displayed
grave remains unfinished.

The examination-table stages now resolve only registry identity
`B583AF8E-4A86E359-FA1FDF88-8EA67879`; the pylon stage resolves only Strange
Machine identity `AFA8DDF4-402C43D9-030F73B9-872E4367`. Nearby Fenkenstrain
stage actors are no longer shown for those scenery interactions. Capturing
lightning is marked `dynamic`: the quest advances when the player is struck
while carrying the supplied vial, and the game does not author one mandatory
world coordinate. Publishing the laboratory or a guessed weather position for
that action would be false precision.

## Reviewed case study: Restless Ghosts

Review date: 2026-10-10. Behavior reference:
<https://www.dragonwildsguide.wiki/walkthrough/runescape-dragonwilds-restless-ghosts>.

The first objective begins after the Crumbling Well grants the amulet, so the
well itself is no longer the next action. Warren is the first useful
interaction: his extracted registry placement is southwest of `Well_RG`,
matching the ruined-village sequence. The other registry identities were named
only after their regions and relative placements agreed with the guide: Lazina
near the Temple Woods windmill, Finn in the southeastern Bramblemead cave, and
Kay on the Whispering Swamp boundary.

The second objective remains one broad state while the player learns four names
and then remembers them. The cooked flow contains four named remembrance
choices and addresses `QuestFlow.Participant.Memorial_RG`; the world contains
exactly four `BP_NPC_RestlessGhost_Quest` actors and one
`BP_NPC_RestlessGhostMemorial` actor. The marker set therefore shows the four
night-time conversations plus the Remembrance Shrine. Until runtime exposes
the per-name booleans, it cannot safely hide completed spirits or know when
only the shrine remains.

## Reviewed case study: Black Knight's Fortress shard sequence

Review date: 2026-10-10. Behavior cross-checks:
<https://www.dragonwildshub.com/quests/black-knights-fortress> and
<https://kingy.wiki/games/dragonwilds/guides/black-knights-fortress>.

The cooked game files remain authoritative for placement.
`QF_DR_MainQuest_Doric` reveals only `DR_MQ_DreamingStone` for investigation.
`QF_DR_MainQuest_Dreamstone` then defines and conditionally reveals the complete
`DR_MQ_Fragment_1` through `DR_MQ_Fragment_13` family. The former heuristic
registry join found only nine named shard landmarks and also contaminated the
investigation and return states with those landmarks.

All 0/13 through 12/13 counter states now resolve the complete thirteen-marker
authored family from the current build. This shows possible locations rather
than claiming all thirteen remain: public progress contains only the count,
while the cooked flow uses separate per-fragment booleans. Both return states
resolve solely to `DR_MQ_DreamingStone`. The prefix relationship stores no
coordinates and fails closed if the authored family disappears.

## Reviewed case study: Even More Restless Ghosts

Review date: 2026-10-10. Behavior reference:
<https://www.dragonwildsguide.wiki/walkthrough/runescape-dragonwilds-even-more-restless-ghosts>.

The three aggregate counter states previously showed three anonymous `Ghost`
landmarks plus Zanik. Region and relative-position checks identify them as the
Windswept Ghost in Emberwood, the Reminiscing Ghost at Hope's Fall, and the
Paint-Spattered Ghost west of its cave route. The current world also contains a
quest-specific `BP_NPC_FHGhostQuest_Sketchbook` interaction at
`0.51465619, 0.08953525`; this is now included as the paint-spattered branch's
collection action. Zanik is excluded until the actual return state.

Because the public objective exposes only 0/3, 1/3, or 2/3, it cannot identify
which branch remains. Each counter state therefore shows every possible spirit
and the sketchbook rather than falsely hiding a valid destination.

## Reviewed case study: Animal Magnetism

Review date: 2026-10-10. Behavior references are the current cooked
`ST_AnimalMagnetism` dialogue and quest flow, cross-checked with the published
four-step quest summary at
<https://metaforge.app/runescape-dragonwilds/database/quests/animal-magnetism>.

The start now points only to the exact Advert-Inna-Bottle interaction. The next
and final conversation states point only to the unique Manktongue placement.
The ingredient state resolves the quest-specific Undead Chicken, the placed
Undead Twig Bundle at the glowing Bloodblight tree, and Manktongue's camp for
Pulpknuckle's magnetised-bar service and the return. The dialogue proves the
Pulpknuckle recipe requires one iron bar and five law runes; no guessed world
coordinate was introduced for a separate smithing point.

## Reviewed case study: Things That Go Boom In The Night

Review date: 2026-10-10. Behavior reference:
<https://gegebase.com/games/dragonwilds/things_that_go_boom_in_the_night>.

The collection objective previously showed only Vannaka, despite requiring ten
Spectral Chinchompa skulls. The current world contains 26 placed
`BP_SpawnPoint_SpectralChinchompa` actors. All 26 possible hunt locations now
appear with a separate Vannaka return marker. This represents respawning enemy
sources rather than pretending a particular ten are mandatory.

## Reviewed case study: Letters For The Dead

Review date: 2026-10-10. Behavior references:
<https://gegebase.com/games/dragonwilds/letters_for_the_dead> and
<https://www.gamingtoolkits.com/runescape-dragonwilds/quests/letters-for-the-dead>.

The quest journal exposes only three aggregate delivery counters, so generic
name matching previously repeated the same Missing Letter and both Postie Pete
placements for every phase. The cooked conversation graph and placed actor
components provide a stronger, fully ordered chain:

1. the Suspicious Skeleton gives `Letter_1` and sets `TookLetter1`; deliver it
   to the actor tagged `QuestFlow.Entry.FH_LettersForTheDead.PostBox1`;
2. the spectral hanging-letter actor enters the quest's letter interaction and
   gives `Letter_2`; deliver it to the actor tagged `PostBox2`;
3. Shaun's conversation gives `Letter_3`; deliver it to the actor tagged
   `PostBox3`, outside Imaru's tower;
4. return to the Fellhollow `BP_NPC_PostiePete`, not the unrelated Dowdun
   Postie Pete placement.

The walkthroughs corroborate the sequence and landmark descriptions. Every
published coordinate is nevertheless taken from the current cooked world actor
or NPC registry. The source and delivery marker remain visible together for a
counter state because the objective key does not reveal whether the player has
already picked up that state's letter; both are valid next-action locations.

## Reviewed case study: Biohazard

Review date: 2026-10-10. Behavior references:
<https://www.dragonwildsguide.wiki/walkthrough/runescape-dragonwilds-biohazard>
and <https://kingy.wiki/games/dragonwilds/guides/biohazard>.

The raven-sample objective previously showed Zanik and unrelated Zanik
placements but none of the enemies that produce progress. The current build has
exactly ten `BP_SpawnPoint_MagicalRaven` placements, backed by the quest-specific
Magical Raven AI data and the Corrupted Essence Sample item. All ten possible
hunt locations now appear, followed by the quest-owned `DR_SQ3_Zanik` hand-in.
The walkthrough confirms that these corrupted ravens provide the ten samples;
their coordinates and the return position come only from extracted game data.

## Reviewed case study: Wanted!

Review date: 2026-10-10. The cooked objective text, conversation graph, and
quest-location actor agree that the 3/3 state is solely a return to Vannaka.
Its `QFT_SetQuestObjective` directly reveals the quest-owned `DR_SQ2_Vannaka`
marker. The previous generic NPC join added Vannaka placements from four other
regions; those are now excluded. The Dowdun Reach quest overview also identifies
Vannaka in The Courtyard as the quest contact:
<https://runescape-dragonwilds.wiki/guides/dowdun-reach-region-guide/>.

## Reviewed case study: Cook's Assistant

## Reviewed case study: Granite Mauled

- The public first objective is the aggregate instruction to investigate the secrets below Ashien's Watch. The recovered blueprint and final quest interaction are inside Skeklac Kara, so the public world map correctly uses the extracted `DungeonEntrance_SkeklacKara` placement.
- Four placed actors use the class `BP_NPC_Vault_GraniteMauled_Statue_Puzzle_C`, but their world coordinates are around the separate Skekven/Wither Ward encounter. A class-name match is therefore not sufficient evidence that those four placements belong to Granite Mauled. They are deliberately excluded from this quest.
- The translator and blueprint hand-in both resolve to the quest-owned `Doric_GM` marker in Ashien's Watch.
- Guide cross-check: the walkthrough describes the vault investigation, blueprint recovery, and return to Doric. The guide establishes sequence only; marker coordinates remain extracted from the game build.

## Reviewed case study: Heartstrings

- The catalog has one aggregate objective for the entire memorial path rather than seven separately exposed objective keys.
- The cooked world contains exactly seven placed `BP_NPC_Hearstrings_Memorial_C` actors at `(80762.62, 113518.41)`, `(93022.90, 113293.63)`, `(111393.53, 113279.49)`, `(123674.06, 116568.02)`, `(130153.25, 99972.48)`, `(140373.33, 124238.12)`, and `(144549.80, 109848.25)`.
- This exact count matches the seven memorial interactions documented by the quest guides. The objective now exposes all seven authored actors instead of only the broad `Mesa_HS` area marker.
- The game asset does not attach stable ordinal labels to the placed actors, so the UI does not invent memorial numbers. The path wording and guide provide sequence context while the coordinates come exclusively from the current cooked world.

## Reviewed case study: Beast in the Sand

- `FollowBeast` is one public objective reused across an internal moving-target sequence. The compiled flow explicitly reveals and hides `TribeC.Task1.Mirage1`, `Mirage2`, `Mirage3`, and `Mirage4` in order.
- The guide confirms the gameplay meaning: follow three separate Dearg Due howl/sighting stops, with the last authored position leading to Efina's hiding place. All four exact quest-owned placements are therefore exposed as a route because the persisted public progress does not distinguish the private substage.
- An automatic candidate previously included `TribeC_Taskgiver1_NPC` because the same flow reveals it during a later return state. That marker is not part of tracking the beast and is now excluded.
- `GiveWater` uses the exact world-registry placement for `Efina - The Lost` (`2736394F-4613C7C2-566C60B3-DFA40E46`). `Return` uses the exact `Manathep - The Bereaved` placement (`66CC10A1-43D25A90-6B505FAB-74A2DDC2`).

## Reviewed case study: Regicide

- The current catalog exposes five public states. `AccessSW` points directly to the quest-owned `SW_Door`; all four Zilyana/Kuldra interior states use that same overworld entrance under the dungeon/interior marker policy.
- The cooked `ZilyanaNPC` placement is interior/non-publishable, while `SW_Door` is the authored public-world transition. Current guides agree that Zilyana and the Kuldra encounter are beyond the Scorned Wilderness entry sequence.
- No marker changed in this pass: the prior entrance substitution remains the most precise truthful world-map destination.

## Reviewed case study: Icthlarin's Little Helper

- Rechecked all eighteen public states against the current guide sequence and compiled quest flow. The two states without manual overrides (`EnterUmS` and `LookForLife`) already resolve directly to the authored Bastion door and first Garou camp area respectively.
- The remaining states retain their reviewed exact elder, task-giver, mirror-socket, relay, and Fight Caves entrance targets. No coordinate change was warranted in this pass.

## Reviewed case study: Brink of Extinction

- The guide sequence is Zi food hand-in, four freely ordered fire pedestals, return to Zi, password/numbering-book interactions with the KotHaar Bouncer at the Cave of Screams entrance, an interior fight, and a final return to the bouncer.
- `Objective1` previously inherited six Blazing Black Knight placements and a quest-Zi proxy from loose matching of the very short name `Zi`. It now uses only the exact quest-owned `Zi` marker at `(305913.44, 194325.72)`.
- The four pedestal positions remain exact current-build registry landmarks and are retained for every 0/4 through 3/4 counter state because the snapshot exposes only the aggregate count.
- `Objective4`, `Objective5`, and `Objective6` all resolve to the exterior bouncer/Cave of Screams access point. The guide clarifies that both password/book dialogue and final reward happen with that bouncer; no interior coordinate is published.

## Reviewed case study: Contact!

- Rechecked the full guide route against `QF_Contact`: Vault Guardian Prime and the damaged guardian are inside the Kalphite Hive; the dead guardian is beside the outdoor kalphite eggs; the buried guardian is at the ruined building; the stolen fragment leads to Zanik's oasis state; and the final Zanik/crystal interactions are inside the Shortcut Chamber.
- `Objective5` and `Objective7` now have explicit reviewed entrance substitutions. Both conversations happen beyond the magical barrier inside the Kalphite Hive, so they use `KalphiteHive` rather than publishing unusable interior coordinates.
- The three-source aggregate fragment states retain all three proven destinations because the public counter does not expose which source has already been completed.

## Reviewed case study: Mark Day

- The compiled flow owns three independent targets: `Noble1` for Brialka, `Noble2` for Alabrais, and `Noble3` for Chirot. The guide confirms those identities and that the councillors may be persuaded in any order.
- `ConvinceNoble.0` previously included each councillor twice plus Shangruuh's task-giver marker. It now contains exactly the three councillor targets, matching the already-reviewed 1/3 and 2/3 states.
- `ReturnToShangruuh` now explicitly uses `TribeB_Taskgiver3_Target`, the exact quest-owned final-return marker in Uzzer Camp.

## Reviewed case study: Rag and Bone Garou

- This quest exposes one aggregate gather-and-return objective. The required raw farm meat, kalphite shell fragments, and cactus steaks have multiple renewable creature, plant, and merchant sources; no single gathering coordinate is mandatory.
- The marker therefore uses the exact world-registry placement for `Lagra - The Butcher` (`685A686A-4A3B33AE-D302F8B9-E65410FF`), where all ingredients are handed in. The destination note explicitly describes why source markers are omitted.

## Reviewed case study: Saga of the Adventurer

- Current guides describe five ancient stone-tablet/statue readings around the rocky spires north of Uzzer Camp, with the final reading at the Crucible of Humiliation spire.
- The cooked quest flow exposes three authored location identities (`TribeB.Task1_Target_1`, `TribeB.Task1_Target_step2`, and `TribeB.Task1_Area_step2`) but does not provide five separately attributable public-world slab transforms. The implementation therefore retains the proven search areas instead of inventing five points from guide prose.
- `FindTheClues` previously included Laguna's task-giver marker alongside the first search area. It now contains only the authored rocky-spire target area. `LocateTheGateway` explicitly uses the direct `TribeB.Task1_Area_step2` final-gateway area.
- Exact individual slab coordinates remain a documented extraction gap and should only replace these areas if their placed actor identities can be proven in a future asset pass.

## Reviewed case study: Scaling Difficulties

- The walkthrough identifies three investigation interactions in the eastern village: two dead Garou scouts and one scorched wall. The generated world cells prove all three as quest-specific actors.
- Exact current-build positions are: first scout `(248867.22, 201643.16)`, second scout `(251153.05, 206210.39)`, and scorched wall `(252749.34, 199589.16)`. These replace the previous mix of broad quest areas, an invalid near-origin marker, and Hedric's unrelated task-giver marker.
- The red dragon is represented by the quest-owned `The Radiant` marker at `(269865.80, 205534.31)`, agreeing with the guide's instruction to travel farther east after the clues.
- The final hand-in uses `TribeC_Taskgiver2_Target`, the exact Hedric return marker. The similar registry landmark is not duplicated.

## Reviewed case study: Song of the Ancients

- The only unreconciled rows, `FindStatues.3` and `FindStatues.4`, are blank string-table records that are not referenced by the compiled flow. They are the two technical records already excluded from the 230-step public catalog, not playable quest states.
- The playable 0/3, 1/3, and 2/3 lyric states continue to use the verified Dragonkin Vault entrance under the interior-location policy, followed by Laguna's exact Manafem return marker.

## Reviewed case study: The Dig Site

- The generated world cells contain exactly four placed `BP_UmS_Mainquest_TribeB_Task2_ArchaeologyDigSite` actors, matching the four Forbidden Scrolls described by the quest flow and guides.
- Exact positions are `(373582.12, 186705.53)`, `(363490.72, 179662.34)`, `(366933.28, 171872.60)`, and `(376591.56, 175543.34)`. These replace the single broad 30,000-unit search-area marker.
- Every 0/4 through 3/4 state shows all four because the persisted counter does not identify the already-completed sites. The final state uses only the exact `TribeB_Taskgiver2_Target` marker for Lehran, removing the nearly duplicate registry point.

## Reviewed case study: The Mirror Crack'd

- The only playable objective is to bring three adamant bars to Valas. Guides mention a suggested adamant-ore search area, but ore and completed bars may be obtained elsewhere; entering that area is not the quest progression trigger.
- The marker now uses the exact `Valas - The Blacksmith` registry placement (`F8250830-405F796E-EC7E0581-8790FA54`) and removes the broad optional resource area plus the duplicate quest-location proxy.


Review date: 2026-10-10. Behavior references:
<https://metaforge.app/runescape-dragonwilds/database/quests/cooks-assistant>
and <https://gamever.io/knowledge-base/cooks-assistant-in-dragonwilds-do-not-milk-the-cow>.

The first objective mixes three resource systems with a hand-in. Cooked
Slopfinger dialogue says chicken or terrorbird eggs both qualify and that wheat
must be grown, so neither has one mandatory world coordinate. The same dialogue
points to the nearby Dairy Cow, and the placed `BP_NPC_Quest_Cow` is the exact
interaction that awards the Bucket of Milk. The public step therefore shows the
Dairy Cow and Slopfinger hand-in, with an explicit note explaining the dynamic
egg and farming sources. Objective 2 now points to the separately placed
`BP_NPC_Quest_Stove`, whose interaction consumes the ingredients and creates the
Glorious Cake, rather than approximating the stove with Slopfinger's position.

## Reviewed case study: Rogue Trader

Review date: 2026-10-10. Behavior references:
<https://metaforge.app/runescape-dragonwilds/rogue-trader-quest-moonstones-guide>
and <https://gegebase.com/games/dragonwilds/icthlarin%27s_little_helper>.

The collection objective previously combined three 14,000-unit search-area
centres, the later Domri return location, and three identically named Moonstone
registry markers. The compiled quest flow actually has three distinct entry
states (`Moonstone1`, `Moonstone2`, and `Moonstone3`), while the current world
registry contains exactly three `FirstCamp.Task2` Moonstone identities. Their
positions align one-for-one with the three authored search areas.

The public objective now publishes the exact item positions and labels them by
their physical context: the northern cave, rock platforms, and southern Ruby
Vine cave. The broad area centres and Domri are excluded from that collection
state. The separate return objective resolves only the exact current-build
`Domri - The Merchant` registry identity. Community walkthroughs corroborate
the north-to-south ordering and gypsum interactions, but no guide coordinate is
stored or used by the analyzer.

## Reviewed case study: Contact!

Review date: 2026-10-10. Behavior reference:
<https://gegebase.com/games/dragonwilds/contact%21>.

The initial Prime objective previously mixed a broad search area, two ordinary
guardian remnants, the Prime itself, and the dungeon entrance. The cooked world
contains a dedicated `BP_NPC_Quest_Contact_VaultGuardian_Leader_UmS` actor, but
because that NPC is inside the Kalphite Hive, the public map now follows the
dungeon rule and points only to the independently extracted hive entrance.

The three-fragment objective is not three interchangeable NPCs. Current-build
placements identify a damaged guardian inside the Prime cave, a dead guardian
among the exterior kalphite eggs, and `BP_Contact_Buried_VaultGuardian` at the
ruined building. It now publishes the hive entrance for the interior damaged
guardian and the exact actor positions for the dead and buried guardians. This
also preserves the important behavior distinction: the dead guardian starts the
footprint/Zanik branch, while the buried guardian requires digging and four
Vault Shards. Guide material was used to verify those semantics only.

## Reviewed case study: Mirror Mirror

Review date: 2026-10-10. Behavior references:
<https://wiki.sasgaming.net/wiki/Dragonwilds%3AMirror%2C_Mirror> and
<https://www.videogameschronicle.com/guide/runescape-dragonwilds-mirror-mirror-quest-guide/>.

The reagent objective previously inherited every later candle and unrelated ram
placement because its flow reveals several future quest locations in advance.
It now shows the exact current-build Bedraggled Spellbook in the Emberwood
windmill, the unique Edna actor that crafts spectral silver thread, the nearby
green dragon as the authored local dragon-blood route, and the exact mirror
hand-in. The note preserves the game's alternative: dragon wolves can also
supply dragon blood, so the selected green dragon is not represented as the
only valid source.

The candle objective now contains exactly the six quest-owned Candle registry
identities. Each was correlated spatially with its separately named cooked area:
Fellhollow, Fractured Plains, Stormtouched Highlands, Bloodblight Swamp,
Whispering Swamp, and Bramblemead Valley. The guide descriptions corroborate
those six contexts, while all marker coordinates remain extracted from the
current game build.

## Reviewed case study: Withering Heights entry and standing stones

Review date: 2026-10-10. Behavior references:
<https://www.dragonwildsguide.wiki/walkthrough/runescape-dragonwilds-withering-heights>
and <https://runescape-dragonwilds.wiki/guides/fellhollow-region-guide/>.

`SpeakToPostiePete` previously retained Death and both Postie Pete registry
placements. The exact Forgotten Temple Postie is proven by its colocation with
the quest-owned `PostiePeteTarget`; the Dowdun Reach copy belongs to a later
region and is now excluded.

`InvestigateStandingStones` previously showed a broad area and three anonymous
stones but omitted Cathan, even though speaking to him is the first trigger.
The selected Ghost of Cathan is only 760 world units from the authored area;
the other two Cathan placements are tens of thousands of units away and belong
to different story beats. The step now shows that exact Cathan followed by the
three exact current-build Standing Stone identities, matching the quest flow in
which those interactions reveal the Despair, Rage, and Grief routes.

## Reviewed case study: Brink of Extinction fire pedestals

Review date: 2026-10-10. Behavior references:
<https://dragonwildsguide.com/the-obsidian-heart-quest-walkthrough/> and
<https://corpus.gg/games/dragonwilds/quests/brink-of-extinction>.

The four counter states (`0/4` through `3/4`) previously expanded into many
duplicate Pedestal candidates plus the broad `UmS_PedestalRadius`. The current
world registry contains exactly four quest-owned Pedestal identities inside
that radius. Their positions establish the same north, east, south, and west
layout around the Sinking Pyramid described by the walkthrough, so each marker
now has a stable geographic label.

The four pedestals may be touched in any order, while the decoded quest state
currently exposes only the count. For the progressed states the catalog keeps
all four possible markers visible and explicitly says why; hiding a guessed
subset would be less accurate than showing the verified candidate set.

## Reviewed case study: Mapping the Sands I and II

Review date: 2026-10-10. Behavior references:
<https://kingy.wiki/games/dragonwilds/guides/mapping-the-sands-i> and
<https://gegebase.com/games/dragonwilds/mapping_the_sands_ii>.

All survey objectives previously matched the shared `Moon Sprites` display
name, publishing every one of the twelve registry entries in both quests. The
placed actor classes provide the missing authoritative partition. Survey I has
exactly three actors named `BP_NPC_UmS_MoonSprite1`, `2`, and `3`. Survey II has
exactly eight `b`-series actors: `1b` through `7b`, plus `9b`. The remaining
twelfth identity is `BP_NPC_UmS_MoonSpriteCamp`, colocated with Anamcara, and is
not a survey target.

Every actor position was matched to its registry identity by exact current-build
world coordinates. Survey I is labelled by its position relative to Alcarrid
Camp. Survey II retains the authored actor numbering because public text sources
do not establish stable landmark names for all eight locations. As with the fire
pedestals, each quest exposes a count rather than individual completion bits;
all verified targets therefore remain visible during progressed counter states.

The override schema now supports an `objectiveKeys` family. The analyzer expands
that family into independently gated objective relationships, so counter states
share one reviewed target definition without weakening per-step validation or
the reviewed-relationship count.

## Reviewed case study: Icthlarin's Little Helper

The parent quest contains three clan arcs and many actors with the same character
name in different phases. Name matching was therefore especially unsafe: it
selected Uzzer elders for the initial Alcarrid step, both Shangruuh copies for a
passage-specific return, and Icthlarin/mirror actors for unrelated lens states.

The current cooked flow resolves the Alcarrid leader, three trust-task givers,
three physical mirror sockets, and camp search area by exact quest-location
identity. Exact registry placements distinguish the Manafem and Uzzer elders and
the Dowdun-passage Shangruuh from their phase copies. The four incomplete
Alcarrid reputation tiers retain Valas, Domri, and Lagra because the objective
stores only a tier, not which freely ordered trust task remains. The final-lens
state similarly retains both Manafem and Uzzer elders because it does not expose
which of those two camps the player completed first.

The early `InvestigateGarouProblem` state is not referenced by a current compiled
objective-setting node and no individual progression actor can be proven. Its
marker is consequently limited to the exact quest-owned Alcarrid camp area.
Ancient Garou slabs and dead scouts shown by community map aggregators belong to
later Uzzer and Manafem subquests; they are explicitly excluded rather than
misrepresented as early-story objectives. Walkthrough sequencing confirms the
three camp arcs and return order, while all published coordinates still resolve
only from current-build game identities.

Sequence and semantic cross-checks used the community-maintained full quest
walkthrough at
<https://gegebase.com/games/dragonwilds/icthlarin%27s_little_helper>, the
step/location index at
<https://dragonwilds.gameslantern.com/quests/icthlarins-little-helper>, and the
independent guide at
<https://metaforge.app/runescape-dragonwilds/icthlarins-little-helper-quest-guide>.
The Games Lantern location lists were treated as parent-quest associations, not
proof that every listed object belongs to every objective.

## Final semantic-review pass

The remaining unreviewed medium-confidence objectives were resolved by applying
the same constraints consistently across the catalog:

- interior actions publish only the corresponding overworld dungeon entrance;
- counter-only states keep every independently proven possible target because
  the snapshot cannot identify which target the player already completed;
- quest-giver hand-ins are excluded while the player is still gathering or
  investigating unless the same objective explicitly requires the return;
- renewable creature hunts publish current-build spawn placements rather than a
  nearby NPC or broad region centre;
- fixed resource interactions use extracted actor types or exact quest areas,
  while player-built crafting stations and ordinary material sources remain
  documented as dynamic rather than receiving invented coordinates.

This pass corrected the Contact! Hive sequence, the three Withering Heights
curse-fragment routes, both late Doric recipe objectives, the Dragon Slayer
Challenge Horn resources, Icthlarin's Fight Caves states, Song of the Ancients,
The Dig Site, Mark Day, and the remaining unambiguous NPC hand-ins. The Doric
semantics were cross-checked against
<https://www.gamingtoolkits.com/runescape-dragonwilds/quests/dorics-quest> and
<https://gegebase.com/games/dragonwilds/doric%27s_quest>; Dragon Slayer against
<https://runescape-dragonwilds.wiki/guides/dragon-slayer/>; and Withering
Heights against
<https://www.dragonwildsguide.wiki/walkthrough/runescape-dragonwilds-withering-heights>.

The analyzer now also supports reviewed `world_actor_type` targets. This resolves
every placed actor of an exact current-build class (used for Dragon Tooth mining
nodes) without storing coordinates in the override. If the class disappears or
is renamed in an update, catalog generation fails closed.

`tools/analyze_quest_steps.py` currently correlates:

1. stable `QuestData.PersistenceID` and each authored objective key/text;
2. `QFT_SetQuestObjective_C` nodes in the compiled conversation graph;
3. adjacent `QFT_UpdateQuestLocation_C` operations and reveal state in both
   cooked graph arrangements: updates owned downstream by an objective and
   updates named directly as an objective node's parent;
4. all placed quest-location actors with the same quest asset and exact location
   name (both `BP_QuestLocation_C` and newer native `QuestLocation` exports);
5. extracted NPC placements where a location operation names an NPC.
6. `GameplayObjectRegistry.Landmarks`, including their exact world position,
   localized display name/key, category, gameplay tag, and originating quest
   string table;
7. counter-state objective families whose normalized authored text is identical
   to a step with an already proven destination set.
8. named dungeon actors, quest teleporters, entrance targets, and interior flow
   identities correlated to their authored overworld entrance;
9. exact named world-registry targets, disambiguated with cooked map-region
   boundary polygons when an NPC or object has several placements.
10. reviewed objective-to-game-identity relationships in
    `deploy/quest-step-overrides.json`. These records contain no coordinates:
    each target must resolve to a quest-owned marker, dungeon entrance, or NPC
    placement extracted from the current build, otherwise that step fails closed.
    A precise override that refines a directly authored area must remain inside
    that area's cooked radius; contradictory cross-region targets fail catalog
    generation.

The generated evidence file is `dist/quest-step-analysis.json`. It is deliberately
not a public runtime catalog. A candidate must have an unambiguous map placement
and a reviewed relationship to its objective before promotion.

Current-build baseline:

- 41 playable quests;
- 232 authored objective-table records;
- 230 reachable, localized player objective states;
- 172 authored quest-location world markers;
- 468 in-bounds authored world-registry landmarks;
- 106 objectives associated with at least one quest-location operation;
- all 230 reachable player objectives have at least one publishable destination;
- two blank `Song of the Ancients` records (`FindStatues.3` and `.4`) are
  excluded because no compiled `QFT_SetQuestObjective` node references them;
- 172 reviewed relationships bridge flow states whose parent-node structure does
  not expose the destination directly;
- 16 verified overworld entrance identities, including the 11 traditional
  `DungeonEntrance_*` actors and five newer teleporter/entrance aliases.

The public schema distinguishes `fixed`, `area`, `multiple`, `dynamic`, and
`unavailable` destinations. `area` is used when the player must act within a
verified authored area but can choose the exact point, such as constructing and
claiming a bed in Bramblemead. Multiple is important: shard, letter, candle,
ghost, statue, and similar objectives can have several valid progression
locations. The player page renders every published marker and reports the marker
count.

`QFT_UpdateQuestLocation_C` nodes with `Revealed: false` remain in the internal
evidence report because they explain phase transitions, but they are never
promoted as destinations. They remove or retain a hidden marker from an earlier
phase; treating them as a positive location leaks stale markers into the next
objective.

Interior objective coordinates are never shown directly. The analyzer joins the
interior identity to an independently extracted overworld entrance. For example,
Dragon Slayer's blue-flame-door state resolves to the `VertentisKara` entrance
inside its directly authored `BMSwamp` area, while its later Cathan/tower states
resolve separately to `DragonsLair` at Velgar's Rise. Regicide's Kuldra and
Zilyana states resolve to `SW_Door`.

Collection states whose underlying per-target booleans are not present in the
public snapshot show every still-possible authored target. Wanted!'s 1/3 and 2/3
states are an example: the count does not reveal which freely ordered journal was
collected, so all three trainer placements remain visible. Interior targets point
to the corresponding overworld entrance and retain the interior location as
metadata.

At runtime, the current objective text/key must be read from the validated quest
progress component. The player page must select markers by stable objective key,
not by array position or fuzzy text matching. If the current objective cannot be
decoded or has no proven placement, the UI must say that its location is
unavailable and must not fall back to the quest start marker.

## Strict sequential quest review

The final review is performed quest by quest and objective by objective. A quest
is only considered reviewed after its walkthrough sequence has been reconciled
with compiled quest-flow state and current-build world identities. Walkthroughs
establish *what the player must do*; only extracted game data supplies public map
coordinates.

### Dragon Slayer

- The Keep of Blue Flames investigation now resolves to the placed
  `BP_LoreItem_CathanJournal_Castle_DS2` journal rather than the castle-area
  center.
- The blue-flame door state resolves to the `VertentisKara` overworld entrance.
- The Ghornfell transition no longer retains the Wise Old Man from the prior
  state.
- The tower evidence state resolves to `BP_LoreItemDS5` (the cooked
  `CathanJournal_Velgarslair` record) and the Velgar's Rise phase-copy of Cathan,
  not either of his unrelated Dowdun or Fellhollow copies.
- The Challenge Horn state identifies the six exact Dragon Tooth deposits and
  the authored Bloodblight ruins. Bronze bars and the player-built workbench are
  documented as dynamic inputs.
- The Velgar fight is represented by the Velgar's Rise entrance and the final
  return by the quest-owned Wise Old Man marker outside it.

Semantic references: <https://runescape-dragonwilds.wiki/guides/dragon-slayer/>
and <https://www.dragonwildsguide.wiki/walkthrough/runescape-dragonwilds-dragon-slayer-quest>.

### Goblin Diplomacy

- The opening objective names two investigation destinations. The compiled flow
  branches to `ReliquaryCliffRuins` and `ReliquaryGuild`, and current world data
  places these as `BP_NPC_ReliquaryCastle` and `BP_NPC_ReliquaryGuild`.
- The Zanik marker revealed near that state is the quest-giver location, not a
  substitute for either requested investigation site; it has therefore been
  removed from the opening objective.
- The two visually identical reliquaries are labelled separately as Outlook
  Ruins and Adventurers' Guild in public data.
- The swamp search remains attached to the quest-only
  `BP_SpawnPoint_MagicBeast_Quest_GD` encounter.
- Both the two-amulet hand-in and the unidentified Bandosian-amulet state return
  to the exact quest-owned Zanik marker.

Semantic references: <https://www.dragonwildsguide.wiki/walkthrough/goblin-diplomacy-runescape-dragonwilds>
and <https://game8.co/games/Runescape-Dragonwilds/archives/514703>.

### Highlighting the Problem

- The initial clue is the password statue inside Thishepen Kara, so the marker is
  the extracted `DungeonEntrance_ThishepenKara` placement under the established
  dungeon policy, not the Runecrafting Guild area center.
- The password is submitted to the one placed quest-specific central actor,
  `BP_NPC_HighLightingTheProblem_Statue`.
- Orb charging now exposes all three exact
  `BP_NPC_HighLightingTheProbem_Pedestal` placements at the surrounding towers
  instead of one broad guild-area marker.
- The charged-orb return points back to that same exact central statue.

Semantic references: <https://www.dragonwildsguide.wiki/walkthrough/runescape-dragonwilds-highlighting-the-problem>
and <https://game8.co/games/Runescape-Dragonwilds/archives/514765>.

### Restless Ghosts

- Warren is the first interaction that establishes the Amulet of Ghostspeak's
  purpose.
- The aggregate resolution state correctly exposes Warren, Lazina, Finn, and Kay
  plus the exact `BP_NPC_RestlessGhostMemorial` Remembrance Shrine. The public
  snapshot does not encode which freely ordered spirit remains, so retaining all
  still-possible interactions is deliberate.
- Names and sequence were cross-checked against the guide; all five coordinates
  come from current-build placed quest actors.

Semantic references: <https://www.dragonwildsguide.wiki/walkthrough/runescape-dragonwilds-restless-ghosts>
and <https://game8.co/games/Runescape-Dragonwilds/archives/514778>.

### Black Knight's Fortress

- The opening sequence is now pinned to the quest-owned Death, Cathan, Dowdun
  Wise Old Man, Dowdun Doric, and Dreaming Stone identities. Global Cathan,
  Doric, and Death discoveries are not allowed to multiply these steps.
- Every 0/13 through 12/13 counter retains all thirteen authored shard search
  areas. The public objective contains only the aggregate count, not the
  thirteen internal completion booleans, so removing a marker based on the count
  would guess which shard was collected.
- The shard circles are authored search areas rather than uniformly exact pickup
  points. Several shards are granted by NPC or combat interactions, and the
  correct grave in the Crypt can vary by world.
- The boss state now resolves only to `DR_GateKeeper`, and the final state returns
  to Avisk at the restored Dreaming Stone.

Semantic reference: <https://kingy.wiki/games/dragonwilds/guides/black-knights-fortress>.

### Biohazard

- The alternate sample-pickup entry now resolves only to Biohazard's underground
  Zanik marker instead of every Zanik phase copy.
- The raven state retains all ten current quest-specific raven spawns plus the
  exact Zanik hand-in.
- The ingredient state now includes the exact Twilight Lily, exact Unholy Water,
  all four current Dowdun Blue Dragon placements for Dragon Blood, and Zanik's
  delivery point. Dragon Wolves in Fellhollow remain a documented alternative;
  all 33 renewable spawns are not dumped onto this Dowdun objective.
- The source is the unique cooked `BP_NPC_Pusilineous` actor, publicly labelled
  Putrilius on the Grand Hall roof, followed by the exact Zanik return.

Semantic reference: <https://www.dragonwildsguide.wiki/walkthrough/runescape-dragonwilds-biohazard>.

### Wanted!

- The freely ordered counter states previously used trainer spawn positions.
  This was imprecise because defeating a trainer does not increment the counter;
  interacting with the journal does.
- The three destinations are now the exact switchable Rat Master, Hound Master,
  and Summoner journal actors. Their cooked entry tags, display names, and world
  placements independently agree with the walkthrough.
- The pre-acceptance journal route and 3/3 state both resolve to the quest-owned
  Courtyard Vannaka marker, excluding his other regional copies.

Semantic reference: <https://gegebase.com/games/dragonwilds/wanted%21>.

### What's Theirs is Mine!

- The Cathedral objective no longer retains Doric. It points to the exact
  Scrawled Notes registry identity on the Cathedral upper ledge.
- The Garrison step now points to its exact Scrawled Notes/schematics interaction
  beside the nodestone rather than the broader Nexus portal proxy.
- The Demon Gate step now points to the exact Dead Zamorak Mage interaction
  beyond the shardbound demons rather than the Aquifer portal proxy.
- The final state returns to the quest-owned Doric marker at The Nexus.

Semantic reference: <https://www.dragonwildsguide.wiki/walkthrough/runescape-dragonwilds-what-s-theirs-is-mine>.

### Letters for the Dead

- Each 0/3 through 2/3 objective is a compound state: recover the next missing
  letter and then deliver it to that letter's matching postbox. Both exact
  interactions therefore remain visible for the active state.
- The three pairs are Suspicious Skeleton/Postbox 1, the hanging spectral
  letter/Postbox 2, and Shaun/Postbox 3 outside Imaru's tower.
- The start and completion states use the Fellhollow Postie Pete identity; his
  separate Dowdun placement is excluded.

Semantic references: <https://gegebase.com/games/dragonwilds/letters_for_the_dead>
and <https://www.gamingtoolkits.com/runescape-dragonwilds/quests/letters-for-the-dead>.

### Mirror, Mirror

- The reagent state uses the exact Bedraggled Spellbook interaction in the
  Emberwood windmill, Edna's unique quest actor for spectral silver thread, the
  nearby Green Dragon as a proven dragon-blood source, and the exact Magic
  Mirror hand-in. Dragon wolves are an alternate blood source, but flooding the
  map with every renewable spawn would obscure the authored quest route.
- All six memorial candles are exact registry identities owned by this quest's
  string table and independently fall inside the six named authored candle
  search areas.
- The final conversation returns to the exact Magic Mirror identity.

Semantic references: <https://gegebase.com/games/dragonwilds/mirror%2C_mirror>
and <https://www.videogameschronicle.com/guide/runescape-dragonwilds-mirror-mirror-quest-guide/>.

### Seeking Salvation

- The `FindRecipe` objective previously inherited the revealed `DoricNPC`
  marker, which points at the quest giver and is not the place where any seal or
  recipe progress occurs.
- The current world contains four individually placed
  `BP_NPC_Vault_WitherWard_Statue_Puzzle` actors around the ruins. All four are
  now published as distinct activation markers.
- The dwarven-writing interaction is inside Skekven Kara. Under the dungeon
  policy, it is represented by the exact `DungeonEntrance_SkekvenKara` actor.
- Both Doric conversation states are constrained to the quest-owned Fellhollow
  marker, preventing unrelated Doric phase copies from leaking into the step.

Semantic reference: <https://dragonwildswiki.com/guides/quests/seeking-salvation/>.

### The Wild Hunt

- The current build places five exact `BP_SpawnPoint_WitherBeast` actors at
  willow groves. The objective accepts any three Primordial Hearts, so all five
  are legitimate choices rather than three fixed mandatory targets.
- The same state includes the quest-owned Fellhollow Vannaka marker because the
  text and flow combine hunting with the three-heart hand-in.
- The pre-start and completed-heart states both resolve only to that quest-owned
  Vannaka marker.

Semantic references: <https://gegebase.com/games/dragonwilds/the_wild_hunt>
and <https://www.dragonwildsguide.wiki/unlock/runescape-dragonwilds-how-to-get-mount>.

### A Melody Remembered

- Both objectives advance at the same unique quest-owned Jewellery Box. The
  first state previously emitted several duplicate representations of that one
  interaction; it is now collapsed to its exact registry identity.
- The step annotations preserve the otherwise easy-to-miss item distinction:
  first an Inspiring Ring of Runecrafting, then the standard Amulet of Magic,
  not the Inspiring Amulet of Magic.

Semantic references: <https://www.gamingtoolkits.com/runescape-dragonwilds/quests/a-melody-remembered>
and <https://www.youtube.com/watch?v=_wpB9QQQwkE>.

### A Room with a Garou

- The only objective is construction around Caratacus the Elder. The exact
  Caratacus registry identity is published as an area destination because the
  player chooses the individual wall and roof snap points.
- This is intentionally not converted into a point interaction marker: quest
  completion validates the home built around the elder's site.

Semantic references: <https://allthings.how/how-to-complete-a-room-with-a-garou-in-runescape-dragonwilds/>
and <https://gamever.io/knowledge-base/a-room-with-a-garou-in-dragonwilds-four-walls-one-roof-twenty-logs>.

### Animal Magnetism

- The opening state correctly uses the exact Advert-Inna-Bottle interaction,
  followed by the unique Manktongue actor.
- The ingredient state now distinguishes all four progression locations: the
  exact Undead Chicken, the exact quest twig bundle, the placed
  `BP_NPC_Goblin_Smith` actor for Pulpknuckle's magnetised iron bar, and
  Manktongue for the component hand-in.
- The earlier catalog used Manktongue's nearby coordinate as a combined proxy
  for Pulpknuckle and the return. That obscured which NPC actually performs the
  magnetisation even though Pulpknuckle has his own cooked actor upstairs.

Semantic reference: <https://gamesfuze.com/guides/runescape-dragonwilds-animal-magnetism-quest-guide-avas-accumulator/>.

### Doric's Quest — Undead Ranger's Bow

- `FindSkeletonShortbow` previously expanded the revealed `DoricNPC` name into
  numerous Doric phase copies. Those copies identify neither the recipe source
  nor distinct valid destinations.
- Multiple walkthroughs agree that the Undead Ranger's Bow recipe is obtained
  from a Dragonkin Effigy inside Skekven Kara. The marker therefore uses the
  current build's exact Skekven Kara overworld entrance under the dungeon
  policy.
- Doric's quest-owned `DoricNPC` marker is retained separately because the
  combined objective also requires crafting the bow and bringing it back to
  him. No interior coordinate has been fabricated.

Semantic references:
<https://nerdschalk.com/how-to-get-the-undead-rangers-bow-in-runescape-dragonwilds-dorics-quest-recipe-and-crafting/>,
<https://www.gamingtoolkits.com/runescape-dragonwilds/quests/dorics-quest>, and
<https://gegebase.com/games/dragonwilds/doric%27s_quest>.

### Withering Heights — full state-bound route

- The opening route is now explicitly state-bound: Wise Old Man at
  `WomGFExitTarget`, Ominous Door at `EnterDeathsDomainTarget`, Death at
  `DeathsTempleArea`, the exact Forgotten Temple Postie Pete, the Fellhollow
  crossing at `EnterFHArea`, and the first pool at `CleansingPoolTarget`.
- Each beat hides its predecessor before revealing its successor. Retaining
  hidden locations produced plausible-looking but stale directions to the
  previous NPC or doorway.
- Cathan and all three exact registry stones remain together for the initial
  investigation. The 0/3, 1/3, and 2/3 counter states retain the same stones
  because the aggregate count does not expose which individual clues remain.
- Fragment objectives retain all three freely ordered destinations: Emberwood
  Village, the Vekchenven Kara entrance for the interior rage fragment, and
  the Silverthorn Keep entrance for the interior despair fragment.
- Once those fragments are complete, all three are removed. `SlayImaruTarget`
  alone identifies the Shattered Spire fight; `DeathTempleTarget` alone
  identifies the final head hand-in.

Semantic references:
<https://dragonwildswiki.com/guides/quests/withering-heights/>,
<https://www.dragonwildsguide.wiki/walkthrough/runescape-dragonwilds-withering-heights>, and
<https://www.gamingtoolkits.com/runescape-dragonwilds/quests/withering-heights>.

### Final uncovered flow states: Regicide and Icthlarin's Little Helper

- Regicide's `AccessSW` state reveals one exact `SW_Door` location. This is the
  Dragon Gate transition into the Scorned Wilderness, not the later Hub or Raid
  contact.
- Icthlarin's opening `EnterUmS` state reveals
  `Bastion_DoorToUmS_Target`, matching the door inside the Bastion boss room.
- On crossing it, `LookForLife` explicitly hides that door and reveals the
  8,000-unit `GarouCamp_A_Area`. The map therefore switches to the Alcarrid
  camp search area instead of leaving a stale marker at the region entrance.

Semantic references:
<https://games.gg/runescape-dragonwilds/guides/runescape-dragonwilds-regicide-quest-complete-walkthrough/>,
<https://metaforge.app/runescape-dragonwilds/icthlarins-little-helper-quest-guide>, and
<https://gegebase.com/games/dragonwilds/icthlarin%27s_little_helper>.

### The Great Body Robbery — lightning without a false strike point

- `Objective5` combines two different location semantics: a dynamic lightning
  strike and a fixed return to Doctor Fenkenstrain.
- The vial fills when the player is struck while carrying it. Stormtouched
  Highlands is the reliable recommendation, but thunderstorms in other regions
  can also satisfy the step. Consequently, the game has no truthful single
  lightning coordinate to extract or publish.
- The prior audit represented this correctly as dynamic but emitted no marker,
  leaving the objective at Low confidence and omitting its explicit hand-in.
  It now retains the dynamic explanation and marks the quest-owned
  `Fenkenstrain` laboratory target for the return portion.

Semantic references:
<https://www.dragonwildsguide.wiki/walkthrough/runescape-dragonwilds-the-great-body-robbery>,
<https://www.dragonwildswiki.wiki/quests/Dragonwilds-the-great-body-robbery>, and
<https://www.gamingtoolkits.com/runescape-dragonwilds/quests/the-great-body-robbery>.

## Residual confidence review

After the state-bound pass, the generated audit contains no Low-confidence
steps. The remaining 18 Medium-confidence rows are intentionally conservative:

- **Dragon Slayer (1):** any ordinary deer can supply the antler. The catalog
  shows every current standard-deer spawn plus the exact Wise Old Man hand-in;
  the save objective does not identify a preferred deer.
- **Black Knight's Fortress (13):** the 0/13 through 12/13 objectives store an
  aggregate shard count. All thirteen exact authored shard areas remain valid,
  but the character progress structure does not say which identities were
  already collected. Hiding markers by count would therefore guess.
- **Biohazard (1):** ten samples are renewable drops from current Corrupted
  Raven spawns. All valid current spawns and the Zanik hand-in remain visible.
- **Wanted! (2):** the 1/3 and 2/3 journal states expose a count, not the
  identity of the pages already read. All three exact journal interactions are
  retained.
- **Things That Go Boom In The Night (1):** ten skulls are renewable drops
  from Spectral Chinchompa spawns. The catalog shows the current spawn set and
  Vannaka hand-in rather than choosing an arbitrary animal.

These ratings express runtime identity ambiguity, not coordinate uncertainty.
Every displayed fixed interaction in these rows comes from the current game
build. Raising them to High would require a proven per-character identity bitset
or equivalent runtime state, not additional wiki interpretation.

## Cross-quest shared-coordinate review

The generated audit currently contains five coordinate groups used by more
than one quest. Each was checked and is intentional:

- Manktongue is shared by Animal Magnetism and Cook's Assistant.
- The Forgotten Temple Postie Pete is shared by Withering Heights and Letters
  For The Dead.
- Vekchenven Kara's entrance is shared by the Withering Heights rage fragment
  and two Doric's Quest recipe routes whose interactions occur inside it.
- Skekven Kara's entrance is shared by Doric's Quest's Undead Ranger's Bow
  recipe and Seeking Salvation's interior dwarven writing.
- Anamcara's Alcarrid camp placement is shared by both Mapping the Sands
  hand-ins.

These are common physical destinations with different quest semantics, not
evidence that unrelated steps were merged. Tooltip labels remain step-specific
even when the marker coordinates are identical.
