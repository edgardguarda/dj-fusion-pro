#!/usr/bin/env python3
"""Repair source-format differences in CI before Android compilation."""
from pathlib import Path
import sys
root=Path(sys.argv[1])
core=root/'app/src/main/java/br/com/edgard/aura/core/PcmSamplerBus.java'
s=core.read_text()
if 'historyHead' not in s:
    old='  private float[] current; private int frame; private double level;'
    new='''  private float[] current; private int frame; private double level;
  private static final int HISTORY=OUTPUT_RATE;
  private final float[] history=new float[HISTORY];
  private int historyHead,historyValid;
  private boolean scratching;
  private int scratchFrame;
  private double scratchLevel;
  private static final int SCRATCH_FRAMES=(int)(OUTPUT_RATE*.66);'''
    assert old in s,'PCM bus field anchor'
    s=s.replace(old,new)
    old='    if(cmd!=null){current=clips[cmd.key];frame=0;level=cmd.level;}'
    new='''    if(cmd!=null){
      scratching=cmd.key==3 && historyValid>=OUTPUT_RATE/3;
      scratchFrame=0;scratchLevel=cmd.level;
      if(!scratching){current=clips[cmd.key];frame=0;level=cmd.level;}
      else current=null;
    }
    if(scratching){
      if(scratchFrame>=SCRATCH_FRAMES){scratching=false;return 0;}
      double seconds=scratchFrame/(double)OUTPUT_RATE;
      int stroke=Math.min(4,(int)(seconds/.132));
      double u=(seconds-stroke*.132)/.132;
      double position=stroke%2==0?.05+.17*u:.22-.17*u;
      double from=historyHead-1-OUTPUT_RATE*(.62-position);
      int pos=(int)Math.floor(from);
      double fraction=from-pos;
      int a=Math.floorMod(pos,HISTORY),b=Math.floorMod(pos+1,HISTORY);
      double sample=history[a]*(1-fraction)+history[b]*fraction;
      double edge=Math.min(1,Math.min(u,1-u)*19);
      double envelope=Math.min(1,Math.min(scratchFrame/(OUTPUT_RATE*.006),
                             (SCRATCH_FRAMES-scratchFrame)/(OUTPUT_RATE*.025)));
      scratchFrame++;
      return sample*edge*Math.max(0,envelope)*scratchLevel;
    }'''
    assert old in s,'PCM bus command anchor'
    s=s.replace(old,new)
    old='  public void reset(){request.set(null);current=null;frame=0;}'
    new='''  public void remember(double mono){
    history[historyHead]=(float)(Double.isFinite(mono)?Math.max(-1,Math.min(1,mono)):0);
    historyHead=(historyHead+1)%HISTORY;
    if(historyValid<HISTORY)historyValid++;
  }
  public void reset(){request.set(null);current=null;frame=0;
    scratching=false;historyValid=0;historyHead=0;}'''
    assert old in s,'PCM bus reset anchor'
    s=s.replace(old,new)
core.write_text(s)
audio=root/'app/src/main/java/br/com/edgard/aura/AudioService.java'
s=audio.read_text()
if 'samplerBus.remember(' not in s:
    old='                    double sample=samplerBus.next();'
    assert old in s,'Audio bus injection anchor'
    s=s.replace(old,'                    samplerBus.remember((fxOut[0]+fxOut[1])*.5);\n'+old)
audio.write_text(s)
ui=root/'app/src/main/assets/native-ui.js'
s=ui.read_text()
if 'const seek=$(\'seek\')' not in s:
    marker=" const samplerVolume=$('samplerVolume')"
    assert marker in s,'UI seek anchor'
    seek=""" // Android seek: quick mode only, never claim arbitrary studio PCM seeking.
 const seek=$('seek'),seekTime=$('seekTime');
 let draggingSeek=false,seekCanPlay=false;
 if(seek){
   seek.disabled=true;seek.min='0';seek.step='0.1';
   seek.style.touchAction='pan-x';
   seek.setAttribute('aria-label','Posição da música atual');
   seek.addEventListener('pointerdown',()=>{if(!seek.disabled)draggingSeek=true;},true);
   seek.addEventListener('input',e=>{
     if(!native||seek.disabled)return;
     draggingSeek=true;
     if(seekTime)seekTime.textContent=fmt(seek.value);
     e.stopImmediatePropagation();
   },true);
   const seekEnd=e=>{
     if(!native||!draggingSeek)return;
     e.stopImmediatePropagation();
     draggingSeek=false;
     if(seekCanPlay&&typeof api.seek==='function')api.seek(Number(seek.value));
   };
   seek.addEventListener('change',seekEnd,true);
   seek.addEventListener('pointerup',seekEnd,true);
   seek.addEventListener('pointercancel',()=>{draggingSeek=false;},true);
 }
"""
    s=s.replace(marker,seek+marker)
if 'seek.disabled=!seekCanPlay' not in s:
    marker='     renderNativeFx(playing&&!v.quick'
    assert marker in s,'UI snapshot anchor'
    seek="""     const activeSeconds=Number(v.time)||0;
     const quickDuration=Math.max(0,(Number(v.durationMs)||0)/1000);
     const studioDuration=Math.max(0,Number(v.tracks?.[idx]?.seconds)||0);
     const seekDuration=v.quick?quickDuration:studioDuration;
     seekCanPlay=!!(v.quick&&playing&&v.mode!=='mixing'&&quickDuration>1.5);
     if(seek){
       seek.disabled=!seekCanPlay;seek.max=String(seekDuration);
       if(!draggingSeek)seek.value=String(Math.min(seekDuration,Math.max(0,activeSeconds)));
       seek.title=seekCanPlay?'Arraste para avançar ou voltar':
         playing&&!v.quick?'Estúdio PCM: busca bloqueada para preservar o sincronismo':
         'Reproduza uma música para ativar a posição';
     }
     if(seekTime&&!draggingSeek)seekTime.textContent=fmt(activeSeconds);
"""
    s=s.replace(marker,seek+marker)
ui.write_text(s)
print('EG MIX 11.3.3 rebased: native seek, live scratch, PCM injection')
