#!/usr/bin/env python3
"""Convert the pinned ZLabs RoundPix 12px TTF to 16-row OSD columns.

Development-only tool: pip install fonttools pillow; pass the downloaded TTF.
The release build copies the checked-in binary verbatim.
"""
from pathlib import Path
import argparse
import hashlib
import struct

from fontTools.ttLib import TTFont
from PIL import Image, ImageDraw, ImageFont

SOURCE_SHA256 = "5adc3a83d11edc2e6f5bb3bc009dd9028dc055cbb3fc5f23715fb4de3c4e3eb0"
SOURCE_URL = "https://github.com/Astro-2539/ZLabs-RoundPix-12px/releases/download/Build_20260813/ZLabsRoundPix_12px_M_CN.ttf"

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("ttf", type=Path)
    parser.add_argument("--output", type=Path, default=Path("cn/assets/misterfpga.cn"))
    args = parser.parse_args()
    source = args.ttf.read_bytes()
    if hashlib.sha256(source).hexdigest() != SOURCE_SHA256:
        raise ValueError("Unexpected ZLabs 12px TTF; use the pinned Build_20260813 asset")
    codes = sorted(TTFont(args.ttf).getBestCmap())
    face = ImageFont.truetype(str(args.ttf), 12)
    records = []
    for cp in codes:
        width = round(face.getlength(chr(cp)))
        if width == 0:
            continue  # Combining marks cannot occupy a standalone OSD cell.
        if width not in (6, 12):
            raise ValueError(f"Unexpected advance {width} for U+{cp:04X}")
        bitmap = Image.new("L", (16, 16))
        ImageDraw.Draw(bitmap).text((0, 2), chr(cp), font=face, fill=255, anchor="lt")
        columns = []
        for x in range(16):
            bits = sum(1 << (15 - y) for y in range(16) if bitmap.getpixel((x, y)) >= 128)
            columns.append((bits >> 8) & 255)
        for x in range(16):
            bits = sum(1 << (15 - y) for y in range(16) if bitmap.getpixel((x, y)) >= 128)
            columns.append(bits & 255)
        records.append(struct.pack("<IB3x", cp, width) + bytes(columns))
    output = b"M16CN002" + struct.pack("<II", 1, len(records)) + b"".join(records)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(output)
    print(f"{len(records)} glyphs, {len(output)} bytes, sha256 {hashlib.sha256(output).hexdigest()}")

if __name__ == "__main__":
    main()
