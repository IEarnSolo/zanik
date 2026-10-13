# Changelog

All notable changes to Zanik are documented here.

## [0.2.5] - 2026-10-13

### Added

- Automatically mirror each new validated stable release from the authoritative GitLab pipeline to the public GitHub repository with retry-safe asset digest checks.
- Add a complete first-class self-hosted Linux installation, upgrade, diagnostics, and Windows-to-Linux asset-transfer workflow alongside Zanik Manager.

### Changed

- Require source branches and annotated release tags to be pushed to GitLab first and then to the public GitHub mirror.
- Mark automatic compost equipment as incomplete and keep `FEATURE_QOL_AUTO_COMPOST` disabled until its native server integration and validation are complete.

## [0.2.4] - 2026-10-13

### Added

- Offer a guided website-asset setup immediately after a successful server installation when the target has no healthy active asset manifest.
- Let the guided workflow validate and reuse a compatible cached bundle or extract, upload, atomically activate, and verify a new bundle for the just-installed server.
- Add an explicit target-server selector and final target confirmation to manual asset uploads.
- Add machine-readable mapping preflight and persistent recovery data under `%LOCALAPPDATA%\Zanik Manager\mapping-recovery\`.

### Fixed

- Safely preserve, verify, temporarily remove, and exactly restore an existing user-managed UE4SS/proxy-loader installation during Manager-driven mapping generation.
- Detect interrupted mapping work and fail closed if either the recovery backup, temporary mapper, game executable, or restoration target changes unexpectedly.
- Offer a graceful **Close game and continue** action when Dragonwilds is running; the Manager never force-terminates the client.
- Make reviewed recipe-history seeding optional so the self-contained Windows extractor builds a complete current catalog without a source checkout, generated metadata, or pre-existing web tree.

## [0.2.3] - 2026-10-13

### Fixed

- Treat a Pterodactyl Wings `500` for a provably absent parent directory as a normal missing path, allowing Zanik Manager first installs to advance from staging to activation instead of retrying the probe as a node outage.

## [0.2.2] - 2026-10-12

### Fixed

- Show the remote-HTTP credential warning only when a user actually attempts an unencrypted connection, instead of displaying an acknowledgement checkbox for HTTPS panels.
- Invalidate discovered servers when the panel URL or API key changes so stale connection results cannot be saved.
- Fall back to Pterodactyl's signed file download when its normal contents endpoint reports a transient node-side error, including while loading configuration.
- Keep internal tracebacks out of error dialogs while retaining redacted technical details in the Activity tab.
- Show application-modal Zanik Manager stage and progress feedback for installation, diagnostics, configuration, extraction, and asset activation so long operations cannot overlap.
- Refuse asset extraction while Dragonwilds is already running, with guidance that the Manager will briefly launch and close the client to generate mappings.

## [0.2.1] - 2026-10-12

### Fixed

- Composite the extracted transparent world-map landmass over Zanik's ocean background instead of preserving transparent white texels as visible JPEG gaps.
- Resize generated world maps to the website's required 4096-by-4096 canvas and reject bundles with unexpected map dimensions.
- Version the corrected extraction recipe so Zanik Manager does not reuse an older affected bundle.
- Select the clean `Tag_Skill` artwork for player-page skill cards instead of the visually different glowing skill-tree node textures.
- Accept structurally valid recipe catalogs whose exact count matches the reviewed asset profile, rather than rejecting new game builds with a stale native count.
- Preserve matching saved player recipe unlocks across reviewed catalog removals and mark new recipes unknown until the character is scanned again.
- Seed newly extracted bundles with reviewed recipe catalog history so saved player snapshots remain migratable.

### Changed

- Use archive integrity, ZIP CRCs, and critical-file sampling for normal Pterodactyl asset activation.
- Make Deep Doctor hash the complete asset tree on the server and download one aggregate report, retaining per-file API downloads only as a compatibility fallback.

## [0.2.0] - 2026-10-12

### Added

- Zanik Manager, a portable Windows GUI for managing one or more Pterodactyl servers without Python, SSH, or manual file copying.
- Windows Credential Manager storage for panel API keys, explicit remote-HTTP warnings, redacted activity logs, and reviewable support bundles.
- Journaled install, repair, update, uninstall, configuration, Doctor, and asset-activation workflows with verified staging, backups, rollback records, and interrupted-operation recovery.
- Integrated licensed-client asset extraction, a two-version validated bundle cache, and direct upload and activation from the manager.
- A shared configuration schema and release payload manifest to keep GUI and command-line behavior aligned.

### Distribution

- The recommended Windows download is an all-in-one manager containing the matching server payload and asset extractor.
- Standalone Linux server and Windows/Linux asset-tool archives remain available for advanced and command-line installations.
- The initial portable manager is unsigned and may display a Windows SmartScreen warning. Release checksums remain published, and CI supports optional protected-variable signing.

### Known limitations

- Pterodactyl is the only GUI hosting provider in this release. Unknown eggs may require a guided startup-command change for wrapper mode.
- The native server loader remains Linux x86-64 only. The GUI and asset extractor run on Windows x86-64.

## [0.1.2] - 2026-10-12

### Added

- A self-contained Windows client asset tool with pinned Python, UnrealAssetScout, and UE4SS components and their license notices.
- Safe local mapping generation with collision refusal, recovery journaling, graceful client shutdown, and an offline `.usmap` override.
- Targeted catalog and artwork generation, reviewed drift profiles, complete bundle manifests, and full web-asset reference validation.
- A conventional asset ZIP plus checksum-indexed Pterodactyl parts below 90 MiB, with hash-identical atomic installation paths.

### Changed

- Linux dedicated-server containers are now explicitly inventory-only; complete web bundles are built from a licensed Windows client.
- Recipe, building, quest, NPC, resource, item, plan, map, font, and icon generation no longer depends on developer-machine paths or the third-party save-editor dataset.
- Reviewed quest relationships use stable game actor identities where generated opaque IDs were not portable between generators.

## [0.1.1] - 2026-10-12

### Fixed

- Safely migrate development servers that use Zanik's legacy Pterodactyl executable-replacement launcher without overwriting the preserved official ELF.
- Report enabled features as compatible or incompatible from their live subsystem diagnostics, while retaining an explicit disabled state for gated features.
- Preserve the `zanik-admin <public-character-id>` Pterodactyl console login proxy in the canonical launcher.

### Changed

- Expanded development-server and asset-initialization validation for the post-0.1.0 packaging layout.
- Reduced NPC scanner and live-refresh slices to a 400-microsecond budget, capped each pass at 128 objects, and exposed slow-slice counters in diagnostics.

## [0.1.0] - 2026-10-12

### Added

- Linux x86-64 server-side player, world, recipe, quest, resource, NPC, map, and administrative web features.
- Recommended non-destructive `zanik-server` wrapper installation and an explicitly acknowledged Pterodactyl executable-replacement compatibility mode.
- Strict configuration and independent feature gates: read-only capabilities default on, while mutations, teleports, recipe administration, and gameplay-changing QoL behavior default off.
- Five-minute single-use console login claims, character allowlisting, CSRF protection, rate limiting, hashed sessions, and redacted audit records.
- Server-first asset initialization with a licensed Windows-client fallback and asset-free web behavior.
- Installer, updater, uninstaller, diagnostics, compatibility analysis, and validated Linux/Windows release bundles.

### Known limitations

- The server loader supports Linux x86-64 only. Windows support is limited to building asset bundles; a Windows server loader is deferred.
- HTTP is intentionally supported for this release. Operators should use a trusted network or reverse proxy because HTTP traffic and session cookies can be intercepted.
- Game-derived assets are not distributed and must be generated from a licensed dedicated server or client installation.
- CurseForge installation is manual because Zanik is a native dedicated-server mod rather than a client-managed UE4SS package.
