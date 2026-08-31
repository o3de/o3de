# Meshlets SP1 — asset pipeline + format v1 + minimal runtime parity

**Date:** 2026-05-08
**Status:** Design — approved for plan-writing
**Parent:** [`2026-05-08-meshlets-architecture.md`](2026-05-08-meshlets-architecture.md) (north-star)
**Sub-project:** SP1 of 6 (see north-star §2)

---

## 1. Goal

Replace the runtime CPU meshletization in `Gems/Meshlets` with an offline asset pipeline that produces an artist-authorable, AssetProcessor-cached `.azmeshletpack` product. The runtime loads the baked pack instead of rebuilding meshlet data per acquire, and the entire scaffolding for SP2-SP6 (cones, DAG, LOD, HZB, vis-buffer, streaming) is forward-compatible at the format level so subsequent sub-projects don't trigger a project-wide re-bake.

In one sentence: **artists can drop a `.fbx`, add a Meshlet Pack rule, and a `.azmeshletpack` appears in the Asset Browser; the entity's existing "Use Virtual Geometry" toggle picks it up automatically.**

## 2. Non-goals (deferred to later sub-projects)

| Out | Why deferred | Reserved at format level? |
|---|---|---|
| Cone-based GPU culling | SP2 | Yes — section kind 6 reserved |
| Iterative LOD simplification | SP3 | Yes — kinds 7, 8 reserved |
| 8-child DAG hierarchy | SP3 | Yes — kind 7 reserved |
| Two-phase HZB occlusion | SP4 | Yes — kind 9 reserved |
| Vis-buffer / PBR materials | SP5 | Yes — kind 10 reserved (per-cluster MaterialAsset map) |
| Page partition + LZ4 + streaming | SP6 | Yes — header flags bit 0; kinds 11, 12 reserved |
| Mesh shaders | back-burner indefinitely | Format is mesh-shader-shaped already (cluster descriptors are meshlet records); no extra reservation needed |

The render output **stays on the existing custom debug shader** (`MeshletsDebugRenderShader`) through SP1. PBR is SP5's deliverable.

## 3. Scope checklist

SP1 ships:

1. `MeshletPackAsset` reflect class + asset handler, in a new `Meshlets.Reflect.Static` target.
2. `MeshletPackBuilderCore`: meshletizer pipeline lifted from `MeshletsRenderObject::CreateMeshlets*` into a shared library callable from both the builder and (for procedural-mesh callers later) the runtime. SP1 only invokes it offline.
3. **Two builders**:
   - SceneAPI export module triggered by `MeshletPackRule` in `.fbx` scenemanifest.
   - JSON-source builder triggered by `*.meshletpack` files referencing an existing `.azmodel` AssetId.
4. Runtime path: `MeshletsFeatureProcessor::AcquireInstance(modelAsset)` resolves a sibling `.azmeshletpack` via product-dependency, loads it, and builds `MeshRenderData` from pack section data instead of from `RPI::ModelAsset`.
5. **Cleanup of debug debt** (full list in §8).
6. `MeshComponent` inspector status field showing pack-resolution state.
7. **Tests**: builder fixtures, runtime load test, integration test.

SP1 does not ship:

- Mesh-shader path (back-burner per north-star §2).
- Anything in §2's "out" column.
- Asset Browser thumbnail rendering (defer to a polish pass).
- Right-click bake actions in Asset Browser (defer).

## 4. Format v1: byte layout

### 4.1 File header (16 bytes, fixed)

| Offset | Size | Field | Value/Notes |
|---|---|---|---|
| 0 | 4 | `magic` | `'MTLP'` (LE: `0x504C544D`) |
| 4 | 4 | `version` | `1` (u32) |
| 8 | 2 | `toc_count` | u16, number of section ToC entries that follow |
| 10 | 2 | `flags` | u16; bit 0 = compressed body (SP6, must be 0 in SP1) |
| 12 | 4 | `reserved` | must be 0 |

### 4.2 Section ToC (32 bytes per entry, immediately after header)

| Offset | Size | Field |
|---|---|---|
| 0 | 4 | `kind` (u32; values from the section-kind enum) |
| 4 | 4 | `flags` (u32; reserved, must be 0 in SP1) |
| 8 | 8 | `offset` (u64; from start of file) |
| 16 | 8 | `size` (u64; bytes) |
| 24 | 8 | `reserved` (u64; must be 0) |

A v1 pack has six ToC entries (one per defined SP1 section, kinds 0-5). Sections appear in ToC-order in the file, 16-byte aligned.

### 4.3 SP1 sections (defined)

#### Kind 0 — `PackHeader` (1 record, 60 bytes)

