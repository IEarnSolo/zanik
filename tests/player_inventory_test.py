from pathlib import Path

root = Path(__file__).resolve().parents[1]
native = (root / "src/engine/admin_storage.cpp").read_text(encoding="utf-8")
main = (root / "src/main.cpp").read_text(encoding="utf-8")
web = (root / "src/web_page.h").read_text(encoding="utf-8")
analyzer = (root / "tools/compat_analyzer.cpp").read_text(encoding="utf-8")

for token in [
    "BuildPlayerInventorySnapshot",
    "FindPlayerInventory(player,&inventories)",
    "FindPlayerLoadout(player,&inventories)",
    "FindPlayerPersonalInventory(player,&inventories)",
    'IsA(inventory,"PersonalInventoryComponent")',
    '"0000000000000005","Personal chest"',
    'PagedSlots(playerInventory,1)',
    'PagedSlots(playerInventory,0)',
    'PagedSlots(playerInventory,2)',
    'PagedSlots(playerInventory,3)',
    'quick.insert(quick.end(),items.begin(),items.end())',
    'inventory.hotbarCount=hotbarCount',
    'EquipmentSlots()',
    'static const std::vector<int> slots{0,1,2,3,4,11,7,8}',
    'constexpr int HeldRightBackingSlot=7',
    'constexpr int HeldLeftBackingSlot=8',
    'bool IsMirroredHeldLeftItem',
    'candidate.slot==HeldRightBackingSlot&&candidate.objectIdentity==item.objectIdentity',
    'item.held=std::any_of(equipment->items.begin(),equipment->items.end()',
    '(candidate.slot==HeldRightBackingSlot||candidate.slot==HeldLeftBackingSlot)&&candidate.objectIdentity==item.objectIdentity',
    'if(!IsMirroredHeldLeftItem(*container,item))stored.items.push_back',
    'DisplaySlot(candidate)==slot&&!IsMirroredHeldLeftItem(*container,candidate)',
    'loadoutSlotValid',
    'addLoadoutItemToSlot',
    'removeLoadoutItem',
    'heldItemEquipmentCompatibility=addLoadoutItemToSlot&&removeLoadoutItem?1:-1',
    'ActionKind::EquipHeld',
    'ActionKind::UnequipHeld',
    'ActionKind::RelocateHeld',
    'loadoutSourceSlot',
    'MatchingInventoryItems',
    'ReadInventoryState',
    'sourcePreserved&&heldReference&&duplicatesPreserved&&inventoryPreserved',
    'sourcePreserved&&heldReferencesCleared&&duplicatesPreserved&&inventoryPreserved',
    'a.destinationSlot==HeldRightBackingSlot||a.destinationSlot==HeldLeftBackingSlot',
    'Move this held item to the character inventory before equipping it',
    'pagedSlotValid',
    'IsPlayerInventoryContainerId',
    'requiresTargetOnline',
    'r.actionIndex.load()==0',
    'OperationFamily::Equipment',
    'EquipmentGameThreadLimitMicroseconds=10000',
    'GameThreadLimit(action.family)',
    'This is already your linked character',
    'value.canUnequip=true',
    'Held-item unequip is unavailable for this game build',
    'playerSlotMove=operation=="slot_move"&&destinationChestId==chestId',
    'const int destinationBacking=BackingSlot(*destination,destinationSlot)',
    'Inventory grid dragging cannot cross container boundaries',
    'The destination item cannot swap into the source slot',
    'Two-handed items remain in HeldRight',
    'a.kind=ActionKind::RelocateHeld',
    'if(!item->held){error="This inventory item is not equipped"',
    'if(!addHeldUnequip(*item,*reference))return false',
    '\\"held\\":"<<(item.held?"true":"false")',
    'chestId=="0000000000000002"?SnapshotFindChest(*snapshot,"0000000000000001"):chest',
    'GetAdminPlayerInventoryJson',
    'ConfigurePlayerInventorySnapshots',
    'snapshotPersistenceCompatibility',
    'snapshotCapturedAt',
    'snapshotAvailable',
    'snapshotPersisted',
    'PersistPlayerInventoryDisplaySnapshot',
    'playerInventorySnapshotOnline',
    'IsPlayerInventoryAutofillId',
    'BuildAutofillSnapshot',
    'value.items.push_back({DisplaySlot(item)',
    'request->requiresTargetOnline=IsPlayerInventoryAutofillId',
    'BackingSlot(*chest,plan.destinationSlot)',
    'Equipment autofill is unavailable',
    'autofillCapturedAt',
    'autofillSnapshotState',
    'event.sourceSlot=sourceSlot',
    'event.destinationSlot=destinationSlot',
    'event.observedAtMicros=MonotonicMicros()',
    'signalPlayer(event.source,event.sourceSlot)',
    'signalPlayer(event.destination,event.destinationSlot)',
    'candidate.ownerGuid==trigger.ownerGuid&&candidate.chestId==trigger.chestId',
    'AutofillGameThreadLimitMicroseconds=10000',
    'duplicate=std::find_if(autofillMutationTriggers.begin()',
    'AutofillRpcCorrelationMicroseconds=250000',
    'rpcWithoutSlot',
    'BuildTargetedPlayerAutofillSnapshot',
    'PlayerOwnsInventory(inventory,*player)',
    'ActionKind::CopyItem',
    'CopyExactItem',
    'ActionKind::AddMasterwork',
    'AddMasterworkItem',
    'onRepMasterworkUpgrades',
    'a.desiredPowerLevel!=9||power<base||power>=a.desiredPowerLevel',
    'int(priorCount)!=power-base',
    'desiredUpgrades<=priorCount',
    'created+0xB9',
    'created+0xBA',
    'ItemPowerLevel',
    'IsMasterworkItemData',
    'basePowerLevel',
    'powerLevel',
    'masterwork',
]:
    assert token in native, f"missing native player-inventory contract: {token}"

