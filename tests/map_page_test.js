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
