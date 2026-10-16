const fs = require('fs');
const path = require('path');
const assert = require('assert/strict');

process.chdir(path.resolve(__dirname, '..'));

const stageFile = 'Data/Stage/LaboratoryCombinedCourse.stage.json';
const backgroundFile = 'Data/Stage/Background.stage.json';
const manifestFile = 'Data/Stage/LaboratoryWalls.generated.json';
const stage = JSON.parse(fs.readFileSync(stageFile));
const background = JSON.parse(fs.readFileSync(backgroundFile));
const previousManifest = JSON.parse(fs.readFileSync(manifestFile));
const generatedIds = new Set(previousManifest.ids);
const authored = background.objects.filter(object => !generatedIds.has(object.id));
const objects = [...authored];
const ids = [];
let nextId = Math.max(0, ...background.objects.map(object => object.id)) + 1;

function add(asset, x, y, z, sx = 1, sy = 1, sz = 1)
{
    ids.push(nextId);
    objects.push({
        id: nextId++,
        asset,
        position: [x, y, z],
        rotation: [0, 0, 0],
        scale: [sx, sy, sz]
    });
}

// Keep the existing wall depth and 6m x 4m panel proportions. Padding covers
// the larger view footprint on the background plane, 14m behind the course.
const cameraRegions = [
    ...stage.cameraBounds.filter(region => !region.isSectionTrigger),
    ...stage.bossApproachCameraZones,
    ...stage.bossEncounterZones
];
const regions = cameraRegions.map(region => ({
    minX: region.minX - 14,
    maxX: region.maxX + 14,
    minY: region.minY - 10,
    maxY: region.maxY + 10
}));

const windows = [
    [30, 36, 8, 12],
    [102, 108, 12, 16],
    [174, 180, 32, 36],
    [270, 276, 32, 36]
];

function intersects(left, right, bottom, top, region)
{
    return left < region.maxX && right > region.minX && bottom < region.maxY && top > region.minY;
}

function isWindow(left, right, bottom, top)
{
    return windows.some(([a, b, c, d]) => left < b && right > a && bottom < d && top > c);
}

const left = Math.floor(Math.min(...regions.map(region => region.minX)) / 6) * 6;
const right = Math.ceil(Math.max(...regions.map(region => region.maxX)) / 6) * 6;
const bottom = Math.floor(Math.min(...regions.map(region => region.minY)) / 4) * 4;
const top = Math.ceil(Math.max(...regions.map(region => region.maxY)) / 4) * 4;

for (let x = left; x < right; x += 6)
{
    for (let y = bottom; y < top; y += 4)
    {
        if (regions.some(region => intersects(x, x + 6, y, y + 4, region)) && !isWindow(x, x + 6, y, y + 4))
        {
            add(0, x + 3, y + 2, 14, 6, 4, 1);
        }
    }
}

for (let x = left + 6; x < right; x += 12)
{
    for (let y = bottom; y < top; y += 8)
    {
        if (regions.some(region => intersects(x - 0.3, x + 0.3, y, y + 8, region)) && !isWindow(x - 0.3, x + 0.3, y, y + 8))
        {
            add(1, x, y, 13.65, 1, 8 / 3, 1);
        }
    }
}

// Equipment shelves follow actual exposed block tops. Omit ceiling tops and
// small wall-kick steps so the background remains quiet around those actions.
const baySpecifications = [
    [5, -1, 0], [25, -1, 1], [40, -1, 2], [59, -1, 0], [67, -1, 1],
    [74, 8, 2], [87, 10, 0], [99, 8, 1], [116, 2, 2], [124, 15, 0],
    [134, 15, 1], [150, 20, 2], [164, 26, 0], [174, 28, 1],
    [180, 14, 2], [188, 15, 0], [196, 23, 1], [203, 26, 0], [207, 15, 2],
    [221, 21, 1], [224, 30, 2], [235, 24, 0], [244, 24, 1],
    [254, 24, 2], [260, 24, 2], [269, 24, 0], [275, 24, 1]
];
const terrain = new Set(stage.objects.filter(object => object.typeId === 100)
    .map(object => `${object.position.x},${object.position.y}`));
const bays = [];

for (const [x, floor, kind] of baySpecifications)
{
    assert(terrain.has(`${x},${floor}`) && !terrain.has(`${x},${floor + 1}`), `Missing floor at ${x},${floor}`);
    const y = floor + 0.5;
    bays.push({ x, y, kind });

    add(5, x, y + 4.2, 13.25, 2, 1.2, 1);
    add(6, x + 2.2, y + 3.1, 13.25, 1.8, 1.8, 1);
    add(7, x - 2.3, y + 2.1, 13.2, 1.25, 1.5, 1);
    add(2, x, y + 0.15, 13.05, 5.2, 0.3, 0.65);

    if (kind === 0)
    {
        add(4, x - 1.1, y + 0.3, 12.95, 1.25, 1.25, 1.25);
        add(3, x + 1.2, y + 0.3, 12.9, 1.2, 1.2, 1.2);
    }
    else if (kind === 1)
    {
        add(10, x - 1.15, y + 0.3, 12.9, 1.4, 1.4, 1.4);
        add(10, x, y + 0.3, 12.9, 1.15, 1.15, 1.15);
        add(3, x + 1.35, y + 0.3, 12.9, 1.2, 1.2, 1.2);
    }
    else
    {
        add(9, x - 1.3, y + 0.3, 12.9, 1.6, 1.6, 1.6);
        add(9, x, y + 0.3, 12.9, 1.3, 1.3, 1.3);
        add(3, x + 1.5, y + 0.3, 12.9, 1.2, 1.2, 1.2);
    }
}

const manifest = {
    source: stageFile,
    ids,
    windows,
    bays,
    regions,
    authoredObjects: authored.length,
    counts: Object.fromEntries(Array.from({ length: 11 }, (_, asset) => [asset, objects.filter(object => object.asset === asset).length]))
};

// Check the whole camera area, including the new corridor and boss room.
// Only the deliberately retained observation windows may expose the sky.
for (const region of cameraRegions)
{
    for (let x = Math.ceil(region.minX); x <= region.maxX; x++)
    {
        for (let y = Math.ceil(region.minY); y <= region.maxY; y++)
        {
            if (isWindow(x - 0.001, x + 0.001, y - 0.001, y + 0.001))
            {
                continue;
            }

            assert(objects.some(object => object.asset === 0 &&
                Math.abs(object.position[0] - x) <= object.scale[0] / 2 &&
                Math.abs(object.position[1] - y) <= object.scale[1] / 2), `Missing wall at ${x},${y}`);
        }
    }
}

console.log(JSON.stringify({ previous: background.objects.length, next: objects.length, counts: manifest.counts, bays: bays.length, authored: authored.length }));

if (!process.argv.includes('--apply'))
{
    console.log('Dry run. Use --apply to save.');
    process.exit(0);
}

const stamp = new Date().toISOString().replace(/[:.]/g, '-');
fs.copyFileSync(backgroundFile, `Data/Stage/Background.before-combined-${stamp}.stage.json`);
fs.copyFileSync(manifestFile, `Data/Stage/LaboratoryWalls.before-combined-${stamp}.generated.json`);
fs.writeFileSync(backgroundFile, JSON.stringify({ ...background, objects }, null, 2) + '\n');
fs.writeFileSync(manifestFile, JSON.stringify(manifest, null, 2) + '\n');
