# Dragonwilds game updates and Zanik maintenance mode

Zanik Manager separates checking for a game update from installing it. **Check game** is read-only. **Update Dragonwilds server** appears only after Manager verifies a newer Linux dedicated-server build and always requires one confirmation for the selected server and exact expected build. All paths derive from that profile's validated `SERVER_ROOT`.

## Capability inventory and reports

`deploy/compatibility-capabilities.json` is the authoritative inventory of independently breakable native, operational, persistence, authorization, and asset capabilities. Each entry maps its static evidence to `/api/update-status`, a detailed diagnostics endpoint, persistence and asset dependencies, tests, and the narrow boundary disabled on failure.

After validating an exact candidate, run `tools/generate_compatibility_report.py` against that candidate's saved `/api/update-status` response. The tool fails when a required field is absent or incompatible, emits authoritative JSON, and derives the Markdown report from the same records. Reports must identify the game build, executable hash, candidate or payload identity, server-payload contract, and asset contract. A runtime check that needs an online player or streamed object remains `pending`; absence of evidence is never rewritten as compatible.

Native analyzer tests include real altered-byte and truncated-signature fixtures. Operational negative tests cover malformed persistence and unavailable write destinations. Source-code assertions remain useful architecture guards but are not sufficient negative compatibility evidence by themselves.

## Research preflight and timing

Every compatibility workflow begins with fresh public build discovery for both the Linux dedicated server and licensed Windows client. `tools/verify_wsl_research.py` compares those results with the WSL client manifest, the staged server executable fingerprint, the current licensed-client manifest, and the hashes of every client research input selected for analysis. A stale or partially copied reference fails closed. Refreshes are staged into a task-owned sibling, verified, and retained atomically; they never delete or overwrite the last reviewed reference in place.

`tools/workflow_timing.py` maintains the task-scoped ignored wall-clock ledger. Zanik Manager journals additionally record bounded start, finish, duration, and outcome data for every install and game-update progress phase. Interrupted operations retain their active phase; successful and failed exits close it explicitly. Legacy journals without timing fields remain readable.

Development-only executable fingerprints use `deploy/validate-dev-update.ps1 -ValidationMode Candidate`, which accepts only the explicit candidate override combination. Promotion removes the candidate marker and must pass `-ValidationMode Production`, which requires production compatibility with no override. Candidate success is never production evidence.

## Wrapper mode

Wrapper mode leaves the official executable untouched. `AUTO_UPDATE` may remain enabled, so Pterodactyl can update Dragonwilds normally and then invoke `zanik-server`. A confirmed Manager update requires `startup.read`; `startup.update` is required only when the current value is `0`. Manager then temporarily enables the variable, reads it back, and restores and verifies the prior value afterward.

## Replace-executable mode

Replacement mode exists for eggs with a hard-coded executable path that the Client API cannot rewrite. Manager preserves the official executable as `.zanik-original`; therefore normal Pterodactyl automatic updates must remain disabled. Installation is blocked unless Manager can use `startup.read` and `startup.update` to set and verify `AUTO_UPDATE=0`.

For a confirmed update, Manager stops the server, creates a recognized recovery point, and installs a bounded one-shot updater at the fixed path. One operation ID binds the local journal, expected build, recovery manifest, exact launcher hash, request, result, and cleanup. The updater runs SteamCMD, moves the new verified ELF to `.zanik-original`, restores the exact hash-verified launcher, and records a non-secret operation-bound result. It fingerprints the complete save-file tree before and after and never copies, deletes, restores, or edits world saves. A changed fingerprint or mismatched transaction leaves the server stopped with recovery evidence retained.

Before Manager marks the update complete or restarts gameplay, it verifies the expected build, executable fingerprint, restored launcher bytes, and removal of the exact transient request, result, and staged-launcher files. Failed or interrupted operations retain only the transaction material required for Manager recovery and identify its paths. Recovery-point retention remains Manager-owned.

## Unsupported builds

The launcher hashes the official executable before Zanik initializes. A reviewed hash starts the normal mod. An unknown hash starts the official game with Zanik's minimal maintenance-only HTTP service:

- no engine discovery, hooks, scanners, dispatchers, native persistence, or periodic game work;
- browser routes show one maintenance page;
- `/api/update-status` reports the safe state;
- other Zanik APIs return `503`;
- a bounded background check reports whether the latest stable GitHub release supports that executable.

