// opl3_sim: render an OPL3 (YMF262) VGM stream to a stereo WAV using
// Greg Taylor's OPL3 FPGA RTL core (gtaylormb/opl3_fpga, LGPLv3) under
// Verilator. The harness only decodes VGM, drives the core's host bus
// at the genuine 14.31818 MHz master clock, captures the core's
// 49715.97 Hz stereo sample stream, and resamples it to 44.1 kHz.
//
// Usage: opl3_sim INPUT.vgm --output FILE.wav [--max-seconds N]
#include "Vopl3.h"
#include "verilated.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace {

constexpr uint64_t kMasterClock = 14318180;  // YMF262 master clock, Hz
constexpr uint32_t kVgmRate = 44100;         // VGM wait unit, Hz

Vopl3* g_top = nullptr;
uint64_t g_cycles = 0;
std::vector<int16_t> g_cap_l;
std::vector<int16_t> g_cap_r;

int16_t Clamp16(int32_t v) {
  if (v > 32767) return 32767;
  if (v < -32768) return -32768;
  return static_cast<int16_t>(v);
}

int32_t Sext24(uint32_t v) {
  return (v & 0x800000) ? static_cast<int32_t>(v | 0xFF000000u)
                        : static_cast<int32_t>(v);
}

void Step() {
  g_top->clk = 0;
  g_top->clk_host = 0;
  g_top->eval();
  g_top->clk = 1;
  g_top->clk_host = 1;
  g_top->eval();
  ++g_cycles;
  if (g_top->sample_valid) {
    // The core presents its 16-bit sample left-shifted by
    // DAC_LEFT_SHIFT (5) in the 24-bit DAC word.
    g_cap_l.push_back(Clamp16(Sext24(g_top->sample_l) >> 5));
    g_cap_r.push_back(Clamp16(Sext24(g_top->sample_r) >> 5));
  }
}

void RunUntil(uint64_t target_cycle) {
  while (g_cycles < target_cycle) Step();
}

void WriteReg(int bank, uint8_t reg, uint8_t data, uint64_t at_cycle) {
  RunUntil(at_cycle);
  g_top->address = bank ? 2 : 0;
  g_top->din = reg;
  g_top->cs_n = 0;
  g_top->wr_n = 0;
  Step();
  Step();
  Step();
  g_top->cs_n = 1;
  g_top->wr_n = 1;
  Step();
  Step();
  g_top->address = bank ? 3 : 1;
  g_top->din = data;
  g_top->cs_n = 0;
  g_top->wr_n = 0;
  Step();
  Step();
  Step();
  g_top->cs_n = 1;
  g_top->wr_n = 1;
  g_top->din = 0;
  Step();
  Step();
}

// Total byte length of the VGM command starting with opcode c at p
// (p points at the opcode; avail counts the remaining bytes).
size_t CmdLen(uint8_t c, const uint8_t* p, size_t avail) {
  if (c == 0x61) return 3;
  if (c == 0x62 || c == 0x63) return 1;
  if (c == 0x64) return 4;
  if (c == 0x66) return 1;
  if (c == 0x67) {
    if (avail < 7) return avail;
    uint32_t size = 0;
    std::memcpy(&size, p + 3, 4);
    return 7 + size;
  }
  if (c == 0x68) return 12;
  if (c >= 0x70 && c <= 0x8F) return 1;
  if (c == 0x90 || c == 0x91 || c == 0x95) return 5;
  if (c == 0x92) return 6;
  if (c == 0x93) return 11;
  if (c == 0x94) return 2;
  if (c == 0x4F || c == 0x50) return 2;
  if (c >= 0x51 && c <= 0x5F) return 3;
  if (c >= 0xA0 && c <= 0xBF) return 3;
  if (c >= 0xC0 && c <= 0xDF) return 4;
  if (c >= 0xE0) return 5;
  if (c >= 0x30 && c <= 0x3F) return 2;
  return 0;  // unknown
}

bool WriteWav(const char* path, const std::vector<int16_t>& l,
              const std::vector<int16_t>& r, uint32_t rate) {
  FILE* f = std::fopen(path, "wb");
  if (!f) return false;
  const uint32_t frames = static_cast<uint32_t>(l.size());
  const uint32_t data_bytes = frames * 4;
  auto w32 = [&](uint32_t v) { std::fwrite(&v, 4, 1, f); };
  auto w16 = [&](uint16_t v) { std::fwrite(&v, 2, 1, f); };
  std::fwrite("RIFF", 1, 4, f);
  w32(36 + data_bytes);
  std::fwrite("WAVEfmt ", 1, 8, f);
  w32(16);
  w16(1);
  w16(2);
  w32(rate);
  w32(rate * 4);
  w16(4);
  w16(16);
  std::fwrite("data", 1, 4, f);
  w32(data_bytes);
  for (uint32_t i = 0; i < frames; ++i) {
    std::fwrite(&l[i], 2, 1, f);
    std::fwrite(&r[i], 2, 1, f);
  }
  std::fclose(f);
  return true;
}

}  // namespace

