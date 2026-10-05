const fs = require('fs'), path = require('path'), assert = require('assert');
const {spawnSync} = require('child_process');
process.chdir(path.resolve(__dirname, '..'));
const dir = 'obj/CameraBoundsTests';
fs.mkdirSync(dir, {recursive: true});
const sample = JSON.parse(fs.readFileSync('Tools/StageConverter/Sample/CameraBoundsSample.tmj', 'utf8'));
function convert(map, name, succeeds = true) {
 const input = `${dir}/${name}.tmj`, output = `${dir}/${name}.stage.json`;
 fs.writeFileSync(input, JSON.stringify(map));
 const result = spawnSync(path.resolve('bin/x64/Release/StageConverter.exe'),
  ['--input', input, '--output', output, '--pixels-per-unit', '32']);
 assert.ifError(result.error);
 assert.equal(result.status, succeeds ? 0 : 2, name);
 return succeeds ? JSON.parse(fs.readFileSync(output, 'utf8')) : null;
}
assert.deepStrictEqual(convert(sample, 'sample').cameraBounds,
 [{minX: 2, maxX: 32, minY: -16, maxY: -1}]);
const nested = structuredClone(sample);
nested.layers[0].offsetx = 32;
nested.layers[0].offsety = 64;
nested.layers[0].objects.push({...nested.layers[0].objects[0], id: 2, x: 1024});
nested.layers[0].objects.push({...nested.layers[0].objects[0], id: 3, visible: false});
nested.layers = [{type: 'group', name: 'Group', visible: true, offsetx: -64,
 offsety: -32, layers: nested.layers}];
const result = convert(nested, 'nested');
assert.deepStrictEqual(result.cameraBounds, [
 {minX: 1, maxX: 31, minY: -17, maxY: -2},
 {minX: 31, maxX: 61, minY: -17, maxY: -2}
]);
assert.deepStrictEqual(result.objects, []);
nested.layers[0].visible = false;
assert.deepStrictEqual(convert(nested, 'hidden').cameraBounds, []);
assert.deepStrictEqual(convert({...sample, layers: []}, 'empty').cameraBounds, []);
for (const [name, changes] of [['rotated', {rotation: 10}], ['zeroWidth', {width: 0}],
 ['negativeHeight', {height: -1}], ['ellipse', {ellipse: true}]]) {
 const invalid = structuredClone(sample);
 Object.assign(invalid.layers[0].objects[0], changes);
 convert(invalid, name, false);
}
const {exportStage} = require('../Tools/export_stage_tiled.cjs');
const exportedMap = `${dir}/exported.tmj`;
exportStage('obj/CameraBoundsTests/sample.stage.json', exportedMap);
const exported = JSON.parse(fs.readFileSync(exportedMap, 'utf8'));
for (const tileset of exported.tilesets) tileset.source = path.resolve(dir, tileset.source);
assert.deepStrictEqual(convert(exported, 'exportedRoundtrip').cameraBounds,
 convert(sample, 'sampleAgain').cameraBounds);
console.log('PASS: camera bounds coordinates, offsets, multiple rectangles, visibility, invalid shapes and export round-trip.');
