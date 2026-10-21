"""Exercise copy custody with a real miniature Git input tree."""
import importlib.util
from pathlib import Path
import subprocess
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('shared_copy', Path(__file__).resolve().parents[1] / 'tools/packaging/prepare_shared_copy.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class SharedCopyTest(unittest.TestCase):
    def test_copy_filters_generated_files_and_refuses_reuse(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp).resolve()
            repo = root / 'repo'
            repo.mkdir()
            subprocess.run(['git', 'init', '-q', str(repo)], check=True)
            source = repo / 'vendor'
            source.mkdir()
            (repo / '.gitignore').write_text('build/\n')
            (source / 'input.c').write_text('source')
            (source / 'build').mkdir()
            (source / 'build/old.a').write_text('preserved')
            output = root / 'output'
            self.assertEqual(module.prepare(source, output), 1)
            self.assertEqual((output / 'input.c').read_text(), 'source')
            self.assertFalse((output / 'build').exists())
            with self.assertRaises(FileExistsError):
                module.prepare(source, output)
            self.assertEqual((source / 'build/old.a').read_text(), 'preserved')
            with self.assertRaises(ValueError):
                module.prepare(source, source / 'copy')
            (root / 'link').symlink_to(root, target_is_directory=True)
            with self.assertRaises(ValueError):
                module.prepare(source, root / 'link/copy')
            (source / 'unsafe').symlink_to(repo / '.gitignore')
            with self.assertRaises(ValueError):
                module.prepare(source, root / 'other')
            self.assertFalse((root / 'other').exists())


if __name__ == '__main__':
    unittest.main()
