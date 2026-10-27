#!/usr/bin/env bash
set -euo pipefail
root="${1:?upstream Menu_MiSTer directory}"
cp experiment/osd256/cn_osd240_spi_adapter.v "$root/sys/"
cp experiment/osd256/cn_osd240_dualclock_ram.v "$root/sys/"
cp experiment/osd256/cn_osd240_pixel.v "$root/sys/"
python3 experiment/osd256/apply_upstream_write_integration.py "$root"
python3 - "$root/sys/osd.v" <<'PY'
from pathlib import Path
import sys
src=Path(sys.argv[1]).read_text()
assert ".clk(clk_sys), .reset(1'b0)" in src, "SPI mode wrongly tied to OSD enable"
assert ".x(osd_hcnt[7:0]), .y(osd_vcnt[7:0])" in src
assert "cn_osd240_dualclock_ram cn240_bank" in src
assert "if (!cn240_vs_prev && vs_in)" in src
assert "cn240_mode_224p_video <= cn240_mode_224p_sync;" in src
assert "cn240_enabled_meta <= cn240_enabled;" in src
assert "cn240_enabled_video <= cn240_enabled_meta;" in src
assert "cn240_video_active && cn240_video_pixel" in src
assert "cn240_preview_pixel" in src
print("PASS: persistent SPI-mode connection and actual scan counter tap")
PY
# Full Menu_MiSTer elaboration requires all official sys/ modules.
# This narrower HDL probe still compiles the actual patched upstream osd.v
# and both newly referenced modules in iverilog.
iverilog -g2012 -Wall -DMENU_CORE -s osd -o /tmp/cn-osd240-patched.vvp \
  "$root/sys/osd.v" "$root/sys/cn_osd240_spi_adapter.v" "$root/sys/cn_osd240_dualclock_ram.v" "$root/sys/cn_osd240_pixel.v"
# Also elaborate the explicitly unsafe preview branch. This is a syntax
# check ONLY; the shipped/default video output remains the original path.
iverilog -g2012 -Wall -DMENU_CORE -DCN_OSD240_PREVIEW_UNSAFE -s osd \
  -o /tmp/cn-osd240-preview.vvp \
  "$root/sys/osd.v" "$root/sys/cn_osd240_spi_adapter.v" \
  "$root/sys/cn_osd240_dualclock_ram.v" "$root/sys/cn_osd240_pixel.v"
echo "PASS: explicitly gated preview video branch also elaborates."
echo "PASS: upstream sys/osd.v parses and elaborates with 240p write bank."
echo "NOT READY: video readout, ARM integration, clock-domain crossing, Quartus."
