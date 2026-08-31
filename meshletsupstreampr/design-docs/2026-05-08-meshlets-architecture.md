# Meshlets Gem — north-star architecture

**Date:** 2026-05-08
**Status:** Design — approved scope, awaiting implementation plan for SP1
**Scope:** Architectural destination for the `Gems/Meshlets` gem. Reference for sub-project specs that follow.

---

## 1. Why this document exists

The Meshlets gem in `Gems/Meshlets` is broken in shape, not just in bugs. There is no offline asset pipeline: meshlet generation runs on the CPU at runtime every time an entity flips on `MeshComponent`'s "Use Virtual Geometry (Meshlets)" toggle. There is no artist-facing asset type, no `.azmeshletpack` in the Asset Browser, no AssetBuilder, no incremental rebuild. The render path is a custom debug pass painting UV-as-color, and the recent commit history is a chain of firefighting (`DXGI_ERROR_DEVICE_HUNG` bisects, dangling `DrawPacket` pointer, "force-bind shared buffer" hack, fresh empty `InputStreamLayout`). That entire chain is now resolved on the rendering side, but the gem still has no path for artists to author or version meshlet content.

This document defines the architectural destination: where the gem is going across all sub-projects, so that SP1's asset format is forward-compatible with SP2-SP6 and we don't re-bake every pack on the project each sprint. It is **not** an implementation plan for any single sub-project — those have their own specs.

The deferred decisions ("we'll think about it later") that this doc locks down:
- pack granularity (per-source-model);
- relationship to `.azmodel` (companion-with-rebake);
- ingestion paths (scenemanifest rule + sidecar JSON);
- runtime resolution (product-dependency-driven, no entity-side asset reference);
- format versioning (header + section ToC, additive sections, version field);
- mesh-shader posture (back-burner, format-reserved).

The reference for the meshlet-rendering target shape is moonlovelj/Nyx (offline meshletize → iterative simplify for LOD chain → 8-child DAG → page-packed → LZ4 → runtime two-phase HZB cull → vis-buffer → GBuffer resolve → existing PBR). We adopt the spirit of that pipeline. We **do not** adopt mesh shaders in this scope; they are deferred indefinitely.

## 2. Scope across sub-projects

The full destination is delivered across six sub-projects. Each gets its own spec → plan → implementation cycle. SP2-SP6 may reorder or merge as we learn, but their format implications are reserved now.

| Sub-project | What lands |
|---|---|
| **SP1** | Asset pipeline + format v1 + minimal runtime parity. `.azmeshletpack` source/product, two ingestion paths, AssetBuilder, runtime loads pack, debug shader stays. Cleanup of bisect debt. |
| **SP2** | GPU culling: per-meshlet frustum + cone (cones baked into pack). |
| **SP3** | Hierarchical LOD + DAG: iterative simplification, 8-child hierarchy, parent/child error metric, per-cluster LOD selection. |
| **SP4** | Two-phase HZB occlusion (last-frame seed + current-frame correction). |
| **SP5** | Vis-buffer pipeline + GBuffer resolve. PBR materials wired. Replaces debug shader. |
| **SP6** | Streaming + 256KB page residency: page partition, LZ4, async page upload, GPU residency tables. |

Mesh shaders are explicitly out of scope across all six. The format and runtime topology are designed to absorb them as an additive change (cluster descriptors are already mesh-shader-shaped, cone bounds and LOD parent/child are exactly the mesh-shader payload, vis-buffer is mesh-shader-friendly).

## 3. Target runtime data flow

```
[Source: .fbx + scenemanifest MeshletPackRule]  OR  [.meshletpack JSON + .azmodel ref]
                                  │
                                  ▼
                 ┌────────────────────────────────────┐
                 │   MeshletPackBuilder (SceneAPI)    │
                 │   meshletize → iterative simplify  │
                 │   → 8-child DAG → cone bounds      │
                 │   → vertex re-pack → cluster→mat   │
                 │   → page partition (SP6) → LZ4     │
                 └────────────────────────────────────┘
                                  │
                                  ▼
                         .azmeshletpack
                                  │
                                  ▼
   ┌──────────────────────────────────────────────────────────┐
   │  Meshlets runtime — per frame:                           │
   │   1. instance cull        (CPU bucket)                   │
   │   2. DAG cull (compute)   frustum + cone +               │
   │                            HZB[N-1] + LOD select         │
   │                            → visible cluster list        │
   │   3. index-buffer build   compute, per visible cluster   │
   │   4. depth + vis-buffer   raster                         │
   │   5. HZB build            compute pyramid                │
   │   6. second-phase cull    correct with HZB[N]            │
   │   7. delta vis-buffer     raster                         │
   │   8. vis-buffer resolve   compute → GBuffer              │
   │   9. existing PBR/lighting (unchanged)                   │
   └──────────────────────────────────────────────────────────┘
```

Steps 1-3 ship across SP1-SP4. Steps 4-8 ship in SP5. Step 9 (existing PBR) is unchanged; meshlet output flows into the existing GBuffer at SP5's resolve step.

## 4. Sub-project ownership matrix

