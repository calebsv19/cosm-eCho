#!/usr/bin/env python3
import argparse
import hashlib
import json
import os
import re
import struct
import subprocess
from pathlib import Path

EXPECTED_SHARED_COMMIT = "854b51ff57c756b4565021fe759c27a459efbfd3"
SOURCE_MODULES = ("vk_runtime", "vk_renderer", "kit/kit_render", "kit/kit_ui",
                  "kit/kit_workspace_authoring", "kit/kit_pane", "core/core_pane", "core/core_layout")


def command_output(command: list[str], cwd: Path) -> str:
    result = subprocess.run(command, cwd=cwd, check=True, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    return result.stdout.strip()


def read_version(path: Path) -> tuple[int, int, int]:
    value = path.read_text(encoding="utf-8").strip()
    match = re.fullmatch(r"(\d+)\.(\d+)\.(\d+)", value)
    if not match:
        raise SystemExit(f"invalid version: {path}: {value!r}")
    return tuple(int(part) for part in match.groups())


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify_shared_source(root: Path, canonical: Path, require_current_canonical: bool = False) -> None:
    commit = command_output(["git", "rev-parse", "HEAD"], canonical)
    ancestry = subprocess.run(
        ["git", "merge-base", "--is-ancestor", EXPECTED_SHARED_COMMIT, commit],
        cwd=canonical, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if ancestry.returncode != 0:
        raise SystemExit("canonical shared does not contain accepted source commit: "
                         f"{EXPECTED_SHARED_COMMIT}, found {commit}")
    status = command_output(
        ["git", "status", "--porcelain", "--untracked-files=all", "--",
         *SOURCE_MODULES], canonical)
    if status and require_current_canonical:
        raise SystemExit("canonical shared Vulkan source is not exact/clean:\n" + status)

    tracked = command_output(
        ["git", "ls-tree", "-r", "--name-only", EXPECTED_SHARED_COMMIT, "--",
         *SOURCE_MODULES], canonical
    ).splitlines()
    if not tracked:
        raise SystemExit("canonical shared Vulkan source has no tracked files")
    # Only the text module advances in this development slice. Every other
    # adopted module remains bound to the original accepted snapshot.
    pin = json.loads(Path(__file__).with_name("text-render-source-pin.json").read_text())
    if pin["module"] != "kit/kit_render" or pin["version"] != "0.17.0":
        raise SystemExit("invalid text renderer source pin")
    expected = {}
    for relative in tracked:
        if relative.startswith("kit/kit_render/"):
            continue
        accepted_bytes = subprocess.check_output(
            ["git", "show", f"{EXPECTED_SHARED_COMMIT}:{relative}"], cwd=canonical)
        expected[relative] = hashlib.sha256(accepted_bytes).hexdigest()
    for name, sha in pin["sha256"].items():
        relative = Path(name)
        if relative.is_absolute() or ".." in relative.parts:
            raise SystemExit("unsafe text source pin path")
        expected["kit/kit_render/" + name] = sha
    mismatches = []
    for relative, sha in expected.items():
        adopted_path = root / relative
        canonical_path = canonical / relative
        if not adopted_path.is_file() or digest(adopted_path) != sha:
            mismatches.append(f"digest {relative}")
        if require_current_canonical and (not canonical_path.is_file() or digest(canonical_path) != sha):
            mismatches.append(f"canonical digest {relative}")
    if mismatches:
        raise SystemExit("adopted Vulkan source differs from declared pins: " +
                         ", ".join(mismatches[:12]))
    print(f"render fidelity exact-source contract: base={EXPECTED_SHARED_COMMIT}; "
          f"text=0.17.0 development snapshot; {len(expected)} files")


def bmp_evidence(path: Path) -> tuple[int, int, int]:
    data = path.read_bytes()
    if len(data) < 54 or data[:2] != b"BM":
        raise SystemExit(f"invalid BMP capture: {path}")
    pixel_offset = struct.unpack_from("<I", data, 10)[0]
    width, height = struct.unpack_from("<ii", data, 18)
    bits_per_pixel = struct.unpack_from("<H", data, 28)[0]
    if width <= 0 or height == 0 or bits_per_pixel not in (24, 32):
        raise SystemExit(
            f"invalid BMP geometry: {path}: {width}x{height} bpp={bits_per_pixel}")
    stride = ((width * bits_per_pixel + 31) // 32) * 4
    bytes_per_pixel = bits_per_pixel // 8
    colors = set()
    for row in range(0, abs(height), max(1, abs(height) // 64)):
        row_start = pixel_offset + row * stride
        for column in range(0, width, max(1, width // 64)):
            start = row_start + column * bytes_per_pixel
            colors.add(data[start:start + bytes_per_pixel])
    if len(colors) < 3:
        raise SystemExit(f"capture lacks nontrivial rendered content: {path}")
    return width, abs(height), len(colors)


def validation_environment(output_dir: Path) -> dict[str, str]:
    env = os.environ.copy()
    candidates = [
        Path("/opt/homebrew/opt/vulkan-validationlayers"),
        Path("/usr/local/opt/vulkan-validationlayers"),
    ]
    prefix = next((candidate for candidate in candidates
                   if (candidate / "lib/libVkLayer_khronos_validation.dylib").is_file()),
                  None)
    if prefix is None:
        raise SystemExit("Khronos validation layer library is unavailable")
    # Scope layer discovery without redirecting system ImageIO dependencies.
    layers = output_dir.resolve() / "validation-layers"
    layers.mkdir(parents=True, exist_ok=True)
    manifest = json.loads((prefix / "share/vulkan/explicit_layer.d/VkLayer_khronos_validation.json").read_text())
    manifest["layer"]["library_path"] = str((prefix / "lib/libVkLayer_khronos_validation.dylib").resolve())
    (layers / "VkLayer_khronos_validation.json").write_text(json.dumps(manifest))
    env.pop("DYLD_LIBRARY_PATH", None)
    env["VK_LAYER_PATH"] = str(layers)
    return env


def run_app(app: Path, initial: Path, resized: Path, log: Path,
            minimum_scale: float) -> None:
    for capture in (initial, resized):
        capture.unlink(missing_ok=True)
    env = validation_environment(log.parent)
    env["MEM_CONSOLE_VULKAN_ROLLOUT_INITIAL_CAPTURE"] = str(initial.resolve())
    env["MEM_CONSOLE_VULKAN_ROLLOUT_RESIZED_CAPTURE"] = str(resized.resolve())
    env["MEM_CONSOLE_VULKAN_ROLLOUT_MIN_SCALE"] = str(minimum_scale)
    result = subprocess.run([str(app.resolve()), "--vulkan-rollout-self-test"],
                            env=env, text=True, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT)
    output = result.stdout or ""
    log.parent.mkdir(parents=True, exist_ok=True)
    log.write_text(output, encoding="utf-8")
    print(output, end="")
    if result.returncode != 0:
        raise SystemExit(f"Memory Console Vulkan self-test exited {result.returncode}")
    if "[vk_runtime validation]" in output:
        raise SystemExit("Vulkan validation emitted warning/error diagnostics")
    receipts = re.findall(
        r"MEM_CONSOLE_VULKAN_RUNTIME schema=1 stage=(startup|resized|restart) "
        r"runtime=0\.6\.0 .* validation_requested=1 validation_enabled=1 "
        r"warnings=0 errors=0", output)
    if receipts != ["startup", "resized", "restart"]:
        raise SystemExit(f"incomplete runtime receipts: {receipts}")
    if not re.search(r"MEM_CONSOLE_VULKAN_ROLLOUT schema=1 status=pass\b", output):
        raise SystemExit("missing successful rollout receipt")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--shared-root", type=Path, required=True)
    parser.add_argument("--canonical-shared-root", type=Path, required=True)
    parser.add_argument("--require-current-canonical", action="store_true")
    parser.add_argument("--app", type=Path)
    parser.add_argument("--initial-capture", type=Path)
    parser.add_argument("--resized-capture", type=Path)
    parser.add_argument("--log", type=Path)
    parser.add_argument("--minimum-scale", type=float, default=1.0)
    args = parser.parse_args()

    root = args.shared_root.resolve()
    canonical = args.canonical_shared_root.resolve()
    verify_shared_source(root, canonical, args.require_current_canonical)
    runtime = read_version(root / "vk_runtime/VERSION")
    renderer = read_version(root / "vk_renderer/VERSION")
    if runtime != (0, 6, 0):
        raise SystemExit(f"vk_runtime 0.6.0 required, found {runtime}")
    if renderer != (1, 6, 0):
        raise SystemExit(f"vk_renderer 1.6.0 required, found {renderer}")
    if read_version(root / "kit/kit_render/VERSION") != (0, 17, 0):
        raise SystemExit("kit_render 0.17.0 required")

    if args.app:
        if not args.initial_capture or not args.resized_capture or not args.log:
            raise SystemExit("app execution requires both captures and a log")
        run_app(args.app, args.initial_capture, args.resized_capture, args.log,
                args.minimum_scale)

    if args.initial_capture or args.resized_capture:
        if not args.initial_capture or not args.resized_capture:
            raise SystemExit("both capture paths are required")
        initial = bmp_evidence(args.initial_capture)
        resized = bmp_evidence(args.resized_capture)
        if initial[:2] == resized[:2]:
            raise SystemExit(f"capture dimensions did not change: {initial[:2]}")
        print("Memory Console Vulkan captures: "
              f"initial={initial[0]}x{initial[1]} colors={initial[2]} "
              f"resized={resized[0]}x{resized[1]} colors={resized[2]}")

    print("Memory Console Vulkan rollout contract: "
          f"shared_root={root} canonical_commit={EXPECTED_SHARED_COMMIT} "
          f"vk_runtime={'.'.join(map(str, runtime))} "
          f"vk_renderer={'.'.join(map(str, renderer))}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
