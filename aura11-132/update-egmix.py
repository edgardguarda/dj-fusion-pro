#!/usr/bin/env python3
"""EG MIX 11.3.2 deterministic Android post-build source update.
Reconstructs Android assets from bundled offline PCM and adds native one-shots.
"""
from pathlib import Path
import sys, re, base64, json, wave
r=Path(sys.argv[1])
def edit(path, transform):
    p=r/path
    old=p.read_text(encoding="utf-8")
    new=transform(old)
    if old==new: raise RuntimeError("No modifications in "+path)
    p.write_text(new,encoding="utf-8")
def apply(s,pairs):
    for a,b in pairs:
        if a not in s: raise RuntimeError("Missing anchor "+a[:70])
        s=s.replace(a,b)
    return s
edit("app/build.gradle", lambda s:apply(s,[
    ("versionCode 1130","versionCode 1132"),
    ("11.3.0-native-ux-fx-beta","11.3.2-egmix-native-samplers-beta"),
    ("  compileOptions {","  androidResources { noCompress += ['wav'] }\n  compileOptions {")
]))
edit("app/src/main/AndroidManifest.xml",lambda s:apply(s,[
    ('android:label="Edgard AURA DJ 11"', 'android:label="EG MIX" android:icon="@mipmap/ic_launcher" android:roundIcon="@mipmap/ic_launcher_round"')
]))
def brand(s):
    pairs=[
      ("Edgard AutoDJ — AURA 11 ANDROID","EG MIX — Android Nativo"),
      ("EDGARD <b>AUTODJ</b>","EG <b>MIX</b>"),
      ("AURA / 11 · ANDROID NATIVO","PRO DJ / ANDROID NATIVO"),
      ('aria-label="Sampler Deejay Edgard Guarda"','aria-label="Sampler DeeJay Edgard Guarda em voz de robô"'),
      ("<b>01 · VINHETA</b><small>Deejay Edgard Guarda</small>","<b>01 · VINHETA</b><small>Voz robô · Edgard Guarda</small>"),
      ("<b>02 · BACKSPIN</b><small>Vinil ao vivo</small>","<b>02 · BACKSPIN</b><small>Padrão club</small>"),
      ("<b>03 · TAPE STOP</b><small>Brake ao vivo</small>","<b>03 · TAPE STOP</b><small>Brake profissional</small>"),
      ("<b>04 · SCRATCH</b><small>Scratch ao vivo</small>","<b>04 · SCRATCH</b><small>Scratch clássico</small>")
    ]
    for a,b in pairs: s=s.replace(a,b)
    return s
edit("app/src/main/assets/aura-fast.html",brand)
edit("app/src/main/assets/aura.html",lambda s:brand(s).replace('value="70" aria-label="Volume dos samplers"/><output id="samplerVolumeValue">70%</output>', 'value="78" aria-label="Volume dos samplers"/><output id="samplerVolumeValue">78%</output>'))
# Generate the four pre-existing, original, offline PCM one-shots as Android assets.
html=(r/"app/src/main/assets/aura.html").read_text()
match=re.search(r'const SAMPLER_DATA=(\{.*?\});\s*function samplerPCM',html,re.S)
if not match: raise RuntimeError("Sampler dataset absent")
samples=json.loads(match.group(1))
folder=r/"app/src/main/assets/samplers";folder.mkdir(exist_ok=True)
for key in ("vinheta","backspin","tapestop","scratch"):
    pcm=base64.b64decode(samples[key]["pcm"],validate=True)
    if len(pcm)<1000 or len(pcm)%2: raise RuntimeError("Invalid sampler "+key)
    with wave.open(str(folder/(key+".wav")),"wb") as w:
        w.setnchannels(1);w.setsampwidth(2);w.setframerate(samples[key]["sr"]);w.writeframes(pcm)
# Native resource vector (adaptive icon) follows the pulse in EG MIX logo.
res=r/"app/src/main/res"
for d in ("mipmap-anydpi-v26","drawable","values"):
    (res/d).mkdir(exist_ok=True)
