const fs = require('fs');
const path = require('path');
const vm = require('vm');

const source = fs.readFileSync(path.join(__dirname, '..', 'src', 'web_page.h'), 'utf8');
const playerPage = source.match(/constexpr const char\* kPlayerPage = R"DWHTML\(([\s\S]*?)\)DWHTML";/);
const playersPage = source.match(/constexpr const char\* kPlayersPage = R"DWHTML\(([\s\S]*?)\)DWHTML";/);
if (!playerPage || !playersPage) throw new Error('player pages not found');
if ((playerPage[1].match(/backdrop\.className='storage-delete-backdrop'/g) || []).length !== (playerPage[1].match(/setAttribute\('aria-modal','true'\)/g) || []).length) {
  throw new Error('a full-window player modal is outside the shared background-scroll lock');
}

for (const token of [
  '@media(max-width:640px)',
  '.storage-grid{grid-template-columns:repeat(8,minmax(0,1fr));width:100%',
  '.grave-grid{grid-template-columns:repeat(8,minmax(0,1fr));width:100%',
  '.storage-slot{box-sizing:border-box;width:100%;height:auto;min-width:0;min-height:0;aspect-ratio:1/1;overflow:hidden',
  '.storage-slot>img{position:absolute;inset:7%;width:86%;height:86%',
  '.storage-drag-toggle { display:none; }',
  '.storage-drag-toggle[hidden] { display:none!important; }',
  'id="storageSelectionTip"',
  '.storage-selection-tip-title',
  'bottom:calc(var(--site-bottom-nav-h) + .4rem)',
  'max-height:min(7rem,22dvh)',
  '.storage-grid .storage-slot{touch-action:pan-y}',
  '.storage-grid.touch-drag-enabled .storage-slot{touch-action:none}',
  'id="recipePagination"',
  'recipeMobilePageSize=100',
  "matchMedia('(max-width:760px)').matches",
  'recipeHeading.focus({preventScroll:true})',
  "recipeHeading.scrollIntoView({behavior:matchMedia('(prefers-reduced-motion: reduce)').matches?'auto':'smooth',block:'start'})",
  "document.addEventListener('zanik:responsive',renderPlayerRecipes)",
  '-webkit-text-size-adjust:100%',
  ':root[data-responsive-mode^="compact"] .storage-actions .item-controls{grid-column:1/-1;display:grid;grid-template-columns:repeat(3,minmax(0,1fr))',
  ':root[data-responsive-mode="compact-landscape"] .storage-mini-map{width:9rem;height:8rem}',
  ':root[data-responsive-mode="compact-landscape"] .player-layout{display:block}',
  '.storage-mini-map{touch-action:pan-y;cursor:default}',
  "function mobilePlayerUi(){return Boolean(window.ZanikResponsive?.compact||matchMedia('(max-width:760px),(max-height:500px) and (pointer:coarse)').matches)}",
  "if(event.target.closest('button')||mobilePlayerUi())return",
  "if(mobilePlayerUi()||!pointers.has(event.pointerId))return",
  'function bindStorageLongPress(target,open)',
  '-webkit-touch-callout:none',
  "target.addEventListener('selectstart',event=>event.preventDefault())",
  "target.addEventListener('dragstart',event=>event.preventDefault())",
  "event.pointerType!=='touch'||!mobilePlayerUi()",
  'Math.hypot(event.clientX-startX,event.clientY-startY)>5',
  'Long-press a slot for actions',
  "dragToggle.textContent=dragEnabled?'Disable drag':'Enable drag'",
  "if(event.pointerType==='touch'&&mobileStorageDragUi()&&storageTouchDragChestId!==chest.id)return",
  "if(view!==activePlayerView)hideStorageSelectionTip()",
  "if(view!==activePlayerView&&storageTouchDragChestId!==null)disableStorageTouchDrag()",
  "window.addEventListener('pagehide',disableStorageTouchDrag)",
  "document.addEventListener('visibilitychange',()=>{if(document.hidden)disableStorageTouchDrag()})",
  "document.addEventListener('zanik:responsive',()=>{if(!mobileStorageDragUi())disableStorageTouchDrag();else{syncStorageTouchDragUi();updateStorageSelectionTip()}})",
  'function storageSelectionTipContent(entry)',
  'function showMobileItemSelectionTip(entry)',
  "(activePlayerView!=='storage'&&activePlayerView!=='inventory')",
  "panel?.querySelectorAll('.storage-grid')",
  'grid._selectionTipEntries=container.slots||[]',
  'function clearAllStorageSelections()',
  "event.target.closest('.storage-actions,.add-item-toggle,.add-panel,.storage-add-dialog')",
  "button.addEventListener('click',()=>{clearAllStorageSelections();showMobileItemSelectionTip(entry)})",
  "if(mobileStorageDragUi())return",
  'hideStorageSelectionTip();closeStorageContext()',
  'updateMoveQueueStatus(chest.id);updateStorageSelectionTip()',
  'function extendStorageSelectionBackward(slots,clickedIndex,selected)',
  "if((event.ctrlKey||event.metaKey)&&event.shiftKey){extendStorageSelectionBackward(chest.slots||[],index,selected)}",
  "preserve=event.ctrlKey||event.metaKey||card.classList.contains('storage-multi-select')",
  'const storageMouseDragArmMs=150',
  'function storageMouseDragBlocked(drag,event,readyAt,now=performance.now())',
  'function bindInventorySlotDrag(slot,entry,index,container,selected,update)',
  "slot._storageDragReadyAt=performance.now()+storageMouseDragArmMs",
  "storageMouseDragBlocked(storageDrag,event,Number(slot._storageDragReadyAt)||0)",
  'bindStorageLongPress(slot,event=>showStorageContext(event,chest,entry,allChests))',
  'bindStorageLongPress(slot,event=>showEmptyStorageContext(event,chest))',
  'bindStorageLongPress(slot,event=>showInventoryContext(event,container,entry))',
  'if(dragAvailable)bindInventorySlotDrag(slot,entry,index,container,selected,update)',
  "dragToggle.onclick=()=>{setStorageTouchDrag(container.id,storageTouchDragChestId!==container.id);update()}",
  "targetGrid?.dataset.chest===container.id",
  "if(!inventoryMutationAvailable&&['0000000000000001','0000000000000002','0000000000000003','0000000000000004','0000000000000005'].includes(storageTouchDragChestId))disableStorageTouchDrag()",
  "withdraw.textContent='Withdraw…'",
  "withdraw.onclick=()=>showInventoryWithdrawDialog",
  'function dismissStorageContextOnly(event)',
  "document.addEventListener('pointerdown',event=>{dismissStorageContextOnly(event);if(event.target.closest('.storage-actions,.add-item-toggle,.add-panel,.storage-add-dialog'))clearAllStorageSelections()},true)",
  "document.addEventListener('click',event=>{if(Date.now()<storageContextDismissUntil)",
  'html.storage-dialog-open { overflow:hidden; overscroll-behavior:none; }',
  'body.storage-dialog-open { position:fixed;',
  '.storage-delete-dialog :is(.storage-destination-list,.catalog-results,.storage-take-list,ul) { overscroll-behavior:contain; }',
  'function syncStorageDialogLock()',
  "document.body.querySelector(':scope > .storage-delete-backdrop')",
  "window.scrollTo(0,storageDialogScrollY)",
  "container.id==='0000000000000001'&&entries[0].held",
  "entry?.held?' held-item':''",
]) {
  if (!playerPage[1].includes(token)) throw new Error(`missing mobile player UI contract: ${token}`);
}

