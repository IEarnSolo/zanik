# Changelog

All notable changes to Zanik are documented here.

## Unreleased

## [0.2.49] - 2026-10-26

### Fixed

- Normalize Zanik Manager's Windows CRLF removal lists before server-side asset-delta validation, so remove-only and mixed deltas delete the intended paths instead of rolling back with a file-set mismatch.
- Include the server helper's explicit rejection reason in Manager errors so future delta failures identify the failed validation stage immediately.

## [0.2.48] - 2026-10-26

- Normalize Windows executable extensions inside the asset wrapper so packaged Python runs synchronously with a reliable exit code even when Zanik Manager inherits a broken `PATHEXT` from its launcher.
- Refuse slow full-bundle uploads when a running server has a valid incremental base but an older launcher payload; identify the required server repair immediately and show the Manager version plus asset recipe in the window title.
- Reduce a tested one-icon Windows asset refresh to about 52 seconds for extraction/bundling and about 83 seconds through atomic activation, reusing a hash-validated private tree and rebuilding only the affected output group.
- Upload only changed asset members and activate them during one verified stop/start cycle, with complete server-side checksums, exact file-set validation, rollback, and no Python dependency in the game container.
- Avoid rescanning every retained catalog export when an extraction recipe only adds explicit texture targets, and remove redundant multipart generation, post-build extraction, and cache copies from Manager-driven updates.
- Exercise one-asset additions with temporary objective-arrow and clipboard extraction fixtures, then retire both fixtures in recipe 11 so the release validates a remove-only delta without shipping test artwork.
- Avoid fully parsing every retained catalog export for explicit texture removals when a fast exact-name scan proves the retired texture is not referenced; mixed add/remove updates share the same optimized path and still fall back safely for possible references.
- Restore native compatibility with Dragonwilds dedicated-server build `26161880`, including independently validated read-only, resource, teleport, storage, and gravestone paths.
- Retry one bounded SteamCMD validation pass inside guarded replace-executable game updates when the initial delta pass marks the managed launcher pathname for a full executable download.
- Run a quiet quick Doctor scan when Manager starts or the active server changes, persist the latest completed result per profile for immediate reuse, and refresh it after server-changing operations.
- Merge the active server's dedicated-game update result into Doctor so a current build is shown explicitly in green as up to date instead of appearing unchecked.
- Report server-payload and website-asset freshness separately from operational health in Doctor, so a working older installation remains Healthy while clearly identifying available updates and incremental asset refreshes.
- Add route-aware public nodestone tooltips with authoritative coordinates and authenticated admin map teleporting through Dragonwilds' native switchable-teleporter named-location pathway, guarded by independent compatibility and game-thread performance gates.
- Add public lodestone hover tooltips to the world map showing authoritative X/Y/Z coordinates. Dragonwilds does not persist historical placement ownership, so Zanik deliberately does not guess or introduce incomplete forward-only attribution.
- Replace the Dowdun Reach nodestone's generic fragment-crystal marker with detailed Zanik-authored top-down structure artwork, retain its full-resolution transparent master, and ship a crisp 256×256 map derivative with a revised browser cache key.
- Render the new nodestone marker at 12×12 pixels on the world map so its detailed artwork does not obscure nearby map information.
- Replace the single-line Manager progress popup with a stable two-level modal showing overall stage, active subtask, elapsed time, and available item counts without flooding Activity.
- Reuse validated generated output groups during incremental extraction, so a texture-only change such as the nodestone marker rebuilds only fixed map artwork while retaining byte-identical catalogs and unrelated images.
- Record per-group generator/input contracts and output digests in extraction state and asset manifests while continuing to validate the complete assembled bundle.
- Disable every Configuration-tab editor and action while settings are loading or saving, including automatic loads after changing the active server, and re-enable only the controls valid for the completed result.

## [0.2.47] - 2026-10-19

- Extract the fixed Dowdun Reach nodestone marker from its authoritative fragment-crystal navigation texture instead of emitting a blank fallback square, and revise the map URL so browsers refresh the corrected image.
- Retry Windows mapping launch through the discovered Steam executable when a Steam client update or restart silently loses the initial `steam://` game-launch request, while retaining bounded failure and exact loader restoration.
- Fix **Resume extraction** before mapping so it repeats loader-safety preflight and remembers that an existing UE4SS installation must be preserved and restored.
- Recover safely when a delayed Steam launch restores the user-owned UE4SS loader but leaves the mapping journal at `running`, allowing only known runtime log/state changes while rejecting operational loader drift.

