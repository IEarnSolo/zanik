# Zanik Manager

Zanik Manager is the recommended installation path for Windows administrators using Pterodactyl. Download `zanik-manager-windows-x86_64-VERSION.zip`, extract the complete folder, and open `ZanikManager.exe`. It includes the matching server package and Windows asset extractor and does not require Python, SSH, or administrator rights.

## First run

1. Enter the Pterodactyl panel URL and a **Client API key** from your panel account.
2. Select one of the Dragonwilds servers visible to that account and give the profile a friendly name.
3. Review the detected installation and choose wrapper or executable-replacement mode.
4. Use **Assets** to locate the licensed Windows client, generate a validated bundle, and upload it.
5. Run **Doctor** before joining the server.

API keys are stored in Windows Credential Manager. They are never written to `profiles.json`, logs, diagnostics, or support ZIPs. A remote panel using plain HTTP requires an explicit acknowledgement because the API key can be intercepted in transit.

## Installation modes

Wrapper mode remains recommended and is selected automatically for a recognized egg with a safe editable startup variable. If an egg cannot be changed through the Client API, the manager shows the exact manual startup action and waits for verification.

Executable replacement is available for fixed eggs. It preserves the verified official ELF as `.zanik-original`, displays the core-file warning, and requires explicit acknowledgement.

Every mutating operation stages and verifies files before activation and records a recovery journal. Backups are stored under `/home/container/zanik-backups/manager/`. An interrupted operation is offered for resume or rollback the next time the profile connects.

Asset activation uses a fast remote verification path: multipart archives are hash-checked before extraction, Pterodactyl's ZIP CRC checks protect their entries, and Manager downloads only the manifest, runtime catalogs, world map, and representative files before the atomic swap. It does not download every extracted asset during a normal install or development iteration. Use **Run deep Doctor** only when corruption is suspected or a complete remote audit is specifically required.

## Assets and local files

Validated bundles are stored below `%LOCALAPPDATA%\Zanik Manager\assets\`. The newest two unpinned bundles for each game installation are retained. Use **Open bundle folder** to inspect or copy a ZIP. Raw game containers, mappings, and extraction work trees are not retained by default.

## Configuration and support

The Configuration page validates strict `KEY=VALUE` data before upload, refuses to overwrite a file changed since it was loaded, creates a backup, and asks whether to restart. State-changing and QoL features remain off by default.

Doctor can perform a quick installation check or a deliberately slower complete remote asset verification. The deep check downloads and hashes every manifest entry and is not part of normal activation. Support ZIPs are generated locally and can be reviewed before sharing. They exclude API keys, console secrets, claims, sessions, authorization files, and game-derived assets.

## Windows trust warning

The initial portable build is not code-signed, so Windows may show a SmartScreen warning. Verify the archive against the published `SHA256SUMS`. Future builds can be signed in CI without changing the portable layout.
