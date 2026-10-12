# Dragonwilds player status

A public, read-only player page for a Linux RuneScape: Dragonwilds dedicated
server. The mod loads with `LD_PRELOAD` and exposes only:

| Request | Result |
| --- | --- |
| `GET /` | Player status webpage |
| `GET /api/players` | Currently online player summaries |
| `GET /players` | Every player recorded by the server |
| `GET /api/known-players` | Online and offline player summaries |
| `GET /player?id=PUBLIC_ID` | Persistent player skill details webpage |
| `GET /api/player?id=PUBLIC_ID` | Status, last seen time, skill levels, and XP |
| `GET /kills` | Persistent enemy kill totals and leaderboard |
| `GET /api/kills` | Enemy kill totals |
| `GET /api/kill-leaderboard` | Per-player kill totals |
| `GET /api/kill-tracker-diagnostics` | Hook compatibility and concise event counters |
| `GET /api/update-status` | Runtime and guarded-updater compatibility status |
| `GET /api/quests/catalog` | Public playable main/side quest catalog |
| `GET /api/player-quests?id=...` | Persisted public quest progress for one player |
| `GET /api/quest-tracker-diagnostics` | Quest catalog, runtime query, persistence, and timing health |

Names are the game's player display names. The API returns HTTP 503 with
`{"error":"Server data unavailable"}` while the game is loading or if the
expected game structures cannot be validated. No admin actions, account IDs,
positions, or stats are exposed. Other paths return 404; other methods return
405. The page refreshes every 10 seconds and uses text nodes for names.

Online names link to their individual detail pages. The Players tab lists all
players observed since persistence was installed. Skill levels use the current
Dragonwilds 1–99 XP thresholds, including the level 93–99 curve, and the detail
API also returns raw XP for verification. Online records refresh every two
seconds and are saved whenever names, XP, or the named map region changes.
Offline pages show the last saved skills, last-seen time, and last known
region. Coordinates and movement history are not exposed.

Persistent player data is stored at `/home/container/zanik-data/players.db`, with complete per-character recipe snapshots in `/home/container/zanik-data/player-recipe-snapshots.db` and quest snapshots in `/home/container/zanik-data/player-quest-snapshots.db`. Account
identifiers are used only as private keys in that server-side file. Public URLs
and APIs receive an opaque derived ID rather than the account identifier.

Direct player kills are captured from the native AI death notification. The
hook records the AI victim and the fatal attacker while both objects are still
valid, accepts only attackers that resolve to a connected player object, and
uses a bounded queue with duplicate suppression. AI-versus-AI, environmental,
despawn, and unresolved deaths are ignored. The hook is enabled only for the
exact supported executable SHA-256 and a unique instruction signature; the
Kills page reports tracking as unavailable when either check fails. Historical
totals remain readable. Multiplayer attribution is implemented through exact
player object matching but has not yet been live-tested.

## Install in Pterodactyl

1. Back up your server files and test on a nonproduction server first. Request
   **one additional TCP allocation** from the panel administrator. Keep the
   existing Dragonwilds UDP allocation.
2. Stop the server and upload these files to the panel paths shown:

   - `dist/libdwstatus.so` → `/home/container/libdwstatus.so`
   - `deploy/zanik-launcher.sh` →
     `/home/container/RSDragonwilds/Binaries/Linux/RSDragonwildsServer-Linux-Shipping`

3. Before replacing the game executable, rename the official file to:

   `/home/container/RSDragonwilds/Binaries/Linux/RSDragonwildsServer-Linux-Shipping.zanik-original`

   Make the replacement launcher executable (`0755`). Keep `Extra Arguments`
   at its normal value; the development server uses
   `-log -unattended -NoCrashDialog`. The checked-in launcher uses TCP port
   `29105`; edit `DW_STATUS_PORT` for a different allocation.

   The panel's file editor creates new files with mode `0644`. Deployment via
   the API preserves executable mode by copying an existing executable, writing
   the launcher into that copy, and renaming it into place.
4. Restart the server. The Pterodactyl console should show
   `Public status page listening on TCP port PORT`. Open
   `http://SERVER_IP:PORT/` and `http://SERVER_IP:PORT/api/players`.
5. During startup the API should return 503. Once the world loads, verify zero
   players, then join and leave to confirm count and names update. If it stays
   at 503, check the game version and engine discovery messages in the console.

After a Steam update replaces the launcher, preserve the new official binary
as `.zanik-original` and reinstall the launcher before starting the server.

The development server uses `deploy/zanik-bootstrap.sh` with Pterodactyl's
`AUTO_UPDATE` disabled. It queries Steam build metadata on each restart and
runs SteamCMD, backups, repair, analysis, and update reporting only when the
public build ID changes. Reports are retained under
`/home/container/zanik-data/update-history/`; ordinary restarts only append an
operational log entry.

The page is intentionally public over plain HTTP for this first release.
Do not put a private world password or other sensitive data in display names.
For a future HTTPS domain, place a reverse proxy in front of this TCP port.

## Build from source

The distributed `.so` was built using Debian Bookworm with GCC 12. Its highest
required glibc version is 2.36. To reproduce it on a Linux host with Docker:

```sh
chmod +x build.sh
./build.sh
```

Or, on Debian Bookworm with `g++` and `make` installed, run `make clean all`.
The output is `dist/libdwstatus.so`. After a game update, recheck the offsets
and validation in `src/engine/` before rebuilding; the game exposes no stable
official player API used by this mod.

Run local checks with `make test` and `python3 tests/http_test.py`. The HTTP
smoke test preloads the library into a harmless `sleep` process, so it expects
503 for game data. A live Dragonwilds server is required to verify real player
discovery and the Pterodactyl allocation.

## Source and license

The engine discovery layer is adapted from
[RSDWServerAPI](https://github.com/dkoz/RSDWServerAPI) under MIT terms. See
[UPSTREAM.md](UPSTREAM.md) and [LICENSE](LICENSE).
