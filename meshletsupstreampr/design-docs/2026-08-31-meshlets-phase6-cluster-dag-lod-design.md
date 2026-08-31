# Meshlets Phase 6 — Cluster-DAG LOD ("Nanite endgame") design

**Status:** DESIGN — nothing implemented. Successor to the retired per-instance discrete-LOD
ladder for the hardware mesh-shader path.
**Scope anchor:** the Phase 5 mesh-shader path is feature-complete (AS cluster cull with
frustum/cone/HiZ, per-triangle cull incl. big-triangle HiZ, all four geometry passes on
DispatchMesh, freeze-cull debug). Phase 6 replaces *what geometry gets dispatched* — per-cluster
continuous LOD from a simplification DAG — without touching how it is culled or shaded.

**Explicit non-goals (v1):**
- No visibility-buffer / deferred-material rewrite (Timberdoodle-parity verdict: don't adopt).
- No streaming/paging — all DAG levels resident. `SectionKind::PageTable/PageData` stay reserved.
- No two-pass occlusion — prev-frame HiZ stays as-is (separate Phase 7 work).
- No shadow-side DAG cut — shadows keep drawing full-detail leaves (see §6).

---

## 1. Why a DAG, one paragraph

The per-instance LOD ladder selects one detail level for a whole mesh, so a statue filling the
screen pays full detail on its far side and a horizon statue still pays whole-cluster granularity.
A cluster DAG lets every *cluster* pick its own detail: the builder repeatedly groups adjacent
clusters, simplifies each group to ~half its triangles, and re-clusters the result into "parent"
clusters, recording an object-space error per group. At runtime each cluster independently decides
"is MY error small enough on screen, and my parent's NOT small enough?" — the set of clusters
passing that test forms a complete, crack-free cut through the DAG with **no traversal, no CPU
work, and no inter-cluster communication**: a flat per-cluster test that drops straight into the
existing amplification-shader lane loop.

## 2. Offline build (MeshletPackBuilderCore)

Vendored meshoptimizer **v1.2** supplies every primitive; no new dependency.

Per mesh, starting from the existing LOD0 meshlets (`meshopt_buildMeshlets`, 128v/256t):

```
level = leaves
while (clusterCount(level) > 1 && level < kMaxDagLevels)
    groups = meshopt_partitionClusters(level, targetGroupSize = 8)   // adjacency by shared verts
    for each group:
        mergedIndices = concat(group's cluster index lists)
        simplified = meshopt_simplifyWithAttributes(
            mergedIndices, target = 50%,
            options = LOCK_BORDER | SPARSE | ERROR_ABSOLUTE,
            attributes = normal(+uv) with modest weights)
        if (no meaningful reduction) mark group's clusters as DAG roots; continue
        groupErrorObj = max(simplifyResultError,                      // object-space units
                            max(child clusters' groupErrorObj))       // MONOTONIC — see §4
        groupSphere   = bounding sphere of the group's source clusters
                        (also maxed to enclose children's group spheres)
        parents = meshopt_buildMeshlets(simplified)                   // ~4 clusters out
        for each parent: selfError/selfSphere = this group's error/sphere
        for each child in group: parentError/parentSphere = this group's error/sphere
    level = parents
```

Rules that make it crack-free:
- **Boundary lock:** `LOCK_BORDER` pins every vertex on a group boundary, so two adjacent groups
  simplified independently still meet exactly. (This is THE crack mechanism — Nanite's too.)
- **Shared group error:** every parent produced from a group carries the *same* selfError/sphere,
  and every child of that group carries the *same* parentError/sphere. The runtime cut test then
  flips all clusters on either side of a group boundary together, or not at all.
- **Leaves:** selfError = 0 (always eligible). **Roots:** parentError = +inf (never rejected for
  "parent is good enough").

Expected totals: geometric series ⇒ ~2× LOD0 triangles / ~2× cluster count across the whole DAG —
comparable to today's 4-LOD discrete chain (~1.9×), so memory is a wash and the discrete chain is
**dropped for DAG-enabled packs** (one LOD entry, see §3).

Sidecar opt-in: `"generateClusterDag": true` in `.meshletpack` (default false). Builder version
bump gates re-bake; non-DAG packs are byte-identical to today's.

## 3. Pack format (MeshletPackFormat.h)

- `PackVersion` 2 → 3 (v2 packs load unchanged — DagNodes absent ⇒ runtime uses current paths).
- Fill reserved `SectionKind::DagNodes = 7`: one 48-byte record per cluster, pack-global order,
  parallel to `ClusterDescriptors` exactly like `ConeBounds`:

```cpp
//! Kind 7 (DagNodes): 48 bytes = 3x float4 -> clean StructuredBuffer.
struct DagNodeRecord
{
    float m_selfSphere[4];    //!< xyz = center, w = radius (object space, group-shared)
    float m_parentSphere[4];
    float m_selfError;        //!< object-space error of the group this cluster BELONGS to (0 for leaves)
    float m_parentError;      //!< error of the group this cluster was simplified INTO (+inf for roots)
    float m_pad[2];
};
```

- Cluster ordering per mesh: **leaves first, contiguous**, interior levels appended after.
  `MeshDescriptorLodEntry`: `m_clusterCount` keeps meaning the LEAF count (uncull paths keep
  today's behavior untouched, §6); repurpose `m_reserved0` → `m_dagClusterCount` (total incl.
  interior; 0 = no DAG). DAG packs write `m_lodCount = 1`.
- `SectionKind::LodError` unused by DAG packs (superseded by per-node errors).

## 4. Runtime cut test (the whole algorithm)

Projected screen-space error of a group, in pixels:

```
errPx(sphereW, errObj) = errObj * maxAxisScale * (viewToClip[1][1] * viewportHeight * 0.5)
                         / max(distance(cameraW, centerW) - radiusW, kNearClamp)
```

- `viewToClip[1][1]` (= cot(FovY/2)) NOT worldToClip[1][1] — the same rotation-independence
  root-cause fix the coverage ladder needed (2026-06-20).
- Camera inside the sphere (`distance <= radius`) ⇒ errPx = +inf (forces refinement — draw
  children; conservative and stable).

Per-cluster decision (`MeshletsClusterCullMath.azsli`, new pure function):

```
DagCutAccepts(node) = errPx(node.selfSphere,   node.selfError)   <= tau
                   && errPx(node.parentSphere, node.parentError) >  tau
```

`tau` = `r_meshletsDagErrorPx` cvar, default 1.0 px. Monotonicity (§2's max()) guarantees exactly
one level per DAG path satisfies this ⇒ complete cut, no gaps, no overlaps. Group-shared
sphere/error guarantees neighbors agree ⇒ no cracks.

Evaluation order in `MeshletClusterVisible` / compute `ClusterVisible`:
**cut test first** (rejects ~half of all DAG clusters, cheapest), then frustum, HiZ, cone —
unchanged. Both cull paths get it from the shared azsli so the compute path stays the
correctness oracle for the AS path.

New SRG inputs (instance + cull SRGs): `StructuredBuffer<DagNodeRecord> m_dagNodes` (object SRG),
`float2 m_viewportSize`, `float m_dagErrorPx`, `uint m_doDagCut`. `m_viewportSize` finally pays
down the standing `ponytail:` debt — the per-triangle micro/big gates switch from the 1080p
constant to the real target size in the same change.

## 5. Runtime integration

- **Gate:** `r_meshletsDagLod` (default OFF) AND pack has DagNodes AND `r_meshletsMsCullAS`
  active. Everything else falls back to exactly today's behavior.
- AS-culled geometry views dispatch `ceil(m_dagClusterCount / 128)` AS groups; object SRG's
  `m_clusterCount` becomes `m_dagClusterCount` for the AS-culled PSOs only (the uncull PSOs keep
  the leaf count — separate constant `m_leafClusterCount` if azslc layout sharing demands it).
- Per-instance screen-coverage LOD selection: **bypassed** for DAG packs (`LodIndex` pinned 0);
  the DAG *is* the LOD system. The hysteresis/group-churn machinery goes dormant, not deleted.
- Buffers: ClusterDescBuffer/BoundsBuffer/triangle/indirection buffers sized to the DAG cluster
  range (they already upload per-mesh slices; the slice just grows). DagNodes uploads like
  ConeBounds (`EnsureCullGpuBuffers`).

## 6. Uncull paths and shadows (the overlap trap)

Any path that draws "all clusters" would draw leaves AND interiors — double geometry z-fighting
at half detail. Guard: **every non-AS path keeps drawing the contiguous LEAF range only** (their
cluster count stays `m_clusterCount` = leaf count, their DispatchMesh args unchanged). That means:
default (no-AS) mesh path, mesh/vertex-pull shadow, non-AS depth/motion, vertex-pull fallback,
and the compute-cull path when DAG is off. Consequences accepted for v1: shadows and the no-AS
path render full detail (today's cost, no regression); shadow-side DAG cuts are future work.

## 7. Debug / verification plan

- `m_meshletDebugColor` gains a mode: color by DAG level (hash level, not cluster id) — the cut
  boundary is directly visible; screenshot sweep of `r_meshletsDagErrorPx` 0.25→8 must show
  detail receding with distance and ZERO cracks at any tau.
- `r_meshletsDagForceLevel` (-1 auto; N pins the cut to level N) — the A/B tool, mirrors the
  force-LOD slider.
- ImGui stats: drawn clusters per level, cut histogram; freeze-cull composes (frozen camera
  freezes the cut too — same constants).
- Correctness oracle: compute path with the same cut ⇒ identical visible-cluster sets between
  AS and compute paths on a frozen camera.
- Unit test (builder): for a synthetic mesh, assert per-path monotonic errors, group-shared
  parent records, leaf-first ordering, and that every DAG path has exactly one cut level for a
  sampled tau range.

## 8. Order of work + effort (single dev)

1. **Builder DAG** (partition/simplify/re-cluster loop, monotonic errors, leaf-first layout,
   sidecar flag, builder-version bump) — 2–3 wk. Risk: simplification stalls on ugly topology →
   handled by early root-marking (correct, just less reduction).
2. **Format + loader** (DagNodeRecord, v3 read/write, per-mesh DAG slices, GPU upload) — 1 wk.
3. **Runtime cut** (shared azsli function, SRG plumbing incl. m_viewportSize, AS + compute
   wiring, dispatch-count switch, LOD-ladder bypass) — 1–1.5 wk.
4. **Debug + crack QA** (level colors, force-level, stats, tau sweep, oracle diff) — 1–2 wk.

Total ~5.5–7.5 wk. Ship gate: tau sweep crack-free on the statue scene + measured
triangle/VS-invocation reduction vs the discrete-LOD baseline at equal visual quality.

## 9. Risks

| Risk | Mitigation |
|---|---|
| Cracks from a monotonicity bug | max()-propagation + group-shared records + tau-sweep QA + builder unit test |
| Simplifier can't reduce (locked borders dominate small groups) | mark roots early; DAG just ends shallower |
| UV/normal seams tear under simplification | simplifyWithAttributes weights; seam verts are implicitly locked by attribute split |
| AS cost grows (2× clusters tested) | cut test is ~20 ALU before any memory-heavy test; still one lane per cluster |
| Uncull/AS layout divergence (azslc stripping) around the new object-SRG fields | same discipline as m_clusterBounds: bind only into the AS-culled object SRG build |
