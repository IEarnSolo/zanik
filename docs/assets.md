# Local game assets

Zanik's core download and Git repository contain no game containers, keys, extracted trees, or game-derived assets. The initializer inventories a locally licensed installation and classifies required generated catalogs, optional visual assets, and unavailable client-only inputs.

Try the Linux dedicated server first:

```sh
/home/container/zanik/scripts/init-assets.sh --game-root /home/container
```

The inventory is written to `/home/container/zanik/state/asset-inventory.json`. Extraction/conversion uses the separately published, checksum-pinned asset-tool bundle. A non-redistributable converter may be supplied with `--tool-path`; Zanik invokes it as `TOOL --game-root PATH --output PATH`. Generated input must be placed in `SERVER_ROOT/zanik-generated-web`; the command then validates, bundles, and atomically activates it while retaining the prior cache on failure.

If required data is client-only, run `init-assets.ps1 -GameRoot PATH` from the Windows asset-tool bundle. It creates `zanik-assets-GAME_BUILD.zip`. Upload that file to `/home/container/` and install it with:

```sh
/home/container/zanik/scripts/init-assets.sh --game-root /home/container \
  --install-bundle /home/container/zanik-assets-GAME_BUILD.zip
```

`asset-manifest.json` records schema/build/source fingerprints, generated-file hashes, and missing required/optional assets. Optional images fall back to Zanik-owned placeholders and system fonts. Missing required catalogs disable only their dependent capability.
