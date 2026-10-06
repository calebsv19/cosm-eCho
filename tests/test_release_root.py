"""Synthetic containment and make-routing checks; no credentials or user DBs."""
import importlib.util
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("release_root", ROOT / "tools/packaging/prepare_release_root.py")
release_root = importlib.util.module_from_spec(spec)
spec.loader.exec_module(release_root)


class ReleaseRootTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name).resolve()
        self.source = self.root / "source/mem_console"
        self.data = self.root / "data"
        self.source.mkdir(parents=True)
        self.data.mkdir()

    def prepare(self, value):
        return release_root.prepare_root(str(value), self.source, self.data)

    def test_relative_and_split_data_target_roots(self):
        local = self.prepare("build/release-authenticated/rapcj_test")
        self.assertTrue(local.is_dir())
        target = self.data / "mem_console/build/release-authenticated/raor_test/targets/rapt_test"
        self.assertEqual(self.prepare(target), target)
        self.assertTrue(target.is_dir())

    def test_existing_destination_preserved(self):
        target = self.prepare("build/release-authenticated/rapcj_test")
        sentinel = target / "keep"
        sentinel.write_bytes(b"preserve existing candidate")
        with self.assertRaises(ValueError): self.prepare(target)
        self.assertEqual(sentinel.read_bytes(), b"preserve existing candidate")

    def test_escape_and_malformed_roots_rejected(self):
        for value in ("", "build/release", "build/release-authenticated/job/extra",
                      "build/release-authenticated/../escape", "build//release-authenticated/job",
                      str(self.root / "outside"), "build/release-authenticated/job/"):
            with self.subTest(value=value), self.assertRaises(ValueError): self.prepare(value)
        self.assertFalse((self.source / "build").exists())
        self.assertFalse((self.root / "outside").exists())

    def test_symlink_ancestor_rejected_without_writes(self):
        outside = self.root / "outside"
        outside.mkdir()
        (self.source / "build").symlink_to(outside, target_is_directory=True)
        with self.assertRaises(ValueError): self.prepare("build/release-authenticated/rapcj_test")
        self.assertEqual(list(outside.iterdir()), [])

    def test_make_routes_bundle_artifact_and_runtime_to_selected_root(self):
        selected = self.data / "mem_console/build/release-authenticated/rapcj_test"
        makefile = ('include make/paths.mk\nproof:\n'
                    '\t@printf "%s\\n" "$(RELEASE_DIR)" "$(PACKAGE_APP_DIR)" '
                    '"$$MEM_CONSOLE_RUNTIME_DIR" "$$MEM_CONSOLE_LOG_DIR" "$$CODEWORK_MEMDB_PATH"\n')
        result = subprocess.run(["make", "-s", "-f", "-", "proof", "RELEASE_ROOT=" + str(selected)],
                                cwd=ROOT, input=makefile, text=True, capture_output=True, check=True)
        self.assertEqual(result.stdout.splitlines(), [str(selected), str(selected / "bundle/eCho.app"),
                         str(selected / "diagnostics/runtime"), str(selected / "diagnostics/logs"),
                         str(selected / "diagnostics/runtime/data/default.sqlite")])
        self.assertFalse(selected.exists())


if __name__ == "__main__":
    unittest.main()
