param([string]$BaseUrl = 'http://game.mcgalaxy.com:29105', [int]$TimeoutSeconds = 180)
$ErrorActionPreference='Stop'
$deadline=(Get-Date).AddSeconds($TimeoutSeconds)
$lastError='validation conditions not yet satisfied'
do {
    try {
        & curl.exe -fsS --max-time 5 "$BaseUrl/" -o NUL
        $pageOk=$LASTEXITCODE -eq 0
        $players=Invoke-RestMethod "$BaseUrl/api/players" -TimeoutSec 5
        $known=Invoke-RestMethod "$BaseUrl/api/known-players" -TimeoutSec 5
        $leaderboard=Invoke-RestMethod "$BaseUrl/api/kill-leaderboard" -TimeoutSec 5
        $kills=Invoke-RestMethod "$BaseUrl/api/kill-tracker-diagnostics" -TimeoutSec 5
        & curl.exe -fsS --max-time 5 "$BaseUrl/map" -o NUL
        $mapOk=$LASTEXITCODE -eq 0
        $mapPlayers=Invoke-RestMethod "$BaseUrl/api/map/players" -TimeoutSec 5
        $labels=Invoke-RestMethod "$BaseUrl/api/map/labels" -TimeoutSec 5
        $lodestones=Invoke-RestMethod "$BaseUrl/api/map/lodestones" -TimeoutSec 5
        $lodestoneDiagnostics=Invoke-RestMethod "$BaseUrl/api/lodestone-diagnostics" -TimeoutSec 5
        $nodestones=Invoke-RestMethod "$BaseUrl/api/map/nodestones" -TimeoutSec 5
        $nodestoneDiagnostics=Invoke-RestMethod "$BaseUrl/api/nodestone-diagnostics" -TimeoutSec 5
        $resourceCatalog=Invoke-RestMethod "$BaseUrl/api/map/resources/catalog" -TimeoutSec 20
        $resourceState=Invoke-RestMethod "$BaseUrl/api/map/resources/state" -TimeoutSec 5
        $resourceDiagnostics=Invoke-RestMethod "$BaseUrl/api/resource-diagnostics" -TimeoutSec 5
        $npcCatalog=Invoke-RestMethod "$BaseUrl/api/map/npcs/catalog" -TimeoutSec 20
        $npcState=Invoke-RestMethod "$BaseUrl/api/map/npcs/state" -TimeoutSec 5
        $npcDiagnostics=Invoke-RestMethod "$BaseUrl/api/diagnostics/npcs" -TimeoutSec 5
        $npcIconsOk=($npcCatalog.types|Where-Object {$_.icon}).Count -gt 0
        foreach($icon in @('T_Icon_Journal_Black_Knight_Warrior.png','T_Icon_Journal_Lesser_Blue_Dragon.png','T_Icon_Journal_Wise_Old_man.png')){
            $code=& curl.exe -sS --max-time 5 -o NUL -w '%{http_code}' "$BaseUrl/assets/npcs/$icon"
            if($code -ne '200'){$npcIconsOk=$false;break}
        }
        $playerDiagnostics=Invoke-RestMethod "$BaseUrl/api/player-discovery-diagnostics" -TimeoutSec 5
        $playerRecipeDiagnostics=Invoke-RestMethod "$BaseUrl/api/player-recipe-diagnostics" -TimeoutSec 5
        $questCatalog=Invoke-RestMethod "$BaseUrl/api/quests/catalog" -TimeoutSec 5
        $questDiagnostics=Invoke-RestMethod "$BaseUrl/api/quest-tracker-diagnostics" -TimeoutSec 5
        $playerSkillDiagnostics=Invoke-RestMethod "$BaseUrl/api/diagnostics/player-skills" -TimeoutSec 5
        $planDiagnostics=Invoke-RestMethod "$BaseUrl/api/diagnostics/storage-plans" -TimeoutSec 5
        $update=Invoke-RestMethod "$BaseUrl/api/update-status" -TimeoutSec 5
        $adminDiagnostics=Invoke-RestMethod "$BaseUrl/api/admin-diagnostics" -TimeoutSec 5
        & curl.exe -sS --max-time 5 -o NUL -w '%{http_code}' "$BaseUrl/admin/claim" | Out-Null
        $claimPageOk=$LASTEXITCODE -eq 0
        $unauthorizedCode=& curl.exe -sS --max-time 5 -o NUL -w '%{http_code}' "$BaseUrl/api/admin/storage/catalog"
        $unlockIconsOk=$true
        foreach($icon in @('DA_Consumable_Vestige_Weapon_Club_Swingslash.png','DA_Consumable_Vestige_Weapon_Longbow_HunterStag.png','DA_Consumable_Vestige_Weapon_Staff_GrievingMoon.png','DA_Consumable_Vestige_Weapon_Staff_ToktzMejTal.png','DA_Consumable_Vestige_Weapon_Whip_Abysmal.png','DA_Consumabe_Mount_TerrorBird_02_04.png','DA_Consumabe_Mount_TerrorBird_02_02.png')){
            $code=& curl.exe -sS --max-time 5 -o NUL -w '%{http_code}' "$BaseUrl/assets/items/$icon"
            if($code -ne '200'){$unlockIconsOk=$false;break}
        }
        if($pageOk -and $null -ne $players.count -and $null -ne $known.count -and
           $null -ne $leaderboard.players -and $mapOk -and
           $null -ne $mapPlayers.players -and $labels.labels.Count -gt 0 -and
           $lodestoneDiagnostics.compatibility -eq 1 -and $null -ne $lodestones.lodestones -and
           $nodestoneDiagnostics.compatibility -eq 1 -and $null -ne $nodestones.nodestones -and
           $resourceCatalog.nodes.Count -gt 0 -and $null -ne $resourceState.states -and
           $resourceDiagnostics.compatibility -eq 1 -and $playerDiagnostics.gameStateValid -eq 1 -and
           $playerRecipeDiagnostics.compatibility -eq 1 -and $playerRecipeDiagnostics.catalogCompatibility -eq 1 -and
           $playerRecipeDiagnostics.progressLayoutCompatibility -eq 1 -and $playerRecipeDiagnostics.queryCompatibility -eq 1 -and
           $playerRecipeDiagnostics.catalogEntries -eq $playerRecipeDiagnostics.resolvedEntries -and
           $questCatalog.quests.Count -gt 0 -and $questDiagnostics.compatibility -eq 1 -and
           $questDiagnostics.catalogCompatibility -eq 1 -and $questDiagnostics.componentCompatibility -eq 1 -and
           $questDiagnostics.queryCompatibility -eq 1 -and $questDiagnostics.catalogEntries -eq $questDiagnostics.resolvedEntries -and
           $questDiagnostics.maximumQueryMicroseconds -le 1000 -and $questDiagnostics.slowQueries -eq 0 -and
           $playerSkillDiagnostics.iconCompatibility -eq 1 -and $playerSkillDiagnostics.iconAssets -eq 12 -and $playerSkillDiagnostics.missingIconAssets -eq 0 -and
           $planDiagnostics.compatibility -eq 1 -and $planDiagnostics.items -eq 123 -and
           ($players.count -eq 0 -or ($playerRecipeDiagnostics.snapshots -gt 0 -and $playerRecipeDiagnostics.scansCompleted -gt 0)) -and
           $npcCatalog.nodes.Count -gt 0 -and $npcState.states.Count -eq $npcCatalog.nodes.Count -and $npcIconsOk -and
           $npcDiagnostics.catalogCompatibility -eq 1 -and $npcDiagnostics.registryCompatibility -eq 1 -and
           $npcDiagnostics.identityCompatibility -eq 1 -and $npcDiagnostics.transformCompatibility -eq 1 -and
           $npcDiagnostics.lifecycleCompatibility -eq 1 -and $npcDiagnostics.respawnCompatibility -eq 1 -and
           $npcDiagnostics.dungeonExclusionCompatibility -eq 1 -and $npcDiagnostics.completedScans -gt 0 -and
           $npcDiagnostics.iconCompatibility -eq 1 -and $npcDiagnostics.missingIconAssets -eq 0 -and $update.npcIconCompatibility -eq 1 -and
           $npcDiagnostics.maximumSliceMicroseconds -gt 0 -and $npcDiagnostics.maximumSliceMicroseconds -le 10000 -and
           $npcDiagnostics.lastGenerationWorkMicroseconds -gt 0 -and
           $npcDiagnostics.lastFinalizationMicroseconds -gt 0 -and
           $npcDiagnostics.lastDutyCycleBasisPoints -ge 0 -and $npcDiagnostics.lastDutyCycleBasisPoints -le 10000 -and
           $npcDiagnostics.idleMilliseconds -ge 250 -and
           ($players.count -eq 0 -or ($playerDiagnostics.spawnedPawns -ge $players.count -and $playerDiagnostics.worldLocations -ge $players.count -and $kills.regionCompatibility -eq 1)) -and
           $kills.hookCompatibility -eq 1 -and $adminDiagnostics.authorizationStorage -eq 1 -and $adminDiagnostics.consoleCommandCompatibility -eq 1 -and
           $adminDiagnostics.teleportCompatibility -eq 1 -and $adminDiagnostics.teleportGroundTraceCompatibility -eq 1 -and $adminDiagnostics.storageTeleportPipelineCompatibility -eq 1 -and
           $adminDiagnostics.storageCompatibility -eq 1 -and $adminDiagnostics.storageDispatcherCompatibility -eq 1 -and $adminDiagnostics.storageMutationCompatibility -eq 1 -and
           $adminDiagnostics.storageBatchCompatibility -eq 1 -and $adminDiagnostics.storageOrganizationCompatibility -eq 1 -and $adminDiagnostics.storageRenameCompatibility -eq 1 -and
           $adminDiagnostics.storageRemovalTrackingCompatibility -eq 1 -and $adminDiagnostics.storageCatalogCompatibility -eq 1 -and $adminDiagnostics.storageUnlockCatalogCompatibility -eq 1 -and $unlockIconsOk -and
           $update.adminBuildingTaxonomyCompatibility -eq 1 -and $update.adminBuildingRequirementsCompatibility -eq 1 -and
           $update.adminBuildingIconsCompatibility -eq 1 -and $update.adminCraftingRecipeCompatibility -eq 1 -and
           $update.adminBuildingExpectedPieces -eq 827 -and $update.adminBuildingResolvedPieces -eq 827 -and
           $update.adminBuildingUnavailablePieces -eq 0 -and $update.adminBuildingReferences -eq 844 -and
           $adminDiagnostics.gravestoneCompatibility -eq 1 -and $adminDiagnostics.gravestoneInventoryCompatibility -eq 1 -and $adminDiagnostics.gravestoneRegistryCompatibility -eq 1 -and
           $adminDiagnostics.gravestoneStreamingCompatibility -eq 1 -and $adminDiagnostics.gravestoneRespawnCaptureCompatibility -eq 1 -and $adminDiagnostics.gravestonePersistenceCompatibility -eq 1 -and
           $update.website -eq $true -and $update.playerSkillIconCompatibility -eq 1 -and $update.playerSkillIconAssets -eq 12 -and $update.playerSkillMissingIconAssets -eq 0 -and $update.adminStoragePlanCatalogCompatibility -eq 1 -and $update.adminStoragePlanCatalogItems -eq 123 -and $update.adminAuthorizationStorage -eq $true -and $update.adminConsoleCompatibility -eq 1 -and
           $update.adminTeleportCompatibility -eq 1 -and $update.adminStorageCompatibility -eq 1 -and $update.adminGravestoneCompatibility -eq 1 -and
           $claimPageOk -and $unauthorizedCode -eq '401' -and $update.mapProjectionCompatibility -eq $true){
            $livePlayerCheck=if($players.count -eq 0){'pending-no-online-player'}else{'passed'}
            [pscustomobject]@{website='ok';onlinePlayers=$players.count;knownPlayers=$known.count;livePlayerCheck=$livePlayerCheck;playerDataCompatibility=$update.playerDataCompatibility;killHookCompatibility=$kills.hookCompatibility;regionCompatibility=$kills.regionCompatibility;mapProjectionCompatibility=$update.mapProjectionCompatibility;mapLabels=$labels.labels.Count;lodestones=$lodestones.lodestones.Count;lodestoneCompatibility=$lodestoneDiagnostics.compatibility;nodestones=$nodestones.nodestones.Count;nodestoneCompatibility=$nodestoneDiagnostics.compatibility;resourceCatalogNodes=$resourceCatalog.nodes.Count;resourceCompatibility=$resourceDiagnostics.compatibility;npcCatalogNodes=$npcCatalog.nodes.Count;npcIcons=$npcIconsOk;npcLiveNodes=$npcDiagnostics.matchedLiveNodes;npcCatalogCompatibility=$npcDiagnostics.catalogCompatibility;npcRuntimeCompatibility=$npcDiagnostics.registryCompatibility;npcLifecycleCompatibility=$npcDiagnostics.lifecycleCompatibility;npcCompletedScans=$npcDiagnostics.completedScans;npcLastScanMilliseconds=$npcDiagnostics.lastScanMilliseconds;npcMaximumSliceMicroseconds=$npcDiagnostics.maximumSliceMicroseconds;npcGenerationWorkMicroseconds=$npcDiagnostics.lastGenerationWorkMicroseconds;npcFinalizationMicroseconds=$npcDiagnostics.lastFinalizationMicroseconds;npcLiveRefreshMicroseconds=$npcDiagnostics.lastLiveRefreshMicroseconds;npcDutyCycleBasisPoints=$npcDiagnostics.lastDutyCycleBasisPoints;npcClassCacheHits=$npcDiagnostics.classCacheHits;npcClassCacheMisses=$npcDiagnostics.classCacheMisses;npcInvalidObjectSlots=$npcDiagnostics.invalidObjectSlots;npcChangedObjectSlots=$npcDiagnostics.changedObjectSlots;npcIdleMilliseconds=$npcDiagnostics.idleMilliseconds;adminAuthorizationCompatibility=$adminDiagnostics.authorizationStorage;adminConsoleCompatibility=$adminDiagnostics.consoleCommandCompatibility;adminTeleportCompatibility=$adminDiagnostics.teleportCompatibility;adminTeleportGroundTraceCompatibility=$adminDiagnostics.teleportGroundTraceCompatibility;storageCompatibility=$adminDiagnostics.storageCompatibility;storageMutationCompatibility=$adminDiagnostics.storageMutationCompatibility;storageBatchCompatibility=$adminDiagnostics.storageBatchCompatibility;storageRenameCompatibility=$adminDiagnostics.storageRenameCompatibility;storageUnlockCatalogCompatibility=$adminDiagnostics.storageUnlockCatalogCompatibility;buildingPieces=$adminDiagnostics.resolvedBuildingPieces;buildingReferences=$adminDiagnostics.buildingReferences;unlockIcons=$unlockIconsOk;gravestoneCompatibility=$adminDiagnostics.gravestoneCompatibility;gravestonePersistenceCompatibility=$adminDiagnostics.gravestonePersistenceCompatibility;update=$update.updater}|ConvertTo-Json -Depth 8
            exit 0
        }
        $lastError='one or more compatibility conditions were false'
    } catch {$lastError="line $($_.InvocationInfo.ScriptLineNumber): $($_.Exception.Message)"}
    Start-Sleep 3
} while((Get-Date)-lt$deadline)
throw "Development validation did not pass within $TimeoutSeconds seconds. Last result: $lastError"
