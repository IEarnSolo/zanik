#!/usr/bin/env node
'use strict';

const crypto = require('crypto');
const fs = require('fs');
const path = require('path');

const [cellsRoot, spawnRoot, characterRoot, placedRoot, publicOutput, privateOutput, exclusionsOutput] = process.argv.slice(2);
if (!cellsRoot || !spawnRoot || !characterRoot || !placedRoot || !publicOutput || !privateOutput || !exclusionsOutput) {
  console.error('usage: node tools/generate_npc_catalog.js CELLS SPAWN_BLUEPRINTS CHARACTER_BLUEPRINTS PLACED_NPC_BLUEPRINTS PUBLIC_JSON PRIVATE_TSV EXCLUSIONS_TSV');
  process.exit(2);
}

function files(root, out = []) {
  for (const entry of fs.readdirSync(root, {withFileTypes: true})) {
    const full = path.join(root, entry.name);
    if (entry.isDirectory()) files(full, out);
    else if (entry.name.endsWith('.json') && entry.name !== '.uas-manifest.json') out.push(full);
  }
  return out;
}
function read(file) { try { return JSON.parse(fs.readFileSync(file, 'utf8')); } catch (_) { return null; } }
function refIndex(ref) { const m = /\.(\d+)$/.exec(ref?.ObjectPath || ''); return m ? Number(m[1]) : -1; }
function slug(value) { return String(value).normalize('NFKD').replace(/[^A-Za-z0-9]+/g, '-').replace(/^-|-$/g, '').toLowerCase(); }
function fallbackName(raw) {
  return String(raw).replace(/_C$/, '').replace(/^BP_AI_/, '').replace(/^BP_SpawnPoint_/, '')
    .replace(/_Character/g, '').replace(/_/g, ' ').replace(/([a-z])([A-Z])/g, '$1 $2')
    .replace(/\b\w/g, c => c.toUpperCase());
}
function canonicalNpcName(aiClass, authoredName) {
  const value = String(aiClass);
  if (/DragonLesserGreen_Blightscale/i.test(value)) return 'Blightscale the Lesser Green Dragon';
  if (/DragonLesserGreen/i.test(value)) return 'Green Dragon';
  if (/DragonLesserBlue/i.test(value)) return 'Blue Dragon';
  if (/DragonLesserRed/i.test(value)) return 'Red Dragon';
  if (/DragonLesserBlack/i.test(value)) return 'Black Dragon';
  if (/MediumBeast_Spinning/i.test(value)) return 'Garou Berserker';
  if (/MagicBeast_BirdKing/i.test(value)) return 'Garou Mage (Bird King)';
  if (/LesserDemon_Character_02/i.test(value)) return 'Lesser Demon';
  if (/Melee2HBlackKnight_Flaming/i.test(value)) return 'Black Knight Crusader';
  if (/MeleeBlackKnight_Flaming/i.test(value)) return 'Blazing Black Knight';
  return authoredName;
}
function canonicalIcon(aiClass, name, journalByName) {
  const exact = journalByName.get(String(name).toLowerCase())?.iconAsset;
  if (exact) return exact;
  const value = String(aiClass);
  if (/DragonLesserGreen_Blightscale/i.test(value)) return journalByName.get('green dragon')?.iconAsset || '';
  if (/Kebbit_Character_02/i.test(value)) return journalByName.get('kebbit')?.iconAsset || '';
  if (/MagicBeast_BirdKing/i.test(value)) return journalByName.get('garou druid')?.iconAsset || '';
  if (/Melee2HBlackKnight_Flaming/i.test(value)) return journalByName.get('elite black knight')?.iconAsset || '';
  if (/MeleeBlackKnight_Flaming/i.test(value)) return journalByName.get('black knight warrior')?.iconAsset || '';
  return '';
}
function objectPathBase(ref) { return (/\/([^/.]+)\.\d+$/.exec(ref?.ObjectPath || '') || [])[1] || ''; }
function assetPathBase(ref) { return (/\/([^/.]+)\.[^/.]+$/.exec(ref?.AssetPathName || '') || [])[1] || ''; }
function durationSeconds(duration) {
  duration ||= {};
  return ((Number(duration.Days) || 0) * 86400) + ((Number(duration.Hours) || 0) * 3600) +
    ((Number(duration.Minutes) || 0) * 60) + (Number(duration.Seconds) || 0);
}
function worldLocation(exports, component, depth = 0) {
  if (!component || depth > 12) return null;
  const p = component.Properties || {};
  const local = p.RelativeLocation || {X: 0, Y: 0, Z: 0};
  if (![local.X, local.Y, local.Z].every(Number.isFinite)) return null;
  const parent = exports[refIndex(p.AttachParent)];
  if (!parent) return local;
  const base = worldLocation(exports, parent, depth + 1);
  return base && {X: base.X + local.X, Y: base.Y + local.Y, Z: base.Z + local.Z};
}

