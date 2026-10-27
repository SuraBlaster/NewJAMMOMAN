const fs = require('fs');
const path = require('path');
const root = path.resolve(__dirname, '..');
const revision2 = process.argv.includes('--revision2');
const output = path.join(root, revision2 ? 'Data/Stage/LaboratoryFinalCourseV2.tmj'
                                         : 'Data/Stage/LaboratoryFinalCourse.tmj');

if (fs.existsSync(output))
{
    throw new Error('Map already exists; refusing to overwrite edits.');
}

const map = JSON.parse(fs.readFileSync(path.join(root, 'Data/Stage/LaboratoryCourse.tmj'), 'utf8'));
const cells = new Map();

function rect(x0, x1, y0, y1)
{
    for (let x = x0; x <= x1; x++)
    {
        for (let y = y0; y <= y1; y++)
        {
            cells.set(`${x},${y}`, [ x, y ]);
        }
    }
}

rect(-4, 10, -2, -1);
rect(14, 17, -1, 0);
rect(22, 26, 0, 2);
rect(31, 36, 0, 1);
rect(8, 13, 8, 10);
rect(19, 24, 10, 12);
rect(33, 37, 8, 10);

rect(42, 46, 4, 15);
rect(30, 38, 16, 17);

rect(27, 57, -6, -5);

rect(62, 64, -5, 5);
rect(62, 64, 8, 14);
rect(70, 80, 6, 7);

if (revision2)
{

    for (let x = 30; x <= 38; x++)
    {
        for (let y = 16; y <= 17; y++)
        {
            cells.delete(`${x},${y}`);
        }
    }

    for (let x = 27; x <= 46; x++)
    {
        for (let y = -6; y <= -5; y++)
        {
            cells.delete(`${x},${y}`);
        }
    }

    rect(42, 46, -18, 15);
    rect(47, 57, -6, -5);

    rect(62, 64, -18, -6);
    rect(65, 67, -18, 14);
}

const chunks = new Map();

for (const [x, gameY] of cells.values())
{
    const y = -gameY, cx = Math.floor(x / 16) * 16, cy = Math.floor(y / 16) * 16;
    const key = `${cx},${cy}`;

    if (!chunks.has(key))
    {
        chunks.set(key, {x : cx, y : cy, width : 16, height : 16, data : Array(256).fill(0)});
    }

    chunks.get(key).data[(y - cy) * 16 + x - cx] = 1;
}

let layerId = 1, objectId = 1;
const chunkList = [...chunks.values() ];
const tileLayer = (name, populated = false) => ({
    id : layerId++,
    name,
    type : 'tilelayer',
    visible : true,
    locked : false,
    opacity : 1,
    x : 0,
    y : 0,
    startx : populated ? Math.min(...chunkList.map(c => c.x)) : 0,
    starty : populated ? Math.min(...chunkList.map(c => c.y)) : 0,
    width : populated ? Math.max(...chunkList.map(c => c.x + c.width)) - Math.min(...chunkList.map(c => c.x))
                      : 0,
    height : populated
                 ? Math.max(...chunkList.map(c => c.y + c.height)) - Math.min(...chunkList.map(c => c.y))
                 : 0,
    chunks : populated ? chunkList : []
});
const objectLayer = (name, color) => ({
    id : layerId++,
    name,
    type : 'objectgroup',
    visible : true,
    locked : false,
    opacity : 1,
    x : 0,
    y : 0,
    draworder : 'index',
    color,
    objects : []
});
const route = objectLayer('Guide_Route (not exported)', '#22ddff');
const wind = objectLayer('Guide_Wind (not exported)', '#99eebb');
const labels = objectLayer('Guide_Labels (not exported)', '#ffffff');

function line(layer, name, points)
{
    const [x, y] = points[0];
    layer.objects.push({
        id : objectId++,
        name,
        type : '',
        x : x * 32,
        y : -y * 32,
        width : 0,
        height : 0,
        rotation : 0,
        visible : true,
        polyline : points.map(([ px, py ]) => ({x : (px - x) * 32, y : -(py - y) * 32}))
    });
}

function arrow(layer, name, points)
{
    line(layer, name, points);
    const end = points.at(-1), before = points.at(-2);
    const dx = end[0] - before[0], dy = end[1] - before[1], len = Math.hypot(dx, dy);
    const ux = dx / len, uy = dy / len;
    line(layer, '', [
        [ end[0] - ux + uy * .55, end[1] - uy - ux * .55 ], end,
        [ end[0] - ux - uy * .55, end[1] - uy + ux * .55 ]
    ]);
}

function text(name, x, y, width = 14, size = 25, color = '#ffffff')
{
    labels.objects.push({
        id : objectId++,
        name,
        type : '',
        x : x * 32,
        y : -y * 32,
        width : width * 32,
        height : 100,
        rotation : 0,
        visible : true,
        text : {text : name, fontfamily : 'Yu Gothic', pixelsize : size, wrap : true, color}
    });
}

