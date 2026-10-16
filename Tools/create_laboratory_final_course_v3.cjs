const fs = require('fs'), path = require('path');
const root = path.resolve(__dirname, '..');
const destination = path.join(root, 'Data/Stage/LaboratoryFinalCourseV3.tmj');

if (fs.existsSync(destination))
{
    throw Error('V3 already exists; refusing to overwrite edits.');
}

const map = JSON.parse(fs.readFileSync(path.join(root, 'Data/Stage/LaboratoryCourse.tmj')));
const cells = new Map();

function block(a, b, bottom, top)
{
    for (let x = a; x <= b; x++)
    {
        for (let y = bottom; y <= top; y++)
        {
            cells.set(`${x},${y}`, [ x, y ]);
        }
    }
}

block(-4, 7, -2, -1);
block(11, 14, -1, 1);
block(18, 21, 0, 4);
block(25, 27, 0, 2);
block(9, 13, 8, 11);
block(16, 20, 11, 14);
block(23, 27, 9, 12);

block(30, 32, -18, 6);
block(33, 36, -18, 10);
block(42, 46, 8, 12);
block(48, 50, 6, 8);
block(38, 46, 18, 20);

block(53, 56, 5, 26);
block(48, 59, -2, -1);

block(64, 67, 0, 7);
block(71, 74, 5, 10);
block(61, 65, 14, 17);
block(69, 73, 18, 21);

block(79, 82, 6, 26);
block(76, 85, -2, -1);

block(90, 92, -18, 5);
block(90, 92, 8, 14);
block(93, 95, -18, 14);
block(101, 111, 7, 8);

const chunks = new Map();

for (const [x, gy] of cells.values())
{
    const y = -gy, cx = Math.floor(x / 16) * 16, cy = Math.floor(y / 16) * 16, key = `${cx},${cy}`;

    if (!chunks.has(key))
    {
        chunks.set(key, {x : cx, y : cy, width : 16, height : 16, data : Array(256).fill(0)});
    }

    chunks.get(key).data[(y - cy) * 16 + x - cx] = 1;
}

let lid = 1, oid = 1;
const list = [...chunks.values() ];

function tiles(name, populated = false)
{
    const startx = populated ? Math.min(...list.map(c => c.x)) : 0,
          starty = populated ? Math.min(...list.map(c => c.y)) : 0;
    return {
        id : lid++,
        name,
        type : 'tilelayer',
        visible : true,
        locked : false,
        opacity : 1,
        x : 0,
        y : 0,
        startx,
        starty,
        width : populated ? Math.max(...list.map(c => c.x + 16)) - startx : 0,
        height : populated ? Math.max(...list.map(c => c.y + 16)) - starty : 0,
        chunks : populated ? list : []
    };
}

function objects(name, color)
{
    return {
        id : lid++,
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
    };
}

const terrain = tiles('Terrain', true), gates = tiles('Gates'), enemies = objects('Enemies', '#ff6677');
const fall = objects('FallRespawnZones', '#ff8844');
fall.objects.push({
    id : oid++,
    name : 'Fall reset',
    type : '',
    x : -8 * 32,
    y : 10 * 32,
    width : 126 * 32,
    height : 30 * 32,
    rotation : 0,
    visible : true
});
const zones =
    [ 'BossApproachCameraZone', 'BossEncounterZone', 'CameraBounds' ].map(n => objects(n, '#55dd88'));
const wind = objects('Guide_Wind (not exported)', '#99eebb');
const route = objects('Guide_Route (not exported)', '#22ddff');
const labels = objects('Guide_Labels (not exported)', '#ffffff');

