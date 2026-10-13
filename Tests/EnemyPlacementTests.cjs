const fs = require('fs'), path = require('path'), assert = require('assert');
const {spawnSync} = require('child_process');
const configuration = process.argv[2] || 'Release';
process.chdir(path.resolve(__dirname, '..'));
const names = ['Wave', 'Scatter', 'Mage', 'Fan', 'Hover', 'Fly', 'Egg', 'Boss', 'Vacuum'];
for (const folder of ['Data/Stage/Tiled', 'Tools/StageConverter/Sample']) {
 const tileset = JSON.parse(fs.readFileSync(`${folder}/EnemyPlacements.tsj`, 'utf8'));
 assert.equal(tileset.objectalignment, 'bottom');
 assert.equal(tileset.tilecount, names.length);
 for (const [id, name] of names.entries()) {
  const tile = tileset.tiles.find(t => t.id === id);
  assert.equal(tile.properties.find(p => p.name === 'enemyType').value, name);
  const height = ['Mage', 'Boss'].includes(name) ? 64 : 32;
  assert.equal(tile.imagewidth, 32);
  assert.equal(tile.imageheight, height);
  const png = fs.readFileSync(path.join(folder, tile.image));
  assert.equal(png.readUInt32BE(16), 32);
  assert.equal(png.readUInt32BE(20), height);
 }
}
const sample = JSON.parse(fs.readFileSync('Tools/StageConverter/Sample/EnemyPlacementSample.tmj', 'utf8'));
for (const object of sample.layers[0].objects) {
 assert.equal(object.width, 32);
 assert.equal(object.height, ['Mage', 'Boss'].includes(object.name) ? 64 : 32);
}
fs.mkdirSync('obj/EnemyPlacementTests', {recursive: true});
const output = 'obj/EnemyPlacementTests/sample.stage.json';
const converter = path.resolve(`bin/x64/${configuration}/StageConverter.exe`);
const result = spawnSync(converter,
 ['--input', 'Tools/StageConverter/Sample/EnemyPlacementSample.tmj', '--output', output, '--pixels-per-unit', '32']);
assert.ifError(result.error);
assert.equal(result.status, 0, String(result.stderr));
const stage = JSON.parse(fs.readFileSync(output, 'utf8'));
assert.deepStrictEqual(stage.enemies.map(e => e.enemyType), names);
const previous = JSON.parse(fs.readFileSync('Tools/StageConverter/Sample/EnemyPlacementSample.stage.json', 'utf8'));
assert.deepStrictEqual(stage.enemies.slice(0, 8), previous.enemies.slice(0, 8));
assert.deepStrictEqual(stage.enemies[8], {enemyType: 'Vacuum', direction: 'Right', position: {x: 34, y: -5}});
require('../Tools/export_stage_tiled.cjs').exportStage(output, 'obj/EnemyPlacementTests/exported.tmj');
const exported = JSON.parse(fs.readFileSync('obj/EnemyPlacementTests/exported.tmj', 'utf8'));
for (const object of exported.layers.find(l => l.name === 'Enemies').objects) {
 assert.equal(object.width, 32);
 assert.equal(object.height, ['Mage', 'Boss'].includes(object.name) ? 64 : 32);
}
assert(fs.existsSync('obj/EnemyPlacementTests/Tiled/EnemyPlacementIcons/Vacuum.png'));
// Accept the class name as an alias, while keeping game data canonical.
const aliasTileset = JSON.parse(fs.readFileSync('Tools/StageConverter/Sample/EnemyPlacements.tsj', 'utf8'));
for (const tile of aliasTileset.tiles) {
 tile.image = path.resolve('Tools/StageConverter/Sample', tile.image);
 for (const property of tile.properties) if (property.value === 'Vacuum') property.value = 'EnemyVacuum';
}
const aliasMap = structuredClone(sample);
aliasMap.tilesets = [{firstgid: 1, ...aliasTileset}];
fs.writeFileSync('obj/EnemyPlacementTests/alias.tmj', JSON.stringify(aliasMap));
const aliasResult = spawnSync(converter, ['--input', 'obj/EnemyPlacementTests/alias.tmj',
 '--output', 'obj/EnemyPlacementTests/alias.stage.json', '--pixels-per-unit', '32']);
assert.ifError(aliasResult.error);
assert.equal(aliasResult.status, 0, String(aliasResult.stderr));
assert.deepStrictEqual(JSON.parse(fs.readFileSync('obj/EnemyPlacementTests/alias.stage.json', 'utf8')), stage);
console.log('PASS: nine enemy types, placement image sizes, Vacuum conversion and unchanged spawn anchors.');
