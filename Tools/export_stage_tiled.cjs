// StageConverter-compatible editable Tiled map. No replacement converter logic.
const fs=require('fs'),path=require('path'),assert=require('assert');
function exportStage(input,output) {
 const stage=JSON.parse(fs.readFileSync(input,'utf8'));
 const dir=path.dirname(output),resources=path.join(dir,'Tiled');
 fs.mkdirSync(resources,{recursive:true});
 for(const file of ['CubeBlockTileset.tsj','BlockTileset.png','EnemyPlacements.tsj','EnemyPlacements.png'])
  fs.copyFileSync(path.join(__dirname,'StageConverter/Sample',file),path.join(resources,file));
 let layerId=1,objectId=1;
 const properties=(name,type,value)=>({name,type,value});
 const layers=[];
 const ranges=[['01_Switchback',-Infinity,35],['02_UpperAndLowerRoutes',35,105],
  ['03_AlternatingDescent',105,146],['04_ReactorClimb',146,177],
  ['05_FinalBridges',177,210],['06_BossApproach',210,225],['07_BossArena',225,Infinity]];
 for(const [name,min,max] of ranges) {
  const chunks=new Map();
  for(const o of stage.objects.filter(o=>o.position.x>=min&&o.position.x<max)) {
   assert(o.typeId===100&&o.rotationDegrees===0&&o.scale.x===1&&o.scale.y===1,'Tile export requires unit blocks');
   const x=o.position.x,y=-o.position.y;assert(Number.isInteger(x)&&Number.isInteger(y));
   const cx=Math.floor(x/16)*16,cy=Math.floor(y/16)*16,key=`${cx},${cy}`;
   if(!chunks.has(key))chunks.set(key,{x:cx,y:cy,width:16,height:16,data:Array(256).fill(0)});
   const c=chunks.get(key),index=(y-cy)*16+x-cx;assert(!c.data[index],'Duplicate cell');c.data[index]=1;
  }
  const cells=[...chunks.values()];
  const startx=cells.length?Math.min(...cells.map(c=>c.x)):0;
  const starty=cells.length?Math.min(...cells.map(c=>c.y)):0;
  const width=cells.length?Math.max(...cells.map(c=>c.x+c.width))-startx:0;
  const height=cells.length?Math.max(...cells.map(c=>c.y+c.height))-starty:0;
  layers.push({id:layerId++,name,type:'tilelayer',visible:true,opacity:1,x:0,y:0,width,height,
   chunks:cells,startx,starty});
 }
 const objectLayer=(name,objects,color)=>layers.push({id:layerId++,name,type:'objectgroup',visible:true,opacity:1,x:0,y:0,draworder:'topdown',color,objects});
 const enemyNames=['Wave','Scatter','Mage','Fan','Hover','Fly','Egg','Boss'];
 objectLayer('Enemies',stage.enemies.map(e=>({id:objectId++,name:e.enemyType,gid:2+enemyNames.indexOf(e.enemyType),
  x:e.position.x*32,y:-e.position.y*32,width:96,height:64,rotation:0,visible:true,
  properties:[properties('direction','string',e.direction)]})),'#ff6677');
 for(const [name,key,color] of [['FallRespawnZones','fallRespawnZones','#ff3333'],
  ['BossApproachCameraZone','bossApproachCameraZones','#ffcc00'],['BossEncounterZone','bossEncounterZones','#00a8ff'],['CameraBounds','cameraBounds','#55dd88']])
  objectLayer(name,(stage[key]||[]).map(z=>({id:objectId++,name,x:z.minX*32,y:-z.maxY*32,
   width:(z.maxX-z.minX)*32,height:(z.maxY-z.minY)*32,rotation:0,visible:true})),color);
 const map={type:'map',version:'1.10',tiledversion:'1.10.2',orientation:'orthogonal',renderorder:'right-down',
  infinite:true,width:0,height:0,tilewidth:32,tileheight:32,compressionlevel:-1,
  backgroundcolor:'#101822',nextlayerid:layerId,nextobjectid:objectId,
  properties:[properties('stageName','string',stage.stageName)],
  tilesets:[{firstgid:1,source:'Tiled/CubeBlockTileset.tsj'},{firstgid:2,source:'Tiled/EnemyPlacements.tsj'}],layers};
 fs.writeFileSync(output,JSON.stringify(map,null,2)+'\n');
 console.log(`Tiled: ${output} / ${stage.objects.length} terrain tiles / ${stage.enemies.length} enemies`);
}
module.exports={exportStage};
if(require.main===module)exportStage(process.argv[2]||'Data/Stage/ConvertedStage.stage.json',process.argv[3]||'Data/Stage/LaboratoryCourse.tmj');