(res/"values/egmix_colors.xml").write_text('<resources><color name="egmix_icon_background">#061F17</color></resources>\n')
(res/"drawable/egmix_launcher_foreground.xml").write_text('''<?xml version="1.0" encoding="utf-8"?>
<vector xmlns:android="http://schemas.android.com/apk/res/android" android:width="108dp" android:height="108dp" android:viewportWidth="108" android:viewportHeight="108">
<path android:fillColor="#09291F" android:pathData="M54,11 A43,43 0,1 1,54 97 A43,43 0,1 1,54 11"/>
<path android:fillColor="@android:color/transparent" android:strokeColor="#36AC79" android:strokeWidth="2.5" android:pathData="M54,18 A36,36 0,1 1,54 90 A36,36 0,1 1,54 18"/>
<path android:fillColor="@android:color/transparent" android:strokeColor="#77FFC0" android:strokeWidth="4.2" android:strokeLineCap="round" android:strokeLineJoin="round" android:pathData="M18,56 L35,56 L44,40 L53,73 L65,34 L75,56 L90,56"/>
</vector>
''')
icon='''<?xml version="1.0" encoding="utf-8"?>
<adaptive-icon xmlns:android="http://schemas.android.com/apk/res/android">
<background android:drawable="@color/egmix_icon_background"/>
<foreground android:drawable="@drawable/egmix_launcher_foreground"/>
</adaptive-icon>
'''
for name in ("ic_launcher","ic_launcher_round"):
    (res/("mipmap-anydpi-v26/"+name+".xml")).write_text(icon)
