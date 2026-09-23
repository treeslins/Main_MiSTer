"""Generate a deterministic, longest-phrase-first display translation table."""
from pathlib import Path
import json
import sys

directory = Path(__file__).resolve().parent
terms = {}
for number, line in enumerate((directory / 'zh_CN.tsv').read_text(encoding='utf-8').splitlines(), 1):
    if not line or line.startswith('#'):
        continue
    key, value = line.split('\t')
    key = ' '.join(key.lower().split())
    if key in terms and terms[key] != value:
        raise SystemExit(f'Conflicting translation at line {number}: {key}')
    terms[key] = value
entries = sorted(terms.items(), key=lambda item: (-len(item[0]), item[0]))
content = ('// Generated from localization/zh_CN.tsv. SPDX-License-Identifier: GPL-3.0-or-later\n'
           '#pragma once\nstruct ZhTerm { const char *english; const char *chinese; };\n'
           'static const ZhTerm zh_terms[] = {\n' +
           ''.join('    {' + json.dumps(k, ensure_ascii=False) + ', u8' + json.dumps(v, ensure_ascii=False) + '},\n' for k, v in entries) + '};\n')
target = directory.parent / 'zh_catalog.h'
if '--check' in sys.argv:
    if target.read_text(encoding='utf-8') != content:
        raise SystemExit('zh_catalog.h is stale; run localization/generate_catalog.py')
else:
    target.write_text(content, encoding='utf-8', newline='\n')
print(f'Translation catalog: {len(entries)} unique phrases')
