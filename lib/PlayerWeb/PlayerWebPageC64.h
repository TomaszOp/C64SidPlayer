// Skorka "Commodore 64": strona w stylu ekranu C64 (czcionka terminalowa,
// niebieski ekran z ramka, lista plikow jak LOAD"$",8). To samo /api/... co PlayerWebPage.h.
#pragma once
#include <pgmspace.h>

static const char PLAYER_WEB_PAGE_C64[] PROGMEM = R"HTML(<!doctype html>
<html lang="pl"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="theme-color" content="#7869c4">
<title>C64 SID PLAYER</title>
<style>
:root{--bd:#7869c4;--scr:#40318d;--txt:#a59de8;--hi:#fff;--err:#ff8a7a}
*{box-sizing:border-box;border-radius:0}
html{-webkit-text-size-adjust:100%}
body{margin:0;min-height:100vh;background:var(--bd);color:var(--txt);
 font:bold 16px/1.3 "C64 Pro Mono","C64 Pro","Commodore 64 Pixelized","Courier New",ui-monospace,Menlo,Consolas,monospace;
 text-transform:uppercase;padding:max(12px,env(safe-area-inset-top)) max(12px,env(safe-area-inset-right)) 24px max(12px,env(safe-area-inset-left))}
.scr{max-width:800px;margin:auto;background:var(--scr);padding:18px 14px 22px;min-height:calc(100vh - 36px)}
@media(min-width:700px){body{padding:36px}.scr{padding:26px 34px 30px;min-height:calc(100vh - 72px)}}
p,pre{margin:0}
.c{text-align:center}
.gap{height:1.3em}
.dim{opacity:.75}
.wrap{word-break:break-word}
button,select,input{font:inherit;text-transform:uppercase;color:var(--scr);background:var(--txt);border:0;
 min-height:44px;padding:0 10px;cursor:pointer;touch-action:manipulation}
button:active,button.on{background:var(--hi)}
button:disabled{opacity:.35}
button:focus-visible,select:focus-visible,input:focus-visible{outline:2px dashed var(--hi);outline-offset:2px}
.ctl{display:grid;grid-template-columns:repeat(5,1fr);gap:6px;margin-top:10px}
.ctl2{grid-template-columns:repeat(2,1fr);margin-top:6px}
.ln{display:flex;align-items:center;gap:10px;flex-wrap:wrap;margin-top:10px}
.ln>span:first-child{min-width:8ch}
select{flex:1;min-width:12ch}
input[type=range]{flex:1;min-width:10ch;-webkit-appearance:none;appearance:none;background:transparent;padding:0;height:44px}
input[type=range]::-webkit-slider-runnable-track{height:16px;background:repeating-linear-gradient(90deg,var(--txt) 0 8px,transparent 8px 10px)}
input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:16px;height:28px;margin-top:-6px;background:var(--hi)}
input[type=range]::-moz-range-track{height:16px;background:repeating-linear-gradient(90deg,var(--txt) 0 8px,transparent 8px 10px)}
input[type=range]::-moz-range-thumb{width:16px;height:28px;background:var(--hi);border:0;border-radius:0}
#search{flex:1;min-width:10ch;background:transparent;color:var(--txt);border-bottom:2px solid var(--txt);cursor:text}
#search::placeholder{color:var(--txt);opacity:.5}
#drop{border:2px dashed var(--txt);padding:16px 10px;text-align:center;cursor:pointer;margin-top:6px}
#drop.over{background:var(--txt);color:var(--scr)}
#fileIn{display:none}
#bar{height:16px;margin-top:8px;display:none;border:2px solid var(--txt)}
#bar i{display:block;height:100%;width:0;background:var(--txt)}
ul{list-style:none;margin:6px 0 0;padding:0;display:grid;gap:4px}
li{display:flex;align-items:center;gap:8px;min-height:44px}
li .b{min-width:4ch;text-align:right}
li .n{flex:1;min-width:0;cursor:pointer;white-space:nowrap;overflow:hidden;text-overflow:ellipsis;padding:10px 0}
li.on .n{background:var(--txt);color:var(--scr)}
li button{min-width:6ch}
li .del{background:transparent;color:var(--txt);border:2px solid var(--txt)}
#msg{min-height:1.3em;margin-top:10px}
.err{color:var(--err)}
.cur{display:inline-block;width:.8em;height:1.05em;background:var(--txt);vertical-align:text-bottom;animation:bl 1s steps(1) infinite}
@keyframes bl{50%{opacity:0}}
</style></head><body>
<div class="scr">
 <p class="c">**** COMMODORE 64 SID PLAYER ****</p>
 <p class="c"><span id="ram">64K RAM SYSTEM</span> &nbsp;<span id="free">?</span> BLOCKS FREE</p>
 <div class="gap"></div>
 <p>READY.</p>
 <p id="load" class="wrap">LOAD"*",8,1</p>
 <div id="info" class="wrap"></div>
 <p id="tech" class="dim wrap"></p>

 <div class="ctl">
  <button id="fprev" title="Poprzedni plik">&lt;F</button>
  <button id="prev" title="Poprzedni podutwor">&lt;T</button>
  <button id="pp" title="Graj / pauza">RUN</button>
  <button id="next" title="Nastepny podutwor">T&gt;</button>
  <button id="fnext" title="Nastepny plik">F&gt;</button>
 </div>
 <div class="ctl ctl2">
  <button id="stop" title="Stop">STOP</button>
  <button id="mute" title="Wycisz">MUTE</button>
 </div>
 <div class="ln"><span>VOLUME</span><input type="range" id="vol" min="0" max="100"><span id="volv"></span></div>
 <div class="ln"><span>SID</span>
  <select id="model"><option value="-1">AUTO</option><option value="0">6581</option><option value="1">8580</option></select></div>
 <div class="ln"><span>WYGLAD</span><select id="skin"></select></div>

 <div class="gap"></div>
 <p>READY.</p>
 <p>SAVE"*.SID",8</p>
 <div id="drop" tabindex="0">DOTKNIJ ALBO UPUSC TU PLIKI .SID</div>
 <input type="file" id="fileIn" multiple>
 <div id="bar"><i id="barI"></i></div>

 <div class="gap"></div>
 <p>READY.</p>
 <p>LOAD"$",8</p>
 <div class="ln"><span>FIND</span><input id="search" type="search" placeholder="NAZWA..." autocomplete="off"></div>
 <p style="margin-top:8px">0 <span style="background:var(--txt);color:var(--scr)">"C64 SID PLAYER  " 64 2A</span></p>
 <ul id="list"></ul>
 <p id="blocks"></p>
 <p id="msg"></p>
 <p>READY.</p>
 <p><span class="cur"></span></p>