assert 'adminPlayerHeldItemEquipmentCompatibility' in main

assert 'power!=8' not in native, "power-9 fallback must support lower native stopping points"

for token in [
    '\\"inventoryRead\\"',
    '\\"inventoryMutation\\"',
    'path=="/api/admin/player-inventory"',
    'playerInventory&&!target.online',
    'adminPlayerInventoryCompatibility',
    'adminPlayerRuneInventoryCompatibility',
    'adminPlayerAmmunitionInventoryCompatibility',
    'adminPlayerEquipmentCompatibility',
    'adminPlayerPersonalChestCompatibility',
    'adminStorageEquipmentMaximumMicroseconds',
    'operation=="equip"||operation=="unequip"',
    'urgentAutofillBurstLimit=8',
    'urgentAutofillBurstBudget=std::chrono::milliseconds(50)',
    'player-inventory-snapshots.db',
    'ConfigurePlayerInventorySnapshots',
    'adminStorageItemInstanceCompatibility',
]:
    assert token in main, f"missing authenticated player-inventory API contract: {token}"

for token in [
    'data-player-view="inventory"',
    'data-player-panel="inventory"',
    'function refreshInventory',
    'function renderInventoryGrid',
    'function bindInventorySlotDrag',
    'function showInventoryContext',
    "take.textContent='Take…'",
    "copy.textContent='Copy'",
    "equip.textContent='Equip'",
    "unequip.textContent='Unequip'",
    "remove.textContent='Delete'",
    "add.textContent='Add…'",
    "if(canMutate&&!container.equipment)",
    "if(inventoryMutationAvailable&&!container.equipment)renderAddPanel",
    "canAutofill=inventoryAutofillAvailable&&!container.equipment",
    "multi.hidden=!selectionAvailable",
    "if(selectionAvailable)slot.onclick",
    "inventoryAutofillAvailable=storageAutofillAvailable&&Boolean(data.snapshotAvailable)",
    "toggle.disabled=!inventoryAutofillAvailable",
    "playerInventory.classList.toggle('offline',!data.online)",
    "new Date(captured*1000).toLocaleString()",
    'renderInventoryContainer(data.runes)',
    'renderInventoryContainer(data.ammunition)',
    'renderInventoryContainer(data.personal)',
    'inventory-hotbar-slot',
    'inventory-hotbar-divider',
    "dragToggle.className='storage-drag-toggle'",
    'dragToggle.hidden=!dragAvailable',
    '.storage-drag-toggle[hidden] { display:none!important; }',
    "dragAvailable=inventoryMutationAvailable",
    "enqueueStorageMove(container,container,entry,index,Number(target.dataset.slot))",
    "if(dragAvailable)bindInventorySlotDrag(slot,entry,index,container,selected,update)",
    "entry?.held?' held-item':'')",
    "entry.held?'; equipped in hand':''",
    ".chest[data-chest=\"0000000000000001\"] .storage-slot.held-item",
    'border-color:#A66424',
    'margin:0 0 -.15rem',
    'entries[0].canUnequip!==false',
    "container.id==='0000000000000001'&&entries[0].held",
    'center calc(50% - .125rem)',
    'index<hotbarCount',
    "ZanikPolling.add('player-inventory',2000",
    "refresh=()=>chest.playerInventory?refreshInventory(true):refreshStorage(true)",
    "snapshotState=entry?.autofillSnapshotState",
    "toggle.setAttribute('aria-label','Autofill from snapshot')",
    "empty or matching slots are restored and foreign items are never overwritten or removed",
    "deposit.textContent='Deposit'",
    "mutateGrid('deposit_inventory'",
    "mutateGrid('deposit_personal'",
    "mutateGrid('move_to_storages_batch'",
    "mutateGrid('move_to_storage_batch'",
    "moveInventorySelectionToStorages(container,entries)",
    "function showInventoryStorageDestinationDialog(source,entries,matchingFallback=true)",
    "No storage contains an existing stack for the selected items",
    "appendStorageDestinationLabel(button,destination)",
    ".storage-context strong,.storage-destination-list strong",
    "direct.textContent=container.id==='0000000000000001'?'Personal chest':container.id==='0000000000000005'?'Inventory':'Personal chest'",
    "container.equipment?'move_equipment_batch':'move_batch'",
    "container.equipment?showInventoryStorageDestinationDialog(container,entries,false):moveInventorySelectionToStorages(container,entries)",
    "async function showInventoryWithdrawDialog(destination)",
    "includePersonal=destination.id==='0000000000000001'",
    "for(const chest of payloads[0].chests||[])collect(chest.items)",
    "if(includePersonal)collect(payloads[1].personal?.items)",
    "Selected: '+entry.name+' · You have '",
    "quantity.focus();quantity.select()",
    "zanik-withdraw-personal-first",
    "priorityLabel.hidden=!includePersonal",
    "mutateGrid('withdraw',destination",
    "container.id==='0000000000000001'||container.id==='0000000000000005'",
    "withdraw.textContent='Withdraw…'",
    "withdraw.onclick=()=>showInventoryWithdrawDialog",
    ".catalog-mode-label[hidden] { display:none!important; }",
    "filling matching stacks before using empty slots. Hotbar and autofill-snapshotted slots are ignored.",
    "power.setAttribute('aria-label','Power level')",
    "power.className='power-level-select'",
    ".power-level-select[hidden],.storage-delete-quantity[hidden]{display:none!important}",
    "body.powerLevel=Number(powerLevel)",
    "entry.masterwork&&Number(entry.powerLevel)>0?' · Power '",
    "#playerInventory.offline{opacity:.64;filter:saturate(.68)}",
]:
    assert token in web, f"missing player-inventory UI contract: {token}"

