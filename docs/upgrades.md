# Upgrades

Do not confuse the three update types. Zanik Manager updates itself from a verified stable GitHub release through **Update and relaunch**. The Servers-tab **Update / repair Zanik** action reinstalls the matching Zanik payload on the selected server using its existing installation mode. Dragonwilds dedicated-server updates remain an operator/hosting responsibility; Manager only reports their availability and never runs Steam `app_update`.

Stop the server, download the new server archive and `SHA256SUMS`, and follow the versioned staging and checksum steps in [installation.md](installation.md). Extract each version into its own `SERVER_ROOT/zanik-release/VERSION/payload/` directory, then run:

```sh
SERVER_ROOT=/srv/dragonwilds
RELEASE_ROOT="$SERVER_ROOT/zanik-release/VERSION"
"$RELEASE_ROOT/payload/scripts/update.sh" --server-root "$SERVER_ROOT"
"$SERVER_ROOT/zanik/scripts/doctor.sh" --server-root "$SERVER_ROOT" --json
```

On Pterodactyl, use `/home/container` as `SERVER_ROOT`.

The updater preserves configuration and creates a dated backup of authorization, sessions, audit logs, persistence, compatibility data, generated assets, installed files, and managed game paths. A failed update restores the prior installation. Retain the backup until the updated server has passed runtime validation.
