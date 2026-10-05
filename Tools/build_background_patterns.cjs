// Refresh built-ins while retaining patterns registered by the editor.
const fs=require('fs'),path=require('path');
process.chdir(path.resolve(__dirname,'..'));
const file='Data/Stage/Background.patterns.json';
const old=JSON.parse(fs.readFileSync(file,'utf8'));
const scene=JSON.parse(fs.readFileSync('Data/Stage/Background.stage.json','utf8'));
const layout=JSON.parse(fs.readFileSync('Data/Stage/LaboratoryWalls.generated.json','utf8'));
const patterns=[];
const object=(asset,position,scale)=>({asset,position,rotation:[0,0,0],scale});
function pattern(name,description,width,objects){patterns.push({name,description,width,userCreated:false,objects});}
// Same dimensions as the current scene. Single parts are also available in Sized object.
for(const [name,asset,scale,height,base] of [
 ['Wall panel 6 x 4m',0,[6,4,1],4,false],['Support column 6m',1,[1,2,1],6,true],
 ['Equipment shelf 5.2m',2,[5.2,.3,.65],.3,false],['Terminal 1.8m',3,[1.2,1.2,1.2],1.8,true],
 ['Specimen capsule 2.5m',4,[1.25,1.25,1.25],2.5,true],['Horizontal pipes 6m',5,[2,1.2,1],.3798,false],
 ['Vent 1.35m',6,[1.8,1.8,1],.7946,false],['Wall pipes 3m',7,[1.25,1.5,1],3,false],
 ['Cargo crate 1.2m',9,[1.6,1.6,1.6],1.2,true],['Service vessel 2.8m',10,[1.4,1.4,1.4],2.8,true]])
 pattern(name,'Current laboratory size; place from bottom center.',asset===0?6:asset===2?5.2:asset===5?6:2,
  [object(asset,[0,base?0:height/2,0],scale)]);
for(const [kind,name] of [[0,'Specimen laboratory bay'],[1,'Power service bay'],[2,'Cargo storage bay']]){
 const bay=layout.bays.find(b=>b.kind===kind);if(!bay)throw Error('Missing bay');
 const parts=scene.objects.filter(o=>o.asset!==0&&o.asset!==1&&Math.abs(o.position[0]-bay.x)<3&&o.position[1]>=bay.y&&o.position[1]<=bay.y+5)
  .map(o=>({...o,position:[o.position[0]-bay.x,o.position[1]-bay.y,o.position[2]-14]}));
 for(const o of parts)delete o.id;
 pattern(name,'6m-wide wall and current-size equipment. Base Z = back wall.',6,
  [object(0,[0,2,0],[6,4,1]),object(0,[0,6,0],[6,4,1]),object(1,[-3,0,-.35],[1,2,1]),...parts]);
}
pattern('Security corridor bay','Wall, terminal, vent and shelf; 3m passage height.',6,[
 object(0,[0,1.5,0],[6,3,1]),object(6,[0,2.1,-.8],[1.8,1.8,1]),
 object(3,[-1,.7,-1.1],[1.15,1.15,1.15]),object(2,[-1,.55,-.95],[1.4,.3,.6])]);
pattern('Boss reactor bank','One side of the symmetric boss-room service equipment.',6,[
 object(0,[0,2,0],[6,4,1]),object(0,[0,6,0],[6,4,1]),object(7,[0,4,-.8],[2.5,3,1]),
 object(6,[0,7.5,-.85],[2.5,2.5,1]),object(10,[0,.7,-1.05],[1.8,1.8,1.8]),object(2,[0,.55,-.95],[2.4,.3,.7])]);
pattern('Observation window 12m','Open center for the city background; modular wall surround.',12,[
 object(0,[-3,2,0],[6,4,1]),object(0,[3,2,0],[6,4,1]),
 object(0,[-3,10,0],[6,4,1]),object(0,[3,10,0],[6,4,1]),
 object(1,[-6,0,-.35],[1,4,1]),object(1,[6,0,-.35],[1,4,1])]);
const users=old.patterns.filter(p=>p.userCreated);
for(const p of users){if(patterns.some(b=>b.name===p.name))throw Error('Built-in/user pattern name collision');patterns.push(p);}
fs.copyFileSync(file,file+'.before-refresh.bak');
fs.writeFileSync(file,JSON.stringify({formatVersion:1,patterns},null,2)+'\n');
console.log(`${patterns.length-users.length} current-size presets / ${users.length} user patterns retained`);
