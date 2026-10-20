'use strict';
const fs=require('fs');
const source=fs.readFileSync(require('path').join(__dirname,'..','src','web_page.h'),'utf8');
const page=source.match(/constexpr const char\* kMapPage = R"DWHTML\(([\s\S]*?)\)DWHTML";/);
if(!page)throw new Error('map page not found');
const scripts=[...page[1].matchAll(/<script>([\s\S]*?)<\/script>/g)].map(match=>match[1]);
if(!scripts.length)throw new Error('map scripts not found');
for(const script of scripts)new Function(script);
for(const token of ['id="questPanel"','id="questSteps"','id="questTip"','/api/quests/steps','renderQuestMarkers','focusQuestMarker','dragonwilds.map.questSteps']){
  if(!page[1].includes(token))throw new Error('missing quest-map token: '+token);
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
