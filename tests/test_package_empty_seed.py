"""Behavior proofs use generated databases only, never an ambient seed."""
import importlib.util
import os
from pathlib import Path
import sqlite3
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("empty_seed", ROOT / "tools/packaging/generate_empty_seed.py")
seed = importlib.util.module_from_spec(spec)
spec.loader.exec_module(seed)
CLI = ROOT / "third_party/codework_shared/core/core_memdb/build/mem_cli"


class EmptySeedTests(unittest.TestCase):
    def launcher_fixture(self, root):
        contents = root / "eCho.app/Contents"
        (contents / "MacOS").mkdir(parents=True)
        (contents / "Resources/data").mkdir(parents=True)
        launcher = contents / "MacOS/mem-console-launcher"
        shutil.copyfile(ROOT / "tools/packaging/macos/mem-console-launcher", launcher)
        shutil.copyfile(ROOT / "tools/packaging/macos/Info.plist", contents / "Info.plist")
        seed.generate_seed(CLI, contents / "Resources/data/default.sqlite")
        return launcher

    def test_launcher_preserves_existing_runtime_database(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            launcher = self.launcher_fixture(root)
            runtime = root / "runtime"
            env = dict(os.environ, MEM_CONSOLE_RUNTIME_DIR=str(runtime),
                       MEM_CONSOLE_LOG_DIR=str(root / "logs"), TMPDIR=str(root))
            for _ in range(2):
                subprocess.run(["sh", str(launcher), "--print-config"], env=env,
                               check=True, stdout=subprocess.DEVNULL)
                database = runtime / "data/default.sqlite"
                if _ == 0:
                    seed.verify_empty_seed(database)
                    with sqlite3.connect(database) as db:
                        db.execute("INSERT INTO mem_tag(name) VALUES ('keep runtime record')")
                    before = database.read_bytes()
                else:
                    self.assertEqual(database.read_bytes(), before)

    def test_launcher_invalid_or_unavailable_root_never_falls_back(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            launcher = self.launcher_fixture(root)
            unavailable = root / "regular-file"
            unavailable.write_bytes(b"preserve")
            for selected in ("relative-runtime", str(root / ".." / "escape"), str(unavailable)):
                env = dict(os.environ, MEM_CONSOLE_RUNTIME_DIR=selected,
                           MEM_CONSOLE_LOG_DIR=str(root / "logs"), TMPDIR=str(root))
                result = subprocess.run(["sh", str(launcher), "--print-config"], env=env,
                                        cwd=root, capture_output=True)
                self.assertNotEqual(result.returncode, 0)
                self.assertFalse((root / "MemConsole").exists())
                self.assertFalse((root / "relative-runtime").exists())
                self.assertEqual(unavailable.read_bytes(), b"preserve")

    def test_generation_ignores_ambient_database_and_preserves_it(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            private = root / "ambient.sqlite"
            private.write_bytes(b"synthetic private sentinel; do not package")
            before = private.read_bytes()
            old = os.environ.get("CODEWORK_MEMDB_PATH")
            os.environ["CODEWORK_MEMDB_PATH"] = str(private)
            try:
                seed.generate_seed(CLI, root / "package/default.sqlite")
            finally:
                if old is None: os.environ.pop("CODEWORK_MEMDB_PATH", None)
                else: os.environ["CODEWORK_MEMDB_PATH"] = old
            self.assertEqual(private.read_bytes(), before)
            seed.verify_empty_seed(root / "package/default.sqlite")

    def test_existing_destination_is_preserved(self):
        with tempfile.TemporaryDirectory() as temp:
            output = Path(temp) / "existing.sqlite"
            output.write_bytes(b"preserve")
            with self.assertRaises(ValueError): seed.generate_seed(CLI, output)
            self.assertEqual(output.read_bytes(), b"preserve")

    def test_domain_records_are_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            output = Path(temp) / "seed.sqlite"
            seed.generate_seed(CLI, output)
            with sqlite3.connect(output) as db:
                db.execute("INSERT INTO mem_tag(name) VALUES ('synthetic contamination')")
            with self.assertRaises(ValueError): seed.verify_empty_seed(output)


if __name__ == "__main__":
    unittest.main()
