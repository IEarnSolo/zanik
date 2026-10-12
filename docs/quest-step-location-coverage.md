# Quest step location coverage

Catalog fingerprint: `5b89d5a15671b69c53aa4d7d521eea68681474d957302aedf36c87a47f72f050`

This report covers 41 quests and 230 reachable player objectives. 2 blank objective-table records that are never referenced by compiled quest flow remain in the internal analysis but are excluded from the public catalog. A mapped objective has an exact quest-location link, a quest-scoped world-registry landmark, or a dungeon identity linked to its authored overworld entrance. Wiki coordinates are never accepted.

The current build contains 16 verified overworld dungeon entrances. Interior steps use those entrances and retain the interior target name as marker metadata.

| Quest | Objectives | Mapped | Missing |
|---|---:|---:|---:|
| Dragon Slayer | 11 | 11 | 0 |
| Goblin Diplomacy | 7 | 7 | 0 |
| Highlighting the Problem | 4 | 4 | 0 |
| Restless Ghosts | 2 | 2 | 0 |
| Scarecrow Scavenging | 0 | 0 | 0 |
| The Great Body Robbery | 13 | 13 | 0 |
| Black Knight's Fortress | 22 | 22 | 0 |
| Biohazard | 5 | 5 | 0 |
| Wanted! | 5 | 5 | 0 |
| What's Theirs is Mine | 4 | 4 | 0 |
| Withering Heights | 16 | 16 | 0 |
| Doric's Quest | 4 | 4 | 0 |
| Even More Restless Ghosts | 5 | 5 | 0 |
| Letters For The Dead | 5 | 5 | 0 |
| Mirror Mirror | 3 | 3 | 0 |
| Seeking Salvation | 3 | 3 | 0 |
| The Wild Hunt | 3 | 3 | 0 |
| Things That Go Boom In The Night | 2 | 2 | 0 |
| What Remains is Written | 7 | 7 | 0 |
| A Melody Remembered | 2 | 2 | 0 |
| A Room with a Garou | 1 | 1 | 0 |
| Animal Magnetism | 4 | 4 | 0 |
| Cook's Assistant | 5 | 5 | 0 |
| Dog Days | 6 | 6 | 0 |
| Granite Mauled | 3 | 3 | 0 |
| Heartstrings | 1 | 1 | 0 |
| Regicide | 5 | 5 | 0 |
| Icthlarin's Little Helper | 18 | 18 | 0 |
| Beast in the Sand | 3 | 3 | 0 |
| Brink of Extinction | 9 | 9 | 0 |
| Contact! | 10 | 10 | 0 |
| Mapping the Sands I | 6 | 6 | 0 |
| Mapping the Sands II | 10 | 10 | 0 |
| Mark Day | 4 | 4 | 0 |
| Rag and Bone Garou | 1 | 1 | 0 |
| Rogue Trader | 2 | 2 | 0 |
| Saga of the Adventurer | 6 | 6 | 0 |
| Scaling Difficulties | 3 | 3 | 0 |
| Song of the Ancients | 4 | 4 | 0 |
| The Dig Site | 5 | 5 | 0 |
| The Mirror Crack'd | 1 | 1 | 0 |

## Missing objective evidence


## Safety behavior

- Active objectives use only exact authored markers from this dataset.
- An active objective without a verified marker displays no destination marker. It never falls back to the quest start point.
- Unstarted and completed quests may continue to show the separately verified quest-start location.
- Interior objectives require an independently verified overworld entrance before they can be marked on the world map.
- The runtime joins progress to this catalog by current objective text; mismatches fail closed.
