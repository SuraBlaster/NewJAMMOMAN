const fs = require('fs'), path = require('path'), assert = require('assert');
const {spawnSync} = require('child_process');
process.chdir(path.resolve(__dirname, '..'));
const configuration = process.argv[2] || 'Release';
const converter = path.resolve(`bin/x64/${configuration}/StageConverter.exe`);
fs.mkdirSync('obj/GatePlacementTests', {recursive:true});
function convert(input, output) {
  const result = spawnSync(converter, ['--input', input, '--output', output, '--pixels-per-unit', '32']);
  assert.ifError(result.error);
  assert.equal(result.status, 0, String(result.stderr));
  return JSON.parse(fs.readFileSync(output, 'utf8'));
}
const sample = convert('Tools/StageConverter/Sample/GatePlacementSample.tmj', 'obj/GatePlacementTests/sample.stage.json');
assert.deepStrictEqual(sample.objects.map(o=>o.typeId), [301,302,301,302,301,302]);
assert.deepStrictEqual(sample.objects[0].position, {x:2,y:0});
assert.deepStrictEqual(sample.objects[5].position, {x:6,y:-2});
assert(sample.objects.every(o=>o.scale.x===1 && o.scale.y===1 && o.rotationDegrees===0));
require('../Tools/export_stage_tiled.cjs').exportStage('obj/GatePlacementTests/sample.stage.json', 'obj/GatePlacementTests/exported.tmj');
const roundtrip = convert('obj/GatePlacementTests/exported.tmj', 'obj/GatePlacementTests/roundtrip.stage.json');
assert.deepStrictEqual(roundtrip.objects, sample.objects);
const mapStage = convert('Data/Stage/LaboratoryCourse.tmj', 'obj/GatePlacementTests/current.stage.json');
const gates = mapStage.objects.filter(o=>[301,302].includes(o.typeId));
assert.equal(gates.length, 6);
assert.deepStrictEqual(gates.map(o=>o.position).sort((a,b)=>a.x-b.x||a.y-b.y),
  [210,225].flatMap(x=>[34,35,36].map(y=>({x,y}))));
const liveStage = JSON.parse(fs.readFileSync('Data/Stage/ConvertedStage.stage.json', 'utf8'));
assert.deepStrictEqual(liveStage.objects.filter(o=>[301,302].includes(o.typeId))
  .sort((a,b)=>a.position.x-b.position.x||a.position.y-b.position.y),
  gates.sort((a,b)=>a.position.x-b.position.x||a.position.y-b.position.y));
assert.deepStrictEqual(liveStage.enemies, mapStage.enemies, 'Game enemy placements must match the editable Data map');
for (const folder of ['Data/Stage/Tiled', 'Tools/StageConverter/Sample']) {
  const tileset = JSON.parse(fs.readFileSync(`${folder}/GateTileset.tsj`, 'utf8'));
  assert.equal(tileset.tilewidth, 32);
  assert.equal(tileset.tileheight, 32);
  assert.deepStrictEqual(tileset.tiles.map(t=>t.properties[0].value), [301,302]);
  const png = fs.readFileSync(`${folder}/GateTileset.png`);
  assert.equal(png.readUInt32BE(16), 64);
  assert.equal(png.readUInt32BE(20), 32);
}
console.log('PASS: Tiled gate directions, unit tiles, conversion, roundtrip, corridor and boss placements.');
