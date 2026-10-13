# Configuration and feature gates

Copy `config/zanik.conf.example` to `config/zanik.conf`. It is strict `KEY=VALUE` text: no quoting, substitution, shell code, or unknown keys. Precedence is launcher/install options, `ZANIK_*` environment variables, this file, legacy `DW_*` compatibility variables, then defaults.

Malformed configuration logs the exact non-secret key/line error and launches the official game without Zanik. It never prevents the server itself starting.

Read-only player/status, kills, map/catalog, resource/NPC, recipe/quest, and authenticated read administration default on. All teleport, mutation, recipe-unlock administration, and QoL gameplay options default off. A disabled admin operation authenticates first, then returns HTTP 403 with `Feature disabled by server operator`; its UI control is hidden.

`FEATURE_QOL_AUTO_COMPOST` is reserved for an incomplete feature with no deployable server integration. Leave it `false`; it is not currently a supported opt-in capability.

`/api/update-status` reports configuration validity, install mode, feature booleans and states, native compatibility, and asset/catalog health.
