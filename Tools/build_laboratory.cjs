// Run from the repository root: node Tools/build_laboratory.cjs
// Originals remain untouched. Generated assets use Y-up, unit transforms.
const fs = require('fs');
const path = require('path');
const assert = require('assert');
const root = 'Data/Model/laboratory';
const out = root + '/Generated';
fs.mkdirSync(out, {recursive:true});
const norm = v => {const l=Math.hypot(...v); assert(l>0); return v.map(x=>x/l);};
const cross = (a,b)=>[a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]];
function rotate(v,q) {
  const t=cross(q.slice(0,3),v).map(x=>2*x), u=cross(q.slice(0,3),t);
  return v.map((x,i)=>x+q[3]*t[i]+u[i]);
}
function bounds(prims) {
  const lo=[Infinity,Infinity,Infinity], hi=lo.map(x=>-x);
  for(const p of prims) for(const v of p.p) for(let i=0;i<3;i++){lo[i]=Math.min(lo[i],v[i]);hi[i]=Math.max(hi[i],v[i]);}
  return {min:lo,max:hi,size:hi.map((v,i)=>v-lo[i])};
}
function read(name) {
  const g=JSON.parse(fs.readFileSync(`${root}/${name}.gltf`));
  const buffers=g.buffers.map(b=>{const bytes=fs.readFileSync(path.join(path.dirname(`${root}/${name}.gltf`),b.uri));assert(bytes.length===b.byteLength);return bytes;});
  function accessor(index) {
    const a=g.accessors[index], v=g.bufferViews[a.bufferView], b=buffers[v.buffer];
    const dims={SCALAR:1,VEC3:3,VEC4:4}[a.type]; assert(dims);
    const bytes={5126:4,5125:4,5123:2}[a.componentType]; assert(bytes);
    return Array.from({length:a.count},(_,i)=>Array.from({length:dims},(_,j)=>{
      const o=(v.byteOffset||0)+(a.byteOffset||0)+i*(v.byteStride||dims*bytes)+j*bytes;
      return a.componentType===5126?b.readFloatLE(o):bytes===4?b.readUInt32LE(o):b.readUInt16LE(o);
    }));
  }
  const prims=[];
  for(const n of g.nodes) {
  assert(!n.matrix && !n.children, 'Unsupported hierarchical export');
  if(n.mesh===undefined)continue;
  const s=n.scale||[1,1,1], q=norm(n.rotation||[0,0,0,1]);
  for(const p of g.meshes[n.mesh].primitives) {
  if(!p.attributes || p.attributes.POSITION===undefined)continue;
  assert((p.mode??4)===4 && g.materials[p.material]);
  prims.push({
    mat:p.material,
    p:accessor(p.attributes.POSITION).map(v=>rotate(v.map((x,i)=>x*s[i]),q).map((x,i)=>x+(n.translation||[0,0,0])[i])),
    n:accessor(p.attributes.NORMAL).map(v=>norm(rotate(v.map((x,i)=>x/s[i]),q))),
    indices:accessor(p.indices).flat()
  });
  }
  }
  assert(prims.length);
  return {prims,materials:g.materials};
}
function box(prims,lo,hi,mat) {
  const p={p:[],n:[],indices:[],mat};
  for(let axis=0;axis<3;axis++) for(const sign of [-1,1]) {
    const normal=[0,0,0]; normal[axis]=sign;
    const u=(axis+1)%3,v=(axis+2)%3, start=p.p.length;
    for(const [a,b] of [[0,0],[1,0],[1,1],[0,1]]) {
      const pt=[0,0,0];pt[axis]=sign<0?lo[axis]:hi[axis];pt[u]=a?hi[u]:lo[u];pt[v]=b?hi[v]:lo[v];
      p.p.push(pt);p.n.push(normal);
    }
    p.indices.push(...(sign>0?[0,1,2,0,2,3]:[0,2,1,0,3,2]).map(i=>i+start));
  }
  prims.push(p);
}
function write(name,prims,materials) {
  // Merge by material: two draw calls instead of one for each side detail.
  const merged=materials.map((_,mat)=>({mat,p:[],n:[],indices:[]}));
  for(const p of prims){const m=merged[p.mat],off=m.p.length;m.p.push(...p.p);m.n.push(...p.n);m.indices.push(...p.indices.map(i=>i+off));}
  const g={asset:{version:'2.0',generator:'Laboratory grid adapter'},scene:0,scenes:[{nodes:[0]}],nodes:[{name,mesh:0}],meshes:[{primitives:[]}],materials,buffers:[],bufferViews:[],accessors:[]};
  const chunks=[];let offset=0;
  function acc(values,type,integer=false) {
    const flat=values.flat(),b=Buffer.alloc(flat.length*4);
    flat.forEach((x,i)=>{assert(Number.isFinite(x));integer?b.writeUInt32LE(x,i*4):b.writeFloatLE(x,i*4);});
    const view=g.bufferViews.push({buffer:0,byteOffset:offset,byteLength:b.length,target:integer?34963:34962})-1;
    offset+=b.length;chunks.push(b);
    const a={bufferView:view,componentType:integer?5125:5126,count:values.length,type};
    if(type==='VEC3') {a.min=[0,1,2].map(i=>Math.min(...values.map(v=>v[i])));a.max=[0,1,2].map(i=>Math.max(...values.map(v=>v[i])));}
    return g.accessors.push(a)-1;
  }
  for(const m of merged.filter(m=>m.p.length)) {
    assert(m.indices.every(i=>i<m.p.length));
    const tangents=m.n.map(n=>[...norm(cross(Math.abs(n[1])<0.9?[0,1,0]:[1,0,0],n)),1]);
    g.meshes[0].primitives.push({attributes:{POSITION:acc(m.p,'VEC3'),NORMAL:acc(m.n,'VEC3'),TANGENT:acc(tangents,'VEC4')},indices:acc(m.indices,'SCALAR',true),material:m.mat,mode:4});
  }
  g.buffers.push({uri:name+'.bin',byteLength:offset});
  fs.writeFileSync(`${out}/${name}.bin`,Buffer.concat(chunks));
  fs.writeFileSync(`${out}/${name}.gltf`,JSON.stringify(g,null,2)+'\n');
  return bounds(prims);
}
const floor=read('FloorTile_Basic'), before=bounds(floor.prims);
const scale=1/before.size[0];
assert(Math.abs(before.size[0]-before.size[2])<0.001);
for(const p of floor.prims) p.p=p.p.map(v=>[
  (v[0]-(before.min[0]+before.max[0])/2)*scale,
  (v[1]-before.max[1])*scale+0.5,
  (v[2]-(before.min[2]+before.max[2])/2)*scale]);
