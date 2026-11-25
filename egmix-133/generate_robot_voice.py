#!/usr/bin/env python3
"""Generate original offline robotic EG MIX station ID. No proprietary samples."""
from pathlib import Path
import subprocess, wave, array, math, tempfile, sys
SR=32000

def speak(text,rate,pitch):
    with tempfile.NamedTemporaryFile(suffix='.wav',delete=False) as f: tmp=Path(f.name)
    try:
        subprocess.run(['espeak','-v','pt-br','-s',str(rate),'-p',str(pitch),
                        '-a','180','-w',str(tmp),text],check=True)
        with wave.open(str(tmp),'rb') as w:
            frames=w.readframes(w.getnframes())
            fs=w.getframerate()
            channels=w.getnchannels()
        data=array.array('h');data.frombytes(frames)
        if channels!=1:raise ValueError('Expected mono WAV')
        source=[v/32768.0 for v in data]
        n=round(len(source)*SR/fs)
        result=[]
        for i in range(n):
            p=i*fs/SR;j=min(len(source)-1,int(p));k=min(len(source)-1,j+1);f=p-j
            result.append(source[j]*(1-f)+source[k]*f)
        return result
    finally:tmp.unlink(missing_ok=True)

def make(path):
    # Explicitly Edgard, starting with an unaccented E.
    data=speak('Dííí Jêêêêi',115,28)+[0.0]*int(.19*SR)+speak('Edgard Guarda',128,28)+[0.0]*int(.35*SR)
    n=len(data);prev=0.0;env=0.0;robot=[0.0]*n
    for i,x in enumerate(data):
        t=i/SR
        prev=prev*.95+x*.05
        high=x-.35*prev
        env=.997*env+.003*high*high
        carrier=.40*math.sin(2*math.pi*96*t)+.18*math.sin(2*math.pi*192*t)+.07*math.sin(2*math.pi*384*t)
        robot[i]=.78*high+1.35*math.sqrt(max(0,env))*carrier
    out=[]
    delays=[(round(.025*SR),.15),(round(.063*SR),-.13),(round(.18*SR),.25),(round(.34*SR),.12)]
    for i in range(n):
        y=robot[i]+sum(robot[i-d]*g for d,g in delays if i>=d)
        y=math.tanh(y*2.4)
        fadein=min(1,i/(.015*SR));fadeout=min(1,(n-1-i)/(.22*SR))
        out.append(y*max(0,min(fadein,fadeout)))
    peak=max(max(abs(x) for x in out),1e-9)
    samples=array.array('h',(max(-32767,min(32767,round(x*.88/peak*32767))) for x in out))
    path.parent.mkdir(parents=True,exist_ok=True)
    with wave.open(str(path),'wb') as w:
        w.setnchannels(1);w.setsampwidth(2);w.setframerate(SR)
        w.writeframes(samples.tobytes())
    print('GENERATED EG MIX ROBOT WAV',len(samples))

if __name__=='__main__':
    make(Path(sys.argv[1]))
