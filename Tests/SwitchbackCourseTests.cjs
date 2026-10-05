const fs=require('fs'),assert=require('assert'),path=require('path');
process.chdir(path.resolve(__dirname,'..'));
const stage=JSON.parse(fs.readFileSync('Data/Stage/ConvertedStage.stage.json'));
const route=JSON.parse(fs.readFileSync('Data/Stage/LaboratoryCourse.route.json'));
const cells=new Set(stage.objects.map(o=>`${o.position.x},${o.position.y}`));
assert.equal(cells.size,stage.objects.length);
const has=(x,y)=>cells.has(`${x},${y}`);
const deck=id=>route.decks.find(d=>d.id===id);
assert(has(0,-1));assert(stage.enemies.length===7);
// Headroom and vertical openings: bridges must not accidentally cap a climbing shaft.
for(const s of route.shafts){
 assert(s.right-s.left>=4&&s.right-s.left<=5);
 for(let x=Math.ceil(s.left);x<s.right;x++)
  for(let y=Math.ceil(s.bottom);y<s.top;y++)assert(!has(x,y),`Blocked shaft ${s.id}: ${x},${y}`);
 const exitX=s.exit==='left'?s.left-.5:s.right+.5;
 assert(has(exitX,s.top-.5),`Missing shaft exit ${s.id}`);
 for(let y=Math.ceil(s.top);y<s.top+1.4;y++)assert(!has(exitX,y),`No exit headroom ${s.id}`);
}
// Integrate held dash jumps at three frame rates. Check the entire body against
// actual tiles, including ceilings and walls, and require a landing on the target.
for(const fps of [30,60,120]){
 for(const j of route.jumps){
  const a=deck(j.from),b=deck(j.to);assert(a&&b);
  let x=a.b-.1,y=a.h-.499,vy=12.6,landed=false;
  for(let frame=0;frame<180;frame++){
   const previous=y;vy-=(vy<0?33.75:22.5)/fps;x+=9/fps;y+=vy/fps;
   for(let tx=Math.ceil(x-.9);tx<=Math.floor(x+.9);tx++)
    for(let ty=Math.ceil(y-.5);ty<=Math.floor(y+1.9);ty++){
     if(!has(tx,ty))continue;
     const top=ty+.5;
     if(x+.4<=tx-.5||x-.4>=tx+.5||y>=top||y+1.4<=ty-.5)continue;
     if(vy<0&&previous>=top-.001){
      assert(tx>=b.a&&tx<=b.b&&Math.abs(top-(b.h-.5))<.01,`Wrong landing ${j.from}->${j.to}`);
      landed=true;break;
     }
     assert.fail(`Body collision ${j.from}->${j.to} at ${tx},${ty}, ${fps} FPS`);
    }
   if(landed)break;
   if(y<b.h-5)break;
  }
  assert(landed,`Missed ${j.from}->${j.to} at ${fps} FPS`);
 }
 for(const s of route.shafts){
  let x=0,y=0,vx=5,vy=10.5,t=0;
  while(x<s.right-s.left-.8&&t<2){if(t>=.2)vx=Math.min(6,vx+15/fps);vy-=(vy<0?33.75:22.5)/fps;x+=vx/fps;y+=vy/fps;t+=1/fps;}
  assert(y>1.3,`Insufficient wall-kick gain in ${s.id}`);
 }
 console.log(`PASS ${fps} FPS: all ${route.jumps.length} jumps land without body/ceiling collisions; wall kicks gain height.`);
}
for(const e of stage.enemies){
 if(e.enemyType!=='Hover')assert(has(e.position.x,e.position.y-.5),`Enemy has no floor at ${e.position.x}`);
}
const arena=stage.bossEncounterZones[0],boss=stage.enemies.find(e=>e.enemyType==='Boss');
assert(boss.position.x>arena.minX&&boss.position.x<arena.maxX);
assert.equal(arena.maxX-arena.minX,14);
// The boss is authored against this exact original room. Translation is allowed;
// resizing, removing the roof, or changing the entrance/facing is not.
const original=JSON.parse(fs.readFileSync('Data/Stage/ConvertedStage.before-course-1790747179560.stage.json'));
const originalRoom=original.objects.filter(o=>o.position.x>=118)
 .map(o=>`${o.position.x+92},${o.position.y+11}`).sort();
assert.deepStrictEqual(stage.objects.filter(o=>o.position.x>=210).map(o=>`${o.position.x},${o.position.y}`).sort(),originalRoom);
const originalBoss=original.enemies.find(e=>e.enemyType==='Boss');
assert.equal(boss.direction,originalBoss.direction);
assert.deepStrictEqual(boss.position,{x:originalBoss.position.x+92,y:originalBoss.position.y+11});
assert(route.mainPath.filter((p,i,a)=>i&&p[0]<a[i-1][0]-5).length>=2,'Route must double back');
assert(Math.max(...route.mainPath.map(p=>p[1]))-Math.min(...route.mainPath.map(p=>p[1]))>=30);
assert.equal(deck('return').h-deck('entry').h,12);
assert.equal(deck('drop entry').h-deck('reactor base').h,24);
assert.equal(deck('reactor summit').h-deck('reactor base').h,28);
assert(route.expectedSeconds[1]<180);
console.log(`PASS: ${stage.objects.length} blocks, shaft openings, upper/lower routes, enemy floors, arena, 34m elevation range.`);
