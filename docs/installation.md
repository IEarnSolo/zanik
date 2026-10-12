# Installation and lifecycle

## Recommended wrapper mode

Stop the server, extract the server archive, and run:

```sh
scripts/install.sh --server-root /home/container --mode wrapper
```

Set the startup command to:

```sh
/home/container/zanik/bin/zanik-server --server-root /home/container --
```

Append the game's existing arguments after `--`. The official executable is not renamed or modified. Zanik configuration lives at `/home/container/zanik/config/zanik.conf`; persistent data lives at `/home/container/zanik-data`.

## Fixed-path compatibility mode

This mode modifies a core game path and may conflict with Jagex's guidance. Use it only when a panel cannot change the startup executable:

```sh
scripts/install.sh --server-root /home/container --mode replace-executable \
  --acknowledge-core-file-replacement
```

The installer atomically preserves the official binary as `RSDragonwildsServer-Linux-Shipping.zanik-original` and puts a small managed launcher at the original path. A repeat install is safe. If Steam replaces that launcher, Zanik recognizes the new official binary and preserves it before reinstalling the launcher.

## Update, health, and uninstall

Extract the new release and run its update script (the installed copy can repair
the current version but cannot supply a newer payload):

```sh
/home/container/zanik-release/scripts/update.sh --server-root /home/container
/home/container/zanik/scripts/doctor.sh --server-root /home/container --json
/home/container/zanik/scripts/uninstall.sh --server-root /home/container
```

Updates back up configuration, compatibility metadata, assets, authorization, sessions, audit records, and persistence under `/home/container/zanik-data/update-history`. Uninstall restores only a verified Zanik-managed launcher and preserves data. Add `--purge-data` only when permanent state removal is intended.
