/**
 * @file PlayerWebPage.h
 * @brief Default skin "Nowoczesny" (Modern): the player web page (HTML + CSS + JS in one string).
 *
 * Other skins: PlayerWebSkins.h. The page talks to PlayerWeb through /api/... (JSON).
 * Texts in Polish and English: dictionary L in the script, languages in PlayerWebLangs.h.
 */
#pragma once
#include <pgmspace.h>

/// Complete HTML page of the Modern skin, served from flash (PROGMEM).
static const char PLAYER_WEB_PAGE[] PROGMEM = R"HTML(<!doctype html>
<html lang="pl"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="theme-color" content="#352879">
<title>C64 SID Player</title>
<style>
:root{--bg:#352879;--card:#40318d;--accent:#7869c4;--hi:#a59de8;--txt:#e8e6ff;--ok:#6fdc8c;--err:#ff7a7a}
*{box-sizing:border-box}
html{-webkit-text-size-adjust:100%}
body{margin:0;background:var(--bg);color:var(--txt);font:16px/1.4 system-ui,-apple-system,"Segoe UI",Roboto,sans-serif;
 padding:12px max(12px,env(safe-area-inset-right)) 24px max(12px,env(safe-area-inset-left))}
.wrap{max-width:960px;margin:auto;display:grid;grid-template-columns:minmax(0,1fr);gap:12px}
@media(min-width:820px){.wrap{grid-template-columns:minmax(0,1fr) minmax(0,1fr)}.full{grid-column:1/-1}}
h1{font:700 18px/1 ui-monospace,Menlo,Consolas,monospace;letter-spacing:1px;margin:4px 0 0;text-align:center;color:var(--hi)}
h2{font-size:14px;text-transform:uppercase;letter-spacing:1px;color:var(--hi);margin:0 0 10px}
.card{background:var(--card);border-radius:14px;padding:14px;box-shadow:0 2px 10px rgba(0,0,0,.25);min-width:0}
#title{font-size:20px;font-weight:700;word-break:break-word}
#meta{color:var(--hi);font-size:14px;margin-top:4px;word-break:break-word}
#tech{font:12px ui-monospace,Menlo,Consolas,monospace;color:var(--hi);opacity:.8;margin-top:6px}
.row{display:flex;gap:8px;align-items:center;flex-wrap:wrap}
button,select{font:inherit;color:#fff;background:var(--accent);border:0;border-radius:10px;min-height:48px;padding:0 16px;cursor:pointer;touch-action:manipulation}
button:active{filter:brightness(1.25)}
button:disabled{opacity:.4}
.ctl{display:grid;grid-template-columns:1fr 1fr 1.4fr 1fr 1fr;gap:6px;margin-top:12px}
.ctl2{grid-template-columns:1fr 1fr;margin-top:6px}
.ctl button{font-size:20px;padding:0 4px;display:flex;flex-direction:column;align-items:center;justify-content:center;line-height:1.1}
.ctl button small{font-size:10px;opacity:.75;margin-top:2px}
.ctl button.on{background:var(--err)}
#song{text-align:center;font-weight:700;margin-top:8px}
.vol{display:flex;align-items:center;gap:10px;margin-top:12px}
.vol span{min-width:44px;text-align:right;font-variant-numeric:tabular-nums}
input[type=range]{flex:1;height:32px;accent-color:var(--hi)}
select{flex:1;background:var(--bg)}
#drop{border:2px dashed var(--accent);border-radius:12px;padding:22px 12px;text-align:center;cursor:pointer;transition:.15s}
#drop.over{background:var(--accent);border-color:var(--hi)}
#drop b{display:block;font-size:17px;margin-bottom:4px}
#drop small{color:var(--hi)}
#fileIn{display:none}
.bar{height:10px;border-radius:5px;background:var(--bg);overflow:hidden;margin-top:10px;display:none}
.bar i{display:block;height:100%;width:0;background:var(--ok);transition:width .1s}
#upmsg{margin-top:8px;min-height:1.4em;font-size:14px;word-break:break-word}
.err{color:var(--err)}.ok{color:var(--ok)}
#search{width:100%;font:inherit;color:var(--txt);background:var(--bg);border:1px solid var(--accent);border-radius:10px;min-height:44px;padding:0 12px;margin-bottom:8px}
ul{list-style:none;margin:0;padding:0}
li{display:flex;align-items:center;gap:8px;padding:6px 4px;border-top:1px solid rgba(120,105,196,.4)}
li .n{flex:1;min-width:0;cursor:pointer;padding:8px 0}
li .n div{white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
li .n small{color:var(--hi)}
li.on .n div{color:var(--ok);font-weight:700}
li button{min-height:44px;min-width:48px;padding:0 12px}
li .del{background:transparent;border:1px solid var(--accent)}
#free{font-size:12px;color:var(--hi);margin-top:8px;text-align:right}
#path{font:14px ui-monospace,Menlo,Consolas,monospace;color:var(--hi);margin-bottom:8px;word-break:break-all}
#upto{font-size:14px;color:var(--hi);margin-bottom:8px;word-break:break-word}
li.dir .n div,li.up .n div{font-weight:700}
li.dir .n div::before,li.up .n div::before{content:"\1F4C1  "}
li.dir button{background:transparent;border:1px solid var(--accent)}
#toast{position:fixed;left:50%;bottom:calc(16px + env(safe-area-inset-bottom));transform:translateX(-50%);background:#000c;color:#fff;padding:10px 16px;border-radius:10px;opacity:0;transition:.2s;pointer-events:none;max-width:90vw}
#toast.show{opacity:1}
</style></head><body>
<div class="wrap">
 <h1 class="full">**** C64 SID PLAYER ****</h1>

 <section class="card">
  <h2 data-i="now">Teraz gra</h2>
  <div id="title">READY.</div>
  <div id="meta"></div>
  <div id="tech"></div>
  <div class="ctl">
   <button id="fprev" title="Poprzedni plik" data-it="fprev">&#9198;<small data-i="file">plik</small></button>
   <button id="prev" title="Poprzedni podutwór" data-it="prev">&#9664;&#9664;<small data-i="tune">utwór</small></button>
   <button id="pp" title="Graj / pauza" data-it="pp">&#9654;</button>
   <button id="next" title="Następny podutwór" data-it="next">&#9654;&#9654;<small data-i="tune">utwór</small></button>
   <button id="fnext" title="Następny plik" data-it="fnext">&#9197;<small data-i="file">plik</small></button>
  </div>
  <div class="ctl ctl2">
   <button id="stop" title="Stop" data-it="stop">&#9632;<small data-i="stop">stop</small></button>
   <button id="mute" title="Wycisz" data-it="mute">&#128264;<small data-i="mute">wycisz</small></button>
  </div>
  <div id="song"></div>
  <div class="vol"><span>&#128264;</span><input type="range" id="vol" min="0" max="100"><span id="volv"></span></div>
  <div class="row" style="margin-top:12px"><span data-i="chip">Układ SID</span>
   <select id="model"><option value="-1" data-i="auto">automatycznie</option><option value="0" data-i="m6581">6581 (stary)</option><option value="1" data-i="m8580">8580 (nowy)</option></select></div>
 <div class="row" style="margin-top:8px"><span data-i="look">Wygląd</span><select id="skin"></select></div>
 <div class="row" style="margin-top:8px"><span data-i="lang">Język</span><select id="lang"></select></div>
 </section>

 <section class="card">
  <h2 data-i="upload">Wgraj muzykę</h2>
  <div id="upto"></div>
  <div id="drop" tabindex="0"><b data-i="dropB">Dotknij, aby wybrać pliki .sid</b><small data-i="dropS">lub przeciągnij je tutaj z komputera</small></div>
  <input type="file" id="fileIn" multiple>
  <div class="bar" id="bar"><i id="barI"></i></div>
  <div id="upmsg"></div>
 </section>

 <section class="card full">
  <h2><span data-i="files">Pliki</span> <span id="count"></span></h2>
  <div class="row" id="srcRow" style="margin-bottom:8px"><span data-i="source">Źródło</span><select id="src"></select></div>
  <div id="path"></div>
  <input id="search" type="search" placeholder="Szukaj w folderze…" data-ip="search" autocomplete="off">
  <ul id="list"></ul>
  <button id="more" style="width:100%;margin-top:8px;display:none">Pokaż więcej</button>
  <div id="free"></div>
 </section>
</div>
<div id="toast"></div>
<script>
const $=id=>document.getElementById(id);
let st={},files=[],toastT,li={},listKey='',sources=[];
// translations: plain texts or functions with parameters; a missing key falls back to Polish
const L={
pl:{now:'Teraz gra',file:'plik',tune:'utwór',stop:'stop',mute:'wycisz',fprev:'Poprzedni plik',prev:'Poprzedni podutwór',pp:'Graj / pauza',
 next:'Następny podutwór',fnext:'Następny plik',chip:'Układ SID',auto:'automatycznie',m6581:'6581 (stary)',m8580:'8580 (nowy)',look:'Wygląd',lang:'Język',
 upload:'Wgraj muzykę',dropB:'Dotknij, aby wybrać pliki .sid',dropS:'lub przeciągnij je tutaj z komputera',files:'Pliki',source:'Źródło',search:'Szukaj w folderze…',
 noConn:'Brak połączenia z ESP32',pick:'Wybierz plik z listy albo wgraj nowy.',song:(a,b,t)=>`Utwór ${a} / ${b} · ${t}`,free:'Wolne miejsce: ',
 up:'poziom wyżej',folder:'folder',open:'Otwórz',play:'Graj',del:'Usuń',srcNA:'Źródło niedostępne',empty:'Pusty folder',emptyUp:' – wgraj pierwszy plik .sid',
 more:(a,b)=>`Pokaż więcej (${a} z ${b})`,upTo:'Pliki trafią do: ',upOff:'Wgrywanie do tego źródła jest wyłączone.',na:' (niedostępne)',
 delQ:f=>'Usunąć '+f+'?',first:'Najpierw wgraj plik .sid',err:n=>'błąd '+n,lost:'brak połączenia',onlySid:'Wybierz pliki z rozszerzeniem .sid',
 sending:(i,n,f)=>`Wysyłam ${i}/${n}: ${f}`,done:n=>`Wgrano ${n} plik(ów) – gra ostatni.`,skipped:n=>`Pominięto ${n} (nie .sid). `},
en:{now:'Now playing',file:'file',tune:'tune',stop:'stop',mute:'mute',fprev:'Previous file',prev:'Previous subtune',pp:'Play / pause',
 next:'Next subtune',fnext:'Next file',chip:'SID chip',auto:'automatic',m6581:'6581 (old)',m8580:'8580 (new)',look:'Look',lang:'Language',
 upload:'Upload music',dropB:'Tap to choose .sid files',dropS:'or drag them here from your computer',files:'Files',source:'Source',search:'Search in folder…',
 noConn:'No connection to the ESP32',pick:'Choose a file from the list or upload a new one.',song:(a,b,t)=>`Tune ${a} / ${b} · ${t}`,free:'Free space: ',
 up:'up one level',folder:'folder',open:'Open',play:'Play',del:'Delete',srcNA:'Source unavailable',empty:'Empty folder',emptyUp:' – upload the first .sid file',
 more:(a,b)=>`Show more (${a} of ${b})`,upTo:'Files will go to: ',upOff:'Uploading to this source is disabled.',na:' (unavailable)',
 delQ:f=>'Delete '+f+'?',first:'Upload a .sid file first',err:n=>'error '+n,lost:'no connection',onlySid:'Choose files with the .sid extension',
 sending:(i,n,f)=>`Sending ${i}/${n}: ${f}`,done:n=>`Uploaded ${n} file(s) – playing the last one.`,skipped:n=>`Skipped ${n} (not .sid). `,
 skin_modern:'Modern',src_flash:'Flash memory',src_sd:'SD card'}};
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
function toast(t,bad){const e=$('toast');e.textContent=t;e.style.color=bad?'var(--err)':'#fff';e.classList.add('show');clearTimeout(toastT);toastT=setTimeout(()=>e.classList.remove('show'),2500)}
async function api(p){try{const r=await fetch('/api/'+p);const j=await r.json();if(j.error)toast(j.error,1);refresh();return j}catch(e){toast(T('noConn'),1)}}
const fmt=ms=>{const s=Math.floor((ms||0)/1000);return Math.floor(s/60)+':'+String(s%60).padStart(2,'0')};
const kb=b=>b<1024?b+' B':b<1048576?(b/1024).toFixed(1)+' KB':b<1073741824?(b/1048576).toFixed(1)+' MB':(b/1073741824).toFixed(1)+' GB';

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
 $('title').textContent=has?(st.name||st.file):'READY.';
 $('meta').textContent=has?[st.author,st.released].filter(Boolean).join(' · '):T('pick');
 $('tech').textContent=(has?`${st.type} ${st.clock} · SID ${st.model} · ${st.hz} Hz · `:'')+`${st.engine} · CPU ${st.load}%`;
 $('song').textContent=has?T('song',st.song,st.songs,fmt(st.ms)):'';
 $('pp').innerHTML=st.state==='playing'?'&#10074;&#10074;':'&#9654;';
 $('mute').classList.toggle('on',!!st.muted);
 $('prev').disabled=$('next').disabled=!has||st.songs<2;
 if(document.activeElement!==$('vol')){$('vol').value=st.volume;$('volv').textContent=st.volume+'%'}
 $('model').value=st.modelSetting;
 $('free').textContent=st.freeFs!=null?T('free')+kb(st.freeFs):'';
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
const srcName=id=>T('src_'+id)||(sources.find(s=>s.id===id)||{name:id}).name;
function render(){
 const q=$('search').value.toLowerCase();
 const l=files.filter(f=>f.name.toLowerCase().includes(q));
 $('count').textContent='('+(li.total||0)+')';
 $('path').textContent=srcName(li.source)+': '+(li.dir||'/');
 let h=li.dir&&li.dir!=='/'?'<li class="up"><div class="n" data-act="up"><div>..</div><small>'+T('up')+'</small></div></li>':'';
 h+=l.map(f=>f.dir?`<li class="dir" data-d="${esc(f.name)}"><div class="n" data-act="cd"><div>${esc(f.name)}</div><small>${T('folder')}</small></div>
  <button data-act="cd" title="${T('open')}">&#10095;</button></li>`
  :`<li data-f="${esc(full(f.name))}" class="${st.fileSource===st.source&&full(f.name)===st.file?'on':''}">
  <div class="n" data-act="play"><div>${esc(f.name.replace(/\.sid$/i,'').replace(/_/g,' '))}</div><small>${kb(f.size)}</small></div>
  <button data-act="play" title="${T('play')}">&#9654;</button>${li.del?`<button class="del" data-act="del" title="${T('del')}">&#128465;</button>`:''}</li>`).join('');
 if(li.ready===false)h+=`<li><div class="n err">${esc(li.error||T('srcNA'))}</div></li>`;
 else if(!files.length)h+='<li><div class="n">'+T('empty')+(li.upload?T('emptyUp'):'')+'</div></li>';
 $('list').innerHTML=h;
 $('more').style.display=files.length<li.total?'block':'none';
 $('more').textContent=T('more',files.length,li.total);
 $('drop').style.display=li.upload?'block':'none';
 $('upto').innerHTML=li.upload?esc(T('upTo'))+'<b>'+esc(srcName(li.source)+': '+li.dir)+'</b>':esc(T('upOff'));
}
async function loadSources(){
 try{const j=await (await fetch('/api/sources')).json();sources=j.sources;
  $('src').innerHTML=sources.map(s=>`<option value="${esc(s.id)}">${esc(srcName(s.id))}${s.ready?'':T('na')}</option>`).join('');
  $('src').value=j.current;$('srcRow').style.display=sources.length>1?'flex':'none';render()}catch(e){}
}
$('src').onchange=e=>api('source?s='+encodeURIComponent(e.target.value)).then(loadSources);
$('more').onclick=()=>loadList(true);
$('list').onclick=e=>{
 const a=e.target.closest('[data-act]');if(!a)return;const r=a.closest('li'),act=a.dataset.act;
 if(act==='up')return api('cd?dir=..');
 if(act==='cd')return api('cd?dir='+encodeURIComponent(r.dataset.d));
 const f=r.dataset.f;if(!f)return;
 if(act==='play')api('play?file='+encodeURIComponent(f));
 else if(confirm(T('delQ',f)))api('delete?file='+encodeURIComponent(f)).then(()=>loadList());
};
$('search').oninput=render;
$('pp').onclick=()=>st.loaded?api('pause'):(files.length?api('play?index=1'):toast(T('first')));
$('stop').onclick=()=>api('stop');
$('mute').onclick=()=>api('mute');
$('fprev').onclick=()=>api('file?d=-1');
$('fnext').onclick=()=>api('file?d=1');
$('prev').onclick=()=>api('song?d=-1');
$('next').onclick=()=>api('song?d=1');
let vt;$('vol').oninput=e=>{$('volv').textContent=e.target.value+'%';clearTimeout(vt);vt=setTimeout(()=>api('volume?v='+e.target.value),120)};
$('model').onchange=e=>api('model?m='+e.target.value);
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
 x.onerror=()=>res(T('lost'));
 x.open('POST','/upload');x.send(d)})}
async function upload(list){
 const sids=list.filter(f=>/\.sid$/i.test(f.name)),skip=list.length-sids.length;
 if(!sids.length){$('upmsg').innerHTML='<span class=err>'+esc(T('onlySid'))+'</span>';return}
 const bar=$('bar'),bi=$('barI');bar.style.display='block';let ok=0,errs=[];
 for(let i=0;i<sids.length;i++){
  const f=sids[i];$('upmsg').textContent=T('sending',i+1,sids.length,f.name);
  const e=await send(f,p=>bi.style.width=((i+p)/sids.length*100)+'%');
  if(e)errs.push(f.name+': '+e);else ok++;
 }
 bi.style.width='100%';setTimeout(()=>{bar.style.display='none';bi.style.width='0'},800);
 $('upmsg').innerHTML=(ok?'<span class=ok>'+esc(T('done',ok))+'</span> ':'')+
  (skip?esc(T('skipped',skip)):'')+errs.map(e=>'<div class=err>'+esc(e)+'</div>').join('');
 loadList();refresh();
}
loadSkins();loadLangs();loadSources();refresh();setInterval(refresh,1000);
</script></body></html>)HTML";
