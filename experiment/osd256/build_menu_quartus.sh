#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail

# Build only an actually integrated, extended 240p FPGA core; never package
# an unmodified upstream menu.rbf as though it were the new CRT OSD.
SRC="${1:?Menu_MiSTer source directory required}"
[[ -f "$SRC/menu.qpf" && -f "$SRC/menu.qsf" && -f "$SRC/sys/osd.v" ]] || {
  echo "ERROR: expected full Menu_MiSTer project" >&2; exit 2;
}
command -v quartus_sh >/dev/null || { echo "ERROR: Quartus Standard not installed on runner" >&2; exit 3; }
command -v quartus_cpf >/dev/null || { echo "ERROR: quartus_cpf missing" >&2; exit 3; }
# A future upstream-integrated patch must provide an unequivocal marker AND
# actual references to its implementation. Fail closed until implemented.
grep -q 'CN_OSD240_INTEGRATED' "$SRC/sys/osd.v" || {
  echo "BLOCKED: 240p video scanning and safe output integration are still incomplete." >&2
  echo "Refusing to compile an ordinary upstream menu.rbf as extended 240p." >&2
  exit 4
}
grep -q 'cn_osd240' "$SRC/sys/osd.v" || {
  echo "BLOCKED: expected 240p hardware modules are not wired" >&2; exit 4;
}
pushd "$SRC" >/dev/null
quartus_sh --flow compile menu
[[ -s output_files/menu.rbf ]] || {
  echo "ERROR: Quartus did not produce output_files/menu.rbf" >&2; exit 5;
}
mkdir -p "$GITHUB_WORKSPACE/dist"
cp output_files/menu.rbf "$GITHUB_WORKSPACE/dist/menu-cn-240p-EXPERIMENTAL.rbf"
sha256sum "$GITHUB_WORKSPACE/dist/menu-cn-240p-EXPERIMENTAL.rbf" > "$GITHUB_WORKSPACE/dist/sha256.txt"
popd >/dev/null
