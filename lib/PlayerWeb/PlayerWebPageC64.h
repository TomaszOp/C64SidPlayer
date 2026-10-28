/**
 * @file PlayerWebPageC64.h
 * @brief "Commodore 64" skin: a page styled like the C64 screen (terminal font,
 *        blue screen with a border, file list like LOAD"$",8).
 *
 * Uses the same /api/... as PlayerWebPage.h.
 * Texts in Polish and English: dictionary L in the script, languages in PlayerWebLangs.h.
 */
#pragma once
#include <pgmspace.h>

/// Complete HTML page of the Commodore 64 skin, served from flash (PROGMEM).
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
li .t{min-width:4ch}
#md{min-height:32px;padding:0 6px;margin-left:8px}
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
  <button id="fprev" title="Poprzedni plik" data-it="fprev">&lt;F</button>
  <button id="prev" title="Poprzedni podutwor" data-it="prev">&lt;T</button>
  <button id="pp" title="Graj / pauza" data-it="pp">RUN</button>
  <button id="next" title="Nastepny podutwor" data-it="next">T&gt;</button>
  <button id="fnext" title="Nastepny plik" data-it="fnext">F&gt;</button>
 </div>
 <div class="ctl ctl2">
  <button id="stop" title="Stop">STOP</button>
  <button id="mute" title="Wycisz" data-it="mute">MUTE</button>
 </div>
 <div class="ln"><span>VOLUME</span><input type="range" id="vol" min="0" max="100"><span id="volv"></span></div>
 <div class="ln"><span>SID</span>
  <select id="model"><option value="-1">AUTO</option><option value="0">6581</option><option value="1">8580</option></select></div>
 <div class="ln"><span data-i="look">WYGLAD</span><select id="skin"></select></div>
 <div class="ln"><span data-i="lang">JEZYK</span><select id="lang"></select></div>

 <div class="gap"></div>
 <p>READY.</p>
 <p id="save" class="wrap">SAVE"*.SID",8</p>
 <div id="drop" tabindex="0" data-i="drop">DOTKNIJ ALBO UPUSC TU PLIKI .SID</div>
 <input type="file" id="fileIn" multiple>
 <div id="bar"><i id="barI"></i></div>

 <div class="gap"></div>
 <p>READY.</p>
 <p id="loaddir">LOAD"$",8</p>
 <div class="ln" id="srcRow"><span>DRIVE</span><select id="src"></select></div>
 <div class="ln"><span>FIND</span><input id="search" type="search" placeholder="NAZWA..." data-ip="search" autocomplete="off"></div>
 <p style="margin-top:8px" class="wrap">0 <span id="hdr" style="background:var(--txt);color:var(--scr)"></span><button id="md" data-it="md" title="Nowy folder" style="display:none">MD</button></p>
 <ul id="list"></ul>
 <button id="more" style="width:100%;margin-top:6px;display:none">MORE</button>
 <p id="blocks"></p>
 <p id="msg"></p>
 <p>READY.</p>
 <p><span class="cur"></span></p>
</div>
<script>
const $=id=>document.getElementById(id);
let st={},files=[],msgT,li={},listKey='',sources=[];
// translations: plain texts or functions with parameters; a missing key falls back to Polish
const L={
pl:{fprev:'Poprzedni plik',prev:'Poprzedni podutwor',pp:'Graj / pauza',next:'Nastepny podutwor',fnext:'Nastepny plik',mute:'Wycisz',
 look:'WYGLAD',lang:'JEZYK',drop:'DOTKNIJ ALBO UPUSC TU PLIKI .SID',search:'NAZWA...',pick:'WYBIERZ PLIK Z LISTY ALBO WGRAJ NOWY',
 open:'Otworz',play:'Graj',del:'Usun',empty:'PUSTY FOLDER',wp:'?WRITE PROTECT ERROR (WGRYWANIE WYLACZONE)',na:' (BRAK)',err:n=>'BLAD '+n,
 saved:(n,k)=>`ZAPISANO ${n}${k?', POMINIETO '+k:''}`,md:'Nowy folder',mdQ:'MD: NAZWA FOLDERU'},
en:{fprev:'Previous file',prev:'Previous subtune',pp:'Play / pause',next:'Next subtune',fnext:'Next file',mute:'Mute',
 look:'LOOK',lang:'LANGUAGE',drop:'TAP OR DROP .SID FILES HERE',search:'NAME...',pick:'CHOOSE A FILE FROM THE LIST OR UPLOAD A NEW ONE',
 open:'Open',play:'Play',del:'Delete',empty:'EMPTY FOLDER',wp:'?WRITE PROTECT ERROR',na:' (NOT PRESENT)',err:n=>'ERROR '+n,
 saved:(n,k)=>`SAVED ${n}${k?', SKIPPED '+k:''}`,md:'New folder',mdQ:'MD: FOLDER NAME',skin_modern:'Modern',src_flash:'Flash memory',src_sd:'SD card'}};
