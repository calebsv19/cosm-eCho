"""Exercise ordinary goal dispatch and refusals without a native build."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]

class DispatchTest(unittest.TestCase):
    def test_dispatch_and_refusals(self):
        with tempfile.TemporaryDirectory() as temp:
            base = Path(temp).resolve()
            repo = base / 'repo'
            repo.mkdir()
            subprocess.run(['git', 'init', '-q', str(repo)], check=True)
            for name in ('Makefile', 'make/preparation.mk', 'tools/packaging/prepare_shared_copy.py'):
                target = repo / name
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(ROOT / name, target)
            shared = repo / 'third_party/codework_shared'
            shared.mkdir(parents=True)
            (shared / 'input.c').write_text('source')
            output = base / 'owned build'
            args = ['make', '-C', str(repo), 'MAKE=/bin/echo',
                    'BUILD_ROOT='+str(output), 'PREPARATION_ROOT='+str(output)]
            result = subprocess.run(args+['run-headless-smoke', 'release-package-self-test'], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
            self.assertIn('PREPARATION_CHILD=1', result.stdout)
            self.assertIn('run-headless-smoke release-package-self-test', result.stdout)
            self.assertEqual((output/'preparation-shared/input.c').read_text(), 'source')
            result = subprocess.run(args+['run-headless-smoke'], capture_output=True, text=True)
            self.assertNotEqual(result.returncode, 0)
            for goal in ('release-notarize', 'package-desktop-refresh', 'clean'):
                result = subprocess.run(args+[goal], capture_output=True, text=True)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn('Unsupported contained preparation goal', result.stderr)
            mismatch = base/'mismatch'
            result = subprocess.run(args+['BUILD_ROOT='+str(mismatch), 'run-headless-smoke'], capture_output=True, text=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertFalse(mismatch.exists())

if __name__ == '__main__':
    unittest.main()
