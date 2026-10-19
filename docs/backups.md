# Backups and storage retention

Zanik creates recovery data only around operations that can replace or remove managed files. These are not substitutes for host-level backups of the Dragonwilds world.

## Remote server recovery points

Manager keeps at most one verified successful recovery point in each family:

| Family | Location | Created when | Retained contents |
| --- | --- | --- | --- |
| Payload | `SERVER_ROOT/zanik-backups/manager/` | Updating, repairing, or migrating an existing Zanik installation | The previous managed payload and any launcher file needed for rollback |
| Game update | `SERVER_ROOT/zanik-backups/game-update/` | A replacement-mode Dragonwilds update | One previous official executable, the Zanik launcher, Steam manifest, and a bounded SteamCMD log |
| Configuration | `SERVER_ROOT/zanik-backups/manager/` | Saving `zanik.conf` | One previous configuration file |
| Uninstall | `SERVER_ROOT/zanik-backups/manager/` | Removing an installed Zanik payload | The removed installation and launcher recovery files |

A fresh installation does not create an empty recovery backup. Wrapper-mode game updates remain backup-free unless Zanik must change a managed file. Shell updates and recognized legacy migrations use the same one-per-family rule under `SERVER_ROOT/zanik-data/update-history/`.

Each new managed backup contains `.zanik-backup.json`. After an operation verifies successfully, Zanik removes older successful copies in that family. Failed or interrupted recovery data remains protected until it is rolled back, resumed, or superseded by a later verified equivalent operation. Completed staging directories and transaction reports are removed.

`SERVER_ROOT/zanik-data/` is persistent application state, not disposable backup data. Zanik never automatically removes Dragonwilds saves, Pterodactyl panel backups, `zanik-release`, `zanik-assets-incoming`, support archives, or unrecognized directories.

## Assets and Windows storage

The server retains `zanik/assets/current` and `zanik/assets/previous`. Manager retains the newest two unpinned validated bundles per game installation; pinned bundles remain until the operator removes them. It also keeps one incremental extraction workspace per game installation because that cache makes later selective extraction substantially faster. Use **Clear incremental cache** only when that tradeoff is acceptable.

Temporary UE4SS preservation backups are deleted after the original loader is restored and verified. Manager update staging is removed after the startup handshake, and only one prior portable Manager directory is retained for rollback. Local operation journals retain unresolved work plus the newest completed journal for each profile and action.

## Reviewing cleanup

Open **Doctor** and choose **Review storage cleanup**. Manager reports local usage and shallowly inventories only recognized Zanik backup roots on the selected server. The confirmation lists every exact remote path eligible for deletion. Pre-manifest legacy directories require this explicit review and are never automatically deleted.

Interrupted or failed recovery points are not selectable for ordinary cleanup. Manager does not recursively download or hash a remote backup merely to calculate its size, so legacy sizes may be unavailable.
