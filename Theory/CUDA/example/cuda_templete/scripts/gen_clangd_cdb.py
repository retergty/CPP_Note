#!/usr/bin/env python3
"""Write a clang-friendly compile_commands.json for clangd."""

from __future__ import annotations

import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / "clangd"
CLANGXX = "/usr/bin/clang++"

COMMON = [
    CLANGXX,
    "-x",
    "cuda",
    "-std=c++17",
    "--cuda-path=/usr/local/cuda",
    "--cuda-gpu-arch=sm_75",
    "--cuda-host-only",
    "--no-cuda-version-check",
    "-Wno-unknown-cuda-version",
    f"-I{ROOT / 'include'}",
    "-isystem",
    "/usr/local/cuda/include",
]


def entries() -> list[dict[str, object]]:
    files = sorted((ROOT / "src").glob("*.cu")) + sorted((ROOT / "include").glob("*.cuh"))
    result = []
    for path in files:
        result.append(
            {
                "directory": str(ROOT),
                "file": str(path),
                "arguments": COMMON + ["-c", str(path)],
            }
        )
    return result


def main() -> int:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    payload = entries()
    out = OUT_DIR / "compile_commands.json"
    out.write_text(json.dumps(payload, indent=2) + "\n")
    print(f"wrote {out} ({len(payload)} files)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
