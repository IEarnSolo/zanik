# Building Catalog Compatibility

Catalog mode uses `web/buildings/catalog.tsv` as its authoritative building taxonomy. Runtime validation requires schema version 1, a well-formed 64-character source fingerprint, 8 tabs, 63 sections, 839 catalog references, 822 unique runtime pieces, and a readable icon for every piece. Build 25808123 removed the Build Protection Totem and the former T4 Square Tile recipe from the live runtime catalogue. The live object scan must resolve all 822 remaining pieces and their material requirements; any unavailable piece is a failed development validation rather than a silent omission.

These checks are reported in the authenticated storage diagnostics and `/api/update-status`. The post-update validator requires the exact counts and zero unavailable pieces. A malformed catalog disables the building portion of catalog mode while ordinary crafting recipes and unrelated admin functions remain available.
