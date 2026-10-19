# Installation and lifecycle

Linux x86-64 is the supported server runtime. Run Zanik's lifecycle commands as the same account that owns and runs the Dragonwilds server. The examples below use `/srv/dragonwilds`; replace that absolute path with the root containing `RSDragonwilds/Binaries/Linux/RSDragonwildsServer-Linux-Shipping` on your server.

## Download and stage the release

For a normal Linux shell installation, download these two files from the same Zanik release:

- `zanik-server-linux-x86_64-VERSION.tar.gz`
- `SHA256SUMS`

The server ZIP contains the same payload and is intended mainly for hosting panels and manual ZIP uploads. Replace `VERSION` below with the release number, without the leading `v`:

```sh
SERVER_ROOT=/srv/dragonwilds
ZANIK_VERSION=VERSION
RELEASE_ROOT="$SERVER_ROOT/zanik-release/$ZANIK_VERSION"

mkdir -p "$RELEASE_ROOT/payload"
mv "zanik-server-linux-x86_64-$ZANIK_VERSION.tar.gz" SHA256SUMS "$RELEASE_ROOT/"
cd "$RELEASE_ROOT"
grep " zanik-server-linux-x86_64-$ZANIK_VERSION.tar.gz$" SHA256SUMS | sha256sum --check -
tar -xzf "zanik-server-linux-x86_64-$ZANIK_VERSION.tar.gz" -C payload
```

The checksum command must report `OK`. Stop if it does not, or if it prints no matching checksum line.

## Recommended wrapper mode

Stop the game server and install from the extracted payload:

```sh
cd "$RELEASE_ROOT/payload"
./scripts/install.sh --server-root "$SERVER_ROOT" --mode wrapper
```

Configure systemd, your container entrypoint, or another service manager to start:

```sh
/srv/dragonwilds/zanik/bin/zanik-server --server-root /srv/dragonwilds -- GAME_ARGS
```

Replace `GAME_ARGS` with the server's existing arguments. The launcher applies `LD_PRELOAD` only to the game process, and the official executable is not renamed or modified.

Before the first start, edit `SERVER_ROOT/zanik/config/zanik.conf`:

- Set `STATUS_PORT` to a separately allocated and open TCP port. It is the Zanik website/API port, not the Dragonwilds game port.
- Set `PUBLIC_BASE_URL` to the address users will open, such as `http://server.example:29105`. Include the public port unless a reverse proxy uses a standard port.
- Allow that TCP port through the host firewall, router/NAT, container mapping, or hosting-panel allocation as applicable.

Important installed paths are:

| Purpose | Example path |
| --- | --- |
| Installed program and configuration | `/srv/dragonwilds/zanik/` |
| Configuration | `/srv/dragonwilds/zanik/config/zanik.conf` |
| Persistent state and secrets | `/srv/dragonwilds/zanik-data/` |
| Staged releases | `/srv/dragonwilds/zanik-release/VERSION/` |
| Incoming generated asset bundles | `/srv/dragonwilds/zanik-assets-incoming/CLIENT_BUILD/` |
| Active generated assets | `/srv/dragonwilds/zanik/assets/current` |
| CLI upgrade history (newest verified recovery point) | `/srv/dragonwilds/zanik-data/update-history/` |

After installation, run:

```sh
"$SERVER_ROOT/zanik/scripts/doctor.sh" --server-root "$SERVER_ROOT" --json
```

Continue with the [Windows-to-Linux asset workflow](assets.md#install-on-a-self-hosted-linux-server). Game-derived web assets are not included in the server archive.

## Fixed-path compatibility mode

This mode modifies a core game path. Use it only when a hosting panel cannot change the startup executable. Self-hosted operators who control their service command should use wrapper mode.

On Pterodactyl, replacement mode additionally requires an editable `AUTO_UPDATE` startup variable and Client API permissions `startup.read` and `startup.update`. Zanik Manager sets and verifies `AUTO_UPDATE=0` before installation. If it cannot, installation is blocked rather than risking Steam overwriting the managed launcher. Manual shell users must disable panel automatic updates themselves before using this mode.

```sh
./scripts/install.sh --server-root "$SERVER_ROOT" --mode replace-executable \
  --acknowledge-core-file-replacement
```

The installer atomically preserves the official binary as `RSDragonwildsServer-Linux-Shipping.zanik-original` and puts a small managed launcher at the original path. A repeat install is safe. If Steam replaces that launcher, Zanik recognizes the new official binary and preserves it before reinstalling the launcher.

## Pterodactyl

For a standard Pterodactyl layout, use `/home/container` as `SERVER_ROOT`. Upload and extract the server ZIP under `/home/container/zanik-release/VERSION/payload/`, then run its `scripts/install.sh`. See [pterodactyl.md](pterodactyl.md) for the exact panel workflows.

## Update, health, and uninstall

Extract each new release into a new versioned payload directory. The installed copy can repair its current version, but only the new release payload can perform an update:

```sh
"$RELEASE_ROOT/payload/scripts/update.sh" --server-root "$SERVER_ROOT"
"$SERVER_ROOT/zanik/scripts/doctor.sh" --server-root "$SERVER_ROOT" --json
"$SERVER_ROOT/zanik/scripts/uninstall.sh" --server-root "$SERVER_ROOT"
```

Updates back up configuration, compatibility metadata, assets, authorization, sessions, audit records, and persistence under `SERVER_ROOT/zanik-data/update-history`. Successful CLI updates retain one verified recovery point; interrupted or failed work is protected. Uninstall restores only a verified Zanik-managed launcher and preserves data. Add `--purge-data` only when permanent state removal is intended. `zanik-data` is live persistent application state, not disposable backup storage.
