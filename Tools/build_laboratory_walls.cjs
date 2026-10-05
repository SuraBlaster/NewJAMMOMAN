// Re-runnable wall and equipment layout. --replace-existing clears the old layout.
const fs=require('fs'),path=require('path');
process.chdir(path.resolve(__dirname,'..'));
const file='Data/Stage/Background.stage.json',manifest='Data/Stage/LaboratoryWalls.generated.json';
const scene=JSON.parse(fs.readFileSync(file,'utf8'));
const oldIds=new Set(fs.existsSync(manifest)?JSON.parse(fs.readFileSync(manifest,'utf8')).ids:[]);
scene.objects=process.argv.includes('--replace-existing')?[]:scene.objects.filter(o=>!oldIds.has(o.id));
const authored=scene.objects.length,ids=[];
let id=Math.max(0,...scene.objects.map(o=>o.id))+1;
function add(asset,x,y,z,sx=1,sy=1,sz=1){ids.push(id);scene.objects.push({id:id++,asset,position:[x,y,z],rotation:[0,0,0],scale:[sx,sy,sz]});}
// Broad low-poly wall panels sit behind wall-mounted equipment.
// Four intentional openings are reserved for the next distant-background pass.
const sections=[[-10,38,-4,32],[38,110,10,38],[110,146,2,38],[146,182,2,42],[182,248,22,46]];
const windows=[[8,20,4,8],[62,74,26,30],[122,134,10,14],[188,200,34,38]];
for(const [left,right,bottom,top] of sections) {
 for(let x=left;x<right;x+=6)for(let y=bottom;y<top;y+=4){
  const w=Math.min(6,right-x),h=Math.min(4,top-y);
  if(windows.some(([a,b,c,d])=>x<b&&x+w>a&&y<d&&y+h>c))continue;
  add(0,x+w/2,y+h/2,14,w,h,1);
 }
 // Vertical supports every twelve metres, using the same modular kit.
 for(let x=left;x<=right;x+=12)for(let y=bottom;y<top;y+=6)
  add(1,x,y,13.65,1,Math.min(6,top-y)/3,1);
}
// Extra header panels and a service pipe give the restored room a lab silhouette.
for(let x=210;x<240;x+=6)add(5,x+3,41.3,13.3,2,1,1);
const decorationStart=ids.length;
const bays=[];
// Equipment follows the playable floors instead of filling unused wall areas.
// Shelves are background props, with no collision or change to the boss arena.
function bay(x,y,kind){
 if(windows.some(([a,b,c,d])=>x+3>a&&x-3<b&&y+5>c&&y<d))return;
 bays.push({x,y,kind});
 add(5,x,y+4.2,13.25,2,1.2,1);
 add(6,x+2.2,y+3.1,13.25,1.8,1.8,1);
 add(7,x-2.3,y+2.1,13.2,1.25,1.5,1);
 // A shallow shelf supports the machinery and visually ties it to the wall.
 add(2,x,y+0.15,13.05,5.2,0.3,0.65);
 if(kind===0){
  add(4,x-1.1,y+0.3,12.95,1.25,1.25,1.25);
  add(3,x+1.2,y+0.3,12.9,1.2,1.2,1.2);
 }else if(kind===1){
  add(10,x-1.15,y+0.3,12.9,1.4,1.4,1.4);
  add(10,x,y+0.3,12.9,1.15,1.15,1.15);
  add(3,x+1.35,y+0.3,12.9,1.2,1.2,1.2);
 }else{
  add(9,x-1.3,y+0.3,12.9,1.6,1.6,1.6);
  add(9,x,y+0.3,12.9,1.3,1.3,1.3);
  add(3,x+1.5,y+0.3,12.9,1.2,1.2,1.2);
 }
}
const route=JSON.parse(fs.readFileSync('Data/Stage/LaboratoryCourse.route.json','utf8'));
for(const [index,deck] of route.decks.entries()){
 if(deck.id==='boss'||deck.id==='antechamber')continue;
 const width=deck.b-deck.a;
 if(width<12)bay((deck.a+deck.b)/2,deck.h+0.5,index%3);
 else for(let x=deck.a+5;x<deck.b-2;x+=14)bay(x,deck.h+0.5,index%3);
}
// Tall service pipes and vents lead the eye up the climbing shafts.
for(const shaft of route.shafts){
 for(let y=shaft.bottom+4;y<shaft.top;y+=6){
  add(7,shaft.left-1.5,y,13.2,1.4,2,1);
  add(6,shaft.right+1.5,y+1,13.2,2,2,1);
 }
}
// Security corridor: compact terminals and vents rather than specimen tanks.
for(const x of [212,218,224]){
 add(6,x,35.6,13.2,1.8,1.8,1);
 add(3,x-1,34.2,12.9,1.15,1.15,1.15);
 add(2,x-1,34.05,13.05,1.4,0.3,0.6);
}
// Boss backdrop remains flat and uncluttered; symmetric reactor/service banks.
for(const x of [228,237]){
 add(7,x,37.8,13.2,2.5,3,1);
 add(6,x,41.3,13.15,2.5,2.5,1);
 add(10,x,34.5,12.95,1.8,1.8,1.8);
 add(2,x,34.35,13.05,2.4,0.3,0.7);
}
fs.copyFileSync(file,file.replace('.stage.json',`.before-walls-${Date.now()}.stage.json`));
fs.writeFileSync(file,JSON.stringify(scene,null,2)+'\n');
fs.writeFileSync(manifest,JSON.stringify({ids,windows,bays,decorationObjects:ids.length-decorationStart,authoredObjects:authored},null,2)+'\n');
console.log(`Walls/equipment: ${ids.length} generated, ${ids.length-decorationStart} decorations / ${authored} authored / ${scene.objects.length} total`);
