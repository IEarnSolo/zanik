#!/usr/bin/env node
'use strict';

const crypto = require('crypto');
const fs = require('fs');
const path = require('path');

const source = process.argv[2];
const output = process.argv[3];
const blueprintRoot = process.argv[4];
const itemRoot = process.argv[5];
const privateOutput = process.argv[6];
if (!source || !output) {
  console.error('usage: node tools/generate_resource_catalog.js EXTRACTED_CELLS OUTPUT_JSON');
  process.exit(2);
}

const friendly = {
  BP_Spawner_AnimaInfusedBark: 'Anima-Infused Bark',
  BP_Spawner_BittercapMushroom: 'Bittercap Mushroom',
  BP_Spawner_HollowBark: 'Hollow Bark',
  BP_Spawner_OakBranch: 'Oak Branch',
  BP_Spawner_SwampWeedPlant: 'Swamp Weed',
  BP_Spawner_Swamp_Tar: 'Swamp Tar',
  BP_RedberryBush: 'Redberries',
  BP_CadavaBush: 'Cadavaberries',
  BP_DwellberryBush: 'Dwellberries',
  BP_ThornyRoot_Irit: 'Irit',
  BP_Plant_Explosive_CorpseCotton: 'Corpse Cotton',
  BP_Weed_Desert: 'Desert Weed',
  BP_Cactus_Barrel: 'Barrel Cactus',
  BP_Cactus_Bunny: 'Bunny Cactus',
  BP_Cactus_Pipe: 'Fibrous Pipe Cactus'
};

// These resources live outside the GatherableResources and
// HarvestableResources directories. Keep their public labels tied to the
// collectible item while accepting the world actor name in map search.
const actionableMetadata = {
  BP_ThornyRoot_Irit: {
    name: 'Irit', aliases: ['Thorny Irit Root', 'Irit Root'],
    iconAsset: '/Game/Art/UI/Icons/Resources_ConceptArt/T_Icon_Irit_Leaf.T_Icon_Irit_Leaf'
  },
  BP_Plant_Explosive_CorpseCotton: {
    name: 'Corpse Cotton', aliases: ['Corpse Cotton Plant'],
    iconAsset: '/Game/Art/UI/Icons/Resources_ConceptArt/T_Icon_Corpse_Cotton.T_Icon_Corpse_Cotton',
    // BP_Plant_Explosive_CorpseCotton explicitly selects this row. The
    // other actionable-resource blueprints leave their row handle empty and
    // therefore follow the native 24-hour runtime fallback below.
    respawnProfile: 'FLORA_BERRY_72H'
  },
  BP_Weed_Desert: {
    name: 'Desert Weed', aliases: ['Desert Weed Plant'],
    iconAsset: '/Game/Art/UI/Icons/Icons_0_12_UmS/Icons/T_Icon_Desert_Weed.T_Icon_Desert_Weed'
  },
  BP_Cactus_Barrel: {
    name: 'Barrel Cactus', aliases: ['Cactus Steak'],
    iconAsset: '/Game/Art/UI/Icons/Icons_0_12_UmS/Icons/T_Icon_Cactus_Steak_Raw.T_Icon_Cactus_Steak_Raw'
  },
  BP_Cactus_Bunny: {
    name: 'Bunny Cactus', aliases: ['Cactus Steak'],
    iconAsset: '/Game/Art/UI/Icons/Icons_0_12_UmS/Icons/T_Icon_Cactus_Steak_Raw.T_Icon_Cactus_Steak_Raw'
  },
  BP_Cactus_Pipe: {
    name: 'Fibrous Pipe Cactus', aliases: ['Pipe Cactus', 'Cactus Fibres'],
    iconAsset: '/Game/Art/UI/Icons/Icons_0_12_UmS/Icons/T_Icon_Fibrous_Cactus.T_Icon_Fibrous_Cactus'
  }
};

function displayName(raw) {
  if (friendly[raw]) return friendly[raw];
  let value = raw.replace(/^BP_Spawner_/, '').replace(/^BP_/, '')
    .replace(/_0[1-9]$/, '').replace(/_/g, ' ')
    .replace(/([a-z])([A-Z])/g, '$1 $2');
  return value.replace(/\b\w/g, c => c.toUpperCase());
}

function typeId(raw) {
  return raw.replace(/^BP_Spawner_/, '').replace(/^BP_/, '')
    .replace(/_0[1-9]$/, '').replace(/([a-z])([A-Z])/g, '$1-$2')
    .replace(/_/g, '-').toLowerCase();
}

function slug(value) {
  return value.normalize('NFKD').replace(/[^A-Za-z0-9]+/g, '-').replace(/^-|-$/g, '').toLowerCase();
}

function respawnHours(profile) {
  const match = /_(\d+(?:\.\d+)?)H$/.exec(String(profile || ''));
  return match ? Number(match[1]) : null;
}

// UResourceRespawnComponent::GetRespawnIntervalInGameTimeAdjustedForPlayerAmount
// asks the in-game time provider for a default when its row handle is empty.
// The matched build's runtime returns exactly 24 in-game hours for that path.
const runtimeDefaultRespawnProfile = 'RUNTIME_DEFAULT_24H';

