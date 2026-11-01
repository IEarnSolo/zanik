from pathlib import Path

root = Path(__file__).resolve().parents[1]
source = (root / "src/web_page.h").read_text()
storage = (root / "src/engine/admin_storage.cpp").read_text()
gravestones = (root / "src/engine/admin_gravestone.cpp").read_text()

assert "let activePlayerView='overview'" in source
assert "activePlayerView==='storage')refreshStorage()" in source
assert "activePlayerView==='gravestones')refreshGravestones()" in source
assert "activePlayerView==='recipes')refreshPlayerRecipes()" in source
assert "if(!gravestoneAdmin.hidden)refreshGravestones()" not in source
assert "if(!storageAdmin.hidden)refreshStorage()" not in source
assert "storageLoading" in source and "graveLoading" in source
assert "finally { storageLoading=false; }" in source
assert "finally{graveLoading=false}" in source
assert "createdAt<std::chrono::seconds(30)" in storage
assert "gravestoneViewGeneration" in gravestones
assert "CachedGravestoneView" in gravestones
assert "now-cached->second.createdAt<std::chrono::seconds(30)" in gravestones
assert "gravestoneViewGeneration.fetch_add(1)" in gravestones

print("player page polling and native snapshot caching avoid repeated registry scans: ok")
