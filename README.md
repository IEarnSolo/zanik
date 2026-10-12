# Zanik

Zanik is a Linux x86-64 server-side mod for RuneScape: Dragonwilds. It exposes player, world, recipe, quest, resource, NPC, and map information through a web interface and provides separately gated administrative and quality-of-life features. Players do not install anything.

Zanik is a native `LD_PRELOAD` library. The recommended installation leaves the official game executable untouched and starts it through `zanik-server`. Executable replacement remains available only for hosting panels whose egg cannot change its startup command; it is explicitly non-recommended.

For Pterodactyl administrators on Windows, the recommended download is **Zanik Manager**. The portable app installs and updates the server package, edits configuration, runs diagnostics, builds assets from a licensed client, and uploads them without requiring Python or SSH. See [docs/manager.md](docs/manager.md).

## Choose an installation

- Can edit the startup command: use **wrapper mode** (recommended).
- Cannot edit the executable path, such as a fixed Pterodactyl egg: use the acknowledged **replace-executable mode** after reading the warning.
- Hosting on Windows: the server loader is not supported yet. The Windows asset tool can create an asset bundle for a Linux server.

Download `zanik-server-linux-x86_64-VERSION.zip`, extract it, then run:

```sh
chmod +x scripts/*.sh bin/zanik-server
scripts/install.sh --server-root /home/container --mode wrapper
/home/container/zanik/bin/zanik-server --server-root /home/container -- GAME_ARGS
```

No game-derived asset is included. The Linux dedicated server supports inventory only; create the complete server-installable bundle from a licensed Windows client with the self-contained Windows asset tool. It produces a full ZIP and sub-90 MiB Pterodactyl parts. Missing optional artwork uses Zanik placeholders; catalog-dependent features report unavailable rather than returning broken data.

Read [installation](docs/installation.md), [configuration](docs/configuration.md), [assets](docs/assets.md), [security](docs/security.md), and [hosting](docs/hosting.md) before public deployment.

## Safe defaults

Public/read-only tracking is enabled by default. Teleports, storage and gravestone mutation, recipe-unlock administration, and every gameplay-changing QoL feature are disabled until the operator explicitly opts in. Disabled native features do not install their hook, scanner, dispatcher, or periodic task.

## Build and test

```sh
make clean all
make test-core       # clean-clone, asset-free tests
make test-assets     # opt-in tests using ignored local game inputs
```

CI builds and validates server `.tar.gz`/`.zip` archives and separate Linux and Windows asset-tool archives. See [contributor architecture](docs/contributor-architecture.md).

Stable releases are built from matching `vMAJOR.MINOR.PATCH` tags and published as permanent GitLab packages. See [releasing](docs/releasing.md) and [CurseForge distribution](docs/curseforge.md).

The engine discovery layer is adapted from RSDWServerAPI under MIT terms. See [UPSTREAM.md](UPSTREAM.md), [LICENSE](LICENSE), and [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
