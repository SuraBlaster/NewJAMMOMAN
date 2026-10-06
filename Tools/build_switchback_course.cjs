const fs=require('fs'),path=require('path');
const root=path.resolve(__dirname,'..');
const tiles=new Map(),decks=[],shafts=[],jumps=[],enemies=[];
const block=(x,y)=>tiles.set(`${x},${y}`,{typeId:100,position:{x,y},rotationDegrees:0,scale:{x:1,y:1}});
function deck(id,a,b,h){decks.push({id,a,b,h});for(let x=a;x<=b;x++)block(x,h-1);}
function wall(x,lo,hi){for(let y=lo;y<=hi;y++)block(x,y);}
function shaft(id,left,right,bottom,top,exit){shafts.push({id,left:left+.5,right:right-.5,bottom:bottom-.5,top:top-.5,exit});}
function jump(from,to,kind='main'){jumps.push({from,to,kind});}
function enemy(type,x,h){enemies.push({enemyType:type,direction:'Left',position:{x,y:h-.5}});}

// Three-storey switchback: right, up, LEFT, up, right.
deck('entry',-4,33,0);wall(-5,0,11);
wall(34,0,23);wall(29,3,11);deck('return',-1,29,12);
shaft('east lift',29,34,0,12,'left');
wall(-1,12,23);wall(4,15,23);deck('upper exit',4,50,24);
shaft('west lift',-1,4,12,24,'right');
block(10,0);block(22,12);
enemy('Mage',20,0);enemy('Mage',14,12);

// Upper shortcut and staggered lower route; missed jumps land on a recovery floor.
deck('lower A',55,62,22);deck('lower B',67,76,24);
deck('lower C',81,89,22);deck('lower D',94,99,24);
deck('recovery',35,104,14);wall(105,14,29);wall(100,17,29);
wall(49,27,29);wall(44,27,29);
deck('shortcut A',49,56,30);deck('shortcut B',63,69,30);
shaft('shortcut access',44,49,24,30,'right');
deck('shortcut C',76,84,30);deck('shortcut D',91,100,30);
jump('upper exit','lower A');jump('lower A','lower B');
jump('lower B','lower C');jump('lower C','lower D');
jump('shortcut A','shortcut B','shortcut');jump('shortcut B','shortcut C','shortcut');jump('shortcut C','shortcut D','shortcut');
shaft('recovery lift',100,105,14,30,'right');
enemy('Mage',72,24);enemy('Hover',79,32);

// Successive descent openings alternate right / left / right.
deck('drop entry',105,128,30);wall(141,9,33);
jump('shortcut D','drop entry','shortcut');
deck('drop middle',117,140,22);deck('drop lower',106,128,14);
deck('reactor base',106,175,6);enemy('Mage',124,22);

// Regain 28 m in two shafts separated by a combat landing.
wall(146,9,17);wall(151,6,17);deck('reactor landing',151,170,18);
shaft('reactor west',146,151,6,18,'right');
wall(176,6,33);wall(171,21,33);deck('reactor summit',176,184,34);
shaft('reactor east',171,176,18,34,'right');enemy('Mage',161,18);

// Final dash bridges, lower recovery passage, then the original-width boss arena.
deck('final bridge',191,199,34);deck('antechamber',206,225,34);
jump('reactor summit','final bridge');jump('final bridge','antechamber');
deck('final recovery',177,205,24);wall(206,24,33);wall(201,27,33);
shaft('final recovery lift',201,206,24,34,'right');
// Restore the original enclosed corridor/arena from BioLaboratoryTrainingStage.
// Only translate it to the new course end; keep dimensions, entry and boss facing.
for(const [key,o] of tiles)if(o.position.x>=210)tiles.delete(key);
const original=JSON.parse(fs.readFileSync(path.join(root,'Data/Stage/ConvertedStage.before-course-1790747179560.stage.json'),'utf8'));
for(const o of original.objects.filter(o=>o.position.x>=118))
 block(o.position.x+92,o.position.y+11);
const oldBoss=original.enemies.find(e=>e.enemyType==='Boss');
enemies.push({...oldBoss,position:{x:oldBoss.position.x+92,y:oldBoss.position.y+11}});
decks.push({id:'boss',a:225,b:240,h:34});

const sections=[
 {name:'01 / THREE-STOREY SWITCHBACK',minX:-6,maxX:35,low:-3,high:29},
 {name:'02 / UPPER SHORTCUT + LOWER ROUTE',minX:35,maxX:106,low:10,high:36},
 {name:'03 / ALTERNATING DESCENT',minX:104,maxX:145,low:3,high:37},
 {name:'04 / REACTOR CLIMB',minX:143,maxX:177,low:3,high:39},
 {name:'05 / FINAL BRIDGES + BOSS',minX:176,maxX:242,low:20,high:46}];
