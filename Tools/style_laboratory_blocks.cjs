const fs = require('fs'), path = require('path'), assert = require('assert'), crypto = require('crypto');
const {spawnSync} = require('child_process');
process.chdir(path.resolve(__dirname, '..'));
const file = 'Data/Stage/LaboratoryCombinedCourse.tmj';
const output = 'Data/Stage/LaboratoryCombinedCourse.stage.json';
const map = JSON.parse(fs.readFileSync(file));
const before = JSON.parse(fs.readFileSync(output));
const backup = 'obj/LaboratoryBlockStyling';
fs.mkdirSync(backup, {recursive: true});
for (const f of [file, output, 'Data/Stage/ConvertedStage.stage.json'])
    fs.copyFileSync(f, path.join(backup, path.basename(f)));
const terrain = map.layers.find(l => l.name === 'Terrain');
assert(terrain && terrain.visible !== false);
const source = 'Tiled/LaboratoryBlocks.tsj';
let palette = map.tilesets.find(t => t.source === source);
if (!palette) {
    // The combined map already uses GID 14 for Walker; do not renumber it.
    palette = {firstgid: 15, source};
    assert(map.tilesets.every(t => t.firstgid < 15), 'Palette GID range needs review');
    map.tilesets.push(palette);
}
const cells = new Map();
for (const c of terrain.chunks) for (let i=0; i<c.data.length; i++) {
    if (!c.data[i]) continue;
    assert([1,13,palette.firstgid,palette.firstgid+1,palette.firstgid+2].includes(c.data[i]),
           'Unexpected terrain tile');
    const x=c.x+i%c.width, y=-(c.y+Math.floor(i/c.width));
    assert(!cells.has(`${x},${y}`)); cells.set(`${x},${y}`, {c,i,x,y});
}
const has=(x,y)=>cells.has(`${x},${y}`);
const mod=(v,n)=>(v%n+n)%n;
const counts={100:0,101:0,102:0,103:0};
for (const {c,i,x,y} of cells.values()) {
    const top=!has(x,y+1), side=!has(x-1,y)||!has(x+1,y);
    let id=100;
    // Walkable edges read as a continuous orange strip; vertical edges use reinforcement.
    if (top && (has(x-1,y)||has(x+1,y))) id=101;
    else if (side) id=102;
    // Vent pairs form occasional service bays inside solid walls/foundations.
    else if (mod(x,12)>=5 && mod(x,12)<=6 && mod(y,6)===2 && has(x,y-1)) id=103;
    // Paired reinforcement columns break up large masses while leaving quiet grey areas.
    else if (mod(x,8)<=1 && mod(y,6)<=3) id=102;
    c.data[i]=id===100?1:palette.firstgid+id-101;
    counts[id]++;
}
const converter=path.resolve('bin/x64/Release/StageConverter.exe');
const tempMap=structuredClone(map);
for (const t of tempMap.tilesets) t.source=path.resolve('Data/Stage',t.source).replaceAll('\\','/');
fs.writeFileSync(`${backup}/styled.tmj`,JSON.stringify(tempMap));
const result=spawnSync(converter,['--input',`${backup}/styled.tmj`,'--output',`${backup}/styled.stage.json`,'--pixels-per-unit','32']);
assert.ifError(result.error);assert.equal(result.status,0,String(result.stderr));
const after=JSON.parse(fs.readFileSync(`${backup}/styled.stage.json`));
const geometry=s=>s.objects.map(o=>({...o,typeId:[100,101,102,103].includes(o.typeId)?100:o.typeId}));
assert.deepStrictEqual(geometry(after),geometry(before),'Terrain layout must stay identical');
assert.deepStrictEqual({...after,objects:[]},{...before,objects:[]},'Enemies and zones must stay identical');
const geometryHash=s=>crypto.createHash('sha256').update(JSON.stringify(geometry(s))).digest('hex');
fs.writeFileSync(file,JSON.stringify(map,null,2)+'\n');
fs.writeFileSync(output,JSON.stringify(after,null,2)+'\n');
fs.writeFileSync('Data/Stage/LaboratoryCombinedCourse.block-style.json',JSON.stringify({
    description:'Orange walkable edges, reinforced vertical edges and supports, paired service vents.',
    counts,terrainCells:cells.size,geometryHash:geometryHash(after),
    preserved:'All object positions, rotations, scales, gates, enemies and zones.'
},null,2)+'\n');
console.log({counts,unchangedTerrainCells:cells.size,geometryHash:geometryHash(after)});
