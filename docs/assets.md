# Local game assets

Zanik does not distribute Dragonwilds containers, mappings, extracted trees, or generated game artwork. The Linux dedicated server is an inventory source only; it cannot create the complete web bundle. Build the bundle on a Windows PC that has a licensed Dragonwilds client, then upload it to the server.

Building artwork comes from the client's authored build-interface assets. Most pieces reference ordinary textures; 68 special pieces reference `GeneratedIcons` packages that contain an embedded thumbnail, and the Wintumber tree references a procedural-thumbnail texture. UnrealAssetScout exports those authored PNGs directly. Zanik does not render building models and does not require Blender.

Resource markers whose world actors do not expose a direct item reference use a reviewed mapping to the harvested inventory item. Item artwork is selected from the explicit cooked `Icon` reference by full package path; it is never guessed from a filename or replaced by a generated marker. Missing resource artwork makes extraction fail before bundling.

The lodestone map marker comes from the client's authored `/Game/Art/UI/Icons/T_LodestoneMapIcon` package. It is intentionally distinct from the similarly named building-piece icon, and the extraction recipe identifies it by its complete cooked package path.

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

Steam must be signed in and able to launch Dragonwilds without an interactive client prompt. Mapping generation first uses Steam's registered game-launch URL, then retries through the discovered `steam.exe -applaunch` path if a Steam client update or restart loses that request. If neither handoff starts the exact Dragonwilds executable, Zanik restores the preserved loader and keeps the extraction at its pre-mapping stage so **Resume extraction** can retry safely after Steam is ready.

Catalog export, generation, ZIP packaging, multipart packaging, and cleanup are journaled. Zanik Manager keeps interrupted work under `%LOCALAPPDATA%\Zanik Manager\asset-work\` and offers **Resume extraction** or **Discard generated work**. A packaging retry reuses the validated generated tree instead of reopening the game. Cleanup problems after both bundle forms validate are warnings; Manager retries verified Zanik-owned cleanup later.

Operational journals are compatible with Windows PowerShell 5.1 and PowerShell 7. On resume, Zanik validates the reviewed minimum raw-export coverage, every expected UnrealAssetScout texture-batch manifest, the font, mapping, recipe, and source fingerprints before reusing exports. A richer mapping may resolve additional raw packages from the same cooked build; exact generated catalog counts and structural validation remain the publication boundary. A retained JSON-only stage resumes with artwork and font export instead of repeating mapping generation or catalog export. Missing or changed required exports still fail closed without launching the game or deleting retained work. Fresh-server guided setup automatically chooses this verified resume path and then completes upload and activation on the server being installed.

The packaged Windows tools use extended-length filesystem paths for deeply nested cooked exports. Every exported JSON record used by generation must be readable and valid; an unreadable or malformed file stops validation with its filename instead of being silently omitted. This is required even when the outer working directory itself is short because cooked package names can still cross the legacy 260-character Windows limit.

Recipe routes require positive cooked evidence. The extractor includes the base and plugin progression tables and never interprets a missing provider as “available from the start.” The reviewed provider-less set is shown as **Unlock trigger not yet identified**; any unreviewed route drift makes the candidate incompatible.

An experienced operator may avoid the temporary loader and use a mapping already generated from the same client:

```powershell
.\init-assets.ps1 `
  -GameRoot "E:\SteamLibrary\steamapps\common\RSDragonwilds" `
  -OutputDirectory ".\output" `
  -UsmapPath "C:\private\Mappings.usmap" `
  -SkipGameLaunch
