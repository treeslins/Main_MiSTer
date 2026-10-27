#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail

# Build only an actually integrated, extended 256p FPGA core; never package
# an unmodified upstream menu.rbf as though it were the new CRT OSD.
SRC="${1:?Menu_MiSTer source directory required}"
[[ -f "$SRC/menu.qpf" && -f "$SRC/menu.qsf" && -f "$SRC/sys/osd.v" ]] || {
  echo "ERROR: expected full Menu_MiSTer project" >&2; exit 2;
}
command -v quartus_sh >/dev/null || { echo "ERROR: Quartus Standard not installed on runner" >&2; exit 3; }
command -v quartus_cpf >/dev/null || { echo "ERROR: quartus_cpf missing" >&2; exit 3; }
# Fail closed unless the generated upstream source routes pixels through the
# extension and preserves the stock OSD enable/active-area fallback.
grep -q 'CN_OSD240_INTEGRATED' "$SRC/sys/osd.v" || {
  echo "BLOCKED: extended OSD output path is not integrated." >&2
  exit 4
}
grep -q 'cn_osd240' "$SRC/sys/osd.v" || {
  echo "BLOCKED: expected 256p hardware modules are not wired" >&2; exit 4;
}
grep -q 'cn240_video_pixel' "$SRC/sys/osd.v" || { echo "BLOCKED: extended pixels are not routed to output" >&2; exit 4; }
grep -q 'ordout1 <= {{cn240_mux_pixel' "$SRC/sys/osd.v" || { echo "BLOCKED: pixel mixer still uses stock OSD only" >&2; exit 4; }
grep -q 'cn240_enabled_video && osd_en\[1\]' "$SRC/sys/osd.v" || { echo "BLOCKED: stock OSD enable fallback is missing" >&2; exit 4; }
for module in cn_osd240_spi_adapter cn_osd240_dualclock_ram cn_osd240_pixel cn_osd240_frame_mode cn_osd240_scan_pulses cn_osd240_vscan; do
  grep -q "$module.v" "$SRC/sys/sys.qip" || { echo "BLOCKED: $module is missing from Quartus sources" >&2; exit 4; }
done
pushd "$SRC" >/dev/null
quartus_sh --flow compile menu
[[ -s output_files/menu.rbf ]] || {
  echo "ERROR: Quartus did not produce output_files/menu.rbf" >&2; exit 5;
}
mkdir -p "$GITHUB_WORKSPACE/dist"
cp output_files/menu.rbf "$GITHUB_WORKSPACE/dist/menu-cn-256p-EXPERIMENTAL.rbf"
sha256sum "$GITHUB_WORKSPACE/dist/menu-cn-256p-EXPERIMENTAL.rbf" > "$GITHUB_WORKSPACE/dist/sha256.txt"
popd >/dev/null
