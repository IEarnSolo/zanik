# Public live NPC map

The NPC map is a server-side public-map capability. Set `DW_NPC_TRACKING_ENABLED=0` to disable it independently. It does not use an admin session.

`tools/generate_npc_catalog.js` joins cooked `AISpawnPoint` defaults (including recursively inherited plugin subclasses), per-placement spawn/respawn overrides, AI character `AIDataClass`, World/Factions and World/Fauna journal entries, Knowledge/People entries, directly placed skeletal dialogue characters, and world-partition actor transforms. Journal data is authoritative for readable names and portrait icons because some character blueprints retain prototype names. Metadata discovery covers the base game plus feature packages such as Dowdun Reach, Umbral Sands, Scorned Wilderness, and FutureMajorVersion. It emits a public catalog plus private runtime identities and exclusions. IDs are hashes of source package and persistent actor identity, so duplicate named characters remain distinct. A placement with `bShouldRespawn=false` never inherits or advertises its blueprint's default respawn duration.

`tools/build_npc_icons.ps1` reproducibly copies every portrait referenced by World/Factions, World/Fauna, and Knowledge/People from the extracted cooked textures. Missing portraits are reported and fail visibility closed to the legacy colored marker. Live markers render the portrait over a white circular backing, inactive markers retain an amber backing, and dead portraits are faded.

NPC diagnostics and `/api/update-status` expose a separate icon compatibility gate. It validates every icon URL referenced by the deployed catalog and reports missing assets without disabling NPC state tracking or the rest of the public map.

Dungeon/instance/test packages, dungeon ancestry, invalid transforms, and out-of-map placements are excluded. Runtime dungeon lookup is a second fail-closed guard. Spawned AI actors are matched by exact character class and nearest unique catalog placement within a bounded leash. Direct quest NPCs use their persistent placed identity. An authored spawn point that is itself loaded but has no matched NPC is `inactive`; `unloaded` is reserved for a placement whose spawn point is not loaded. Only a correlated `OnAIKilled` event creates a respawn countdown.

Compatibility is split across catalog, registry scanning, identity matching, transforms, lifecycle capture, respawn timing, and dungeon exclusion. Static catalog markers remain usable when live state is unavailable. `/api/diagnostics/npcs` contains counts and gates but no addresses or pointers.

## Unloaded respawns

Current-build native analysis confirms that NPC respawn eligibility is owned by
the global `AAiDirector`, rather than by the streamed NPC actor. On death the
director associates the spawn-point `FGuid` with an `FAIRespawnTimer` whose
only reflected field is the absolute `InGameTimeOfRespawn`, while the same GUID
is retained in `ExhaustedSpawnPointIds`. `HandleInGameTimeChanged` delegates to
`RefreshRespawnTimers`; an overdue entry calls `ReplenishSpawnPoint`, removes
the timer and exhausted state, and makes the placement eligible again. This
also handles an authoritative game-time jump after an idle period or restart.

Eligibility and actor existence are deliberately separate. The director's
`TrySpawningAIForSpawnPoints` path evaluates spawn requests around players, and
World Partition may leave a distant spawn point and its AI actor unloaded.
Consequently the map reports an elapsed but unloaded death as `ready`, not
`live`: the game has replenished or is due to replenish the placement, but the
actor is only confirmed after the area streams and a live AI is observed.
Ordinary server logs do not emit successful AI replenishment entries, so this
state comes from the authoritative game-time lifecycle and live-object scan,
not from an assumed log message.

Identity-catalog schema 2 retains each spawn point's native `FGuid`. When that
catalog is available, Zanik reconciles the director's bounded, reflected
`RespawnTimerSpawnPointIds` map on every completed scan. This restores an
in-progress death countdown after a Zanik or server restart and recovers a
death that occurred while the kill hook was unavailable. The container is
accepted only when its header is stable across the read, its limits are sane,
and every consumed GUID maps uniquely to a reviewed catalog placement. A
schema-1 catalog or an incompatible runtime layout fails closed and leaves the
existing observed-death lifecycle as the fallback.

Object discovery is incremental and resumable: a worker-loop slice is limited to 128 object slots and 400 microseconds, and completed generations are separated by a 500 ms idle interval so the server is not under perpetual discovery load. Object slots must be live and retain the same Unreal index/serial identity before relevant actor data is consumed. Runtime class ancestry is cached by class object, and dungeon exclusion reuses a validated gameplay-registry object instead of nesting another full object-array scan inside discovery. Previously matched live actors use a separate bounded transform-only refresh every two seconds, so movement remains current while background discovery progresses without starving HTTP requests.

NPC diagnostics report scan progress, generation wall time, accumulated scanner work, slice and finalization time, transform-refresh time, scanner duty cycle, class-cache effectiveness, rejected stale slots, identity changes, and the configured inter-generation idle interval. These metrics deliberately expose counts and durations only; they never publish native pointers or addresses.

## Pterodactyl destinations

- `dist/libdwstatus.so` → `/home/container/libdwstatus.so`
- `web/npc-catalog.json` → `/home/container/zanik/web/npc-catalog.json`
- `web/npcs/*.png` → `/home/container/zanik/web/npcs/*.png`
- `deploy/npc-identities.tsv` → `/home/container/zanik/npc-identities.tsv`

Compatibility validation treats the private identity catalog as a versioned artifact. Its declared node count and catalog fingerprint must be present, every opaque ID and persistent state key must be unique, and the parsed row count must match exactly. At runtime, the reflected `AActor.RootComponent` and `USceneComponent.RelativeLocation` offsets must also match the offsets used by the bounded scanner. A failure disables NPC tracking only and is reported by `/api/diagnostics/npcs` and `/api/update-status`.
- `deploy/npc-exclusions.tsv` → `/home/container/zanik/npc-exclusions.tsv` (private classification report)
- `deploy/zanik-bootstrap.sh` → `/home/container/zanik/zanik-bootstrap.sh`

Validate on development server `0f95b34b`. Production server `ef68362b` must not be changed without separate explicit authorization.
