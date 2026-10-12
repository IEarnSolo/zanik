# Dragonwilds resource respawn times

These durations come from the authoritative respawn profiles attached to the non-farming world resource placements. Dragonwilds advances game time at a nominal **60 game seconds per real second**, so one in-game hour takes one real minute. The live map continues to use the server's measured game-time rate for its active countdown.

| Resources | In-game time | Real-life time |
|---|---:|---:|
| Ash Logs; Stone | 12.0 hours | 00:12:00 |
| Animal Bone; Bittercap Mushroom; Marrentill; Swamp Tar | 1.0 day | 00:24:00 |
| Anima-infused Bark; Swamp Weed | 2.0 days | 00:48:00 |
| Cabbage; Cadavaberries; Dwellberries; Flax; Harralander; Onion; Potato; Pumpkin; Redberries; Salvage Metal; Snapdragon; Toadflax | 3.0 days | 01:12:00 |

## Runtime default

Some older placements have an empty data-table row handle:

- Animal Bone (7 placements)
- Bittercap Mushroom (207 legacy placements)
- Marrentill (120 placements)

They still respawn because the native interval resolver asks the server's in-game time provider for its fallback when the row handle is empty. Static call-chain analysis and a live legacy `BP_Spawner_Mushroom` component both confirmed that fallback is exactly **24 in-game hours** on executable `d26de80f…3b259`. The component then schedules the normal respawn timer. These placements are recorded as `RUNTIME_DEFAULT_24H` so saved-state countdowns use the same duration.

Bittercap Mushroom uses two configurations: 68 explicitly select `FLORA_VEGETABLES_24H`, while 207 legacy placements use the runtime 24-hour default. Salvage Metal inherits `SALVAGE_72H` from its base blueprint; the catalog generator follows that archetype inheritance.
