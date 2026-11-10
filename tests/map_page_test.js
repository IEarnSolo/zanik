'use strict';
const fs=require('fs');
const source=fs.readFileSync(require('path').join(__dirname,'..','src','web_page.h'),'utf8');
const sharedScripts={};
for(const name of ['scaleBootstrap','responsiveScript','shellScript']){
  const match=source.match(new RegExp('const std::string '+name+' = R"DWHTML\\(<script(?:[^>]*)>([\\s\\S]*?)<\\/script>\\)DWHTML";'));
  if(!match)throw new Error('missing shared site script: '+name);
  sharedScripts[name]=match[1];
  new Function(match[1]);
}
for(const token of ['site-scale-select','site-admin-state','zanik.uiScale','dragonwilds.map.uiScale','zanik:admin-session','zanik:ui-scale',"window.addEventListener('storage'","event.key==='zanik.uiScale'","applyScale(event.newValue??1,false)",'--app-safe-top','--app-touch-target','visualViewport',"root.dataset.responsiveMode=mode","new CustomEvent('zanik:responsive'"]){
  if(!source.includes(token))throw new Error('missing shared site-shell token: '+token);
}
for(const token of ['--site-bottom-nav-h','site-more-toggle','siteMorePanel','window.ZanikSiteShell={closeMore}',"new CustomEvent('zanik:site-surface'",'grid-template-columns:repeat(5,1fr)','bottom:var(--app-safe-bottom)','data-responsive-mode="compact-portrait"','--site-bottom-nav-h:calc(max(var(--app-touch-target),2.75rem) + var(--app-safe-bottom))','data-responsive-mode="compact-landscape"','--site-bottom-nav-h:calc(2rem + var(--app-safe-bottom))','app.webmanifest','apple-mobile-web-app-capable','app-icon-180.png']){
  if(!source.includes(token))throw new Error('missing compact site-shell token: '+token);
}
{
  const listeners={},styles=new Map(),events=[],scaleListeners={};
  const scale={value:'1',addEventListener:(name,handler)=>{scaleListeners[name]=handler}};
  const root={style:{fontSize:'100%',setProperty:(name,value)=>styles.set(name,value)}};
  const documentMock={documentElement:root,querySelector:selector=>selector==='.site-header'?{offsetHeight:56}:null,querySelectorAll:selector=>selector==='.site-scale-select'?[scale]:[],dispatchEvent:event=>events.push(event),addEventListener:(name,handler)=>{listeners['document:'+name]=handler}};
  const windowMock={ZanikUiScale:1,addEventListener:(name,handler)=>{listeners[name]=handler}};
  const storage={setItem:()=>{throw new Error('storage-event scale sync must not write the preference again')}};
  const ResizeObserverMock=class{constructor(handler){this.handler=handler}observe(){}};
  const CustomEventMock=class{constructor(type,init){this.type=type;this.detail=init.detail}};
  const fetchMock=async()=>({ok:false});
  const polling={add:()=>{}};
  new Function('window','document','localStorage','ResizeObserver','requestAnimationFrame','CustomEvent','fetch','ZanikPolling',sharedScripts.shellScript)(windowMock,documentMock,storage,ResizeObserverMock,handler=>handler(),CustomEventMock,fetchMock,polling);
  if(typeof listeners.storage!=='function')throw new Error('shared UI scale does not listen for cross-tab storage changes');
  listeners.storage({key:'zanik.uiScale',newValue:'1.4'});
  if(root.style.fontSize!=='140%'||styles.get('--ui-scale')!=='1.4'||scale.value!=='1.4'||windowMock.ZanikUiScale!==1.4)throw new Error('cross-tab UI scale did not update the shared page shell');
  if(!events.some(event=>event.type==='zanik:ui-scale'&&event.detail.scale===1.4))throw new Error('cross-tab UI scale did not notify page-specific controls');
}
const playerPage=source.match(/constexpr const char\* kPlayerPage = R"DWHTML\(([\s\S]*?)\)DWHTML";/);
if(!playerPage)throw new Error('player page not found');
const playerScripts=[...playerPage[1].matchAll(/<script>([\s\S]*?)<\/script>/g)].map(match=>match[1]);
if(!playerScripts.length)throw new Error('player scripts not found');
for(const script of playerScripts)new Function(script);
for(const token of ['class="player-sidebar"','id="manageSidebarGroup"','id="onlineDuration"','Online for ','data-player-view="overview"','data-player-view="storage"','data-player-panel="recipes"','manageSidebarGroup.hidden=!adminStorage','selectPlayerView','class="overview-grid"','id="activityList"','id="activityPrevious"','id="activityNext"',"refreshActivity('previous')","refreshActivity('next')",'&limit=10','/assets/region-discovery-icon.png','id="activeQuestContent"','renderActiveQuest','trackedAvailable','titleNode.textContent','detail.textContent','@media(max-width:1050px)','container-type:inline-size','@container(max-width:29rem)',"grid-template-areas:'heading' 'objective' 'meta' 'map'",'quest-player-marker',"fetch('/api/map/players'",'Center on player','questMap(activeQuestContent,quest,true)',"ZanikPolling.add('player-location',2000",'Teleport to quest step',"fetch('/api/admin/quest/teleport'",'capabilities?.locationTeleport','markerIndex:saved.markerIndex',"node.classList.contains('quest-card')",'objective:teleportObjective']){
  if(!playerPage[1].includes(token))throw new Error('missing focused-player token: '+token);
}
if(!playerPage[1].includes("value==='/assets/region-discovery-icon.png'?'/assets/region-discovery-icon.png?v=places-selected-gold-1'")){
  throw new Error('region discovery activity icon must use a revisioned URL and normalize persisted legacy references');
}
const playersPage=source.match(/constexpr const char\* kPlayersPage = R"DWHTML\(([\s\S]*?)\)DWHTML";/);
if(!playersPage)throw new Error('known players page not found');
for(const script of [...playersPage[1].matchAll(/<script>([\s\S]*?)<\/script>/g)].map(match=>match[1]))new Function(script);
for(const token of ['player-presence','player-online-dot','data-online-since','Online for ','Last seen ','updateOnlineDurations',"ZanikPolling.add('known-players',2000,refresh)"]){
  if(!playersPage[1].includes(token))throw new Error('missing known-players token: '+token);
}
const killsPage=source.match(/constexpr const char\* kKillsPage = R"DWHTML\(([\s\S]*?)\)DWHTML";/);
if(!killsPage)throw new Error('kills page not found');
const killsScripts=[...killsPage[1].matchAll(/<script>([\s\S]*?)<\/script>/g)].map(match=>match[1]);
if(!killsScripts.length)throw new Error('kills scripts not found');
for(const script of killsScripts)new Function(script);
for(const token of ['class="kills-sidebar"','id="combatTab"','id="skillsTab"','id="enemyList"','Overall kills','Overall skill levels','Total level','.kills-summary[hidden]{display:none}',"total.parentElement.hidden=mode==='skills'",'selectScore','/api/skill-leaderboard?skill=']){
  if(!killsPage[1].includes(token))throw new Error('missing focused-kills token: '+token);
}
const diagnosticsPage=source.match(/constexpr const char\* kDiagnosticsPage = R"DWHTML\(([\s\S]*?)\)DWHTML";/);
if(!diagnosticsPage)throw new Error('diagnostics page not found');
for(const script of [...diagnosticsPage[1].matchAll(/<script>([\s\S]*?)<\/script>/g)].map(match=>match[1]))new Function(script);
for(const token of ['id="chart"','1h',"['All',0]",'engineTickMs','zanikGameThreadMs','maximumPingMs','loadCapacity','Spike analysis','item.occurrences','data-sort="recent"','data-sort="worst"','id="insightPrevious"','id="insightNext"','insightPageSize=10',"ZanikPolling.add('diagnostics',2000,load)",'document.hidden','repeat(2,minmax(0,1fr))','insight-pagination{justify-content:space-between;flex-wrap:wrap}'])if(!diagnosticsPage[1].includes(token))throw new Error('missing diagnostics token: '+token);
const page=source.match(/constexpr const char\* kMapPage = R"DWHTML\(([\s\S]*?)\)DWHTML";/);
if(!page)throw new Error('map page not found');
const scripts=[...page[1].matchAll(/<script>([\s\S]*?)<\/script>/g)].map(match=>match[1]);
if(!scripts.length)throw new Error('map scripts not found');
for(const script of scripts)new Function(script);
for(const token of ['id="mapToolbar"','role="toolbar"','id="questPanel"','id="questSteps"','id="questTip"','/api/quests/steps','renderQuestMarkers','focusQuestMarker','dragonwilds.map.questSteps','id="finderTabs"','data-finder-tab="resources"','data-finder-tab="npcs"','data-finder-tab="quests"','role="tabpanel"','dragonwilds.map.finderTab','selectFinderTab','ArrowRight','id="questSearch"','id="questChips"','openQuestIds','details.open=openQuestIds.has','id="settingsToggle"','id="settingsPopover"','setSettingsOpen','id="clearResources"','id="clearNpcs"','id="clearResourceSearch"','id="clearNpcSearch"','id="clearQuestSearch"','bindSearchClear','trackFocusedQuestTip','activateMobileSurface','safeMapRect','focusMapPoint','focusQuestMarker(marker,true)','--site-bottom-nav-h','var(--app-touch-target)','orientation:portrait','orientation:landscape','prefers-reduced-motion','<svg viewBox="0 0 24 24"',"Number(b.type==='main')-Number(a.type==='main')","e.target.closest('#popup a,#popup button,#overlapMenu button",'.search-wrap:has(.search-clear[hidden]) .finder-search','map-detail-close','map-detail-more','appendDetailMore','closeMapDetails(true)','queueMapResize','zoomRatio=scale/oldMin','bottom:calc(var(--site-bottom-nav-h) + .5rem)','map-online-open','map-finder-open','map-panel-open','syncCompactPanels',"compactMap()?'Map finder'","compact?'Players':'Online players'",'toolbar.top>vr.top+vr.height*.5','--map-control-size:33px','--map-cycle-control-size:1.5rem','--map-cycle-control-size:1.4rem','body.map-panel-open #controls{z-index:10011}',"e.pointerType==='touch'&&e.target.closest('#questTip')",'#questTip{touch-action:pan-y','finder-tabs button{min-height:27px}','#questTip{font-size:.66rem;line-height:1.25}','#controls svg{display:block;width:1.2rem;height:1.2rem','grid-auto-rows:var(--map-control-size);row-gap:.45rem','width:var(--map-cycle-control-size);height:var(--map-cycle-control-size);min-height:0','--map-sheet-height:48dvh','--map-sheet-height:40dvh','bindResizableSheet','sheetLimits(panel)','sheet-resizing','fastDown=downward>24','#onlinePanel[open],#questPanel[open]{display:block;overflow:hidden}','top:3.1rem;bottom:0;height:auto','overflow-y:scroll;overflow-x:hidden','overscroll-behavior:contain;touch-action:pan-y','touch-action:none;user-select:none','right:calc(var(--map-control-size) + .3rem);top:0','width:min(15rem,calc(var(--app-viewport-width)']){
  if(!page[1].includes(token))throw new Error('missing quest-map token: '+token);
}
for(const token of ['--map-detail-action-size:1.55rem','map-detail-back','addQuestDetailBack','selectFinderTab(\'quests\');questPanel.open=true','questTip.classList.remove(\'has-detail-back\')'])if(!page[1].includes(token))throw new Error('missing mobile quest-detail navigation token: '+token);
for(const token of ['completedPinch=pinch','drag={id:remainingId,x:point.x,y:point.y,tx,ty,moved:true'])if(!page[1].includes(token))throw new Error('missing world-map pinch-to-pan handoff token: '+token);
for(const token of ['if(pointers.size===1){const [id,point]=pointers.entries().next().value','drag={id,x:point.x,y:point.y,cx:saved.centerX,cy:saved.centerY}'])if(!source.includes(token))throw new Error('missing embedded-map pinch-to-pan handoff token: '+token);
for(const token of ['function commitMapRender()','translate(${tx}px,${ty}px) scale(${scale})','function queueMapRender()','mapRenderFrame=requestAnimationFrame','zoomAt(Math.hypot(a.x-b.x,a.y-b.y)/pinch.start,pinch.cx,pinch.cy,true)','ty=drag.ty+dy;queueMapRender()','mx=mapViewportWidth*.5','v.classList.remove(\'dragging\');render()'])if(!page[1].includes(token))throw new Error('missing frame-throttled map rendering token: '+token);
for(const token of ['will-change:transform','backface-visibility:hidden','translate3d(${tx}px,${ty}px,0)'])if(page[1].includes(token))throw new Error('map stage still forces a 4096px compositor surface: '+token);
for(const token of ['#stage.map-compact-surface{width:1024px;height:1024px}','#stage.map-compact-surface .marker{transform:translate(-50%,-4px) scale(.25)','#stage.map-compact-surface :is(.lodestone,.nodestone,.resource,.npc,.quest-step-marker){transform:translate(-50%,-50%) scale(.25)','let mapRenderFrame=0,mapViewportWidth=v.clientWidth,mapViewportHeight=v.clientHeight,mapSurfaceSize=SIZE','function setMapSurfaceSize(next)','stage.classList.toggle(\'map-compact-surface\',compact)','setMapSurfaceSize(compactMap()?1024:SIZE)','mapViewportWidth/mapSurfaceSize','Number(x)*mapSurfaceSize*scale','Number(player.x)*mapSurfaceSize*scale'])if(!page[1].includes(token))throw new Error('missing compact 1024px map-surface token: '+token);
if(!page[1].includes('#stage.map-compact-surface .resource img,#stage.map-low-detail .lodestone'))throw new Error('compact resource icons still apply one drop-shadow filter per marker');
if(!page[1].includes('.resource img{display:block;width:12px;height:12px;object-fit:contain;image-rendering:auto'))throw new Error('resource icon source quality or interpolation contract changed');
for(const token of ['#stage.map-compact-surface .npc{--npc-state-glow:#ff5544;isolation:isolate}','npc.non-hostile{--npc-state-glow:#63ff9a}','npc.quest{--npc-state-glow:#ffe05c}','npc.inactive{--npc-state-glow:#ffd34e}','npc.ready{--npc-state-glow:#79e7b0}','npc.dead,#stage.map-compact-surface .npc.unloaded{--npc-state-glow:#fff}','background:radial-gradient(circle,transparent 28%,var(--npc-state-glow) 48%,transparent 74%)','#stage.map-compact-surface .npc img{position:relative;z-index:1;filter:none}','#stage.map-compact-surface .npc.no-icon{box-shadow:none}'])if(!page[1].includes(token))throw new Error('missing compact low-cost NPC state indicator token: '+token);
if(!page[1].includes('.npc img{display:block;width:100%;height:100%;object-fit:contain'))throw new Error('NPC icon source quality contract changed');
if(page[1].includes('width:4096px;height:4096px;zoom:.25'))throw new Error('compact marker layers still use low-quality CSS zoom');
if((page[1].match(/minScale\*\(compactMap\(\)\?18:32\)/g)||[]).length<2)throw new Error('compact manual and resize zoom are not capped at 18x');
for(const token of ['const displayed=SIZE*scale','const w=SIZE*scale,h=SIZE*scale','mapViewportWidth/SIZE,mapViewportHeight/SIZE','Number(x)*SIZE*scale','Number(player.x)*SIZE*scale'])if(page[1].includes(token))throw new Error('compact map calculations still depend on the 4096px visual surface: '+token);
for(const token of ['rel="preload" href="/assets/world-map-2048.jpg"','src="/assets/world-map-1024.jpg"','decoding="async" fetchpriority="high"','id="regionLabels" aria-hidden="true"','document.createElement(\'div\')','paint-order:stroke fill','function desiredMapResolution()','maximum=compactMap()?2048:4096','mapResolution===1024&&displayed<=1408','maximum===4096&&mapResolution===4096&&displayed>=2304','function syncMapResolution()','world.src=next===4096?\'/assets/world-map.jpg\'','function drawLabels()','stage.classList.toggle(\'map-low-detail\',next===1024)','function scheduleMapDetail(delay=240)','if(mapDetailTimer)return','if(drag||pinch||v.classList.contains(\'dragging\'))','labelsDirty=true;scheduleMapDetail()','const fallback=compactMap()?\'/assets/world-map-2048.jpg\''])if(!page[1].includes(token))throw new Error('missing mobile-safe adaptive map-resolution token: '+token);
for(const token of ['.region-label{position:absolute','-webkit-text-stroke:var(--label-stroke) var(--label-glow)','#viewport>#regionLabels{z-index:1}','if(compact&&regionLabels.parentElement!==v){stage.after(regionLabels);drawLabels()}','stage.insertBefore(regionLabels,resources)','const SIZE=4096,LABEL_RENDER_SCALE=2','function syncLabelViewport()','screenPerCanonical=mapSurfaceSize*scale/SIZE','label.x*screenPerCanonical+tx','screenPerCanonical/LABEL_RENDER_SCALE','node.className=\'region-label\'','node._mapLabel={x,y,font,stroke,rotation}','font*LABEL_RENDER_SCALE','stroke*LABEL_RENDER_SCALE','if(regionLabels.parentElement===v)syncLabelViewport()','syncLabelViewport();scheduleMapDetail()'])if(!page[1].includes(token))throw new Error('missing compact screen-space label token: '+token);
for(const unstable of ['node.style.fontSize=label.font*screenPerCanonical','node.style.setProperty(\'--label-stroke\',label.stroke*screenPerCanonical'])if(page[1].includes(unstable))throw new Error('compact labels still re-hint font geometry during zoom: '+unstable);
for(const sample of [{x:0,y:0,scale:.35,tx:-25,ty:18,vw:375,vh:667},{x:2048,y:3072,scale:1.4,tx:-640,ty:-980,vw:320,vh:568},{x:4096,y:4096,scale:4.75,tx:-1800,ty:-2500,vw:844,vh:390}]){
  const screenPerCanonical=1024*sample.scale/4096;
  const svgX=sample.x*screenPerCanonical+sample.tx,svgY=sample.y*screenPerCanonical+sample.ty;
  const markerX=sample.x*.25*sample.scale+sample.tx,markerY=sample.y*.25*sample.scale+sample.ty;
  if(Math.abs(svgX-markerX)>1e-9||Math.abs(svgY-markerY)>1e-9)throw new Error('compact screen-space labels do not preserve marker coordinates');
  const baseFont=38,baseStroke=7,renderScale=2,elementScale=screenPerCanonical/renderScale;
  if(Math.abs(baseFont*renderScale*elementScale-baseFont*screenPerCanonical)>1e-9||Math.abs(baseStroke*renderScale*elementScale-baseStroke*screenPerCanonical)>1e-9)throw new Error('fixed-resolution labels do not preserve their intended type or outline size');
}
for(const token of ['storedSelection(\'dragonwilds.map.resources\')','resourceCatalogPromise','npcCatalogPromise','questCatalogPromise','function ensureFinderTab(name)','wantResources=Boolean(resourceCatalog&&selectedResources.size)','wantLabels=!labelsLoaded'])if(!page[1].includes(token))throw new Error('missing lazy map-data token: '+token);
if(/function commitMapRender\(\)\{[^}]*syncMapResolution\(\)/.test(page[1]))throw new Error('map commit still performs resolution or canvas work during gestures');
for(const removed of ['markerPerformanceToggle','Hide all overlays (performance test)','map-image-only'])if(page[1].includes(removed))throw new Error('temporary marker-isolation control remains: '+removed);
if(page[1].includes('ty=drag.ty+dy;render()'))throw new Error('map drag still renders synchronously for every pointer event');
if(page[1].includes('#questTip[style*="display: block"]::before'))throw new Error('quest detail still renders a mobile swipe indicator');
if(!page[1].includes('box-shadow:0 -.5rem 0 .5rem #17231f,0 1px #78948733;isolation:isolate'))throw new Error('compact finder tabs lack the opaque overscan seam mask');
if(!page[1].includes('#onlinePanel[open]>summary,#questPanel[open]>summary{height:2.25rem;padding:.35rem .65rem}'))throw new Error('landscape sheet headers were not tightened after removing swipe handles');
if(!page[1].includes('#onlinePanel[open]>summary::before,#questPanel[open]>summary::before,#questTip::before'))throw new Error('landscape map modal swipe handles are not suppressed');
for(const token of ['flex-wrap:wrap','overflow-y:auto;overflow-x:hidden','zanik:admin-session','showMapStatus'])if(!page[1].includes(token))throw new Error('missing clean map-control token: '+token);
for(const removed of ['id="headerContent"','id="adminState"','resourceMenu.hidden','npcMenu.hidden','rail.scrollLeft'])if(page[1].includes(removed))throw new Error('obsolete map header/control token remains: '+removed);
if(page[1].includes('#onlinePanel[open]::before')||page[1].includes('#questPanel[open]::before'))throw new Error('sheet drag handles must be anchored inside the fixed summaries');
for(const token of ["state.state==='ready'","Respawn ready · area currently unloaded","Dead · game clock paused",'npcClockAdvancing']){
  if(!page[1].includes(token))throw new Error('missing authoritative NPC respawn token: '+token);
}
for(const token of ['id="lodestoneTip"','showLodestoneTip','Coordinates: X ','pointer-events:auto;cursor:pointer']){
  if(!page[1].includes(token))throw new Error('missing public lodestone-tooltip token: '+token);
}
if(!page[1].includes("if(adminSession?.capabilities?.teleport){const action=document.createElement('div')") ||
   !page[1].includes("action.textContent='Click to teleport'")){
  throw new Error('lodestone tooltip must advertise teleporting only to an authorized admin session');
}
for(const token of ['id="nodestoneTip"','showNodestoneTip','displayLocation','teleportToNodestone','nodestoneId:id','admin-nodestone-active',"nodestoneId:e.target.closest('.nodestone')?.dataset.id||null",'completed.nodestoneId&&adminSession?.capabilities?.nodestoneTeleport']){
  if(!page[1].includes(token))throw new Error('missing nodestone tooltip/teleport token: '+token);
}
if(page[1].includes('Placed by: '))throw new Error('lodestone tooltip must not claim unavailable historical ownership');
if(!page[1].includes("/assets/lodestone-icon.png?v=lodestone-map-icon-2")){
  throw new Error('lodestone map icon must use a revisioned URL so corrected artwork replaces cached responses');
}
if(!page[1].includes(".nodestone{position:absolute;transform:translate(-50%,-50%);width:12px;height:12px") ||
   !page[1].includes("/assets/nodestone-icon.png?v=nodestone-topdown-2")){
  throw new Error('nodestone map icon must use a revisioned URL so corrected artwork replaces cached responses');
}
