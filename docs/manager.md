# Zanik Manager

The Servers tab shows the active server's Zanik compatibility, installation mode, public website URL, and dedicated website port. **Open website** opens the configured public URL in your default browser. It remains disabled until the server has a non-local public URL saved in its Zanik configuration.

The compatibility line appears directly below the server name and above its panel details. It uses the same status colors as the update indicators: green means the installed server-side files match this Manager, amber means an update is available, and gray means compatibility could not be proven. Hover over the line for a high-contrast explanation. Zanik's displayed release number can be older than the Manager version while still being current when a Manager-only release did not change any server-side files.

Use **Edit servers** beside the active-server selector to rename, reorder, or remove saved Manager profiles. Removing a profile affects only Zanik Manager on the current PC; it does not delete the Pterodactyl server or change any remote files.

The guided Configuration tab displays a responsive description for every setting. Descriptions use the complete available width and wrap only when necessary, without horizontal scrolling or clipping. Boolean descriptions remain directly beside their checkbox. Compact networking and cookie controls leave the remainder of the row available to their full-width guidance.

The **Changelog** tab shows the release notes bundled with the running portable Manager. Each version has its own lightweight disclosure row with a compact caret and subtle divider, newest first; the newest included version starts expanded. Notes from a newer release are never shown until that Manager version is installed. Every line written to **Activity**, including captured technical errors, is prefixed with the local date and time so operation reports can be correlated with panel and server logs.

Manager automatically removes recognized update-download staging after a successful self-update and deletes reproducible extraction output after the validated full ZIP has entered its bundle cache. It keeps the current and previous validated bundle plus the raw targeted JSON, textures, font, mapping, and checkpoints required for incremental extraction. Generated web trees, private audits, export logs, duplicate ZIPs, and temporary multipart files are not retained. **Clear incremental cache** on the Assets tab reclaims the remaining raw cache without changing cached bundles or any server; the next asset update then requires a slower full extraction.

At startup, cleanup waits for the native updater's successful handshake for up to 30 seconds. This keeps the fail-closed updater boundary without racing the process that is still finalizing an otherwise successful update. If completion cannot be confirmed, staging is preserved and cleanup retries on the next launch.

Zanik Manager is the recommended installation path for Windows administrators using Pterodactyl. Download `zanik-manager-windows-x86_64-VERSION.zip`, extract the complete folder, and open `ZanikManager.exe`. It includes the matching server package and Windows asset extractor and does not require Python, SSH, or administrator rights.

Asset extraction can take several minutes on the first run, and Manager states that before setup and throughout the modal operation. Its PowerShell coordinator runs as a hidden Manager worker. Progress and errors appear in the modal operation view and Activity tab; no separate console window needs to remain open. The standalone `init-assets.ps1` command continues to use the terminal from which an advanced operator launches it.

The bottom-right status bar quietly tracks two independent update states. **Manager** checks stable public GitHub releases shortly after startup and every ten minutes; **Check** refreshes it immediately, and **Update and relaunch** appears only after the exact archive, release manifest, size, and SHA-256 digest have been validated. The updater swaps the portable directory transactionally, waits for the new GUI to start, and restores the prior copy if startup fails. Profiles, credentials, cached bundles, and operation journals remain under `%LOCALAPPDATA%` and are not part of that swap.

