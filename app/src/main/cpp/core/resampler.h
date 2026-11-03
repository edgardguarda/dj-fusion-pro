// Conversor de taxa de amostragem em fluxo (estéreo intercalado), sinc com janela Hann.
// Roda só fora do tempo real (ao importar a faixa), nunca no callback de áudio.
//
// O tempo de cada amostra de saída é calculado com inteiros exatos (n * taxaEntrada / taxaSaída),
// então o resultado é idêntico qualquer que seja o tamanho dos pedaços de entrada e não acumula erro
// em faixas longas. A tabela tem 4096 fases: erro de tempo de até 1/4096 de amostra.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace djf {

class StreamResampler {
 public:
  static constexpr int kHalf = 16;      // cruzamentos por zero de cada lado
  static constexpr int kPhases = 4096;

  StreamResampler(int inRate, int outRate)
      : bypass_(inRate == outRate), inRate_(inRate), outRate_(outRate) {
    if (bypass_) return;
    // Ao reduzir a taxa, o corte cai para abaixo da nova frequência de Nyquist.
    const double scale = std::min(1.0, static_cast<double>(outRate) / inRate) * 0.98;
    table_.assign(static_cast<size_t>(kPhases) * 2 * kHalf, 0.f);
    const double pi = 3.14159265358979323846;
    for (int p = 0; p < kPhases; ++p) {
      const double f = static_cast<double>(p) / kPhases;
      double sum = 0;
      float* w = &table_[static_cast<size_t>(p) * 2 * kHalf];
      for (int k = -(kHalf - 1); k <= kHalf; ++k) {
        const double x = k - f;
        const double y = scale * x;
        const double sinc = (y == 0.0) ? 1.0 : std::sin(pi * y) / (pi * y);
        const double hann = 0.5 * (1.0 + std::cos(pi * x / kHalf));
        const double v = scale * sinc * hann;
        w[k + kHalf - 1] = static_cast<float>(v);
        sum += v;
      }
      for (int j = 0; j < 2 * kHalf; ++j) w[j] = static_cast<float>(w[j] / sum);  // ganho DC = 1
    }
    buf_.assign(static_cast<size_t>(kHalf) * 2, 0.f);  // silêncio antes do início
    bufStartAbs_ = -kHalf;
  }

  // Entrada: estéreo intercalado. A saída é acrescentada em `out`.
  void process(const float* in, size_t frames, std::vector<float>& out) {
    if (bypass_) {
      out.insert(out.end(), in, in + frames * 2);
      return;
    }
    totalIn_ += static_cast<int64_t>(frames);
    buf_.insert(buf_.end(), in, in + frames * 2);
    produce(out);
  }

  // Esvazia o que ficou retido. Chame uma vez, no fim.
  void flush(std::vector<float>& out) {
    if (bypass_) return;
    buf_.insert(buf_.end(), static_cast<size_t>(kHalf) * 2, 0.f);
    produce(out);
  }

 private:
  void produce(std::vector<float>& out) {
    const int64_t nbuf = static_cast<int64_t>(buf_.size() / 2);
    while (true) {
      const int64_t num = n_ * inRate_;
      const int64_t abs = num / outRate_;       // amostra de entrada logo antes do instante
      const int64_t rem = num % outRate_;       // fração, em unidades de 1/outRate
      if (abs >= totalIn_) break;               // além da última amostra real
      const int64_t first = abs - (kHalf - 1) - bufStartAbs_;
      if (first + 2 * kHalf > nbuf) break;      // ainda faltam amostras futuras
      const int p = static_cast<int>((rem * kPhases) / outRate_);
      const float* w = &table_[static_cast<size_t>(p) * 2 * kHalf];
      const float* s = &buf_[static_cast<size_t>(first) * 2];
      float l = 0.f, r = 0.f;
      for (int j = 0; j < 2 * kHalf; ++j) {
        l += w[j] * s[2 * j];
        r += w[j] * s[2 * j + 1];
      }
      out.push_back(l);
      out.push_back(r);
      ++n_;
    }
    const int64_t absNext = (n_ * inRate_) / outRate_;
    const int64_t newStart = absNext - (kHalf - 1);
    if (newStart > bufStartAbs_) {
      buf_.erase(buf_.begin(), buf_.begin() + (newStart - bufStartAbs_) * 2);
      bufStartAbs_ = newStart;
    }
  }

  bool bypass_;
  int64_t inRate_, outRate_;
  int64_t n_ = 0;               // índice da próxima amostra de saída
  int64_t totalIn_ = 0;         // quadros reais recebidos
  int64_t bufStartAbs_ = 0;     // índice absoluto do primeiro quadro de buf_
  std::vector<float> table_;
  std::vector<float> buf_;
};

}  // namespace djf
