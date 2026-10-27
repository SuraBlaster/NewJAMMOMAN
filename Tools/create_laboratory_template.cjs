const fs = require('fs');
const path = require('path');
const root = path.resolve(__dirname, '..');
const source = path.join(root, 'Data/Stage/LaboratoryCourse.tmj');
const output = path.join(root, 'Data/Stage/LaboratoryNewStage.tmj');

if (fs.existsSync(output))
{
    throw new Error('Template already exists; refusing to overwrite edits.');
}

const map = JSON.parse(fs.readFileSync(source, 'utf8'));
let id = 1;

function tiles(name, floor = false)
{
    const chunks = [];

    if (floor)
    {
        for (const x of [-16, 0])
        {
            const data = Array(256).fill(0);

            for (let column = 0; column < 16; column++)
            {
                if (x + column >= -4 && x + column <= 15)
                {
                    data[16 + column] = 1;
                }
            }

            chunks.push({x, y : 0, width : 16, height : 16, data});
        }
    }

    return {
        id : id++,
        name,
        type : 'tilelayer',
        visible : true,
        locked : false,
        opacity : 1,
        x : 0,
        y : 0,
        startx : floor ? -16 : 0,
        starty : 0,
        width : floor ? 32 : 0,
        height : floor ? 16 : 0,
        chunks
    };
}

function objects(name, color)
{
    return {
        id : id++,
        name,
        type : 'objectgroup',
        visible : true,
        locked : false,
        opacity : 1,
        x : 0,
        y : 0,
        draworder : 'topdown',
        color,
        objects : []
    };
}

map.infinite = true;
map.width = map.height = 0;
map.layers = [
    tiles('Terrain', true), tiles('Gates'), objects('Enemies', '#ff6677'),
    objects('FallRespawnZones', '#ff3333'), objects('BossApproachCameraZone', '#ffcc00'),
    objects('BossEncounterZone', '#00a8ff'), objects('CameraBounds', '#55dd88')
];
map.properties = [ {name : 'stageName', type : 'string', value : 'Laboratory - New Stage'} ];
map.nextlayerid = id;
map.nextobjectid = 1;
fs.writeFileSync(output, JSON.stringify(map, null, 2) + '\n');
console.log(output);
