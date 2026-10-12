# Distribution architecture rules

- Linux x86-64 uses a native process-scoped `LD_PRELOAD` library. Wrapper mode is canonical; managed executable replacement is compatibility-only.
- Read-only features default on. Every state-changing or gameplay QoL feature owns an off-by-default gate and must not initialize native work when disabled.
- New native or operational dependencies must update compatibility analysis, runtime validation, diagnostics, backups, reports, positive/negative tests, and safe feature-local failure behavior.
- Hooks and dispatchers are latency-critical: no I/O, waits, unbounded scans, or blocking locks on the game thread. Observe per-operation timing, queue depth, drops, cache state, and slow calls; preserve the aggregate tick budget.
- Game assets and containers never enter Git or core releases. Asset bundles are locally generated, fingerprinted, validated, and atomically activated.
- Keep clean-clone tests asset-free. Validate every release archive after clean extraction for layout, modes, checksums, credentials, raw game files, operator files, and research data.
- Maintain `.tmp/distribution-implementation.md` during distribution work; it is local, ignored, and must never be committed.
- Treat `VERSION` as authoritative and publish stable releases only from matching protected `vMAJOR.MINOR.PATCH` tags. Tagged archives must pass clean extraction and checksum validation before durable publication.
- Use GitLab Generic Packages and Releases for permanent binaries; pipeline artifacts are handoff and secondary retention only. Never add release or CurseForge credentials to Git or job output.
- Maintain an ignored `.tmp/release-implementation.md` checklist while changing release automation.
