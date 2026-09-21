"""Merge an upstream commit into a disposable candidate, never the base branch."""
import argparse
import json
import subprocess
from pathlib import Path


def git(*args, check=True):
    return subprocess.run(['git', *args], check=check, text=True,
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE)


def prepare(upstream, report):
    if git('status', '--porcelain').stdout.strip():
        raise RuntimeError('A clean disposable checkout is required')
    base = git('rev-parse', 'HEAD').stdout.strip()
    target = git('rev-parse', '--verify', upstream + '^{commit}').stdout.strip()
    status = {'base': base, 'upstream': target, 'state': 'unchanged'}
    if git('merge-base', '--is-ancestor', target, base, check=False).returncode == 0:
        Path(report).write_text(json.dumps(status, indent=2), encoding='utf-8')
        return status
    branch = 'sync/upstream-' + target[:12] + '-' + base[:12]
    git('checkout', '-b', branch)
    result = git('merge', '--no-ff', '--no-edit', target, check=False)
    if result.returncode:
        status.update(state='conflict', files=git('diff', '--name-only', '--diff-filter=U').stdout.splitlines(),
                      diagnostic=result.stdout + result.stderr)
        git('merge', '--abort')
        git('checkout', '--detach', base)
    else:
        status.update(state='ready', branch=branch, commit=git('rev-parse', 'HEAD').stdout.strip())
    Path(report).write_text(json.dumps(status, indent=2), encoding='utf-8')
    return status


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--upstream', default='refs/remotes/upstream/master')
    parser.add_argument('--report', required=True)
    args = parser.parse_args()
    print(json.dumps(prepare(args.upstream, args.report)))
