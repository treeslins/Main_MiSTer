"""Rebuild the checked-in demo font without network access."""
from pathlib import Path


def generate(source):
    entries = []
    seen = set()
    for line in source.splitlines():
        cp, hx = line.split(':')
        code = int(cp, 16)
        if code in seen or len(hx) not in (32, 64):
            raise ValueError('Duplicate code point or invalid glyph: ' + cp)
        seen.add(code)
        width = len(hx) // 4
        rows = [int(hx[i:i+width//4], 16) for i in range(0, len(hx), width//4)]
        cols = [sum(((rows[y] >> (width-1-x)) & 1) << y for y in range(16)) for x in range(width)]
        entries.append('{0x%s, %d, {%s}}' % (cp, width, ','.join(hex(c) for c in cols)))
    return ('// Generated from localization/demo.hex; font license: localization/OFL-1.1.txt\n'
            '#pragma once\n#include <stdint.h>\n'
            'struct ZhGlyph { uint32_t code; unsigned width; uint16_t columns[16]; };\n'
            'static const ZhGlyph zh_glyphs[] = {\n' + ',\n'.join(entries) + '\n};\n')


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    directory = Path(__file__).resolve().parent
    generated = generate((directory / 'demo.hex').read_text(encoding='utf-8'))
    target = directory.parent / 'zh_font.h'
    if args.check:
        if target.read_text(encoding='utf-8') != generated:
            raise SystemExit('zh_font.h differs from demo.hex; regenerate the font')
        print('Font reproducibility: PASS')
    else:
        target.write_text(generated, encoding='utf-8', newline='\n')