</div>
<script>
const $=id=>document.getElementById(id);
let st={},files=[],msgT;
const esc=s=>String(s).replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
const blk=b=>Math.ceil((b||0)/254);
const fmt=ms=>{const s=Math.floor((ms||0)/1000);return Math.floor(s/60)+':'+String(s%60).padStart(2,'0')};
function msg(t,bad){const e=$('msg');e.textContent=bad?'?'+t+' ERROR':t;e.className=bad?'err wrap':'wrap';clearTimeout(msgT);msgT=setTimeout(()=>e.textContent='',4000)}
async function api(p){try{const r=await fetch('/api/'+p);const j=await r.json();if(j.error)msg(j.error,1);refresh();return j}catch(e){msg('DEVICE NOT PRESENT',1)}}

async function refresh(){
 try{st=await (await fetch('/api/status')).json()}catch(e){return}
 const has=!!st.name;
 $('load').textContent=has?`LOAD"${st.file}",8,1`:'LOAD"*",8,1';
 $('info').innerHTML=has?
  `<p>NAME:&nbsp;&nbsp; ${esc(st.name)}</p><p>AUTHOR: ${esc(st.author||'?')}</p><p>YEAR:&nbsp;&nbsp; ${esc(st.released||'?')}</p>`+
  `<p>TUNE:&nbsp;&nbsp; ${st.song}/${st.songs} &nbsp;TIME ${fmt(st.ms)}${st.state==='paused'?' &nbsp;BREAK':''}</p>`
  :'<p>SEARCHING FOR *</p><p class="dim">WYBIERZ PLIK Z LISTY ALBO WGRAJ NOWY</p>';
 $('tech').textContent=(has?`${st.type} ${st.clock} SID ${st.model} ${st.hz} HZ · `:'')+`${st.engine} · CPU ${st.load}%`;
 $('pp').textContent=st.state==='playing'?'PAUSE':'RUN';
 $('mute').classList.toggle('on',!!st.muted);
 $('mute').textContent=st.muted?'MUTED':'MUTE';
 $('prev').disabled=$('next').disabled=!has||st.songs<2;
 if(document.activeElement!==$('vol')){$('vol').value=st.volume;$('volv').textContent=st.volume}
 $('model').value=st.modelSetting;
 if(st.freeFs!=null){$('free').textContent=Math.floor(st.freeFs/254);$('blocks').textContent=Math.floor(st.freeFs/254)+' BLOCKS FREE.'}
 document.querySelectorAll('#list li').forEach(li=>li.classList.toggle('on',li.dataset.f===st.file));
}
async function loadList(){
 try{files=await (await fetch('/api/list')).json()}catch(e){return}
 render();
}
function render(){
 const q=$('search').value.toLowerCase();
 const l=files.filter(f=>f.name.toLowerCase().includes(q)).sort((a,b)=>a.name.localeCompare(b.name));
 $('list').innerHTML=l.map(f=>`<li data-f="${esc(f.name)}" class="${f.name===st.file?'on':''}">
  <span class="b">${blk(f.size)}</span><span class="n" data-act="play">"${esc(f.name.replace(/\.sid$/i,'').replace(/_/g,' '))}" SID</span>
  <button data-act="play" title="Graj">RUN</button><button class="del" data-act="del" title="Usun">DEL</button></li>`).join('')
  ||'<li class="dim">BRAK PLIKOW - WGRAJ PIERWSZY .SID</li>';
}
async function loadSkins(){
 try{const s=await (await fetch('/api/skins')).json();
  $('skin').innerHTML=s.skins.map(k=>`<option value="${esc(k.id)}">${esc(k.name)}</option>`).join('');$('skin').value=s.current}catch(e){}
}
$('skin').onchange=async e=>{await api('skin?s='+encodeURIComponent(e.target.value));location.reload()};
$('list').onclick=e=>{
 const a=e.target.closest('[data-act]');if(!a)return;const f=a.closest('li').dataset.f;if(!f)return;
 if(a.dataset.act==='play')api('play?file='+encodeURIComponent(f));
 else if(confirm('SCRATCH "'+f+'"?'))api('delete?file='+encodeURIComponent(f)).then(loadList);
};
$('search').oninput=render;
$('pp').onclick=()=>st.loaded?api('pause'):(files.length?api('play?index=1'):msg('FILE NOT FOUND',1));
$('stop').onclick=()=>api('stop');
$('mute').onclick=()=>api('mute');
$('fprev').onclick=()=>api('file?d=-1');
$('fnext').onclick=()=>api('file?d=1');
$('prev').onclick=()=>api('song?d=-1');
$('next').onclick=()=>api('song?d=1');
let vt;$('vol').oninput=e=>{$('volv').textContent=e.target.value;clearTimeout(vt);vt=setTimeout(()=>api('volume?v='+e.target.value),120)};
$('model').onchange=e=>api('model?m='+e.target.value);

