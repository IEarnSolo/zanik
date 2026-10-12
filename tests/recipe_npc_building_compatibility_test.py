from pathlib import Path

root = Path(__file__).resolve().parents[1]
player_recipes = (root / "src/engine/player_recipes.cpp").read_text()
npcs = (root / "src/engine/npcs.cpp").read_text()
storage = (root / "src/engine/admin_storage.cpp").read_text()
bootstrap = (root / "deploy/zanik-bootstrap.sh").read_text()
validator = (root / "deploy/validate-dev-update.ps1").read_text()

assert 'ValidateQueryLayout(recipeQuery,"RecipeData")' in player_recipes
assert 'ValidateQueryLayout(buildingQuery,"BuildingData")' in player_recipes
assert 'input==0&&result==8' in player_recipes
assert '# zanik-npc-identities' in npcs
assert 'catalog.size()==expected' in npcs
assert 'ValidateTransformLayout()' in npcs
assert 'Offsets::Actor_RootComponent' in npcs and 'Offsets::SceneComp_RelativeLocation' in npcs
assert 'value.schema==1&&fingerprintValid' in storage
assert '$DATA/player-recipe-snapshots.db' in bootstrap
for check in ('expectedBuildingPieces -eq 824', 'resolvedBuildingPieces -eq 824',
              'unavailableBuildingPieces -eq 0', 'buildingReferences -eq 841'):
    assert check in validator

header = (root / "deploy/npc-identities.tsv").read_text(encoding="utf-8").splitlines()[0].split("\t")
assert header[0] == "# zanik-npc-identities"
assert header[1] == "1"
assert int(header[2]) == 2926
assert len(header[3]) == 64

print("recipe/NPC/building compatibility coverage: ok")
