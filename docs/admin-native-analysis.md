# Admin native integration analysis

## Development build

- Steam build: `25465077`
- Executable SHA-256: `d26de80fd01f4b73b726aed855a20377996b7c140ff33f1ec40150c0dd73b259`
- Server: `0f95b34b`

## Proven runtime metadata

The live reflected functions report these parameter buffer sizes:

| Function | Size |
| --- | ---: |
| `Client_ReceiveChatMessage` | 136 bytes |
| `Server_SendChatMessage` | 200 bytes |
| `Server_TryTeleport` | 32 bytes |

`FChatMessageData` is 136 bytes. `Server_TryTeleport` therefore contains two
16-byte GUID parameters as expected.

The `.sym` file records source-line addresses. These addresses can occur inside
inlined code and must not be treated as callable function entries. This was
confirmed by disassembling the records for chat validation, chat delivery,
`UObject::ProcessEvent`, and teleport success.

The native-looking pointer recovered from the reflected chat function points
into an alternate compiler entry that depends on preloaded callee-saved
registers. A normal C++ hook cannot safely forward through it. The nearby
read-only reference belongs to a mixed compiler table rather than a proven RPC
vtable slot.

## Current compatibility state

- Authorization storage: compatible
- HTTP claim exchange and sessions: compatible
- Pterodactyl console claim command: compatible
- In-game chat command interception: superseded by the console command
- Private client reply: not required by the console command
- Admin teleport execution: disabled

The console implementation avoids the unproven chat ABI. The permanent
bootstrap owns the server's standard input, consumes `zanik-admin`, calls the
authenticated loopback claim endpoint, and forwards all other input to the
game process.

Teleporting must remain gated until analysis proves:

1. a reliable way to schedule the HTTP request on the game thread;
2. a callable actor movement or teleport entry with its exact ABI;
3. the exact linked player controller and pawn relationship;
4. destination collision and replication behavior that cannot strand or
   desynchronize the character.

The later full lodestone sequence additionally requires the exact player
teleport component lookup, presentation path, and request-scoped cost bypass.

Directly writing actor or component coordinates is not permitted.

## Fixed nodestone map teleporting (build 25991736)

Fixed Dowdun Reach nodestones use the game's named-location system rather than
the player-built lodestone GUID network. The public map derives reviewed names,
world coordinates, and opaque IDs from paired `DR_*` and `DR_Nexus_*` target
records. Nexus markers are labelled by both their physical location and route,
for example `The Nexus · Garrison route`. Internal `FName` values and native
object identities are never serialized.

Authenticated map requests are prepared on the HTTP worker from a live paired
target and switchable-teleporter snapshot. The game-thread dispatcher validates
the actor index/serial identity and calls the reflected
`ASwitchableTeleporter::PerformTeleport(PlayerCharacter, TargetDestination)`
wrapper once. Static analysis requires both that wrapper and
`UTeleportationSubsystem::TeleportToNamedLocation`, and proves that both
converge on the reviewed `TeleportWithParams` implementation. Runtime
reflection independently requires `PlayerCharacter` at parameter offset `0x0`
and `TargetDestination` at `0x8`. This preserves the native wind-up, loading,
VFX, delayed movement, and completion callback instead of substituting a raw
coordinate teleport.

The capability has its own compatibility gate, counters, timeout handling, and
1 ms circuit breaker. A stale target, unloaded or changed teleporter, changed
reflection layout, duplicate request, timeout, or slow native initiation fails
closed. Read-only nodestone markers and their public location/coordinate
tooltips remain available when teleporting is unavailable. No placement-owner
history is inferred or tracked.
# Development lodestone teleport implementation (build 25630937)

The server now queues authenticated map requests and drains them from the exact
`ADominionPlayerCharacter::TickActor` game-thread path. The hook is bound to
executable SHA-256 `445f8b65047d208adb04a3cb03bc7706f069306d814172ae29caec16c15bec7e`
and a unique 16-byte prologue.

Static analysis proved the ordinary flow is
`Server_TryTeleport_Implementation` -> `TryTeleport` -> `CanTeleport`. It
requires distinct source and destination GUIDs, source proximity, affordability,
and deferred payment. `TryTeleport` resolves the destination through
`ULodestoneTeleportationComponent::GetTeleportationLocation`, then uses the
game teleport subsystem. The success RPC broadcasts the Blueprint event used by
the lodestone VFX and audio graph.