int main(int argc, char** argv) {
  Verilated::commandArgs(argc, argv);
  std::string input, output;
  double max_seconds = 0.0;
  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    if (a == "--output" && i + 1 < argc) {
      output = argv[++i];
    } else if (a == "--max-seconds" && i + 1 < argc) {
      max_seconds = std::atof(argv[++i]);
    } else if (!a.empty() && a[0] != '-') {
      input = a;
    }
  }
  if (input.empty() || output.empty()) {
    std::fprintf(stderr, "usage: opl3_sim INPUT.vgm --output FILE.wav "
                         "[--max-seconds N]\n");
    return 1;
  }
  FILE* f = std::fopen(input.c_str(), "rb");
  if (!f) {
    std::fprintf(stderr, "cannot open %s\n", input.c_str());
    return 1;
  }
  std::fseek(f, 0, SEEK_END);
  long sz = std::ftell(f);
  std::fseek(f, 0, SEEK_SET);
  std::vector<uint8_t> vgm(static_cast<size_t>(sz));
  if (std::fread(vgm.data(), 1, vgm.size(), f) != vgm.size()) return 1;
  std::fclose(f);
  if (vgm.size() < 0x40 || std::memcmp(vgm.data(), "Vgm ", 4) != 0) {
    std::fprintf(stderr, "not a VGM file\n");
    return 1;
  }
  uint32_t total_samples = 0, data_off = 0;
  std::memcpy(&total_samples, vgm.data() + 0x18, 4);
  std::memcpy(&data_off, vgm.data() + 0x34, 4);
  size_t pos = data_off ? 0x34 + data_off : 0x40;

  g_top = new Vopl3;
  g_top->clk = 0;
  g_top->clk_host = 0;
  g_top->clk_dac = 0;
  g_top->ic_n = 0;
  g_top->cs_n = 1;
  g_top->rd_n = 1;
  g_top->wr_n = 1;
  g_top->address = 0;
  g_top->din = 0;
  for (int i = 0; i < 400; ++i) Step();
  g_top->ic_n = 1;
  for (int i = 0; i < 400; ++i) Step();
  g_cap_l.clear();
  g_cap_r.clear();
  g_cycles = 0;

  uint64_t sample_pos = 0;
  const uint64_t max_pos =
      max_seconds > 0 ? static_cast<uint64_t>(max_seconds * kVgmRate) : 0;
  bool ended = false;
  bool stopped_early = false;
  while (!ended && pos < vgm.size()) {
    const uint8_t c = vgm[pos];
    const size_t len = CmdLen(c, vgm.data() + pos, vgm.size() - pos);
    if (len == 0 || pos + len > vgm.size()) {
      std::fprintf(stderr, "unsupported VGM command 0x%02x at 0x%zx\n", c,
                   pos);
      return 1;
    }
    const uint8_t* p = vgm.data() + pos;
    switch (c) {
      case 0x5E:
        WriteReg(0, p[1], p[2], sample_pos * kMasterClock / kVgmRate);
        break;
      case 0x5F:
        WriteReg(1, p[1], p[2], sample_pos * kMasterClock / kVgmRate);
        break;
      case 0x61: {
        uint16_t w = 0;
        std::memcpy(&w, p + 1, 2);
        sample_pos += w;
        break;
      }
      case 0x62:
        sample_pos += 735;
        break;
      case 0x63:
        sample_pos += 882;
        break;
      case 0x66:
        ended = true;
        break;
      default:
        if (c >= 0x70 && c <= 0x7F) {
          sample_pos += (c & 0xF) + 1;
        } else if (c >= 0x80 && c <= 0x8F) {
          sample_pos += (c & 0xF);
        }
        break;
    }
    pos += len;
    if (max_pos && sample_pos > max_pos) {
      stopped_early = true;
      break;
    }
  }
  // Let the last notes ring for a short tail, then stop.
  RunUntil(sample_pos * kMasterClock / kVgmRate + kMasterClock / 10);
  g_top->final();

  // Resample the native 49715.97 Hz stream (master/288) to 44.1 kHz
  // with linear interpolation.
  const double native_rate = static_cast<double>(kMasterClock) / 288.0;
  const size_t want = total_samples ? total_samples : 0;
  const size_t covered = static_cast<size_t>(sample_pos);
  const size_t out_frames =
      stopped_early ? covered
                    : (want ? want : covered);
  std::vector<int16_t> out_l(out_frames), out_r(out_frames);
  for (size_t i = 0; i < out_frames; ++i) {
    const double src = i * native_rate / kVgmRate;
    const size_t j = static_cast<size_t>(src);
    const double frac = src - j;
    auto interp = [&](const std::vector<int16_t>& v) -> int16_t {
      if (j >= v.size()) return 0;
      const double a = v[j];
      const double b = (j + 1 < v.size()) ? v[j + 1] : a;
      return Clamp16(static_cast<int32_t>(a + (b - a) * frac));
    };
    out_l[i] = interp(g_cap_l);
    out_r[i] = interp(g_cap_r);
  }
  if (!WriteWav(output.c_str(), out_l, out_r, kVgmRate)) {
    std::fprintf(stderr, "cannot write %s\n", output.c_str());
    return 1;
  }
  std::printf("%s: %llu vgm samples, %zu native samples, %zu frames out\n",
              output.c_str(),
              static_cast<unsigned long long>(sample_pos), g_cap_l.size(),
              out_frames);
  return 0;
}
