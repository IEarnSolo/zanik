# Pterodactyl installation

The easiest path is the portable Windows [Zanik Manager](manager.md), which performs the file, power, configuration, game-update, diagnostic, and asset workflows through a normal Client API key. An Application API or panel-administrator key is not required or recommended.

## Choose the installation mode

**Wrapper mode is recommended.** It is best for panel administrators or eggs with an editable executable variable. The official executable stays untouched, `AUTO_UPDATE` may remain enabled, and normal Pterodactyl Steam updates run before Zanik's wrapper. If a new executable is not yet supported, gameplay starts and Zanik serves only its maintenance page.

**Replace-executable mode is for fixed egg commands.** A Client API key cannot rewrite the raw egg startup command. Manager therefore preserves the official executable as `.zanik-original` and places its small launcher at the fixed path. This mode requires editable `AUTO_UPDATE`; Manager sets and verifies it as `0` and blocks installation if it cannot. Future game updates must use Manager's launcher-aware workflow.

Required account permissions are:

- Read-only discovery: access to the server, resources and files, plus `startup.read`.
- Install, repair and assets: `file.read`, `file.read-content`, `file.create`, `file.update`, `file.delete`, `control.start`, `control.stop`, and `control.restart`.
- Replacement installation and game updates: `startup.read` and `startup.update`.
- Console diagnostics and commands: `control.console`.

The Client API key cannot grant permissions the panel account does not already have. Wrapper mode needs an exposed editable variable or a panel administrator to change the egg command.

Stop the server, upload the server ZIP, and extract it to `/home/container/zanik-release/VERSION/payload/`. Copy no individual library into the game directory.

Preferred mode:

```sh
/home/container/zanik-release/VERSION/payload/scripts/install.sh --server-root /home/container --mode wrapper
```

Set the egg command to `/home/container/zanik/bin/zanik-server --server-root /home/container --` followed by its existing arguments.

Allocate a separate TCP port for Zanik in the panel. Do not use the Dragonwilds game port. Set that allocation as `STATUS_PORT` in `/home/container/zanik/config/zanik.conf`, and set `PUBLIC_BASE_URL` to the website address users will open, including the allocated public port. Restart the server after changing either value.

If the egg command cannot be changed, use the explicitly acknowledged command in [installation.md](installation.md). That compatibility mode is non-recommended. Upload a client-built asset bundle to `/home/container/zanik-assets-incoming/CLIENT_BUILD/` without manually extracting it, then activate it only through `/home/container/zanik/scripts/init-assets.sh` as described in [assets.md](assets.md#install-on-pterodactyl).

Do not enable panel automatic updates in replacement mode. Manager's confirmed **Update Dragonwilds server** action journals and backs up the launcher and official executable, runs SteamCMD through a bounded one-shot updater, restores the launcher, verifies the save tree did not change, and restarts. Wrapper mode retains the existing `AUTO_UPDATE` preference; if it is disabled for an explicit update, Manager enables it only for the update and restores it afterward.

The Manager's game indicator checks only the active server. It reads `/home/container/steamapps/appmanifest_4019830.acf` and compares it with public dedicated-server metadata, even before Zanik is installed. When an update is available, **Update Dragonwilds server** asks once before changing anything. See [game-update safety](game-update-safety.md).

Enter `zanik-player-id [name]` in the Pterodactyl console to list or search known characters and their public IDs. Duplicate names include a last-seen UTC timestamp. For an allowlisted administrator, enter `zanik-admin <public-character-id>`. The canonical launcher intercepts these Zanik commands, prints a five-minute single-use login link for the latter, and forwards every other console command unchanged to the game server. Authenticated administrators can manage authorization, create links, inspect valid sessions, and revoke individual sessions at `/admin/sessions`.
