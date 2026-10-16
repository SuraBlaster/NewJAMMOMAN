const fs = require('fs'), path = require('path');
const root = path.resolve(__dirname, '..');
const output = path.join(root, 'Data/Stage/LaboratoryFinalCourseV4.tmj');

if (fs.existsSync(output))
{
    throw Error('V4 exists; refusing to overwrite edits.');
}

const map = JSON.parse(fs.readFileSync(path.join(root, 'Data/Stage/LaboratoryFinalCourseV3.tmj')));
const terrain = map.layers.find(l => l.name === 'Terrain');

function set(x, gameY, value)
{
    const y = -gameY;
    const chunk = terrain.chunks.find(c => x >= c.x && x < c.x + c.width && y >= c.y && y < c.y + c.height);

    if (!chunk)
    {
        throw Error('Missing chunk for changed cell');
    }

    chunk.data[(y - chunk.y) * chunk.width + x - chunk.x] = value;
}

for (let x = 48; x <= 51; x++)
{
    set(x, -1, 0);
    set(x, -3, 1);
}

for (let x = 52; x <= 55; x++)
{
    set(x, -1, 0);
    set(x, -2, 0);
}

let id = map.nextobjectid;
const guides = map.layers.find(l => l.name.startsWith('Guide_Route'));

function removeArrow(name)
{
    const i = guides.objects.findIndex(o => o.name === name);

    if (i < 0)
    {
        throw Error(`Missing guide ${name}`);
    }

    guides.objects.splice(i, 2);
}

function line(name, points)
{
    const [x, y] = points[0];
    guides.objects.push({
        id : id++,
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
    const e = points.at(-1), p = points.at(-2), len = Math.hypot(e[0] - p[0], e[1] - p[1]);
    const dx = (e[0] - p[0]) / len, dy = (e[1] - p[1]) / len;
    line('',
         [ [ e[0] - dx + dy * .5, e[1] - dy - dx * .5 ], e, [ e[0] - dx - dy * .5, e[1] - dy + dx * .5 ] ]);
}

removeArrow('3 Drop under ceiling partition');
arrow('3 Land, dash-jump below hanging wall', [
    [ 45, 14 ], [ 48, 12 ], [ 49, 10 ], [ 51, 8 ], [ 50.5, 4 ], [ 50.5, 0 ], [ 51, 0 ], [ 52.5, 1.6 ],
    [ 54, 2.2 ], [ 55.5, 1.4 ], [ 57, 1 ], [ 58, 1 ]
]);
const labels = map.layers.find(l => l.name.startsWith('Guide_Labels'));
const rest = labels.objects.find(o => o.name === '休憩・復活地点の候補');
rest.x = -3 * 32;
rest.y = -4 * 32;
rest.width = 20 * 32;
rest.name = rest.text.text = '中盤→終盤の休憩・復活地点候補';
const entrance = labels.objects.find(o => o.name === '入口');
entrance.name = entrance.text.text = '中盤から';
entrance.y = -7 * 32;
const heading = labels.objects.find(o => o.name === '③ 吊り壁の下へ潜る');
heading.name = heading.text.text = '③ 着地→吊り壁の下をジャンプ';
labels.objects.push({
    id : id++,
    name : '着地からすぐダッシュジャンプ',
    type : '',
    x : 48 * 32,
    y : 4 * 32,
    width : 22 * 32,
    height : 110,
    rotation : 0,
    visible : true,
    text : {
        text : '着地からすぐダッシュジャンプ',
        fontfamily : 'Yu Gothic',
        pixelsize : 23,
        wrap : true,
        color : '#22ddff'
    }
});

const wind = map.layers.find(l => l.name.startsWith('Guide_Wind'));
wind.objects = wind.objects.filter(o => o.x !== 0);
const windNote = labels.objects.find(o => o.name === '風は右向き／ガイドのみ・未実装');
windNote.name = windNote.text.text = '入口は無風／終盤は右向きの風（未実装）';
windNote.width = 35 * 32;
map.properties.find(p => p.name === 'stageName').value = 'Laboratory - Final Wind Course V4';
map.nextobjectid = id;
fs.writeFileSync(output, JSON.stringify(map, null, 2) + '\n');
console.log(output);
