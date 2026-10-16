const fs = require('fs'), path = require('path'), assert = require('assert');
const dir = path.resolve(__dirname, '../Data/Stage'), file = path.join(dir, 'LaboratoryCombinedCourse.tmj');
const raw = fs.readFileSync(file), map = JSON.parse(raw);
const insertions = [ {at : 75, count : 5, copyColumn : 74}, {at : 86, count : 2, copyColumn : 86} ];

if (fs.existsSync(path.join(dir, 'LaboratoryCombinedCourse.spacing-extension.json')))
{
    throw Error('Spacing extension already applied; refusing to repeat.');
}

const stamp = new Date().toISOString().replace(/[:.]/g, '-');
const backup = `LaboratoryCombinedCourse.before-spacing-${stamp}.tmj`;
fs.writeFileSync(path.join(dir, backup), raw);

const transform = x => x + insertions.reduce((n, s) => n + (x >= s.at - .5 ? s.count : 0), 0);
const cellShift = x => x + insertions.reduce((n, s) => n + (x >= s.at ? s.count : 0), 0);

function layer(l)
{
    if (l.type === 'group')
    {
        for (const c of l.layers)
        {
            layer(c);
        }

        return;
    }

    assert(!l.offsetx && !l.x, 'Unexpected X offset: preserve by extending the tool first');

    if (l.type === 'tilelayer')
    {
        const cells = [];

        for (const c of l.chunks || [])
        {
            for (let i = 0; i < c.data.length; i++)
            {
                if (c.data[i])
                {
                    cells.push({x : c.x + i % c.width, y : c.y + Math.floor(i / c.width), gid : c.data[i]});
                }
            }
        }

        const shifted = cells.map(c => ({...c, x : cellShift(c.x)}));

        for (const s of insertions)
        {
            const start = cellShift(s.at) - s.count;

            for (const c of cells.filter(c => c.x === s.copyColumn))
            {
                for (let i = 0; i < s.count; i++)
                {
                    shifted.push({...c, x : start + i});
                }
            }
        }

        const chunks = new Map();

        for (const c of shifted)
        {
            const cx = Math.floor(c.x / 16) * 16, cy = Math.floor(c.y / 16) * 16, key = `${cx},${cy}`;

            if (!chunks.has(key))
            {
                chunks.set(key, {x : cx, y : cy, width : 16, height : 16, data : Array(256).fill(0)});
            }

            const out = chunks.get(key), index = (c.y - cy) * 16 + c.x - cx;
            assert(!out.data[index] || out.data[index] === c.gid, 'Conflicting tile');
            out.data[index] = c.gid;
        }

        l.chunks = [...chunks.values() ];
        l.startx = l.chunks.length ? Math.min(...l.chunks.map(c => c.x)) : 0;
        l.starty = l.chunks.length ? Math.min(...l.chunks.map(c => c.y)) : 0;
        l.width = l.chunks.length ? Math.max(...l.chunks.map(c => c.x + 16)) - l.startx : 0;
        l.height = l.chunks.length ? Math.max(...l.chunks.map(c => c.y + 16)) - l.starty : 0;
    }
    else if (l.type === 'objectgroup')
    {
        for (const o of l.objects)
        {
            const oldX = o.x, newX = transform(oldX / 32) * 32;

            if (o.polyline || o.polygon)
            {
                for (const p of o.polyline || o.polygon)
                {
                    p.x = transform((oldX + p.x) / 32) * 32 - newX;
                }
            }
            else if (!o.gid && !o.text && !o.point)
            {
                assert(!o.rotation, 'Rotated rectangle cannot be stretched');
                o.width = transform((oldX + o.width) / 32) * 32 - newX;
            }

            o.x = newX;
        }
    }
}

for (const l of map.layers)
{
    layer(l);
}

fs.writeFileSync(file, JSON.stringify(map, null, 2) + '\n');
fs.writeFileSync(path.join(dir, 'LaboratoryCombinedCourse.spacing-extension.json'), JSON.stringify({
    backup,
    insertions,
    oldGaps : [ 5, 4, 4, 3, 2 ],
    newGaps : [ 5, 6, 4, 3, 2 ],
    restWidthBefore : 5,
    restWidthAfter : 10,
    totalExtension : 7
},
                                                                                                   null, 2) +
                                                                                        '\n');
console.log({backup, totalExtension : 7});
