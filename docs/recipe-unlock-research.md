# Recipe unlock reverse-engineering method

## Sources of truth

The catalog is generated from the complete cooked JSON export, not recipe-name guesses. The generator reconciles:

- every exported `RecipeData`/`RuneRecipeData` persistent ID with the save-editor GUID catalog, while treating that catalog as legacy metadata rather than an authority for newly shipped content;
- all 824 unique entries in `DA_BuildPieceCatalogue_Default`;
- `ProgressionBundleRow` tables, which contain the game's item, actor/station, skill, agility, and entitlement Boolean queries;
- recipe/vestige consumables, decorative plans, agility first-time medal rewards, skill perk modules, skill trimmed-cape fields, quest-flow grant nodes, entitlement grants, boss-altars, and vault recipe dispensers;
- `BuildingBaseUnlocks.json` as a legacy GUID cross-check only. It is not unlock evidence: live testing proved it groups base-building content across tiers rather than pieces visible on a fresh character.

The JSON record contains the exact source asset/property or progression table row for every unlock path. A name, tier, ingredient, journal location, or apparent relationship is never accepted as an unlock trigger by itself.

### Active recipe denominator

A crafting recipe is active when at least one current game-side authority exposes it: an explicit cooked unlock grant/progression row, membership in the current crafting-station catalog, or the legacy recipe registry. This union is intentional. Umbral Sands and Scorned Wilderness introduced hundreds of obtainable T7/T8 recipes before the bundled third-party `RecipeID.json` was updated; using that file as a gate incorrectly excluded them. Each JSON row records its `activeSources`, and `counts.activeCraftingRecipesBeyondLegacyRegistry` makes registry drift visible.

Recipe workstation sections come from the cooked UI catalogs, not the legacy
`StationName` label. Crafting interfaces enumerate recipes in
`DT_CraftingStationsDataTable.Rows.*.LabeledRecipes[].Collection`; processing
interfaces enumerate them separately in
`DT_ProcessingStationDataTable.Rows.*.Recipes`. The generator records every
matching row in `stationCandidates`, selects only among those proven
memberships, and marks the canonical primary value with
`stationEvidence=cookedStationCatalog`. The public catalog's `sections` field
retains every proven workstation, so a recipe exposed at both a Campfire and a
Grill (or a basic and advanced processor) appears in both sections. The more
capable/specialized station remains the singular `station` value only for
backward compatibility with older consumers. The full candidate evidence is
retained so a future game update can be diffed without repeating the reverse
engineering. Station upgrade components and journal station row handles are
also recorded for recipes added dynamically rather than embedded in a global
station table.

The public section index intentionally suppresses the redundant `Air Altar`
and `Fire Altar` headings because those recipes are also present in the Rune
Altar catalog. Their candidate memberships remain in canonical JSON evidence;
only the public navigation labels are filtered. `web/recipe-unlocks.json`
also contains a `sectionIcons` map built from the corresponding active
workstation/building icons, with the Building Pieces section represented by a
tier-one foundation icon.

An exported recipe with none of those relationships remains in the exclusions appendix. For example, the current Dragon-metal armour/scimitar/crossbow placeholder assets have no material costs, no progression/grant reference, and no crafting-station membership; they are distinct from the obtainable Red/Black Dragonhide and other T7/T8 families.

## Status and evidence rules

- `active` means at least one direct static or runtime-proven unlock path is recorded.
- `unresolved` means the recipe is in the active denominator but no proven path has yet been found. Any unresolved row makes the default update command fail.
- Building pieces are never classified as fresh-character defaults from `BuildingBaseUnlocks.json`. They require an actual cooked progression, station-interaction, perk, plan, quest, entitlement, or reward grant.
- Provider absence is never treated as proof of automatic availability. Active recipes without a positive cooked provider must match the reviewed unknown-trigger set and are published as “Unlock trigger not yet identified.” Any additional unresolved recipe, or a change to that reviewed set, fails catalog validation.
- Compound progression rows preserve their operator and both operands in `unlockPaths[].query`; the Markdown action is a readable summary, while JSON remains authoritative.
- Actor/station operands retain their internal Blueprint names and asset paths in JSON, but readable actions resolve the cooked display name from the station component or the `BuildingPieceData.DisplayName` associated through `BuildableActor`. Thus an internal actor such as `BP_Crafting_MysticForge_C` is shown as `Mystic Forge` without discarding its technical identity.
- Deleted, soft-deleted, deprecated, debug/test, journal-only, vendor-only, proxy, and otherwise rejected assets are retained in the exclusions report. This includes every exported `RecipeData` GUID absent from all active game-side authorities. A building piece in the shipped catalogue but absent from every cooked grant provider is recorded as shipped-but-unobtainable, never silently dropped.