| Concern | SP1 | SP2 | SP3 | SP4 | SP5 | SP6 |
|---|---|---|---|---|---|---|
| `.azmeshletpack` format | **define v1** | reserve cone-bounds field | reserve DAG/LOD-error fields | reserve HZB-cull metadata | reserve cluster→material map | reserve page-table |
| Asset builder | one cluster set per mesh | + cones | + DAG + iterative simplify | — | + cluster→material | + page partition + LZ4 |
| Runtime asset load | one-shot upload | — | — | — | — | demand-streamed pages |
| Per-frame culling | none | frustum + cone | DAG hierarchy + LOD | two-phase HZB | — | — |
| Index buffer build | per-mesh dispatch | per-visible-cluster | + per-LOD | — | — | — |
| Raster output | debug shader | debug | debug | debug | **vis-buffer + GBuffer resolve** | — |
| PBR materials | n/a | n/a | n/a | n/a | **wired** | — |

## 5. Cross-cutting design decisions

These span sub-projects. Locking them down now is what makes SP1's format choices durable.

### 5.1 Companion-with-rebake (data ownership)

`.azmodel` and `.azmeshletpack` coexist for the same source. The pack owns its own vertex streams (re-laid-out for cluster-friendly access via `meshopt_optimizeVertexFetch`); the `.azmodel` keeps the standard-renderer-friendly layout. They cross-reference by AssetId, but neither reads the other's vertex data at runtime.

