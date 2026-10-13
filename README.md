# Zanik

Zanik is a Linux x86-64 server-side mod for RuneScape: Dragonwilds. It exposes player, world, recipe, quest, resource, NPC, and map information through a web interface and provides separately gated administrative and quality-of-life features. Players do not install anything.

## What should I download?

Most server owners should download **`zanik-manager-windows-x86_64-VERSION.zip`**. Extract the whole ZIP on your Windows PC and run `ZanikManager.exe`. It guides you through installing Zanik on a Linux Pterodactyl server, configuring it, running Doctor, extracting web assets from your licensed Windows game client, and uploading those assets.

Use a different archive only when you specifically need a manual or command-line workflow:

| Download | Who it is for |
| --- | --- |
| `zanik-manager-windows-x86_64-VERSION.zip` | **Recommended for most users.** Complete Windows management app, Linux server payload, and asset extractor. |
| `zanik-server-linux-x86_64-VERSION.tar.gz` | **Recommended manual package for self-hosted Linux servers.** Preserves executable modes for a shell installation. |
| `zanik-server-linux-x86_64-VERSION.zip` | Pterodactyl and other hosting-panel administrators performing a manual upload. |
| `zanik-asset-tools-windows-x86_64-VERSION.zip` | Advanced users who only want to build web assets from a licensed Windows client. Already included in Zanik Manager. |
| `zanik-asset-tools-linux-x86_64-VERSION.tar.gz` | Advanced Linux inventory and catalog tooling. A Linux dedicated server cannot produce the complete visual bundle. |

Players joining a Zanik-enabled server download **nothing**. Zanik Manager runs on Windows, but the public server loader currently supports Linux x86-64 only.

Zanik is a native `LD_PRELOAD` library. The recommended installation leaves the official game executable untouched and starts it through `zanik-server`. Executable replacement remains available only for hosting panels whose egg cannot change its startup command; it is explicitly non-recommended.

For detailed Manager instructions, see [docs/manager.md](docs/manager.md).

## Choose an installation

- Can edit the startup command: use **wrapper mode** (recommended).
- Cannot edit the executable path, such as a fixed Pterodactyl egg: use the acknowledged **replace-executable mode** after reading the warning.
- Hosting on Windows: the server loader is not supported yet. The Windows asset tool can create an asset bundle for a Linux server.

For a self-hosted Linux server, download the server `.tar.gz` and `SHA256SUMS` from the same release. Place them under `SERVER_ROOT/zanik-release/VERSION/`, verify the selected archive, and extract it into `payload/`:

```sh
SERVER_ROOT=/srv/dragonwilds
ZANIK_VERSION=VERSION
RELEASE_ROOT="$SERVER_ROOT/zanik-release/$ZANIK_VERSION"
mkdir -p "$RELEASE_ROOT/payload"
mv "zanik-server-linux-x86_64-$ZANIK_VERSION.tar.gz" SHA256SUMS "$RELEASE_ROOT/"
cd "$RELEASE_ROOT"
grep " zanik-server-linux-x86_64-$ZANIK_VERSION.tar.gz$" SHA256SUMS | sha256sum --check -
tar -xzf "zanik-server-linux-x86_64-$ZANIK_VERSION.tar.gz" -C payload
"$RELEASE_ROOT/payload/scripts/install.sh" --server-root "$SERVER_ROOT" --mode wrapper
"$SERVER_ROOT/zanik/bin/zanik-server" --server-root "$SERVER_ROOT" -- GAME_ARGS
```

Before the first start, edit `SERVER_ROOT/zanik/config/zanik.conf`. Set `STATUS_PORT` to a separately allocated and open **TCP** port; it must not be the Dragonwilds game port. Set `PUBLIC_BASE_URL` to the address users will open, including the public port when needed.

No game-derived asset is included. The Linux dedicated server supports inventory only; create the complete server-installable bundle from a licensed Windows client with the self-contained Windows asset tool. Transfer the generated full ZIP to `SERVER_ROOT/zanik-assets-incoming/CLIENT_BUILD/` and let `init-assets.sh` validate, extract, and atomically activate it—do not unzip it manually. See the [self-hosted Linux guide](docs/linux.md) and [asset instructions](docs/assets.md#install-on-a-self-hosted-linux-server).

Read [installation](docs/installation.md), [configuration](docs/configuration.md), [assets](docs/assets.md), [security](docs/security.md), and [hosting](docs/hosting.md) before public deployment.

## Safe defaults

Public/read-only tracking is enabled by default. Teleports, storage and gravestone mutation, recipe-unlock administration, and every gameplay-changing QoL feature are disabled until the operator explicitly opts in. Disabled native features do not install their hook, scanner, dispatcher, or periodic task.

Automatic compost equipment is incomplete and has no deployable server integration. Keep `FEATURE_QOL_AUTO_COMPOST=false`.

## Build and test

```sh
make clean all
make test-core       # clean-clone, asset-free tests
make test-assets     # opt-in tests using ignored local game inputs
```

CI builds and validates server `.tar.gz`/`.zip` archives and separate Linux and Windows asset-tool archives. See [contributor architecture](docs/contributor-architecture.md).

Stable releases are built from matching `vMAJOR.MINOR.PATCH` tags. GitLab is the authoritative development and package platform; new releases and source refs are also mirrored to the public GitHub repository. See [releasing](docs/releasing.md) and [CurseForge distribution](docs/curseforge.md).

The engine discovery layer is adapted from RSDWServerAPI under MIT terms. See [UPSTREAM.md](UPSTREAM.md), [LICENSE](LICENSE), and [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
