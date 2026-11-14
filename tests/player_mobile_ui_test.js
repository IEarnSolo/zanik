const fs = require('fs');
const path = require('path');
const vm = require('vm');

const source = fs.readFileSync(path.join(__dirname, '..', 'src', 'web_page.h'), 'utf8');
const playerPage = source.match(/constexpr const char\* kPlayerPage = R"DWHTML\(([\s\S]*?)\)DWHTML";/);
const playersPage = source.match(/constexpr const char\* kPlayersPage = R"DWHTML\(([\s\S]*?)\)DWHTML";/);
if (!playerPage || !playersPage) throw new Error('player pages not found');

for (const token of [
  '@media(max-width:640px)',
  '.storage-grid{grid-template-columns:repeat(8,minmax(0,1fr));width:100%',
  '.grave-grid{grid-template-columns:repeat(8,minmax(0,1fr));width:100%',
  '.storage-slot{box-sizing:border-box;width:100%;height:auto;min-width:0;aspect-ratio:1',
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
  'bindStorageLongPress(slot,event=>showStorageContext(event,chest,entry,allChests))',
  'bindStorageLongPress(slot,event=>showEmptyStorageContext(event,chest))',
  'function dismissStorageContextOnly(event)',
  "document.addEventListener('pointerdown',event=>{dismissStorageContextOnly(event)},true)",
  "document.addEventListener('click',event=>{if(Date.now()<storageContextDismissUntil)",
]) {
  if (!playerPage[1].includes(token)) throw new Error(`missing mobile player UI contract: ${token}`);
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
