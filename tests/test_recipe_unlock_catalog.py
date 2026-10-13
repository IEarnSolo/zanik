import json
import hashlib
import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


class RecipeUnlockCatalogTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads((ROOT / "reference" / "recipe-unlocks.json").read_text(encoding="utf-8"))
        cls.web = json.loads((ROOT / "web" / "recipe-unlocks.json").read_text(encoding="utf-8"))

    def test_no_unresolved_or_duplicate_active_ids(self):
        self.assertEqual([], self.data["validation"]["unresolved"])
        self.assertEqual([], self.data["validation"]["duplicatePersistentIds"])
        self.assertEqual([], self.data["validation"]["duplicateExclusionPersistentIds"])
        self.assertEqual({}, self.data["validation"]["duplicateSourceRecipeIds"])

    def test_every_active_entry_has_one_or_more_paths(self):
        for entry in self.data["recipes"]:
            self.assertTrue(entry["active"], entry["internalName"])
            self.assertTrue(entry["unlockPaths"], entry["internalName"])
            self.assertNotEqual("unresolved", entry["obtainableStatus"], entry["internalName"])

    def test_every_building_catalog_piece_is_accounted_for_once(self):
        active = {entry["internalName"] for entry in self.data["recipes"] if entry["kind"] == "building"}
        excluded = {entry["internalName"] for entry in self.data["exclusions"] if entry["kind"] == "building"}
        self.assertFalse(active & excluded)
        self.assertEqual(self.data["counts"]["sourceBuildingCatalogPieces"], len(active | excluded))
        self.assertEqual(self.data["counts"]["sourceBuildingCatalogPieces"], self.data["counts"]["accountedBuildingCatalogPieces"])

    def test_every_exported_recipe_asset_is_accounted_for(self):
        self.assertEqual([], self.data["validation"]["unaccountedSourceRecipeIds"])
        self.assertEqual(self.data["counts"]["sourceRecipeAssets"], self.data["counts"]["accountedSourceRecipeAssets"])

    def test_excluded_ids_do_not_leak_into_active_catalog(self):
        active_ids = {entry["persistentId"] for entry in self.data["recipes"] if entry["persistentId"]}
        excluded_ids = {entry.get("persistentId") for entry in self.data["exclusions"] if entry.get("persistentId")}
        self.assertFalse(active_ids & excluded_ids)

    def test_fresh_baseline_is_identity_guarded(self):
        baseline = json.loads((ROOT / "reference" / "recipe-unlock-testdude-baseline.json").read_text(encoding="utf-8"))
        self.assertEqual("Testdude", baseline["character"]["name"])
        self.assertEqual("ADD3B8504FCAB14CBC9E2B8ADA268927", baseline["character"]["persistentId"])
        self.assertEqual(64, len(baseline["source"]["sha256"]))

    def test_recent_t7_t8_recipe_families_are_active_with_expected_triggers(self):
        recipes = {entry["internalName"]: entry for entry in self.data["recipes"]}
        expected = {
            "RECIPE_Weapon_Sword_Adamant": "obtain_item",
            "RECIPE_Weapon_Sword_Rune": "obtain_item",
            "RECIPE_Armour_T7_Body_RedDragonHide": "obtain_item",
            "RECIPE_Armour_T8_Body_BlackDragonHide": "obtain_item",
            "RECIPE_Armour_T7_Body_KalphiteCarapace": "consume_vestige",
            "RECIPE_Armour_T7_Body_Desert": "consume_vestige",
            "RECIPE_Armour_T7_Body_Infinity": "obtain_item",
            "RECIPE_Armour_T7_Body_LunarGarouMage": "consume_vestige",
            "RECIPE_Armour_T8_Body_Ancestral": "obtain_item",
            "RECIPE_Weapon_Wand_Ancestral": "obtain_item",
        }
        for internal_name, trigger_type in expected.items():
            self.assertIn(internal_name, recipes)
            self.assertIn(trigger_type, {path["type"] for path in recipes[internal_name]["unlockPaths"]})
            self.assertIn("cookedUnlockGrant", recipes[internal_name]["activeSources"])

    def test_buildings_are_not_inferred_as_fresh_character_defaults(self):
        building_defaults = [
            entry["internalName"] for entry in self.data["recipes"]
            if entry["kind"] == "building" and "default" in {path["type"] for path in entry["unlockPaths"]}
        ]
        self.assertEqual([], building_defaults)

    def test_bedroll_has_two_independent_unlock_paths(self):
        bedroll = next(entry for entry in self.data["recipes"] if entry["internalName"] == "BUILDPIECE_Interior_BedRoll")
        actions = {path["action"] for path in bedroll["unlockPaths"]}
        self.assertIn("ANY ACTORS[ Crafting Table ]", actions)
        self.assertIn("ALL ITEMS[ Ash Logs, Coarse Animal Fur ]", actions)
        self.assertNotIn("default", {path["type"] for path in bedroll["unlockPaths"]})

    def test_perk_building_paths_identify_perk_skill_and_level(self):
        piece = next(entry for entry in self.data["recipes"] if entry["internalName"] == "DA_T1_Roof_Large_Hip_Shallow")
        self.assertIn("Unlock Sacred Geometry at Construction level 8", {path["action"] for path in piece["unlockPaths"]})
        for entry in self.data["recipes"]:
            self.assertNotIn("Unlock perk in ", {path["action"] for path in entry["unlockPaths"]})

    def test_observed_starting_bench_bundle_matches_testdude_unlocks(self):
        observation = json.loads((ROOT / "reference" / "recipe-unlock-testdude-t1-observation.json").read_text(encoding="utf-8"))
        self.assertEqual("Testdude", observation["character"]["name"])
        observed = set(observation["buildingsUnlocked"])
        expected = {
            entry["persistentId"] for entry in self.data["recipes"]
            if entry["kind"] == "building" and any(
                path.get("source", {}).get("row") == "T1_BuildPieces_BasicBuilding_Bundle_Ash"
                for path in entry["unlockPaths"]
            )
        }
        self.assertTrue(expected)
        self.assertTrue(expected <= observed, sorted(expected - observed))

    def test_duplicate_and_dominated_progression_paths_are_canonicalized(self):
        recipes = {entry["internalName"]: entry for entry in self.data["recipes"]}
        expected = {
            "BUILDPIECE_ProcessingStation_AdvancedTannery": "ANY ITEMS[ Yew Logs ]",
            "BUILDPIECE_CraftingStation_ArmourBench": "ALL ITEMS[ Bronze Bar ]",
            "BUILDPIECE_ProcessingStation_Cauldron": "ANY ITEMS[ Clay ]",
            "BUILDPIECE_CraftingStiation_Jewelry_Bench": "ANY ITEMS[ Gold Bar, Silver Bar ]",
            "BUILDPIECE_ProcessingStation_CharcoalKiln": "ANY ITEMS[ Clay ]",
        }
        for internal_name, action in expected.items():
            self.assertEqual([action], [path["action"] for path in recipes[internal_name]["unlockPaths"]])
        self.assertEqual(2, len(recipes["BUILDPIECE_CraftingStation_ArmourBench"]["unlockPaths"][0]["sources"]))
        self.assertEqual(2, len(recipes["BUILDPIECE_CraftingStiation_Jewelry_Bench"]["unlockPaths"][0]["sources"]))
        self.assertEqual(
            ["ALL ITEMS[ Ash Logs, Clay ]"],
            [path["action"] for path in recipes["BUILDPIECE_ProcessingStation_Cauldron"]["redundantUnlockPaths"]],
        )
        self.assertEqual(
            ["ALL ITEMS[ Ash Logs, Stone, Clay ]"],
            [path["action"] for path in recipes["BUILDPIECE_ProcessingStation_CharcoalKiln"]["redundantUnlockPaths"]],
        )

    def test_recipe_dispenser_paths_include_resolved_world_locations(self):
        recipes = {entry["internalName"]: entry for entry in self.data["recipes"]}
        expected = {
            "RECIPE_Vestige_Weapon_T4_Dagger_Dragonbone": ("Chaktan Kara", "Stormtouched Highlands"),
            "RECIPE_Armour_T4_Head_DragonkinMage": ("Chaktan Kara", "Stormtouched Highlands"),
            "RECIPE_Armour_T5_Body_Skeleton": ("Kalistrakthen Kara", "Emberwood — Fellhollow"),
            "RECIPE_Armour_T5_Head_Skeleton": ("Vekchenven Kara", "Hope's Fall — Fellhollow"),
            "RECIPE_Armour_T5_Legs_Skeleton": ("Skekven Kara", "Lake of Lost Souls — Fellhollow"),
            "RECIPE_Weapon_Staff_Necromancer": "Spectral Room (Fellhollow — Silverthorn Keep)",
            "RECIPE_Armour_T4_Head_Paladin": ("Takla Kara", "Fractured Plains"),
            "RECIPE_Weapon_Shortbow_Skeleton": ("Skekven Kara", "Lake of Lost Souls — Fellhollow"),
            "RECIPE_Armour_T4_Head_WildArcher": ("Kletterbuja Kara", "Bloodblight Swamp"),
            "RECIPE_Weapon_Dagger_Wolfbane": ("Vekchenven Kara", "Hope's Fall — Fellhollow"),
        }
        for internal_name, expected_location in expected.items():
            location, region = expected_location if isinstance(expected_location, tuple) else (expected_location, "Silverthorn Keep — Fellhollow")
            paths = [path for path in recipes[internal_name]["unlockPaths"] if path["type"] == "interact_recipe_dispenser"]
            self.assertEqual(1, len(paths))
            self.assertIn(location, paths[0]["action"])
            self.assertTrue(paths[0]["locations"])
            self.assertNotIn("unknown world placement", {item["name"] for item in paths[0]["locations"]})
            self.assertTrue(all(item.get("coordinates") for item in paths[0]["locations"]))
            self.assertEqual({region}, {item["region"]["displayName"] for item in paths[0]["locations"]})

        kalistrakthen = next(path for path in recipes["RECIPE_Armour_T5_Body_Skeleton"]["unlockPaths"]
                             if path["type"] == "interact_recipe_dispenser")["locations"][0]["region"]
        self.assertEqual("Alcarrid Oasis", kalistrakthen["boundaryRegionAtEntrance"])
        self.assertIn("disagree", kalistrakthen["validationNote"])

    def test_quest_grants_name_the_quest_and_exact_milestone(self):
        recipes = {entry["internalName"]: entry for entry in self.data["recipes"]}
        quest_paths = [
            path for entry in self.data["recipes"] for path in entry["unlockPaths"]
            if path["type"] == "complete_quest"
        ]
        self.assertTrue(quest_paths)
        for path in quest_paths:
            self.assertTrue(path["quest"]["internalName"], path)
            self.assertTrue(path["quest"]["displayName"], path)
            self.assertTrue(path["graph"]["rewardNodeGuid"], path)
            self.assertNotRegex(path["action"], r"Reach quest-flow|Reach the recorded|Activate reward node")

        self.assertIn(
            "Complete What Remains is Written",
            {path["action"] for path in recipes["RECIPE_Trinket_Runic_Ring"]["unlockPaths"]},
        )
        self.assertIn(
            "Complete Animal Magnetism",
            {path["action"] for path in recipes["RECIPE_Armour_T0_Cape_Avas_Accumulator"]["unlockPaths"]},
        )
        self.assertTrue(any(
            path["action"].startswith("Progress Even More Restless Ghosts:")
            for path in recipes["RECIPE_Ammo_Bolts_Bone"]["unlockPaths"]
        ))
        self.assertTrue(any(
            path["action"].startswith("Progress Heartstrings:")
            for path in recipes["RECIPE_Weapon_Shortbow_CrystalBow"]["unlockPaths"]
        ))
        self.assertTrue(any(
            path["action"].startswith("Progress Black Knight's Fortress:")
            for path in recipes["RECIPE_Consumable_Toastie_Maple"]["unlockPaths"]
        ))

    def test_shared_and_nonquest_flows_are_not_misattributed(self):
        recipes = {entry["internalName"]: entry for entry in self.data["recipes"]}
        shrine = recipes["DA_BaseBuilding_Decoration_LougrimMemorialShrine"]["unlockPaths"]
        self.assertTrue(any(path["action"].startswith("Progress Regicide:") for path in shrine))

        anti_dragon = recipes["RECIPE_Masterworks_Shield_Anti_Dragon"]["unlockPaths"]
        self.assertTrue(any(
            path["type"] == "quest_flow_event" and
            path["action"] == "Trigger Masterwork Statues event: Interact with Anti Dragon Statue"
            for path in anti_dragon
        ))
        abyssal = recipes["RECIPE_Masterworks_Club_AbyssalWhip_New"]["unlockPaths"]
        self.assertTrue(any(path["type"] == "quest_flow_event" for path in abyssal))

        horn_paths = [
            path for path in recipes["RECIPE_Masterworks_Challenge_Horn"]["unlockPaths"]
            if path["type"] == "complete_quest"
        ]
        self.assertGreaterEqual(len(horn_paths), 4)
        self.assertEqual({"Dragon Slayer"}, {path["quest"]["displayName"] for path in horn_paths})

    def test_dialogue_recipe_grants_identify_the_speaking_npc(self):
        recipes = {entry["internalName"]: entry for entry in self.data["recipes"]}
        npc_paths = [
            path for entry in self.data["recipes"] for path in entry["unlockPaths"] if path.get("npc")
        ]
        self.assertEqual(10, len(npc_paths))
        for path in npc_paths:
            npc = path["npc"]
            self.assertIn(f"Talk to {npc['displayName']}", path["action"])
            self.assertTrue(npc["participantTag"].startswith("QuestFlow.Participant."), path)
            self.assertTrue(npc["evidenceNode"].startswith("QFT_PromptMessage_C_"), path)

        expected = {
            "DA_BaseBuilding_Decoration_LougrimMemorialShrine": "Ancamairon",
            "RECIPE_Masterworks_Club_AbyssalWhip_New": "Knowledge Statue",
            "RECIPE_Consumable_Toastie_Maple": "Slugtooth",
            "RECIPE_Armour_T0_Cape_Zamorak": "Zanik",
            "RECIPE_Ammo_Bolts_Bone": "Zanik",
        }
        for internal_name, npc_name in expected.items():
            self.assertIn(npc_name, {
                path["npc"]["displayName"] for path in recipes[internal_name]["unlockPaths"] if path.get("npc")
            })
        maple_npc = next(path["npc"] for path in recipes["RECIPE_Consumable_Toastie_Maple"]["unlockPaths"]
                         if path.get("npc"))
        self.assertEqual(["BP_NPC_Slugtooth_Dowdun"],
                         [asset["internalName"] for asset in maple_npc["assetCandidates"]])

    def test_readable_station_triggers_use_cooked_display_names(self):
        recipes = {entry["internalName"]: entry for entry in self.data["recipes"]}
        expected = {
            "RECIPE_Weapon_Longbow_Oak": "Fletching Bench",
            "RECIPE_Weapon_Staff_Oak": "Mystic Forge",
            "RECIPE_Process_IronBar": "Furnace",
        }
        for internal_name, display_name in expected.items():
            actor_paths = [
                path for path in recipes[internal_name]["unlockPaths"]
                if any(operand.get("type") == "open_station_ui"
                       for operand in path.get("query", {}).get("operands", []))
            ]
            self.assertTrue(actor_paths, internal_name)
            self.assertTrue(any(f"ACTORS[ {display_name}" in path["action"] for path in actor_paths))
            for path in actor_paths:
                self.assertNotRegex(path["action"], r"ACTORS\[[^]]*BP_")
                actor_operands = [operand for operand in path["query"]["operands"]
                                  if operand.get("type") == "open_station_ui"]
                self.assertTrue(all(target.get("internalName") and target.get("assetPath")
                                    for operand in actor_operands for target in operand.get("targets", [])))

    def test_public_recipe_catalog_is_complete_and_redacted(self):
        self.assertLessEqual(self.web["counts"]["recipes"], self.data["counts"]["totalActive"])
        self.assertEqual(self.web["counts"]["recipes"], len(self.web["entries"]))
        self.assertEqual(len(self.web["entries"]), len({entry["id"] for entry in self.web["entries"]}))
        raw = (ROOT / "web" / "recipe-unlocks.json").read_text(encoding="utf-8")
        self.assertNotIn("persistentId", raw)
        self.assertNotIn("internalName", raw)
        self.assertIsNone(re.search(r"\b(?:RECIPE|BUILDPIECE|BP|SKILL|ED)_[A-Za-z0-9_]+", raw))
        self.assertIsNone(re.search(r"(?:ALL|ANY)\s+(?:ITEMS|ACTORS|\[)|SKILL LEVEL", raw))
        self.assertNotIn("/Game/", raw)
        ids = {entry["id"] for entry in self.web["entries"]}
        referenced = {
            recipe_id for entry in self.web["entries"] for route in entry["routes"]
            for step in route["steps"] for target in step["targets"] for recipe_id in target["recipes"]
        }
        self.assertTrue(referenced <= ids)
        for entry in self.web["entries"]:
            if entry.get("icon"):
                self.assertRegex(entry["icon"], r"^/assets/recipe-icons/[0-9a-f]{16}\.png$")
            for route in entry["routes"]:
                for step in route["steps"]:
                    for target in step["targets"]:
                        if target.get("icon"):
                            self.assertRegex(target["icon"], r"^/assets/recipe-icons/[0-9a-f]{16}\.png$")

    def test_sections_come_from_authoritative_cooked_station_catalogs(self):
        recipes = {entry["internalName"]: entry for entry in self.data["recipes"]}
        expected = {
            "RECIPE_Process_Adhesive_From_SwampTar": ("Kiln", "Kiln"),
            "RECIPE_Process_Fibres_Vis": ("AdvancedSpinningWheel", "Advanced Spinning Wheel"),
            "RECIPE_Process_Cloth_Vis": ("Loom", "Loom"),
            "RECIPE_Trinket_Amulet_of_Strength": ("JewelersBench", "Jeweller's Bench"),
        }
        web_by_name = {}
        for entry in self.web["entries"]:
            web_by_name.setdefault(entry["name"], set()).update(entry.get("sections", [entry["section"]]))
        for internal_name, (station, section) in expected.items():
            entry = recipes[internal_name]
            self.assertEqual(station, entry["station"])
            self.assertEqual("cookedStationCatalog", entry["stationEvidence"])
            self.assertIn(section, web_by_name[entry["displayName"]])

        active_crafting = [entry for entry in self.data["recipes"] if entry["kind"] == "crafting"]
        self.assertTrue(active_crafting)
        for entry in active_crafting:
            self.assertNotEqual("Crafting Bench", entry["station"], entry["internalName"])
            self.assertTrue(entry["stationCandidates"], entry["internalName"])
            self.assertIn("cookedStationCatalog", entry["activeSources"], entry["internalName"])

    def test_multi_station_recipes_choose_the_higher_available_station(self):
        recipes = {entry["internalName"]: entry for entry in self.data["recipes"]}
        expected = {
            "RECIPE_Process_Thread_Linen_From_Flax": "AdvancedSpinningWheel",
            "RECIPE_Process_Leather_From_Hide": "AdvancedTannery",
            "RECIPE_Consumable_Fish_Salmon": "Grill",
            "RECIPE_Weapon_Shortbow_Wood": "FletchingBenchv2",
        }
        for internal_name, station in expected.items():
            entry = recipes[internal_name]
            self.assertEqual(station, entry["station"])
            self.assertGreater(len({candidate["station"] for candidate in entry["stationCandidates"]}), 1)

        web_by_name = {entry["name"]: entry for entry in self.web["entries"]}
        self.assertTrue({"Spinning Wheel", "Advanced Spinning Wheel"} <=
                        set(web_by_name["Coarse Thread"]["sections"]))
        self.assertTrue({"Campfire", "Grill"} <= set(web_by_name["Fried Salmon"]["sections"]))

    def test_public_sections_have_icons_and_hide_redundant_elemental_altars(self):
        sections = {section for entry in self.web["entries"] for section in entry["sections"]}
        self.assertNotIn("Air Altar", sections)
        self.assertNotIn("Fire Altar", sections)
        self.assertIn("Rune Altar", sections)
        self.assertEqual(sections, set(self.web["sectionIcons"]))
        for section in sections:
            self.assertRegex(self.web["sectionIcons"][section],
                             r"^/assets/recipe-icons/[0-9a-f]{16}\.png$", section)

        rune_entries = [entry for entry in self.web["entries"]
                        if entry["name"] in {"Air Rune", "Fire Rune"}]
        self.assertTrue(rune_entries)
        self.assertTrue(all("Rune Altar" in entry["sections"] for entry in rune_entries))

    def test_fishing_nets_are_fishing_items_not_weapons(self):
        expected = {"Coarse Net", "Desert Net", "Fine Net", "Mystic Net", "Wool Net"}
        nets = {entry["name"]: entry for entry in self.web["entries"] if entry["name"] in expected}
        self.assertEqual(expected, set(nets))
        for name, entry in nets.items():
            self.assertIn("fishing", entry["types"], name)
            self.assertIn("items", entry["types"], name)
            self.assertNotIn("weapons", entry["types"], name)

    def test_shields_are_armour_not_weapons(self):
        public = {entry["id"]: entry for entry in self.web["entries"]}
        shields = []
        for source in self.data["recipes"]:
            output_paths = " ".join(str(output.get("assetPath", ""))
                                    for output in source.get("output", [])).casefold()
            if source["kind"] != "crafting" or "/equipment/held/shield/" not in output_paths:
                continue
            recipe_id = hashlib.sha256(
                f"{source['kind']}:{source['persistentId']}:{source['internalName']}".encode()
            ).hexdigest()[:16]
            self.assertIn(recipe_id, public, source["displayName"])
            shields.append(public[recipe_id])
        self.assertTrue(shields)
        for entry in shields:
            self.assertIn("armour", entry["types"], entry["name"])
            self.assertIn("items", entry["types"], entry["name"])
            self.assertNotIn("weapons", entry["types"], entry["name"])

    def test_live_station_catalog_overrides_conflicting_journal_metadata(self):
        desert = next(entry for entry in self.data["recipes"]
                      if entry["internalName"] == "RECIPE_Tool_FishingNet_Desert")
        self.assertEqual("CraftingTable", desert["station"])
        self.assertEqual({"CraftingTable"},
                         {candidate["station"] for candidate in desert["stationCandidates"]})
        self.assertIn("ToolsMeleeBench",
                      {candidate["station"] for candidate in desert["stationMetadataCandidates"]})
        public = next(entry for entry in self.web["entries"] if entry["name"] == "Desert Net")
        self.assertEqual(["Crafting Table"], public["sections"])

    def test_self_dependent_routes_remain_configurable(self):
        coarse = [entry for entry in self.web["entries"] if entry["name"] == "Coarse Thread"]
        self.assertEqual(2, len(coarse))
        targets = [target["name"] for route in coarse[0]["routes"] for step in route["steps"]
                   for target in step["targets"]] + [target["name"] for route in coarse[1]["routes"]
                   for step in route["steps"] for target in step["targets"]]
        self.assertIn("Flax", targets)
        self.assertIn("Coarse Thread", targets)
        wool = [entry for entry in self.web["entries"] if entry["name"] == "Wool Thread"]
        self.assertEqual(2, len(wool))

    def test_compound_route_operator_is_preserved(self):
        draconic = [entry for entry in self.web["entries"] if entry["name"] == "Draconic Leather"]
        compound = [route for entry in draconic for route in entry["routes"]
                    if len(route["steps"]) == 2]
        self.assertEqual(2, len(compound))
        self.assertTrue(all(route["mode"] == "or" for route in compound))
        self.assertTrue(all(route["steps"][0]["mode"] == "all" and
                            route["steps"][1]["mode"] == "all" for route in compound))

    def test_wizard_robes_public_dependency_workflow(self):
        by_id = {entry["id"]: entry for entry in self.web["entries"]}
        wizard = next(entry for entry in self.web["entries"] if entry["name"] == "Wizard Robes")
        seen = set()
        names = set()
        def visit(entry):
            if entry["id"] in seen:
                return
            seen.add(entry["id"])
            names.add(entry["name"])
            for route in entry["routes"]:
                for step in route["steps"]:
                    names.update(target["name"] for target in step["targets"])
                    for target in step["targets"]:
                        for recipe_id in target["recipes"]:
                            visit(by_id[recipe_id])
        visit(wizard)
        self.assertTrue({"Wizard Robes", "Rough Cloth", "Mystic Forge", "Coarse Thread",
                         "Loom", "Flax", "Spinning Wheel"} <= names)
        self.assertLessEqual(len(seen), len(by_id))

    def test_runtime_catalog_is_private_and_complete(self):
        runtime = ROOT / "web" / "recipe-runtime-catalog.tsv"
        self.assertTrue(runtime.is_file())
        lines = runtime.read_text(encoding="utf-8").splitlines()
        expected_count = self.web["counts"]["recipes"]
        self.assertRegex(lines[0], rf"^# schema=1 fingerprint=[0-9a-f]{{64}} count={expected_count}$")
        rows = [line.split("\t") for line in lines[1:] if line]
        self.assertEqual(expected_count, len(rows))
        self.assertEqual(expected_count, len({row[0] for row in rows}))
        self.assertTrue(all(len(row) == 3 and len(row[0]) == 16 and row[1] in {"crafting", "building"} for row in rows))
        main = (ROOT / "src" / "main.cpp").read_text(encoding="utf-8")
        self.assertNotIn('path == "/recipe-runtime-catalog.tsv"', main)

    def test_reviewed_runtime_history_preserves_prior_catalogs(self):
        history = ROOT / "web" / "recipe-runtime-catalog-history.tsv"
        rows = [line.split("\t") for line in history.read_text(encoding="utf-8").splitlines()
                if line.startswith("C\t")]
        self.assertGreaterEqual(len(rows), 2)
        self.assertTrue(all(len(row) == 3 and len(row[1]) == 64 for row in rows))
        self.assertTrue(all(all(len(recipe_id) == 16 for recipe_id in row[2].split(","))
                            for row in rows))
        runtime_header = (ROOT / "web" / "recipe-runtime-catalog.tsv").read_text(
            encoding="utf-8").splitlines()[0]
        current = re.search(r"fingerprint=([0-9a-f]{64})", runtime_header).group(1)
        self.assertIn(current, {row[1] for row in rows})

    def test_windows_extractor_seeds_reviewed_recipe_history(self):
        pipeline = (ROOT / "tools" / "windows_asset_pipeline.py").read_text(encoding="utf-8")
        self.assertIn('history_seed = repo / "asset-tools/recipe-runtime-catalog-history.tsv"', pipeline)
        self.assertIn('"--runtime-history-seed", str(history_seed)', pipeline)

    def test_player_page_recipe_catalog_contract(self):
        page = (ROOT / "src" / "web_page.h").read_text(encoding="utf-8")
        main = (ROOT / "src" / "main.cpp").read_text(encoding="utf-8")
        self.assertIn('id="recipeSection"', page)
        self.assertIn("<summary>Recipe collection</summary>", page)
        self.assertIn("Unlocked only", page)
        self.assertNotIn("Available from Start", page)
        self.assertNotIn("stations:'Stations'", page)
        self.assertIn("effigies:'Effigies'", page)
        self.assertIn("plans:'Plans'", page)
        self.assertIn("'building-pieces':'Building Pieces'", page)
        self.assertIn("weapons:'Weapons'", page)
        self.assertIn("armour:'Armour'", page)
        self.assertIn("capes:'Capes'", page)
        self.assertIn("jewellery:'Jewellery'", page)
        self.assertIn("'arrows-runes':'Arrows & Runes'", page)
        self.assertIn("fishing:'Fishing'", page)
        self.assertIn("farming:'Farming'", page)
        self.assertIn("food:'Food'", page)
        self.assertIn("drink:'Drink'", page)
        self.assertIn("potions:'Potions'", page)
        self.assertIn("{all:'All','building-pieces':'Building Pieces',items:'Items'", page)
        self.assertIn(".recipe-tabs{display:flex;flex-wrap:wrap", page)
        self.assertIn("Skills & Perks", page)
        self.assertIn("RecipeCatalogUnknownIds", main)
        self.assertIn("recipe-runtime-catalog-history.tsv", main)
        self.assertIn('path == "/api/player-recipes"', main)
        self.assertIn("Player recipe tracking is unavailable for this game build", main)

    def test_player_recipe_output_types_are_complete_and_redacted(self):
        allowed = {"items", "weapons", "armour", "capes", "jewellery",
                   "arrows-runes", "fishing", "farming", "food", "drink", "potions"}
        for entry in self.web["entries"]:
            self.assertTrue(set(entry.get("types", [])) <= allowed)
            if entry["kind"] == "building":
                self.assertNotIn("items", entry.get("types", []))
        by_name = {}
        for entry in self.web["entries"]:
            by_name.setdefault(entry["name"], set()).update(entry.get("types", []))
        self.assertIn("weapons", by_name["Shadow Sword"])
        self.assertIn("armour", by_name["Adamant Platebody"])
        self.assertIn("capes", by_name["Agility Cape"])
        self.assertIn("jewellery", by_name["Amulet of Accuracy"])
        self.assertIn("arrows-runes", by_name["Air Rune"])
        self.assertIn("food", by_name["Bread"])
        self.assertIn("drink", by_name["Arcane Infusion"])
        self.assertIn("potions", by_name["Antifire Potion"])
        for name in ("Animated Feather", "Anchovy Bait", "Ash Rod", "Yew Rod"):
            self.assertIn("fishing", by_name[name])
            self.assertNotIn("weapons", by_name[name])
        farming = (
            "Adamant Spade", "Bronze Spade", "Rune Spade", "Steel Spade", "Undead Spade", "Wooden Spade",
            "Adamant Watering Can", "Bronze Watering Can", "Rune Watering Can", "Steel Watering Can",
            "Wooden Watering Can", "Compost Bucket", "Iron Secateurs",
        )
        for name in farming:
            self.assertIn("farming", by_name[name])
            self.assertIn("items", by_name[name])
            self.assertNotIn("weapons", by_name[name])
        for name in ("Torch", "Blue Torch", "Green Torch", "Purple Torch", "Red Torch", "Yellow Torch"):
            self.assertIn("items", by_name[name])
            self.assertNotIn("weapons", by_name[name])
        for name in ("Glass Owl", "Wooden Club", "Challenge Horn Mouthpiece"):
            self.assertEqual(set(), by_name[name])

    def test_recipe_unlock_page_search_and_disclosure_contract(self):
        page = (ROOT / "src" / "web_page.h").read_text(encoding="utf-8")
        self.assertIn("constexpr const char* kRecipeUnlocksPage", page)
        self.assertIn("const AUTO_EXPAND=8", page)
        self.assertIn("matches.length<=AUTO_EXPAND", page)
        self.assertIn("ranked=ranked.filter(pair=>pair[0]===0)", page)
        self.assertIn("appendLogic(parent,step.mode==='any'?'OR':'AND')", page)
        self.assertIn("appendLogic(parent,route.mode==='or'?'OR':'AND')", page)
        self.assertIn("appendLogic(dependencies,'OR')", page)
        self.assertNotIn("already shown in this route", page)
        self.assertIn("overflow:hidden", page)
        self.assertIn("flex-wrap:wrap", page)
        self.assertNotIn("route.scrollLeft", page)
        self.assertIn("entry.name===target.name", page)
        self.assertIn("appendCombinedDependencyRoutes", page)
        self.assertIn("routeSignature", page)
        self.assertIn("/admin/recipe-unlocks", page)
        self.assertIn("/api/recipe-unlock-settings", page)
        self.assertIn("View more", page)
        self.assertIn("moreDialog", page)
        self.assertIn("These are less common unlock routes kept hidden to keep the main tree readable.", page)
        self.assertIn("hidden-route-icon", page)
        self.assertIn("visual.src=selection.entry.icon", page)
        self.assertIn('id="hideDuplicates" type="checkbox" checked', page)
        self.assertIn("zanik.recipeUnlocks.hideDuplicates", page)
        self.assertIn("queueDuplicateLayout", page)
        self.assertIn("const depth=node=>", page)
        self.assertIn("const difference=depth(a)-depth(b)", page)
        self.assertIn("ar.top-br.top||ar.left-br.left", page)
        self.assertIn("rect.bottom-lane.top+22", page)
        self.assertIn("width:min(100%,6.4rem)", page)
        self.assertIn("max-width:6.4rem", page)
        self.assertIn("desired=branches.length*88+logic.length*24", page)
        self.assertIn("while(node.nextElementSibling)node.nextElementSibling.remove()", page)
        self.assertNotIn("branch.remove()", page)
        self.assertIn("route-options", page)
        self.assertIn("catalog.sectionIcons?.[name]", page)
        self.assertIn("recipe-link-icon", page)
        self.assertIn("disabled.has(entry.id+'.'+index)", page)
        self.assertIn('id="all" type="button">Show all', page)
        self.assertIn('id="none" type="button">Hide filtered', page)
        self.assertIn("entry.routes.length===1?routeText(entry.routes[0])", page)
        self.assertIn("bulk:'true',ids", page)
        self.assertIn("Recipe Unlocks", page)
        self.assertIn("aria-live=\"polite\"", page)

    def test_recipe_unlock_http_and_compatibility_contract(self):
        source = (ROOT / "src" / "main.cpp").read_text(encoding="utf-8")
        self.assertIn('path == "/recipe-unlocks"', source)
        self.assertIn('path == "/api/recipe-unlocks"', source)
        self.assertIn('Respond(fd,503,"application/json; charset=utf-8"', source)
        self.assertIn('health.recipes>=100', source)
        self.assertIn('ids==static_cast<size_t>(health.recipes)', source)
        self.assertNotIn("kExpectedRecipeCatalogCount", source)
        self.assertIn('JsonValue(request.body,"bulk")=="true"', source)
        self.assertIn("SetManyEnabled(ids,enabled)", source)
        for field in ("recipeUnlockCatalogCompatibility", "recipeUnlockCatalogRecipes",
                      "recipeUnlockDependencyEdges", "recipeUnlockCycles",
                      "recipeUnlockCatalogGenerated"):
            self.assertIn(field, source)


if __name__ == "__main__":
    unittest.main()
