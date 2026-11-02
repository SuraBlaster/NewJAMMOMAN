const fs=require('fs'),assert=require('assert');
const root='Data/Stage/',dir=root+'Tiled/';
const read=p=>JSON.parse(fs.readFileSync(p,'utf8'));
const mapPath=root+'LaboratoryCombinedCourse.tmj',map=read(mapPath);
fs.copyFileSync(mapPath,'tmp/tileset-cleanup/before.tmj');
const cube=read(dir+'CubeBlockTileset.tsj'),lab=read(dir+'LaboratoryBlocks.tsj');
const blocks={type:'tileset',version:'1.10',name:'Blocks',tilewidth:32,tileheight:32,tilecount:4,columns:0,grid:{orientation:'orthogonal',width:1,height:1},margin:0,spacing:0,tiles:[{...cube.tiles[0],id:0,image:cube.image,imagewidth:32,imageheight:32},...lab.tiles.map((t,i)=>({...t,id:i+1,image:`LaboratoryBlock${101+i}.png`,imagewidth:32,imageheight:32}))]};
const enemies=read(dir+'EnemyPlacements.tsj');enemies.name='Enemies';enemies.tiles.push({...read(dir+'WalkerPlacements.tsj').tiles[0],id:9});enemies.tilecount=10;
const remap=new Map([[1,1],[13,1],[15,2],[16,3],[17,4],[14,14],[11,15],[12,16],...Array.from({length:9},(_,i)=>[i+2,i+5])]);
function gid(v){const base=(v>>>0)&0x0fffffff,flags=(v>>>0)&0xf0000000;if(!base)return v;assert(remap.has(base),`Unknown GID ${v}`);return (flags|remap.get(base))>>>0;}
function layers(ls){for(const l of ls){if(l.data){assert(Array.isArray(l.data));l.data=l.data.map(gid);}for(const c of l.chunks||[])c.data=c.data.map(gid);for(const o of l.objects||[])if(o.gid!==undefined)o.gid=gid(o.gid);if(l.layers)layers(l.layers);}}
layers(map.layers);map.tilesets=[{firstgid:1,source:'Tiled/Blocks.tsj'},{firstgid:5,source:'Tiled/Enemies.tsj'},{firstgid:15,source:'Tiled/GateTileset.tsj'}];
for(const [p,v] of [[dir+'Blocks.tsj',blocks],[dir+'Enemies.tsj',enemies],[mapPath,map]])fs.writeFileSync(p,JSON.stringify(v,null,2)+'\n');
console.log('Consolidated: Blocks (4), Enemies (10), OneWayGates (2).');
