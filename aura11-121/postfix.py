from pathlib import Path
import sys
path=Path(sys.argv[1])/'app/src/main/java/br/com/edgard/aura/AudioService.java'
s=path.read_text()
old='                    if(n<0)throw new IllegalStateException("Falha AudioTrack: "+n);\n                    if(n==0)throw new IllegalStateException("Áudio parou de avançar");'
new='                    // STOP pauses the device to unblock WRITE_BLOCKING; a short\n                    // transfer during intentional shutdown is not a crash.\n                    if(!running.get())break;\n                    if(n<0)throw new IllegalStateException("Falha AudioTrack: "+n);\n                    if(n==0)throw new IllegalStateException("Áudio parou de avançar");'
assert s.count(old)==1, 'Cannot locate AudioTrack shutdown guard'
path.write_text(s.replace(old,new,1))