Rationale: companion-only forces either a constrained model importer (intrusive) or a runtime re-pack (wasteful). Pure replacement breaks the "drop the same FBX, both renderers work" UX. Companion-with-rebake gives the meshlet runtime its preferred vertex layout (which SP6's quantization needs) while keeping `.azmodel` available for the standard mesh path.

Disk-cost: bounded — only models flagged with the rule (or sidecar) get a pack. Not every model.

### 5.2 Per-source-model packs (granularity)

One `.azmeshletpack` product per source asset. SP6 may add a separate page-atlas aggregator asset later, but the per-model pack remains the primary unit.

Rationale: matches AssetProcessor's per-source incremental-rebuild model — change one `.fbx`, re-bake one pack. Matches the existing artist mental model identical to today's `.azmodel`. Cross-mesh hierarchies (Nyx-style page packing) are a streaming-density optimization, not a culling correctness one — they belong in SP6 as an aggregator, not as a refactor of the per-model pack.

### 5.3 Two ingestion paths, one product

The builder accepts two source forms:

- **SceneAPI scenemanifest rule** on the FBX: artist right-clicks `Cathedral.fbx` in Asset Browser → "Add Modifier" → "Meshlet Pack Rule." The `.assetinfo` gets a `MeshletPackRule` block (max_vertices, max_triangles, cone_weight). Same pattern as `LodRule`, `TangentRule`, `MaterialRule`. Asset Editor provides the GUI for free.
- **JSON sidecar source file** `*.meshletpack` next to the source. Artist authors a JSON descriptor referencing an existing `.azmodel` AssetId. Builder watches `*.meshletpack` files. Targets non-FBX sources (procedural meshes, code-generated geometry).

Both produce byte-identical product output. SP1 ships both — neither is a fast follow.

### 5.4 Product-dependency-driven runtime resolution

No new entity-side asset reference. `MeshComponent`'s "Use Virtual Geometry (Meshlets)" toggle stays put. AssetProcessor records the pack as a product dependency of the source FBX (or as the product of the `.meshletpack` JSON, which carries an `.azmodel` AssetId). The Meshlets feature processor's `AcquireInstance(modelAsset)` looks up the pack via product-dependency table.

Missing pack surfaces in `MeshComponent`'s inspector as a status field (`"No meshlet pack for this model — flag the source FBX with a Meshlet Pack rule"`). No silent fallback to the standard renderer — that hides bake regressions.

### 5.5 Format versioning

```
File header (16 bytes):
  u32 magic       = 'MTLP' (LE: 0x504C544D)
  u32 version     = 1
  u16 toc_count
  u16 flags       (bit 0: compressed body — SP6)
  u32 reserved

Section ToC entries (32 bytes each):
  u32 kind
  u32 flags
  u64 offset_from_file_start
  u64 size_bytes
  u64 reserved
```

SP1 = v1 with section kinds 0-5 defined and 6-12 reserved. SP2-SP6 add new kinds; existing kinds' record formats are append-only, never modified in place. Forward compat: unknown sections ignored. Backward compat: a v2+ runtime checks for required SP1 sections by kind, errors loudly if absent.

Hard rebuild (version bump) only when an existing section's record format changes — additions are free.

### 5.6 No new shader stages required (SP1-SP5)

Compute + vertex/pixel only. Vis-buffer resolve (SP5) is fullscreen compute. SP6's streaming uses compute for decompression. Mesh shaders are deferred; format reservation makes them additive when added. No DXR, no SM6.6 features, no platform restrictions beyond what Atom already requires.

### 5.7 Cluster constants are baked into the pack

`max_vertices_per_cluster`, `max_triangles_per_cluster`, `cone_weight` go in the pack header (kind 0). Today's runtime hard-codes 64/64; with the constants in the pack, an artist can tune per-asset (a tessellation-heavy hero asset can use 128/124, a low-poly prop can use 32/32) without touching shader code. SP1 still defaults to 64/64 — the field exists so we don't have to bump the format when SP3 needs different cluster sizes for higher LOD levels.

### 5.8 Existing `MeshletsFeatureProcessorInterface` API stays

`AcquireInstance(Data::Asset<RPI::ModelAsset>)`, `ReleaseInstance`, `SetInstanceTransform` keep their signatures. Internally, AcquireInstance now resolves to a pack via product dependency and routes through the new pack-driven render-object path. Callers (the MeshComponent toggle) are unchanged. This preserves the bridge to the rest of the engine and makes SP1 a drop-in replacement for the broken internals.

### 5.9 Dead code is deleted in SP1

`MeshletsAssets.{h,cpp}` is marked "reference/demo" by its own header comment, isn't on the runtime path, and its presence has been a source of confusion. Removing it is the cleanest signal that the runtime path is the AcquireInstance-via-pack path, not the CPU-build-from-ModelAsset path.

## 6. Format section kinds (full table)

SP1 defines kinds 0-5. SP2-SP6 reserve kinds 6-12. Each row is a section; the data layout per record is documented per-SP in that SP's spec.

| kind | name | defined in | record content (summary) |
|---|---|---|---|
| 0 | `PackHeader` | SP1 | source `ModelAsset` AssetId, mesh count, cluster constants, global AABB |
| 1 | `MeshDescriptors` | SP1 | per-mesh: name, LOD count, per-LOD ranges (cluster_first/count, vertex_first/count, material_id reserved) |
| 2 | `ClusterDescriptors` | SP1 | per-cluster: vertex_offset, triangle_offset, vertex_count, triangle_count |
| 3 | `TriangleIndices` | SP1 | encoded triangles (3×8-bit packed in u32) |
| 4 | `VertexIndirection` | SP1 | cluster-local → mesh-global vertex index |
| 5 | `VertexStreams` | SP1 | sub-headered: POSITION/NORMAL/TANGENT/BITANGENT/UV0, reordered for vertex-fetch locality |
| 6 | `ConeBounds` | SP2 | per-cluster: cone apex (3×f32), axis (3×f32), cutoff (f32) |
| 7 | `DagNodes` | SP3 | hierarchy nodes: bounds, child count, child offsets, simplification error |
| 8 | `LodError` | SP3 | per-cluster parent/child error for LOD selection |
| 9 | `HzbCullMetadata` | SP4 | per-cluster bounds + last-frame visibility hint |
| 10 | `ClusterMaterialMap` | SP5 | per-cluster `MaterialAsset` reference index |
| 11 | `PageTable` | SP6 | page descriptors: offset, compressed size, decompressed size, residency hint |
| 12 | `PageData` | SP6 | LZ4-compressed page bodies (when header.flags bit 0 is set) |

## 7. Runtime module layout

The Meshlets gem grows three new targets in SP1; SP2-SP6 add no new targets, only extend existing ones.

| Target | Tier | Purpose |
|---|---|---|
| `Meshlets.Reflect.Static` | RPI.Reflect | `MeshletPackAsset`, asset handler, section-kind enum, byte-layout structs. Linkable from runtime *and* builder. |
| `Meshlets.Builders.Static` | builder-only | `MeshletPackBuilderCore` (the meshletizer pipeline lifted out of the runtime), `MeshletPackRule` reflect class, JSON-source descriptor reflect class. |
| `Meshlets.Builders` | builder module | SceneAPI behavior context registration + JSON-source `AssetBuilderRegistrationBus` registration. Deployed to AssetProcessor's builders directory. |
| `Meshlets` (existing) | runtime | Loses CPU meshletization; gains pack loader; per-frame topology unchanged through SP1. |
| `Meshlets.Editor` (existing) | editor | Gains scenemanifest rule UI + asset type registration in SP1; gains MeshComponent inspector status read in SP1. |

## 8. What's not in this document

- Implementation byte-layouts of section records (in each sub-project's spec).
- Test plans (in each sub-project's spec).
- Risk registers for individual sub-projects (in each sub-project's spec).
- Mesh-shader implementation path (deferred indefinitely; format reservation only).
- The page-atlas aggregator asset (decision deferred until SP6 measures whether per-model packs hurt streaming density).

## 9. References

- Existing gem: `Gems/Meshlets/`
- Reference repository: [moonlovelj/Nyx](https://github.com/moonlovelj/Nyx) — `.mini` cache format, two-phase HZB, vis-buffer pipeline.
- Builder pattern reference: `Gems/AtomTressFX/Code/Builders/HairAssetBuilder.cpp`, `Gems/SceneProcessing/Code/Source/SceneBuilder/`, `Gems/Atom/RPI/Code/Source/RPI.Builders/Model/ModelAssetBuilderComponent.cpp`.
- meshoptimizer (already in `3rdParty::meshoptimizer`).