The official game remains available while the operator waits. Maintenance mode may load Zanik's library only for this minimal HTTP boundary and release checker; it initializes no engine integration. It is distinct from `ZANIK_ENABLED=false`, which loads no Zanik library and serves no website. If a compatible release is available, install its reviewed server payload through **Update / repair Zanik**. Update Manager only when its own or embedded-payload contract requires it, and refresh website assets only when their independent source or extraction contract requires it.

The end-user update path never runs the compatibility analyzer, generates signatures, modifies the compatibility manifest, or treats a freshly analyzed executable as reviewed. Research and full feature/performance validation happen separately on an explicitly authorized development target and ship only in a validated release.

## Named-storage update gate

Named-storage compatibility is a required game-update gate, not an optional UI check. For every new dedicated-server build:

1. Run the compatibility analyzer and require the rename getter, setter-identity getter, setter, native value lifetimes, renameable-interface gate, and authoritative server caller/argument-order checks to pass.
2. On the development server, select a chest type already known to support in-game naming, record its original name, assign a unique temporary name through the website, and verify that the website and game both show it. Do not use a generic inventory building such as a crate merely because it appears in the storage list.
3. Restore the original name through the website and verify the restored value in both places. Do not promote the build while either readback fails.

Every rename request uses the authoritative player-controller server path and verifies the authoritative interface readback. A storage type that does not retain names fails only that request; ABI validation or a slow native call can still circuit-break storage naming while leaving read-only storage and independently compatible mutations available for diagnosis.

## Region-discovery update gate

Recent Activity follows Dragonwilds' authoritative `LogMapRegions` first-discovery record from the active dedicated-server log. It starts at the end of the current log, matches the controller name in each newly appended record to a live character, and never backfills older discoveries.

For every new dedicated-server build, require the compatibility analyzer to find both the `LogMapRegions` category and the exact `Revealing map region [%s] (Region %d) for player [%s]` format contract. On the development server, enter one genuinely fogged region and verify exactly one Recent Activity entry appears within the shared two-second refresh interval. Return to that region and reconnect the character to confirm neither action creates a duplicate. Do not promote a build if the log contract or live test fails.

## Player-network diagnostics update gate

Player latency monitoring reads Unreal's authoritative compressed ping from the already cached two-second player-status snapshot. It never adds a game-thread scan. The service worker retains per-player history, ignores the first ten connected seconds, and requires three consecutive elevated samples before reporting an incident. Linux interface throughput, error, and drop deltas come from `/proc/net/dev`; direct client packet loss remains explicitly unavailable unless a future reviewed game ABI exposes it.

For every new dedicated-server build, require the compatibility analyzer to retain the `CompressedPing` reflection contract and, when the exact `APlayerState::GetPingInMilliseconds() const` symbol is present, verify that its compiled layout still references offset `0x2C8`. On a development server, reconnect one player and confirm login warm-up samples do not produce an incident, then verify the per-player row, jitter history, and host-interface counters update. Do not silently reinterpret missing or implausible ping data as zero, and do not promote a build if the authoritative field contract fails.

## Game-thread attribution update gate

The bounded timing trace normally records Zanik follow-up work. The reviewed inventory exception times Dragonwilds' original `MoveItem` and `RemoveFromSlot` calls so an in-game storage or gravestone delay can be separated from Zanik's own capture overhead. Those hooks copy only source, destination, initiating-controller, result, and timing evidence into a fixed 128-slot non-blocking ring; player matching and object classification happen later on the service worker. Zanik-dispatched website grave mutations are suppressed from this native observer because their website request already carries the authoritative administrator identity. Other player-triggered native events and website mutations must carry their authoritative character or public ID through the existing fixed-size event/request path. Engine/background work may have no initiator; diagnostics must report that attribution gap and must not substitute a storage owner, grave owner, or nearby player unless that identity is the actual initiating argument.

Storage autofill additionally observes the authoritative reflected inventory-controller RPCs for full-stack movement, split-and-put withdrawal, and in-place stack splitting. The shared `ProcessEvent` hook records only the source and destination inventory pointers after the game call returns, then the service worker validates live object identities and storage ownership before prioritizing a single-container refresh. Reflected parameter offsets are discovered at runtime; a fixed layout fallback is allowed only for the exact reviewed executable. If the RPCs, reflection layout, shared hook, or bounded queue are unavailable, only the immediate trigger is disabled and the 500 ms autofill reconciliation fallback remains active.

