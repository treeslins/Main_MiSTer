"""Rebuild a portable binary font from the checked-in Unifont HEX subset."""
from pathlib import Path
import struct
import sys

directory=Path(__file__).resolve().parent
entries=[]
for line in (directory/'zh_font.hex').read_text(encoding='utf-8').splitlines():
    cp,hx=line.split(':')
    width=len(hx)//4
    if width not in (8,16): raise ValueError(cp)
    rows=[int(hx[i:i+width//4],16) for i in range(0,len(hx),width//4)]
    cols=[sum(((rows[y]>>(width-1-x))&1)<<y for y in range(16)) for x in range(width)]
    entries.append((int(cp,16),width,cols+[0]*(16-width)))
if len({e[0] for e in entries})!=len(entries): raise ValueError('Duplicate glyph')
data=struct.pack('<I',len(entries))+b''.join(struct.pack('<IB16H',cp,width,*cols) for cp,width,cols in sorted(entries))
target=directory/'zh_font.bin'
if '--check' in sys.argv:
    if target.read_bytes()!=data: raise SystemExit('zh_font.bin is stale')
else: target.write_bytes(data)
terms=(directory/'zh_CN.tsv').read_text(encoding='utf-8')
missing=set(ord(c) for c in terms if ord(c)>127)-{e[0] for e in entries}
if missing: raise SystemExit('Missing translated glyphs: '+''.join(chr(c) for c in sorted(missing)))
print(f'Font: {len(entries)} glyphs; all translated characters covered')
