# Pterodactyl installation

The easiest path is the portable Windows [Zanik Manager](manager.md), which performs the file, power, configuration, diagnostic, and asset workflows through a normal Client API key. The commands below remain supported for manual and advanced use.

Stop the server, upload the server ZIP, and extract it to `/home/container/zanik-release/VERSION/payload/`. Copy no individual library into the game directory.

Preferred mode:

```sh
/home/container/zanik-release/VERSION/payload/scripts/install.sh --server-root /home/container --mode wrapper
```

Set the egg command to `/home/container/zanik/bin/zanik-server --server-root /home/container --` followed by its existing arguments.

Allocate a separate TCP port for Zanik in the panel. Do not use the Dragonwilds game port. Set that allocation as `STATUS_PORT` in `/home/container/zanik/config/zanik.conf`, and set `PUBLIC_BASE_URL` to the website address users will open, including the allocated public port. Restart the server after changing either value.

If the egg command cannot be changed, use the explicitly acknowledged command in [installation.md](installation.md). That compatibility mode is non-recommended. Upload a client-built asset bundle to `/home/container/zanik-assets-incoming/CLIENT_BUILD/` without manually extracting it, then activate it only through `/home/container/zanik/scripts/init-assets.sh` as described in [assets.md](assets.md#install-on-pterodactyl).

Steam may restore the official executable over the compatibility launcher during an update or restart. After Manager activates assets and restarts a replacement-mode server, it verifies that Zanik's small launcher is still present. If Steam displaced it, Manager keeps the successfully activated assets and offers a safe repair for that exact server. Repair preserves the updated official executable as `.zanik-original` before restoring the managed launcher.

The Manager's game indicator checks only the active server. It reads `/home/container/steamapps/appmanifest_4019830.acf` and compares that installed build with public dedicated-server metadata, so it works before Zanik is installed and does not require the game server to be running. It never installs an update. Installed launchers retain `zanik-check-game-update REQUEST_ID` as a bounded SteamCMD fallback and cache its result at `/home/container/zanik/state/game-update-status.json`. Update Dragonwilds through the panel or host, then repair the compatibility launcher if Steam displaced it and re-run Doctor.

For an allowlisted administrator, enter `zanik-admin <public-character-id>` in the Pterodactyl console. The canonical launcher intercepts only that command and prints a five-minute, single-use login link; every other console command is forwarded unchanged to the game server.
