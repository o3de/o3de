# Meshlets — geometry streaming / paging design (Phase 7 of the DAG program)

**Status:** PHASES 1-2 IMPLEMENTED (2026-08-31, unbuilt): page-aware builder + format v4 (PageTable/PageData/ParentIndex, PageMaxClusters=16 fixed-slot caps, duplicate-fallback ALWAYS on in v1) + the CPU residency core (MeshletsPageResidency: conservative classifier, hysteresis band, LRU-under-pressure, reserved-slot free-list; unit-tested) integrated behind r_meshletsStreaming with live ImGui stats — rendering still draws the monolithic fallback exactly as phase 2 prescribes. PHASE 3 ALSO IMPLEMENTED (2026-08-31, unbuilt): interior levels are now paged too (always-resident contiguous runs, PageFlagAlwaysResident — required so the AS path can draw entirely from the pool), one global fixed-slot u32 page pool (PageSlotU32s=36932 words/slot) uploaded by a MeshletsPageUpload compute dispatch on the cull compute pass (staging = ReadOnly initial-data; pool imported RW + finalized there; barrier pass declares it Read/pre-raster each frame), per-cluster paged map buffer ([31] group-complete, [30] resident, [29:8] slot, [7:0] local; rebuilt on residency change from ParentIndex-derived leaf groups with exact parent ranges), the fail-safe-coarse cut in the AS (leaf: complete&&resident; interior: forced when !complete), and paged fetch in all four AS-culled MS entries. m_pagedMode flips per mesh only once every always-resident page is uploaded, and same-frame coherence holds because uploads execute before DepthPrePass. PHASE 4 (soak) TOOLING + SIMULATIONS IMPLEMENTED (2026-08-31, unbuilt): live budget re-sizing (changing r_meshletsStreamingPoolMB rebuilds the pool in place), r_meshletsStreamingHysteresis (tunable eviction band, ImGui slider), churn counter (evict->reload within 60 frames), starved-pages readout (pool too small for the wanted set), a CancelLoad slot-leak fix the soak accounting surfaced, and five deterministic soak simulations in MeshletsStreamingSoakTest.cpp (budget sweep convergence at every capacity, teleport recovery within the throttle bound, hysteresis-band thrash suppression proven both ways, CancelLoad accounting, 500-update random-walk invariants). The GPU half of the soak follows the runtime protocol below once built. THE MONOLITHIC-DROP SWITCH IS ALSO IMPLEMENTED (r_meshletsStreamingExclusive, load-time): paged meshes then skip every monolithic geometry buffer (vertex-stream SRVs incl. the expanded-index buffer, index/IA buffers, MS triangle/indirection copies — the actual VRAM reclaim; cull metadata stays resident). Enforcement is by starvation: EnsureIndirectArgs/CreateAndBindRenderBuffers refuse for dropped meshes and every non-paged path's existing lazy-load guards self-skip permanently; BuildInstanceDrawPacket never falls through to vertex-pull; the AS gates on m_pagedExclusive (nothing dispatches until paged mode is live, and with the cut off, non-resident leaves are refused too). Such meshes render ONLY via hwMesh+MsCullAS(+DagLod)+Streaming; anything else renders nothing, by design (warned once). Flip requires a level reload. This is the residency
system that lets DAG-baked worlds exceed VRAM: coarse DAG levels always resident, fine
levels streamed in/out by what the cut actually requests. It is a multi-month program;
this spec fixes the architecture so the pack format work can land early (format churn
is the expensive part — it forces re-bakes).

## Scope guard

Do NOT start this until (a) Phase 6 DAG is visually verified and profiled, and (b) a
real content set demonstrates geometry memory pressure. All-resident DAG is ~2× LOD0
triangles; for current NightFox content that fits. Streaming pays off only at
open-world scale.

## 1. Page model (offline — fills the reserved PageTable/PageData sections)

- **Page = the unit of IO and residency**: a self-contained group of clusters (target
  ~128 KB) holding its clusters' descriptors, bounds, dag nodes, triangle words,
  indirection, AND its own vertex data (positions/normals/tangents/bitangents/uvs for
  the vertices its clusters reference — duplicated across pages when shared; ~10-15%
  overhead buys page self-containment, the property that makes everything else simple).
