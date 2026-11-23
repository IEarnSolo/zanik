#!/usr/bin/env python3
"""Regression checks for audited Add Item and tooltip categories."""

from pathlib import Path


source = (Path(__file__).resolve().parents[1] / "src/engine/admin_storage.cpp").read_text(encoding="utf-8")
page = (Path(__file__).resolve().parents[1] / "src/web_page.h").read_text(encoding="utf-8")
start = source.index("CatalogMeta CatalogMetadata(const std::string&key,const std::string&name){")
end = source.index("struct Chest", start)
classifier = source[start:end]

expected_rules = {
    'set("Miscellaneous",999)': 'name.find("(Expired)")',
    'set("Tomes",75)': "item_consumable_tome_",
    'set("Plans",85)': "item_consumable_planpack_",
    'set("Weapon enhancements",35)': "item_resources_fireoil",
    'set("Reward packs",87)': "item_consumable_birdnest",
    'set("Armour",20)': "item_masterworks_shield_",
    'set("Weapons",10)': "item_masterworks_greatsword_titanswrath",
    'set("Tools",25)': 'lower=="item_torch"',
    'set("Ammunition",60)': "item_fishingbait_",
    'set("Seeds",80)': "_treeseed_",
    'set("Currencies",92)': "item_currency_",
    'set("Resources",90)': "item_masterworks_imbued_granite_maul_head",
    'set("Quest items",110)': "item_masterworks_challenge_horn",
}
for category, token in expected_rules.items():
    assert category in classifier
    assert token in classifier

# Narrow exceptions must win before broad consumable, resource, farming, and
# miscellaneous fallbacks. Scarab Staff, the duplicate Shadow Sword, and
# throwable Fire Oil intentionally have no promoted rule and remain fallback
# miscellaneous items.
assert classifier.index("item_consumable_raidpack_") < classifier.index('lower.find("item_consumable_")==0')
assert classifier.index('name.find("(Expired)")') < classifier.index("_consumable_potion_")
assert classifier.index("_consumable_potion_") < classifier.index('lower.find("pack")')
assert classifier.index("item_consumable_tome_") < classifier.index('lower.find("item_consumable_")==0')
assert classifier.index("item_consumable_planpack_") < classifier.index('lower.find("item_consumable_")==0')
assert classifier.index("_currency_") < classifier.index('lower.find("item_resources_")==0')
assert classifier.index("_treeseed_") < classifier.index('lower.find("item_farming_")==0')
assert "item_scarabstaff_" not in classifier
assert "item_dr_main_reforged_greatsword_gatekeeper" not in classifier
assert "item_throwable_fireoil" not in classifier
assert classifier.index("item_resources_fireoil") < classifier.index('lower.find("item_resources_")==0')

# The empty-slot dialog, persistent Add Item panel, and Withdraw dialog expose
# the same explicit clear-search behavior instead of relying on a
# browser-specific search-field affordance.
assert page.count("searchClear.className='search-clear'") == 3
assert page.count("searchClear.setAttribute('aria-label','Clear item search')") == 3
assert page.count("search.dispatchEvent(new Event('input',{bubbles:true}))") == 3
assert ".search-clear[hidden] { display:none; }" in page

print("item catalog categories: ok")
