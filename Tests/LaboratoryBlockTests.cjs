const fs = require('fs'), path = require('path'), assert = require('assert');
const {spawnSync} = require('child_process');
const {exportStage} = require('../Tools/export_stage_tiled.cjs');
process.chdir(path.resolve(__dirname, '..'));
const dir = 'obj/LaboratoryBlockTests';
fs.mkdirSync(dir, {recursive: true});
const defs = JSON.parse(fs.readFileSync('Data/Stage/TileTypes.json')).definitions;
for (const id of [101, 102, 103]) {
    const def = defs.find(d => d.typeId === id);
    assert(def.collision && def.collisionModelPath === 'Data/Model/Cube/Cube.gltf');
    const model = JSON.parse(fs.readFileSync(def.modelPath));
    const bytes = fs.readFileSync(path.join(path.dirname(def.modelPath), model.buffers[0].uri));
    assert.equal(bytes.length, model.buffers[0].byteLength);
    const lo = [Infinity, Infinity, Infinity], hi = [-Infinity, -Infinity, -Infinity];
    for (const p of model.meshes[0].primitives) {
        const a = model.accessors[p.attributes.POSITION], v = model.bufferViews[a.bufferView];
        for (let n = 0; n < a.count; n++) for (let k = 0; k < 3; k++) {
            const x = bytes.readFloatLE((v.byteOffset || 0) + n * 12 + k * 4);
            assert(Number.isFinite(x)); lo[k] = Math.min(lo[k], x); hi[k] = Math.max(hi[k], x);
        }
        assert.equal(model.accessors[p.attributes.NORMAL].count, a.count);
        assert.equal(model.accessors[p.attributes.TANGENT].count, a.count);
    }
    assert(lo.every(v => Math.abs(v + .5) < 1e-6));
    assert(hi.every(v => Math.abs(v - .5) < 1e-6));
}
function convert(input, output) {
    const r = spawnSync(path.resolve('bin/x64/Release/StageConverter.exe'),
        ['--input', input, '--output', output, '--pixels-per-unit', '32']);
    assert.ifError(r.error); assert.equal(r.status, 0, String(r.stderr));
    return JSON.parse(fs.readFileSync(output));
}
const map = JSON.parse(fs.readFileSync('Data/Stage/LaboratoryCourse.tmj'));
// Adding a palette must not alter any existing placements or zones.
const originalMap = JSON.parse(spawnSync('git', ['show', 'HEAD:Data/Stage/LaboratoryCourse.tmj'],
                                       {encoding: 'utf8'}).stdout);
for (const t of originalMap.tilesets)
    t.source = path.resolve('Data/Stage', t.source).replaceAll('\\', '/');
fs.writeFileSync(`${dir}/original.tmj`, JSON.stringify(originalMap));
assert.deepStrictEqual(convert('Data/Stage/LaboratoryCourse.tmj', `${dir}/current.stage.json`),
                       convert(`${dir}/original.tmj`, `${dir}/original.stage.json`));
for (const t of map.tilesets) t.source = path.resolve('Data/Stage', t.source).replaceAll('\\', '/');
const chunk = map.layers.find(l => l.type === 'tilelayer').chunks.find(c => c.data.filter(v => v === 1).length >= 3);
let i = 0;
for (let n = 0; n < chunk.data.length && i < 3; n++) if (chunk.data[n] === 1) chunk.data[n] = 14 + i++;
assert.equal(i, 3);
fs.writeFileSync(`${dir}/edited.tmj`, JSON.stringify(map));
const edited = convert(`${dir}/edited.tmj`, `${dir}/edited.stage.json`);
for (const id of [101, 102, 103]) assert.equal(edited.objects.filter(o => o.typeId === id).length, 1);
exportStage(`${dir}/edited.stage.json`, `${dir}/exported.tmj`);
const roundtrip = convert(`${dir}/exported.tmj`, `${dir}/roundtrip.stage.json`);
const canonical = objects => objects.toSorted((a,b) => a.position.x-b.position.x || a.position.y-b.position.y);
assert.deepStrictEqual(canonical(roundtrip.objects), canonical(edited.objects));
assert.deepStrictEqual(roundtrip.enemies, edited.enemies);
console.log('PASS: 3 unit-sized glTF blocks, shared cube collisions, and all terrain IDs survive Tiled conversion/export.');