- **Cut-level partitioning**: pages group clusters of the SAME DAG level that are
  spatially adjacent (the builder already has `meshopt_partitionClusters`; reuse it at
  page granularity). Root + coarse levels (everything above a baked "always-resident
  cut", ~10-20% of clusters) go into the ALWAYS-RESIDENT page set — the mesh can render
  at coarse detail with zero pages streamed.
- **Format**: `SectionKind::PageTable` = per-page {byteOffset, byteSize, clusterFirst,
  clusterCount, dagLevel, residencySetId, aabb}; `SectionKind::PageData` = the page
  payloads, 64 KB-aligned, laid out for whole-page reads. PackVersion 4. The v3
  monolithic sections remain valid (a v4 pack carries BOTH only if
  `streamingDuplicateFallback` is set; default is PageData-only to avoid 2× disk).

## 2. Runtime residency (CPU-driven v1 — no GPU feedback yet)

- **Request source v1**: the CPU already computes per-instance camera distance and the
  DAG cut is a pure function of (groupSphere, groupError, camera). A CPU pass over the
  PAGE table (pages carry aabb + dagLevel → conservative errPx interval) classifies
  each page: needed / soon (prefetch ring) / evictable. No GPU readback in v1 — the
  same math the AS runs, evaluated at page granularity. Conservative by construction:
  a page is "needed" if ANY camera within the prefetch radius could select its level.
- **Residency states**: Resident / Loading / Evictable / NonResident, LRU over a fixed
  VRAM budget (`r_meshletsStreamingPoolMB`). Eviction only of pages whose level is
  strictly finer than the current cut needs everywhere (hysteresis band to stop
  thrash).
- **IO**: AZ::IO streamer reads straight from the pack asset (pages are contiguous
  spans); double-buffered upload through the existing ReadOnly-pool initial-data path
  (the ONLY upload path proven on this AMD GPU — see the SP1 UpdateData failures).
- **GPU tables**: one global cluster→page indirection is NOT needed in v1 because
  pages are level+locality grouped: instead each mesh keeps a per-cluster
  `m_clusterResident` bitfield (u32 per 32 clusters, CPU-written, uploaded on change).

## 3. Cut integration (the key invariant)

The cut test gains one clause, and it must FAIL SAFE COARSE:

```
DagCutAccepts(node) && ClusterResident(id)
    -> draw id
DagCutAccepts(node) && !ClusterResident(id)
    -> draw the nearest RESIDENT ANCESTOR instead (coarser, always resident by
       construction of the always-resident set)
```

Implementing "draw the ancestor" without traversal: a non-resident cluster simply
fails the cut; its ancestor's parentError test must then pass. That requires the
residency-aware cut to treat a non-resident CHILD region as "parent not refinable":
store per-node a `m_childrenResident` bit (CPU-maintained per group) and change the
parent-side test to `parentErrPx > tau || !childrenResident`. Group-shared like every
other DAG record, so it stays crack-free: either a whole group's children are resident
and refine together, or none do.

## 4. Later (v2+, explicitly deferred)

- GPU feedback buffer (clusters the AS wanted but found non-resident → readback →
  prioritized requests) replacing the CPU conservative classifier.
- Vertex-data dedup via a shared vertex page pool (drop the 10-15% duplication).
- Compressed pages (meshopt vertex/index codecs — the `m_flags` bit 0 the FileHeader
  reserved in SP1) decoded on the transfer queue.
- Shared residency across instances of different models (global pool today is
  per-mesh).

## 5. Order of work / effort

1. Page-aware builder + format v4 + roundtrip/self-containment unit tests — 3-4 wk.
2. CPU residency manager + IO + upload + eviction (no rendering change yet: pages load
   but everything still draws from the monolithic buffers behind a cvar) — 3-4 wk.
3. Paged GPU buffers + residency-aware cut (`m_clusterResident`/`m_childrenResident`,
   fail-safe-coarse) + kill the monolithic buffers for v4 packs — 3-4 wk.
4. Soak: budget sweeps, thrash hysteresis tuning, teleport worst-case (always-resident
   set must carry the frame alone) — 1-2 wk.

Total ~10-14 wk. **Exit**: a scene whose full DAG exceeds the pool renders correctly
at every budget setting — never a hole, never a crack, only temporarily coarser
geometry — with steady-state IO near zero for a static camera.


---

## Phase 4 — runtime soak protocol (run once built; the CPU half is unit-tested)

Setup: a `generate_pages` (v4) pack, `r_meshletsHwMeshShader` + `r_meshletsMsCullAS`
+ `r_meshletsDagLod` + `r_meshletsStreaming` on, Meshlets Debug ImGui open.

1. **Budget sweep**: step `r_meshletsStreamingPoolMB` 512 -> 256 -> 64 -> 16 -> 4 -> 512
   live. Each step: pool rebuilds, everything goes coarse for a few frames, then
   reloads. PASS = geometry only ever coarsens (never holes/cracks), the STARVED
   readout is zero at big budgets and stable-nonzero at small ones, and the resident
   count tracks the slot capacity.
2. **Thrash tune**: orbit the camera at a distance where detail sits near the cut
   boundary. PASS = churn counter stays flat at hysteresis 1.5; drop the slider to
   1.0 and churn should climb (proves the readout works), then pick the smallest
   value that stays flat for the content.
3. **Teleport worst case**: bind two far-apart viewpoints and cut between them.
   PASS = the arrival frame renders complete coarse geometry instantly (interior
   pages are pinned — never a hole), detail fills in over
   ceil(wantedPages / r_meshletsStreamingMaxLoadsPerFrame) frames; sweep the
   throttle to trade refinement latency vs upload spikes.
4. **Static-camera steady state**: PASS = loads AND evicts read +0/-0 for minutes
   (the design's "steady-state IO near zero").
5. **PIX spot-checks**: MeshletsPageUpload dispatches appear only on load events;
   the page pool binds SRV in the mesh stages; no UAV/SRV hazards flagged on the
   pool or the visibility ledgers.
