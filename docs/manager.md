# Zanik Manager

Zanik Manager is the recommended installation path for Windows administrators using Pterodactyl. Download `zanik-manager-windows-x86_64-VERSION.zip`, extract the complete folder, and open `ZanikManager.exe`. It includes the matching server package and Windows asset extractor and does not require Python, SSH, or administrator rights.

The bottom-right status bar quietly tracks two independent update states. **Manager** checks stable public GitHub releases shortly after startup and every six hours; **Check** refreshes it immediately, and **Update and relaunch** appears only after the exact archive, release manifest, size, and SHA-256 digest have been validated. The updater swaps the portable directory transactionally, waits for the new GUI to start, and restores the prior copy if startup fails. Profiles, credentials, cached bundles, and operation journals remain under `%LOCALAPPDATA%` and are not part of that swap.

The updater runs from `%LOCALAPPDATA%\Zanik Manager\updates\`, outside the portable installation it replaces. Transient antivirus or filesystem locks are retried briefly. If an update still fails, the existing installation is preserved or restored and `self-update-journal.json.status` identifies the failed operation and Windows error code for support.

**Game (server name)** checks only the active Pterodactyl profile. A running current Zanik launcher asks that server's own SteamCMD installation whether Linux dedicated-server app `4019830` has a newer public build. This is read-only: it never runs `app_update`, changes files, or stops the server. An offline server shows its last cached result when available. If an update exists, install it through the host or Steam workflow, then run Doctor and revalidate compatibility and assets. Windows-client and Linux-server build IDs are intentionally not compared.

## First run

1. Enter the Pterodactyl panel URL and a **Client API key** from your panel account.
2. Select one of the Dragonwilds servers visible to that account and give the profile a friendly name.
3. Review the detected installation and choose wrapper or executable-replacement mode.
4. Optionally configure the Zanik website port and public URL. The port must be a separate TCP allocation, not the Dragonwilds game port. **Save and continue** writes the validated settings and restarts the server; **Skip for now** continues without changing them.
5. If the server has no healthy website assets, accept **Set up website assets now**. Manager will validate a compatible cached bundle, resume any verified retained extraction, or start a new extraction before it uploads, activates, and verifies the assets on that exact server. Resume never repeats mapping generation; it continues any extraction stages that were not already completed. Choose **Later** to use the Assets tab manually.
6. Run **Doctor** before joining the server.

API keys are stored in Windows Credential Manager. They are never written to `profiles.json`, logs, diagnostics, or support ZIPs. Before sending a key to a remote panel over plain HTTP, the Manager presents an explicit confirmation and then keeps a compact warning on that profile because the traffic can be intercepted. HTTPS and loopback HTTP connections do not show the warning. Certificate validation cannot be bypassed.

If the panel's normal file-contents route reports a transient node-side `5xx` error, the Manager retries the read through Pterodactyl's signed download route. If both routes fail, the dialog shows a concise redacted error and places technical details in the Activity tab.

Long-running operations use an application-modal progress window and record each stage in **Activity**. This prevents another server mutation from starting while an install, update, asset operation, or diagnostic is still running. Guided asset setup is a separate transaction: a mapping, extraction, or upload failure never rolls back a successful server installation, and a completed local bundle remains cached for Retry.

The Servers-tab **Update Zanik server** action updates only the Linux Zanik payload embedded in the current Manager. It does not update the Manager application or Dragonwilds.

The Assets tab always shows its explicit **Target server**. Before a manual upload, Manager confirms the bundle, profile, panel, and server identifier so changing the active server cannot silently redirect an upload. After a standalone extraction succeeds, Manager offers **Upload and activate now** for that displayed target; choosing **Later** keeps the validated bundle in the local cache. Cached bundle entries include the time they were validated, the client build, and a manifest fingerprint.

Every primary action and guided-workflow button has hover help explaining what it will inspect or change. This is especially useful before power-changing, upload, repair, or discard actions.

## Installation modes

Wrapper mode remains recommended and is selected automatically for a recognized egg with a safe editable startup variable. If an egg cannot be changed through the Client API, the manager shows the exact manual startup action and waits for verification.

Executable replacement is available for fixed eggs. It preserves the verified official ELF as `.zanik-original`, displays the core-file warning, and requires explicit acknowledgement.

Some Steam updates restore the official executable at the fixed startup path. After Manager restarts a replacement-mode server for asset activation, it verifies that the small Zanik launcher is still present. If Steam displaced it, Manager leaves the successfully activated assets alone and offers **Repair launcher now** for that exact server. Repair preserves the newly updated official executable, configuration, persistent state, authorization data, and assets.

Every mutating operation stages and verifies files before activation and records a recovery journal. Backups are stored under `/home/container/zanik-backups/manager/`. An interrupted operation is offered for resume or rollback the next time the profile connects.

Asset activation uses a fast remote verification path: multipart archives are hash-checked before extraction, Pterodactyl's ZIP CRC checks protect their entries, and Manager downloads only the manifest, runtime catalogs, world map, and representative files before the atomic swap. It does not download every extracted asset during a normal install or development iteration. Use **Run deep Doctor** only when corruption is suspected or a complete remote audit is specifically required.

## Assets and local files

Validated bundles are stored below `%LOCALAPPDATA%\Zanik Manager\assets\`. The newest two unpinned bundles for each game installation are retained. Use **Open bundle folder** to inspect or copy a ZIP. Interrupted extraction checkpoints live under `%LOCALAPPDATA%\Zanik Manager\asset-work\`; use **Resume extraction** to continue from the last verified stage or **Discard generated work** to remove only the journal-owned workspace. Raw exports are removed after successful packaging.

Resume accepts journals created by both Windows PowerShell 5.1 and PowerShell 7. If a journal update was interrupted after UnrealAssetScout finished, Manager verifies every expected export batch and resumes catalog generation without reopening Dragonwilds. This recovery is also automatic in the fresh-server guided setup flow, which continues through upload and activation on the pinned server. Manager never infers completion from directory existence alone.

Manager handles deeply nested exports with Windows extended-length paths. If any retained JSON or artwork cannot be read, generation fails with the affected filename rather than publishing a partial catalog. Corrected builds can resume the same retained exports after such a failure.
The **Discard generated work** action uses the same long-path support and removes only a workspace carrying Zanik's ownership marker.

Dragonwilds must be closed when extraction starts. If it is running, Manager can request a normal close and wait; it never force-kills the client. Mapping generation briefly opens and closes the game automatically. If the client already uses UE4SS or another `dwmapi.dll` proxy loader, Manager asks first, backs up and verifies the complete installation outside the game directory, temporarily installs Zanik's pinned mapper, and restores the original files exactly. Persistent recovery records live under `%LOCALAPPDATA%\Zanik Manager\mapping-recovery\` so an interrupted run can be safely restored on the next attempt.

The extractor copies authored build-interface thumbnails from the client, including embedded thumbnails in special `GeneratedIcons` packages. It does not render models and does not install or require Blender.

## Configuration and support

When Manager starts with a saved server selected, it automatically loads that server's configuration. Changing profiles clears the prior editable state so it cannot be saved to the wrong server; use **Load from server** to load the newly selected profile. The Configuration tab validates strict `KEY=VALUE` data before upload, refuses to overwrite a file changed since it was loaded, creates a backup, and asks whether to restart. If the post-install website step was skipped, select the server, open **Configuration**, load its configuration, and edit the **Networking** section. `STATUS_PORT` is Zanik's separately allocated TCP port; `PUBLIC_BASE_URL` is the address users will open. State-changing and QoL features remain off by default.

After a validated asset bundle is activated, Zanik Manager restarts the target when it is currently running. This startup reload is required for native player recipe and quest tracking; an offline target simply loads the catalogs on its next start. Manual asset uploads show this restart in the target confirmation and operation progress.

Doctor can perform a quick installation check or a deliberately slower complete remote asset verification. Deep Doctor normally asks Zanik's launcher to hash the active tree on the server and downloads one aggregate report. Older installations that lack the helper can fall back to the much slower per-file API audit; that fallback should be used only when a complete audit is explicitly needed. Neither deep path is part of normal activation. Support ZIPs are generated locally and can be reviewed before sharing. They exclude API keys, console secrets, claims, sessions, authorization files, and game-derived assets.

## Windows trust warning

The initial portable build is not code-signed, so Windows may show a SmartScreen warning. Verify the archive against the published `SHA256SUMS`. Future builds can be signed in CI without changing the portable layout.

When Steam updates Dragonwilds during a paused asset extraction, the Manager will refuse to reuse the old mapping. It offers **Discard stale work and start fresh**, removes only the marked Zanik extraction workspace, and returns to the guided extraction flow. Existing validated cached bundles remain available, but a bundle produced from a different client executable is not automatically offered for a new installation.