const mainPath=[[0,1],[31,1],[32,10],[27,13],[2,13],[2,23],[8,25],[47,25],
 [58,23],[71,25],[85,23],[97,25],[92,25],[92,15],[103,15],[103,31],[125,31],[136,23],
 [112,23],[112,15],[135,15],[136,7],[148,7],[148,17],[156,19],[173,19],[173,33],
 [181,35],[194,35],[208,35],[225,35],[234,35]];
const shortcutPath=[[47,25],[47,31],[54,31],[65,31],[81,31],[98,31],[107,31]];
const stage={formatVersion:1,stageName:'Laboratory - Switchback Reactor',objects:[...tiles.values()],enemies,
 fallRespawnZones:[{minX:-30,maxX:270,minY:-1000,maxY:-5}],
 bossApproachCameraZones:[{minX:209.5,maxX:225.5,minY:33.5,maxY:36.5}],
 bossEncounterZones:[{minX:225.5,maxX:239.5,minY:33.5,maxY:42.5}]};
const route={sections,decks,shafts,jumps,mainPath,shortcutPath,expectedSeconds:[100,165],
 timingNote:'Design budget including six optional enemy encounters, not a measured clear time.',
 reference:'https://static.capcom.com/megaman/manuals/PSP_MMMHX_Manual.pdf'};
const target=path.join(root,'Data/Stage/ConvertedStage.stage.json'),content=JSON.stringify(stage,null,2)+'\n';
if(fs.existsSync(target)&&fs.readFileSync(target,'utf8')!==content)
 fs.copyFileSync(target,target.replace('.stage.json',`.before-course-${Date.now()}.stage.json`));
fs.writeFileSync(target,content);
fs.writeFileSync(path.join(root,'Data/Stage/LaboratoryCourse.route.json'),JSON.stringify(route,null,2)+'\n');
// Same scale on both axes, showing the real vertical structure.
const X=x=>55+(x+6)*5.3,Y=y=>380-y*5.3;
let svg=`<svg xmlns="http://www.w3.org/2000/svg" width="1420" height="1540"><defs><marker id="arrow" markerWidth="5" markerHeight="5" refX="4" refY="2.5" orient="auto"><path d="M0 0 L5 2.5 L0 5" fill="#65e7f4"/></marker></defs><rect width="1420" height="1540" fill="#101822"/><style>text{font-family:Arial,sans-serif;fill:#e6edf3}.tile{fill:#7188a1;stroke:#253447;stroke-width:.4}</style><text x="35" y="35" font-size="25">SWITCHBACK REACTOR / COURSE REVISION 2</text><text x="35" y="61" font-size="15">Cyan: main route / Gold: upper shortcut / Red: enemies / Same scale on both axes</text>`;
for(const o of tiles.values())svg+=`<rect class="tile" x="${X(o.position.x-.5)}" y="${Y(o.position.y+.5)}" width="5.3" height="5.3"/>`;
function arrows(points,x,y,color){let out='';for(let i=1;i<points.length;i++)out+=`<line x1="${x(points[i-1][0])}" y1="${y(points[i-1][1])}" x2="${x(points[i][0])}" y2="${y(points[i][1])}" stroke="${color}" stroke-width="1.6" marker-end="url(#arrow)"/>`;return out;}
svg+=arrows(mainPath,X,Y,'#65e7f4')+arrows(shortcutPath,X,Y,'#ffbf60');
for(const e of enemies)svg+=`<circle cx="${X(e.position.x)}" cy="${Y(e.position.y+1)}" r="4" fill="#ff637c"/>`;
sections.forEach((s,i)=>{
 const col=i%2,row=Math.floor(i/2),left=35+col*705,top=430+row*360;
 const scale=Math.min(8,660/(s.maxX-s.minX),300/(s.high-s.low)),x=v=>left+(v-s.minX)*scale,y=v=>top+325-(v-s.low)*scale;
 svg+=`<text x="${left}" y="${top}" font-size="17">${s.name}</text>`;
 for(const o of tiles.values())if(o.position.x>=s.minX&&o.position.x<=s.maxX&&o.position.y>=s.low&&o.position.y<=s.high)
 svg+=`<rect class="tile" x="${x(o.position.x-.5)}" y="${y(o.position.y+.5)}" width="${scale}" height="${scale}"/>`;
 svg+=arrows(mainPath.filter(p=>p[0]>=s.minX&&p[0]<=s.maxX),x,y,'#65e7f4');
});
svg+='<text x="760" y="1260" font-size="17">24 m switchback climb / 24 m descent</text><text x="760" y="1290" font-size="17">28 m reactor climb / Lower recovery routes</text><text x="760" y="1320" font-size="16">100-165 s design budget; human playtest needed.</text></svg>';
fs.writeFileSync(path.join(root,'Data/Stage/LaboratoryCourse.svg'),svg);
require('./export_stage_tiled.cjs').exportStage(target,path.join(root,'Data/Stage/LaboratoryCourse.tmj'));
console.log(`${tiles.size} blocks / ${enemies.length-1} enemies + boss / ${shafts.length} shafts / ${jumps.length} bridge jumps`);