### Changed

- Persist each server profile's read-only Files metadata index across profile switches and Zanik Manager restarts instead of expiring it after five minutes.
- Reindex Files only after a Zanik-changing server operation or an explicit **Refresh**; Manager-only updates do not invalidate remote metadata.
- Show when the Files index was last updated and enlarge its Explorer navigation controls.

## [0.2.46] - 2026-10-19

### Changed

- Give Zanik Manager's read-only Files browser familiar Explorer-style navigation buttons and native file and folder icons.
- Display recursively calculated sizes for indexed folders as well as individual files.
- Add a **Show raw server paths** toggle for switching between friendly Zanik area names and exact remote directories.
- Support mouse Back and Forward side buttons while navigating the Files tab.

## [0.2.45] - 2026-10-19

### Added

- Add Human-readable and Raw diagnostics views to Zanik Manager's Doctor, backed by one authoritative diagnostic result.
- Add a metadata-only Files tab with Explorer-style navigation across the selected server's Zanik installation, persistent data, backups, recovery data, release uploads, incoming assets, and verified managed launcher.
- Show per-area storage totals and a grand total while a bounded background index is built and cached for five minutes.

### Changed

- Present Doctor findings as clear installation, launcher, game-update, configuration, asset, backup, and local-storage sections with written status and suggested actions.
- Keep Files navigation read-only and exclude Dragonwilds saves, panel backups, unrelated files, symlinks, and the preserved official executable.

## [0.2.44] - 2026-10-19

### Added

- Add a Doctor **Storage and backups** inventory with explicit review and cleanup of recognized superseded Zanik recovery data on the selected server.
- Publish bounded backup-retention health through diagnostics and `/api/update-status` without exposing backup contents or secrets.

### Changed

- Keep one verified recovery point for each Zanik payload, Dragonwilds update, configuration, and uninstall backup family while protecting unresolved operations.
- Keep local Zanik Manager updates, validated bundles, incremental extraction caches, staging, and operation journals under explicit bounded retention rules.
- Consolidate replacement-mode Dragonwilds updates into one backup directory containing one prior official executable.

### Fixed

- Prevent repeated Manager repairs, configuration saves, game updates, CLI updates, and successful transaction journals from accumulating unbounded recovery copies.
- Avoid creating an empty server backup for a first-time Zanik installation.

## [0.2.43] - 2026-10-19

### Added

- Add a confirmed, installation-mode-aware **Update Dragonwilds server** workflow to Zanik Manager.
- Serve a safe maintenance website for unsupported Dragonwilds executables while leaving gameplay online and loading no Zanik hooks, scanners, dispatchers, or persistence workers.
- Publish reviewed Linux executable hashes in release metadata so Manager and the maintenance service can identify a compatible stable release.

### Changed

- Require replacement-mode installations to set and verify the Pterodactyl `AUTO_UPDATE` variable as `0`; wrapper installations retain their normal automatic-update preference.
- Explain the installation-mode tradeoffs and exact Pterodactyl Client API permissions in Manager and the public documentation.

### Fixed

- Prevent Pterodactyl Steam updates from silently overwriting a replacement-mode launcher by using a journaled one-shot updater that restores the launcher and verifies the game save tree was unchanged.

## [0.2.42] - 2026-10-17

### Fixed

- Restore read-only native compatibility with Dragonwilds dedicated-server build `25991736`, including validated Unreal globals, resource lifecycle tracking, storage display, and the bounded recipe/quest query dispatcher.
- Keep storage, gravestone, teleport, and unfinished automatic-compost mutations independently fail-closed on the new build until their complete native paths are separately revalidated.

### Changed

- Record the guarded game-update procedure and feature-specific compatibility evidence so Zanik Manager can safely distribute the October 17 dedicated-server compatibility update.

## [0.2.41] - 2026-10-17

### Fixed

- Force web-map clients to fetch the corrected lodestone marker instead of reusing the previously cached building-piece artwork under the same asset URL.
- Zanik Manager will identify this release as a server-payload update; existing extracted assets remain valid and do not need to be regenerated.

## [0.2.40] - 2026-10-17

### Added

- Make Zanik Manager upload only added and changed website asset files during compatible deployments, with an explicit removal list for retired files.
- Apply asset deltas through a trusted server-side helper that clones the active tree locally, proves the base and target manifests, validates the complete candidate, and atomically activates it.

### Changed

