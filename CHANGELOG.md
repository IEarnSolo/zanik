# Changelog

All notable changes to Zanik are documented here.

## [0.2.17] - 2026-10-15

### Fixed

- Launch the external Zanik Manager updater from its LocalAppData directory so Windows does not keep the portable installation directory locked during the transactional rename.
- Have the native updater independently leave the portable directory, retry transient rename locks, and record the precise failed operation and Windows error code in its status journal.

## [0.2.16] - 2026-10-15

### Fixed

- Restore native compatibility with Dragonwilds dedicated-server build 25811348 and record the corresponding development-server performance validation.
- Decode the updated cooked quest-objective schema so player quest cards regain their objective mini-maps and the world map regains all quest-step markers.
- Require reviewed nested quest-objective, step, and marker coverage during asset validation so an empty catalog cannot be published as compatible after a future schema change.
- Classify the Large Compost Bucket and Ultra Compost Bin as Farming entries in player recipe collections.
- Package the corrected quest extraction and validation logic in both Zanik Manager and the standalone Windows asset tool.

## [0.2.15] - 2026-10-15

### Added

- Add quiet Zanik Manager and active-game-server update indicators with separate manual checks and six-hour background refreshes.
- Validate stable Manager releases from GitHub and provide a transactional, rollback-capable **Update and relaunch** workflow.
- Add a bounded read-only launcher command that compares the installed Linux dedicated-server build with SteamCMD without installing an update.
- Include cached dedicated-server update health in `/api/update-status` and Doctor output.

### Changed

- Rename the server payload action to **Update Zanik server** and document the distinction between Manager, Zanik server payload, and Dragonwilds updates.

## [0.2.14] - 2026-10-14

### Added

- Have Zanik Manager offer to upload and atomically activate a newly extracted asset bundle on the explicitly selected Assets-tab server.
- Show each cached bundle's local validation timestamp alongside its game build and manifest fingerprint.
- Add concise hover help to Manager actions and guided-workflow buttons.

### Fixed

- Automatically load the selected startup server's configuration when Manager opens, while preventing a late response from being displayed or saved against another profile.
- After asset activation restarts a replacement-mode server, verify that Steam did not replace Zanik's compatibility launcher with the official executable. If it did, keep the activated assets and offer a pinned, data-preserving launcher repair.
- Keep explicitly pinned Manager workflows bound to their intended server when another profile is selected while the operation is in progress.

## [0.2.13] - 2026-10-14

### Fixed

- Stop treating arbitrary Unreal component properties named `Icon` as web textures; only authored texture packages and the reviewed embedded build-UI thumbnail locations are targeted.
- Correctly ignore the updated world level's ambience-controller billboard component, which previously created an impossible request to export `L_World` as a PNG.
- Validate coverage per requested texture package instead of requiring the total number of emitted PNGs to equal the number of packages; Unreal packages may legitimately emit additional textures.
- Allow Zanik Manager to resume the verified 3,113-PNG retained workspace from 0.2.12 directly into catalog generation without reopening Dragonwilds or repeating UnrealAssetScout exports.

### Changed

- Review the five building pieces added by the current client—Farming and Fishing skill tomes in flat and standing variants, plus the Ultra Compost Bin—and update building compatibility to 827 pieces, 844 references, and 827 authored icons.

## [0.2.12] - 2026-10-14

### Fixed

- Detect when Steam updated Dragonwilds after a resumable mapping or extraction was saved, before Zanik Manager starts the resume operation.
- Offer to safely discard only the stale Zanik workspace and immediately restart the guided extraction instead of failing inside PowerShell.
- Exclude cached asset bundles from guided reuse when their source executable does not match the currently installed client.

## [0.2.11] - 2026-10-14

### Fixed

- Keep the Zanik Manager Windows extractor runtime version in lockstep with extraction recipe version 5, preventing the packaged 0.2.10 tool from rejecting its own recipe immediately after mapping generation.
- Reuse the verified mapping when resuming this failure, without reopening Dragonwilds, while accurately reporting that UnrealAssetScout still needs to perform the catalog and artwork exports.
- Add a release-gating regression check that fails whenever the pipeline and extraction recipe versions are not bumped together.

## [0.2.10] - 2026-10-14

