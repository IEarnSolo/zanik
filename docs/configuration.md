# Configuration and feature gates

Copy `config/zanik.conf.example` to `config/zanik.conf`. It is strict `KEY=VALUE` text: no quoting, substitution, shell code, or unknown keys. Precedence is launcher/install options, `ZANIK_*` environment variables, this file, legacy `DW_*` compatibility variables, then defaults.

`ZANIK_ENABLED=true` is the master switch. Set it to `false` and restart to make the canonical launcher sanitize inherited `LD_PRELOAD` and execute the official game server without the Zanik library, native hooks, website, scanners, dispatchers, console proxy, maintenance watcher, asset activation, persistence writes, or periodic Zanik work. A persisted master-off setting cannot be overridden back on by the environment. The Zanik installation, configuration, authorization records, generated assets, and persistent data remain in place so the mod can be re-enabled safely later.

Malformed configuration logs the exact non-secret key/line error and launches the official game without Zanik. It never prevents the server itself starting.

`STATUS_PORT` is the TCP port on which Zanik serves its website and API. Allocate and open a separate TCP port for it in the firewall, router, container, or hosting panel; do not reuse the Dragonwilds game port. `PUBLIC_BASE_URL` is the externally reachable URL placed in generated links, including the public port when one is required. A reverse proxy may expose a different public port than `STATUS_PORT`.

Read-only player/status, kills, map/catalog, resource/NPC, recipe/quest, and authenticated read administration default on. All teleport, mutation, recipe-unlock administration, and QoL gameplay options default off. A disabled admin operation authenticates first, then returns HTTP 403 with `Feature disabled by server operator`; its UI control is hidden.

`FEATURE_ADMIN_TELEPORT` controls every authenticated teleport action: teleports between players, player-built lodestones, fixed nodestones, storage, and gravestones. Its independently validated compatibility gates can still disable only an unsafe destination family while leaving the others available.

`FEATURE_QOL_AUTO_COMPOST` is reserved for an incomplete feature with no deployable server integration. The launcher, Manager, and native runtime enforce `false`; it is not currently a supported opt-in capability.

`FEATURE_QOL_STORAGE_AUTOFILL` is an off-by-default administrative QoL capability. When enabled, an authenticated administrator can snapshot an individual storage container. Zanik persistently restores missing quantities, rebalances only matching items between snapshot-approved slots, and resets recognized matching stacks to their saved quantities so each saved slot returns to its exact state. A matching stack later placed in a slot that was empty in the snapshot is preserved at its pre-refill quantity even if the native add temporarily consolidates into it. A refill and its matching-stack normalization complete in one bounded game-thread transaction before the updated storage snapshot is published. It never removes or overwrites an unexpected occupied item. Successful native storage moves and removals prioritize the affected container, reread only that identity-bound chest, and reconcile it through the ordinary bounded dispatcher; whole-object discovery and a 500 ms round-robin scan remain fallbacks for stale, missed, or dropped events. The capability depends on the independently validated storage snapshot, mutation, persistence, and dispatcher gates.

`/api/update-status` reports configuration validity, install mode, feature booleans and states, native compatibility, and asset/catalog health.
