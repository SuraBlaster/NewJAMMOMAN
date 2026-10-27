const fs = require('fs'), path = require('path');
const root = path.resolve(__dirname, '..');
const output = path.join(root, 'Data/Stage/LaboratoryEarlyMiddleV2.tmj');

if (fs.existsSync(output))
{
    throw Error('Output exists; refusing to overwrite edits.');
}

const map = JSON.parse(fs.readFileSync(path.join(root, 'Data/Stage/LaboratoryNewStage.tmj')));
const terrain = map.layers.find(l => l.name === 'Terrain');
const cells = new Map();

for (const c of terrain.chunks)
{
    for (let i = 0; i < c.data.length; i++)
    {
        if (c.data[i])
        {
            const x = c.x + i % c.width, y = -(c.y + Math.floor(i / c.width));
            cells.set(`${x},${y}`, [ x, y ]);
        }
    }
}

function rect(a, b, lo, hi, v = 1)
{
    for (let x = a; x <= b; x++)
    {
        for (let y = lo; y <= hi; y++)
        {
            const k = `${x},${y}`;

            if (v)
            {
                cells.set(k, [ x, y ]);
            }
            else
            {
                cells.delete(k);
            }
        }
    }
}

rect(45, 52, -32, -31, 0);
rect(45, 50, -34, -33, 0);

rect(44, 50, -39, -38);
rect(49, 49, -37, -37);
rect(50, 50, -37, -36);
rect(51, 51, -37, -34);

rect(56, 58, -32, -30);

rect(101, 107, -18, -15);
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

terrain.chunks = [...chunks.values() ];
terrain.startx = Math.min(...terrain.chunks.map(c => c.x));
terrain.starty = Math.min(...terrain.chunks.map(c => c.y));
terrain.width = Math.max(...terrain.chunks.map(c => c.x + 16)) - terrain.startx;
terrain.height = Math.max(...terrain.chunks.map(c => c.y + 16)) - terrain.starty;
let lid = map.nextlayerid, oid = map.nextobjectid;
const guide = {
    id : lid++,
    name : 'Guide_Learning (not exported)',
    type : 'objectgroup',
    visible : true,
    locked : false,
    opacity : 1,
    x : 0,
    y : 0,
    draworder : 'index',
    color : '#22ddff',
    objects : []
};

function label(t, x, y, w = 24, color = '#ffffff')
{
    guide.objects.push({
        id : oid++,
        name : t,
        type : '',
        x : x * 32,
        y : -y * 32,
        width : w * 32,
        height : 115,
        rotation : 0,
        visible : true,
        text : {text : t, fontfamily : 'Yu Gothic', pixelsize : 23, wrap : true, color}
    });
}

function line(name, points)
{
    const [x, y] = points[0];
    guide.objects.push({
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

function arrow(name, points)
{
    line(name, points);
    const e = points.at(-1), p = points.at(-2), len = Math.hypot(e[0] - p[0], e[1] - p[1]),
          dx = (e[0] - p[0]) / len, dy = (e[1] - p[1]) / len;
    line('',
         [ [ e[0] - dx + dy * .5, e[1] - dy - dx * .5 ], e, [ e[0] - dx - dy * .5, e[1] - dy + dx * .5 ] ]);
}

label('① 安全な移動・ジャンプ練習', 15, -25);
label('② ダッシュジャンプ練習', 40, -22);
label('落ちても下の床から戻れる', 43, -42, 24, '#88ffaa');
label('③ 短い壁で壁キックを覚える', 55, -17);
label('④ 既存の柱渡りで応用', 83, -2);
label('⑤ 短く押して低く跳ぶ', 98, -11);
label('ジャンプを早めに離す', 99, -27, 24, '#22ddff');
label('⑥ 終盤への登り', 118, -2);
arrow('Dash jump', [ [ 41, -31 ], [ 44, -31 ], [ 46, -29 ], [ 48, -28.5 ], [ 50, -29.5 ], [ 52, -31 ] ]);
arrow('Safe return', [ [ 46, -36 ], [ 48, -36 ], [ 49, -35 ], [ 50, -34 ], [ 52, -31 ] ]);
arrow('Short same-wall kick',
      [ [ 54, -31 ], [ 55, -31 ], [ 54, -29.5 ], [ 55, -28 ], [ 57, -28 ], [ 59, -28 ] ]);
arrow('Low dash jump', [ [ 97, -20 ], [ 99, -20 ], [ 101, -20 ], [ 103, -21 ], [ 105, -22 ] ]);
map.layers.forEach(l => l.locked = false);
map.layers.push(guide);
map.properties = map.properties || [];
const name = map.properties.find(p => p.name === 'stageName');

if (name)
{
    name.value = 'Laboratory - Early and Middle Revision';
}

map.nextlayerid = lid;
map.nextobjectid = oid;
fs.writeFileSync(output, JSON.stringify(map, null, 2) + '\n');
console.log(`${output}: ${cells.size} blocks`);