// Journal entries are the authoritative public-facing identity for AI.  The
// character classes themselves occasionally retain prototype names (notably
// "Axe Goblin" on Black Knights), so join Character -> AIDataClass -> Journal.
const journalByAiData = new Map(), journalByName = new Map();
for (const file of files(characterRoot)) {
  const normalized = file.replaceAll('\\', '/');
  if (!/\/UI\/JournalData\/Entries\/(?:World\/(?:Factions|Fauna)|Knowledge\/People)\//i.test(normalized)) continue;
  const exports = read(file); const props = exports?.[0]?.Properties || {};
  const name = props.DisplayName?.LocalizedString || props.DisplayName?.SourceString || '';
  const iconAsset = assetPathBase(props.Image);
  const aiData = assetPathBase(props.UnlockCondition?.AIData);
  if (!name || !iconAsset) continue;
  const entry = {name, iconAsset, journalSource: path.relative(characterRoot, file)};
  if (aiData) journalByAiData.set(aiData, entry);
  journalByName.set(name.toLowerCase(), entry);
}

const characterMetadata = new Map();
for (const file of files(characterRoot)) {
  const exports = read(file); if (!exports) continue;
  const klass = exports.find(x => x?.Type === 'BlueprintGeneratedClass');
  const cdo = klass && exports.find(x => x?.Name === `Default__${klass.Name}`);
  if (!klass || !cdo) continue;
  const className = String(klass.Name).replace(/_C$/, '');
  const text = cdo.Properties?.AIName;
  const aiData = assetPathBase(cdo.Properties?.AIDataClass);
  const journal = journalByAiData.get(aiData);
  const name = journal?.name || text?.LocalizedString || text?.SourceString || fallbackName(className);
  characterMetadata.set(className, {name, aiData, iconAsset: journal?.iconAsset || '', source: path.relative(characterRoot, file)});
}

const spawnRaw = new Map();
for (const file of files(spawnRoot)) {
  const exports = read(file); if (!exports) continue;
  const klass = exports.find(x => x?.Type === 'BlueprintGeneratedClass');
  const cdo = exports.find(x => x?.Name === `Default__${klass?.Name}`);
  if (!klass || !cdo) continue;
  const spawnType = String(klass.Name).replace(/_C$/, '');
  const parent = String(klass.SuperStruct?.ObjectName || klass.Super?.ObjectName || '')
    .replace(/^(?:BlueprintGeneratedClass|Class)'/, '').replace(/_C'$/, '').replace(/'$/, '');
  const aiClass = String(cdo.Properties?.AIClass?.ObjectName || '').replace(/^BlueprintGeneratedClass'/, '').replace(/_C'$/, '');
  const aiBase = aiClass || objectPathBase(cdo.Properties?.AIClass);
  spawnRaw.set(spawnType, {parent, aiClass: aiBase, properties: cdo.Properties || {},
    source: path.relative(spawnRoot, file)});
}
const spawnMetadata = new Map();
function resolveSpawn(spawnType, seen = new Set()) {
  if (spawnMetadata.has(spawnType)) return spawnMetadata.get(spawnType);
  if (seen.has(spawnType)) return null;
  seen.add(spawnType);
  const raw = spawnRaw.get(spawnType); if (!raw) return null;
  const parent = raw.parent === 'AISpawnPoint' ? {} : resolveSpawn(raw.parent, seen);
  if (raw.parent !== 'AISpawnPoint' && !parent) return null;
  const aiBase = raw.aiClass || parent.aiClass || '';
  if (!aiBase) return null;
  const character = characterMetadata.get(aiBase) || {};
  const hasDuration = Object.prototype.hasOwnProperty.call(raw.properties, 'RespawnDuration');
  const hasRespawn = Object.prototype.hasOwnProperty.call(raw.properties, 'bShouldRespawn');
  const respawnGameSeconds = hasDuration ? durationSeconds(raw.properties.RespawnDuration) : (parent.respawnGameSeconds || 0);
  const name = canonicalNpcName(aiBase, character.name || fallbackName(aiBase || spawnType));
  const lower = `${spawnType} ${aiBase} ${name}`.toLowerCase();
  const passive = /(chicken|cow|sheep|ram|deer|kebbit|chinchompa|falcon|magpie|raven|goat|vulture)/.test(lower);
  const quest = /quest|postie|wise old|npc/.test(lower);
  const shouldRespawn = hasRespawn ? raw.properties.bShouldRespawn !== false : parent.shouldRespawn !== false;
  const result = {aiClass: aiBase, name, iconAsset: character.iconAsset || canonicalIcon(aiBase, name, journalByName), category: quest ? 'quest' : passive ? 'non-hostile' : 'hostile',
    hostile: !quest && !passive, shouldRespawn,
    respawnGameSeconds: shouldRespawn ? respawnGameSeconds : 0, source: raw.source};
  spawnMetadata.set(spawnType, result);
  return result;
}
for (const name of spawnRaw.keys()) resolveSpawn(name);

// Directly placed quest/dialogue characters are not AAISpawnPoints.  Build a
// class graph and require an actual skeletal character mesh in the class or an
// ancestor.  This deliberately excludes static interactables such as post
// boxes and books even though they inherit BP_BaseInteractableNPC and expose a
// conversation participant.
const placedRaw = new Map();
for (const file of files(placedRoot)) {
  const exports = read(file); if (!exports) continue;
  const klass = exports.find(x => x?.Type === 'BlueprintGeneratedClass');
  if (!klass) continue;
  const className = String(klass.Name).replace(/_C$/, '');
  // Spawned AI classes also carry skeletal meshes and often inherit dialogue
  // helpers. They belong in the spawn-point join above, never in the directly
  // placed quest/NPC catalog.
  if (!className.startsWith('BP_NPC_')) continue;
  const parent = String(klass.Super?.ObjectName || '').replace(/^BlueprintGeneratedClass'/, '').replace(/_C'$/, '');
  const participant = exports.find(x => x?.Type === 'DomConversationParticipant' && x?.Properties?.DisplayName);
  const mesh = exports.find(x => x?.Type === 'SkeletalMeshComponent' && x?.Name === 'CharacterMesh0' &&
    (x?.Properties?.SkeletalMesh || x?.Properties?.SkinnedAsset));
  placedRaw.set(className, {parent, name: participant?.Properties?.DisplayName || '', hasMesh: !!mesh,
    source: path.relative(placedRoot, file)});
}
const placedMetadata = new Map();
function resolvePlaced(className, seen = new Set()) {
  if (placedMetadata.has(className)) return placedMetadata.get(className);
  if (seen.has(className)) return null;
  seen.add(className);
  const raw = placedRaw.get(className); if (!raw) return null;
  const parent = resolvePlaced(raw.parent, seen);
  const valid = raw.hasMesh || !!parent?.valid;
  const name = raw.name || parent?.name || fallbackName(className.replace(/^BP_NPC_/, ''));
  const journal = journalByName.get(name.toLowerCase());
  const result = {valid, aiClass: className, name, iconAsset: journal?.iconAsset || parent?.iconAsset || '',
    category: 'quest', hostile: false, respawnGameSeconds: 0, source: raw.source};
  placedMetadata.set(className, result);
  return result;
}
for (const name of placedRaw.keys()) resolvePlaced(name);

const nodes = [], identities = [], exclusions = [];
const types = new Map();
let rejected = 0, scannedFiles = 0;
for (const file of files(cellsRoot)) {
  const exports = read(file); if (!exports) { ++rejected; continue; }
  ++scannedFiles;
  const relative = path.relative(cellsRoot, file).replaceAll('\\', '/');
  const packageIsWorld = /\/Maps\/World\/L_World\//i.test('/' + relative);
  for (const actor of exports) {
    const spawnType = String(actor?.Type || '').replace(/_C$/, '');
    let meta = spawnMetadata.get(spawnType);
    const placed = placedMetadata.get(spawnType);
    if (!meta && placed?.valid) meta = placed;
    if (!meta) continue;
    let reason = '';
    if (!packageIsWorld) reason = 'non-overworld-map';
    else if (/Dungeon|Developers|Test/i.test(relative + ' ' + String(actor?.Outer?.ObjectPath || ''))) reason = 'dungeon-instance-or-test';
    const root = exports[refIndex(actor?.Properties?.RootComponent)];
    const location = worldLocation(exports, root);
    if (!location || ![location.X, location.Y, location.Z].every(Number.isFinite)) reason ||= 'invalid-transform';
    const mapX = location ? (location.X + 11075) / 420000 : NaN;
    const mapY = location ? (location.Y + 117685) / 420000 : NaN;
    if (!reason && !(mapX >= 0 && mapX <= 1 && mapY >= 0 && mapY <= 1)) reason = 'outside-playable-map';
    const stateKey = String(actor.Name || actor?.Properties?.Guid || actor?.ActorInstanceGuid?.ActorGuid || '');
    if (!stateKey) reason ||= 'missing-persistent-identity';
    if (reason) { exclusions.push({stateKey, spawnType, reason, sourceFile: relative}); continue; }
    const typeId = slug(meta.name);
    const nativeGuid = actor?.Properties?.Guid || actor?.ActorInstanceGuid?.ActorGuid || '';
    const stable = nativeGuid || stateKey;
    const id = crypto.createHash('sha256').update(`${relative}:${stable}`).digest('hex').slice(0, 20);
    // World-partition actors may override both values from the spawn-point CDO.
    // In particular, optional ambient placements commonly set bShouldRespawn=false;
    // never advertise the inherited CDO timer for those one-shot slots.
    const hasRespawnOverride = Object.prototype.hasOwnProperty.call(actor?.Properties || {}, 'bShouldRespawn');
    const shouldRespawn = hasRespawnOverride ? actor.Properties.bShouldRespawn !== false : meta.shouldRespawn !== false;
    const instanceDuration = Object.prototype.hasOwnProperty.call(actor?.Properties || {}, 'RespawnDuration')
      ? durationSeconds(actor.Properties.RespawnDuration) : meta.respawnGameSeconds;
    const placementRespawn = shouldRespawn ? instanceDuration : 0;
    const node = {id, type: typeId, x: +mapX.toFixed(8), y: +mapY.toFixed(8), worldX: +location.X.toFixed(3),
      worldY: +location.Y.toFixed(3), worldZ: +location.Z.toFixed(3)};
    if (placementRespawn > 0) node.respawnGameSeconds = placementRespawn;
    nodes.push(node);
    identities.push({stateKey, id, typeId, spawnType, aiClass: meta.aiClass, respawnGameSeconds: placementRespawn,
      worldX: location.X, worldY: location.Y, worldZ: location.Z, nativeGuid, sourceFile: relative});
    if (!types.has(typeId)) {
      const type = {id: typeId, name: meta.name, category: meta.category, hostile: meta.hostile, count: 0};
      if (meta.iconAsset) type.icon = `/assets/npcs/${meta.iconAsset}.png`;
      types.set(typeId, type);
    }
    types.get(typeId).count++;
  }
}
nodes.sort((a,b) => a.type.localeCompare(b.type) || a.id.localeCompare(b.id));
identities.sort((a,b) => a.typeId.localeCompare(b.typeId) || a.id.localeCompare(b.id));
exclusions.sort((a,b) => a.reason.localeCompare(b.reason) || a.stateKey.localeCompare(b.stateKey));
const typeList = [...types.values()].sort((a,b) => a.name.localeCompare(b.name));
const fingerprint = crypto.createHash('sha256').update(JSON.stringify({typeList,nodes})).digest('hex');
const catalog = {version:`npcs-${fingerprint.slice(0,12)}`,contentFingerprint:fingerprint,mapVersion:'world-map-1.0.0.2',
  generatedAt:new Date().toISOString(),types:typeList,nodes,
  diagnostics:{files:scannedFiles,nodes:nodes.length,types:typeList.length,rejected,excluded:exclusions.length}};
fs.mkdirSync(path.dirname(publicOutput),{recursive:true});
fs.writeFileSync(publicOutput, JSON.stringify(catalog));
const identityHeader = `# zanik-npc-identities\t2\t${identities.length}\t${fingerprint}\tstateKey\topaqueId\ttypeId\tspawnType\taiClass\trespawnGameSeconds\tworldX\tworldY\tworldZ\tnativeGuid\tsourceFile`;
fs.mkdirSync(path.dirname(privateOutput),{recursive:true});
fs.writeFileSync(privateOutput, identityHeader+'\n'+identities.map(x => [x.stateKey,x.id,x.typeId,x.spawnType,x.aiClass,x.respawnGameSeconds,x.worldX,x.worldY,x.worldZ,x.nativeGuid,x.sourceFile].join('\t')).join('\n')+'\n');
fs.writeFileSync(exclusionsOutput, '# stateKey\tspawnType\treason\tsourceFile\n'+exclusions.map(x => [x.stateKey,x.spawnType,x.reason,x.sourceFile].join('\t')).join('\n')+'\n');
console.log(JSON.stringify(catalog.diagnostics));