function files(root, out = []) {
  for (const entry of fs.readdirSync(root, {withFileTypes: true})) {
    const full = path.join(root, entry.name);
    if (entry.isDirectory()) files(full, out);
    else if (entry.name.endsWith('.json') && entry.name !== '.uas-manifest.json') out.push(full);
  }
  return out;
}

const metadata = new Map();
for (const [rawType, value] of Object.entries(actionableMetadata)) metadata.set(rawType, value);
if (blueprintRoot && itemRoot) {
  const itemFiles = new Map();
  for (const file of files(itemRoot)) itemFiles.set(path.basename(file, '.json'), file);
  const blueprintFiles = files(blueprintRoot);
  const packages = new Map();
  const parsed = new Map();
  for (const file of blueprintFiles) {
    let exports; try { exports = JSON.parse(fs.readFileSync(file, 'utf8')); } catch (_) { continue; }
    parsed.set(file, exports);
    const packageName = exports.find(x => typeof x?.Package === 'string')?.Package;
    if (packageName) packages.set(packageName, exports);
  }
  function inheritedRespawnProfile(component, seen = new Set()) {
    const direct = component?.Properties?.SpawnSettings?.ResourceRespawnSettingsRowHandle?.RowName || '';
    if (direct) return direct;
    const templatePath = component?.Template?.ObjectPath || '';
    if (!templatePath || seen.has(templatePath)) return '';
    seen.add(templatePath);
    const match = /^(.*)\.(\d+)$/.exec(templatePath);
    if (!match) return '';
    const exports = packages.get(match[1]);
    const parent = exports?.[Number(match[2])] || exports?.find(x => x?.ObjectPath === templatePath);
    return inheritedRespawnProfile(parent, seen);
  }
  for (const file of blueprintFiles) {
    const exports = parsed.get(file);
    if (!exports) continue;
    const cdo = exports.find(x => String(x?.Name || '').startsWith('Default__BP_'));
    const rawType = String(cdo?.Name || '').replace(/^Default__/, '').replace(/_C$/, '');
    const itemPath = cdo?.Properties?.ItemData?.ObjectPath || '';
    const itemBase = (/\/([^/.]+)\.\d+$/.exec(itemPath) || [])[1];
    const itemFile = itemFiles.get(itemBase);
    let item;
    if (itemFile) {
      try { item = JSON.parse(fs.readFileSync(itemFile, 'utf8'))[0]; } catch (_) {}
    }
    const respawn = exports.find(x => x?.Type === 'ResourceRespawnComponent' && x?.Name === 'RespawnComponent');
    if (rawType) metadata.set(rawType, {
      name: item?.Properties?.Name?.LocalizedString || item?.Properties?.Name?.SourceString || displayName(rawType),
      iconAsset: item?.Properties?.Icon?.AssetPathName || '',
      respawnProfile: inheritedRespawnProfile(respawn),
      itemKey: itemBase || '',
      aliases: actionableMetadata[rawType]?.aliases || []
    });
  }
}

