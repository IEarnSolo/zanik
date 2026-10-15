# Quest tracking and compatibility

The public quest catalog is `web/quest-catalog.json`; the fingerprint-bound runtime identity list is `web/quest-runtime-catalog.tsv`. It contains playable main and side quests only. Tutorials, crafting helpers, item packs, journal records, and other technical quest records are excluded. Unknown authored metadata is left empty rather than inferred.

Online state is read through `UQuestProgressComponent::GetQuestState` in one catalog query per eligible game tick, separated by 20 ms. Component discovery and catalog resolution are cached; missing, ambiguous, stale, or changed objects fail the quest capability closed. No quest mutation function is invoked. Snapshots are persisted with mode `0600` in `/home/container/zanik-data/player-quest-snapshots.db` and remain public through the owning player's opaque public ID while offline.

Current objective text is optional even when the game reports a quest as in progress. In particular, the game can return a default `FText` with a null `TextData` reference during login or between objectives. The tracker checks that reference before calling `FTextInspector::GetDisplayString`; an empty value is stored as an unavailable objective instead of being decoded or treated as a tracking failure. This guard is a required runtime validation boundary because Unreal's display-string helper does not accept a default/null `FText`.

The public live map loads `/api/quests/catalog` and `/api/quests/steps` directly; it does not require or request an admin session. Its Quest steps panel groups quests by region, supports a region filter, and lets visitors enable a complete quest or individual objectives. Selections persist in browser-local storage. Each selected objective publishes all of its verified destination markers, and clicking the objective focuses the first destination. Hovering or clicking a marker uses the map's custom tooltip to show quest type, region, objective, destination, dungeon-entrance context, notes, and extracted world coordinates. No live quest state or private character identity is exposed by this map layer.

`DW_QUEST_TRACKING_ENABLED=0` disables only quest tracking. `/api/quest-tracker-diagnostics` and the `playerQuest*` fields in `/api/update-status` report catalog, component, query, persistence, cache, scan, and timing health. A changed exact query symbol, reflected parameter layout, catalog fingerprint, runtime QuestData identity, invalid state enum, or component ownership relationship disables quest data without affecting player, map, kill, recipe, or admin capabilities.

Quest-step destinations are a separate generated operational dependency in
`web/quest-step-locations.json`. Reviewed relationships store stable game
identities rather than coordinates; the generator re-resolves their placements
and fails if an identity is absent or ambiguous. Markers carry an action role,
the extracted actor type and quest metadata when available, review state, and
semantic warnings. Duplicate display names at different coordinates and
numeric objectives with too few action markers are downgraded in the audit
until reviewed. Dynamic objectives intentionally publish no fixed marker rather
than falling back to a quest giver or quest start.

The cooked quest schema is versioned independently from the native query ABI.
Generators accept both the legacy `ObjectiveTexts` map and the current
`Objectives` records whose localized text is stored in `Value.Description`.
The reviewed asset profile counts quests, player-facing objective texts,
quest-step states, and published markers separately. A bundle containing the
right number of quest shells but missing nested objective or marker data is
incompatible and cannot be activated.

After every game update, regenerate and review the two catalogs, compare all
nested coverage counts, run the negative catalog/layout tests, then validate
start/progress/completion and offline persistence on development. Promotion
requires zero slow queries, a maximum query below 1 ms, and
enabled-versus-disabled combat testing.