// --- wgrywanie ---
const drop=$('drop'),fin=$('fileIn');
drop.onclick=()=>fin.click();
drop.onkeydown=e=>{if(e.key==='Enter'||e.key===' '){e.preventDefault();fin.click()}};
['dragenter','dragover'].forEach(ev=>drop.addEventListener(ev,e=>{e.preventDefault();drop.classList.add('over')}));
['dragleave','drop'].forEach(ev=>drop.addEventListener(ev,e=>{e.preventDefault();drop.classList.remove('over')}));
drop.addEventListener('drop',e=>upload([...e.dataTransfer.files]));
fin.onchange=()=>{upload([...fin.files]);fin.value=''};
function send(f,onp){return new Promise(res=>{
 const x=new XMLHttpRequest(),d=new FormData();d.append('file',f,f.name);
 x.upload.onprogress=e=>e.lengthComputable&&onp(e.loaded/e.total);
 x.onload=()=>{let j={};try{j=JSON.parse(x.responseText)}catch(e){}res(j.error||(x.status==200?'':'BLAD '+x.status))};
 x.onerror=()=>res('DEVICE NOT PRESENT');
 x.open('POST','/upload');x.send(d)})}
async function upload(list){
 const sids=list.filter(f=>/\.sid$/i.test(f.name)),skip=list.length-sids.length;
 if(!sids.length){msg('FILE TYPE MISMATCH',1);return}
 const bar=$('bar'),bi=$('barI');bar.style.display='block';let ok=0,errs=[];
 for(let i=0;i<sids.length;i++){
  const f=sids[i];$('msg').className='wrap';$('msg').textContent=`SAVING "${f.name}" ${i+1}/${sids.length}`;
  const e=await send(f,p=>bi.style.width=((i+p)/sids.length*100)+'%');
  if(e)errs.push(f.name+': '+e);else ok++;
 }
 bi.style.width='100%';setTimeout(()=>{bar.style.display='none';bi.style.width='0'},800);
 if(errs.length)msg(errs.join(' / '),1);else msg(`ZAPISANO ${ok}${skip?', POMINIETO '+skip:''}`);
 loadList();refresh();
}
loadSkins();loadList();refresh();setInterval(refresh,1000);
</script></body></html>)HTML";
