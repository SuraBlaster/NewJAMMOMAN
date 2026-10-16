const fs = require('fs'), path = require('path'), assert = require('assert'),
      {spawnSync} = require('child_process');
process.chdir(path.resolve(__dirname, '..'));
const mapFile = 'Data/Stage/LaboratoryCombinedCourse.tmj', out = 'obj/LaboratoryCombinedCourse.roundtrip.stage.json';

function convert(input, output)
{
    const r = spawnSync(path.resolve('bin/x64/Release/StageConverter.exe'),
                        [ '--input', input, '--output', output, '--pixels-per-unit', '32' ]);
    assert.equal(r.status, 0, `StageConverter failed: ${r.error || r.stderr}`);
    return JSON.parse(fs.readFileSync(output, 'utf8'));
}

const canonical = s => ({
    ...s,
    cameraBounds : s.cameraBounds || [],
    objects : s.objects.toSorted((a, b) => a.position.x - b.position.x || a.position.y - b.position.y)
});
const stage = JSON.parse(fs.readFileSync('Data/Stage/LaboratoryCombinedCourse.stage.json', 'utf8'));
assert.deepStrictEqual(canonical(convert(mapFile, out)), canonical(stage));

const map = JSON.parse(fs.readFileSync(mapFile, 'utf8'));

for (const t of map.tilesets)
{
    t.source = path.resolve('Data/Stage', t.source).replaceAll('\\', '/');
}

const chunk = map.layers.find(l => l.type === 'tilelayer').chunks[0];
const remove = chunk.data.findIndex(v => v === 1), add = chunk.data.findIndex(v => v === 0);
chunk.data[remove] = 0;
chunk.data[add] = 1;
const enemy = map.layers.find(l => l.name === 'Enemies').objects[0];
enemy.x += 32;
map.layers.find(l => l.name === 'BossEncounterZone').objects[0].width += 32;
fs.writeFileSync('obj/LaboratoryCourse.edited.tmj', JSON.stringify(map));
const edited = convert('obj/LaboratoryCourse.edited.tmj', 'obj/LaboratoryCourse.edited.stage.json');
const cell = i => ({x : chunk.x + i % 16, y : -(chunk.y + Math.floor(i / 16))});
const exists = p => edited.objects.some(o => o.position.x === p.x && o.position.y === p.y);
assert(!exists(cell(remove)));
assert(exists(cell(add)));
assert.equal(edited.enemies[0].position.x, stage.enemies[0].position.x + 1);
assert.equal(edited.bossEncounterZones[0].maxX, stage.bossEncounterZones[0].maxX + 1);

const bg = JSON.parse(fs.readFileSync('Data/Stage/Background.stage.json'));
const manifest = JSON.parse(fs.readFileSync('Data/Stage/LaboratoryWalls.generated.json'));
const authored = bg.objects.filter(o => !manifest.ids.includes(o.id));
assert.equal(authored.length, manifest.authoredObjects);
const decorationObjects = bg.objects.filter(o => manifest.ids.includes(o.id) && o.asset >= 2).length;
assert(decorationObjects > 0);
assert.equal(manifest.ids.length, bg.objects.length - authored.length);

for (const asset of [3, 4, 5, 6, 7, 9, 10])
{
    assert(bg.objects.some(o => o.asset === asset), `Missing equipment ${asset}`);
}

assert(bg.objects.every(o => Number.isFinite(o.position[2]) && o.position[2] > 0));
console.log(`PASS: ${stage.objects.length} tiles, ${
    stage.enemies.length} enemies and all zones round-trip; edits survive conversion; ${
    decorationObjects} background decorations.`);