The source `AssetId` is stored as raw GUID bytes + sub-id rather than embedding `AZ::Data::AssetId` directly. Reason: `AssetId` is a 32-byte type in memory (16-byte `alignas(16)` Uuid + u32 sub-id + 12 bytes of trailing pad), and `#pragma pack(1)` cannot strip the embedded padding from `AssetId` itself. Storing the components inline gives a stable 60-byte on-disk record that's independent of any future O3DE-internal change to `AssetId`'s memory layout.

| Offset | Size | Field | Notes |
|---|---|---|---|
| 0 | 16 | `source_model_guid` | u8[16] — Uuid raw bytes |
| 16 | 4 | `source_model_sub_id` | u32 |
| 20 | 4 | `reserved0` | u32 (must be 0; aligns subsequent fields and reserves space if `AssetId` grows) |
| 24 | 4 | `mesh_count` | u32 |
| 28 | 2 | `max_vertices_per_cluster` | u16; SP1 default 64 |
| 30 | 2 | `max_triangles_per_cluster` | u16; SP1 default 64 |
| 32 | 4 | `cone_weight` | f32; SP1 default 0.5 (used at bake time even though SP1 doesn't read cones) |
| 36 | 24 | `global_aabb` | min (3×f32) + max (3×f32) |

#### Kind 1 — `MeshDescriptors` (mesh_count records)

Per-mesh, variable-stride: fixed prefix (40 B) + per-LOD entries (32 B each). LOD count is in the prefix.

Prefix:

| Offset | Size | Field |
|---|---|---|
| 0 | 4 | `name_offset` (u32; offset into the mesh-name string blob at end of section) |
| 4 | 4 | `name_size` (u32 bytes) |
| 8 | 2 | `lod_count` (u16; SP1 always 1) |
| 10 | 2 | `reserved` |
| 12 | 4 | `reserved` |
| 16 | 24 | `bounds_aabb` |

Per-LOD entry:

| Offset | Size | Field |
|---|---|---|
| 0 | 4 | `cluster_first` (u32; index into `ClusterDescriptors`) |
| 4 | 4 | `cluster_count` (u32) |
| 8 | 4 | `vertex_first` (u32; index into `VertexStreams` per-stream) |
| 12 | 4 | `vertex_count` (u32) |
| 16 | 4 | `material_id` (u32; **reserved for SP5** — must be `0xFFFFFFFF` in SP1) |
| 20 | 4 | `reserved` |
| 24 | 8 | `reserved` |

After all mesh descriptors: a name blob (concatenated UTF-8 strings, no terminators; ranges given by each descriptor's `name_offset` + `name_size`).

#### Kind 2 — `ClusterDescriptors` (sum of all `cluster_count` records)

Records are 16 bytes, in the same shape as `meshopt_Meshlet` so the runtime can `memcpy` directly into a struct compatible with the existing GPU-side `MeshletDescriptor`.

| Offset | Size | Field |
|---|---|---|
| 0 | 4 | `vertex_offset` (u32; into `VertexIndirection`) |
| 4 | 4 | `triangle_offset` (u32; into `TriangleIndices`) |
| 8 | 4 | `vertex_count` (u32) |
| 12 | 4 | `triangle_count` (u32) |

#### Kind 3 — `TriangleIndices` (sum of all `triangle_count` records)

Each record is one u32. Three 8-bit local indices packed into bits 0-7, 8-15, 16-23 (top byte zero), exactly the encoding the existing compute shader reads. This is the same encoding `MeshletsData::EncodeTrianglesData` produces today; SP1 lifts it from the runtime to the builder.

#### Kind 4 — `VertexIndirection`

Each record is one u32 — a cluster-local-vertex-index → mesh-global-vertex-index map. Element count is the sum of `vertex_count` across all clusters.

#### Kind 5 — `VertexStreams` (sub-headered)

Sub-header (128 bytes for SP1: 8-byte header + 5 descriptors × 24 B):

| Offset | Size | Field |
|---|---|---|
| 0 | 4 | `total_vertex_count` (u32) |
| 4 | 4 | `stream_count` (u32; SP1 = 5) |
| 8 | 24 × `stream_count` | `stream_descriptors[stream_count]` — for each: `format` (u32, RHI::Format), `byte_offset_in_section` (u32), `byte_stride` (u32), `semantic_kind` (u32, 0=POS/1=NORM/2=TAN/3=BITAN/4=UV0), reserved (u32 + u32) |

(Each descriptor is 24 bytes — `RHI::Format` + 3×u32 of payload + 2×u32 reserved = 24 B, sized for future extensibility without changing record stride.)

Stream data follows the sub-header in the order described by stream_descriptors. SP1 emits five streams: POSITION (`R32G32B32_FLOAT`), NORMAL (`R32G32B32_FLOAT`), TANGENT (`R32G32B32A32_FLOAT`), BITANGENT (`R32G32B32_FLOAT`), UV0 (`R32G32_FLOAT`). Vertex order is whatever `meshopt_optimizeVertexFetch` produced; the indirection table (kind 4) maps cluster-local indices into this re-ordered stream space.

### 4.4 Reserved kinds (declared, not emitted in SP1)

| Kind | Name | Reserved for |
|---|---|---|
| 6 | `ConeBounds` | SP2 |
| 7 | `DagNodes` | SP3 |
| 8 | `LodError` | SP3 |
| 9 | `HzbCullMetadata` | SP4 |
| 10 | `ClusterMaterialMap` | SP5 |
| 11 | `PageTable` | SP6 |
| 12 | `PageData` | SP6 |

The section-kind enum is shared between `Meshlets.Reflect.Static` (runtime) and `Meshlets.Builders.Static` (builder) so neither side can drift.

## 5. Builder

### 5.1 Common pipeline (`MeshletPackBuilderCore`)

Lives in `Meshlets.Builders.Static`. Both ingestion paths normalize to a `SourceMeshSet` (vertex streams + index buffer per mesh) and call this:

```
for each mesh in source_mesh_set:
    1. collect vertex streams: POSITION, NORMAL, TANGENT, BITANGENT, UV0
    2. meshopt_optimizeVertexFetch
       → reorder vertices for cluster locality, returns remap table
       → re-index triangles via remap
    3. meshopt_buildMeshlets(
         max_vertices  = rule.max_vertices  (default 64),
         max_triangles = rule.max_triangles (default 64),
         cone_weight   = rule.cone_weight   (default 0.5))
    4. encode triangle indices: pack 3×8-bit local indices into u32 (top byte 0)
    5. trim arrays to actual cluster count's bounds
    6. append to pack-level vectors:
         - cluster descriptors  (kind 2)
         - encoded triangles    (kind 3)
         - vertex indirection   (kind 4)
         - reordered vertex streams (kind 5)
         - mesh descriptor with single LOD entry (kind 1)
```

After all meshes processed:

```
emit:
    file header (magic, version=1, toc_count=6, flags=0)
    section ToC (6 entries, 16-byte aligned offsets)
    section data:
        kind 0  PackHeader     (1 record)
        kind 1  MeshDescriptors (mesh_count records + name blob)
        kind 2  ClusterDescriptors (Σ cluster_count records)
        kind 3  TriangleIndices (Σ triangle_count records)
        kind 4  VertexIndirection (Σ cluster vertex_count records)
        kind 5  VertexStreams (sub-headered, total_vertex_count vertices × 5 streams)
register product dependency: pack → `.azmodel` (both ingestion paths; for the SceneAPI path the `.azmodel` is a sibling product of the same source FBX, for the JSON path it's the `source_model_asset_id` named in the JSON)
```

The pipeline is deterministic: same input bytes produce same output bytes. meshoptimizer's clustering depends on FP arithmetic, so SP1 locks the builder to the scalar/SSE2 path explicitly to make this hold across hosts. CI asserts byte-identical output for a fixture model on the Windows host; non-Windows host verification deferred until first non-Windows builder run.

### 5.2 SceneAPI export module (FBX path)

- New target: `Meshlets.Builders` (module).
- Reflects `MeshletPackRule` — fields: `max_vertices` (u16), `max_triangles` (u16), `cone_weight` (f32), `mesh_filter` (string list, default `*` = all meshes).
- `MeshletPackRule` registers as a SceneAPI rule on the `IMeshGroup` data type. Asset Editor surfaces it under the existing "Add Modifier" menu like `LodRule`/`TangentRule`.
- Export module reads the SceneAPI graph for the source FBX, walks `IMeshNode` instances, builds a `SourceMeshSet`, calls `MeshletPackBuilderCore`, writes the `.azmeshletpack` product, registers the product dependency.

Reference patterns to follow: `Gems/AtomTressFX/Code/Builders/HairAssetBuilder.cpp`, `Gems/SceneProcessing/Code/Source/SceneBuilder/SceneBuilderWorker.cpp`. SceneAPI export module shape can drift between O3DE versions — a 1-day spike against the current EMotionFX builder pre-commit confirms the current API surface (R1 in §10).

### 5.3 JSON sidecar source builder

- Same module (`Meshlets.Builders`), separate `AssetBuilderRegistrationBus` registration.
- Watches `*.meshletpack` files. JSON shape:

  ```json
  {
    "source_model_asset_id": "{12345678-1234-1234-1234-123456789012}:0",
    "max_vertices": 64,
    "max_triangles": 64,
    "cone_weight": 0.5,
    "mesh_filter": ["*"]
  }
  ```

- Builder loads the referenced `.azmodel` (blocking on first build, normal `Asset::AssetManager::GetAsset`), constructs `SourceMeshSet` from its `RPI::ModelAsset` lod[0] meshes (mirroring what `MeshletsRenderObject::CreateMeshletsFromModelAsset` does today, but offline), calls `MeshletPackBuilderCore`, writes the product, registers the source-`.azmodel` → product dependency.

### 5.4 Output product

`Cathedral.fbx` + scenemanifest rule → `Cathedral.azmodel` (existing, unchanged) + `Cathedral.azmeshletpack` (new product, sibling).

`Cathedral.meshletpack` (JSON, manually authored) → `Cathedral.azmeshletpack` (same byte format).

Both AssetProcessor product entries register a dependency on the corresponding `.azmodel`'s AssetId. The runtime queries this dependency table to do `modelAsset → packAsset` resolution at acquire time.

## 6. Runtime load + render

### 6.1 New asset class

`AZ::Meshlets::MeshletPackAsset`:

- `AZ_RTTI` + `AZ_CLASS_ALLOCATOR`.
- Subclass of `Data::AssetData`.
- Reflected: registered with the asset catalog, file extension `.azmeshletpack`.
- `MeshletPackAssetHandler` (subclass of `AzFramework::GenericAssetHandler<MeshletPackAsset>`):
  - On load: parses header, validates magic + version, walks ToC, slices section data into runtime structs (a struct per kind 0-5).
  - On unload: releases the parsed sections; the underlying byte buffer goes with the `Data::Asset`.
- Lives in `Meshlets.Reflect.Static`, linked into both runtime gem and builder.

Section-kind enum and byte-layout structs are also in `Meshlets.Reflect.Static` so builder + runtime share one source of truth.

### 6.2 Runtime acquire path

`MeshletsFeatureProcessor::AcquireInstance(Data::Asset<RPI::ModelAsset> modelAsset)`:

1. Look up product dependency: query AssetCatalog for products that depend on `modelAsset.GetId()`. Filter by extension `.azmeshletpack`. **Cache the result by `modelAsset.GetId()` so this is O(1) on subsequent acquires.**
2. If no pack: set `m_lastPackResolutionStatus[modelAsset.GetId()]` to `"no_rule"` (read by MeshComponent inspector — see §7); return `InvalidInstanceHandle`.
3. If pack exists: `Asset::AssetManager::Instance().GetAsset<MeshletPackAsset>(packId, AssetLoadBehavior::PreLoad)`, blocking on first use.
4. On load failure: status = `"load_failed"`, return `InvalidInstanceHandle`.
5. On success: ensure a `SharedRenderObjectEntry` exists for this `modelAsset.GetId()` (existing pattern, refcounted). The entry's `MeshletsRenderObject` is now constructed from the pack instead of from `RPI::ModelAsset`.
6. Build per-instance `MeshletsRenderInstance` (existing pattern: ObjectId, transform, InstanceSrg, DrawPacket).
7. Increment refcount, return handle.

### 6.3 `MeshletsRenderObject` constructor (pack-driven)

Currently: `MeshletsRenderObject(Data::Asset<RPI::ModelAsset>, MeshletsFeatureProcessor*)` runs CPU meshletization in `CreateMeshletsFromModelAsset`. SP1 changes the constructor signature to also accept the loaded `MeshletPackAsset`, and `CreateMeshletsFromModelAsset` is **deleted**. The new path:

1. Read pack `PackHeader` — assert `source_model_asset_id == modelAsset.GetId()`.
2. For each mesh in `MeshDescriptors`:
   a. Allocate from `SharedBuffer` for: index buffer (capacity = `cluster_count` × `max_triangles_per_cluster` × 3), uv buffer (debug). Sizing matches today's allocation.
   b. Create runtime buffers from pack vertex streams (kind 5). One `RHI::Buffer` per stream, bind flags `ShaderRead`. Vertex layout matches `MeshletsObjectRenderSrg.azsli` declarations exactly (canonical contract — see R3 in §10).
   c. Build `MeshRenderData` (existing struct):
      - `MeshletsCount` = cluster count for this mesh's LOD 0
      - `IndexCount` = `cluster_count` × `max_triangles_per_cluster` × 3 (worst-case allocation)
      - `ComputeSrg` / `ComputeBuffersDescriptors` / `ComputeBuffersViews` / `ComputeBuffers`: pack-driven, descriptors come from `ClusterDescriptors`, `TriangleIndices`, `VertexIndirection` sections.
      - `MeshDispatchItem`: existing pattern; `dispatchCount = cluster_count` (1 group per cluster).
      - `ObjectSrg` / `RenderBuffersDescriptors` / `RenderBuffersViews` / `RenderBuffers`: pack-driven from kind 5.
      - `IndexBufferView`: same `SharedBuffer` slice as today.
3. Push `MeshRenderData` into `m_modelRenderData[lodIdx]` (one LOD in SP1).

`PrepareRenderSrgDescriptors` / `PrepareComputeSrgDescriptors` / `CreateAndBindComputeSrgAndDispatch` / `CreateAndBindRenderBuffers`: kept, **unchanged in shape**. They take `MeshRenderData`; SP1 just feeds them pack-derived data instead of ModelAsset-derived data. The compute shader (`MeshletsCompute.azsl`) is unchanged. The debug render shader (`MeshletsDebugRenderShader.azsl`) is unchanged.

### 6.4 Per-frame topology (unchanged)

`MeshletsFeatureProcessor::Render` keeps its current shape:

1. Phase 1: dispatch one compute group per render object (one per cluster after the cluster-count update).
2. Phase 2: submit one DrawPacket per instance.

The compute pass writes the index buffer for each cluster; the render pass rasterizes the rebuilt index buffer via vertex-pull from the shared buffer + per-object SRG vertex streams. Same as today — only the data source for the SRGs has changed.

### 6.5 Pass-attach timing fix (cleanup C3 in §8)

Today: per-frame retry loop in `Render()` ([MeshletsFeatureProcessor.cpp:639-669](Gems/Meshlets/Code/Source/Meshlets/MeshletsFeatureProcessor.cpp:639)) checks if the SharedBuffer is attached to the compute and render passes; attaches if not.

SP1 replaces this with a one-shot attach:

1. End of `OnRenderPipelineChanged` (after `CreateResources` allocates the buffer and `Init` calls `InitComputePass`/`InitRenderPass` succeed): call a new `AttachSharedBufferToPasses()` once. Set `m_sharedBufferAttached = true`.
2. `CleanPasses` resets the flag (existing behavior).
3. Per-frame retry block in `Render()` is **deleted**.

Rationale: at the point `OnRenderPipelineChanged` returns successfully, both the buffer and the passes exist. The retry was an over-defensive workaround written when the timing wasn't well understood. The corrected path is one call site.

## 7. Editor integration

### 7.1 Asset Browser

- Register `.azmeshletpack` extension with `AssetBrowserBus::Handler` so the file shows up in the Asset Browser. Default icon (no custom thumbnail in SP1 — defer).
- Asset type registration through `Meshlets.Editor`'s existing system component.

### 7.2 Scenemanifest rule UI

- `MeshletPackRule` reflected with `EditContext` so Asset Editor's existing "Add Modifier" menu surfaces it. Same path as `LodRule` etc.; no new editor code required beyond the reflection.

### 7.3 `MeshComponent` inspector status field

- Add a read-only `AZStd::string` field `m_meshletPackStatus` to `MeshComponentController`'s edit context, group `"Virtual Geometry"`, visibility-bound to `m_useMeshlets == true`.
- Possible values:
  - `"OK"` — pack resolved and loaded.
  - `"No meshlet pack for this model — flag the source FBX with a Meshlet Pack rule"` — pack not found in dependency table.
  - `"Pack failed to load"` — pack found but `GetAsset` returned an error state.
  - `"Renderer disabled — meshlet pass not in pipeline"` — `m_loggedDisabledPipelines` includes the entity's render pipeline.
- Updated by `MeshComponent` polling the FP's status map on `OnEntityVisibilityChanged` / `OnAssetReady` / `OnRenderPipelineChanged`.

This is a status display, not a configuration field. Artists don't edit it; they see it.

## 8. Cleanup debt (atomic with SP1)

Mechanical removals — all are dead code, debug toggles, or workarounds for bugs that have since been fixed.

| # | What | Location | Why |
|---|---|---|---|
| C1 | Delete bisect toggle block in `Render()` (`s_disableComputeSubmit`/`s_disableRenderSubmit` + `AZ_TracePrintf`) | [MeshletsFeatureProcessor.cpp:671-701](Gems/Meshlets/Code/Source/Meshlets/MeshletsFeatureProcessor.cpp:671) | Author flagged "should come out" in commit 7e6e8ff208; bisect served its purpose |
| C2 | Delete `MESHLETS_DEBUG_HARDCODED_VS` block | [MeshletsDebugRenderShader.azsl:106-169](Gems/Meshlets/Assets/Shaders/MeshletsDebugRenderShader.azsl:106) | Author flagged "Set back to 0 once bisect done" in commit 47f71300fb |
| C3 | Replace per-frame attach-retry with one-shot in `OnRenderPipelineChanged` | [MeshletsFeatureProcessor.cpp:639-669](Gems/Meshlets/Code/Source/Meshlets/MeshletsFeatureProcessor.cpp:639) | Per §6.5; the right window is `OnRenderPipelineChanged` after `Init` succeeds |
| C4 | Demote noisy `AZ_Warning` to `AZ_TracePrintf` | `MeshletsAssets.cpp:140` and similar successful-completion warnings | Currently fires at WARNING level on every successful build — log noise. (Note: `MeshletsAssets.cpp` itself is deleted by C5; this applies to any analogous calls elsewhere. Check `MeshletsRenderObject.cpp` too.) |
| C5 | Delete dead `MeshletsAssets.{h,cpp}` files; remove from cmake | [meshlets_files.cmake:45-46](Gems/Meshlets/Code/meshlets_files.cmake:45) | Header self-documents as "reference/demo" — dead code on the runtime path |
| C6 | Delete `.tmp` artifact | `Gems/Meshlets/Code/Source/Meshlets/MeshletsFeatureProcessor.h.tmp.20040.1778293241262` | Editor backup file that escaped into the working tree |
| C7 | Delete CPU meshletization in `MeshletsRenderObject::CreateMeshletsFromModelAsset` and helpers | `MeshletsRenderObject.cpp` | Replaced by pack load (§6.3); whole code path goes away |

C7 is the largest mechanical change — it removes the runtime meshoptimizer dependency. The builder gains it (already a transitive dep via `3rdParty::meshoptimizer`).

**Kept (load-bearing despite "looks debug" framing):**

- `SRG_PerPass_WithFallback` and the `m_meshletsSharedBuffer` field in `MeshletsDebugRenderShader.azsl`'s PassSrg — this is the vertex-pull mechanism, not a placeholder. The VS reads the rebuilt index buffer via `PassSrg::m_meshletsSharedBuffer[m_indicesOffset + linearIndex]`.
- The deferred-DrawPacket-build retry loop in `Render()` (lines 681-695). This is a real, current pattern for "instance registered before pipeline state ready" (commit fda9893e31). Stays.
- `IsExecuteOnce()` skip in `OnRenderPipelineChanged` and `TryAutoInjectPasses` (commit fda9893e31). Stays.

## 9. Tests

### 9.1 Builder unit tests (new — `Meshlets.Builders.Tests`)

- **Determinism:** fixture mesh → pack bytes A; same fixture + same builder → pack bytes B; assert A == B byte-for-byte.
- **Round-trip:** build pack from a known mesh; decode `ClusterDescriptors`/`TriangleIndices`/`VertexIndirection` back into a flat triangle list; assert the set is equal to the source mesh's triangle list (ignoring order).
- **Bounds check:** every triangle's three vertex indices, after decoding, are in `[0, total_vertex_count)`. (Catches the kind of bug `MeshletsData::ValidateData` fixes today at runtime — the builder catches it offline now.)
- **Negative — pathological config:** `max_vertices=4`, `max_triangles=2` against a non-trivial mesh. Either errors out at `meshopt_buildMeshlets` returning 0 clusters, or produces a valid but useless pack. Assert that the failure is detected and surfaces as a build error, not a silent empty product.
- **Format compliance:** open the produced bytes, verify magic, version, ToC count, every section's offset is 16-byte aligned, every section's offset+size is within file size.

Fixtures live in `Gems/Meshlets/Code/Tests/Fixtures/`:
- `cube.fbx` (12 triangles, simplest possible) + `cube.assetinfo` with a default `MeshletPackRule`.
- `bunny.fbx` (Stanford bunny, ~70k triangles, exercises multi-cluster) + scenemanifest with default rule.

### 9.2 Vertex-stream layout cross-check (new CI test)

The pack's stream order/format must match `MeshletsObjectRenderSrg.azsli`'s expectations exactly. CI test parses the `.azsli` for `GetPosition`/`GetNormal`/`GetTangent`/`GetBiTangent`/`GetUV` declarations and asserts each maps to the corresponding stream in `kSemanticKind` enum + format match. Catches R3 silent failure at build time, not at GPU time.

### 9.3 Runtime load test (new — `Meshlets.Tests`)

- Load `cube.azmeshletpack` fixture, construct a `MeshletsRenderObject` from it, assert `MeshRenderData` has expected cluster count, run one frame against a headless renderer, assert no validation errors and no `DrawListContext` asserts.
- Negative: load a corrupted pack (truncated file, bad magic, version=0, unknown section kind in v1 reader) — assert handler returns failure cleanly without crashing.
- Bug repros (regression tests for the recent fixes — must keep passing):
  - Dangling DrawPacket: instance with deferred build, then assert `instance.DrawPacket` is `RHI::Ptr`-managed and survives the frame.
  - DXGI device-hung: pipeline state has empty `InputStreamLayout` (no stale channels).
  - Bitset OOB: `BuildInstanceDrawPacket` returns success-with-deferral on null-tag, doesn't ship the packet.

### 9.4 Integration test (new — extend the existing GameplayFramework integration test pattern, or new `Meshlets.IntegrationTests` gem)

Per recent precedent (`feat(GameplayFramework): add integration test Gem validating full GHT → compile pipeline`), an integration test gem that:

1. Drops a fixture `.fbx` with a scenemanifest containing `MeshletPackRule`.
2. Runs AssetProcessor.
3. Asserts `.azmeshletpack` product exists in the cache.
4. Asserts product-dependency entry: pack depends on `.azmodel`.
5. Loads a level with a `MeshComponent` referencing the fixture `.fbx`'s `.azmodel`, "Use Virtual Geometry" toggled on.
6. Tick one frame; assert no GPU validation errors, no asserts, no warnings.
7. Toggle off, toggle on; assert handle release/reacquire is clean (refcount returns to 0 then 1).

### 9.5 Manual smoke test (release checklist)

Not automated, but documented:

1. Drop `Cathedral.fbx` (or any production-shaped model) into a project's `Assets/`.
2. Open Asset Editor on the FBX, add Meshlet Pack Rule, save.
3. Wait for AssetProcessor to bake.
4. Confirm `Cathedral.azmeshletpack` appears in Asset Browser.
5. Place an entity with `MeshComponent` → `Cathedral.azmodel`, toggle "Use Virtual Geometry" on.
6. Confirm it renders (UV-as-color is expected — debug shader through SP1).
7. Confirm `MeshComponent` inspector status shows `"OK"`.
8. Remove the rule, force re-bake, reload level. Confirm status shows `"No meshlet pack for this model..."`.

## 10. Risks

| # | Risk | Mitigation |
|---|---|---|
| **R1** | SceneAPI export module API has drifted; the patterns in `HairAssetBuilder` and `EMotionFXBuilder` may use different APIs. | 1-day spike at the start of implementation: build a no-op SceneAPI rule + export module that just logs, confirm the registration shape works in the current codebase, then commit. Follow whichever existing builder is most recently updated. |
| **R2** | meshoptimizer cluster output differs across platforms due to FP arithmetic. | Lock builder to scalar/SSE2 path explicitly. Determinism CI test (§9.1) catches drift on the Windows host. Cross-platform verification deferred until first non-Windows builder run. |
| **R3** | Vertex stream layout mismatch between builder and runtime SRG = silent visual corruption. | Vertex-stream layout cross-check CI test (§9.2). Single source of truth: `MeshletsObjectRenderSrg.azsli`'s declarations are canonical; builder reads them at compile time (or has a hard-coded mirror with a cross-check test). |
| **R4** | Removing dead `MeshletsAssets.cpp` may break someone's example reference. | One-line update to `Gems/Meshlets/README.md`. The class wasn't documented as a public API. |
| **R5** | In-flight test scenes with the toggle on will start showing `"No meshlet pack..."` after SP1 ships. | One-time project-startup log line listing models in the scene that would benefit from a Meshlet Pack rule. Status surfaces in inspector — artists discover the issue immediately. |
| **R6** | `AcquireInstance` blocks on first pack load (synchronous). For a level with N meshlet entities, this serializes N pack loads on level open. | Acceptable for SP1 (current code blocks similarly on `RPI::ModelAsset`). SP6's streaming addresses async-load properly. |
| **R7** | Existing `MeshletsRenderObject` constructor signature is used by callers in the integration test we'll add. Changing it is a breaking change. | The constructor is internal to the gem (private to `MeshletsFeatureProcessor::AddInstance` / `AddMeshletsRenderObject`). No external callers. Confirmed by grep. |

## 11. Out of scope (explicit non-goals — restated)

- Cone bounds, GPU culling (SP2).
- LOD chains, simplification, DAG (SP3).
- HZB, two-phase culling (SP4).
- Vis-buffer, PBR materials, GBuffer integration (SP5).
- Streaming, page partition, LZ4 (SP6).
- Mesh shaders (back-burner indefinitely).
- Asset Browser thumbnail rendering for `.azmeshletpack`.
- Right-click bake / rebuild actions in Asset Browser.
- Editor preview viewport for `.azmeshletpack`.

## 12. Acceptance criteria

SP1 is done when **all** of these are true:

1. A fixture `.fbx` with a `MeshletPackRule` produces a `.azmeshletpack` through AssetProcessor.
2. A fixture `.meshletpack` JSON sidecar referencing an `.azmodel` produces a `.azmeshletpack` that round-trips correctly (decode → triangle set equals source mesh's triangle set).
3. For each path, two consecutive builds of the same source produce byte-identical output (intra-path determinism). The two paths are not expected to produce byte-identical output to each other — they consume different source representations (FBX scene graph vs. `.azmodel` post-import).
4. Format compliance test passes: magic, version=1, 6 ToC entries, all offsets aligned and in-bounds.
5. The runtime path no longer calls `meshopt_buildMeshlets` (grep confirms).
6. `MeshletsAssets.{h,cpp}` are deleted.
7. Bisect toggles (C1, C2) and per-frame attach-retry (C3) are deleted.
8. The GameplayFramework-style integration test passes one full asset-build → load → render-frame cycle.
9. The vertex-stream-layout cross-check CI test passes.
10. Smoke test (§9.5) renders a real-shaped model with `"OK"` status and no validation errors. Visual is expected to be UV-as-color (debug shader through SP1; PBR is SP5).

---

## Appendix A — files touched

**New files** (rough list; plan refines):
- `Gems/Meshlets/Code/Source/Meshlets/Reflect/MeshletPackAsset.{h,cpp}`
- `Gems/Meshlets/Code/Source/Meshlets/Reflect/MeshletPackAssetHandler.{h,cpp}`
- `Gems/Meshlets/Code/Source/Meshlets/Reflect/MeshletPackFormat.h` (section-kind enum, byte-layout structs)
- `Gems/Meshlets/Code/Source/Builders/MeshletPackBuilderCore.{h,cpp}`
- `Gems/Meshlets/Code/Source/Builders/MeshletPackRule.{h,cpp}`
- `Gems/Meshlets/Code/Source/Builders/SceneApiExportModule.{h,cpp}`
- `Gems/Meshlets/Code/Source/Builders/JsonSidecarBuilder.{h,cpp}`
- `Gems/Meshlets/Code/Source/Builders/BuilderModule.{h,cpp}`
- `Gems/Meshlets/Code/Tests/MeshletPackBuilderCoreTests.cpp`
- `Gems/Meshlets/Code/Tests/MeshletPackAssetTests.cpp`
- `Gems/Meshlets/Code/Tests/VertexStreamLayoutCrossCheckTest.cpp`
- `Gems/Meshlets/Code/Tests/Fixtures/cube.fbx` (+ `.assetinfo`)
- `Gems/Meshlets/Code/Tests/Fixtures/bunny.fbx` (+ scenemanifest)
- `Gems/Meshlets/IntegrationTest/` (new gem, optional — alternative is extending GameplayFramework's pattern)
- `Gems/Meshlets/Code/meshlets_reflect_files.cmake`
- `Gems/Meshlets/Code/meshlets_builders_files.cmake`

**Modified files**:
- `Gems/Meshlets/Code/CMakeLists.txt` — three new targets.
- `Gems/Meshlets/Code/meshlets_files.cmake` — remove `MeshletsAssets.{h,cpp}`.
- `Gems/Meshlets/Code/Source/Meshlets/MeshletsFeatureProcessor.{h,cpp}` — pack-resolution path, one-shot attach (C3), bisect-toggle delete (C1), status map for inspector field.
- `Gems/Meshlets/Code/Source/Meshlets/MeshletsRenderObject.{h,cpp}` — pack-driven constructor, delete `CreateMeshletsFromModelAsset` and helpers.
- `Gems/Meshlets/Assets/Shaders/MeshletsDebugRenderShader.azsl` — delete `MESHLETS_DEBUG_HARDCODED_VS` block (C2).
- `Gems/Meshlets/README.md` — update for asset pipeline; remove `MeshletsAssets.cpp` references.
- `Gems/AtomLyIntegration/CommonFeatures/Code/Source/Mesh/MeshComponentController.{h,cpp}` (or wherever the toggle lives) — add status field.

**Deleted files**:
- `Gems/Meshlets/Code/Source/Meshlets/MeshletsAssets.h`
- `Gems/Meshlets/Code/Source/Meshlets/MeshletsAssets.cpp`
- `Gems/Meshlets/Code/Source/Meshlets/MeshletsFeatureProcessor.h.tmp.20040.1778293241262`

## Appendix B — open questions, deferred to plan

These are implementation choices that don't change the design but the plan should resolve:

1. Should `Meshlets.IntegrationTests` be its own gem (per GHT precedent) or live inside `Meshlets/Code/Tests/`? Slight preference for own gem to mirror the established pattern.
2. The status map for inspector field reads — bus call into FP, or AssetSystemBus query, or AssetCatalog direct? Plan picks one based on what's accessible from `MeshComponentController`'s tier.
3. JSON schema validation for `*.meshletpack` files — use `AzCore::JSON` directly or a JSON-schema validator? Plan picks based on existing patterns (most likely `JsonSerialization` direct).
