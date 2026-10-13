# CurseForge listing copy

The project description and reviewer notes below are maintained as ready-to-paste copy for the CurseForge author dashboard. Keep the download guidance near the top of the public description because CurseForge treats the manual Linux server ZIP as the primary file while most Windows administrators should use the additional Zanik Manager download.

## Project description

```markdown
# Zanik

Zanik is a server-side mod for RuneScape: Dragonwilds dedicated servers. It adds a web interface for player, world, map, recipe, quest, resource, and NPC information, along with optional administration and quality-of-life features. Players joining the server do not need to install anything.

## Which file should I download?

**Most server owners should download `Zanik Manager - Recommended Windows Download`.** Extract the ZIP on your Windows PC and run `ZanikManager.exe`. It can install and update Zanik on a Linux Pterodactyl server, edit configuration, run diagnostics, build the required web assets from your licensed game client, and upload them for you.

Use `Zanik - Linux Dedicated Server` only if you want to install the server files manually. The standalone Windows and Linux asset-tool downloads are intended for advanced command-line workflows; the Windows extractor is already included with Zanik Manager.

Zanik Manager is a Windows administration tool, not a Windows server loader. The Zanik server mod currently supports Linux x86-64 dedicated servers.

## Installation and safety

Wrapper installation is recommended and leaves the official game executable untouched. A clearly marked executable-replacement compatibility mode is available for fixed hosting-panel setups that cannot change their startup command.

Read-only features are enabled by default. Teleports, storage changes, recipe administration, and gameplay-changing QoL features remain disabled until the server owner enables them.

Documentation and source code: https://github.com/IEarnSolo/zanik
```

## Reviewer notes

```text
Zanik is a server-only native Linux x86-64 mod for RuneScape: Dragonwilds dedicated servers. It uses a process-scoped LD_PRELOAD library and does not use UE4SS. We selected UE4SS Mods > Utilities only because there is currently no native Linux dedicated-server mod category.

The recommended wrapper installation does not modify the official game executable. An optional, explicitly warned compatibility mode can preserve the official executable and place a managed launcher at its path for hosting panels with fixed startup commands.

The Windows Zanik Manager is an administration client for Linux servers and includes the local asset extractor. It does not modify a player's client for normal gameplay. Generated game assets, raw game containers, mappings, and extraction keys are not redistributed. Users generate the web asset bundle locally from their own licensed Dragonwilds installation.

All mutation and gameplay-changing QoL features are disabled by default. Source and documentation are public at https://github.com/IEarnSolo/zanik.
```

## File labels

The automated upload job uses these labels:

- `Zanik VERSION - Linux Dedicated Server` — primary manual server ZIP.
- `Zanik Manager - Recommended Windows Download VERSION` — recommended download for most server owners.
- `Asset Tools - Windows x86_64 VERSION` — advanced standalone extractor.
- `Asset Tools - Linux x86_64 VERSION` — advanced Linux inventory/catalog tooling.
