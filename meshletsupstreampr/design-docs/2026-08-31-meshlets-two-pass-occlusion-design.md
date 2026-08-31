# Meshlets — two-pass occlusion culling design (AS path)

**Status:** IMPLEMENTED IN FULL (2026-08-31, unbuilt — latency half included; see the
implementation notes appended at the bottom). The correctness half — **occlusion-safe
depth** — was implemented first
(2026-08-31, unbuilt): under `r_meshletsMsCullAS` the depth prepass item rides its own
DrawPacket (`MeshletsRenderInstance::DepthDrawPacket`) with its own instance SRG whose
`m_doHiZCull` is never set, so depth always renders the complete (cut-selected) set and
the HiZ pyramid is never built from occlusion-culled depth. Forward/motion keep
prev-frame HiZ. What remains below is the latency half: recovering the shading cost of
false-culls in the SAME frame instead of one frame late.

## Problem

Prev-frame HiZ culling has two failure modes:
1. **Feedback**: if the depth prepass itself is HiZ-culled, a false-cull removes the
   cluster from depth → next frame's pyramid marks that region "far" → the cluster
   stays culled. Permanent holes. **Fixed** by occlusion-safe depth (above).
2. **Latency**: a cluster that becomes visible this frame (camera cut, fast motion,
   occluder moved) was culled against last frame's pyramid and only appears next
   frame. One-frame pops on disocclusion. This is what true two-pass removes.

## Design (Timberdoodle/Nanite shape, adapted to the meshlet AS path)

Frame layout:

1. **Pass 1 (existing depth prepass item)**: AS culls with frustum + cone + DAG cut +
   PREV-frame HiZ, and additionally writes a per-cluster visibility bitfield
   (`RWStructuredBuffer<uint> m_visBits`, 1 bit per DAG-range cluster, per instance) —
   bit set for every cluster it dispatched. NOTE: this reintroduces HiZ on depth, which
   is safe ONLY together with pass 2 (pass 2 completes the depth).
2. **HiZ regenerate**: the persistent HiZ pass (already in MainPipeline's
   GpuCullAndDrawPass, after DepthPrePass) builds THIS frame's pyramid from pass-1
   depth.
3. **Pass 2 (new raster pass, inserted after GpuCullAndDrawPass, before OpaquePass)**:
   a second depth-only meshlet item whose AS tests ONLY clusters whose bit is UNSET
   (skipped by pass 1), against THIS frame's pyramid. Survivors are the disoccluded
   set — drawn into depth. Typically near-zero clusters; the pass is cheap.
4. **Forward/motion**: cull against this frame's pyramid state as of pass 2 — or,
   simpler v1, draw the UNION (bit set in pass 1 OR pass 2) via the bitfield alone,
   no third HiZ test.

## Required infrastructure (why this is its own phase)

- A new gem-injected raster pass between GpuCullAndDrawPass and OpaquePass bound to
  the SAME depth attachment as DepthPrePass (pass template + auto-injection like
  `TryAutoInjectCullPass`).
- Reading the CURRENT frame's in-progress pyramid slot from pass 2 / forward requires
  frame-graph-declared reads AFTER the HiZ write — the existing early barrier-pass
  trick (which transitions the LAST-completed slot before DepthPrePass) does not
  cover it. Either declare the read on the new pass (it IS after the write) or add a
  second barrier pass post-HiZ.
- Per-instance visibility bitfields: UAV in pass 1's AS, SRV in pass 2's — sized
  ceil(dagClusterCount/32) u32s, allocated alongside the cull buffers, imported into
  the frame graph both passes (same MeshletsImportedAttachment machinery, buffer form).
- AS variants: pass-1 AS = current AS + bit writes; pass-2 AS = bit-test + current-HiZ
  test. Both compose the existing MeshletsClusterCullMath functions.

**Effort**: ~2-3 wk. **Exit**: camera cuts and fast occluder motion show zero
one-frame pops (validate with a swinging occluder + freeze-cull), pyramid feedback
stays impossible, and total AS+raster cost stays below the single-pass baseline in an
occlusion-heavy scene.

**Explicitly not now**: per-shadow-cascade two-pass (light-view pyramids), and using
the bitfield to skip forward-pass AS work entirely (needs DrawIndirect of the AS —
DispatchMeshIndirect RHI support, still absent).


---

## Implementation notes (latency half, 2026-08-31)

Deviations/decisions vs the sketch above:
- **Ledger, not bitfield**: `m_clusterVisFrame` is one u32 FRAME-ID per cluster
  (equality test against `m_frameId`, which starts at 1 and skips 0 on wrap) — a
  fresh zeroed buffer never matches, so NO clear pass exists at all.
- **Current-pyramid access is a pass CONNECTION, not a second import**:
  `GpuCullAndDrawTemplate` gained an `HiZOutput` slot (→ its HiZGeneratePass child);
  the injected `MeshletsLateDepthPass` (template `MeshletsLateDepthPassTemplate`,
  PassClass MeshletsRenderPass, DrawListTag `meshletslatedepth`, inserted via
  `AddPassAfter("GpuCullAndDrawPass")`) connects `HiZInput` to it. That declared read
  orders the UAV→shader-read transition after the mip-chain writes and makes the
  CURRENT slot (`HiZGeneratePass::GetCurrentPyramid()`, new accessor) legal for the
  late AS and every later pass (forward/motion sample it directly — no ledger read
  in forward, exactness comes from testing the same fresh pyramid).
- **Ledger barriers**: declared ReadWrite on the meshlets cull BARRIER pass
  (pre-depth UAV state + sync) and ReadWrite on the late pass
  (`MeshletsImportedAttachment::m_renderPassReadWrite`) — the sync point after
  pass 1's in-DepthPrePass-scope AS writes.
- **Pass roles**: depth packet SRG = visMode 1 + prev-frame pyramid (HiZ on depth is
  safe again only because pass 2 completes it; with the cvar off it reverts to
  occlusion-safe no-HiZ depth). Late packet SRG = visMode 2 + current pyramid +
  current matrix. Camera SRG = visMode 0 + current pyramid when two-pass is active,
  last-completed pyramid otherwise.
- Gate: `r_meshletsTwoPassOcclusion` (default off) on top of r_meshletsMsCullAS;
  toggle invalidates all packets. The late pass opens its scope only on frames with
  late work (`SetHasDrawWork`), and per-instance activation and that gate key off the
  same LateDepthDrawPacket/ledger state so they cannot disagree.
