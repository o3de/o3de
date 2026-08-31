# Meshlets hardware mesh-shader program — upstream PR package

Source: NightFox (internal O3DE fork), branch `feature/atom-render-scale`,
commit `a204f69f3f5`, diffed against `o3de/o3de` `development` @ `5c38f36e3fd`
(2026-08-31). Author sign-off: DCO (`git commit -s`) on the fork commit.

## What this is

The complete source material to build a mainline PR (or PR SERIES — see
"Suggested split") for the hardware mesh-shader meshlets program:

- **DispatchMesh rendering** of meshlet clusters through the standard Atom
  passes (forward PBR / depth prepass / shadows / motion vectors).
- **Amplification-shader cluster culling**: frustum + backface-cone + HiZ
  occlusion, plus per-triangle backface/degenerate/micro-polygon/big-triangle
  culls in the mesh shader.
- **Persistent double-buffered HiZ pyramid** + **two-pass occlusion** (frame-id
  visibility ledger, late depth pass re-testing only skipped clusters against
  the current pyramid — no disocclusion pop, no pyramid feedback).
- **Cluster-DAG continuous LOD** (Nanite-style): meshoptimizer-built
  simplification DAG with group-shared monotonic errors; a flat per-cluster
  screen-space-error cut — no traversal, crack-free by construction.
- **Geometry streaming/paging**: self-contained pages, always-resident coarse
  set, CPU residency (LRU + hysteresis), a fixed-slot GPU page pool filled by
  compute-dispatch uploads, a residency-aware fail-safe-coarse cut, and a
  monolithic-drop switch for real VRAM reclaim.
- The `.azmeshletpack` offline pipeline (meshoptimizer clusterization, LODs,
  DAG, pages) + unit tests for the builder, residency, and streaming soak.

## ⚠ Status disclosure (read first)

The final session's work (DAG / two-pass / streaming — fork commit
`a204f69f3f5`, patch 0001) is **code-complete but not yet compiled or
visually verified**; the verification ladder is in `design-docs/`. Earlier
layers (DispatchMesh path, AS cull bring-up, RHI mesh-shader support) ARE
built and were verified rendering on DX12/AMD in the fork. Do not open the
upstream PR before running the ladder.

## Package contents

- `patches/0001-*.patch` — the final fork commit (DAG + two-pass + streaming),
  `git am`-able onto the FORK, reference-quality history for the reviewer.
- `patches/0002-meshlets-gem-vs-upstream.patch` — the ENTIRE `Gems/Meshlets`
  vs mainline (the gem is effectively rewritten; mainline's is the old
  experimental compute gem).
- `patches/0003-engine-mesh-shader-prereqs-vs-upstream.patch` — the engine
  prerequisites vs mainline: RHI `ShaderStage::Mesh/Amplification` +
  `DrawType::DispatchMesh` + pipeline descriptor mesh/amp functions; DX12
  stream-subobject mesh PSOs, `DispatchMesh` submission, mesh root-signature
  flags, capability probe; shader-compile pipeline (azslc Mesh/Amplification
  entries → `ms_6_5`/`as_6_5`, no-Vertex program relax, builder version
  bumps); Vulkan (`VK_EXT_mesh_shader`, `-fspv-target-env=vulkan1.3` — the
  AMD-critical flag) and Metal switch-compat; RPI GpuDriven passes
  (HiZGeneratePass incl. the persistent pyramid, GpuCullPass,
  IndirectRasterPass) + PassFactory registration.
- `patches/0004-integration-layer-vs-upstream.patch` — Mesh component
  `m_useMeshlets` entry point + sidecar auto-write, GpuDriven pass/shader
  assets, MainPipeline wiring, pass-template registry. **CAVEAT: these files
  carry OTHER fork changes too; extract the meshlets/GpuDriven hunks rather
  than applying wholesale.**
- `gem-snapshot/Gems/Meshlets/` — the full gem tree at the fork commit
  (apply-without-git fallback; easiest reviewer browsing).
- `design-docs/` — architecture + the three 2026-08-31 designs (DAG LOD,
  two-pass occlusion, streaming/paging) with implementation notes and the
  runtime verification protocols.

## Suggested upstream split (in dependency order)

1. RHI mesh/amplification stages + DX12 backend + capability probe.
2. Shader-compile pipeline (azslc/DXC ms_6_5/as_6_5, no-Vertex relax).
3. Vulkan/Metal compat.
4. RPI GpuDriven passes (HiZ incl. persistent, cull, indirect raster).
5. The Meshlets gem (0002) + Mesh component integration (extracted from 0004).
Each lands independently buildable; 1 is the only one with a hard gotcha:

## Reviewer gotchas (learned the hard way in the fork)

- **Growing `RHI::ShaderStageCount` invalidates every serialized shader asset**
  (`ShaderStageAttributeMapList`/`m_functionsByStage` are Count-sized; the
  TypeId is size-dependent). The shader builder version bumps in patch 0003
  force the one-time full reprocess — do not drop them.
- Vulkan mesh stages MUST compile with `-fspv-target-env=vulkan1.3` or DXC
  emits `SPV_NV_mesh_shader`, which does not load on AMD.
- AMD/DX12: typed `Buffer<T>` SRVs on pooled buffers mis-set NumElements —
  every meshlet buffer is deliberately a StructuredBuffer.
- The persistent HiZ template is consumed by the meshlets FP via
  `GetLastCompletedPyramid()`; the current-frame slot is exposed as a pass
  CONNECTION (`GpuCullAndDrawTemplate` `HiZOutput`) — never import the same
  RHI image under a second attachment id.
- CPU→GPU uploads for meshlet data use ONLY the ReadOnly-pool initial-data
  path (post-creation `UpdateData` on pooled buffers proved unreliable on
  AMD); the streaming page pool is GPU-written by a compute copy for the
  same reason.
- Everything ships OFF by default: `r_meshletsHwMeshShader`,
  `r_meshletsMsCullAS`, `r_meshletsDagLod`, `r_meshletsTwoPassOcclusion`,
  `r_meshletsStreaming*`, and the `.meshletpack` sidecar opt-ins
  (`generate_cluster_dag`, `generate_pages`). Pack format versions are
  strictly additive (v2/v3/v4) with duplicate-fallback.

## Verification ladder (run before opening the PR)

1. Build `Meshlets`, `Meshlets.Editor`, `Meshlets.Builders(+Tests)`,
   `Meshlets.Tests`; run `MeshletPackBuilderDag.*`, `MeshletPackBuilderPages.*`,
   `MeshletsPageResidency.*`, `MeshletsStreamingSoak.*` via AzTestRunner.
2. Visual: DispatchMesh parity → AS cull toggles + freeze-cull → DAG error-px
   sweep (crack-free recession, shadows follow) → two-pass (swinging occluder,
   no one-frame holes) → streaming soak protocol (in the streaming design doc)
   → `r_meshletsStreamingExclusive` (PIX: monolithic buffers gone).

## House process for the actual submission

Per the fork's contribution playbook: rebuild the change in a fresh worktree
off `o3de-upstream/development` against clean files (not a cherry-pick),
`git format-patch --zero-commit --no-signature`, verify with `git am` in a
throwaway worktree, DCO sign-off on every commit. The patches in this zip are
the source material and review reference, not the final submission artifacts.