// the server sends the saved language in a cookie with the page, so it is right before the first draw
let lang=(document.cookie.match(/(?:^|;\s*)lang=(\w+)/)||[])[1];if(!L[lang])lang='pl';
const T=(k,...a)=>{const v=k in L[lang]?L[lang][k]:L.pl[k];return typeof v==='function'?v(...a):v};
function applyLang(){
 document.documentElement.lang=lang;
 document.querySelectorAll('[data-i]').forEach(e=>e.textContent=T(e.dataset.i));
 document.querySelectorAll('[data-it]').forEach(e=>e.title=T(e.dataset.it));
 document.querySelectorAll('[data-ip]').forEach(e=>e.placeholder=T(e.dataset.ip));
 $('lang').value=lang;
}
applyLang();
const esc=s=>String(s).replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
const blk=b=>Math.ceil((b||0)/254);
const fmt=ms=>{const s=Math.floor((ms||0)/1000);return Math.floor(s/60)+':'+String(s%60).padStart(2,'0')};
function msg(t,bad){const e=$('msg');e.textContent=bad?'?'+t+' ERROR':t;e.className=bad?'err wrap':'wrap';clearTimeout(msgT);msgT=setTimeout(()=>e.textContent='',4000)}
async function api(p){try{const r=await fetch('/api/'+p);const j=await r.json();if(j.error)msg(j.error,1);refresh();return j}catch(e){msg('DEVICE NOT PRESENT',1)}}

