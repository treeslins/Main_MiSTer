#!/usr/bin/env python3
"""Verify the PINNED misterfpga.cn bitmap font; never regenerate it."""
from pathlib import Path
import argparse
import hashlib
import json
import struct

EXPECTED_SHA256 = "0f2857e3f2c559db4efdad381a0eb8384aa05810971ba9ab963535aa548b4ec9"
EXPECTED_SIZE = 531856
EXPECTED_COUNT = 13296

def verify(path: Path):
    data = path.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    if len(data) != EXPECTED_SIZE or digest != EXPECTED_SHA256:
        raise ValueError(f"PINNED FONT MISMATCH: expected {EXPECTED_SHA256}, got {digest} ({len(data)} bytes)")
    if len(data) < 16 or data[:8] != b"M16CN004":
        raise ValueError("bad M16CN004 font header")
    version, count = struct.unpack_from("<II", data, 8)
    if version != 1 or count != EXPECTED_COUNT or 16 + count * 40 != len(data):
        raise ValueError("unexpected Unicode table layout")
    codes = set()
    before = -1
    for i in range(count):
        offset = 16 + i * 40
        cp, advance = struct.unpack_from("<IB", data, offset)
        if cp <= before or cp > 0x10FFFF or 0xD800 <= cp <= 0xDFFF:
            raise ValueError(f"unordered/invalid Unicode value at index {i}")
        if advance not in (6, 12):
            raise ValueError(f"invalid advance width for U+{cp:04X}")
        before = cp
        codes.add(cp)
    ascii_count = sum(cp in codes for cp in range(32, 127))
    gb_count = sum(
        len((b := chr(cp).encode("gb2312", errors="ignore"))) == 2
        and 0xB0 <= b[0] <= 0xF7
        and 0xA1 <= b[1] <= 0xFE
        for cp in codes
    )
    if ascii_count != 95 or gb_count != 6763:
        raise ValueError(f"font coverage changed: ASCII {ascii_count}, GB2312 {gb_count}")
    catalog = Path(__file__).resolve().parents[1] / 'localization/zh_CN.tsv'
    translated = ''.join(line.split('\t', 1)[1] for line in
                         catalog.read_text(encoding='utf-8').splitlines()
                         if line and not line.startswith('#'))
    # Dynamic labels written directly by menu.cpp are outside the catalog.
    translated += '上级目录目录代码有线连接无线未连接电量充电放电核心分辨率菜单分辨率←→'
    required = {ord(char) for char in translated if not char.isspace()}
    missing = required - codes
    if missing:
        raise ValueError('Menu glyphs missing: ' + ', '.join(f'U+{cp:04X}' for cp in sorted(missing)))
    return {
        "format": "M16CN004", "source": "pinned-static-file",
        "sha256": digest, "file_size": len(data), "glyph_count": count,
        "ascii_printable_count": ascii_count, "gb2312_hanzi_count": gb_count,
        "menu_required_glyph_count": len(required), "menu_missing_glyph_count": len(missing),
    }

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path, default=Path("cn/assets/misterfpga.cn"))
    parser.add_argument("--report", type=Path, default=Path("dist/coverage.json"))
    args = parser.parse_args()
    report = verify(args.input)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report, ensure_ascii=False, indent=2))

if __name__ == "__main__":
    main()
