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