const bottom=bounds(floor.prims).min[1];
// Dark shell with inset-looking lighter side panels, entirely inside the unit box.
box(floor.prims,[-0.5,-0.42,-0.5],[0.5,bottom-0.04,0.5],1);
for(const side of [-1,1]) {
  const z=side*0.5;
  box(floor.prims,[-0.43,-0.40,z<0?z:0.485],[0.43,bottom-0.055,z<0?z+0.015:0.5],0);
  const x=side*0.5;
  box(floor.prims,[x<0?x:0.485,-0.40,-0.43],[x<0?x+0.015:0.5,bottom-0.055,0.43],0);
}
// Side panels are flush with shell; move shell inward on X/Z to avoid coplanar faces.
const shell=floor.prims.find(p=>p.p.length===24 && p.mat===1);
for(const v of shell.p){v[0]*=0.97;v[2]*=0.97;}
// Full perimeter bands close the silhouette at the top and bottom.
box(floor.prims,[-0.5,-0.5,-0.5],[0.5,-0.42,0.5],1);
box(floor.prims,[-0.5,bottom-0.04,-0.5],[0.5,bottom,0.5],1);
const blockBounds=write('LabBlock_1m',floor.prims,floor.materials);
assert(blockBounds.min.every(v=>Math.abs(v+0.5)<1e-5));
assert(blockBounds.max.every(v=>Math.abs(v-0.5)<1e-5));
const col=read('Column_2'), cb=bounds(col.prims), cs=3/cb.size[1];
for(const p of col.prims) p.p=p.p.map(v=>[(v[0]-(cb.min[0]+cb.max[0])/2)*cs,(v[1]-cb.min[1])*cs,(v[2]-(cb.min[2]+cb.max[2])/2)*cs]);
const columnBounds=write('LabColumn_3m',col.prims,col.materials);
const wall=[];
box(wall,[-0.5,-0.5,-0.06],[0.5,0.5,0.06],1);
box(wall,[-0.44,-0.44,-0.065],[0.44,0.44,-0.06],0);
box(wall,[-0.44,-0.44,0.06],[0.44,0.44,0.065],0);
const wallBounds=write('LabWall_1m',wall,floor.materials);
const report={sourceFloorBounds:before,floorScale:scale,blockBounds,columnBounds,wallBounds};
// Preserve the existing asset IDs 0..2 for saved backgrounds. Append new entries.
const catalog=[
  {name:'Wall (1 x 1)',file:'LabWall_1m',bounds:wallBounds},
  {name:'Column (height 3)',file:'LabColumn_3m',bounds:columnBounds},
  {name:'Block (1 x 1 x 1)',file:'LabBlock_1m',bounds:blockBounds}
];
for(const [source,label,axis,target,base] of [
  ['Props_Computer','Computer (height 1.5)',1,1.5,true],
  ['Props_Capsule','Capsule (height 2)',1,2,true],
  ['Pipes','Pipes (width 3)',0,3,false],
  ['Details/Details_Vent_1','Vent (width 0.75)',0,0.75,false],
  ['Details/Details_Pipes_Long','Wall pipes (height 2)',1,2,false],
  ['Door_Double','Double door (height 3)',1,3,true],
  ['Props_Crate','Crate (height 0.75)',1,0.75,true],
  ['Props_Vessel_Tall','Vessel (height 2)',1,2,true]
]) {
  const asset=read(source),b=bounds(asset.prims),factor=target/b.size[axis];
  for(const p of asset.prims)p.p=p.p.map(v=>v.map((x,i)=>(x-(i===1&&base?b.min[i]:(b.min[i]+b.max[i])/2))*factor));
  // Face the gameplay camera (-Z); the exported detail fronts face +Z.
  for(const p of asset.prims){p.p=p.p.map(v=>[-v[0],v[1],-v[2]]);p.n=p.n.map(v=>[-v[0],v[1],-v[2]]);}
  const file='Lab_'+path.basename(source);
  catalog.push({name:label,file,bounds:write(file,asset.prims,asset.materials)});
}
report.assets=catalog;
const vec=a=>'{'+a.map(x=>Number(x).toFixed(7)+'f').join(',')+'}';
fs.writeFileSync('Source/LaboratoryAssets.h',`// Generated by Tools/build_laboratory.cjs; IDs are append-only.\n#pragma once\n#include <DirectXCollision.h>\nnamespace LaboratoryAssets {\ninline const char* names[] = {${catalog.map(x=>JSON.stringify(x.name)).join(',')}};\ninline const char* paths[] = {${catalog.map(x=>JSON.stringify(out+'/'+x.file+'.gltf')).join(',')}};\ninline const DirectX::BoundingBox bounds[] = {\n${catalog.map(x=>'    {'+vec(x.bounds.min.map((v,i)=>(v+x.bounds.max[i])/2))+','+vec(x.bounds.size.map(v=>v/2))+'}').join(',\n')}\n};\ninline constexpr int Count = sizeof(names)/sizeof(names[0]);\n}\n`);
fs.writeFileSync(`${out}/dimensions.json`,JSON.stringify(report,null,2)+'\n');
console.log(JSON.stringify(report,null,2));