```

`-KeepWork` retains the targeted intermediate exports for debugging and prints their location. Those files are game-derived: do not publish or commit them. `-CatalogsOnly` still keeps any artwork reachable from a generated runtime catalog so it cannot create broken URLs.

Allow roughly 10–30 minutes and at least 10 GiB of free space for the first extraction; game updates, storage speed, antivirus scanning, and mapping generation can change the total. The final bundle is much smaller than the private targeted-export cache.

After the first successful extraction, Zanik Manager retains the verified UnrealAssetScout workspace under `%LOCALAPPDATA%\Zanik Manager\asset-work\`. The cache records the exact catalog selectors, reachable texture targets, font selector, recipe fingerprint, and source fingerprint used for that run. With an unchanged game client, unchanged catalog and font selectors make **no** UnrealAssetScout calls. A reviewed recipe change exports only newly added catalog selectors and newly reachable textures, removes only retired cached outputs, and preserves everything still referenced. Zanik then regenerates and validates the complete website bundle from that updated cache. The server continues to receive one complete atomic bundle; raw exports and mappings are never uploaded.

A Dragonwilds executable change is different: packages behind existing paths may have changed even when the extraction recipe did not. In that case Zanik deliberately performs broad targeted discovery across the recipe—not a full cooked-tree export—so UnrealAssetScout can update changed packages and remove stale ones safely. This is the expensive fallback required after a game update. A Manager-only or generator-only update does not trigger that scan.

Generated ZIP entries use the bundle's generation time, and multipart archives preserve it when Pterodactyl extracts them. Asset files deployed from older Zanik bundles may display December 31, 1979 in western time zones because those archives used the ZIP format's January 1, 1980 minimum timestamp; that date is cosmetic and is corrected the next time a newly generated bundle is uploaded.

Use **Discard generated work** to remove this private cache. Validated ZIPs are retained, but the next extraction will be a full extraction. A bundle created before incremental caching was introduced, or a bundle copied from another computer, cannot reconstruct the raw cache and therefore requires one final full extraction. Changes to the pinned extraction tool also require a full rebuild because carrying old outputs across tool versions would not be reproducible.

## Extraction troubleshooting

- If extraction stops before packaging, leave the reported work directory intact. In Zanik Manager, reopen **Assets** and select **Resume extraction**. For standalone use, rerun the same command with the original `-WorkDirectory PATH -Resume` arguments.
- A resume message may report an older stage than the files actually reached if the process stopped while updating its journal. Zanik re-derives the effective export stage from validated UnrealAssetScout manifests rather than repeating extraction or trusting the stage string alone.
- If a complete ZIP and multipart set were produced but cleanup was deferred, the bundles are still valid. Manager retries only the ownership-marked workspace at startup; **Discard generated work** performs the same bounded safe cleanup on demand.
- If the game or its existing loader changed during an incomplete checkpoint, discard the old work and start a new extraction. A completed verified cache may be updated across a game build through the bounded broad-discovery path, but Zanik never combines an incomplete mapping/export checkpoint with mismatched inputs.
- If a generated catalog or icon count drifts, do not bypass compatibility validation. Preserve the work directory and Activity output for review. Raw JSON/package totals are minimum coverage signals rather than public catalog identities; increases caused by a more complete mapping continue to the exact generated-output checks.
- Raw exports, mappings, recovery journals, and retained work contain game-derived or machine-local data and must not be attached to support bundles or published.

The output contains:

- `zanik-assets-CLIENT_BUILD.zip`, the conventional complete bundle;
- `zanik-assets-CLIENT_BUILD-parts/bundle-index.json` and checksum-indexed logical ZIP parts, each below 90 MiB.

The manifest records the client build, container and executable fingerprints, mapping hash, exact tool/recipe versions, extraction stage, raw-export coverage, catalog counts, building-icon source coverage, recipe-route coverage, every generated file hash, cleanup warnings, and reviewed compatibility profile. Build-ID differences are informational. Raw-export coverage must meet the reviewed floor; structural errors or unreviewed generated-catalog drift mark the candidate incompatible, and the server refuses to activate it.

## Install on a self-hosted Linux server

Zanik Manager compares the selected bundle manifest with the active server manifest. An exact match uploads nothing and does not restart the server. With a current running Zanik installation, it uploads only added or changed files plus a bounded removal list. The server clones the active tree locally, applies the delta, validates the complete candidate, and atomically activates it. A concurrent manifest change is rejected rather than overwritten.

Older installations, offline servers, and missing or invalid active manifests use the complete-bundle fallback. The complete ZIP remains the authoritative recovery artifact. Normal activation never downloads every remote asset for comparison; deep per-file verification is diagnostic-only.

The complete ZIP is recommended for an ordinary Linux server. On the Linux host, create a versioned incoming directory under the server root. This example assumes the generated filename contains client build `CLIENT_BUILD`:

```sh
SERVER_ROOT=/srv/dragonwilds
CLIENT_BUILD=CLIENT_BUILD
mkdir -p "$SERVER_ROOT/zanik-assets-incoming/$CLIENT_BUILD"
```

From PowerShell on the Windows machine, transfer the generated ZIP with SCP, or use an equivalent SFTP client:

```powershell
scp ".\output\zanik-assets-CLIENT_BUILD.zip" `
  "game-server:/srv/dragonwilds/zanik-assets-incoming/CLIENT_BUILD/"
```