### Fixed

- Restore the seven missing web-map resource icons for Barrel Cactus, Bunny Cactus, Corpse Cotton, Desert Weed, Fibrous Pipe Cactus, Irit, and Salvage Metal using reviewed harvested-item identities.
- Extract explicit item artwork by full cooked package path so Fang Arrow and Poisoned Fang Arrow receive their authored recipe-card icon even though its texture does not use the `T_Icon_*` naming convention.
- Reject missing required resource artwork before bundling instead of silently publishing generated placeholders, and invalidate older incomplete asset caches with extraction recipe version 5.
- Package the corrected recipe in Zanik Manager and the standalone Windows asset tool so newly extracted bundles cannot reuse the affected older cache.

## [0.2.9] - 2026-10-13

### Fixed

- Make Zanik Manager restart a running Pterodactyl server after atomically activating a website asset bundle so native recipe and quest tracking load their runtime catalogs immediately.
- Install the bounded native query dispatcher when either default-on read-only player tracker is enabled, without enabling any mutation feature.
- Report recipe and quest dispatcher compatibility independently and fail closed when their query dispatcher is unavailable.
- Keep recipe-catalog object discovery and compatibility retries on the worker loop instead of allowing a full Unreal object scan on the game thread during startup.
- Clarify that self-hosted and manual Pterodactyl installations must restart Zanik after first asset activation or an asset update.

## [0.2.8] - 2026-10-13

### Fixed

- Publish Zanik Manager and both asset-tool archives to CurseForge as independent normal files instead of unsupported additional files attached to the server ZIP.
- Preserve partial-upload recovery for each CurseForge file while supplying the supported Dragonwilds game version on every independent upload.

## [0.2.7] - 2026-10-13

### Fixed

- Read extraction journals written by Windows PowerShell 5.1 with a UTF-8 byte-order mark and write future operational JSON as UTF-8 without a BOM.
- Allow Zanik Manager to recover completed, manifest-verified UnrealAssetScout exports when an interrupted journal remains at an earlier stage, avoiding another game launch or extraction pass.
- Reset only partial generated output during resume while preserving verified mappings, catalogs, textures, fonts, and recovery evidence.
- Automatically resume verified retained work from guided post-install asset setup instead of incorrectly attempting a new extraction.
- Read deeply nested Windows client exports through extended-length paths, and fail with the affected filename instead of silently omitting an unreadable JSON record or image.
- Remove owned extraction workspaces through extended-length paths so **Discard generated work** can clean deeply nested cooked exports.

## [0.2.6] - 2026-10-13

### Added

- Extract all 822 building icons from the client’s authored build-interface assets, including embedded `GeneratedIcons` and the Wintumber procedural thumbnail, without Blender or model rendering.
- Preserve validated extraction checkpoints and expose **Resume extraction** and **Discard generated work** actions in Zanik Manager.
- Record building-artwork and recipe-route coverage in generated asset manifests and diagnostics.

### Fixed

- Include the base and plugin progression tables needed to recover explicit recipe unlock providers.
- Replace unsupported “Available from the start” claims with the reviewed **Unlock trigger not yet identified** state and fail closed on provider drift.
- Pass pinned tool versions through a file instead of fragile inline PowerShell JSON.
- Move temporary-work cleanup into a bounded, ownership-checked Python operation so filesystem races become deferred warnings after valid bundles are produced.

## [0.2.5] - 2026-10-13

### Added

- Automatically mirror each new validated stable release from the authoritative GitLab pipeline to the public GitHub repository with retry-safe asset digest checks.
- Add a complete first-class self-hosted Linux installation, upgrade, diagnostics, and Windows-to-Linux asset-transfer workflow alongside Zanik Manager.
- Offer an optional post-install website setup step in Zanik Manager for the separately allocated TCP port and public URL before guided asset extraction begins.

### Changed

- Require source branches and annotated release tags to be pushed to GitLab first and then to the public GitHub mirror.
- Mark automatic compost equipment as incomplete and keep `FEATURE_QOL_AUTO_COMPOST` disabled until its native server integration and validation are complete.
- Clarify throughout the installation guides that Zanik's website port is separate from the Dragonwilds game port and must be opened or allocated by the operator.

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
