const fs = require('fs');
const path = require('path');
const zlib = require('zlib');
const root = path.resolve(__dirname, '..');
const width = 64, height = 32;
const pixels = Buffer.alloc(height * (1 + width * 4));

for (let y = 0; y < height; ++y)
{
    for (let x = 0; x < width; ++x)
    {
        const localX = x % 32;
        const border = localX < 3 || localX >= 29 || y < 3 || y >= 29;
        let color = border ? [ 37, 55, 67 ] : (y % 6 === 0 ? [ 31, 84, 102 ] : [ 66, 151, 170 ]);
        const arrowX = x < 32 ? localX : 31 - localX;

        if ((arrowX >= 8 && arrowX <= 21 && y >= 14 && y <= 17) ||
            (arrowX >= 17 && arrowX <= 25 && Math.abs(y - 15.5) <= 25 - arrowX))
        {
            color = [ 255, 214, 101 ];
        }

        const offset = y * (1 + width * 4) + 1 + x * 4;
        pixels.set([...color, 255 ], offset);
    }
}

function crc32(buffer)
{
    let crc = 0xffffffff;

    for (const byte of buffer)
    {
        crc ^= byte;

        for (let bit = 0; bit < 8; ++bit)
        {
            crc = (crc >>> 1) ^ (crc & 1 ? 0xedb88320 : 0);
        }
    }

    return (crc ^ 0xffffffff) >>> 0;
}

function chunk(type, data)
{
    const payload = Buffer.concat([ Buffer.from(type), data ]);
    const length = Buffer.alloc(4), checksum = Buffer.alloc(4);
    length.writeUInt32BE(data.length);
    checksum.writeUInt32BE(crc32(payload));
    return Buffer.concat([ length, payload, checksum ]);
}

const header = Buffer.alloc(13);
header.writeUInt32BE(width, 0);
header.writeUInt32BE(height, 4);
header[8] = 8;
header[9] = 6;
const png = Buffer.concat([
    Buffer.from([ 137, 80, 78, 71, 13, 10, 26, 10 ]), chunk('IHDR', header),
    chunk('IDAT', zlib.deflateSync(pixels)), chunk('IEND', Buffer.alloc(0))
]);
const tileset = {
    columns : 2,
    image : 'GateTileset.png',
    imageheight : 32,
    imagewidth : 64,
    margin : 0,
    name : 'OneWayGates',
    spacing : 0,
    tilecount : 2,
    tileheight : 32,
    tilewidth : 32,
    type : 'tileset',
    version : '1.10',
    tiles : [
        {id : 0, type : 'OneWayGateRight', properties : [ {name : 'typeId', type : 'int', value : 301} ]},
        {id : 1, type : 'OneWayGateLeft', properties : [ {name : 'typeId', type : 'int', value : 302} ]}
    ]
};

for (const folder of ['Data/Stage/Tiled', 'Tools/StageConverter/Sample'])
{
    fs.writeFileSync(path.join(root, folder, 'GateTileset.png'), png);
    fs.writeFileSync(path.join(root, folder, 'GateTileset.tsj'), JSON.stringify(tileset, null, 2) + '\n');
}

console.log('Created right/left gate tiles: 32 x 32 pixels each.');
