#!/usr/bin/env python3
"""Guarded, repeatable overlay: fail closed when upstream source moves."""
import argparse
import shutil
from pathlib import Path

HERE = Path(__file__).resolve().parent

def inject_once(text, anchor, replacement, description):
    if replacement in text:
        raise RuntimeError(description + ' already patched; refuse double apply')
    count = text.count(anchor)
    if count != 1:
        raise RuntimeError('%s upstream changed: expected 1 anchor, got %d' % (description, count))
    return text.replace(anchor, replacement, 1)

def main():
    p = argparse.ArgumentParser()
    p.add_argument('upstream', type=Path)
    args = p.parse_args()
    osd = args.upstream / 'osd.cpp'
    if not osd.is_file() or not (args.upstream / 'Makefile').is_file():
        raise SystemExit('expected Main_MiSTer upstream root')
    s = osd.read_text(encoding='utf-8')
    s = inject_once(s, '#include "osd.h"',
                    '#include "osd.h"\n#include "cn_font.h"', 'include')
    opener = ('void OsdWriteOffset(unsigned char n, const char *s, unsigned char invert, unsigned char stipple, '
              'char offset, char leftchar, char usebg, int maxinv, int mininv)\n{')
    hook = (opener + '\n\t// Experimental, opt-in only; see cn/README.md.'
            '\n\tif (!stipple && !offset && !leftchar && !usebg &&'
            '\n\t    mister_cn_render(n, s, invert, osdbuf, &osdset)) return;')
    s = inject_once(s, opener, hook, 'OsdWriteOffset')
    s = inject_once(s, 'int n = is_menu() ? 19 : osd_size;',
        'int n = mister_cn_enabled() ? 32 : (is_menu() ? 19 : osd_size);', 'OsdUpdate')
    osd.write_text(s, encoding='utf-8')
    for name in ('cn_font.h', 'cn_font.cpp'):
        dst = args.upstream / name
        if dst.exists():
            raise RuntimeError(name + ' exists in upstream; refusing overwrite')
        shutil.copy2(HERE / name, dst)
    print('Guarded overlay applied to', args.upstream)

if __name__ == '__main__':
    main()
