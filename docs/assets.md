# Local game assets

Zanik does not distribute Dragonwilds containers, mappings, extracted trees, or generated game artwork. The Linux dedicated server is an inventory source only; it cannot create the complete web bundle. Build the bundle on a Windows PC that has a licensed Dragonwilds client, then upload it to the server.

## Build on Windows

Extract `zanik-asset-tools-windows-x86_64-VERSION.zip`, open PowerShell in that directory, close Dragonwilds, and run:

```powershell
.\init-assets.ps1 `
  -GameRoot "E:\SteamLibrary\steamapps\common\RSDragonwilds" `
  -OutputDirectory ".\output"
```

The standalone command refuses to overwrite an existing UE4SS or proxy-loader installation by default. Zanik Manager is the recommended path when another loader is present because it presents the safety confirmation and keeps persistent recovery state. Advanced standalone use may opt in explicitly:

```powershell
.\init-assets.ps1 `
  -GameRoot "E:\SteamLibrary\steamapps\common\RSDragonwilds" `
  -OutputDirectory ".\output" `
  -PreserveExistingLoader `
  -RecoveryDirectory "$env:LOCALAPPDATA\Zanik Manager\mapping-recovery\manual"
```

The recovery directory must remain outside the game directory. Zanik inventories and verifies the existing loader before removal, restores it after mapping, and refuses to overwrite any file that changes unexpectedly.

The archive includes its own Python runtime, UnrealAssetScout, UE4SS mapping dumper, licenses, recipes, and generators. It does not need system Python or Node.

By default the tool temporarily installs its pinned UE4SS loader, launches the official client through Steam, calls UE4SS's supported `DumpUSMAP(true)` operation, asks the game to close normally, and removes only its own verified files. It refuses to start when Dragonwilds is running and, unless preservation was explicitly selected, when an existing loader occupies a required path. It never overwrites an unknown or changed collision. If an interrupted run leaves a recovery journal, rerun the command after closing the game; recovery completes before another extraction begins.

An experienced operator may avoid the temporary loader and use a mapping already generated from the same client:

```powershell
.\init-assets.ps1 `
  -GameRoot "E:\SteamLibrary\steamapps\common\RSDragonwilds" `
  -OutputDirectory ".\output" `
  -UsmapPath "C:\private\Mappings.usmap" `
  -SkipGameLaunch
```

`-KeepWork` retains the targeted intermediate exports for debugging and prints their location. Those files are game-derived: do not publish or commit them. `-CatalogsOnly` still keeps any artwork reachable from a generated runtime catalog so it cannot create broken URLs.

The output contains:

- `zanik-assets-CLIENT_BUILD.zip`, the conventional complete bundle;
- `zanik-assets-CLIENT_BUILD-parts/bundle-index.json` and checksum-indexed logical ZIP parts, each below 90 MiB.

The manifest records the client build, container and executable fingerprints, mapping hash, exact tool/recipe versions, catalog counts, every generated file hash, fallbacks, and reviewed compatibility profile. Build-ID differences are informational. Structural errors or unreviewed catalog-count drift mark the candidate incompatible, and the server refuses to activate it.

## Install on Pterodactyl

For the multipart form, upload the index and every part to:

```text
/home/container/zanik-assets-incoming/CLIENT_BUILD/
```

Then run in the server console or startup environment:

```sh
/home/container/zanik/scripts/init-assets.sh \
  --game-root /home/container \
  --install-bundle-set /home/container/zanik-assets-incoming/CLIENT_BUILD/bundle-index.json
```

For the full ZIP, upload it to `/home/container/zanik-assets-incoming/CLIENT_BUILD/` and run:

```sh
/home/container/zanik/scripts/init-assets.sh \
  --game-root /home/container \
  --install-bundle /home/container/zanik-assets-incoming/CLIENT_BUILD/zanik-assets-CLIENT_BUILD.zip
```

Both paths reject traversal, symlinks, missing or unexpected parts, duplicate paths, checksum mismatches, incomplete catalogs, unresolved web URLs, and incompatible manifests. Assembly and validation happen in a temporary tree; the previous active cache remains untouched on failure.

Zanik-authored placeholders cover genuinely unavailable optional artwork. Missing required catalogs or static assets never produce a partially activated bundle.
