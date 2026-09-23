"""List source literals added upstream for a human translation review."""
import argparse
import re
import subprocess
from pathlib import Path


def git(*args):
    return subprocess.check_output(['git', *args], text=True, encoding='utf-8')


def report(base, upstream):
    ancestor = git('merge-base', base, upstream).strip()
    diff = git('diff', '--unified=0', ancestor, upstream, '--', '*.cpp', '*.h')
    quoted = re.compile(r'"((?:\\.|[^"\\])*)"')
    path = None
    line_no = 0
    items = []
    for line in diff.splitlines():
        if line.startswith('+++ b/'):
            path = line[6:]
        elif line.startswith('@@'):
            match = re.search(r'\+(\d+)', line)
            if match:
                line_no = int(match.group(1))
        elif line.startswith('+') and not line.startswith('+++'):
            source = line[1:]
            for match in quoted.finditer(source):
                value = match.group(1)
                if re.search(r'[A-Za-z]{3}', value) and not source.lstrip().startswith('#include'):
                    items.append((path, line_no, value))
            line_no += 1
    lines = ['# 上游新增文字校对', '',
             '以下是上游新增的 C/C++ 字符串常量。请检查用户可见文字是否已收入 `localization/zh_CN.tsv`；文件名、日志和技术标识可忽略。', '']
    if items:
        for path, number, value in items[:100]:
            safe = value[:200].replace('`', '\\`').replace('|', '\\|')
            if len(value) > 200:
                safe += '…'
            lines.append(f'- `{path}:{number}` — `{safe}`')
        if len(items) > 100:
            lines.append(f'- ……另有 {len(items) - 100} 项，请查看上游差异。')
    else:
        lines.append('本次上游差异没有新增 C/C++ 字符串常量。')
    lines.append('')
    return '\n'.join(lines)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--base', required=True)
    parser.add_argument('--upstream', required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.write_text(report(args.base, args.upstream), encoding='utf-8')
