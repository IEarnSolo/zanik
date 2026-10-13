# Hosting compatibility

## Pterodactyl

Upload the server ZIP and extract it to `/home/container/zanik-release/VERSION/payload/`. For wrapper mode run its `scripts/install.sh` and set the egg startup command to `/home/container/zanik/bin/zanik-server --server-root /home/container --` plus the normal game arguments.

If the egg fixes the original executable path, run the acknowledged replacement command in [installation.md](installation.md). This is supported for operator convenience but discouraged. Generated assets are activated under `/home/container/zanik/assets` via `init-assets.sh`; persistent state belongs in `/home/container/zanik-data`.

Every host must provide a dedicated TCP allocation for Zanik's website/API in addition to the Dragonwilds game port. Configure the internal listener with `STATUS_PORT` and the user-facing address with `PUBLIC_BASE_URL`.

## Docker and generic Linux hosts

Mount the game root read/write only if using replacement mode; wrapper mode can keep game content read-only except the normal save paths. Keep `zanik-data` on a persistent volume, expose the configured TCP status port, and launch `zanik-server` as the game process. Linux x86-64 is the supported runtime.

Self-hosted Linux operators should use the `.tar.gz` server package and the versioned staging layout in [linux.md](linux.md). Complete web assets must be generated from a licensed Windows client and transferred using the [self-hosted asset workflow](assets.md#install-on-a-self-hosted-linux-server).

Hosting providers need shell/file access, a configurable command or executable path, persistent storage, and one TCP allocation. CurseForge supplies manual downloads; it does not remotely install the server.
