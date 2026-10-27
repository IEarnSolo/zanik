# Zanik agent instructions

## Remote server development hygiene

- Apply these rules to every server, including development, staging, and production servers.
- Prefer Zanik Manager's managed backup and recovery system for operations it supports. Do not create a parallel ad-hoc backup merely for convenience.
- If development work requires an ad-hoc backup, temporary copy, staged payload, diagnostic file, or rollback directory outside Zanik Manager's managed backup system, record its exact remote path and purpose while the task is active.
- After the change and rollback boundary have been verified, delete every ad-hoc remote artifact created for that task before declaring the work complete. Verify that each exact path is gone.
- If the task fails or rollback is still unresolved, retain only the material required for recovery and report its exact path and reason; clean it up as soon as recovery is completed or superseded.
- Never treat Dragonwilds saves, Pterodactyl backups, live `zanik-data`, active assets, release uploads, incoming user assets, pinned bundles, or unrecognized paths as disposable development artifacts.
- Do not manually remove Zanik Manager-managed backups or recovery data solely because they look old. Inspect ownership metadata and use the Manager's retention or reviewed-cleanup workflow unless the user explicitly requests otherwise.
