# Hosting compatibility

## Pterodactyl

Upload the extracted server archive contents to `/home/container/zanik-release/`. For wrapper mode run its `scripts/install.sh` and set the egg startup command to `/home/container/zanik/bin/zanik-server --server-root /home/container --` plus the normal game arguments.

If the egg fixes the original executable path, run the acknowledged replacement command in [installation.md](installation.md). This is supported for operator convenience but discouraged. Generated assets are activated under `/home/container/zanik/assets` via `init-assets.sh`; persistent state belongs in `/home/container/zanik-data`.

## Docker and generic Linux hosts

Mount the game root read/write only if using replacement mode; wrapper mode can keep game content read-only except the normal save paths. Keep `zanik-data` on a persistent volume, expose the configured TCP status port, and launch `zanik-server` as the game process. Linux x86-64 is the supported runtime.

Hosting providers need shell/file access, a configurable command or executable path, persistent storage, and one TCP allocation. CurseForge supplies manual downloads; it does not remotely install the server.
