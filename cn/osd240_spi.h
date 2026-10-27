// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstddef>
#include <cstdint>

// Experimental protocol; not enabled automatically and has no FPGA
// capability negotiation. Caller must ensure matching test menu.rbf.
namespace cn_osd240 {
enum class Bank : uint8_t { Body=0xF2, Header=0xF3 };
struct Sender {
  void (*begin)(uint8_t command);
  void (*byte)(uint8_t value);
  void (*end)();
};
inline bool send_enable(const Sender &s) {
  if (!s.begin || !s.byte || !s.end) return false;
  s.begin(0xF0);
  s.byte('C'); s.byte('N'); s.byte('2'); s.byte('4');
  s.end();
  return true;
}
inline bool send_mode(const Sender &s, bool use_224p) {
  if (!s.begin || !s.byte || !s.end) return false;
  s.begin(0xF4); s.byte(use_224p ? 1 : 0); s.end();
  return true;
}
inline bool send_row(const Sender &s, Bank bank, uint8_t row,
                     const uint8_t *data, std::size_t len) {
  if (!s.begin || !s.byte || !s.end || !data || len != 256) return false;
  if (row >= (bank==Bank::Header ? 3 : 28)) return false;
  s.begin(static_cast<uint8_t>(bank));
  s.byte(row);
  for (std::size_t i=0; i<256; ++i) s.byte(data[i]);
  s.end();
  return true;
}
inline bool send_disable(const Sender &s) {
  if (!s.begin || !s.end) return false;
  s.begin(0xF1); s.end(); return true;
}
} // namespace cn_osd240
