"""Build 1m terrain variants from the original FBX geometry (numpy and Pillow)."""
import struct,zlib,pathlib,math
import numpy as np
from PIL import Image,ImageDraw,ImageFont
ROOT=pathlib.Path(__file__).resolve().parents[1] / 'Data/Model/laboratory'
def parse(path):
 b=path.read_bytes(); pos=27
 assert b.startswith(b'Kaydara FBX Binary ') and struct.unpack_from('<I',b,23)[0] == 7400, 'Expected FBX 7.4'
 def prop():
  nonlocal pos
  t=chr(b[pos]);pos+=1
  if t in 'Y C I F D L'.split():
   fmt={'Y':'h','C':'?','I':'i','F':'f','D':'d','L':'q'}[t];v=struct.unpack_from('<'+fmt,b,pos)[0];pos+=struct.calcsize(fmt);return v
  if t in 'SR':
   n=struct.unpack_from('<I',b,pos)[0];pos+=4;v=b[pos:pos+n];pos+=n;return v.decode('utf8',errors='replace') if t=='S' else v
  n,enc,size=struct.unpack_from('<III',b,pos);pos+=12;v=b[pos:pos+size];pos+=size
  if enc:v=zlib.decompress(v)
  return np.frombuffer(v,dtype={'f':'<f4','d':'<f8','i':'<i4','l':'<i8','b':'u1','c':'u1'}[t]).copy()
 def node():
  nonlocal pos
  end,n,size,nl=struct.unpack_from('<IIIB',b,pos);pos+=13
  if end==0:return None
  name=b[pos:pos+nl].decode();pos+=nl;p=[prop() for _ in range(n)];ch=[]
  while pos<end:
   c=node()
   if c:ch.append(c)
   else:break
  pos=end;return (name,p,ch)
 nodes=[]
 while pos<len(b):
  n=node()
  if not n:break
  nodes.append(n)
 return nodes
def child(n,key):return next((x for x in n[2] if x[0]==key),None)
def props(n):
 p=child(n,'Properties70');return {x[1][0]:x[1][4:] for x in p[2]} if p else {}
def load(path):
 ns=parse(path);objs=next(n for n in ns if n[0]=='Objects')[2];con=next(n for n in ns if n[0]=='Connections')[2];links=[n[1] for n in con];byid={n[1][0]:n for n in objs};tri=[];colors=[]
 for g in objs:
  if g[0]!='Geometry':continue
  v=child(g,'Vertices')[1][0].reshape(-1,3).copy();idx=child(g,'PolygonVertexIndex')[1][0]
  mid=next(c[2] for c in links if c[1]==g[1][0]);model=byid[mid];p=props(model)
  v*=np.array(p.get('Lcl Scaling',[1,1,1]))
  for axis,ang in enumerate(p.get('Lcl Rotation',[0,0,0])):
   a=math.radians(ang);c,s=math.cos(a),math.sin(a);r=np.eye(3);i,j=[x for x in range(3) if x!=axis];r[i,i]=r[j,j]=c;r[i,j]=-s if axis!=1 else s;r[j,i]=-r[i,j];v=v@r.T
  v+=p.get('Lcl Translation',[0,0,0]);mats=[byid[c[1]] for c in links if c[2]==mid and c[1] in byid and byid[c[1]][0]=='Material'];mc=[props(m).get('DiffuseColor',[.65,.68,.7])[:3] for m in mats]
  layer=child(g,'LayerElementMaterial');mi=child(layer,'Materials')[1][0] if layer else [0];poly=[];pi=0
  for ix in idx:
   poly.append(int(ix if ix>=0 else -ix-1))
   if ix<0:
    col=mc[int(mi[min(pi,len(mi)-1)])] if mc else [.65,.68,.7]
    for k in range(1,len(poly)-1):tri.append(v[[poly[0],poly[k],poly[k+1]]]);colors.append(col)
    poly=[];pi+=1
 return np.array(tri),np.array(colors)

def box(lo,hi,color):
    v=np.array([[x,y,z] for x in [lo[0],hi[0]] for y in [lo[1],hi[1]] for z in [lo[2],hi[2]]])
    ids=[[0,1,3,2],[4,6,7,5],[0,4,5,1],[2,3,7,6],[0,2,6,4],[1,5,7,3]]
    t=np.array([v[[a,b,c]] for a,b,c,d in ids]+[v[[a,c,d]] for a,b,c,d in ids])
    return t,np.tile(color,(len(t),1))

