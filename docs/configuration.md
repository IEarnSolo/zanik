# Configuration and feature gates

Copy `config/zanik.conf.example` to `config/zanik.conf`. It is strict `KEY=VALUE` text: no quoting, substitution, shell code, or unknown keys. Precedence is launcher/install options, `ZANIK_*` environment variables, this file, legacy `DW_*` compatibility variables, then defaults.

`ZANIK_ENABLED=true` is the master switch. Set it to `false` and restart to make the canonical launcher execute the official game server without `LD_PRELOAD`, native hooks, the website, scanners, or periodic Zanik work. The Zanik installation, configuration, authorization records, generated assets, and persistent data remain in place so the mod can be re-enabled safely later.

Malformed configuration logs the exact non-secret key/line error and launches the official game without Zanik. It never prevents the server itself starting.

`STATUS_PORT` is the TCP port on which Zanik serves its website and API. Allocate and open a separate TCP port for it in the firewall, router, container, or hosting panel; do not reuse the Dragonwilds game port. `PUBLIC_BASE_URL` is the externally reachable URL placed in generated links, including the public port when one is required. A reverse proxy may expose a different public port than `STATUS_PORT`.

Read-only player/status, kills, map/catalog, resource/NPC, recipe/quest, and authenticated read administration default on. All teleport, mutation, recipe-unlock administration, and QoL gameplay options default off. A disabled admin operation authenticates first, then returns HTTP 403 with `Feature disabled by server operator`; its UI control is hidden.

`FEATURE_QOL_AUTO_COMPOST` is reserved for an incomplete feature with no deployable server integration. Leave it `false`; it is not currently a supported opt-in capability.

`/api/update-status` reports configuration validity, install mode, feature booleans and states, native compatibility, and asset/catalog health.