core=r/"app/src/main/java/br/com/edgard/aura/core"
(core/"PcmSamplerBus.java").write_text('''package br.com.edgard.aura.core;
import java.util.concurrent.atomic.AtomicReference;
public final class PcmSamplerBus {
  public static final int OUTPUT_RATE=48000;
  private static final String[] NAMES={"vinheta","backspin","tapestop","scratch"};
  private final float[][] clips=new float[NAMES.length][];
  private final AtomicReference<Command> request=new AtomicReference<>();
  private volatile boolean ready;
  private float[] current; private int frame; private double level;
  private static final class Command {
    final int key; final double level;
    Command(int key,double level){this.key=key;this.level=level;}
  }
  public static int keyIndex(String name){
    if(name==null)return -1;
    for(int i=0;i<NAMES.length;i++)if(NAMES[i].equals(name))return i;
    return -1;
  }
  public boolean ready(){return ready;}
  public void load(float[][] decoded){
    if(decoded==null||decoded.length!=NAMES.length)throw new IllegalArgumentException("clips");
    for(int i=0;i<decoded.length;i++){
      if(decoded[i]==null||decoded[i].length==0||decoded[i].length>OUTPUT_RATE*15)throw new IllegalArgumentException("size");
      for(float value:decoded[i])if(!Float.isFinite(value)||Math.abs(value)>1.01f)throw new IllegalArgumentException("value");
      clips[i]=decoded[i].clone();
    }
    ready=true;
  }
  public boolean trigger(String name,double volume){
    int key=keyIndex(name);
    if(!ready||key<0||!Double.isFinite(volume))return false;
    request.set(new Command(key,Math.min(.38,Math.max(0,volume*.38))));
    return true;
  }
  public double next(){
    Command cmd=request.get();
    if(cmd!=null)cmd=request.getAndSet(null);
    if(cmd!=null){current=clips[cmd.key];frame=0;level=cmd.level;}
    if(current==null)return 0;
    double x=current[frame++]*level;
    double attack=Math.min(1,frame/(OUTPUT_RATE*.008));
    double release=Math.min(1,(current.length-frame)/(OUTPUT_RATE*.035));
    double y=x*Math.min(attack,Math.max(0,release));
    if(frame>=current.length){current=null;frame=0;}
    return y;
  }
  public void reset(){request.set(null);current=null;frame=0;}
}
''')
java=r/"app/src/main/java/br/com/edgard/aura"
(java/"PcmSamplerAssets.java").write_text('''package br.com.edgard.aura;
import android.content.Context;
import br.com.edgard.aura.core.PcmSamplerBus;
import java.io.InputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
final class PcmSamplerAssets {
  static final String[] KEYS={"vinheta","backspin","tapestop","scratch"};
  private PcmSamplerAssets(){}
  static float[][] load(Context context) throws IOException {
    float[][] result=new float[KEYS.length][];
    for(int i=0;i<KEYS.length;i++){
      byte[] bytes;
      try(InputStream in=context.getAssets().open("samplers/"+KEYS[i]+".wav")){
        ByteArrayOutputStream out=new ByteArrayOutputStream();
        byte[] chunk=new byte[8192];int n;
        while((n=in.read(chunk))!=-1){
          out.write(chunk,0,n);
          if(out.size()>PcmSamplerBus.OUTPUT_RATE*15*4)throw new IOException("Sampler too large");
        }
        bytes=out.toByteArray();
      }
      if(bytes.length<44||bytes[0]!='R'||bytes[1]!='I'||bytes[2]!='F'||bytes[3]!='F'||bytes[8]!='W'||bytes[9]!='A'||bytes[10]!='V'||bytes[11]!='E')throw new IOException("Invalid WAV");
      int ch=0,hz=0,bits=0,format=0,off=-1,length=-1,p=12;
      while(p+8<=bytes.length){
        int size=(bytes[p+4]&255)|((bytes[p+5]&255)<<8)|((bytes[p+6]&255)<<16)|((bytes[p+7]&255)<<24);
        if(size<0||size>bytes.length-(p+8))throw new IOException("RIFF");
        if(bytes[p]=='f'&&bytes[p+1]=='m'&&bytes[p+2]=='t'&&bytes[p+3]==' '&&size>=16){
          int start=p+8;
          format=u16(bytes,start);ch=u16(bytes,start+2);hz=u32(bytes,start+4);bits=u16(bytes,start+14);
        }else if(bytes[p]=='d'&&bytes[p+1]=='a'&&bytes[p+2]=='t'&&bytes[p+3]=='a'){off=p+8;length=size;}
        p+=8+size+(size&1);
      }
      if(format!=1||ch!=1||bits!=16||hz<16000||hz>96000||off<0||length<2||length%2!=0)throw new IOException("Unsupported WAV");
      int frames=length/2,count=(int)Math.round(frames*(double)PcmSamplerBus.OUTPUT_RATE/hz);
      if(count<=0||count>PcmSamplerBus.OUTPUT_RATE*15)throw new IOException("Length");
      float[] converted=new float[count];
      for(int j=0;j<count;j++){
        double pos=j*hz/(double)PcmSamplerBus.OUTPUT_RATE;
        int a=Math.min(frames-1,(int)pos),b=Math.min(frames-1,a+1);
        double f=pos-a;
        converted[j]=(float)((s16(bytes,off+a*2)*(1-f)+s16(bytes,off+b*2)*f)/32768.0);
      }
      result[i]=converted;
    }
    return result;
  }
  private static int u16(byte[] b,int p){return (b[p]&255)|((b[p+1]&255)<<8);}
  private static int u32(byte[] b,int p){return u16(b,p)|(u16(b,p+2)<<16);}
  private static short s16(byte[] b,int p){return (short)u16(b,p);}
}
''')
def audio(s):
  return apply(s,[
    ('    public static final String FX="br.com.edgard.aura.FX";',
     '    public static final String FX="br.com.edgard.aura.FX";\n    public static final String SAMPLER="br.com.edgard.aura.SAMPLER";\n    private volatile PcmSamplerBus pcmSamplers;'),
    ('        if(FX.equals(intent.getAction())){',
     '''        if(SAMPLER.equals(intent.getAction())){
          PcmSamplerBus bus=pcmSamplers;
          String name=intent.getStringExtra("key");
          double volume=intent.getDoubleExtra("volume",.78);
          if(!running.get()||bus==null||!bus.trigger(name,volume))
            status("Sampler nativo ainda não está pronto");
          return START_NOT_STICKY;
        }
        if(FX.equals(intent.getAction())){'''),
    ('            MasterCreativeFx creativeFx=new MasterCreativeFx();',
     '''            MasterCreativeFx creativeFx=new MasterCreativeFx();
            PcmSamplerBus samplerBus=new PcmSamplerBus();
            pcmSamplers=samplerBus;
            new Thread(()->{
              try{samplerBus.load(PcmSamplerAssets.load(getApplicationContext()));}
              catch(Exception e){Log.w("EG-MIX","Native PCM sampler loading",e);}
            },"EG-MIX-sampler-loader").start();'''),
    ('                    creativeFx.process(left,right,fxOut);\n                    dcBlocker.process(fxOut[0],fxOut[1],dc);',
     '''                    creativeFx.process(left,right,fxOut);
                    double sample=samplerBus.next();
                    dcBlocker.process(fxOut[0]+sample,fxOut[1]+sample,dc);''')
  ]).replace('AURA DJ — tocando','EG MIX — tocando')
