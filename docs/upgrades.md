# Upgrades

Do not confuse the three update types. **Update and relaunch** updates Manager. **Update / repair Zanik** installs or repairs the native server payload. **Update Dragonwilds server** performs one confirmed dedicated-server update using the current installation mode. Wrapper mode uses the normal Pterodactyl Steam flow and restores the prior `AUTO_UPDATE` preference. Replacement mode requires `AUTO_UPDATE=0` and uses a journaled one-shot updater so Steam never permanently displaces Zanik's launcher.

After a game update, an executable absent from this release's reviewed compatibility manifest starts the official game normally but puts Zanik into maintenance mode. The website explains that Zanik is out of date, checks whether a compatible stable GitHub release exists, and initializes no native hooks, scanners, dispatchers, persistence workers, or ordinary APIs. This is the expected fast recovery state—not a failed game server. See [game-update safety](game-update-safety.md).

Stop the server, download the new server archive and `SHA256SUMS`, and follow the versioned staging and checksum steps in [installation.md](installation.md). Extract each version into its own `SERVER_ROOT/zanik-release/VERSION/payload/` directory, then run:

```sh
SERVER_ROOT=/srv/dragonwilds
RELEASE_ROOT="$SERVER_ROOT/zanik-release/VERSION"
"$RELEASE_ROOT/payload/scripts/update.sh" --server-root "$SERVER_ROOT"
"$SERVER_ROOT/zanik/scripts/doctor.sh" --server-root "$SERVER_ROOT" --json
```

On Pterodactyl, use `/home/container` as `SERVER_ROOT`.

Zanik Manager's **Update / repair Zanik** action preserves configuration and persistent data while reconciling the packaged server payload. It verifies uncertain files, reuses unchanged files whose prior hash and remote metadata remain valid, uploads only changed or missing files, and retires only paths recorded in the prior package-owned inventory. Its dated rollback backup contains only replaced or retired managed files; a fully current repair needs neither a backup nor a restart. The standalone shell updater remains a complete archive replacement for manual recovery workflows. A failed update restores the prior installation. Zanik retains the newest verified recovery point for each backup family and protects failed or interrupted work; after a newer operation verifies, superseded successful copies are removed automatically. Doctor's **Review storage cleanup** action handles recognized older pre-retention backups only after showing their exact paths.

Replacement-mode Dragonwilds updates retain one prior official executable, the compatibility launcher, Steam manifest, and a bounded SteamCMD log in one `SERVER_ROOT/zanik-backups/game-update/` directory. Wrapper-mode game updates do not make executable backups because Steam updates the untouched official executable.
