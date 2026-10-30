# Character-linked web administration

Development authorization is stored in `/home/container/zanik/admins.txt`. Each
non-comment line contains one stable public character ID. Removing an ID
invalidates that character's browser sessions the next time the file is read.

Persistent session hashes are stored in
`/home/container/zanik-data/admin-sessions.db` with mode `0600`. Teleport audit
records are appended to `/home/container/zanik-data/admin-audit.log`. Raw claim
and session tokens are never persisted or logged.

Recipe-tree visibility preferences are administered at
`/admin/recipe-unlocks` and persisted in
`/home/container/zanik-data/recipe-unlock-settings.db` with mode `0600`.
Unchecked routes become a public “View more” tile instead of being discarded.

Development and production use distinct browser cookie names because HTTP
cookies are scoped by hostname and path, not TCP port. Development uses
`zanik_admin_dev` on port 29105; production keeps `zanik_admin` on port 29103.
This allows both admin sessions to remain active in the same browser profile.

The embedded HTTP listener applies bounded receive and send deadlines to each
accepted browser connection. A suspended tab, interrupted storage refresh, or
client that stops reading a large storage response therefore cannot block the
public map and every administrative API indefinitely.

The browser endpoints, claim exchange, cookies, CSRF validation, logout,
allowlist reload, revocation storage, destination validation, rate limiting,
and map controls are implemented.

## Creating a browser session

Enter `zanik-admin` in the development server's Pterodactyl console. The
persistent Zanik bootstrap intercepts that console line and prints a private,
single-use login URL. The URL expires after five minutes. When exactly one
character is allowlisted, no ID is required. If several characters are
allowlisted, use `zanik-admin PUBLIC_ID`.

The bootstrap authenticates to a loopback-only claim endpoint with a random
secret stored at `/home/container/zanik-data/console-admin.key`. Requests to
that endpoint without the secret are rejected. Other console input is passed
unchanged to the game server.

Public lodestone map tooltips expose authoritative X/Y/Z coordinates from the
validated lodestone registry. Dragonwilds does not persist the character that
placed a lodestone, so Zanik does not guess or maintain a separate forward-only
ownership history. Native pointers and registry identities remain server-side.

Lodestone, player, and storage teleport requests use fingerprint-bound native
dispatchers. Storage mutation requests accept only opaque revisions. The HTTP
worker resolves them into an immutable identity-bound plan; the game thread
revalidates those identities and performs at most one native action per
eligible tick. The browser never supplies native addresses or world
coordinates.

## Update compatibility

The compatibility analyzer and runtime validator cover the following admin
boundaries independently:

- authorization storage permissions, allowlist readability, malformed session
  records, the console proxy secret, claim creation, and unauthenticated API
  rejection;
- lodestone and player teleport entry points, delayed teleport capture,
  `TeleportWithParams`, collision tracing, actor bounds, success effects, and
  the scoped lodestone cost flag;
- storage game-thread dispatch, inventory mutations, batch additions, slot
  organization, naming, catalog metadata, the cooked `ITEM_*`, vestige, and
  mount-unlock asset families and their inventory icons, and immediate
  building-removal tracking;
- gravestone ownership, inventory, registry layout and record size, native
  transfer and removal, World Partition streaming, SPUD restoration, and the
  passive respawn capture hook used for restart-persistent remote operations;
  registry-triggered inventory capture is time-budgeted and incrementally
  finalized, durable snapshot writes run only on the website worker, and
  persisted cards are retired only by the authoritative registry-unregister
  event rather than transient actor or registry visibility.

`/api/update-status` and `/api/admin-diagnostics` expose these results instead
of reducing the admin panel to one build fingerprint. A failed subfeature must
fail closed without disabling unrelated read-only player pages or admin tools.
The update bootstrap records the admin results in both Markdown and JSON update
reports. It backs up the allowlist, session database, audit log, console secret,
gravestone capture data, compatibility manifest, mod, catalogs, and web assets
before applying an update.

Post-update validation checks authorization storage health, console claim
creation through the loopback secret, rejection of an unauthenticated admin API
request, and every native admin compatibility group. A missing symbol, changed
signature, changed layout, failed hook installation, malformed session store,
or unavailable persistence path is recorded in the update report and disables
the affected admin capability. It does not roll back an otherwise compatible
game update or disable unrelated public and read-only features.