edit("app/src/main/java/br/com/edgard/aura/AudioService.java",audio)
def activity(s):
    return apply(s,[
      ('import br.com.edgard.aura.core.PrivateAudioExport;', 'import br.com.edgard.aura.core.PrivateAudioExport;\nimport br.com.edgard.aura.core.PcmSamplerBus;'),
      ('    private WebView web;',
       '''    private WebView web;
    private android.media.SoundPool quickSamplers;
    private final java.util.Map<String,Integer> quickSamplerIds=new java.util.HashMap<>();
    private final java.util.Set<Integer> quickSamplerReady=new java.util.HashSet<>();
    private void ensureQuickSamplers(){
      if(quickSamplers!=null)return;
      try{
        android.media.AudioAttributes attr=new android.media.AudioAttributes.Builder()
          .setUsage(android.media.AudioAttributes.USAGE_MEDIA)
          .setContentType(android.media.AudioAttributes.CONTENT_TYPE_SONIFICATION).build();
        quickSamplers=new android.media.SoundPool.Builder().setMaxStreams(2).setAudioAttributes(attr).build();
        quickSamplers.setOnLoadCompleteListener((pool,id,status)->{if(status==0)quickSamplerReady.add(id);});
        for(String key:PcmSamplerAssets.KEYS){
          android.content.res.AssetFileDescriptor afd=getAssets().openFd("samplers/"+key+".wav");
          int id=quickSamplers.load(afd,1);quickSamplerIds.put(key,id);afd.close();
        }
      }catch(Exception ex){Log.w(TAG,"Quick sampler loading",ex);}
    }
    private void playSampler(String key,double volume){
      if(PcmSamplerBus.keyIndex(key)<0||!Double.isFinite(volume))return;
      if(AudioService.isPlaying()){
        Intent i=new Intent(this,AudioService.class).setAction(AudioService.SAMPLER);
        i.putExtra("key",key);i.putExtra("volume",Math.max(0,Math.min(1,volume)));
        startService(i);return;
      }
      ensureQuickSamplers();
      if(quickSamplers!=null){
        int id=quickSamplerIds.containsKey(key)?quickSamplerIds.get(key):0;
        if(id>0&&quickSamplerReady.contains(id)){
          float gain=(float)Math.max(0,Math.min(.32,volume*.32));
          quickSamplers.play(id,gain,gain,1,0,1);
        }
      }
    }'''),
      ('        setContentView(web);','        setContentView(web);\n        ensureQuickSamplers();'),
      ('        @JavascriptInterface public void diagnostic(){diagnostics();}',
       '''        @JavascriptInterface public void sampler(String key,double volume){
          main.post(()->playSampler(key,volume));
        }
        @JavascriptInterface public void diagnostic(){diagnostics();}'''),
      ('        if(web!=null){web.removeJavascriptInterface("AURA_ANDROID");web.destroy();web=null;}super.onDestroy();}',
       '''        if(quickSamplers!=null){quickSamplers.release();quickSamplers=null;quickSamplerIds.clear();quickSamplerReady.clear();}
        if(web!=null){web.removeJavascriptInterface("AURA_ANDROID");web.destroy();web=null;}super.onDestroy();}''')
    ]).replace('AURA Android nativo pronto','EG MIX Android nativo pronto')
