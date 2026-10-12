# Local game assets

Zanik's core download and Git repository contain no game containers, keys, extracted trees, or game-derived assets. The initializer inventories a locally licensed installation and classifies required generated catalogs, optional visual assets, and unavailable client-only inputs.

Try the Linux dedicated server first:

```sh
/home/container/zanik/scripts/init-assets.sh --game-root /home/container
```

The inventory is written to `/home/container/zanik/state/asset-inventory.json`. Extraction/conversion uses the separately published, checksum-pinned asset-tool bundle. A converter supplied with `--tool-path` receives `--game-root PATH --output PATH --source-kind KIND` and, when supplied, `--usmap-path PATH`. Its output must pass Zanik's required JSON/TSV and checksum validation before it can be bundled or atomically activated. The prior cache remains active on every failure.

If required data is client-only, run `init-assets.ps1 -GameRoot PATH -ToolPath CONVERTER [-UsmapPath FILE]` from the Windows asset-tool bundle. It creates `zanik-assets-GAME_BUILD.zip`. Upload that file to `/home/container/` and install it with:

```sh
/home/container/zanik/scripts/init-assets.sh --game-root /home/container \
  --install-bundle /home/container/zanik-assets-GAME_BUILD.zip
```

`asset-manifest.json` records schema/build/source fingerprints, generated-file hashes, and missing required/optional assets. Runtime identity catalogs (`resource-identities.tsv` and `npc-identities.tsv`) are required bundle files alongside their public catalogs; the launcher reads them from the atomically activated asset directory. Optional images fall back to Zanik-owned placeholders and system fonts. Missing required catalogs disable only their dependent capability.
