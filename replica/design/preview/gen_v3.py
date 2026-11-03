import math, random, colorsys
W,H=844,390
o=[];a=o.append
A,B='#ffa726','#5cc8ff'
a(f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {H}" width="{W}" height="{H}" font-family="Inter, Roboto, Arial, sans-serif">')
a('''<defs>
<linearGradient id="plate" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#d5dadf"/><stop offset=".3" stop-color="#a9b1b9"/><stop offset=".55" stop-color="#c4cad0"/><stop offset="1" stop-color="#8f979f"/></linearGradient>
<linearGradient id="knob" x1="0" y1="0" x2="1" y2="1"><stop offset="0" stop-color="#e6e9ec"/><stop offset=".45" stop-color="#8f979f"/><stop offset="1" stop-color="#4a525a"/></linearGradient>
<linearGradient id="ring" x1="0" y1="0" x2="1" y2="1"><stop offset="0" stop-color="#f1f3f5"/><stop offset=".3" stop-color="#7d868f"/><stop offset=".55" stop-color="#d5dadf"/><stop offset="1" stop-color="#4a525a"/></linearGradient>
<linearGradient id="panel" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#22272c"/><stop offset="1" stop-color="#171a1e"/></linearGradient>
<linearGradient id="sheen" x1="0" y1="0" x2="1" y2="1"><stop offset="0" stop-color="#fff" stop-opacity=".0"/><stop offset=".5" stop-color="#fff" stop-opacity=".16"/><stop offset="1" stop-color="#fff" stop-opacity=".0"/></linearGradient>
<linearGradient id="padhi" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#fff" stop-opacity=".38"/><stop offset=".5" stop-color="#fff" stop-opacity=".02"/><stop offset="1" stop-color="#000" stop-opacity=".22"/></linearGradient>
<pattern id="brush" width="4" height="3" patternUnits="userSpaceOnUse"><rect width="4" height="1" fill="#fff" opacity=".10"/><rect width="4" height="1" y="1.5" fill="#000" opacity=".07"/></pattern>
<radialGradient id="labA"><stop offset="0" stop-color="#ffd27a"/><stop offset="1" stop-color="#b8610a"/></radialGradient>
<radialGradient id="labB"><stop offset="0" stop-color="#b9ecff"/><stop offset="1" stop-color="#1f6f9e"/></radialGradient>
<filter id="glow"><feGaussianBlur stdDeviation="2.5" result="b"/><feMerge><feMergeNode in="b"/><feMergeNode in="SourceGraphic"/></feMerge></filter>
</defs>''')
a(f'<rect width="{W}" height="{H}" fill="#0e1013"/>')
a('<rect width="844" height="24" fill="#16191d"/><line x1="0" y1="24" x2="844" y2="24" stroke="#3a4149"/>')
for i,(t,act) in enumerate([('DJ LIVE',1),('TIMELINE',0),('BIBLIOTECA',0)]):
    x=14+i*92
    a(f'<text x="{x}" y="16" fill="{"#ffa726" if act else "#9aa4ae"}" font-size="11" font-weight="700" letter-spacing="1">{t}</text>')
    if act: a(f'<rect x="{x}" y="21" width="{len(t)*7.5:.0f}" height="2" fill="#ffa726"/>')
a('<text x="830" y="16" fill="#9aa4ae" font-size="11" font-weight="600" text-anchor="end">USB: sem controladora   ·   48 kHz  ·  10 ms</text>')

def wave(y,seed,t):
    random.seed(seed)
    a(f'<rect x="0" y="{y}" width="844" height="40" fill="#0b0d10"/>')
    for k in range(0,844,31):
        a(f'<line x1="{k}" y1="{y+1}" x2="{k}" y2="{y+39}" stroke="#fff" stroke-opacity="{.20 if (k//31)%4==0 else .07}"/>')
    mid=y+20
    for i in range(0,844,3):
        env=abs(math.sin(i*0.045+seed))*0.7+0.3
        lo=env*18*(0.7+0.3*math.sin(i*0.31+seed)); mi=lo*0.62*(0.7+random.random()*0.5); hi=lo*0.32*(0.6+random.random()*0.8)
        for h,c in ((lo,'#ff6a3d'),(mi,'#ffd166'),(hi,'#8fe3ff')):
            a(f'<rect x="{i}" y="{mid-h:.1f}" width="2" height="{2*h:.1f}" fill="{c}" opacity=".92"/>')
    a(f'<polygon points="200,{y+1} 208,{y+1} 200,{y+10}" fill="#ff5a5f"/><line x1="200" y1="{y+1}" x2="200" y2="{y+39}" stroke="#ff5a5f" stroke-opacity=".8"/>')
    a(f'<rect x="560" y="{y+1}" width="62" height="38" fill="#fff" opacity=".10"/>')
    a(f'<rect x="421" y="{y}" width="2" height="40" fill="#fff"/>')
    a(f'<rect x="770" y="{y+3}" width="70" height="15" rx="4" fill="#0e1013" opacity=".85"/><text x="836" y="{y+14}" fill="#e6e9ec" font-size="11" font-weight="700" text-anchor="end" font-family="JetBrains Mono, monospace">{t}</text>')
wave(26,3,'-02:41.3'); wave(68,11,'-03:12.8')

def plate(x,y,w,h,label=None,glyph=None,led=None,fs=13):
    a(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="8" fill="url(#plate)"/><rect x="{x}" y="{y}" width="{w}" height="{h}" rx="8" fill="url(#brush)"/>')
    if label: a(f'<text x="{x+w/2}" y="{y+h/2+4.5}" fill="#14171b" font-size="{fs}" font-weight="800" text-anchor="middle">{label}</text>')
    if glyph=='play': a(f'<polygon points="{x+w/2-8},{y+h/2-11} {x+w/2-8},{y+h/2+11} {x+w/2+12},{y+h/2}" fill="#14171b"/>')
    if led: a(f'<circle cx="{x+w-9}" cy="{y+9}" r="3.5" fill="{led}" filter="url(#glow)"/>')

def knob(cx,cy,r,col,ang):
    a(f'<circle cx="{cx}" cy="{cy}" r="{r+2}" fill="#0e1013"/><circle cx="{cx}" cy="{cy}" r="{r}" fill="url(#knob)"/><circle cx="{cx}" cy="{cy}" r="{r}" fill="url(#brush)"/>')
    a(f'<line x1="{cx}" y1="{cy}" x2="{cx+(r-3)*math.sin(ang):.1f}" y2="{cy-(r-3)*math.cos(ang):.1f}" stroke="#14171b" stroke-width="2.5" stroke-linecap="round"/>')
    for t in range(-135,136,15):
        rr=r+5; ra=math.radians(t)
        a(f'<circle cx="{cx+rr*math.sin(ra):.1f}" cy="{cy-rr*math.cos(ra):.1f}" r="1" fill="{col if t<=math.degrees(ang) else "#3a4149"}"/>')

def deck(x0,side,col,letter,lab,title,artist,bpm,pitch,key,pads,prog):
    w=330
    L=lambda lx,ww=0: x0+lx if side==0 else x0+w-lx-ww
    C=lambda lx: x0+lx if side==0 else x0+w-lx
    a(f'<rect x="{x0}" y="110" width="{w}" height="280" fill="url(#panel)"/><rect x="{x0}" y="110" width="{w}" height="2" fill="{col}" opacity=".85"/>')
    a(f'<text x="{x0+12}" y="136" fill="{col}" font-size="18" font-weight="800">{letter}</text>')
    a(f'<text x="{x0+36}" y="133" fill="#e6e9ec" font-size="13" font-weight="700">{title}</text><text x="{x0+36}" y="147" fill="#9aa4ae" font-size="11">{artist}</text>')
    a(f'<text x="{x0+w-12}" y="140" fill="{col}" font-size="24" font-weight="700" text-anchor="end" font-family="JetBrains Mono, monospace">{bpm}</text>')
    a(f'<text x="{x0+w-12}" y="151" fill="#9aa4ae" font-size="10" font-weight="700" text-anchor="end">BPM  {pitch}  TOM {key}</text>')
    # fader de pitch
    fx=C(19)
    a(f'<rect x="{fx-5}" y="160" width="10" height="164" rx="5" fill="#262b31" stroke="#3a4149"/>')
    for k in range(9): a(f'<line x1="{fx-14}" y1="{164+k*19.5:.0f}" x2="{fx-8}" y2="{164+k*19.5:.0f}" stroke="#6b747d"/>')
    a(f'<rect x="{fx-18}" y="220" width="36" height="40" rx="7" fill="url(#plate)"/><rect x="{fx-18}" y="220" width="36" height="40" rx="7" fill="url(#brush)"/><rect x="{fx-12}" y="239" width="24" height="3" fill="{col}"/>')
    a(f'<text x="{fx}" y="339" fill="#9aa4ae" font-size="8" font-weight="700" text-anchor="middle">PITCH</text>')
    # prato de vinil
    cx,cy=C(118),240
    a(f'<circle cx="{cx}" cy="{cy}" r="76" fill="url(#ring)"/><circle cx="{cx}" cy="{cy}" r="71" fill="#0b0c0e"/>')
    for r in range(33,70,3): a(f'<circle cx="{cx}" cy="{cy}" r="{r}" fill="none" stroke="#fff" stroke-opacity="{.035+0.03*((r//3)%3)}"/>')
    a(f'<path d="M{cx},{cy} L{cx-62},{cy-38} A71,71 0 0 1 {cx-18},{cy-69} Z" fill="url(#sheen)"/><path d="M{cx},{cy} L{cx+62},{cy+38} A71,71 0 0 1 {cx+18},{cy+69} Z" fill="url(#sheen)"/>')
    a(f'<circle cx="{cx}" cy="{cy}" r="30" fill="url(#{lab})"/><circle cx="{cx}" cy="{cy}" r="30" fill="none" stroke="{col}" stroke-width="2"/>')
    a(f'<text x="{cx}" y="{cy+8}" fill="#14171b" font-size="22" font-weight="800" text-anchor="middle" opacity=".65">♪</text><circle cx="{cx}" cy="{cy}" r="3" fill="#0b0c0e"/>')
    ma=math.radians(-60); a(f'<circle cx="{cx+64*math.cos(ma):.1f}" cy="{cy+64*math.sin(ma):.1f}" r="3.2" fill="{col}" filter="url(#glow)"/>')
    # braço (posição da faixa)
    px,py=C(194),168
    hx=cx+(-1 if side==0 else 1)*0 + (70-prog*38)*(1 if side==0 else -1)*0.62*(-1 if side==0 else -1)*-1
    hx=C(118+int((70-prog*38)*0.55)); hy=cy+int((70-prog*38)*0.55)-16
    a(f'<line x1="{px}" y1="{py}" x2="{hx}" y2="{hy}" stroke="#c4cad0" stroke-width="4" stroke-linecap="round"/><line x1="{px}" y1="{py}" x2="{hx}" y2="{hy}" stroke="#14171b" stroke-width="1" opacity=".5"/>')
    a(f'<rect x="{hx-6}" y="{hy-3}" width="12" height="9" rx="2" fill="#e6e9ec"/><circle cx="{px}" cy="{py}" r="9" fill="url(#knob)"/><circle cx="{px}" cy="{py}" r="3" fill="#14171b"/>')
    # transporte e loops
    plate(L(208,56),158,56,52,glyph='play',led=col); plate(L(208,56),216,56,52,label='CUE'); plate(L(208,56),274,56,48,label='SYNC',fs=12)
    for i,t in enumerate(['LOOP','½','2×']):
        gx=L(270,56); gy=158+i*54
        a(f'<rect x="{gx}" y="{gy}" width="56" height="48" rx="8" fill="#1c2025" stroke="#6b747d"/><text x="{gx+28}" y="{gy+29}" fill="#e6e9ec" font-size="{11 if i==0 else 15}" font-weight="800" text-anchor="middle">{t}</text>')
    # pads coloridos
    for i,c in enumerate(pads):
        gx=(x0+40+i*72) if side==0 else (x0+8+i*72)
        a(f'<rect x="{gx}" y="332" width="66" height="54" rx="9" fill="{c}"/><rect x="{gx}" y="332" width="66" height="54" rx="9" fill="url(#padhi)"/><rect x="{gx}" y="332" width="66" height="54" rx="9" fill="none" stroke="#000" stroke-opacity=".35"/>')
        a(f'<text x="{gx+33}" y="365" fill="#14171b" font-size="17" font-weight="800" text-anchor="middle">{i+1}</text>')
deck(0,0,A,'A','labA','Faixa de exemplo A','Artista de exemplo','124.00','+0.0%','8A',['#ff5a5f','#ffd166','#4cd08a','#5cc8ff'],0.35)
deck(514,1,B,'B','labB','Faixa de exemplo B','Artista de exemplo','126.00','−1.6%','9A',['#b57bff','#ff8a3d','#4cd08a','#ff5a5f'],0.6)

mx=330
a(f'<rect x="{mx}" y="110" width="184" height="280" fill="#13161a"/><line x1="{mx}" y1="110" x2="{mx}" y2="390" stroke="#3a4149"/><line x1="{mx+184}" y1="110" x2="{mx+184}" y2="390" stroke="#3a4149"/>')
# analisador de espectro arco-íris
a(f'<rect x="{mx+10}" y="116" width="164" height="40" rx="6" fill="#0b0d10"/>')
random.seed(5)
for i in range(30):
    h=3+abs(math.sin(i*0.5))*13+random.random()*7
    r,g,b=[int(255*v) for v in colorsys.hsv_to_rgb(i/30*0.78,0.9,1)]
    a(f'<rect x="{mx+14+i*5.4:.1f}" y="{136-h:.1f}" width="4" height="{2*h:.1f}" rx="1" fill="#{r:02x}{g:02x}{b:02x}"/>')
for ci,(col,cx) in enumerate([(A,mx+46),(B,mx+138)]):
    for ri,ang in enumerate([0.4,-0.1,0.0]):
        knob(cx,178+ri*34,13,col,ang)
for ri,lab in enumerate(['AGUDOS','MÉDIOS','GRAVES']):
    a(f'<text x="{mx+92}" y="{181+ri*34}" fill="#9aa4ae" font-size="7.5" font-weight="700" text-anchor="middle">{lab}</text>')
for ci,(col,fx) in enumerate([(A,mx+30),(B,mx+154)]):
    a(f'<rect x="{fx-5}" y="260" width="10" height="76" rx="5" fill="#262b31" stroke="#3a4149"/>')
    a(f'<rect x="{fx-16}" y="282" width="32" height="30" rx="7" fill="url(#plate)"/><rect x="{fx-16}" y="282" width="32" height="30" rx="7" fill="url(#brush)"/><rect x="{fx-10}" y="295" width="20" height="3" fill="{col}"/>')
cols=['#4cd08a']*5+['#ffd166']*2+['#ff8a3d']+['#ff5a5f']*2
for ci,vx in enumerate([mx+72,mx+104]):
    for s in range(10):
        a(f'<rect x="{vx}" y="{330-s*7.4:.1f}" width="10" height="5.4" rx="1.5" fill="{cols[s]}" opacity="{1 if s<(7 if ci==0 else 6) else .2}"/>')
a(f'<rect x="{mx+14}" y="364" width="156" height="8" rx="4" fill="#262b31" stroke="#3a4149"/>')
a(f'<rect x="{mx+62}" y="346" width="60" height="40" rx="8" fill="url(#plate)"/><rect x="{mx+62}" y="346" width="60" height="40" rx="8" fill="url(#brush)"/><rect x="{mx+90}" y="352" width="4" height="28" fill="#14171b"/>')
a(f'<text x="{mx+10}" y="358" fill="{A}" font-size="10" font-weight="800">A</text><text x="{mx+174}" y="358" fill="{B}" font-size="10" font-weight="800" text-anchor="end">B</text>')
a('</svg>')
open('dj-live-v3.svg','w').write('\n'.join(o))