edit("app/src/main/java/br/com/edgard/aura/MainActivity.java",activity)
def ui(s):
    old='''  /* Replace unsupported pads/sliders with one accurate action per feature. */
  body.aura10-native .sampler-pads,body.aura10-native .stem-grid{display:none!important}
  body.aura10-native .sampler-head label,body.aura10-native #samplerVolume,
  body.aura10-native #samplerVolumeValue{display:none!important}'''
    new='''  /* Native sampler pads work; offline neural stems remain unavailable. */
  body.aura10-native .stem-grid{display:none!important}
  body.aura10-native .sampler-panel{display:grid!important}
  body.aura10-native .sampler-pads{display:grid!important;grid-template-columns:repeat(2,minmax(0,1fr));gap:8px}
  body.aura10-native .sampler-pads button{min-height:56px;touch-action:manipulation}
  body.aura10-native .sampler-pads button[aria-pressed=true]{background:#2b8b5b!important;color:white;border-color:#90ffc4}
  body.aura10-native #aura10-native-tracklist .empty{min-height:150px}'''
    if old not in s:raise RuntimeError("sampler CSS anchor")
    s=s.replace(old,new).replace('AURA 11 · MIXAGEM IMEDIATA','EG MIX · PERFORMANCE IMEDIATA')
    a=s.index(' // Sampler pads in the native-only HTML have no WebAudio backend.')
    b=s.index(" announceFx('Para efeitos",a)
    s=s[:a]+''' // Four working offline Android sampler pads, limited gain.
 const samplerVolume=$('samplerVolume'),samplerOutput=$('samplerVolumeValue');
 const pads=[...document.querySelectorAll('[data-sampler]')];
 function sampleLevel(){
   if(samplerOutput)samplerOutput.textContent=String(Number(samplerVolume?.value)||0)+'%';
 }
 samplerVolume?.addEventListener('input',sampleLevel);
 sampleLevel();
 for(const pad of pads){
   pad.disabled=typeof api.sampler!=='function';
   pad.setAttribute('aria-pressed','false');
   pad.title='Sampler de performance offline';
   pad.addEventListener('click',()=>{
     if(pad.disabled)return;
     api.sampler(pad.dataset.sampler,Math.max(0,Math.min(1,(Number(samplerVolume?.value)||0)/100)));
     pad.setAttribute('aria-pressed','true');
     setTimeout(()=>pad.setAttribute('aria-pressed','false'),450);
   });
 }
 const samplerPanel=document.querySelector('.sampler-panel');
 if(samplerPanel){
   const note=document.createElement('p');note.className='aura-native-audio-note';
   note.textContent='EG MIX: samplers offline. Em Estúdio PCM entram antes do limiter; no modo rápido usam canal de volume seguro.';
   samplerPanel.appendChild(note);
 }
''' + s[b:]
    return s.replace("AURA Android pronto","EG MIX Android pronto")
edit("app/src/main/assets/native-ui.js",ui)
# Re-scope old 11.3.0 safety tests to the newly functional offline native pads.
edit("tests/check-ux-fx.py",lambda s:apply(s,[
  ("'inert native samplers disabled': \"pad.disabled=true;pad.title='Sampler WebAudio\" in native,",
   "'native samplers enabled': \"api.sampler(pad.dataset.sampler\" in native and 'PcmSamplerBus' in service,"),
  ("'11.3.0-native-ux-fx-beta' in (r/'app/build.gradle').read_text()",
   "'11.3.2-egmix-native-samplers-beta' in (r/'app/build.gradle').read_text()")
]))
# The CI build now contains actual Android code and generated WAVs.
assert (r/"app/src/main/java/br/com/edgard/aura/core/PcmSamplerBus.java").is_file()
print("EG MIX 11.3.2 source update applied")