Do **not** manually unzip the asset bundle. On the Linux server, run the installed initializer:

```sh
"$SERVER_ROOT/zanik/scripts/init-assets.sh" \
  --game-root "$SERVER_ROOT" \
  --install-bundle "$SERVER_ROOT/zanik-assets-incoming/$CLIENT_BUILD/zanik-assets-$CLIENT_BUILD.zip"

"$SERVER_ROOT/zanik/scripts/doctor.sh" --server-root "$SERVER_ROOT" --json
```

The initializer validates the manifest, catalogs, paths, and hashes in a temporary tree and then atomically activates it as `SERVER_ROOT/zanik/assets/current`. The previous active cache remains available if validation or activation fails.

Restart the Dragonwilds server after activation. Zanik's native recipe and quest trackers load their runtime catalogs during process startup, so an already-running server cannot use a newly activated or updated catalog until it restarts. Zanik Manager performs this restart automatically when it activates assets on a running Pterodactyl server.

Use the multipart bundle only when a hosting service has an upload-size limit. Upload `bundle-index.json` and every part without renaming them, then pass the index to `--install-bundle-set` exactly as shown in the Pterodactyl example below.

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

Both paths reject traversal, symlinks, missing or unexpected parts, duplicate paths, checksum mismatches, incomplete catalogs, unresolved web URLs, and incompatible manifests. Assembly and validation happen in a temporary tree; the previous active cache remains untouched on failure. Do not manually extract either form on the server.

Restart the Pterodactyl server after the initializer succeeds. This is required for native recipe and quest tracking to load the activated runtime catalogs. Zanik Manager handles this automatically for Manager-driven uploads.

Zanik-authored placeholders cover genuinely unavailable optional artwork. Missing required catalogs or static assets never produce a partially activated bundle.

Fixed Dowdun Reach nodestones use the client-authored fragment-crystal navigation texture at `Art/UI/NavIcons/T_DR_Icon_Compass_FragmentCrystal`. The public `nodestone-icon.png` name is a Zanik web name, not the cooked asset basename. Extraction therefore resolves the exact full cooked package path and fails closed if it is absent; it must not guess `T_Icon_Nodestone` or substitute a generated square.

If Dragonwilds updates while an extraction is paused, the incomplete mapping and exports remain fail-closed and cannot be resumed. A completed verified workspace follows the update path instead: Manager generates a current mapping when necessary, then runs broad discovery only across Zanik's bounded recipe because the contents of existing packages may have changed. For an unchanged client and a recipe-only change, its structural selector inventory instead limits UnrealAssetScout to newly added catalog and texture targets and deletes retired outputs locally. Guided setup never offers a cached final bundle whose game executable, exact extraction-recipe SHA-256, recipe version, or reviewed compatibility profile differs from the current contract.
