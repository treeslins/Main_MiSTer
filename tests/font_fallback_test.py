"""Run the real font loader in a fresh process for each damaged-font fixture."""
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile


def main():
    executable = str(Path(sys.argv[1]).resolve())
    root = Path(__file__).resolve().parents[1]
    valid = (root / 'cn/assets/misterfpga.cn').read_bytes()
    cases = {'missing': None, 'empty': b'', 'truncated': valid[:-1]}
    for name, offset, replacement in [
        ('magic', 0, b'INVALID!'),
        ('version', 8, struct.pack('<I', 99)),
        ('empty-index', 12, struct.pack('<I', 0)),
        ('width', 20, b'\x11'),
        ('reserved', 21, b'\x01'),
        ('duplicate', 56, valid[16:20]),
        ('out-of-range', len(valid) - 40, struct.pack('<I', 0x110000)),
    ]:
        damaged = bytearray(valid)
        damaged[offset:offset + len(replacement)] = replacement
        cases[name] = bytes(damaged)
    # Keep this index ordered so the Unicode-scalar check is what rejects it.
    cases['surrogate'] = (valid[:12] + struct.pack('<I', 2) + valid[16:56] +
                          struct.pack('<I', 0xDFFF) + valid[60:96])
    with tempfile.TemporaryDirectory() as directory:
        for name, contents in cases.items():
            path = Path(directory) / (name + '.cn')
            if contents is not None:
                path.write_bytes(contents)
            env = dict(os.environ, MISTER_CN_FONT_PATH=str(path))
            subprocess.run([executable, '--expect-fallback'], env=env, check=True)
    print(f'Font fallback: {len(cases)} missing/corrupt fixtures PASS')


if __name__ == '__main__':
    main()
