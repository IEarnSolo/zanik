# Storage plan items

Individual building plans are cooked `DA_Consumable_Plan_*` `ItemData` objects rather than ordinary `ITEM_*` assets. `tools/generate_plan_item_catalog.py` validates their display names, persistence IDs, and `T_Icon_Plans` dependency, then produces the fingerprinted `web/plan-items.tsv` manifest and one same-named PNG per plan under `web/items`.

The storage catalog admits plan objects only when all 141 manifest records and icons validate. Valid plans appear in Catalog mode under **Plans**, participate in search, can be added using their real `ItemData`, and use the game's plan-scroll inventory icon in storage. Recipe Collection remains separate and continues using the model thumbnail of the building piece each plan unlocks. The generator normalizes inconsistent game-authored `Plan:` casing and the two rug names that omit the prefix into the catalog's canonical `PLAN: ` display form while retaining their exact persistence IDs.

`/api/diagnostics/storage-plans` and the `adminStoragePlanCatalogCompatibility` fields in `/api/update-status` report operational health. Failure disables only adding plan items from Catalog mode; existing stored items and unrelated storage features remain available. The normal update backup covers both `web/plan-items.tsv` and the complete `web/items` directory through its full web-directory snapshot.
