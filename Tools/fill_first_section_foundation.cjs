const fs = require('fs');
const path = require('path');
const assert = require('assert');
const root = path.resolve(__dirname, '..');
const file = path.join(root, 'Data/Stage/LaboratoryCombinedCourse.tmj');
const map = JSON.parse(fs.readFileSync(file, 'utf8'));
const terrain = map.layers.find(l => l.name === 'Terrain');
assert(terrain?.chunks);
const added = [];

for (let x = -2; x <= 71; ++x)
{
    const column = [];

    for (const c of terrain.chunks)
    {
        if (x >= c.x && x < c.x + c.width)
        {
            for (let row = 0; row < c.height; ++row)
            {
                if (c.data[row * c.width + x - c.x])
                {
                    column.push(-(c.y + row));
                }
            }
        }
    }

    const bottom = Math.min(...column);
    assert(Number.isFinite(bottom) && bottom <= -2, `Missing floor at ${x}`);

    for (let gy = bottom - 1; gy >= -12; --gy)
    {
        const ty = -gy, cx = Math.floor(x / 16) * 16, cy = Math.floor(ty / 16) * 16;
        let c = terrain.chunks.find(c => c.x === cx && c.y === cy);

        if (!c)
        {
            c = {x : cx, y : cy, width : 16, height : 16, data : Array(256).fill(0)};
            terrain.chunks.push(c);
        }

        const index = (ty - cy) * c.width + x - cx;
        assert(c.data[index] === 0);
        c.data[index] = 1;
        added.push({x, y : gy});
    }
}

fs.writeFileSync(file, JSON.stringify(map, null, 2) + '\n');
console.log(`Added ${added.length} foundation blocks beneath First (down to Y=-12).`);
