# Building Catalog Compatibility

Catalog mode uses `web/buildings/catalog.tsv` as its authoritative building taxonomy. Runtime validation requires schema version 1, a well-formed 64-character source fingerprint, 8 tabs, 64 sections, 867 catalog references, 850 unique runtime pieces, and a readable icon for every piece. Client build 26420214 added 23 authored pieces: four fishing trophies, two rugs, two postboxes, two tapestries, a fishing rack, and twelve Penguin decorations in a new section. The live object scan must resolve all 850 pieces and their material requirements; any unavailable piece is a failed development validation rather than a silent omission.

These checks are reported in the authenticated storage diagnostics and `/api/update-status`. The post-update validator requires the exact counts and zero unavailable pieces. A malformed catalog disables the building portion of catalog mode while ordinary crafting recipes and unrelated admin functions remain available.