const nodes = [];
const privateNodes = [];
const types = new Map();
let rejected = 0;
for (const filename of files(source)) {
  let exports;
  try { exports = JSON.parse(fs.readFileSync(filename, 'utf8')); }
  catch (_) { ++rejected; continue; }
  function objectAt(ref) {
    const index = Number((/\.(\d+)$/.exec(ref?.ObjectPath || '') || [])[1]);
    return Number.isInteger(index) ? exports[index] : null;
  }
  function componentWorldTranslation(component, depth = 0) {
    if (!component || depth > 12) return null;
    const local = component?.Properties?.RelativeLocation || {X: 0, Y: 0, Z: 0};
    if (![local.X, local.Y, local.Z].every(Number.isFinite)) return null;
    const parent = objectAt(component?.Properties?.AttachParent);
    if (!parent) return local;
    const base = componentWorldTranslation(parent, depth + 1);
    if (!base) return null;
    return {X: base.X + local.X, Y: base.Y + local.Y, Z: base.Z + local.Z};
  }
  function addNode(rawType, stableKey, stateKey, worldX, worldY, worldZ, sourceFile) {
    const meta = metadata.get(rawType) || {};
    const id = meta.name ? slug(meta.name) : typeId(rawType);
    const mapX = (worldX - (-11075.0)) / 420000.0;
    // The web map and native ProjectWorldToMap both use Unreal Y directly.
    const mapY = (worldY - (-117685.0)) / 420000.0;
    if (!(mapX >= 0 && mapX <= 1 && mapY >= 0 && mapY <= 1)) { ++rejected; return; }
    types.set(id, {id, name: meta.name || displayName(rawType), icon: `/assets/resources/${id}.png`, aliases: meta.aliases || []});
    const opaque = crypto.createHash('sha256').update(stableKey).digest('hex').slice(0, 20);
    const effectiveProfile = meta.respawnProfile || runtimeDefaultRespawnProfile;
    const hours = respawnHours(effectiveProfile);
    const node = {
      id: opaque, type: id, x: +mapX.toFixed(8), y: +mapY.toFixed(8),
      worldX: +worldX.toFixed(3), worldY: +worldY.toFixed(3),
      worldZ: Number.isFinite(worldZ) ? +worldZ.toFixed(3) : null
    };
    if (hours !== null) node.respawnGameHours = hours;
    if (!meta.respawnProfile) node.respawnSource = 'runtime-default';
    nodes.push(node);
    privateNodes.push({stateKey, id: opaque, type: id, respawnProfile: effectiveProfile, iconAsset: meta.iconAsset || '',
      worldX, worldY, worldZ: Number.isFinite(worldZ) ? worldZ : null, sourceFile: path.relative(source, sourceFile || filename)});
  }
  for (const object of exports) {
    const actorPath = object?.Properties?.ActorClass?.ObjectPath || '';
    const ordinary = /\/Spawners\/(GatherableResources|HarvestableResources)\//.test(actorPath);
    const actionable = /\/ActionableResources\/(BP_Weed_Desert|BP_Cactus_Barrel|BP_Cactus_Bunny|BP_Cactus_Pipe)\./.test(actorPath);
    if (!ordinary && !actionable) continue;
    const match = /\/([^/.]+)\.\d+$/.exec(actorPath);
    if (!match) { ++rejected; continue; }
    const rawType = match[1];
    const origin = componentWorldTranslation(objectAt(object?.Properties?.AttachParent));
    if (!origin || !Number.isFinite(origin.X) || !Number.isFinite(origin.Y)) { ++rejected; continue; }
    const instances = Array.isArray(object.PerInstanceSMData) ? object.PerInstanceSMData : [];
    const guid = String(object?.Properties?.Guid || object.Name || path.basename(filename));
    for (let index = 0; index < instances.length; ++index) {
      const p = instances[index]?.TransformData?.Translation;
      if (!p || !Number.isFinite(p.X) || !Number.isFinite(p.Y)) { ++rejected; continue; }
      const worldX = origin.X + p.X;
      const worldY = origin.Y + p.Y;
      const worldZ = origin.Z + (Number.isFinite(p.Z) ? p.Z : 0);
      addNode(rawType, `${guid}:${index}`, `${guid.toUpperCase()}:${index}`, worldX, worldY, worldZ, filename);
    }
  }
  for (const actor of exports) {
    const match = /^(BP_(?:Spawner_.+|.+Bush|ThornyRoot_Irit|Plant_Explosive_CorpseCotton))_C$/.exec(actor?.Type || '');
    if (!match || !(actor?.Properties?.RespawnComponent || actor?.Properties?.ResourceRespawn)) continue;
    const rawType = match[1];
    const root = objectAt(actor?.Properties?.RootComponent || actor?.Properties?.Root || actor?.Properties?.DefaultSceneRoot);
    const location = componentWorldTranslation(root);
    if (!location) { ++rejected; continue; }
    // Actionable actors persist in SPUD under their placed actor name. Their
    // ActorGuid is useful editor metadata but is not the runtime/save lookup
    // key used by these Blueprint actors.
    const guid = rawType === 'BP_ThornyRoot_Irit' || rawType === 'BP_Plant_Explosive_CorpseCotton'
      ? actor.Name : (actor?.ActorInstanceGuid?.ActorGuid || actor.Name);
    addNode(rawType, String(guid), String(actor.Name || guid), location.X, location.Y, location.Z, filename);
  }
}

nodes.sort((a, b) => a.type.localeCompare(b.type) || a.id.localeCompare(b.id));
const typeList = [...types.values()].sort((a, b) => a.name.localeCompare(b.name));
for (const type of typeList) type.count = nodes.reduce((n, node) => n + (node.type === type.id), 0);
const fingerprint = crypto.createHash('sha256').update(JSON.stringify(nodes)).digest('hex');
const catalog = {
  version: `resources-${fingerprint.slice(0, 12)}`,
  contentFingerprint: fingerprint,
  mapVersion: 'world-map-1.0.0.2',
  generatedAt: new Date().toISOString(),
  types: typeList,
  nodes,
  diagnostics: {files: files(source).length, nodes: nodes.length, types: typeList.length, rejected}
};
fs.mkdirSync(path.dirname(output), {recursive: true});
fs.writeFileSync(output, JSON.stringify(catalog));
if (privateOutput) {
  const lines = ['# stateKey\topaqueId\ttypeId\trespawnProfile\ticonAsset\tworldX\tworldY\tworldZ\tsourceFile'];
  for (const n of privateNodes) lines.push([n.stateKey,n.id,n.type,n.respawnProfile,n.iconAsset,n.worldX,n.worldY,n.worldZ??'',n.sourceFile].join('\t'));
  fs.mkdirSync(path.dirname(privateOutput), {recursive:true});
  fs.writeFileSync(privateOutput, lines.join('\n') + '\n');
}
console.log(JSON.stringify(catalog.diagnostics));
