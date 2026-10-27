const fs = require('fs'), path = require('path'), assert = require('assert');
const root = path.resolve(__dirname, '..'), dir = path.join(root, 'Data/Stage');
const output = path.join(dir, 'LaboratoryCombinedCourse.tmj');

if (fs.existsSync(output))
{
    throw Error('Combined map already exists; refusing to overwrite edits.');
}

const read = p => JSON.parse(fs.readFileSync(path.join(root, p)));
const early = read('Data/Stage/LaboratoryEarlyMiddleV2.tmj');
const final = read('Data/Stage/LaboratoryFinalCourseV4.tmj');
const active = read('Data/Stage/ConvertedStage.stage.json');
assert.deepStrictEqual(early.tilesets, final.tilesets);
const transforms = {
    early : {x : -14, y : 32},
    final : {x : 123, y : 16},
    boss : {x : 31, y : -9}
};
const terrain = new Map(), gates = new Map();
let lid = 1, oid = 1;

function tile(x, y, gid)
{
    const bag = gid === 1 ? terrain : gates, key = `${x},${y}`;
    assert(!bag.has(key) || bag.get(key).gid === gid, 'Conflicting tiles');
    bag.set(key, {x, y, gid});
}

function importTiles(m, t)
{
    for (const l of m.layers.filter(l => l.type === 'tilelayer' && l.visible !== false))
    {
        assert(!l.offsetx && !l.offsety && !l.x && !l.y, 'Unexpected layer offset');

        for (const c of l.chunks)
        {
            for (let i = 0; i < c.data.length; i++)
            {
                if (c.data[i])
                {
                    tile(c.x + i % c.width + t.x, -(c.y + Math.floor(i / c.width)) + t.y, c.data[i]);
                }
            }
        }
    }
}

importTiles(early, transforms.early);
importTiles(final, transforms.final);
const bossObjects = active.objects.filter(o => o.position.x >= 210);
assert(bossObjects.length > 0);
assert(active.bossEncounterZones.length === 1);

for (const o of bossObjects)
{
    assert(o.scale.x === 1 && o.scale.y === 1 && o.rotationDegrees === 0);
    assert([ 100, 301, 302 ].includes(o.typeId));
    tile(o.position.x + 31, o.position.y - 9, o.typeId === 100 ? 1 : o.typeId === 301 ? 11 : 12);
}

for (let x = 235; x <= 240; x++)
{
    for (const y of [23, 24])
    {
        tile(x, y, 1);
    }
}

function tileLayer(name, bag)
{
    const chunks = new Map();

    for (const o of bag.values())
    {
        const y = -o.y, cx = Math.floor(o.x / 16) * 16, cy = Math.floor(y / 16) * 16, key = `${cx},${cy}`;

        if (!chunks.has(key))
        {
            chunks.set(key, {x : cx, y : cy, width : 16, height : 16, data : Array(256).fill(0)});
        }

        chunks.get(key).data[(y - cy) * 16 + o.x - cx] = o.gid;
    }

    const list = [...chunks.values() ], sx = list.length ? Math.min(...list.map(c => c.x)) : 0,
          sy = list.length ? Math.min(...list.map(c => c.y)) : 0;
    return {
        id : lid++,
        name,
        type : 'tilelayer',
        visible : true,
        locked : false,
        opacity : 1,
        x : 0,
        y : 0,
        startx : sx,
        starty : sy,
        width : list.length ? Math.max(...list.map(c => c.x + 16)) - sx : 0,
        height : list.length ? Math.max(...list.map(c => c.y + 16)) - sy : 0,
        chunks : list
    };
}

function group(name, color = '#ffffff')
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

const layers = [ tileLayer('Terrain', terrain), tileLayer('Gates', gates) ];