The updater runs from `%LOCALAPPDATA%\Zanik Manager\updates\`, outside the portable installation it replaces. Transient antivirus or filesystem locks are retried briefly. If an update still fails, the existing installation is preserved or restored and `self-update-journal.json.status` identifies the failed operation and Windows error code for support.

**Game (server name)** checks only the active Pterodactyl profile, shortly after selection and every ten minutes. It reads Linux dedicated-server app `4019830`'s installed build from the server's Steam manifest and compares it with public metadata, even before Zanik is installed. When newer, **Update Dragonwilds server** asks once, then uses the recorded installation mode. It never updates every configured server implicitly.

## First run

1. Enter the Pterodactyl panel URL and a **Client API key** from your panel account.
2. Select one of the Dragonwilds servers visible to that account and give the profile a friendly name.
3. Review the detected installation and choose wrapper or executable-replacement mode.
4. Optionally configure the Zanik website port and public URL. The port must be a separate TCP allocation, not the Dragonwilds game port. **Save and continue** writes the validated settings and restarts the server; **Skip for now** continues without changing them.
5. If the server has no healthy website assets, accept **Set up website assets now**. Manager will validate a contract-compatible cached bundle, incrementally update a completed private extraction cache, resume verified incomplete work, or start a new extraction before it uploads, activates, and verifies the assets on that exact server. Resume never repeats completed stages. Choose **Later** to use the Assets tab manually.
6. Run **Doctor** before joining the server.

API keys are stored in Windows Credential Manager. They are never written to `profiles.json`, logs, diagnostics, or support ZIPs. Before sending a key to a remote panel over plain HTTP, the Manager presents an explicit confirmation and then keeps a compact warning on that profile because the traffic can be intercepted. HTTPS and loopback HTTP connections do not show the warning. Certificate validation cannot be bypassed.

If the panel's normal file-contents route reports a transient node-side `5xx` error, the Manager retries the read through Pterodactyl's signed download route. If both routes fail, the dialog shows a concise redacted error and places technical details in the Activity tab.

Long-running operations use an application-modal progress window and record each stage in **Activity**. This prevents another server mutation from starting while an install, update, asset operation, or diagnostic is still running. Guided asset setup is a separate transaction: a mapping, extraction, or upload failure never rolls back a successful server installation, and a completed local bundle remains cached for Retry.

The Servers tab separates first installation from maintenance. **Install Zanik** is enabled only when the selected server is unmodded and runs the complete mode, website, and asset setup workflow. For an installed server, the summary reports `Zanik INSTALLED_VERSION · Up to date with Manager MANAGER_VERSION` or identifies the required update, followed by a separate `Install mode: MODE` line. Manager compares the deterministic operational server-payload contract rather than release numbers, so a Manager-only or documentation-only release does not require reinstalling an unchanged server mod.

Website assets have an independent status line and contract. Manager compares the active bundle's extraction-recipe version, reviewed compatibility profile, recipe hash, and critical files. When a newer GitHub release changes only that contract, Manager says to update Manager and then refresh website assets; it does not tell the operator to reinstall an unchanged Zanik server payload. When possible, that refresh uses the retained incremental extraction cache.

**Update / repair Zanik** remains enabled for an existing installation, reuses its recorded mode, and preserves configuration, authorization, persistent data, sessions, and active assets. When contracts match it performs an intentional verified repair; when the embedded contract differs it performs an update. If a newer stable Manager contains a different server contract, the summary instructs the operator to update Manager first and then update Zanik. It never updates Dragonwilds.

After update or repair, Manager checks the preserved networking configuration without contacting the public website. It also validates the active asset manifest against the packaged extraction-recipe SHA-256, recipe version, and reviewed-profile version, then hashes only the critical representative files. Matching assets are retained without prompting; missing, incompatible, stale-contract, or critically damaged assets produce an offer to update and deploy a bundle. Cached final bundles must match that exact contract before reuse. Full remote per-file verification remains a deep Doctor operation.

The last selected server is restored from `%LOCALAPPDATA%\Zanik Manager\preferences.json` on startup. This preference contains only the stable local profile identifier; API keys remain exclusively in Windows Credential Manager.

The Assets tab always shows its explicit **Target server**. Before a manual upload, Manager confirms the bundle, profile, panel, and server identifier so changing the active server cannot silently redirect an upload. After a standalone extraction succeeds, Manager offers **Upload and activate now** for that displayed target; choosing **Later** keeps the validated bundle in the local cache. Cached bundle entries include the time they were validated, the client build, and a manifest fingerprint.

Every primary action and guided-workflow button has hover help explaining what it will inspect or change. This is especially useful before power-changing, upload, repair, or discard actions.

## Installation modes

Wrapper mode remains recommended and is selected automatically for a recognized egg with a safe editable startup variable. If an egg cannot be changed through the Client API, the manager shows the exact manual startup action and waits for verification.

Executable replacement is available for fixed eggs. It preserves the verified official ELF as `.zanik-original`, displays the core-file warning, requires explicit acknowledgement, and requires Manager to set and read back `AUTO_UPDATE=0`. Missing `startup.read`, `startup.update`, an absent variable, or a read-only variable blocks installation with remediation guidance.

Wrapper updates preserve the existing automatic-update preference. Replacement updates run a guarded one-shot SteamCMD transaction, restore the launcher, and compare the save-tree fingerprint before restarting. If the new executable is unsupported, the game remains online while Zanik serves a maintenance page and loads no native integration. [Game-update safety](game-update-safety.md) documents the complete boundary.

Some Steam updates restore the official executable at the fixed startup path. After Manager restarts a replacement-mode server for asset activation, it verifies that the small Zanik launcher is still present. If Steam displaced it, Manager leaves the successfully activated assets alone and offers **Repair launcher now** for that exact server. Repair preserves the newly updated official executable, configuration, persistent state, authorization data, and assets.

Every mutating operation stages and verifies files before activation and records a recovery journal. Backups are stored under `/home/container/zanik-backups/manager/`. An interrupted operation is offered for resume or rollback the next time the profile connects.

Doctor's **Storage and backups** section reports local Manager storage and shallowly inventories only Zanik's recognized backup roots on the selected server. Zanik retains one verified recovery point for each payload, Dragonwilds-update, configuration, and uninstall family. A newer verified operation removes older successful copies in that family; failed or interrupted recovery data remains protected until it is recovered or safely superseded. **Review storage cleanup** lists every eligible path before deletion. It never scans or removes Dragonwilds saves, Pterodactyl backups, `zanik-data`, active assets, release uploads, incoming asset uploads, pinned bundles, or unrecognized paths.

A failed local operation journal is automatically archived when a newer operation of the same type completes successfully for that server. This prevents obsolete failures from continuing to request recovery while retaining the journal for audit. Journals left in a critical in-progress state remain visible until explicitly recovered.

Completed local journals are compacted to the newest record per server and operation type. Superseded failures and rolled-back records are removed after they no longer provide recovery value.

Asset activation uses a fast remote verification path: multipart archives are hash-checked before extraction, Pterodactyl's ZIP CRC checks protect their entries, and Manager downloads only the manifest, runtime catalogs, world map, and representative files before the atomic swap. It does not download every extracted asset during a normal install or development iteration. Use **Run deep Doctor** only when corruption is suspected or a complete remote audit is specifically required.

## Assets and local files

Asset deployment is incremental when both the current server helper and a valid active manifest are available. Manager reads one small remote manifest and uploads only changed or added files plus removals. If the manifests are identical, it skips both upload and restart. The server performs complete validation locally and returns one small report; Manager does not scan or download the remote tree file by file. Full-bundle upload remains the safe fallback for older or offline installations.

Validated bundles are stored below `%LOCALAPPDATA%\Zanik Manager\assets\`. The newest two unpinned bundles for each game installation are retained. Use **Open bundle folder** to inspect or copy a ZIP. Interrupted extraction checkpoints live under `%LOCALAPPDATA%\Zanik Manager\asset-work\`; use **Resume extraction** to continue from the last verified stage or **Discard generated work** to remove only the journal-owned workspace. Raw exports are removed after successful packaging.

Before a manual extraction, Manager checks those validated bundles against the selected client's executable fingerprint and the packaged extraction-recipe SHA-256, recipe version, and reviewed compatibility profile. An exact match is selected immediately without launching Dragonwilds or UnrealAssetScout. **Run extraction anyway** remains available only as an explicit troubleshooting choice. When that rebuild uses an unchanged client, the retained selector inventory skips unchanged catalog, texture, and font exporter passes entirely. Recipe changes export only added selectors/targets and remove retired cached outputs. A changed game executable still requires broad discovery across Zanik's bounded recipe because existing packages may have changed internally.

Changelog releases use compact, left-aligned disclosure headings. Expanded notes sit just inside their heading rather than using a deep nested indent.

Resume accepts journals created by both Windows PowerShell 5.1 and PowerShell 7. If targeted JSON export finished but later export stages did not, Manager verifies that retained JSON and continues with artwork and font export without reopening Dragonwilds or repeating the catalog pass. If UnrealAssetScout finished every export, Manager verifies every expected batch and resumes catalog generation. This recovery is also automatic in the fresh-server guided setup flow, which continues through upload and activation on the pinned server. Manager never infers completion from directory existence alone.

Manager handles deeply nested exports with Windows extended-length paths. If any retained JSON or artwork cannot be read, generation fails with the affected filename rather than publishing a partial catalog. Corrected builds can resume the same retained exports after such a failure.
The **Discard generated work** action uses the same long-path support and removes only a workspace carrying Zanik's ownership marker.

Dragonwilds must be closed when extraction starts. If it is running, Manager can request a normal close and wait; it never force-kills the client. Mapping generation briefly opens and closes the game automatically. If the client already uses UE4SS or another `dwmapi.dll` proxy loader, Manager asks first, backs up and verifies the complete installation outside the game directory, temporarily installs Zanik's pinned mapper, and restores the original files exactly. Persistent recovery records live under `%LOCALAPPDATA%\Zanik Manager\mapping-recovery\` so an interrupted run can be safely restored on the next attempt.

The extractor copies authored build-interface thumbnails from the client, including embedded thumbnails in special `GeneratedIcons` packages. It does not render models and does not install or require Blender.

## Configuration and support

When Manager starts or the active profile changes, it automatically loads that server's configuration and clears the previous server's editable state first. If Zanik is not installed, Manager reports that normally instead of presenting a file-download failure; install it and the settings will load automatically. The Configuration tab validates strict `KEY=VALUE` data before upload, refuses to overwrite a file changed since it was loaded, creates a backup, and asks whether to restart. If the post-install website step was skipped, select the server, open **Configuration**, and edit the **Networking** section. `STATUS_PORT` is Zanik's separately allocated TCP port; `PUBLIC_BASE_URL` is the address users will open. `ZANIK_ENABLED` is the master switch; turning it off preserves the installation and data but starts the official server without loading the mod. State-changing and QoL features remain off by default.

After a validated asset bundle is activated, Zanik Manager restarts the target when it is currently running. This startup reload is required for native player recipe and quest tracking; an offline target simply loads the catalogs on its next start. Manual asset uploads show this restart in the target confirmation and operation progress.

Doctor can perform a quick installation check or a deliberately slower complete remote asset verification. Deep Doctor normally asks Zanik's launcher to hash the active tree on the server and downloads one aggregate report. Older installations that lack the helper can fall back to the much slower per-file API audit; that fallback should be used only when a complete audit is explicitly needed. Neither deep path is part of normal activation. Support ZIPs are generated locally and can be reviewed before sharing. They exclude API keys, console secrets, claims, sessions, authorization files, and game-derived assets.

## Windows trust warning

The initial portable build is not code-signed, so Windows may show a SmartScreen warning. Verify the archive against the published `SHA256SUMS`. Future builds can be signed in CI without changing the portable layout.

When Steam updates Dragonwilds during a paused asset extraction, the Manager will refuse to reuse the old mapping. It offers **Discard stale work and start fresh**, removes only the marked Zanik extraction workspace, and returns to the guided extraction flow. Existing validated cached bundles remain available, but a bundle produced from a different client executable is not automatically offered for a new installation.
