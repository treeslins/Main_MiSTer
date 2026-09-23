import importlib.util
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

script = Path(__file__).resolve().parents[1] / 'localization/prepare_sync.py'
spec = importlib.util.spec_from_file_location('sync', script)
sync = importlib.util.module_from_spec(spec)
spec.loader.exec_module(sync)
review_spec = importlib.util.spec_from_file_location('review', script.with_name('review_upstream.py'))
review = importlib.util.module_from_spec(review_spec)
review_spec.loader.exec_module(review)


class MergeTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.previous = os.getcwd()
        os.chdir(self.tmp.name)
        self.git('init', '-b', 'chinese')
        self.git('config', 'user.name', 'Test')
        self.git('config', 'user.email', 'test@example.invalid')
        self.save('menu', 'base')
        self.git('branch', 'upstream')

    def tearDown(self):
        os.chdir(self.previous)
        self.tmp.cleanup()

    def git(self, *args):
        return subprocess.check_output(['git', *args], stderr=subprocess.STDOUT, text=True).strip()

    def save(self, file, value):
        Path(file).write_text(value)
        self.git('add', file)
        self.git('commit', '-m', value)

    def test_unchanged(self):
        result = sync.prepare('upstream', 'report.json')
        self.assertEqual(result['state'], 'unchanged')
        self.assertEqual(self.git('branch', '--show-current'), 'chinese')

    def test_merge_preserves_translation_and_base(self):
        self.save('translation', 'Chinese')
        base = self.git('rev-parse', 'chinese')
        self.git('checkout', 'upstream')
        self.save('feature', 'new feature')
        self.git('checkout', 'chinese')
        result = sync.prepare('upstream', 'report.json')
        self.assertEqual(result['state'], 'ready')
        self.assertEqual(Path('translation').read_text(), 'Chinese')
        self.assertEqual(Path('feature').read_text(), 'new feature')
        self.assertEqual(self.git('rev-parse', 'chinese'), base)

    def test_conflict_restores_base(self):
        self.save('menu', 'Chinese menu')
        base = self.git('rev-parse', 'HEAD')
        self.git('checkout', 'upstream')
        self.save('menu', 'changed English menu')
        self.git('checkout', 'chinese')
        result = sync.prepare('upstream', 'report.json')
        self.assertEqual(result['state'], 'conflict')
        self.assertEqual(result['files'], ['menu'])
        self.assertEqual(self.git('rev-parse', 'HEAD'), base)
        self.assertEqual(self.git('rev-parse', 'chinese'), base)
        self.assertEqual(Path('menu').read_text(), 'Chinese menu')
        self.assertFalse(Path('.git/MERGE_HEAD').exists())

    def test_dirty_checkout_rejected(self):
        Path('menu').write_text('unsaved work')
        with self.assertRaises(RuntimeError):
            sync.prepare('upstream', 'report.json')
        self.assertEqual(Path('menu').read_text(), 'unsaved work')

    def test_upstream_review_lists_new_source_text(self):
        self.git('checkout', 'upstream')
        self.save('menu.cpp', '#include "menu.h"\nOsdWrite(0, "New menu option");\n')
        self.git('checkout', 'chinese')
        report = review.report('chinese', 'upstream')
        self.assertIn('New menu option', report)
        self.assertIn('menu.cpp:2', report)
        self.assertNotIn('`menu.h`', report)


if __name__ == '__main__':
    unittest.main()
