#!/usr/bin/env python3
"""Generate a package seed from schema code, never from a user's database."""
import argparse
import os
from pathlib import Path
import shutil
import sqlite3
import subprocess
import tempfile


def verify_empty_seed(path):
    with sqlite3.connect(path.resolve().as_uri() + "?mode=ro&immutable=1", uri=True) as db:
        if db.execute("PRAGMA integrity_check").fetchall() != [("ok",)]:
            raise ValueError("package seed integrity check failed")
        if db.execute("SELECT key, value FROM mem_meta").fetchall() != [("schema_version", "6")]:
            raise ValueError("package seed metadata is not the expected empty schema")
        for table in ("mem_item", "mem_tag", "mem_item_tag", "mem_link", "mem_audit", "mem_event", "mem_item_fts"):
            if db.execute('SELECT count(*) FROM "' + table + '"').fetchone()[0]:
                raise ValueError("package seed contains application records")


def generate_seed(mem_cli, output):
    output = Path(output)
    if os.path.lexists(output):
        raise ValueError("refusing to replace an existing seed destination")
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=".empty-seed-", dir=output.parent) as temp:
        seed = Path(temp) / "default.sqlite"
        subprocess.run([str(Path(mem_cli).resolve()), "list", "--db", str(seed.resolve())],
                       check=True, stdout=subprocess.DEVNULL)
        verify_empty_seed(seed)
        # Exclusive creation also protects an output introduced after the check.
        try:
            with output.open("xb") as target, seed.open("rb") as source:
                shutil.copyfileobj(source, target)
        except FileExistsError:
            raise ValueError("seed destination appeared during generation") from None
    verify_empty_seed(output)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mem-cli", required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    generate_seed(args.mem_cli, args.output)
    print("Verified empty synthetic package seed:", args.output)


if __name__ == "__main__":
    main()
