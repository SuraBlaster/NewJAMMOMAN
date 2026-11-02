const fs=require('fs'),assert=require('assert');
const p='Data/Stage/LaboratoryCombinedCourse.tmj';const expected=JSON.parse(fs.readFileSync(p));let s=fs.readFileSync('tmp/tileset-cleanup/before.tmj','utf8');
const m=new Map([[1,1],[13,1],[15,2],[16,3],[17,4],[14,14],[11,15],[12,16],...Array.from({length:9},(_,i)=>[i+2,i+5])]);
const gid=v=>{v=Number(v);const b=(v>>>0)&0x0fffffff;return b?(((v&0xf0000000)|m.get(b))>>>0):v};
s=s.replace(/("data"\s*:\s*\[)([\s\S]*?)(\])/g,(_,a,b,c)=>a+b.replace(/\d+/g,gid)+c).replace(/("gid"\s*:\s*)(\d+)/g,(_,a,b)=>a+gid(b));
s=s.replace(/("tilesets"\s*:\s*)\[[\s\S]*?\]/,(_,a)=>a+JSON.stringify(expected.tilesets,null,4));
assert.deepStrictEqual(JSON.parse(s),expected);fs.writeFileSync(p,s);
const doc='Data/Stage/TILED_EDITING.md';let d=fs.readFileSync(doc,'utf8');d=d.replace('`Tiled/EnemyPlacements.tsj` のタイル','統合版では `Tiled/Enemies.tsj` のタイル（Walkerを含む全10種類）').replace('Mage・Bossは32×64px（1×2）、その他は32×32px（1×1）','Mage・Bossは32×64px（1×2）、Walkerは32×40px、その他は32×32px（1×1）').replace('タイルセット **LaboratoryBlocks** を選びます。\r\n左から','タイルセット **Blocks** を選びます。通常ブロック（100）に続き、').replace('タイルセット **LaboratoryBlocks** を選びます。\n左から','タイルセット **Blocks** を選びます。通常ブロック（100）に続き、').replace('統合版の新ブロックGIDは15～17です（14はWalker）。旧版・再出力ツールでは14～16です。既存の敵・ゲートのGIDは変更していません。','統合版のタイルセットは Blocks（GID 1～4）、Enemies（GID 5～14）、OneWayGates（GID 15～16）の3つです。重複したCubeBlock参照を統合し、WalkerもEnemiesに含めています。旧版・再出力ツールのGID体系とは異なります。');fs.writeFileSync(doc,d);
console.log('PASS: original map formatting retained; documentation updated.');