- Skip website asset uploads and server restarts entirely when the selected local bundle already matches the active server manifest.
- Retain full-bundle deployment as a compatibility fallback for missing, invalid, offline, or older installations that do not support atomic deltas.

### Fixed

- Avoid re-uploading thousands of unchanged website files after a targeted incremental extraction.

## [0.2.39] - 2026-10-17

### Changed

- Make Zanik Manager's retained Windows asset refreshes structurally incremental: unchanged catalog selectors, texture targets, and the web font now make no UnrealAssetScout calls.
- Export only newly added catalog selectors and texture targets, remove only retired cached outputs, and then regenerate and validate the complete atomic website bundle.
- Retain a versioned extraction-recipe selector history so caches created by earlier Managers can be upgraded without repeating unchanged broad scans.

### Fixed

- Stop an unchanged-client asset refresh from running the full catalog, texture, and font discovery passes merely because the operator requested an incremental rebuild.
- Remove empty retired texture batches without invalidating the remaining verified incremental cache.

## [0.2.38] - 2026-10-17

### Added

- Show website-asset compatibility as a separate status on the Zanik Manager Servers tab, including explicit guidance to update Manager and then refresh assets when a newer release changes the extraction contract.
- Publish the extraction recipe, reviewed compatibility profile, and recipe hash in release and Manager payload metadata so future Managers can distinguish Manager-only, server-payload, and asset-refresh updates.

### Changed

- Re-evaluate active asset compatibility after server selection, release update checks, and successful asset activation. Asset-only releases no longer imply that the Zanik server payload needs reinstalling.

## [0.2.37] - 2026-10-17

### Changed

- Left-align Zanik Manager's Changelog version headings and reduce the indentation of expanded release notes.

### Fixed

- Extract the authored `T_LodestoneMapIcon` texture for world-map lodestone markers instead of the similarly named building-piece icon. Extraction recipe 6 makes existing bundles eligible for a targeted incremental refresh.

## [0.2.36] - 2026-10-17

### Fixed

- Make Zanik Manager check the validated local bundle cache before starting a manual or incremental asset extraction. When the game and extraction contract are unchanged, Manager now selects the current bundle without launching Dragonwilds or UnrealAssetScout; an explicit troubleshooting rebuild remains available.

## [0.2.35] - 2026-10-17

### Changed

- Restyle Zanik Manager's Changelog sections as clean, borderless disclosure rows with compact text carets, subtle dividers, and indented release notes.

## [0.2.34] - 2026-10-17

### Fixed

- Retry Zanik Manager update-staging cleanup while the native updater completes its startup handshake, preventing a first-launch race from retaining otherwise recognized historical versions.

## [0.2.33] - 2026-10-17

### Added

- Add an explicit **Clear incremental cache** action that preserves validated bundles and server assets while warning that the next extraction will take longer.

### Fixed

- Zanik Manager now removes recognized download and extraction staging after a successful self-update instead of retaining every previous version under LocalAppData.
- Remove duplicate full ZIPs, temporary multipart uploads, generated web trees, private audit output, and extraction logs after a bundle is safely cached.
- Preserve only the raw JSON, textures, font, mapping, checkpoints, current bundle, and previous bundle needed for fast incremental extraction and rollback.

## [0.2.32] - 2026-10-17

### Added

- Add a Changelog tab to Zanik Manager with one collapsible section per included release and the newest release expanded by default.
- Add local timestamps to every message and technical error recorded in the Activity tab.

### Changed

- Bundle `CHANGELOG.md` with the portable Manager and hide release notes newer than the running Manager version.

## [0.2.31] - 2026-10-17

### Added

- Tell Zanik Manager users before and during asset setup that extraction can take several minutes.

### Fixed

- Stamp generated asset ZIPs and Pterodactyl upload parts with their real generation time instead of the ZIP-format 1980 epoch.

## [0.2.30] - 2026-10-17

### Fixed

- Run Zanik Manager-owned PowerShell and asset helper processes without a visible console window while preserving captured progress in Activity.

## [0.2.29] - 2026-10-16

### Added

- Zanik Manager now retains a private local UnrealAssetScout cache so later game and extraction-recipe updates export only changed, added, or removed targeted packages.
- Regenerate and fully validate a complete atomic server bundle after each incremental update.

### Fixed

- Require locally cached and remotely active bundles to match the Manager's exact extraction-recipe SHA-256, recipe version, and reviewed-profile contract before reuse.

## [0.2.28] - 2026-10-16

### Fixed