The direct `AActor::TeleportTo` fallback has been retired. It could move a pawn
before the destination World Partition cell was streamed, which made the fixed
Temple Woods lodestone unsafe. Calling `UTeleportationSubsystem::TeleportWithParams`
directly was also rejected: it omitted the callback and state assembled by the
lodestone component and produced the wrong transition while leaving the player
movement locked.

The implementation now calls the exact fingerprint-bound
`UBaseTeleportationComponent::TryTeleport(From, To)` entry. This function builds
the game's `FPlayerTeleportParams`, starts delayed streaming, and installs the
native completion callback that runs the normal client success presentation.
Its `CanTeleport` call is bypassed only on the game thread while the exact
component and exact source/destination GUID pair for one authenticated request
are active. Every other call forwards to the original function.

Rune charging is suppressed only on that player's teleport component. Runtime
reflection must resolve `bEnableTeleportationCost` at the statically proven
offset `0x110`; the old byte is restored after arrival or after a bounded
timeout. A unique prologue is required for `TryTeleport`, `CanTeleport`, and the
player tick hook, and all addresses remain bound to executable SHA-256
`445f8b65047d208adb04a3cb03bc7706f069306d814172ae29caec16c15bec7e`.
If any check fails, web teleporting is disabled rather than falling back to raw
movement.

## Storage location maps and teleporting

Authenticated storage responses project each live storage actor through the
same fingerprint-bound world-to-map transform as player and resource markers.
Only normalized browser coordinates are published. The actor's raw world
location remains server-side and is re-read from the live authoritative actor
when a teleport request reaches the game thread.

Storage teleporting first attempts an immediate placement when the chest is in
the player's nearby loaded area. It traces vertically on the game thread to
find the actual blocking floor, raises that surface by the character capsule
half-height, and requires `AActor::TeleportTo` to accept the complete capsule.
This nearby path does not invoke the fade or streaming sequence.

For a distant chest it uses the validated native lodestone `TryTeleport`
pipeline as a scoped World Partition preload. Static analysis proves that
`TryTeleport` synchronously copies the destination respawn vector into its
`FPlayerTeleportParams` before scheduling `TriggerDelayedTeleport`. Zanik
temporarily substitutes one validated registry record's respawn vector, calls
the native path with the existing request-scoped cost bypass, and immediately
restores the registry record. After the native delayed teleport reports arrival
Zanik re-resolves the chest and repeatedly uses the reflected
`UKismetSystemLibrary::LineTraceSingle` path as the collision-readiness probe.
Only a candidate with a real blocking floor and an accepted full-capsule
placement is used. Candidate rings start beyond the chest collision and sample
sixteen directions at increasing radii. There is no fixed settling delay: the
first authoritative safe result completes immediately. If no candidate becomes
safe during the bounded streaming window, the request is
rejected and the pawn is returned to its original position. Browser requests
still contain only the opaque chest ID.

After an update, the executable fingerprint, player tick hook, `TeleportTo`
entry, lodestone `TryTeleport`/`CanTeleport` entries, the fingerprint-bound
`FLodestoneData` size and respawn-vector layout, world-map transform, storage
ownership layout, root-component location layout, reflected line-trace
parameters and implementation,
and live actor resolution
must all pass before storage teleporting is enabled. Storage maps or teleport
controls report unavailable independently of read-only contents and item editing.

## Named storage grids and organization

Storage responses preserve native inventory slot indices, including empty
slots, and expose only opaque item revisions to the browser. The HTTP worker
builds one immutable, generation-bound snapshot for both serialization and
mutation planning. It reads the reflected `CustomName` `FText` property
directly, rather than polling `IRenameable::Execute_GetCustomName` from the
game thread, and records every actor, inventory, controller, item, and
item-data index/serial identity used by a mutation plan.

Website name changes are also executed on the game thread. Zanik builds a
native `FText`, resolves the storage object's authoritative setter identity
through `IRenameable::Execute_GetSetterID`, and calls
`IRenameable::Execute_SetCustomName`. On build `25630937`, the returned
`FUniqueNetIdRepl` occupies 48 bytes: the getter writes its final 16-byte field
at offset `0x20`. The runtime allocates and aligns the full 48-byte value, and
the compatibility analyzer verifies the exact getter and setter prologues so a
changed ABI disables storage mutation initialization. Both temporary native values are released
with their matching destructors so the normal replication and save path remains
owned by Dragonwilds.