Autofill latency diagnostics preserve monotonic timestamps through the bounded capture event, service-thread drain, targeted reconciliation, game-thread dispatch, and successful refill. `/api/update-status` exposes last and maximum durations for each queue boundary plus end-to-end capture-to-refill time, while `/api/diagnostics/storage-autofill-latency` provides the same focused measurements. These values diagnose scheduling delay only; they never relax the game-thread execution budget or expose object addresses, storage identities, or player data.

After a successful slot-exact autofill, Zanik resolves the reflected `AActor::FlushNetDormancy` and `AActor::ForceNetUpdate` functions and invokes them on the identity-verified owning storage actor through the shared reviewed `ProcessEvent` trampoline. This wakes a dormant storage and requests an immediate replication pass only after the authoritative mutation and normalization have completed. Missing reflection functions, a stale actor identity, or an unavailable trampoline disables only the replication acceleration; autofill and Unreal's ordinary replication continue. `/api/diagnostics/storage-autofill-replication` reports compatibility, calls, failures, and maximum game-thread execution time without exposing object identities.

For every new dedicated-server build, keep the existing kill, gravestone lifecycle, resource collection, teleport, storage mutation, native inventory-mutation attribution, and shared-dispatcher compatibility checks fail-closed. Static analysis must validate complete instruction boundaries for both inventory trampolines. On a development server, perform one action through each available game and website path, confirm the expected feature/operation/player appears when its measured work crosses the diagnostic threshold, and confirm trace contention or overflow increments the bounded drop counter without blocking the game thread. Any changed hook signature or identity path requires corresponding compatibility-analyzer evidence before promotion. After live validation, add the exact executable to the shared storage production-review policy, rerun the analyzer with `--require-storage-production`, and require `adminStorageProductionCompatibility=1` plus `adminStorageCandidateOverride=false` in the development report. A candidate marker is research evidence, never proof that an ordinary release enables the capability.

## Retired-gravestone restore gate

Retired gravestones restore item copies directly into a live player inventory; they never try to reopen or mutate the removed in-game gravestone. Each new snapshot retains the exact Unreal `ItemData` asset key and also retains `PersistenceID` when the current build exposes it. Resolution after a game update accepts either identity, with the asset key as the required durable fallback, while the compatibility analyzer must continue to validate `UInventoryComponent::AddItemByData` and its capacity-check ABI.

For every new dedicated-server build, create and retire a development-server gravestone containing at least two different items, restart the server, and restore one item and then all items to a live player. Verify the inventory changes in game, verify the retired record remains on the website, and do not promote the build if either identity resolution or native insertion fails. Pre-retirement legacy snapshots are intentionally discarded because they did not preserve a trustworthy, reusable item identity.

## Home teleport update gate

Player Overview pages obtain Home teleport destinations from Dragonwilds' authoritative `GetPlayerBedRespawnLocationAndRotation` path. The runtime invokes its reflected wrapper only on the game thread, caches only a successfully returned finite location, and sends administrator requests through the already validated delayed location-teleport transport. It does not initialize or cast the spell and does not charge runes.

For every new dedicated-server build, require the compatibility analyzer to find the exact bed-respawn accessor and validate its native entry signature. On the development server, log in with a character that has a bed, verify the Home card marker matches the bed/base location, invoke the website button from a valid admin session, and verify the linked admin arrives there without rune consumption. Also verify that a non-admin session sees no teleport control and that a character without a bed reports no home instead of receiving a fallback location.

Saved home locations use a strict, bounded persistence parser and atomic `0600` writes. Read, permission, malformed-record, and write failures are reported independently from the native bed-location compatibility gate. A persistence failure may disable restart guarantees without hiding a currently captured live destination; it must appear in `/api/update-status`, `/api/player-home`, and admin diagnostics.

## Quest notification update gate

Quest tracking time-slices state queries and uses a bounded notification queue for tracked-quest changes. The compatibility analyzer must validate the exact complete instruction sequence patched at `UObject::ProcessEvent+14`; runtime installation validates the same bytes again before arming. Quest state, objective text, tracked-quest notification, dispatcher, catalog, and persistence results remain separate status values.

For each new build, verify a real quest step advancement, tracked-quest selection, objective text refresh, persistence across restart, zero queue drops, and correct two-second website refresh. Mutated and truncated hook-signature fixtures must fail before runtime installation.
