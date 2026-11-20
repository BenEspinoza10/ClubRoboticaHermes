// Páginas web servidas por el portal META (sin internet, todo embebido)
//   PAGINA_HTML   ->  "/"          panel de control
//   PANTALLA_HTML ->  "/pantalla"  vista pública para proyectar (solo lectura)
#pragma once

// =====================================================================
//  PANEL DE CONTROL
// =====================================================================
const char PAGINA_HTML[] PROGMEM = R"HTML(
<!doctype html><html lang="es"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Cronómetro Robot</title>
<style>
:root{--bg:#0e1116;--card:#171b22;--card2:#1d222b;--line:#262c36;--tx:#e8ecf1;--mut:#8a94a3;
 --ok:#3ccf7a;--run:#f5b83d;--fin:#4da3ff;--bad:#ff5d5d;--oro:#f2c94c;--plata:#c3ccd6;--bronce:#d08a52}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--tx);
 font:15px/1.4 system-ui,-apple-system,Segoe UI,Roboto,sans-serif}
main{max-width:820px;margin:0 auto;padding:16px}
header{display:flex;justify-content:space-between;align-items:center;margin:2px 0 12px}
h1{font-size:15px;font-weight:600;letter-spacing:.08em;text-transform:uppercase;color:var(--mut);margin:0}
h2{font-size:12px;font-weight:600;letter-spacing:.08em;text-transform:uppercase;color:var(--mut);margin:0 0 10px}
a{color:var(--fin);text-decoration:none;font-weight:600;font-size:14px}
.card{background:var(--card);border:1px solid var(--line);border-radius:14px;padding:16px;margin-bottom:12px}
.mono{font-family:ui-monospace,SFMono-Regular,Menlo,Consolas,monospace;font-variant-numeric:tabular-nums}
.reloj{text-align:center;padding:22px 14px}
#eqR{font-size:clamp(20px,5vw,30px);font-weight:700;min-height:1.2em}
#intR{color:var(--mut);font-size:14px;margin:2px 0 8px;min-height:1.2em}
.barra{height:10px;border-radius:99px;background:#222a35;overflow:hidden;margin:14px auto 4px;max-width:520px;visibility:hidden}
.barra i{display:block;height:100%;width:100%;background:var(--fin);border-radius:99px;transition:width .2s linear}
.barra.fin i{background:var(--bad)}.corriendo .barra{visibility:visible}
#lim{color:var(--mut);font-size:13px;min-height:1.2em}
#t{font-weight:700;font-size:clamp(54px,16vw,116px);line-height:1;letter-spacing:-.02em}
#est{display:inline-block;margin-top:12px;padding:5px 14px;border-radius:99px;font-weight:600;
 font-size:13px;letter-spacing:.06em;text-transform:uppercase;background:#222a35}
.listo #est{color:var(--ok)} .corriendo #est,.corriendo #t{color:var(--run)}
.terminado #est,.terminado #t{color:var(--fin)} .nulo #t,.nulo #est{color:var(--bad)}
.esperando #est{color:#f5d33d} .listo.armado #est,.terminado.armado #est{color:var(--ok)}
.listo.armado #est{box-shadow:0 0 0 2px var(--ok) inset}
#acc .armar{background:#173a26;border:2px solid var(--ok);color:var(--ok);font-size:18px;padding:16px;min-width:280px}
#acc .armar:disabled{border-color:var(--line);color:var(--mut);background:#1f2530}
.atajo{color:var(--mut);font-size:12px;width:100%;margin-top:2px}
#res{margin-top:12px;color:var(--mut);min-height:1.4em}
#res b{color:var(--tx)} .tag{display:inline-block;padding:2px 8px;border-radius:6px;font-size:12px;
 font-weight:700;margin-left:6px;background:#1f3a2a;color:var(--ok)} .tag.lider{background:#3d3416;color:var(--oro)}
.tag.mal{background:#3a1f1f;color:var(--bad)}
#acc{display:flex;gap:8px;flex-wrap:wrap;justify-content:center;margin-top:14px}
#acc:empty{display:none}#acc button{flex:0 1 auto;min-width:200px}
.repetir{border-color:var(--run)!important;color:var(--run)}
#aviso{display:none;margin-top:12px;padding:10px;border-radius:10px;background:#3a1f1f;color:#ffb3b3;font-weight:600}
.chips{display:flex;flex-wrap:wrap;gap:8px}
.chip{max-width:100%;overflow-wrap:anywhere;padding:9px 14px;border-radius:10px;border:1px solid var(--line);background:var(--card2);
 color:var(--tx);font-weight:600;cursor:pointer;font-size:15px;text-align:left;display:flex;flex-direction:column;gap:3px}
.chip.sel{border-color:var(--fin);background:#16304d;box-shadow:0 0 0 1px var(--fin) inset}
.chip.ago{opacity:.45}
.dots{display:inline-flex;gap:3px}.dots i{width:8px;height:8px;border-radius:50%;background:var(--fin)}
.dots i.u{background:#3a4250}
.opts{display:flex;gap:18px;flex-wrap:wrap;margin-top:12px;color:var(--mut);font-size:14px;align-items:center}
.opts label{display:flex;gap:6px;align-items:center;cursor:pointer}
.opts input[type=number]{width:56px;padding:5px;border-radius:8px;border:1px solid var(--line);background:#10141a;color:var(--tx);font-size:14px}
.podio{display:grid;grid-template-columns:1fr 1.15fr 1fr;gap:10px;align-items:end;text-align:center}
.esc{border-radius:12px;padding:12px 6px;background:var(--card2);border-top:4px solid}
.esc .p{font-size:26px;font-weight:800}.esc .n{font-weight:700;margin:2px 0;overflow-wrap:anywhere}
.esc .tm{font-size:17px}.esc.v .n,.esc.v .tm{color:var(--mut)}
.e1{border-color:var(--oro);padding-top:22px}.e1 .p{color:var(--oro)}
.e2{border-color:var(--plata)}.e2 .p{color:var(--plata)}
.e3{border-color:var(--bronce)}.e3 .p{color:var(--bronce)}
table{width:100%;border-collapse:collapse}
td,th{padding:8px 6px;border-bottom:1px solid var(--line);text-align:right}
th{color:var(--mut);font-weight:500;font-size:12px;text-transform:uppercase}
td:nth-child(2),th:nth-child(2){text-align:left}td:first-child,th:first-child{text-align:left;color:var(--mut);width:32px}
#rank td:nth-child(2){font-family:system-ui,-apple-system,Segoe UI,Roboto,sans-serif;font-weight:600}
tr.sinT td{color:var(--mut)}
details{border:1px solid var(--line);border-radius:10px;margin-bottom:8px;background:var(--card2)}
summary{padding:12px;cursor:pointer;display:flex;justify-content:space-between;align-items:center;gap:8px;list-style:none}
summary::-webkit-details-marker{display:none}summary span{color:var(--mut)}
.det{padding:0 12px 12px}.info{color:var(--mut);margin-bottom:10px;font-size:14px}.info b{color:var(--tx)}
.fila{display:flex;gap:8px;flex-wrap:wrap}
button,.btn{flex:1;min-width:110px;padding:11px;border-radius:10px;border:1px solid var(--line);text-align:center;
 background:#1f2530;color:var(--tx);font-weight:600;font-size:14px;cursor:pointer;font-family:inherit}
button:active{transform:scale(.98)}button:disabled{opacity:.35;cursor:default}
.peligro{color:var(--bad)}.peq{flex:0;min-width:44px}
input[type=text]{flex:3;min-width:160px;padding:11px;border-radius:10px;border:1px solid var(--line);
 background:#10141a;color:var(--tx);font-size:15px}
.portales{display:grid;grid-template-columns:1fr 1fr;gap:12px}
.portal{display:flex;gap:10px;align-items:center}
.dot{width:12px;height:12px;border-radius:50%;background:var(--bad);flex:none}
.dot.on{background:var(--ok)}.portal small{display:block;color:var(--mut)}
.vacio{color:var(--mut);text-align:center;padding:10px}
#off{display:none;color:var(--bad);text-align:center;margin-bottom:10px}
@media(max-width:520px){.portales{grid-template-columns:1fr}.esc .p{font-size:20px}.esc .tm{font-size:14px}
 #acc button{min-width:0;flex:1}}
</style></head><body><main>
<header><h1>Cronómetro de vuelta</h1><a href="/pantalla" target="_blank">Pantalla pública ↗</a></header>
<div id="off">Sin conexión con el portal de meta…</div>

<div class="card reloj listo" id="reloj">
 <div id="eqR"></div><div id="intR"></div>
 <div id="t" class="mono">0:00.000</div>
 <div class="barra" id="barra"><i></i></div><div id="lim"></div>
 <div id="est">Listo</div><div id="res"></div>
 <div id="acc"></div>
 <div id="aviso"></div>
</div>

<div class="card"><h2>Equipo en pista</h2>
 <div class="chips" id="chips"></div>
 <div class="opts">
  <label><input type="checkbox" id="auto" onchange="post('config',{auto:this.checked?1:0})"> Pasar al siguiente equipo al agotar intentos</label>
  <label><input type="checkbox" id="son" onchange="sonido(this.checked)"> Sonido</label>
 </div>
</div>

<div class="card"><h2>Top 3</h2><div class="podio" id="podio"></div></div>

<div class="card"><h2>Clasificación <span style="text-transform:none;letter-spacing:0">(mejor de cada equipo)</span></h2>
 <table><thead><tr><th>#</th><th>Equipo</th><th>Mejor</th><th>Dif.</th><th>Int.</th></tr></thead>
 <tbody id="rank" class="mono"></tbody></table>
</div>

<div class="card"><h2>Equipos</h2>
 <div id="lista"></div>
 <div class="fila" style="margin-top:6px">
  <input type="text" id="nuevo" maxlength="60" placeholder="Nombre del nuevo equipo"
   onkeydown="if(event.key==='Enter')agregar()">
  <button onclick="agregar()">+ Añadir equipo</button>
 </div>
</div>

<div class="card"><h2>Control</h2>
 <div class="opts" style="margin:0 0 12px">
  <label>Intentos por equipo <input type="number" id="nInt" min="1" max="20"
   onchange="post('config',{intentos:this.value})"></label>
 </div>
 <div class="fila">
  <button onclick="post('recalibrar')">Recalibrar sensores</button>
  <a class="btn" href="/api/csv">Descargar resultados (CSV)</a>
 </div>
 <div class="fila" style="margin-top:8px">
  <button class="peligro" onclick="confirm('¿Reiniciar tiempos e intentos de TODOS los equipos?')&&post('reiniciar-todo')">Reiniciar competencia</button>
  <button class="peligro" onclick="confirm('¿Eliminar todos los equipos y volver a los 3 de ejemplo?')&&post('fabrica')">Restablecer equipos</button>
 </div>
</div>

<div class="card portales">
 <div class="portal"><div class="dot" id="dI"></div><div>Portal inicio<small id="sI">–</small></div></div>
 <div class="portal"><div class="dot" id="dM"></div><div>Portal meta<small id="sM">–</small></div></div>
</div>
</main><script>
const $=i=>document.getElementById(i);
const esc=s=>String(s).replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
let E=null,D=null,rev=-1,tSrv=0,tRef=0,fallos=0,prev=null,ac=null,accKey='';
function fmt(ms){if(!ms&&ms!==0)return'–';const m=Math.floor(ms/60000),s=Math.floor(ms%60000/1000),x=ms%1000;
 return m+':'+String(s).padStart(2,'0')+'.'+String(x).padStart(3,'0')}
const eqDe=id=>D&&D.equipos.find(e=>e.id===id);
const nombre=id=>{const e=eqDe(id);return e?e.nombre:''};
const dots=e=>'<span class="dots">'+Array.from({length:D.intentos},(_,k)=>`<i class="${k<e.usados?'u':''}"></i>`).join('')+'</span>';
const resta=e=>Math.max(0,D.intentos-e.usados);

async function post(acc,p={}){
 const q=new URLSearchParams(p).toString();
 try{const r=await fetch('/api/'+acc+(q?'?'+q:''),{method:'POST'});if(!r.ok)alert(await r.text())}catch(e){}
 leer()}
const elegir=id=>post('activo',{id});
function agregar(){const i=$('nuevo');post('equipo/nuevo',{nombre:i.value.trim()});i.value=''}
function renombrar(id){const n=prompt('Nuevo nombre:',nombre(id));if(n&&n.trim())post('equipo/renombrar',{id,nombre:n.trim()})}
function reiniciar(id){confirm('¿Reiniciar '+nombre(id)+'? Se borra su mejor tiempo y recupera todos sus intentos.')&&post('equipo/reiniciar',{id})}
function eliminar(id){confirm('¿Eliminar el equipo '+nombre(id)+'?')&&post('equipo/eliminar',{id})}
function anular(id){confirm('¿Anular el último intento de '+nombre(id)+'? Se le devuelve el intento y su tiempo anterior.')&&post('equipo/anular',{id})}
function devolver(id){confirm('¿Devolver 1 intento a '+nombre(id)+'? (su mejor tiempo no cambia)')&&post('equipo/devolver',{id})}
function repetir(){post('repetir')}
function armar(){post('armar')}
// Atajo: barra espaciadora = LISTO (si no se está escribiendo en un campo)
document.addEventListener('keydown',ev=>{
 if(ev.code!=='Space'||/INPUT|TEXTAREA|BUTTON/.test(document.activeElement.tagName)||!E)return;
 ev.preventDefault();
 if(E.estado!=='corriendo')E.armado?post('desarmar'):armar()});
function nulo(){confirm('¿Marcar este intento como NULO? Cuenta como intento usado, sin tiempo.')&&post('nulo')}

// ---------- sonido ----------
function sonido(on){try{localStorage.son=on?1:0}catch(e){}
 if(on&&!ac){ac=new(window.AudioContext||window.webkitAudioContext)()}}
function beep(f,d,n=1){if(!ac||!$('son').checked)return;for(let i=0;i<n;i++){
 const o=ac.createOscillator(),g=ac.createGain(),t=ac.currentTime+i*(d+.07);
 o.frequency.value=f;g.gain.setValueAtTime(.25,t);g.gain.exponentialRampToValueAtTime(.001,t+d);
 o.connect(g).connect(ac.destination);o.start(t);o.stop(t+d)}}

// ---------- ranking ----------
function ranking(){
 const r=D.equipos.slice();
 return r.filter(e=>e.mejor).sort((a,b)=>a.mejor-b.mejor).concat(r.filter(e=>!e.mejor))}
function ultimoTxt(e){return e.ultimo<0?'—':e.ultimo===0?'nulo':fmt(e.ultimo)}

function pintarDatos(){
 const R=ranking(),lider=R[0]&&R[0].mejor;
 $('auto').checked=D.auto;
 if(document.activeElement!==$('nInt'))$('nInt').value=D.intentos;
 $('chips').innerHTML=D.equipos.map(e=>`<button class="chip ${e.id===D.activo?'sel':''} ${resta(e)?'':'ago'}" onclick="elegir(${e.id})">${esc(e.nombre)}${dots(e)}</button>`).join('')
  ||'<div class="vacio">Añade un equipo abajo</div>';
 $('podio').innerHTML=[R[1],R[0],R[2]].map((e,k)=>{const p=[2,1,3][k];const ok=e&&e.mejor;
  return`<div class="esc e${p} ${ok?'':'v'}"><div class="p">${p}º</div><div class="n">${ok?esc(e.nombre):'—'}</div><div class="tm mono">${ok?fmt(e.mejor):'sin tiempo'}</div></div>`}).join('');
 $('rank').innerHTML=R.map((e,i)=>e.mejor
  ?`<tr><td>${i+1}</td><td>${esc(e.nombre)}</td><td>${fmt(e.mejor)}</td><td>${i?'+'+((e.mejor-lider)/1000).toFixed(3):'—'}</td><td>${e.usados}/${D.intentos}</td></tr>`
  :`<tr class="sinT"><td>–</td><td>${esc(e.nombre)}</td><td>sin tiempo</td><td></td><td>${e.usados}/${D.intentos}</td></tr>`).join('');
 const abiertos=new Set([...document.querySelectorAll('#lista details[open]')].map(d=>+d.dataset.id));
 $('lista').innerHTML=D.equipos.map((e,i)=>`<details data-id="${e.id}" ${abiertos.has(e.id)?'open':''}>
  <summary><b>${esc(e.nombre)}</b><span class="mono">${e.mejor?fmt(e.mejor):'sin tiempo'} ${dots(e)}</span></summary>
  <div class="det"><div class="info">Mejor: <b class="mono">${e.mejor?fmt(e.mejor):'—'}</b> · Último intento: <b class="mono">${ultimoTxt(e)}</b> · Quedan <b>${resta(e)}</b> de ${D.intentos}</div>
  <div class="fila"><button onclick="elegir(${e.id})">Poner en pista</button><button onclick="renombrar(${e.id})">Renombrar</button>
  <button class="peq" title="Subir en el turno" onclick="post('equipo/mover',{id:${e.id},dir:-1})" ${i?'':'disabled'}>↑</button>
  <button class="peq" title="Bajar en el turno" onclick="post('equipo/mover',{id:${e.id},dir:1})" ${i<D.equipos.length-1?'':'disabled'}>↓</button></div>
  <div class="fila" style="margin-top:8px"><button onclick="anular(${e.id})" ${e.anulable?'':'disabled'}>Anular último intento</button>
  <button onclick="devolver(${e.id})" ${e.usados?'':'disabled'}>Devolver 1 intento</button></div>
  <div class="fila" style="margin-top:8px"><button class="peligro" onclick="reiniciar(${e.id})">Reiniciar equipo</button>
  <button class="peligro" onclick="eliminar(${e.id})">Eliminar equipo</button></div></div></details>`).join('');
}

function portal(d,s,p){$(d).className='dot'+(p.online&&p.ok?' on':'');
 $(s).textContent=!p.online?'desconectado':!p.ok?'sensor con error':p.dist+' mm (piso '+p.base+' mm)'}

function pintarEstado(){
 const u=E.ult,st=E.estado,nul=st==='terminado'&&u.nulo;
 $('reloj').className='card reloj '+st+(nul?' nulo':'');
 const arm=E.armado&&st!=='corriendo';
 $('reloj').className+=st==='corriendo'?'':arm?' armado':st==='listo'?' esperando':'';
 $('est').textContent=st==='listo'?(arm?'Listo · esperando largada':'Esperando'):
  nul?(u.motivo===2?'Tiempo excedido':'Intento nulo'):{corriendo:'En carrera',terminado:'Terminado'}[st];
 const idR=st==='listo'?E.activo:st==='corriendo'?E.enCarrera:(u.id>=0?u.id:E.enCarrera);
 const e=eqDe(idR);
 $('eqR').textContent=e?e.nombre:(st==='listo'?'Selecciona un equipo':'Sin equipo');
 let it='';
 if(e){if(st==='corriendo')it=`Intento ${e.usados+1} de ${D.intentos}`;
  else if(st==='terminado'&&u.num)it=`Intento ${u.num} de ${D.intentos}`;
  else it=resta(e)?`Próximo: intento ${e.usados+1} de ${D.intentos}`:`Sin intentos restantes`}
 $('intR').textContent=it;
 let r='';
 if(st!=='listo'&&u.id>=0&&nombre(u.id)&&st==='terminado'){
  r=u.nulo?(u.motivo===2?'Superó el límite de '+fmtLim(E.limite)+' · no cuenta tiempo':'No cuenta tiempo<span class="tag mal">NULO</span>')
   :(u.pos?`${u.pos}º lugar`:'')+(u.lider?'<span class="tag lider">¡NUEVO LÍDER!</span>':u.record?'<span class="tag">Mejor personal</span>':'<span class="tag mal">No mejoró</span>');
  const eq=eqDe(u.id);if(eq&&eq.mejor)r+=` · mejor del equipo <b class="mono">${fmt(eq.mejor)}</b>`}
 $('res').innerHTML=r;
 // botones según el momento del intento
 const ea0=eqDe(E.activo),puede=ea0&&resta(ea0)&&E.inicioOnline;
 let k=st+(st==='terminado'&&u.anulable?'A':'')+(arm?'R':'')+(puede?'P':'')+(E.inicioOnline?'I':'')+E.activo+'-'+(ea0?resta(ea0):''),h='';
 if(st==='corriendo')h='<button class="repetir" onclick="repetir()">↺ Repetir intento (no cuenta)</button><button class="peligro" onclick="nulo()">✕ Intento nulo (cuenta)</button>';
 else{
  if(arm)h='<button onclick="post(\'desarmar\')">✕ Cancelar · volver a esperando</button>';
  else h=`<button class="armar" onclick="armar()" ${puede?'':'disabled'}>▶ LISTO PARA LARGAR</button>`
   +(puede?'':`<div class="atajo">${!ea0?'Selecciona un equipo':!resta(ea0)?'El equipo no tiene intentos':'El portal de inicio no está conectado'}</div>`);
  if(st==='terminado'&&u.anulable)h+='<button class="repetir" onclick="repetir()">↺ Repetir intento (anular este)</button>';
  h+='<div class="atajo">Atajo: barra espaciadora</div>'}
 if(k!==accKey){$('acc').innerHTML=h;accKey=k}
 const a=$('aviso');
 const ea=eqDe(E.activo);
 if(E.aviso&&E.avisoMotivo===1&&!E.armado){a.style.display='block';
  a.textContent='Largada ignorada: el portal no estaba listo. Presiona LISTO PARA LARGAR antes de soltar el robot.'}
 else if(E.aviso&&E.avisoMotivo===2&&!(ea&&resta(ea))){a.style.display='block';
  a.textContent=ea?`Largada ignorada: ${ea.nombre} ya usó sus ${D.intentos} intentos. Elige otro equipo.`:'Largada ignorada: no hay equipo seleccionado.'}
 else a.style.display='none';
 portal('dI','sI',E.inicio);portal('dM','sM',E.meta)}

async function leer(){
 try{const j=await (await fetch('/api/estado',{cache:'no-store'})).json();
  if(j.rev!==rev){D=await (await fetch('/api/equipos',{cache:'no-store'})).json();rev=j.rev;pintarDatos()}
  if(prev&&prev!==j.estado){if(j.estado==='corriendo')beep(880,.15);
   if(j.estado==='terminado')j.ult.nulo?beep(300,.4):j.ult.lider||j.ult.record?beep(1320,.12,3):beep(660,.2,2)}
  prev=j.estado;E=j;tSrv=j.t;tRef=performance.now();pintarEstado();fallos=0;$('off').style.display='none'}
 catch(e){if(++fallos>3)$('off').style.display='block'}}
const fmtLim=ms=>{const s=Math.round(ms/1000);return Math.floor(s/60)+':'+String(s%60).padStart(2,'0')};
function barra(ms){const b=$('barra');if(!b||!E.limite)return;
 const rest=Math.max(0,E.limite-ms),pct=E.estado==='corriendo'?rest/E.limite*100:100;
 b.firstChild.style.width=pct+'%';b.classList.toggle('fin',rest<30000);
 $('lim').textContent=E.estado==='corriendo'?'Quedan '+fmtLim(rest)+' de '+fmtLim(E.limite):'Tiempo límite: '+fmtLim(E.limite)}
function tick(){if(E){let ms=E.estado==='listo'?0:tSrv;if(E.estado==='corriendo')ms+=Math.round(performance.now()-tRef);
 $('t').textContent=E.estado==='terminado'&&E.ult.nulo?'NULO':fmt(ms);barra(ms)}requestAnimationFrame(tick)}
try{if(localStorage.son==='1'){$('son').checked=true;document.addEventListener('click',()=>sonido(true),{once:true})}}catch(e){}
setInterval(leer,200);leer();tick();
</script></body></html>
)HTML";

// =====================================================================
//  PANTALLA PÚBLICA (para proyector / TV) — solo lectura
// =====================================================================
const char PANTALLA_HTML[] PROGMEM = R"HTML(
<!doctype html><html lang="es"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Cronómetro — Pantalla</title>
<style>
:root{--bg:#090b0f;--card:#141920;--tx:#f1f4f8;--mut:#7f8a99;--run:#f5b83d;--fin:#4da3ff;--ok:#3ccf7a;--bad:#ff5d5d;
 --oro:#f2c94c;--plata:#c3ccd6;--bronce:#d08a52}
*{box-sizing:border-box}html,body{height:100%}body{margin:0;background:var(--bg);color:var(--tx);
 font:16px/1.3 system-ui,-apple-system,Segoe UI,Roboto,sans-serif;display:flex;flex-direction:column;
 justify-content:center;padding:3vh 4vw;gap:3.5vh;cursor:none}
.mono{font-family:ui-monospace,SFMono-Regular,Menlo,Consolas,monospace;font-variant-numeric:tabular-nums}
#top{text-align:center}
#eq{font-size:5vw;font-weight:800}
#it{font-size:2.2vw;color:var(--mut);min-height:1.2em}
#t{font-weight:800;font-size:16vw;line-height:1;letter-spacing:-.03em}
.corriendo #t{color:var(--run)}.terminado #t{color:var(--fin)}.nulo #t{color:var(--bad)}
#res{font-size:2.6vw;color:var(--mut);min-height:1.3em;margin-top:1vh}
.barra{height:1.4vh;border-radius:99px;background:#1b212b;overflow:hidden;margin:2vh auto 0;width:70%;visibility:hidden}
.barra i{display:block;height:100%;width:100%;background:var(--fin);border-radius:99px;transition:width .25s linear}
.barra.fin i{background:var(--bad)}.corriendo .barra{visibility:visible}#lim{display:none}
#res b{color:var(--ok)}#res b.lider{color:var(--oro)}#res b.mal{color:var(--bad)}#res b.esp{color:#f5d33d}
.podio{display:grid;grid-template-columns:1fr 1.15fr 1fr;gap:2vw;align-items:end;text-align:center}
.esc{background:var(--card);border-radius:1.4vw;padding:2vh 1vw;border-top:.6vw solid}
.esc .p{font-size:4vw;font-weight:900}.esc .n{font-size:2.6vw;font-weight:700;overflow-wrap:anywhere}
.esc .tm{font-size:2.6vw}.e1{border-color:var(--oro);padding-top:4vh}.e1 .p{color:var(--oro)}
.e2{border-color:var(--plata)}.e2 .p{color:var(--plata)}.e3{border-color:var(--bronce)}.e3 .p{color:var(--bronce)}
.esc.v{opacity:.35}
#resto{display:flex;flex-wrap:wrap;justify-content:center;gap:1vh 3vw;font-size:1.8vw;color:var(--mut)}
#resto span b{color:var(--tx)}
@media(orientation:portrait){#eq{font-size:7vw}#it{font-size:3.6vw}#t{font-size:20vw}#res{font-size:4vw}
 .esc .n,.esc .tm{font-size:4vw}.esc .p{font-size:6vw}#resto{font-size:3.4vw}}
</style></head><body>
<div id="top" class="listo"><div id="eq">&nbsp;</div><div id="it"></div><div id="t" class="mono">0:00.000</div><div class="barra" id="barra"><i></i></div><div id="lim"></div><div id="res"></div></div>
<div class="podio" id="podio"></div>
<div id="resto"></div>
<script>
const $=i=>document.getElementById(i);
const esc=s=>String(s).replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
let E=null,D=null,rev=-1,tSrv=0,tRef=0;
function fmt(ms){const m=Math.floor(ms/60000),s=Math.floor(ms%60000/1000),x=ms%1000;
 return m+':'+String(s).padStart(2,'0')+'.'+String(x).padStart(3,'0')}
const eqDe=id=>D&&D.equipos.find(e=>e.id===id);
function datos(){
 const R=D.equipos.filter(e=>e.mejor).sort((a,b)=>a.mejor-b.mejor);
 $('podio').innerHTML=[R[1],R[0],R[2]].map((e,k)=>{const p=[2,1,3][k];
  return`<div class="esc e${p} ${e?'':'v'}"><div class="p">${p}º</div><div class="n">${e?esc(e.nombre):'—'}</div><div class="tm mono">${e?fmt(e.mejor):''}</div></div>`}).join('');
 $('resto').innerHTML=R.slice(3,12).map((e,i)=>`<span>${i+4}º <b>${esc(e.nombre)}</b> <span class="mono">${fmt(e.mejor)}</span></span>`).join('')}
function estado(){
 const u=E.ult,st=E.estado,nul=st==='terminado'&&u.nulo;
 $('top').className=st+(nul?' nulo':'');
 const e=eqDe(st==='listo'?E.activo:st==='corriendo'?E.enCarrera:(u.id>=0?u.id:E.enCarrera));
 $('eq').textContent=e?e.nombre:' ';
 $('it').textContent=!e?'':st==='corriendo'?`Intento ${e.usados+1} de ${D.intentos}`:
  st==='terminado'&&u.num?`Intento ${u.num} de ${D.intentos}`:
  e.usados<D.intentos?`Intento ${e.usados+1} de ${D.intentos}`:'';
 $('res').innerHTML=st==='listo'?(!e||e.usados>=D.intentos?'':E.armado?'<b>LISTO</b>':'<b class="esp">Esperando…</b>'):
  st!=='terminado'||u.id<0?'':u.nulo?(u.motivo===2?'<b class="mal">Tiempo excedido</b>':'<b class="mal">Intento nulo</b>'):
  (u.pos?`${u.pos}º lugar`:'')+(u.lider?' · <b class="lider">¡NUEVO LÍDER!</b>':u.record?' · <b>Mejor personal</b>':'')}
async function leer(){try{const j=await (await fetch('/api/estado',{cache:'no-store'})).json();
 if(j.rev!==rev){D=await (await fetch('/api/equipos',{cache:'no-store'})).json();rev=j.rev;datos()}
 E=j;tSrv=j.t;tRef=performance.now();estado()}catch(e){}}
const fmtLim=ms=>{const s=Math.round(ms/1000);return Math.floor(s/60)+':'+String(s%60).padStart(2,'0')};
function barra(ms){const b=$('barra');if(!b||!E.limite)return;
 const rest=Math.max(0,E.limite-ms),pct=E.estado==='corriendo'?rest/E.limite*100:100;
 b.firstChild.style.width=pct+'%';b.classList.toggle('fin',rest<30000);
 $('lim').textContent=E.estado==='corriendo'?'Quedan '+fmtLim(rest)+' de '+fmtLim(E.limite):'Tiempo límite: '+fmtLim(E.limite)}
function tick(){if(E){let ms=E.estado==='listo'?0:tSrv;if(E.estado==='corriendo')ms+=Math.round(performance.now()-tRef);
 $('t').textContent=E.estado==='terminado'&&E.ult.nulo?'NULO':fmt(ms);barra(ms)}requestAnimationFrame(tick)}
setInterval(leer,250);leer();tick();
</script></body></html>
)HTML";
