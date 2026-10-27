const fs = require('fs'), path = require('path'), assert = require('assert');
const dir = path.resolve(__dirname, '../Data/Stage'), file = path.join(dir, 'LaboratoryCombinedCourse.tmj');
const original = fs.readFileSync(file), m = JSON.parse(original);
const bounds = m.layers.find(l => l.name === 'CameraBounds'),
      triggers = m.layers.find(l => l.name === 'CameraSectionTriggers');
assert(bounds && triggers);
const stamp = new Date().toISOString().replace(/[:.]/g, '-');
const backup = `LaboratoryCombinedCourse.before-continuous-descent-${stamp}.tmj`;
fs.writeFileSync(path.join(dir, backup), original);

const local = o =>
    o.properties?.some(p => p.name === 'cameraSection' && String(p.value).startsWith('landing_'));
bounds.objects = bounds.objects.filter(o => !local(o));
const recentNames = new Set([
    'LeaveMiddlePreviewRight', 'LeaveMiddlePreviewLeft', 'LeaveFinalDrop1', 'ReturnFromFinalDrop1',
    'LeaveFinalDrop2', 'ReturnFromFinalDrop2'
]);
triggers.objects = triggers.objects.filter(o => !local(o) && !recentNames.has(o.name));
const terrain = m.layers.find(l => l.name === 'Terrain');
const edits = [];

function tile(x, gy, v)
{
    const y = -gy, cx = Math.floor(x / 16) * 16, cy = Math.floor(y / 16) * 16;
    let c = terrain.chunks.find(c => c.x === cx && c.y === cy);

    if (!c)
    {
        c = {x : cx, y : cy, width : 16, height : 16, data : Array(256).fill(0)};
        terrain.chunks.push(c);
    }

    const index = (y - c.y) * c.width + x - c.x;

    if (c.data[index] !== v)
    {
        edits.push({x, y : gy, before : c.data[index], after : v});
    }

    c.data[index] = v;
}

function block(a, b, lo, hi)
{
    for (let x = a; x <= b; x++)
    {
        for (let y = lo; y <= hi; y++)
        {
            tile(x, y, 1);
        }
    }
}

block(100, 102, 2, 3);

block(173, 174, 20, 21);
block(175, 177, 17, 18);

block(198, 199, 21, 23);
block(200, 201, 19, 20);
block(202, 203, 16, 17);
fs.writeFileSync(file, JSON.stringify(m, null, 2) + '\n');
fs.writeFileSync(path.join(dir, 'LaboratoryCombinedCourse.continuous-descent.json'), JSON.stringify({
    backup,
    edits,
    method : 'Terrain-only descent steps; removed recently added landing camera sections and their triggers.'
},
                                                                                                    null, 2) +
                                                                                         '\n');
console.log(
    {backup, editedTiles : edits.length, bounds : bounds.objects.length, triggers : triggers.objects.length});