const inventoryMenu = playerPage[1].slice(
  playerPage[1].indexOf('function showInventoryContext('),
  playerPage[1].indexOf('function renderInventoryGrid('),
);
for (const [before, after] of [
  ["take.textContent='Take…'", "add.textContent='Add…'"],
  ["add.textContent='Add…'", "trigger.textContent='Move…'"],
  ["trigger.textContent='Move…'", "withdraw.textContent='Withdraw…'"],
  ["withdraw.textContent='Withdraw…'", "equip.textContent='Equip'"],
]) {
  if (inventoryMenu.indexOf(before) < 0 || inventoryMenu.indexOf(before) >= inventoryMenu.indexOf(after)) {
    throw new Error(`mobile inventory menu order is wrong: ${before} before ${after}`);
  }
}

const storageTipSource = playerPage[1].match(/function storageTooltipText\([\s\S]*?\n    function hideStorageSelectionTip/);
if (!storageTipSource) throw new Error('storage selection-tip content helper not found');
const storageTipContext = { Number, String };
vm.createContext(storageTipContext);
vm.runInContext(storageTipSource[0].replace(/\n    function hideStorageSelectionTip$/, '') + ';this.content=storageSelectionTipContent', storageTipContext);
let tip = storageTipContext.content({name: 'Oak Logs'});
if (tip.title !== 'Oak Logs' || tip.detail !== '') throw new Error('ordinary selected item did not produce a name-only mobile detail');
tip = storageTipContext.content({name: 'Oak Logs', autofillSnapshotState: 'matched', autofillSnapshotQuantity: 50, autofillSnapshotName: 'Oak Logs'});
if (tip.title !== 'Oak Logs' || tip.detail !== 'Autofill snapshot — restores this slot to 50 × Oak Logs.') {
  throw new Error('matched autofill snapshot detail was not preserved in the mobile panel');
}
tip = storageTipContext.content({name: 'Ash Planks', autofillSnapshotState: 'foreign', autofillSnapshotQuantity: 25, autofillSnapshotName: 'Oak Logs'});
if (!tip.detail.includes('autofill protects this item') || !tip.detail.includes('Snapshot expects 25 × Oak Logs here.')) {
  throw new Error('foreign autofill snapshot detail was not preserved in the mobile panel');
}

if ((playerPage[1].match(/mobilePlayerUi\(\)\|\|!pointers\.has\(event\.pointerId\)/g) || []).length !== 2) {
  throw new Error('both player-page interactive map implementations must relinquish touch panning');
}

const longPressSource = playerPage[1].match(/function bindStorageLongPress\([\s\S]*?\n    async function updateStorageAutofill/);
if (!longPressSource) throw new Error('storage long-press helper not found');
let pendingTimer = null;
const longPressContext = {
  mobilePlayerUi: () => true,
  storageDrag: null,
  storageSuppressClick: false,
  Date,
  Math,
  Number,
  setTimeout: callback => { pendingTimer = callback; return 1; },
  clearTimeout: () => { pendingTimer = null; },
};
vm.createContext(longPressContext);
vm.runInContext(longPressSource[0].replace(/\n    async function updateStorageAutofill$/, '') + ';this.bind=bindStorageLongPress', longPressContext);
const makeLongPressTarget = () => {
  const listeners = {};
  const classes = new Set();
  return {
    listeners,
    classes,
    target: {
      addEventListener: (name, listener) => { listeners[name] = listener; },
      classList: { add: name => classes.add(name), remove: name => classes.delete(name) },
    },
  };
};
let opened = 0;
let fixture = makeLongPressTarget();
longPressContext.bind(fixture.target, () => { opened += 1; });
fixture.listeners.pointerdown({ pointerType: 'touch', pointerId: 4, clientX: 20, clientY: 30 });
if (!pendingTimer || !fixture.classes.has('long-pressing')) throw new Error('touch hold did not arm');
pendingTimer();
if (opened !== 1 || fixture.classes.has('long-pressing') || !longPressContext.storageSuppressClick) {
  throw new Error('completed touch hold did not open and suppress the synthetic click');
}
pendingTimer = null;
fixture = makeLongPressTarget();
longPressContext.bind(fixture.target, () => { opened += 1; });
fixture.listeners.pointerdown({ pointerType: 'touch', pointerId: 5, clientX: 10, clientY: 10 });
fixture.listeners.pointermove({ pointerId: 5, clientX: 18, clientY: 10 });
if (pendingTimer || fixture.classes.has('long-pressing')) throw new Error('moving touch did not cancel long-press');

const modalLockSource = playerPage[1].match(/let storageDialogScrollY=[\s\S]*?new MutationObserver\(syncStorageDialogLock\)\.observe\(document\.body,\{childList:true\}\);/);
if (!modalLockSource) throw new Error('shared storage-dialog scroll lock not found');
const htmlClasses = new Set(), bodyClasses = new Set();
let modalOpen = true, restoredScroll = null;
const modalLockContext = {
  window: { scrollY: 321, scrollTo: (x, y) => { restoredScroll = [x, y]; } },
  document: {
    documentElement: { classList: { add: name => htmlClasses.add(name), remove: name => htmlClasses.delete(name) } },
    body: {
      querySelector: () => modalOpen ? {} : null,
      classList: { add: name => bodyClasses.add(name), remove: name => bodyClasses.delete(name) },
      style: { top: '' },
    },
  },
  MutationObserver: class { constructor(callback) { this.callback = callback; } observe() {} },
};
vm.createContext(modalLockContext);
vm.runInContext(modalLockSource[0] + ';this.sync=syncStorageDialogLock', modalLockContext);
modalLockContext.sync();
if (!htmlClasses.has('storage-dialog-open') || !bodyClasses.has('storage-dialog-open') || modalLockContext.document.body.style.top !== '-321px') {
  throw new Error('opening a full-window modal did not lock the background at its current scroll position');
}
modalOpen = false;
modalLockContext.sync();
if (htmlClasses.size || bodyClasses.size || modalLockContext.document.body.style.top !== '' || String(restoredScroll) !== '0,321') {
  throw new Error('closing the last full-window modal did not restore the background scroll position');
}

const dismissalSource = playerPage[1].match(/function closeStorageContext\([\s\S]*?\n    async function updateStorageAutofill/);
if (!dismissalSource) throw new Error('storage context dismissal helper not found');
let removed = 0;
const outside = {};
const menu = { contains: target => target !== outside, remove: () => { removed += 1; } };
const dismissalContext = {
  mobilePlayerUi: () => true,
  storageContextMenu: menu,
  storageContextDismissUntil: 0,
  Date,
  window: { scrollY: 0, scrollTo: () => {} },
  document: { body: { querySelector: () => null, classList: {add() {}, remove() {}}, style: {} }, documentElement: {classList: {add() {}, remove() {}}} },
  MutationObserver: class { observe() {} },
};
vm.createContext(dismissalContext);
vm.runInContext(dismissalSource[0].replace(/\n    async function updateStorageAutofill$/, '') + ';this.dismiss=dismissStorageContextOnly', dismissalContext);
let prevented = 0;
let stopped = 0;
if (!dismissalContext.dismiss({target: outside, preventDefault: () => { prevented += 1; }, stopImmediatePropagation: () => { stopped += 1; }})) {
  throw new Error('outside mobile tap did not dismiss the storage menu');
}
if (removed !== 1 || prevented !== 1 || stopped !== 1 || dismissalContext.storageContextMenu !== null || dismissalContext.storageContextDismissUntil <= Date.now()) {
  throw new Error('outside mobile tap was not exclusively consumed by menu dismissal');
}

let desktopRemoved = 0;
const desktopMenu = { contains: target => target !== outside, remove: () => { desktopRemoved += 1; } };
const desktopDismissalContext = {
  mobilePlayerUi: () => false,
  storageContextMenu: desktopMenu,
  storageContextDismissUntil: 0,
  Date,
  window: { scrollY: 0, scrollTo: () => {} },
  document: { body: { querySelector: () => null, classList: {add() {}, remove() {}}, style: {} }, documentElement: {classList: {add() {}, remove() {}}} },
  MutationObserver: class { observe() {} },
};
vm.createContext(desktopDismissalContext);
vm.runInContext(dismissalSource[0].replace(/\n    async function updateStorageAutofill$/, '') + ';this.dismiss=dismissStorageContextOnly', desktopDismissalContext);
prevented = 0;
stopped = 0;
if (!desktopDismissalContext.dismiss({target: outside, preventDefault: () => { prevented += 1; }, stopImmediatePropagation: () => { stopped += 1; }})) {
  throw new Error('outside desktop pointer did not dismiss the storage menu');
}
if (desktopRemoved !== 1 || prevented !== 0 || stopped !== 0 || desktopDismissalContext.storageContextMenu !== null || desktopDismissalContext.storageContextDismissUntil !== 0) {
  throw new Error('desktop click-away dismissal incorrectly consumed the underlying pointer action');
}

const dragDelaySource = playerPage[1].match(/function storageMouseDragBlocked\([\s\S]*?\n    function syncStorageTouchDragUi/);
if (!dragDelaySource) throw new Error('mouse drag-delay helper not found');
const dragDelayContext = { Math, performance: { now: () => 0 } };
vm.createContext(dragDelayContext);
vm.runInContext(dragDelaySource[0].replace(/\n    function syncStorageTouchDragUi$/, '') + ';this.blocked=storageMouseDragBlocked', dragDelayContext);
const dragFixture = { active: false, startX: 10, startY: 10 };
if (!dragDelayContext.blocked(dragFixture, {clientX: 17, clientY: 10}, 150, 149)) {
  throw new Error('mouse drag activated before the 150ms arm delay');
}
if (dragDelayContext.blocked(dragFixture, {clientX: 17, clientY: 10}, 150, 150)) {
  throw new Error('mouse drag remained blocked after the 150ms arm delay');
}
if (dragDelayContext.blocked(dragFixture, {clientX: 15, clientY: 10}, 150, 100)) {
  throw new Error('sub-threshold mouse motion was treated as a pending drag');
}
if (dragDelayContext.blocked({...dragFixture, active: true}, {clientX: 17, clientY: 10}, 150, 100)) {
  throw new Error('an active mouse drag was blocked by the arm delay');
}

const rangeSource = playerPage[1].match(/function extendStorageSelectionBackward\([\s\S]*?\n    function clearStorageSelection/);
if (!rangeSource) throw new Error('storage range-selection helper not found');
const rangeContext = {};
vm.createContext(rangeContext);
vm.runInContext(rangeSource[0].replace(/\n    function clearStorageSelection$/, '') + ';this.extend=extendStorageSelectionBackward', rangeContext);
const rangeSlots = [
  {id: 'a'}, {id: 'b'}, null, {id: 'c'}, {id: 'd'}, null, {id: 'e'}, {id: 'f'},
];
let rangeSelected = new Set(['b']);
if (!rangeContext.extend(rangeSlots, 6, rangeSelected) || [...rangeSelected].join(',') !== 'b,c,d,e') {
  throw new Error('Ctrl+Shift did not add the occupied row-major range from the nearest prior selection');
}
rangeSelected = new Set(['a', 'd']);
if (!rangeContext.extend(rangeSlots, 7, rangeSelected) || [...rangeSelected].join(',') !== 'a,d,e,f') {
  throw new Error('Ctrl+Shift did not preserve earlier selections while extending from the nearest prior selection');
}
rangeSelected = new Set();
if (rangeContext.extend(rangeSlots, 7, rangeSelected) || rangeSelected.size !== 0) {
  throw new Error('Ctrl+Shift selected a range without an earlier selected anchor');
}
rangeSelected = new Set(['e']);
if (rangeContext.extend(rangeSlots, 4, rangeSelected) || [...rangeSelected].join(',') !== 'e') {
  throw new Error('Ctrl+Shift searched forward instead of backward for its anchor');
}

for (const token of [
  '.player{grid-template-columns:minmax(0,1fr)',
  '.player-presence{display:flex;align-items:center;justify-content:flex-start',
  '.player-time{max-width:none;line-height:1.3}',
]) {
  if (!playersPage[1].includes(token)) throw new Error(`missing known-player mobile reflow: ${token}`);
}

const pageWindowSource = playerPage[1].match(/function recipePageWindow\([\s\S]*?\n    function renderPlayerRecipes/);
if (!pageWindowSource) throw new Error('recipe page-window helper not found');
const context = { recipeMobilePageSize: 100 };
vm.createContext(context);
vm.runInContext(pageWindowSource[0].replace(/\n    function renderPlayerRecipes$/, '') + ';this.pageWindow=recipePageWindow', context);

const cases = [
  [0, true, 0, { pageCount: 1, index: 0, start: 0, end: 0 }],
  [100, true, 0, { pageCount: 1, index: 0, start: 0, end: 100 }],
  [101, true, 0, { pageCount: 2, index: 0, start: 0, end: 100 }],
  [101, true, 1, { pageCount: 2, index: 1, start: 100, end: 101 }],
  [250, true, 99, { pageCount: 3, index: 2, start: 200, end: 250 }],
  [250, true, -4, { pageCount: 3, index: 0, start: 0, end: 100 }],
  [250, false, 2, { pageCount: 3, index: 2, start: 0, end: 250 }],
];
for (const [total, paginated, page, expected] of cases) {
  const actual = context.pageWindow(total, paginated, page);
  if (JSON.stringify(actual) !== JSON.stringify(expected)) {
    throw new Error(`bad recipe page window for ${total}/${paginated}/${page}: ${JSON.stringify(actual)}`);
  }
}

// Eight fractional tracks plus seven gaps always consume exactly the grid's
// inner width. Check the narrow viewports we support after player/card/wrapper
// padding, borders, and the largest responsive gap are accounted for.
for (const viewport of [320, 375, 390, 430]) {
  const gridWidth = viewport - 2 * 10.4 - 2 * 11.2 - 2 * 7.2 - 2;
  const gap = Math.min(4.48, viewport * 0.01);
  const slot = (gridWidth - 7 * gap) / 8;
  if (!(slot > 0) || 8 * slot + 7 * gap > gridWidth + 1e-9) {
    throw new Error(`inventory grid overflows at ${viewport}px`);
  }
}

console.log('player mobile layouts and recipe pagination: ok');
