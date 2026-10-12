# Changelog

All notable changes to Zanik are documented here.

## [0.1.1] - 2026-10-12

### Fixed

- Safely migrate development servers that use Zanik's legacy Pterodactyl executable-replacement launcher without overwriting the preserved official ELF.
- Report enabled features as compatible or incompatible from their live subsystem diagnostics, while retaining an explicit disabled state for gated features.

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
