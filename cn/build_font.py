#!/usr/bin/env python3
"""Convert ZLabs RoundPix .kbitx bitmaps into independent M16CN001 data.

Font source: SIL OFL 1.1. No Oldman binaries or licensed glyph data.
"""
import argparse
import base64
import json
import struct
import xml.etree.ElementTree as ET
from pathlib import Path

MAGIC = b'M16CN001'
HEADER = struct.Struct('<8sII')
ENTRY = struct.Struct('<IB3x')
STRIDE = 40

def uleb(raw, i):
    num = 0
    for shift in range(0, 35, 7):
        if i >= len(raw):
            raise ValueError('truncated ULEB')
        ch = raw[i]
        i += 1
        num |= (ch & 127) << shift
        if not ch & 128:
            return num, i
    raise ValueError('oversized ULEB')

def bitmap(encoded):
    raw = base64.b64decode(encoded + '=' * (-len(encoded) % 4))
    h, i = uleb(raw, 0)
    w, i = uleb(raw, i)
    if w > 64 or h > 64:
        raise ValueError('invalid bitmap dimensions')
    data = bytearray()
    total = w * h
    while len(data) < total:
        if i >= len(raw):
            raise ValueError('truncated bitmap')
        tag = raw[i]
        i += 1
        count = tag & 31
        if tag & 32:
            count <<= 5
        if count <= 0:
            raise ValueError('zero run')
        mode = tag & 192
        if mode == 0:
            data.extend(bytes(count))
        elif mode == 64:
            data.extend(b'\xff' * count)
        elif mode == 128:
            if i >= len(raw):
                raise ValueError('missing color')
            data.extend(bytes([raw[i]]) * count)
            i += 1
        else:
            if i + count > len(raw):
                raise ValueError('missing literal')
            data.extend(raw[i:i + count])
            i += count
    if len(data) != total or i != len(raw):
        raise ValueError('RLE length mismatch')
    return w, h, data

def convert(node, ascent, stats):
    cp = int(node.attrib['u'])
    dx = int(node.attrib.get('x', '0'))
    dy = ascent - int(node.attrib.get('y', '0'))
    width = int(node.attrib.get('w', '16'))
    cell = 8 if width <= 8 else 16
    if width not in (8, 16):
        stats['adjusted_advance'].append({'codepoint': cp, 'original': width, 'cell': cell})
    w, h, data = bitmap(node.attrib['d'])
    out = bytearray(32)
    cropped = 0
    for y in range(h):
        for x in range(w):
            if data[y*w+x] <= 127:
                continue
            xx, yy = dx+x, dy+y
            if 0 <= xx < cell and 0 <= yy < 16:
                out[xx + (yy // 8) * 16] |= 1 << (7 - (yy % 8))
            else:
                cropped += 1
    if cropped:
        stats['cropped_glyphs'] += 1
        stats['cropped_pixels'] += cropped
    return cp, cell, out

def main():
    arg = argparse.ArgumentParser()
    arg.add_argument('source', type=Path)
    arg.add_argument('--output', type=Path, default=Path('misterfpga.cn'))
    arg.add_argument('--report', type=Path, default=Path('coverage.json'))
    opt = arg.parse_args()
    root = ET.parse(opt.source).getroot()
    if root.tag != 'kbits':
        raise ValueError('input is not KBITX')
    props = {e.attrib['id']: int(e.attrib['value']) for e in root.findall('prop')}
    stats = {'format': 'M16CN001', 'cropped_pixels': 0, 'cropped_glyphs': 0,
             'adjusted_advance': []}
    glyphs = {}
    for node in root.findall('g'):
        if 'u' not in node.attrib or 'd' not in node.attrib:
            continue
        cp, adv, bits = convert(node, props.get('emAscent', 14), stats)
        if cp in glyphs:
            raise ValueError('duplicate codepoint ' + str(cp))
        glyphs[cp] = (adv, bits)
    required = '中文显示测试平潭游戏菜单0123ABC?'
    missing = [ch for ch in required if ord(ch) not in glyphs]
    if missing:
        raise ValueError('missing glyphs ' + ''.join(missing))
    opt.output.parent.mkdir(parents=True, exist_ok=True)
    with opt.output.open('wb') as dst:
        dst.write(HEADER.pack(MAGIC, 1, len(glyphs)))
        for cp in sorted(glyphs):
            advance, bits = glyphs[cp]
            dst.write(ENTRY.pack(cp, advance))
            dst.write(bits)
    assert opt.output.stat().st_size == HEADER.size + STRIDE*len(glyphs)
    count = 0
    for cp in glyphs:
        code = chr(cp).encode('gb2312', errors='ignore')
        if len(code) == 2 and 0xB0 <= code[0] <= 0xF7 and 0xA1 <= code[1] <= 0xFE:
            count += 1
    stats.update({'glyph_count': len(glyphs), 'file_size': opt.output.stat().st_size,
                  'gb2312_hanzi_count': count,
                  'ascii_printable_count': sum(cp in glyphs for cp in range(32,127))})
    opt.report.parent.mkdir(parents=True, exist_ok=True)
    opt.report.write_text(json.dumps(stats, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(stats, ensure_ascii=False, indent=2))

if __name__ == '__main__':
    main()