for (const name
         of ['Enemies', 'FallRespawnZones', 'BossApproachCameraZone', 'BossEncounterZone', 'CameraBounds'])
{
    const l = group(name, name === 'Enemies' ? '#ff6677' : '#55dd88');

    for (const [m, t] of [[ early, transforms.early ], [ final, transforms.final ]])
    {
        for (const old of m.layers.filter(l => l.name === name))
        {
            for (const obj of old.objects || [])
            {
                const o = structuredClone(obj);
                o.id = oid++;
                o.x += t.x * 32 + (old.offsetx || 0);
                o.y -= t.y * 32 - (old.offsety || 0);
                l.objects.push(o);
            }
        }
    }

    if (name === 'Enemies')
    {
        for (const e of active.enemies.filter(e => e.enemyType === 'Boss'))
        {
            l.objects.push({
                id : oid++,
                name : 'Boss',
                gid : 9,
                x : (e.position.x + 31) * 32,
                y : -(e.position.y - 9) * 32,
                width : 32,
                height : 64,
                rotation : 0,
                visible : true,
                properties : [ {name : 'direction', type : 'string', value : e.direction} ]
            });
        }
    }

    const keys = {
        BossApproachCameraZone : 'bossApproachCameraZones',
        BossEncounterZone : 'bossEncounterZones'
    };

    if (keys[name])
    {
        for (const z of active[keys[name]])
        {
            l.objects.push({
                id : oid++,
                name,
                type : '',
                x : (z.minX + 31) * 32,
                y : -(z.maxY - 9) * 32,
                width : (z.maxX - z.minX) * 32,
                height : (z.maxY - z.minY) * 32,
                rotation : 0,
                visible : true
            });
        }
    }

    if (name === 'FallRespawnZones' && !(early.layers.find(l => l.name === name)?.objects.length))
    {
        l.objects.push({
            id : oid++,
            name : 'EarlyMiddle fall reset',
            type : '',
            x : -8 * 32,
            y : 12 * 32,
            width : 128 * 32,
            height : 30 * 32,
            rotation : 0,
            visible : true
        });
    }

    layers.push(l);
}

for (const [m, t, prefix] of [[ early, transforms.early, 'EarlyMiddle' ],
                              [ final, transforms.final, 'Final' ]])
{
    for (const old of m.layers.filter(l => l.name.startsWith('Guide_')))
    {
        const l = structuredClone(old);
        l.id = lid++;
        l.name = `Guide_${prefix}_${l.name.slice(6)}`;
        l.locked = false;

        for (const o of l.objects)
        {
            o.id = oid++;
            o.x += t.x * 32;
            o.y -= t.y * 32;
        }

        layers.push(l);
    }
}

const links = group('Guide_Connections (not exported)', '#88ffaa');

for (const [text, x, y] of [[ '中盤→終盤の接続・休憩地点', 118, 21 ], [ '終盤→ボス前通路', 234, 30 ],
                            [ 'ボス部屋', 256, 37 ]])
{
    links.objects.push({
        id : oid++,
        name : text,
        type : '',
        x : x * 32,
        y : -y * 32,
        width : 24 * 32,
        height : 100,
        rotation : 0,
        visible : true,
        text : {text, fontfamily : 'Yu Gothic', pixelsize : 23, wrap : true, color : '#88ffaa'}
    });
}

layers.push(links);
const map = structuredClone(early);
map.layers = layers;
map.width = map.height = 0;
map.infinite = true;
map.properties = [ {name : 'stageName', type : 'string', value : 'Laboratory - Combined Course'} ];
map.nextlayerid = lid;
map.nextobjectid = oid;
fs.writeFileSync(output, JSON.stringify(map, null, 2) + '\n');
fs.writeFileSync(path.join(dir, 'LaboratoryCombinedCourse.assembly.json'), JSON.stringify({
    sources : {
        early : 'LaboratoryEarlyMiddleV2.tmj',
        final : 'LaboratoryFinalCourseV4.tmj',
        boss : 'ConvertedStage.stage.json'
    },
    transforms,
    bridge : {minX : 235, maxX : 240, minY : 23, maxY : 24},
    terrain : terrain.size,
    gates : gates.size
},
                                                                                          null, 2) +
                                                                               '\n');
console.log(`${output}: ${terrain.size} blocks, ${gates.size} gate tiles`);
