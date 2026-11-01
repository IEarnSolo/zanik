# Dragonwilds game updates and Zanik maintenance mode

Zanik Manager separates checking for a game update from installing it. **Check game** is read-only. **Update Dragonwilds server** appears only after Manager verifies a newer Linux dedicated-server build and always requires one confirmation for the selected server.

## Wrapper mode

Wrapper mode leaves the official executable untouched. `AUTO_UPDATE` may remain enabled, so Pterodactyl can update Dragonwilds normally and then invoke `zanik-server`. If an operator disabled automatic updates, Manager temporarily enables the variable for a confirmed update and restores the prior value afterward.

## Replace-executable mode

Replacement mode exists for eggs with a hard-coded executable path that the Client API cannot rewrite. Manager preserves the official executable as `.zanik-original`; therefore normal Pterodactyl automatic updates must remain disabled. Installation is blocked unless Manager can use `startup.read` and `startup.update` to set and verify `AUTO_UPDATE=0`.

For a confirmed update, Manager stops the server, journals and backs up the launcher and official binary, installs a bounded one-shot updater at the fixed path, and starts it. The updater runs SteamCMD, moves the new official ELF to `.zanik-original`, restores the verified launcher, and records a non-secret result. It fingerprints the game save tree before and after and never deletes, restores, or edits world saves.

## Unsupported builds

The launcher hashes the official executable before Zanik initializes. A reviewed hash starts the normal mod. An unknown hash starts the official game with Zanik's minimal maintenance-only HTTP service:

- no engine discovery, hooks, scanners, dispatchers, native persistence, or periodic game work;
- browser routes show one maintenance page;
- `/api/update-status` reports the safe state;
- other Zanik APIs return `503`;
- a bounded background check reports whether the latest stable GitHub release supports that executable.

Gameplay remains available while the operator waits. If a compatible release is available, update Manager and then use **Update / repair Zanik**. Website assets need refreshing only when that release reports a changed asset extraction contract.

## Named-storage update gate

Named-storage compatibility is a required game-update gate, not an optional UI check. For every new dedicated-server build:

1. Run the compatibility analyzer and require the rename getter, setter-identity getter, setter, native value lifetimes, renameable-interface gate, and authoritative server caller/argument-order checks to pass.
2. On the development server, select a chest type already known to support in-game naming, record its original name, assign a unique temporary name through the website, and verify that the website and game both show it. Do not use a generic inventory building such as a crate merely because it appears in the storage list.
3. Restore the original name through the website and verify the restored value in both places. Do not promote the build while either readback fails.

Every rename request uses the authoritative player-controller server path and verifies the authoritative interface readback. A storage type that does not retain names fails only that request; ABI validation or a slow native call can still circuit-break storage naming while leaving read-only storage and independently compatible mutations available for diagnosis.

## Retired-gravestone restore gate

Retired gravestones restore item copies directly into a live player inventory; they never try to reopen or mutate the removed in-game gravestone. Each new snapshot retains the exact Unreal `ItemData` asset key and also retains `PersistenceID` when the current build exposes it. Resolution after a game update accepts either identity, with the asset key as the required durable fallback, while the compatibility analyzer must continue to validate `UInventoryComponent::AddItemByData` and its capacity-check ABI.

For every new dedicated-server build, create and retire a development-server gravestone containing at least two different items, restart the server, and restore one item and then all items to a live player. Verify the inventory changes in game, verify the retired record remains on the website, and do not promote the build if either identity resolution or native insertion fails. Pre-retirement legacy snapshots are intentionally discarded because they did not preserve a trustworthy, reusable item identity.

## Home teleport update gate

Player Overview pages obtain Home teleport destinations from Dragonwilds' authoritative `GetPlayerBedRespawnLocationAndRotation` path. The runtime invokes its reflected wrapper only on the game thread, caches only a successfully returned finite location, and sends administrator requests through the already validated delayed location-teleport transport. It does not initialize or cast the spell and does not charge runes.

For every new dedicated-server build, require the compatibility analyzer to find the exact bed-respawn accessor and validate its native entry signature. On the development server, log in with a character that has a bed, verify the Home card marker matches the bed/base location, invoke the website button from a valid admin session, and verify the linked admin arrives there without rune consumption. Also verify that a non-admin session sees no teleport control and that a character without a bed reports no home instead of receiving a fallback location.