def panel(source,front=False,width=.86):
    t,c=load(ROOT/source)
    if source.startswith('Walls/'):
        # Keep the three vent grilles, removing the full-height wall and its rails.
        keep=(t[:,:,1].min(1)>150)&(t[:,:,1].max(1)<290)&(np.abs(t[:,:,0]).max(1)<150)
        keep &= (c[:,0] > c[:,1]*1.5) | (c[:,0] < .05)
        t,c=t[keep],c[keep]
        t*=np.array([-1,1,-1])
        assert len(t)>20
    lo=t.min((0,1));hi=t.max((0,1));t-=(lo+hi)/2
    if source.startswith('Walls/'):
        t*=width/(hi[0]-lo[0]);t[:,:,2]+=-.5-t[:,:,2].min();return t,c
    t*=width/(hi[0]-lo[0])
    if front:
        t=t[:,:,[0,2,1]]*np.array([1,1,-1]);t[:,:,2]+=-.5-t[:,:,2].min()
    else:
        t[:,:,1]+=.5-t[:,:,1].max()
    return t,c

def original_block_shell():
    """Reuse the existing block's added sides/base, excluding its original floor tile."""
    import json
    source=json.loads((ROOT/'FloorTile_Basic.gltf').read_text())
    source_counts={}
    for node in source['nodes']:
        if 'mesh' not in node:continue
        for p in source['meshes'][node['mesh']]['primitives']:
            mat=p['material']
            source_counts[mat]=source_counts.get(mat,0)+source['accessors'][p['attributes']['POSITION']]['count']
    g=json.loads((ROOT/'Generated/LabBlock_1m.gltf').read_text())
    data=(ROOT/'Generated'/g['buffers'][0]['uri']).read_bytes()
    def accessor(index):
        a=g['accessors'][index];v=g['bufferViews'][a['bufferView']]
        dims={'SCALAR':1,'VEC3':3}[a['type']]
        return np.frombuffer(data,dtype={5125:'<u4',5126:'<f4'}[a['componentType']],
                             count=a['count']*dims,offset=v.get('byteOffset',0)+a.get('byteOffset',0)).reshape(-1,dims)
    parts=[]
    for p in g['meshes'][0]['primitives']:
        indices=accessor(p['indices']).reshape(-1,3)
        indices=indices[np.all(indices>=source_counts.get(p['material'],0),axis=1)]
        if not len(indices):continue
        triangles=accessor(p['attributes']['POSITION'])[indices].astype(float)
        color=g['materials'][p['material']]['pbrMetallicRoughness']['baseColorFactor'][:3]
        parts.append((triangles,np.tile(color,(len(triangles),1))))
    return parts

def build(kind):
    if kind==101:
        parts=original_block_shell()
        top,colors=panel('FloorTile_Basic2.fbx',width=1)
        # Rotate the top 90 degrees around Y so its long motif runs along stage X.
        top=top[:,:,[2,1,0]]*np.array([1,1,-1])
        parts.append((top,colors))
        return np.concatenate([p[0] for p in parts]),np.concatenate([p[1] for p in parts])
    parts=[box([-.5,-.5,-.46],[.5,.44,.5],[.24,.24,.24])]
    # Upper cap keeps all three collision envelopes visually filled.
    parts.append(box([-.5,.44,-.5],[.5,.445 if kind!=103 else .5,.5],[.40,.40,.40]))
    parts.append(box([-.5,-.5,-.5],[.5,-.44,.5],[.24,.24,.24]))
    if kind==102:
        parts.append(panel('RoofTile_Plate2.fbx',front=True))
        parts.append(panel('RoofTile_Plate2.fbx',width=.94))
    else:
        parts.append(box([-.46,-.41,-.461],[.46,.41,-.46],[.40,.40,.40]))
        parts.append(panel('Walls/Wall_5.fbx',width=.88))
    return np.concatenate([p[0] for p in parts]),np.concatenate([p[1] for p in parts])

def write_gltf(name,t,c):
    chunks=[];views=[];accessors=[];prims=[];materials=[];offset=0
    def acc(a,typ,integer=False):
        nonlocal offset
        a=np.asarray(a,dtype='<u4' if integer else '<f4');data=a.tobytes();views.append(dict(buffer=0,byteOffset=offset,byteLength=len(data),target=34963 if integer else 34962));offset+=len(data);chunks.append(data)
        item=dict(bufferView=len(views)-1,componentType=5125 if integer else 5126,count=len(a),type=typ)
        if typ=='VEC3':item.update(min=a.min(0).tolist(),max=a.max(0).tolist())
        accessors.append(item);return len(accessors)-1
    for color in np.unique(c,axis=0):
        tri=t[np.all(c==color,axis=1)];normal=np.cross(tri[:,1]-tri[:,0],tri[:,2]-tri[:,0]);length=np.linalg.norm(normal,axis=1);tri=tri[length>1e-10];normal=normal[length>1e-10]/length[length>1e-10,None]
        n=np.repeat(normal,3,axis=0);p=tri.reshape(-1,3);ref=np.tile([0,1,0],(len(n),1));ref[np.abs(n[:,1])>.9]=[1,0,0];tan=np.cross(ref,n);tan/=np.linalg.norm(tan,axis=1)[:,None];tan=np.c_[tan,np.ones(len(n))]
        materials.append(dict(name=f'Material{len(materials)}',pbrMetallicRoughness=dict(baseColorFactor=[*color.tolist(),1],metallicFactor=0,roughnessFactor=.8)))
        prims.append(dict(attributes=dict(POSITION=acc(p,'VEC3'),NORMAL=acc(n,'VEC3'),TANGENT=acc(tan,'VEC4')),indices=acc(np.arange(len(p)),'SCALAR',True),material=len(materials)-1,mode=4))
    g=dict(asset=dict(version='2.0',generator='build_laboratory_blocks.py'),scene=0,scenes=[dict(nodes=[0])],nodes=[dict(name=name,mesh=0)],meshes=[dict(primitives=prims)],materials=materials,buffers=[dict(uri=name+'.bin',byteLength=offset)],bufferViews=views,accessors=accessors)
    out=ROOT/'Generated';(out/(name+'.bin')).write_bytes(b''.join(chunks));(out/(name+'.gltf')).write_text(__import__('json').dumps(g,indent=2)+'\n')

