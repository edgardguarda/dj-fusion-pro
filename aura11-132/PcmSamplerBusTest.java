import br.com.edgard.aura.core.PcmSamplerBus;
import java.util.Arrays;
public final class PcmSamplerBusTest {
  static int checks=0;
  static void ok(boolean p,String m){checks++;if(!p)throw new AssertionError(m);}
  public static void main(String[] a) {
    PcmSamplerBus bus=new PcmSamplerBus();
    ok(!bus.ready(),"initial");ok(!bus.trigger("vinheta",.5),"no sample yet");
    float[][] clips=new float[4][];
    for(int i=0;i<4;i++){clips[i]=new float[4800+i*100];Arrays.fill(clips[i],.75f);}
    bus.load(clips);ok(bus.ready(),"loaded");
    ok(!bus.trigger("unknown",.5),"unknown name");
    ok(!bus.trigger("vinheta",Double.NaN),"NaN reject");
    ok(bus.trigger("vinheta",1.0),"accepted");
    double peak=0,prev=0;
    for(int i=0;i<6000;i++){
      double x=bus.next();
      ok(Double.isFinite(x),"finite audio");
      ok(Math.abs(x)<=.38+.00001,"headroom");
      if(i>0)ok(Math.abs(x-prev)<.008,"anti-click");
      peak=Math.max(peak,Math.abs(x));prev=x;
    }
    ok(peak>.2,"non-silent sample");
    ok(bus.next()==0,"stopped");
    bus.trigger("scratch",.75);
    for(int i=0;i<5500;i++)ok(Double.isFinite(bus.next()),"scratch");
    bus.reset();ok(bus.next()==0,"silence");
    System.out.println("PASS EG MIX PcmSamplerBusTest "+checks+" assertions");
  }
}