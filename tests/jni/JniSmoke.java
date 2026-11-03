import app.djfusion.audio.NativeEngine;
import java.nio.*;
public class JniSmoke {
  static int fails = 0;
  static void check(boolean ok, String what) { System.out.println((ok ? "  ok     " : "  FALHOU ") + what); if (!ok) fails++; }
  static long buildTrack(int rate, int seconds, double hz, boolean asFloat) {
    long h = NativeEngine.nativeBeginTrack(rate, 48000L * 900);
    int total = rate * seconds, chunk = 4410;
    ByteBuffer bb = ByteBuffer.allocateDirect(chunk * 2 * (asFloat ? 4 : 2)).order(ByteOrder.nativeOrder());
    for (int pos = 0; pos < total; pos += chunk) {
      int n = Math.min(chunk, total - pos); bb.clear();
      for (int i = 0; i < n; i++) {
        double v = 0.6 * Math.sin(2 * Math.PI * hz * (pos + i) / rate);
        for (int c = 0; c < 2; c++) { if (asFloat) bb.putFloat((float) v); else bb.putShort((short) (v * 32767)); }
      }
      NativeEngine.nativeAppend(h, bb, 0, n * 2 * (asFloat ? 4 : 2), 2, asFloat);
    }
    return NativeEngine.nativeFinishTrack(h);
  }
  public static void main(String[] a) throws Exception {
    check(NativeEngine.nativeStart(), "nativeStart abre o fluxo");
    int[] info = new int[4]; NativeEngine.nativeStreamInfo(info);
    check(info[0] == 48000, "taxa do fluxo = " + info[0]);

    long t = buildTrack(44100, 2, 440, false);
    check(t > 0, "faixa PCM16 44,1 kHz construída");
    long frames = NativeEngine.nativeTrackFrames(t);
    check(Math.abs(frames - 96000) <= 2, "quadros após reamostrar = " + frames + " (esperado 96000)");
    byte[] peaks = NativeEngine.nativeTrackPeaks(t);
    check(peaks.length >= 3 * 299, "picos da waveform: " + peaks.length + " bytes");
    check(NativeEngine.nativeLoad(0, t), "carregar no deck A");
    long t2 = buildTrack(48000, 1, 330, true);
    check(t2 > 0 && NativeEngine.nativeLoad(1, t2), "faixa float 48 kHz no deck B");

    NativeEngine.nativeSetCrossfader(0f); NativeEngine.nativeSetMaster(1f); NativeEngine.nativeSetVolume(0, 1f);
    Thread.sleep(300);
    long[] st = new long[4]; NativeEngine.nativeDeckState(0, st);
    check((st[0] & 1) == 1 && (st[0] & 2) == 0 && st[3] == frames, "estado: carregada, parada, tamanho certo");
    NativeEngine.nativePlay(0); Thread.sleep(500);
    NativeEngine.nativeDeckState(0, st);
    check((st[0] & 2) == 2, "tocando");
    check(st[1] > 10000 && st[1] < 30000, "posição avança (~0,5 s): " + st[1]);
    float peak = NativeEngine.nativeMasterPeak();
    check(peak > 0.4f && peak <= 1f, "nível master com a música = " + peak);
    check(NativeEngine.nativeDeckPeak(0) > 0.4f, "nível do deck A");
    NativeEngine.nativeSetCrossfader(1f); Thread.sleep(1100);
    check(NativeEngine.nativeMasterPeak() < 0.05f, "crossfader todo para B (parado): master fica quase mudo (medidor cai em ~0,8 s)");
    NativeEngine.nativePause(0); NativeEngine.nativeSeek(0, 50000); Thread.sleep(150);
    NativeEngine.nativeDeckState(0, st);
    check(!((st[0] & 2) == 2) && st[1] == 50000, "pausa + busca para 50000 = " + st[1]);
    NativeEngine.nativeCue(0); Thread.sleep(100); NativeEngine.nativeDeckState(0, st);
    check(st[2] == 50000, "cue gravado nessa posição");

    long big = NativeEngine.nativeBeginTrack(44100, 1000);
    ByteBuffer bb = ByteBuffer.allocateDirect(44100 * 2 * 2).order(ByteOrder.nativeOrder());
    for (int i = 0; i < 44100 * 2; i++) bb.putShort((short) 1000);
    NativeEngine.nativeAppend(big, bb, 0, 44100 * 2 * 2, 2, false);
    check(NativeEngine.nativeBuilderTooLong(big), "limite de tamanho detectado");
    check(NativeEngine.nativeFinishTrack(big) == -1, "faixa longa demais devolve -1");
    check(NativeEngine.nativeBeginTrack(100, 1000) == 0, "taxa inválida recusada");
    long abort = NativeEngine.nativeBeginTrack(44100, 100000); NativeEngine.nativeAbortTrack(abort);

    NativeEngine.nativeFreeTrack(t); NativeEngine.nativeFreeTrack(t2);
    NativeEngine.nativeUnload(0); Thread.sleep(100); NativeEngine.nativeCollectGarbage();
    NativeEngine.nativeDeckState(0, st);
    check((st[0] & 1) == 0, "descarregar deck A");
    NativeEngine.nativeStop();
    System.out.println(fails == 0 ? "JNI OK" : ("JNI FALHOU: " + fails));
    System.exit(fails == 0 ? 0 : 1);
  }
}
