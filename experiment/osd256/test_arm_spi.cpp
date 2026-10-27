#include "../../cn/osd240_spi.h"
#include <cassert>
#include <cstdio>
#include <vector>
#include <cstdint>
static std::vector<std::vector<uint8_t>> packets;
static void begin(uint8_t cmd) { packets.push_back({cmd}); }
static void put(uint8_t b) { assert(!packets.empty()); packets.back().push_back(b); }
static void end() {}
int main() {
  cn_osd240::Sender s{begin,put,end};
  assert(cn_osd240::send_enable(s));
  assert(packets.back()==std::vector<uint8_t>({0xF0,'C','N','2','4'}));
  assert(cn_osd240::send_mode(s,true));
  assert(packets.back()==std::vector<uint8_t>({0xF4,1}));
  uint8_t data[256]={}; data[0]=0x80; data[255]=0x01;
  auto before=packets.size();
  assert(!cn_osd240::send_row(s,cn_osd240::Bank::Body,26,data,256));
  assert(!cn_osd240::send_row(s,cn_osd240::Bank::Header,3,data,256));
  assert(!cn_osd240::send_row(s,cn_osd240::Bank::Body,0,data,255));
  assert(packets.size()==before);
  assert(cn_osd240::send_row(s,cn_osd240::Bank::Body,25,data,256));
  assert(packets.back().size()==258);
  assert(packets.back()[0]==0xF2 && packets.back()[1]==25);
  assert(packets.back()[2]==0x80 && packets.back()[257]==0x01);
  assert(cn_osd240::send_row(s,cn_osd240::Bank::Header,2,data,256));
  assert(packets.back()[0]==0xF3 && packets.back()[1]==2);
  assert(cn_osd240::send_disable(s));
  assert(packets.back()==std::vector<uint8_t>({0xF1}));
  puts("PASS: ARM OSD240 enable/mode/body/header/disable packet formatting");
}
