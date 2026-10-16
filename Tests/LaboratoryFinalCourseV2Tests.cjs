const fs = require('fs'), assert = require('assert'), path = require('path');
process.chdir(path.resolve(__dirname, '..'));
const map = JSON.parse(fs.readFileSync('Data/Stage/LaboratoryFinalCourseV2.tmj'));
const stage = JSON.parse(fs.readFileSync('Data/Stage/LaboratoryFinalCourseV2.stage.json'));
const cells = new Set(stage.objects.map(o => `${o.position.x},${o.position.y}`));
const has = (x, y) => cells.has(`${x},${y}`);
assert.equal(cells.size, stage.objects.length);
assert(map.infinite && map.layers.every(l => !l.locked));

for (const tileset of map.tilesets)
{
    assert(fs.existsSync(path.resolve('Data/Stage', tileset.source)));
}

assert.equal(stage.enemies.length, 0);
assert.equal(stage.fallRespawnZones.length, 1);

function reachable(start, target, minX, maxX, maxY)
{
    const queue = [ start ], seen = new Set([ start.join(',') ]);

    for (let i = 0; i < queue.length; i++)
    {
        const [x, y] = queue[i];

        if (x === target[0] && y === target[1])
        {
            return true;
        }

        for (const [nx, ny] of [[ x + 1, y ], [ x - 1, y ], [ x, y + 1 ], [ x, y - 1 ]])
        {
            const key = `${nx},${ny}`;

            if (nx < minX || nx > maxX || ny <= -10 || ny > maxY || has(nx, ny) || seen.has(key))
            {
                continue;
            }

            seen.add(key);
            queue.push([ nx, ny ]);
        }
    }

    return false;
}

assert(!reachable([ 35, 3 ], [ 51, -3 ], -8, 59, 15), 'Can skip the first climb by dropping');
assert(reachable([ 35, 3 ], [ 51, -3 ], -8, 59, 18), 'No open route over the first summit');
assert(!reachable([ 55, -3 ], [ 76, 9 ], 47, 85, 14), 'Can pass through/below the final notch');
assert(reachable([ 55, -3 ], [ 76, 9 ], 47, 85, 17), 'No open route over the final summit');

for (let x = 48; x <= 57; x++)
{
    assert(has(x, -5));

    for (let y = -4; y <= 3; y++)
    {
        assert(!has(x, y));
    }
}

for (let x = 62; x <= 64; x++)
{
    for (let y = 6; y <= 7; y++)
    {
        assert(!has(x, y));
    }
}

for (let x = 65; x <= 67; x++)
{
    for (let y = 6; y <= 7; y++)
    {
        assert(has(x, y));
    }
}

console.log(`PASS: ${
    cells
        .size} blocks; drop/underpass shortcuts blocked; summit paths open; landing and notch clear. Physics and wind still require playtesting.`);