# Orthographic preview with a depth buffer, facing the same -Z side as the game.
def render(t,c,size=256):
    eye=np.array([1.,.7,-2.4]);eye/=np.linalg.norm(eye);right=np.cross([0,1,0],eye);right/=np.linalg.norm(right);up=np.cross(eye,right);q=t@np.array([right,up,eye]).T
    q[:,:,:2]*=size*.61;q[:,:,0]+=size/2;q[:,:,1]=size/2-q[:,:,1]
    im=np.full((size,size,3),[25,33,45],dtype=np.uint8);zb=np.full((size,size),-np.inf);light=np.array([-.3,.8,-.8]);light/=np.linalg.norm(light)
    for tri,col,orig in zip(q,c,t):
        normal=np.cross(orig[1]-orig[0],orig[2]-orig[0]);normal/=max(np.linalg.norm(normal),1e-9)
        if normal@eye<=0:continue
        linear=np.clip(col*(.45+.55*max(0,normal@light)),0,1);rgb=np.clip((np.where(linear<=.0031308,linear*12.92,1.055*linear**(1/2.4)-.055))*255,0,255).astype('uint8')
        x0,y0=np.maximum(np.floor(tri[:,:2].min(0)).astype(int),[0,0]);x1,y1=np.minimum(np.ceil(tri[:,:2].max(0)).astype(int),[size-1,size-1])
        if x1<x0 or y1<y0:continue
        X,Y=np.meshgrid(np.arange(x0,x1+1)+.5,np.arange(y0,y1+1)+.5);a,b,d=tri;den=(b[1]-d[1])*(a[0]-d[0])+(d[0]-b[0])*(a[1]-d[1])
        if abs(den)<1e-8:continue
        w=((b[1]-d[1])*(X-d[0])+(d[0]-b[0])*(Y-d[1]))/den;u=((d[1]-a[1])*(X-d[0])+(a[0]-d[0])*(Y-d[1]))/den;z=w*a[2]+u*b[2]+(1-w-u)*d[2];mask=(w>=0)&(u>=0)&(w+u<=1)&(z>zb[y0:y1+1,x0:x1+1]);zb[y0:y1+1,x0:x1+1][mask]=z[mask];im[y0:y1+1,x0:x1+1][mask]=rgb
    return Image.fromarray(im)

def main():
    import json
    specs=[(101,'LabBlock_Orange_1m','Orange floor'),(102,'LabBlock_Panel_1m','Reinforced panel'),(103,'LabBlock_Vent_1m','Vent block')]
    atlas=Image.new('RGB',(96,32));preview=Image.new('RGB',(900,340),(25,33,45));draw=ImageDraw.Draw(preview);font=ImageFont.truetype('C:/Windows/Fonts/arial.ttf',19)
    for i,(tid,name,label) in enumerate(specs):
        t,c=build(tid);assert np.allclose(t.min((0,1)),[-.5]*3) and np.allclose(t.max((0,1)),[.5]*3)
        write_gltf(name,t,c);img=render(t,c,300);preview.paste(img,(300*i,0));draw.text((300*i+15,306),f'{tid}  {label}',fill='white',font=font);atlas.paste(img.resize((32,32),Image.Resampling.LANCZOS),(i*32,0))
    ts=dict(type='tileset',version='1.10',name='LaboratoryBlocks',tilewidth=32,tileheight=32,tilecount=3,columns=3,image='LaboratoryBlocks.png',imagewidth=96,imageheight=32,margin=0,spacing=0,tiles=[dict(id=i,properties=[dict(name='typeId',type='int',value=tid),dict(name='label',type='string',value=label)]) for i,(tid,name,label) in enumerate(specs)])
    for folder in ['Data/Stage/Tiled','Tools/StageConverter/Sample']:
        dest=ROOT.parents[2]/folder;dest.mkdir(parents=True,exist_ok=True);atlas.save(dest/'LaboratoryBlocks.png');(dest/'LaboratoryBlocks.tsj').write_text(json.dumps(ts,indent=2)+'\n')
    preview.save(ROOT/'Generated/block-variants-preview.png')
    print('Built blocks 101-103, Tiled palettes and preview.')
if __name__=='__main__':main()