inventory_context = web[web.index("function showInventoryContext("):web.index("function renderInventoryGrid(")]
assert "container.id==='0000000000000001'||container.id==='0000000000000002'||container.id==='0000000000000005'" in inventory_context
assert "moveInventorySelectionToStorages(container,entries)" in inventory_context
assert inventory_context.index("take.textContent='Take…'") < inventory_context.index("add.textContent='Add…'")
assert inventory_context.index("add.textContent='Add…'") < inventory_context.index("trigger.textContent='Move…'")
assert inventory_context.index("trigger.textContent='Move…'") < inventory_context.index("withdraw.textContent='Withdraw…'")
assert inventory_context.index("withdraw.textContent='Withdraw…'") < inventory_context.index("equip.textContent='Equip'")
assert inventory_context.index("entries[0].held") < inventory_context.index("unequip.textContent='Unequip'")

inventory_render = web[web.index("function renderInventoryContainer("):web.index("async function refreshInventory(")]
assert "card.addEventListener('click',event=>" in inventory_render
assert "event.target.closest('.storage-slot:not(.occupied)')" in inventory_render
assert "event.target.closest('button,input,select,textarea,label,a,summary')" in inventory_render
assert "clearStorageSelection(container.id)" in inventory_render

withdraw_dialog = web[web.index("async function showInventoryWithdrawDialog("):web.index("function showInventoryContext(")]
assert "quantity.max=String(entry.quantity)" in withdraw_dialog
assert "prioritizePersonalChest:includePersonal&&priority.checked" in withdraw_dialog
assert "No items are available in Storages or Personal Chest." in withdraw_dialog
assert "No items are available in Storages." in withdraw_dialog
assert "priorityLabel.hidden=!includePersonal" in withdraw_dialog

assert web.index('data-player-view="inventory"') < web.index('data-player-view="storage"')
assert web.index('data-player-panel="inventory"') < web.index('data-player-panel="storage"')
assert 'renderInventoryContainer(data.inventory),renderInventoryContainer(data.equipment)' in web
for removed in [
    'Backpack and subtly highlighted hotbar · 40 slots',
    'Eight equipped weapon, shield, armour, cape, jewellery and amulet slots.',
    'Rune backpack.',
    'Ammunition and arrows backpack.',
]:
    assert removed not in web
for token in ["playerInventoryGetter", "playerPersonalInventoryGetter", "playerLoadoutGetter", "playerLoadoutSlot", "playerPagedSlotValid", "playerPagedSlotOffset", "playerPagedSlotCount", "playerLoadoutSlotValid", "playerLoadoutRemoveItem", "loadoutRemoveItemSignature", "player inventory, personal chest, or eight-slot loadout symbols missing", "storageCopyItem", "storageItemPower", "storageCanMasterwork", "storageApplyMasterwork", "storageOnRepMasterwork", "OnRep_NumMasterworkUpgradesApplied", "exact item copy or masterwork signature changed"]:
    assert token in analyzer, f"missing player-inventory analyzer contract: {token}"

print("admin player inventory and equipment regression checks passed")