function line(layer, name, points)
{
    const [x, y] = points[0];
    layer.objects.push({
        id : oid++,
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
    const e = points.at(-1), p = points.at(-2), len = Math.hypot(e[0] - p[0], e[1] - p[1]),
          dx = (e[0] - p[0]) / len, dy = (e[1] - p[1]) / len;
    line(layer, '',
         [ [ e[0] - dx + dy * .5, e[1] - dy - dx * .5 ], e, [ e[0] - dx - dy * .5, e[1] - dy + dx * .5 ] ]);
}

function text(t, x, y, width = 20, color = '#ffffff')
{
    labels.objects.push({
        id : oid++,
        name : t,
        type : '',
        x : x * 32,
        y : -y * 32,
        width : width * 32,
        height : 110,
        rotation : 0,
        visible : true,
        text : {text : t, fontfamily : 'Yu Gothic', pixelsize : 23, wrap : true, color}
    });
}

arrow(route, '1 Weave between masses', [
    [ 0, 1 ], [ 6, 1 ], [ 9, 4 ], [ 12, 3 ], [ 14, 3 ], [ 16, 6 ], [ 20, 6 ], [ 22, 6.8 ], [ 26, 4 ],
    [ 27, 4 ], [ 29, 5 ]
]);
arrow(route, '2 Stepped same-wall climb', [
    [ 29, 5 ], [ 27.5, 6.5 ], [ 29, 8 ], [ 31, 8 ], [ 32, 8 ], [ 30.5, 9.5 ], [ 32, 11 ], [ 34, 13 ],
    [ 36, 13 ], [ 39, 15 ], [ 43, 14 ], [ 45, 14 ]
]);
arrow(route, '3 Drop under ceiling partition',
      [ [ 45, 14 ], [ 48, 12 ], [ 49, 10 ], [ 51, 8 ], [ 51.5, 4 ], [ 51.5, 1 ], [ 55, 1 ], [ 58, 1 ] ]);
arrow(route, '4 Climb floating wall and transfer', [
    [ 58, 1 ], [ 61, 4 ], [ 63, 4 ], [ 61.5, 5.5 ], [ 63, 7 ], [ 65, 9 ], [ 67, 9 ], [ 69, 11 ], [ 70, 10 ],
    [ 69, 11.5 ], [ 71, 12.5 ], [ 73, 12.5 ]
]);
arrow(route, '5 Descend and pass under next partition',
      [ [ 73, 12.5 ], [ 76, 10 ], [ 77, 6 ], [ 77, 1 ], [ 81, 1 ], [ 84, 1 ] ]);
arrow(route, '6 Broken wall finale', [
    [ 84, 1 ], [ 87, 4 ], [ 89, 3 ], [ 87.5, 4.5 ], [ 89, 6 ], [ 87, 7.5 ], [ 88, 9.5 ], [ 89, 10 ],
    [ 87.5, 11.5 ], [ 89, 13 ], [ 88, 14.5 ], [ 91, 16 ], [ 95, 16 ], [ 98, 13 ], [ 103, 10 ], [ 109, 10 ]
]);

for (const [x, y] of [[ 0, 6 ], [ 14, 8 ], [ 37, 16 ], [ 49, 2 ], [ 58, 10 ], [ 74, 3 ]])
{
    arrow(wind, 'Right wind: design only', [ [ x, y ], [ x + 4, y ] ]);
}

arrow(wind, 'Weak wind at final wall', [ [ 85, 7 ], [ 88, 7 ] ]);
text('① 天井と足場の間を渡る', 0, 30, 24);
text('② 段違いの壁を連続で登る', 28, 30, 25);
text('③ 吊り壁の下へ潜る', 48, 33, 22);
text('④ 空中の壁へ跳び移る', 60, 30, 22);
text('⑤ もう一度下へ潜る', 76, 33, 22);
text('⑥ 切れ目を越えて出口へ', 87, 30, 27);
text('入口', 0, 3, 6, '#88ffaa');
text('休憩・復活地点の候補', 48, -4, 21, '#88ffaa');
text('出口', 105, 12, 7, '#88ffaa');
text('上 → 下 → 上 → 下 → 最後の登り', 30, -22, 55, '#22ddff');
text('風は右向き／ガイドのみ・未実装', 0, -6, 28, '#99eebb');
map.infinite = true;
map.width = map.height = 0;
map.layers = [ terrain, gates, enemies, fall, ...zones, wind, route, labels ];
map.properties = [ {name : 'stageName', type : 'string', value : 'Laboratory - Final Wind Course V3'} ];
map.nextlayerid = lid;
map.nextobjectid = oid;
fs.writeFileSync(destination, JSON.stringify(map, null, 2) + '\n');
console.log(`${destination}: ${cells.size} blocks`);
