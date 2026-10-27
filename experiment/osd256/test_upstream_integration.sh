#!/usr/bin/env bash
set -euo pipefail
root="${1:?upstream Menu_MiSTer directory}"
cp experiment/osd256/cn_osd240_spi_adapter.v "$root/sys/"
cp experiment/osd256/cn_osd240_dualclock_ram.v "$root/sys/"
cp experiment/osd256/cn_osd240_pixel.v "$root/sys/"
cp experiment/osd256/cn_osd240_frame_mode.v "$root/sys/"
cp experiment/osd256/cn_osd240_scan_pulses.v "$root/sys/"
cp experiment/osd256/cn_osd240_vscan.v "$root/sys/"
python3 experiment/osd256/apply_upstream_write_integration.py "$root"
python3 - "$root/sys/osd.v" <<'PY'
from pathlib import Path
import sys
src=Path(sys.argv[1]).read_text()
assert ".clk(clk_sys), .reset(1'b0)" in src, "SPI mode wrongly tied to OSD enable"
assert ".x(cn240_scan_x), .y(cn240_scan_y)" in src
assert "if(ce_pix) cn240_scan_x_full <= osd_hcnt;" in src
assert "cn240_scan_x_full[7:0]" in src
assert "cn_osd240_vscan cn240_vscan" in src
assert "cn_osd240_dualclock_ram cn240_bank" in src
assert "cn_osd240_frame_mode cn240_frame_latch" in src
assert ".requested_224p_async(cn240_mode_224p_requested)" in src
assert ".mode_224p(cn240_mode_224p_requested)" in src
assert ".mode_224p(cn240_mode_224p_video)" in src
assert "cn_osd240_scan_pulses cn240_scan_events" in src
assert "cn240_enabled_meta <= cn240_enabled;" in src
assert "cn240_enabled_video <= cn240_enabled_meta;" in src
assert "(cn240_enabled_video && cn240_running_d2 &&" in src
assert "cn240_x_in_range_d2 && cn240_video_active) ?" in src
assert "cn240_video_pixel : osd_pixel" in src
assert "ordout1 <= {{cn240_mux_pixel" in src
assert "cn240_mux_pixel" in src
assert "cn240_running_d2 <= cn240_running_d1;" in src
assert "cn240_x_in_range_d1 <= (cn240_scan_x_full < 22\u0027d256);" in src
assert "cn240_x_in_range_d2 <= cn240_x_in_range_d1;" in src
assert "(cn240_enabled_video && osd_en[1]) ? (cn240_running_d2 && cn240_x_in_range_d2 && cn240_video_active) : osd_de[2]" in src
assert "CN_OSD240_INTEGRATED" in src
print("PASS: enabled extended video output, legacy fallback, and real scan taps")
PY
# Full Menu_MiSTer elaboration requires all official sys/ modules.
# This probe compiles the actual patched upstream osd.v and all new modules.
iverilog -g2012 -Wall -DMENU_CORE -s osd -o /tmp/cn-osd240-patched.vvp \
  "$root/sys/osd.v" "$root/sys/cn_osd240_spi_adapter.v" "$root/sys/cn_osd240_dualclock_ram.v" \
  "$root/sys/cn_osd240_pixel.v" "$root/sys/cn_osd240_frame_mode.v" \
  "$root/sys/cn_osd240_scan_pulses.v" "$root/sys/cn_osd240_vscan.v"
echo "PASS: actual extended video output path elaborates with explicit legacy fallback."
echo "Remaining gates: ARM command sender integration, Quartus synthesis, physical timing/hardware."