arrow(route, '01 Dash jumps', [
    [ 0, 1 ], [ 9, 1 ], [ 12, 3 ], [ 15, 2 ], [ 17, 2 ], [ 20, 5 ], [ 24, 4 ], [ 26, 4 ], [ 29, 5 ],
    [ 33, 3 ], [ 36, 3 ], [ 39, 5 ], [ 41, 6 ]
]);

if (revision2)
{
    arrow(route, '02 Climb and cross the summit', [
        [ 41, 6 ], [ 39.5, 7.5 ], [ 41, 9 ], [ 39.5, 10.5 ], [ 41, 12 ], [ 39.5, 13.5 ], [ 41, 15 ],
        [ 40, 18 ], [ 43, 18 ], [ 46, 18 ], [ 49, 17 ]
    ]);
    arrow(route, '03 Descend only on the right',
          [ [ 49, 17 ], [ 50, 12 ], [ 51, 6 ], [ 51, 0 ], [ 51, -3 ] ]);
    arrow(route, '03 Safe landing and dash', [ [ 51, -3 ], [ 56, -3 ] ]);
}
else
{
    arrow(route, '02 Same-wall climb', [
        [ 41, 6 ], [ 39.5, 7.5 ], [ 41, 9 ], [ 39.5, 10.5 ], [ 41, 12 ], [ 39.5, 13.5 ], [ 41, 15 ],
        [ 40, 18.8 ], [ 37, 19 ], [ 32, 19 ]
    ]);
    arrow(route, '03 Turn left and descend',
          [ [ 32, 19 ], [ 28.5, 19 ], [ 28, 15 ], [ 28, 9 ], [ 28, 3 ], [ 28, -3 ] ]);
    arrow(route, '03 Run under floating blocks', [ [ 28, -3 ], [ 34, -3 ], [ 47, -3 ], [ 56, -3 ] ]);
}

arrow(route, '04 Jump to final wall', [ [ 56, -3 ], [ 59, 0 ], [ 61, -1 ] ]);
arrow(route, '04 Same-wall climb and gap', [
    [ 61, -1 ], [ 59.5, .5 ], [ 61, 2 ], [ 59.5, 3.5 ], [ 61, 5 ], [ 59, 6.5 ], [ 60, 9 ], [ 61, 9.5 ],
    [ 59.5, 11 ], [ 61, 12.5 ], [ 60, 14 ], [ 62, 16 ], [ 67, 16 ], [ 69, 13 ], [ 72, 9 ], [ 78, 9 ]
]);

for (const x of [1, 15, 28, 40])
{
    arrow(wind, 'Wind Right (design only)', [ [ x, 7 ], [ x + 5, 7 ] ]);
}

for (const x of [50, 65])
{
    arrow(wind, 'Weak wind (design only)', [ [ x, 4 ], [ x + 3, 4 ] ]);
}

text('① 上下の隙間を渡る', 0, 23, 25);
text(revision2 ? '② 壁を登って、右へ越える' : '② 同じ壁を登り、左上へ', 30, 23, 24);
text(revision2 ? '③ 壁の右側へ降りる' : '③ 左端から降りて、下を右へ', revision2 ? 47 : 27,
     revision2 ? 21 : -8, 20);
text('④ 壁の切れ目を越えて出口へ', 58, 23, 30);
text('入口', 0, 3, 5, 22, '#88ffaa');
text('休憩・復活地点の候補', revision2 ? 48 : 27, -1.5, 13, 19, '#88ffaa');

if (revision2)
{
    text('下から通り抜けられない壁', 32, -8, 22, 19, '#ffbb77');
    text('切れ目の奥は壁：上まで登る', 60, -8, 25, 19, '#ffbb77');
}

text('出口', 76, 11, 6, 22, '#88ffaa');
text('風向き →（ガイドのみ・未実装）', 0, 16, 26, 20, '#99eebb');
const fall = objectLayer('FallRespawnZones', '#ff8844');
fall.objects.push({
    id : objectId++,
    name : 'Fall reset',
    type : '',
    x : -8 * 32,
    y : 10 * 32,
    width : 94 * 32,
    height : revision2 ? 30 * 32 : 8 * 32,
    rotation : 0,
    visible : true
});
map.infinite = true;
map.width = map.height = 0;
map.layers = [
    tileLayer('Terrain', true), tileLayer('Gates'), objectLayer('Enemies', '#ff6677'), fall,
    objectLayer('BossApproachCameraZone', '#ffcc00'), objectLayer('BossEncounterZone', '#00a8ff'),
    objectLayer('CameraBounds', '#55dd88'), wind, route, labels
];
map.properties = [ {
    name : 'stageName',
    type : 'string',
    value : revision2 ? 'Laboratory - Final Wind Course V2' : 'Laboratory - Final Wind Course Prototype'
} ];
map.nextlayerid = layerId;
map.nextobjectid = objectId;
fs.writeFileSync(output, JSON.stringify(map, null, 2) + '\n');
console.log(`${output}: ${cells.size} blocks`);
