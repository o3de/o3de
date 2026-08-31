#!/usr/bin/env python3
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
# gen_meshletpack_seeds.py
# -----------------------------------------------------------------------------
# Pre-deploy helper: enumerate the project's `.azmeshletpack` products and emit
# a seed list so they get packaged by `o3de export-project`.
#
# WHY THIS IS NEEDED
#   A `.azmeshletpack` is a per-model product (MeshletPackAsset). The builder
#   gives the *pack* a dependency ON its model (`modelAsset:{guid}`), and at
#   runtime `PackResolver.Find(modelId)` discovers the pack by SCANNING the
#   asset catalog by type. Nothing references the pack by assetId, and the model
#   does NOT depend on the pack -- so the bundler's dependency walker never
#   reaches it and the pack is left out of the deploy bundle. Runtime-discovered
#   assets like this must be SEEDED explicitly. Pack files are project content,
#   so they belong in a *project* seed list (the gem's own seedList.seed only
#   covers the gem's passes/shaders). See memory: o3de-deploy-seedlist-rules.
#
# USAGE
#   1) Run after assets are processed, before/at export:
#        python Gems/Meshlets/Tools/gen_meshletpack_seeds.py \
#            --project-path <YourProject> [--platform pc]
#      -> writes <YourProject>/AssetBundling/SeedLists/MeshletPacks.seed
#
#   2) Pass that seed list to the exporter so the packs land in the game bundle:
#        o3de export-project ... --seedlist AssetBundling/SeedLists/MeshletPacks.seed
#      (the exporter forwards --seedlist into the *game* asset list, which is
#       where per-project content belongs).
#
# The seed assetIds (guid+subId) are resolved by AssetBundlerBatch against the
# real asset catalog -- no hand-computed GUIDs -- so this stays correct even if
# the pack product naming changes.
import argparse
import glob
import os
import pathlib
import subprocess
import sys

PACK_EXT = ".azmeshletpack"


def find_engine_root(start: pathlib.Path) -> pathlib.Path:
    # Tools/ -> Meshlets/ -> Gems/ -> <engine root>
    for p in [start, *start.parents]:
        if (p / "engine.json").is_file():
            return p
    return start.parents[3] if len(start.parents) >= 4 else start


def autodetect_bundler(engine_root: pathlib.Path) -> str | None:
    exe = "AssetBundlerBatch.exe" if os.name == "nt" else "AssetBundlerBatch"
    patterns = [
        engine_root / "install" / "bin" / "*" / "*" / "*" / exe,
        engine_root / "install" / "bin" / "*" / "*" / exe,
        engine_root / "build" / "*" / "bin" / "profile" / exe,
        engine_root / "build" / "*" / "bin" / "*" / exe,
    ]
    for pat in patterns:
        hits = sorted(glob.glob(str(pat)))
        if hits:
            return hits[-1]
    return None


def main() -> int:
    script_dir = pathlib.Path(__file__).resolve().parent
    engine_root = find_engine_root(script_dir)

    ap = argparse.ArgumentParser(description="Emit a seed list of .azmeshletpack products for export-project.")
    ap.add_argument("--project-path", required=True, help="Path to the O3DE project being deployed.")
    ap.add_argument("--platform", default="pc", help="Asset platform (default: pc).")
    ap.add_argument("--bundler", default=None, help="Path to AssetBundlerBatch (auto-detected if omitted).")
    ap.add_argument("--out", default=None,
                    help="Output .seed (default: <project>/AssetBundling/SeedLists/MeshletPacks.seed).")
    args = ap.parse_args()

    project = pathlib.Path(args.project_path).expanduser().resolve()
    cache = project / "Cache" / args.platform
    if not cache.is_dir():
        print(f"ERROR: asset cache not found: {cache}\n"
              f"       Run the Asset Processor for project '{project.name}' / platform '{args.platform}' first.",
              file=sys.stderr)
        return 2

    bundler = args.bundler or autodetect_bundler(engine_root)
    if not bundler or not pathlib.Path(bundler).is_file():
        print(f"ERROR: AssetBundlerBatch not found (searched under {engine_root}). "
              f"Pass it explicitly with --bundler.", file=sys.stderr)
        return 2

    packs = sorted(p for p in cache.rglob("*") if p.suffix.lower() == PACK_EXT)
    out = pathlib.Path(args.out).resolve() if args.out \
        else project / "AssetBundling" / "SeedLists" / "MeshletPacks.seed"
    out.parent.mkdir(parents=True, exist_ok=True)
    if out.exists():
        out.unlink()  # regenerate fresh so removed packs don't linger

    if not packs:
        print(f"No '{PACK_EXT}' products under {cache} -- nothing to seed.\n"
              f"(Add a Meshlet Pack rule to a source FBX or author a .meshletpack JSON sidecar, "
              f"then reprocess.)")
        return 0

    rel_paths = [p.relative_to(cache).as_posix() for p in packs]
    cmd = [str(bundler), "seeds", "--seedListFile", str(out)]
    for r in rel_paths:
        cmd += ["--addSeed", r]
    cmd += ["--platform", args.platform, "--project-path", str(project)]

    print(f"Seeding {len(rel_paths)} '{PACK_EXT}' product(s) into {out}:")
    for r in rel_paths:
        print(f"    {r}")
    rc = subprocess.call(cmd)
    if rc != 0:
        print(f"ERROR: AssetBundlerBatch exited {rc}", file=sys.stderr)
        return rc

    print(f"\nDone. Pass this to the exporter so the packs are bundled:\n"
          f"    o3de export-project ... --seedlist {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