let polling=0;
async function refresh(){
 // only one status request at a time: while the ESP is busy (e.g. a DeepSid listing) the polls
 // would pile up beyond what the ESP accepts and the browser would retry them after seconds
 if(polling){polling=2;return}polling=1;
 // 3 s limit: a request stuck while the ESP was restarting must not stop the polling
 try{st=await (await fetch('/api/status',{signal:AbortSignal.timeout(3000)})).json()}catch(e){polling=0;return}
 if(polling==2)setTimeout(refresh,0);polling=0;
 // the ESP answers again - load what failed while it was unreachable (e.g. right after flashing)
 if(!sources.length)loadSources();
 // language changed here or on another device
 if(st.lang&&st.lang!==lang&&L[st.lang]){lang=st.lang;applyLang();loadSkins();loadSources()}
 const has=!!st.name;
 $('load').textContent=has?`LOAD"${st.file}",8,1`:'LOAD"*",8,1';
 $('info').innerHTML=has?
  `<p>NAME:&nbsp;&nbsp; ${esc(st.name)}</p><p>AUTHOR: ${esc(st.author||'?')}</p><p>YEAR:&nbsp;&nbsp; ${esc(st.released||'?')}</p>`+
  `<p>TUNE:&nbsp;&nbsp; ${st.song}/${st.songs} &nbsp;TIME ${fmt(st.ms)}${st.state==='paused'?' &nbsp;BREAK':''}</p>`
  :'<p>SEARCHING FOR *</p><p class="dim">'+esc(T('pick'))+'</p>';
 $('tech').textContent=(has?`${st.type} ${st.clock} SID ${st.model} ${st.hz} HZ · `:'')+`${st.engine} · CPU ${st.load}%`;
 $('pp').textContent=st.state==='playing'?'PAUSE':'RUN';
 $('mute').classList.toggle('on',!!st.muted);
 $('mute').textContent=st.muted?'MUTED':'MUTE';
 $('prev').disabled=$('next').disabled=!has||st.songs<2;
 if(document.activeElement!==$('vol')){$('vol').value=st.volume;$('volv').textContent=st.volume}
 $('model').value=st.modelSetting;
 if(st.freeFs!=null){const fb=Math.min(65535,Math.floor(st.freeFs/254));$('free').textContent=fb;$('blocks').textContent=fb+' BLOCKS FREE.'}
 else{$('free').textContent='0';$('blocks').textContent=''}
 document.querySelectorAll('#list li').forEach(l=>l.classList.toggle('on',st.fileSource===st.source&&l.dataset.f===st.file));
 if(st.source&&st.source+':'+st.dir!==listKey){listKey=st.source+':'+st.dir;loadList()}
}
// lista biezacego folderu, w porcjach po 100
async function loadList(more){
 let j;try{j=await (await fetch('/api/list?count=100&offset='+(more?files.length:0))).json()}catch(e){if(!more)listKey='';return}   // '' = retry on the next refresh()
 files=more?files.concat(j.items):j.items;li=j;listKey=j.source+':'+j.dir;
 if(!more)$('search').value='';
 render();
}
const full=n=>(li.dir==='/'?'/':li.dir+'/')+n;
// numer stacji jak w C64: 8 = pamiec flash, 9 = karta SD...
const drive=id=>8+Math.max(0,sources.findIndex(s=>s.id===id));
function render(){
 const q=$('search').value.toLowerCase();
 const l=files.filter(f=>f.name.toLowerCase().includes(q));
 const dv=drive(li.source);
 $('loaddir').textContent=`LOAD"$${li.dir&&li.dir!=='/'?':'+li.dir:''}",${dv}`;
 $('hdr').textContent=`"${(li.source||'')+':'+(li.dir||'/')}" ${dv} 2A`;
 let h=li.dir&&li.dir!=='/'?'<li data-act="up"><span class="b">0</span><span class="n" data-act="up">".."</span><span class="t">DIR</span></li>':'';
 h+=l.map(f=>f.dir?`<li data-d="${esc(f.name)}"><span class="b">0</span><span class="n" data-act="cd">"${esc(f.name)}"</span><span class="t">DIR</span>
  <button data-act="cd" title="${T('open')}">CD</button>${li.del?`<button class="del" data-act="rd" title="${T('del')}">DEL</button>`:''}</li>`
  :`<li data-f="${esc(full(f.name))}" class="${st.fileSource===st.source&&full(f.name)===st.file?'on':''}">
  <span class="b">${blk(f.size)}</span><span class="n" data-act="play">"${esc(f.name.replace(/\.sid$/i,'').replace(/_/g,' '))}"</span><span class="t">SID</span>
  <button data-act="play" title="${T('play')}">RUN</button>${li.del?`<button class="del" data-act="del" title="${T('del')}">DEL</button>`:''}</li>`).join('');
 if(li.ready===false)h+=`<li class="err">?${esc((li.error||'DEVICE NOT PRESENT').toUpperCase())} ERROR</li>`;
 else if(!files.length)h+='<li class="dim">'+T('empty')+'</li>';
 $('list').innerHTML=h;
 $('more').style.display=files.length<li.total?'block':'none';
 $('more').textContent=`MORE (${files.length}/${li.total})`;
 $('drop').style.display=li.upload?'block':'none';
 $('md').style.display=li.upload?'':'none';
 $('save').textContent=li.upload?`SAVE"${li.dir==='/'?'':li.dir+'/'}*.SID",${dv}`:T('wp');
}
async function loadSources(){
 try{const j=await (await fetch('/api/sources')).json();sources=j.sources;
  $('src').innerHTML=sources.map((s,i)=>`<option value="${esc(s.id)}">${8+i}: ${esc(T('src_'+s.id)||s.name)}${s.ready?'':T('na')}</option>`).join('');
  $('src').value=j.current;$('srcRow').style.display=sources.length>1?'flex':'none';render()}catch(e){}
}
$('src').onchange=e=>api('source?s='+encodeURIComponent(e.target.value)).then(loadSources);
$('more').onclick=()=>loadList(true);
$('md').onclick=()=>{const n=prompt(T('mdQ'));if(n&&n.trim())api('mkdir?name='+encodeURIComponent(n.trim())).then(()=>loadList())};
async function loadSkins(){
 try{const s=await (await fetch('/api/skins')).json();
  $('skin').innerHTML=s.skins.map(k=>`<option value="${esc(k.id)}">${esc(T('skin_'+k.id)||k.name)}</option>`).join('');$('skin').value=s.current}catch(e){}
}
$('skin').onchange=async e=>{await api('skin?s='+encodeURIComponent(e.target.value));location.reload()};
async function loadLangs(){
 try{const s=await (await fetch('/api/langs')).json();
  $('lang').innerHTML=s.langs.map(k=>`<option value="${esc(k.id)}">${esc(k.name)}</option>`).join('');$('lang').value=lang}catch(e){}
}
$('lang').onchange=e=>api('lang?l='+encodeURIComponent(e.target.value));
$('list').onclick=e=>{
 const a=e.target.closest('[data-act]');if(!a)return;const r=a.closest('li'),act=a.dataset.act;
 if(act==='up')return api('cd?dir=..');
 if(act==='cd')return api('cd?dir='+encodeURIComponent(r.dataset.d));
 if(act==='rd')return api('rmdir?dir='+encodeURIComponent(r.dataset.d)).then(()=>loadList());
 const f=r.dataset.f;if(!f)return;
 if(act==='play')api('play?file='+encodeURIComponent(f));
 else if(confirm('SCRATCH "'+f+'"?'))api('delete?file='+encodeURIComponent(f)).then(()=>loadList());
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
 x.onload=()=>{let j={};try{j=JSON.parse(x.responseText)}catch(e){}res(j.error||(x.status==200?'':T('err',x.status)))};
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
 if(errs.length)msg(errs.join(' / '),1);else msg(T('saved',ok,skip));
 loadList();refresh();
}
loadSkins();loadLangs();loadSources();refresh();setInterval(refresh,1000);
</script></body></html>)HTML";