## Repeatable update

From the repository root, run:

```powershell
.\tools\update_recipe_unlock_catalog.ps1
```

The updater rebuilds the complete cooked JSON export with Blueprint bytecode metadata, regenerates JSON and Markdown, validates uniqueness and coverage, and prints a deterministic added/removed/changed diff. `-AllowUnresolved` is research-only and must not be used for a final release.

The generated JSON stamps the UAS version and SHA-256 hashes for the executable, `.usmap`, extraction manifest, building catalog, and clean-character baseline. Count drift, missing building assets, duplicate persistent IDs, an unaccounted building-catalog entry, or an unresolved active entry fails closed.

The same command writes `web/recipe-unlocks.json`, the redacted public dependency graph consumed by `/recipe-unlocks`. It contains opaque IDs and display text only: persistent GUIDs, internal recipe names, and asset names remain exclusive to the canonical reference catalog. Item requirements link to active recipes producing that item; station requirements link through `BuildingPieceData.BuildableActor` to the station's building unlock. The web server validates its schema, entry count, generated timestamp, dependency counts, and redaction before serving it.

The generator also builds `web/recipe-icons/` from the existing item and building artwork. Public filenames are opaque hashes. Self-dependent and other non-bootstrap routes remain in the web catalog so administrators can control them through Recipe Unlock Settings; traversal detects and terminates their cycles instead of removing or rewriting the source route. The website retains each active recipe as its own configurable entry, including recipes that share the same display output and station.

The generator additionally writes `web/recipe-runtime-catalog.tsv`. This server-private mapping connects the public opaque IDs to runtime `RecipeData` and `BuildingPieceData` object names. It is never served by an HTTP route. The private `web/recipe-runtime-catalog-history.tsv` retains prior fingerprint-to-ID sets. An old complete snapshot remains usable only when that historical ID set is a strict subset of the current catalog; newly added IDs are reported as unknown until the next authoritative scan, while removed or changed IDs fail closed. Per-player tracking validates the current schema, count, fingerprint, every runtime object, the controller progress-component relationship, and the reflected `IsRecipeUnlocked`/`IsBuildingUnlocked` functions before capturing data. Complete snapshots are stored atomically in `/home/container/zanik-data/player-recipe-snapshots.db` with mode `0600`; incomplete scans never replace a prior snapshot.

When the stripped server exposes reflected query fields, validation proves the call ABI before invoking either function: the recipe/building input parameter must remain at byte offset `0` and the Boolean return value at byte offset `8`. Current stripped Linux builds omit both fields from runtime enumeration; that explicit case instead requires the fingerprint-bound executable, both exact `ProgressComponent` function objects, all typed runtime recipe objects, and a complete non-empty behavioral scan. A partial or contradictory reflected layout fails player-recipe tracking closed. The snapshot database, private runtime mapping, generated public graph, and route settings are all update-backup inputs.

Compound progression queries preserve both levels of boolean logic. Each operand records whether its targets are `all` or `any`, and each route records whether its operand groups are joined by `and` or `or`. For example, Draconic Leather remains `Dragonwolf Hide OR (Dragon Blood AND Hard Leather)` throughout the canonical catalog, web graph, settings text, and rendered tree.

For deployment, copy `web/recipe-unlocks.json`, `web/recipe-runtime-catalog.tsv`, and `web/recipe-runtime-catalog-history.tsv` to `/home/container/zanik/web/` together with the rebuilt `dist/libdwstatus.so` at `/home/container/libdwstatus.so`. The normal update backup preserves these files and the player snapshot database. A missing, malformed, count-drifted, or unresolved runtime mapping disables only per-player recipe tracking; other public and administrative pages remain available.

The baseline is extracted read-only with:

```powershell
python .\tools\extract_recipe_baseline_from_spud.py <save> `
  --character-name Testdude `
  --character-guid ADD3B8504FCAB14CBC9E2B8ADA268927 `
  --output .\reference\recipe-unlock-testdude-baseline.json