Slot reordering, swapping, merging, and cross-container movement use the
game's inventory movement path. Copy and recipe operations are capacity
checked as complete plans, then decomposed into individual additions so no
unbounded native batch is executed in one tick.
Organize operations resolve the selected item types from opaque live slot
revisions, enumerate every authoritative storage owned by the page character,
and move every matching stack into the chosen storage. The complete aggregate
is capacity checked before the first move. Cross-container moves are recorded
and reversed in last-in-first-out order if any later native move fails.
Discovery, catalog lookup, allocation, sorting, capacity planning, and result
formatting occur on the HTTP worker. A fixed-capacity SPSC ring gives the game
thread stable job pointers; the dispatcher revalidates all captured identities
and executes at most one native action per eligible tick. Rollback is likewise
one action per tick. A 1 ms action trips that operation family's runtime circuit
breaker, and any stale identity or world change fails closed and invalidates
the snapshot. `POST /api/admin/storage/mutate` therefore returns `202` with a
session-bound opaque request ID, which the UI polls through the authenticated
status endpoint for up to five minutes.

The compatibility analyzer requires the internal move function,
renameable name executor, and `FText` destructor in addition to the existing
storage mutation symbols. If these dependencies move or change, organization
controls remain disabled while the read-only storage view stays available.
The analyzer also requires the rename setter, setter identity getter, native
`FText` constructor, and `FUniqueNetIdRepl` destructor.

## Admin compatibility closure

Build `25630937` adds analyzer and runtime gates for the native delayed
teleport family, the storage `ABaseBuildingActor::EndPlay` removal hook, and
the complete gravestone registry getter ABI. The gravestone check proves the
registry array remains at `AGameplayObjectRegistry + 0x2e0` and that copied
records remain `0x68` bytes before any registry-backed remote operation is
enabled.

Runtime diagnostics separate the teleport ground trace, reflected storage
teleport pipeline, delayed-teleport capture, storage dispatcher, batch
operations, organization, rename, removal tracking, gravestone inventory,
registry, streaming, respawn capture, and persistence paths. The web API uses
the relevant substatus when returning `503`, so a broken collision pipeline
does not misreport an ordinary lodestone request as incompatible and a broken
rename ABI does not need to disable read-only storage contents. Compatibility
metadata for the ABI is necessary but not sufficient: each reviewed game build
also requires a live setter/readback check against the reflected `CustomName`
property. Build 25991736 currently fails that functional check, so its rename
control is hidden and rename requests fail closed while all independently
validated storage operations remain available to the development candidate.

The update validator also exercises the non-native authorization boundary. It
creates a five-minute claim through the loopback console secret, redeems the
claim into a temporary cookie, verifies the session, proves an invalid CSRF
token is rejected, logs out with the valid token, proves that session is then
invalid, and verifies an unauthenticated storage catalog request returns
`401`. The temporary cookie file is created with restrictive permissions and
removed during the same validation attempt.

## Authoritative building recipe catalogue

Recipe mode uses the cooked `DA_BuildPieceCatalogue_Default` asset rather
than guessing sections from asset names. `tools/generate_building_catalog.py`
validates the current shape (8 tabs, 63 sections, 844 ordered references, and
827 unique runtime pieces), exports localized metadata and icons, and writes the
runtime index to `web/buildings/catalog.tsv`. The JSON companion is the
human-readable update artifact. Intentional General-page duplicates remain
menu memberships pointing to one recipe identity; Favourites remains empty.

At runtime only objects whose keys occur in that catalogue can become building
recipes. Each object must still pass the `BuildingPieceData` class check and
`GetScaledResourceRequirements` validation. A missing object, invalid item,
invalid quantity, missing icon, malformed catalogue, or changed requirements
ABI fails that piece or capability closed without disabling ordinary crafting
recipes. Storage diagnostics and `/api/update-status` report taxonomy, icon,
requirements, and crafting compatibility independently, together with
expected, resolved, and unavailable piece counts.