- Let Zanik Manager configuration descriptions use the complete available width and wrap only when the window requires it.
- Prevent the guided configuration view from creating a horizontal scrollbar.
- Use compact controls for the website port, public URL, and administrator cookie name while keeping their descriptions full-width.

## [0.2.27] - 2026-10-16

### Fixed

- Keep Zanik Manager toggle descriptions close to their checkboxes instead of separating them with elastic whitespace.
- Reserve the full wrapped height of guided configuration descriptions so the website-port and public-URL guidance is no longer clipped by following rows.

## [0.2.26] - 2026-10-16

### Changed

- Wrap every guided Zanik Manager configuration description to the available window width.
- Align continued boolean-description lines beneath the beginning of the description instead of beneath the checkbox.
- Show wrapped descriptions beneath numeric and text configuration fields instead of limiting that guidance to hover tooltips.

## [0.2.25] - 2026-10-16

### Changed

- Move the Zanik Manager server-payload compatibility line above the Pterodactyl panel details.
- Place **Open panel** and **Open website** directly after **Update / repair Zanik** and before recovery actions.
- Keep status labels color-coded while rendering their hover-tooltip text in white for consistent contrast, including game-update status tooltips.

## [0.2.24] - 2026-10-16

### Changed

- Color the Zanik server-payload compatibility line green when current and amber when an update is available, matching Zanik Manager's update indicators.
- Explain server-payload compatibility and required update actions in hover tooltips, including why an older Zanik release number may still be fully current.
- Place **Open panel** immediately before **Open website** on the Servers tab.

## [0.2.23] - 2026-10-16

### Added

- Show the configured public website URL and dedicated website port in the selected server's Zanik Manager information.
- Add an **Open website** action for configured public URLs.
- Add an **Edit servers** dialog for renaming, reordering, and removing local Manager profiles without modifying their Pterodactyl servers.

### Changed

- Combine asynchronous installation and configuration results into one stable server summary instead of allowing the last completed check to overwrite other information.

## [0.2.22] - 2026-10-16

### Added

- Track a deterministic operational server-payload contract independently from the Zanik Manager release version.
- Show whether the installed Zanik payload is current with the running Manager, requires an embedded update, or requires updating Manager first.

### Changed

- Display the installed Zanik version and installation mode on separate lines in the server summary.
- Keep **Update / repair Zanik** available for intentional same-contract repairs without implying that every Manager update requires a server reinstall.
- Publish verified server-payload contract metadata in installation, Manager payload, and release manifests.

## [0.2.21] - 2026-10-16

### Added

- Remember the last selected server and restore it when Zanik Manager starts.
- Validate preserved website assets against the packaged extraction recipe, reviewed compatibility profile, manifest coverage, and a bounded critical-file subset after server maintenance.

### Changed

- Separate the full **Install Zanik** workflow from **Update / repair Zanik**, which reuses the existing installation mode and preserves configuration, data, sessions, and compatible assets.
- Enable installation and maintenance actions only after Manager determines whether the selected server already has Zanik installed.
- After update or repair, offer website configuration or asset extraction only when the preserved configuration or assets actually require attention.

## [0.2.20] - 2026-10-16

### Fixed

- Archive an older failed Zanik Manager transaction when a newer operation of the same type succeeds for that server, preventing obsolete install failures from showing a recovery warning forever.
- Keep genuinely interrupted operations visible when they stopped in a critical in-progress state or have not been superseded by a later success.

## [0.2.19] - 2026-10-15

### Added

- Add the `ZANIK_ENABLED` master configuration switch, which starts the untouched official server without loading Zanik while preserving all installed files and persistent data.

### Fixed

- Make Zanik Manager game-build checks work before Zanik is installed by comparing the remote Steam app manifest with public dedicated-server metadata; results also report whether Zanik is installed or disabled.
- Treat missing configuration on an unmodded server as a normal state and automatically load configuration whenever the active server changes.
- Poll Manager and active-server updates every ten minutes while retaining quiet status-bar reporting and manual checks.

## [0.2.18] - 2026-10-15

### Fixed

- Accept a richer deterministic UnrealAssetScout raw export when a newly generated mapping resolves at least the reviewed package coverage; exact generated catalog counts and structural checks still fail closed on meaningful drift.
- Let Zanik Manager resume a retained JSON-only extraction by exporting only its remaining artwork and font inputs instead of repeating mapping generation or targeted catalog export.
- Update reviewed quest landmark and actor identities to the current cooked world data so all 230 quest relationships and 695 map markers validate against a fresh full export.

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