```

Both identity fields are guards. A mismatch aborts without writing output.

`reference/recipe-unlock-testdude-t1-observation.json` is a second, read-only observation captured after `Testdude` picked up Stone and Ash Logs, built the Crafting Table, and opened its UI. Its `BuildingsUnlocked` set contains every GUID in progression row `T1_BuildPieces_BasicBuilding_Bundle_Ash`, proving that the bundle is granted by interaction with `BP_Crafting_StartingBench_C`.

The bedroll also appears in the independent row `T1_Furniture_Bed_BedRoll`, whose condition is `ALL ITEMS[ Ash Logs, Coarse Animal Fur ]`. These are alternative grant rows, not one combined condition. Human-readable output therefore joins multiple `unlockPaths` with **OR**.

### Duplicate and redundant progression rows

The game can name the same effective grant in multiple progression tables (for example, a material milestone table and a region/station table). Generation canonicalizes these semantically:

- `ANY` and `ALL` over one target are equivalent.
- Exact equivalent conditions become one `unlockPaths` entry whose `sources` array retains every cooked row.
- A stricter condition is removed from the human-facing effective paths when another path always unlocks the same entry sooner. The original condition remains in `redundantUnlockPaths` with `redundantBecause` evidence.
- Compound conditions and different trigger types are never simplified by this rule.

For example, the Cauldron's Clay milestone makes the older `ALL ITEMS[ Ash Logs, Clay ]` row unreachable as a distinct unlock route: any player satisfying the latter has already satisfied the former. The readable catalog therefore says only `ANY ITEMS[ Clay ]`, while JSON retains the older row for auditing.

### Recipe-dispenser locations

Vault recipe dispensers all display as `Dragonkin Effigy`, so the dispenser definition alone does not identify where a player must go. Generation resolves location context from the cooked `L_World` world-partition placements:

- a final-chest or dungeon-exit actor in the same partition cell supplies the dungeon name;
- otherwise, the dispenser's world coordinates are matched to the nearest named `DungeonActor`/`DungeonExit` anchor within 300 metres;
- a dispenser outside a named dungeon is associated with a colocated named overworld landmark. The Necromancer's Staff placement resolves to the Spectral Room at Silverthorn Keep in Fellhollow.
- the dungeon entrance is tested against cooked `BP_MapRegion` boundary splines and localized region names. Fellhollow subregions retain `Subregion — Fellhollow` in the readable output.

If an entrance's boundary polygon conflicts with its explicit world-context tag, generation rejects the cross-world polygon and uses the nearest localized map label from the correct world region. Kalistrakthen Kara is the current example: its `CTX_Fellhollow` entrance is nearest Emberwood, while overlapping cooked geometry reports Alcarrid Oasis. Both results and the discrepancy note remain in the JSON evidence.

Each dispenser path stores `locations[]` with the world-partition cell, cooked map asset, coordinates, matching evidence actor, and anchor distance when applicable. The readable action includes the resolved location. This is derived from placements rather than a recipe-specific lookup, so moved, added, or removed effigies change the generated diff after a game update.

### Quest reward milestones

Quest recipe and building grants are resolved from the compiled quest-flow graph rather than reported as generic reward nodes. The generator indexes every `QuestData` asset for its localized title and objective text, follows `Compiled_NodeGUID`/`OutputConnections` in both directions from each `QFT_GiveCraftingRecipe` or `QFT_UnlockBuildingPieces` node, and records the reward node GUID as audit evidence.

- A reachable matching `QFT_CompleteQuest` node is reported as `Complete <quest>`.
- A grant before completion is reported at the nearest preceding localized objective, dialogue/prompt, or named quest-state transition.
- When the milestone is dialogue, `SpeakerTag` identifies the participant who delivers the reward. The readable action says `Talk to <NPC>`, while JSON stores the full participant tag, the exact dialogue node, and any matching `BP_NPC_*` asset candidates. Player and narrator lines are not mislabeled as NPC interactions.
- The quest whose name matches the flow asset is authoritative in flows that reference other quests. This prevents a nearby subquest branch from mislabeling the enclosing quest—for example, the Lougrim Memorial Shrine belongs to `Regicide` in `QF_SW_MainQuest`.
- A reward in a conversation/event flow with no associated quest is classified as `quest_flow_event`, not `complete_quest`. Masterwork Statue interactions are the current example.

The JSON preserves the quest internal name, localized name, asset path, milestone type and description, flow asset, reward node, GUID, and completion-node distance. Generic wording such as “Reach quest-flow reward node” is forbidden by validation.

## Runtime validation protocol

Static ambiguity may be tested only on development server `0f95b34b` and only after backing up the relevant save state.

1. Resolve the selected character name and persistent ID before every write; abort unless the name is exactly `Testdude`.
2. Treat `I Earn Solo` as read-only. Never mutate its inventory, quests, recipes, buildings, location, or save.
3. Capture a clean `Testdude` recipe/building baseline.
4. Apply one action (item pickup, station interaction, vestige study, quest reward, or skill/perk) and diff persistent IDs.
5. Restore the baseline before testing the next independent action.
6. Record the before/after evidence alongside the corresponding catalog row.

Production server `ef68362b` is never used for this research.
