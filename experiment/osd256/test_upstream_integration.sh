#!/usr/bin/env bash
set -euo pipefail
root="${1:?upstream Menu_MiSTer directory}"
cp experiment/osd256/cn_osd240_spi_adapter.v "$root/sys/"
cp experiment/osd256/cn_osd240_ram.v "$root/sys/"
python3 experiment/osd256/apply_upstream_write_integration.py "$root"
# Full Menu_MiSTer elaboration requires all official sys/ modules.
# This narrower HDL probe still compiles the actual patched upstream osd.v
# and both newly referenced modules in iverilog.
iverilog -g2012 -Wall -DMENU_CORE -s osd -o /tmp/cn-osd240-patched.vvp \
  "$root/sys/osd.v" "$root/sys/cn_osd240_spi_adapter.v" "$root/sys/cn_osd240_ram.v"
echo "PASS: upstream sys/osd.v parses and elaborates with 240p write bank."
echo "NOT READY: video readout, ARM integration, clock-domain crossing, Quartus."
