const fs=require('fs'),assert=require('assert'),crypto=require('crypto');
const {spawnSync}=require('child_process');
const pairs=[['LaboratoryCombinedCourse','LaboratoryCombinedCourse'],['LaboratoryCourse','ConvertedStage']];
for(const [map,json] of pairs){
 const out=`obj/${map}.sync-test.stage.json`;
 const r=spawnSync('bin/x64/Release/StageConverter.exe',['--input',`Data/Stage/${map}.tmj`,'--output',out,'--pixels-per-unit','32']);
 assert.ifError(r.error);assert.equal(r.status,0,String(r.stderr));
 assert.deepStrictEqual(JSON.parse(fs.readFileSync(out)),JSON.parse(fs.readFileSync(`Data/Stage/${json}.stage.json`)));
}
const stage=JSON.parse(fs.readFileSync('Data/Stage/LaboratoryCombinedCourse.stage.json'));
const report=JSON.parse(fs.readFileSync('Data/Stage/LaboratoryCombinedCourse.block-style.json'));
const geometry=stage.objects.map(o=>({...o,typeId:[100,101,102,103].includes(o.typeId)?100:o.typeId}));
assert.equal(crypto.createHash('sha256').update(JSON.stringify(geometry)).digest('hex'),report.geometryHash);
for(const [id,count] of Object.entries(report.counts))assert.equal(stage.objects.filter(o=>o.typeId===Number(id)).length,count);
assert.equal(stage.objects.filter(o=>[301,302].includes(o.typeId)).length,6);
assert.equal(stage.enemies.length,18);
// The normal combined conversion must never install into the legacy map's output.
const command=fs.readFileSync('Data/Stage/ConvertLaboratoryCombined.cmd','utf8');
assert(!command.includes('ConvertedStage.stage.json'));
console.log('PASS: active and legacy maps match their JSON; 3,121 terrain cells, 6 gates and 18 enemies preserved.');
