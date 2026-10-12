# Distribution architecture rules

- Linux x86-64 uses a native process-scoped `LD_PRELOAD` library. Wrapper mode is canonical; managed executable replacement is compatibility-only.
- Read-only features default on. Every state-changing or gameplay QoL feature owns an off-by-default gate and must not initialize native work when disabled.
- New native or operational dependencies must update compatibility analysis, runtime validation, diagnostics, backups, reports, positive/negative tests, and safe feature-local failure behavior.
- Hooks and dispatchers are latency-critical: no I/O, waits, unbounded scans, or blocking locks on the game thread. Observe per-operation timing, queue depth, drops, cache state, and slow calls; preserve the aggregate tick budget.
- Game assets and containers never enter Git or core releases. Linux dedicated-server containers are inventory-only. The self-contained Windows client tool owns mapping generation, targeted extraction, catalog generation, reviewed drift checks, full/multipart packaging, and atomic server activation.
- Never broaden extraction to the complete cooked tree. Keep recipes versioned and restricted to runtime catalogs plus artwork reachable from Zanik's web application. Treat `.usmap`, raw exports, generated bundles, and temporary UE4SS files as local game-derived data.
- Temporary mapping generation must refuse pre-existing loader collisions, never force-kill the client, recover from its journal before new work, and verify that every touched client path is restored.
- A bundle with unreviewed catalog drift is a candidate marked `incompatible`; installers must reject it until the profile is reviewed. Build-ID differences alone are informational.
- The full ZIP and Pterodactyl parts must assemble to the same file/hash tree. Part installation rejects missing, unexpected, corrupt, duplicate, traversal, and symlink entries before atomic activation.
- Keep clean-clone tests asset-free. Validate every release archive after clean extraction for layout, modes, checksums, credentials, raw game files, operator files, and research data.
- Maintain `.tmp/distribution-implementation.md` during distribution work; it is local, ignored, and must never be committed.
- Treat `VERSION` as authoritative and publish stable releases only from matching protected `vMAJOR.MINOR.PATCH` tags. Tagged archives must pass clean extraction and checksum validation before durable publication.
- Use GitLab Generic Packages and Releases for permanent binaries; pipeline artifacts are handoff and secondary retention only. Never add release or CurseForge credentials to Git or job output.
- Maintain an ignored `.tmp/release-implementation.md` checklist while changing release automation.
- Maintain an ignored `.tmp/windows-extractor-implementation.md` checklist while changing or validating the Windows asset pipeline.
- Zanik Manager is a portable PySide6 Windows application. Keep hosting operations behind the provider interface, keep Pterodactyl credentials exclusively in Windows Credential Manager, and make every remote mutation journaled, hash-verified, recoverable, and explicitly scoped to one selected server.
- The manager archive embeds the exact server ZIP and Windows asset-tool ZIP published beside it. Release validation must prove those nested payload hashes and reject credentials, game assets, mappings, raw exports, operator files, or developer-machine paths.
- Maintain an ignored `.tmp/windows-manager-implementation.md` checklist while changing or validating the manager.
