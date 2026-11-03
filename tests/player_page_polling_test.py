from pathlib import Path

root = Path(__file__).resolve().parents[1]
source = (root / "src/web_page.h").read_text()
storage = (root / "src/engine/admin_storage.cpp").read_text()
gravestones = (root / "src/engine/admin_gravestone.cpp").read_text()
activity_discovery = (root / "src/engine/player_activity_discovery.cpp").read_text()
activity_store = (root / "src/player_activity_store.cpp").read_text()
quest_discovery = (root / "src/engine/player_quests.cpp").read_text()

assert "let activePlayerView='overview'" in source
assert "activePlayerView==='storage')refreshStorage()" in source
assert "activePlayerView==='gravestones')refreshGravestones()" in source
assert "activePlayerView==='recipes')refreshPlayerRecipes()" in source
assert "if(!gravestoneAdmin.hidden)refreshGravestones()" not in source
assert "if(!storageAdmin.hidden)refreshStorage()" not in source
assert "storageLoading" in source and "graveLoading" in source
assert "finally { storageLoading=false; }" in source
assert "finally{graveLoading=false;if(graveRefreshPending)" in source
assert "const std::string pollingScript" in source
assert "window.ZanikPolling" in source
assert "ZanikPolling.add('global-live-status',2000,refreshLive)" in source
assert "if(!liveLoaded||count!==liveCount)" in source
assert "ZanikPolling.add('player-storage',2000" in source
assert "ZanikPolling.add('player-gravestones',2000" in source
assert "if(graveLoading){if(force)graveRefreshPending=true;return}" in source
assert "if(graveRefreshPending){graveRefreshPending=false;queueMicrotask(()=>refreshGravestones(true))}" in source
assert "ZanikPolling.add('player-quest-maps',2000,()=>{if(activePlayerView==='overview'||activePlayerView==='quests')refreshQuestStepMaps()})" in source
assert "ZanikPolling.add('world-map',2000" in source
assert "document.hidden&&task.pauseWhenHidden" in source
assert "task.running" in source
assert "scheduleOnlineDuration()" in source
assert "setInterval(updateOnlineDuration" not in source
assert "1025 - Date.now() % 1000" in source
assert "activityEventSignature" in source
assert "if(direction!=='refresh')renderActivity(false)" in source
assert "renderActivity(rebuild)" in source
assert "if(activityEvents.length)renderActivity()" not in source
assert "ZanikPolling.add('player-activity',2000,()=>{if(activePlayerView==='overview')refreshActivity()})" in source
assert "activityList.querySelectorAll('time[data-timestamp]')" in source
assert "RefreshKnownStorageSnapshot" in storage
assert "createdAt<std::chrono::seconds(2)" in storage
assert "discoveredAt<std::chrono::seconds(30)" in storage
assert "gravestoneViewGeneration" in gravestones
assert "CachedGravestoneView" in gravestones
assert "ObservedLiveGrave" in gravestones
assert "ObserveKnownLiveGraves(characterGuid)" in gravestones
assert "now-cached->second.createdAt<std::chrono::seconds(30)" in gravestones
assert "gravestoneViewGeneration.fetch_add(1)" in gravestones
assert '{3,"Whispering Swamp"}' in activity_discovery
assert 'FindFunction("DominionPlayerController","GetRevealedMapFogComponent")' in activity_discovery
assert 'FindUniqueFunctionByName("Client_OnRegionRevealed")' in activity_discovery
assert "regionResolutionAttempts<3" in activity_discovery
assert "ResolveRegionRevealFunction()" in activity_discovery
assert "EnsurePlayerProcessEventCapture(moduleBase,moduleEnd)" in activity_discovery
assert "ObservePlayerActivityProcessEvent(object,function,params)" in quest_discovery
assert "DrainNativeRegionDiscoveries()" in activity_discovery
assert "RecordRegionDiscoveries" in activity_store
assert "LogMapRegions: Revealing map region [" in activity_discovery
assert "ReadRegionServerLog(current,capturedAt)" in activity_discovery
assert "regionServerLogOffset=end<0?0:std::streamoff(end)" in activity_discovery
assert '"BP_RevealedMapFogComponent","RevealedMapFogComponent","RevealedMapFogComponentRef"' in activity_discovery
assert "regionFallbackScannedControllers.insert(controllerIdentity).second" in activity_discovery
assert "regionCatalogScanned" not in activity_discovery

print("unified two-second polling and known-object observation avoid repeated registry scans: ok")
