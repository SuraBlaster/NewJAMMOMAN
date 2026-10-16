const fs = require('fs'), path = require('path'), assert = require('assert');
const root = path.resolve(__dirname, '..'), dir = path.join(root, 'Data/Stage');
const file = path.join(dir, 'LaboratoryCombinedCourse.tmj');
const original = fs.readFileSync(file), map = JSON.parse(original);
const bounds = map.layers.find(l => l.name === 'CameraBounds');
const triggers = map.layers.find(l => l.name === 'CameraSectionTriggers');
assert(bounds && triggers, 'Current camera section support is required');
assert(!bounds.objects.some(o => o.name === 'LandingPreview_Middle'), 'Already applied');
const stamp = new Date().toISOString().replace(/[:.]/g, '-');
const backup = path.join(dir, `LaboratoryCombinedCourse.before-landing-visibility-${stamp}.tmj`);
fs.writeFileSync(backup, original);
let id = Math.max(map.nextobjectid || 1, ...map.layers.flatMap(l => (l.objects || []).map(o => o.id + 1)));

function rect(layer, name, x0, x1, y0, y1, section)
{
    layer.objects.push({
        id : id++,
        name,
        type : '',
        x : x0 * 32,
        y : -y1 * 32,
        width : (x1 - x0) * 32,
        height : (y1 - y0) * 32,
        rotation : 0,
        visible : true,
        properties : [ {name : 'cameraSection', type : 'string', value : section} ]
    });
}

function view(name, section, x0, x1, y0, y1)
{
    rect(bounds, name, x0, x1, y0, y1, section);
}

function trigger(name, section, x0, x1, y0, y1)
{
    rect(triggers, name, x0, x1, y0, y1, section);
}

view('LandingPreview_Middle', 'landing_middle', 94, 119, -3, 9.5);
trigger('PreviewMiddleLanding', 'landing_middle', 96, 100, 5, 12);
trigger('LeaveMiddlePreviewRight', 'second', 107, 112, -12, 48);
trigger('LeaveMiddlePreviewLeft', 'second', 93, 95, -12, 48);

view('LandingPreview_FinalDrop1', 'landing_final_drop1', 165, 184, 12, 28);
trigger('PreviewFinalDrop1', 'landing_final_drop1', 170, 173, 23, 27);
view('LandingPreview_FinalDrop1Low', 'landing_final_drop1_low', 168, 185, 11, 23);
trigger('PreviewFinalDrop1Low', 'landing_final_drop1_low', 173, 175, 18, 21);
trigger('LeaveFinalDrop1', 'third', 180, 183, -12, 48);
trigger('ReturnFromFinalDrop1', 'third', 167, 169, -12, 48);

view('LandingPreview_FinalDrop2', 'landing_final_drop2', 189, 211, 18, 29);
trigger('PreviewFinalDrop2', 'landing_final_drop2', 194, 198, 24, 29);
view('LandingPreview_FinalDrop2Low', 'landing_final_drop2_low', 194, 213, 12, 24);
trigger('PreviewFinalDrop2Low', 'landing_final_drop2_low', 198, 201, 19, 22);
trigger('LeaveFinalDrop2', 'third', 203, 208, -12, 48);
trigger('ReturnFromFinalDrop2', 'third', 191, 193, -12, 48);
const terrain = map.layers.find(l => l.name === 'Terrain');

function tile(x, gy, v)
{
    const y = -gy, cx = Math.floor(x / 16) * 16, cy = Math.floor(y / 16) * 16;
    let c = terrain.chunks.find(c => c.x === cx && c.y === cy);

    if (!c)
    {
        c = {x : cx, y : cy, width : 16, height : 16, data : Array(256).fill(0)};
        terrain.chunks.push(c);
    }

    c.data[(y - c.y) * c.width + x - c.x] = v;
}

for (let y = 22; y <= 24; y++)
{
    tile(173, y, 0);
}

for (let x = 173; x <= 174; x++)
{
    for (let y = 18; y <= 19; y++)
    {
        tile(x, y, 1);
    }
}

for (let x = 198; x <= 200; x++)
{
    for (let y = 19; y <= 20; y++)
    {
        tile(x, y, 1);
    }
}

map.nextobjectid = id;
fs.writeFileSync(file, JSON.stringify(map, null, 2) + '\n');
fs.writeFileSync(path.join(dir, 'LaboratoryCombinedCourse.landing-visibility.json'), JSON.stringify({
    backup : path.basename(backup),
    changes : {
        removed : [ [ 173, 22 ], [ 173, 23 ], [ 173, 24 ] ],
        added : [
            [ 173, 18 ], [ 173, 19 ], [ 174, 18 ], [ 174, 19 ], [ 198, 19 ], [ 198, 20 ], [ 199, 19 ],
            [ 199, 20 ], [ 200, 19 ], [ 200, 20 ]
        ]
    },
    cameraSections : [
        'landing_middle', 'landing_final_drop1', 'landing_final_drop1_low', 'landing_final_drop2',
        'landing_final_drop2_low'
    ]
},
                                                                                                    null, 2) +
                                                                                         '\n');
console.log(`Updated ${file}\nBackup: ${backup}`);
