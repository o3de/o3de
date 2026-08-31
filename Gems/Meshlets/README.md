# Meshlets Gem

GPU-driven mesh rendering using meshlets (mesh clusters). Converts FBX source
geometry into pack assets (`.azmeshletpack`) at build time, then loads and
renders them without per-frame CPU meshletization.

## Status

SP1 complete: asset-pipeline infrastructure (source-to-pack ingestion,
runtime loading, basic rendering). Mesh shaders, GPU culling, LOD, HZB, 
vis-buffer, and streaming are tracked separately across SP2–SP6 per the
north-star architecture doc (`docs/superpowers/specs/2026-05-08-meshlets-architecture.md`).
The current debug shader outputs UV-as-color.

## Enabling the gem in a project

1. Add `Meshlets` to your project's `enabled_gems.cmake`.
2. That's it for stock pipelines (`MainPipeline`, `LowEndPipeline`). The
   feature processor will auto-inject `MeshletsParentPass` after the first
   matching insertion point it finds (`OpaquePass`, then
   `MSAAResolvePass`, then `ForwardPass`).

For custom pipelines or to take explicit control, declare the pass yourself
in your pipeline template and the feature processor will use that instead
of auto-injecting. Reference wiring lives in
[`Assets/Passes/MeshletsPassRequest.azasset`](Assets/Passes/MeshletsPassRequest.azasset).

Example pipeline-template fragment (place after your opaque pass):

```json
{
  "Name": "MeshletsParentPass",
  "TemplateName": "MeshletsParentPassTemplate",
  "Enabled": true,
  "Connections": [
    {
      "LocalSlot": "DepthStencilInputOutput",
      "AttachmentRef": { "Pass": "DepthPrePass", "Attachment": "Depth" }
    },
    {
      "LocalSlot": "RenderTargetInputOutput",
      "AttachmentRef": { "Pass": "OpaquePass", "Attachment": "Output" }
    }
  ]
}
```

If the pass can neither be found nor injected, the feature processor
self-disables for that pipeline and logs the failure exactly once.

### Creating and using meshlet packs

An entity's `MeshComponent` can toggle "Use Virtual Geometry (Meshlets)" if
a sibling `.azmeshletpack` product exists for the referenced mesh asset. To
produce a pack:

- **Scenemanifest rule path:** Add a rule to the source FBX's scenemanifest
  (Asset Editor → Add Modifier → Meshlet Pack Rule). AssetProcessor will
  generate a `.azmeshletpack` product.
- **JSON sidecar path:** Author a `*.meshletpack` JSON descriptor next to
  the source FBX. AssetProcessor will load the descriptor and produce the
  pack. Refer to `docs/superpowers/specs/2026-05-08-meshlets-asset-pipeline-design.md`
  for the JSON schema and byte-layout details.

The gem's `MeshComponent` inspector shows the resolved pack status. If the
pack is missing or invalid, the status field reads diagnostic information.

## Pass templates

| Template | Purpose |
|---|---|
| `MeshletsParentPassTemplate` | Container; owns the compute → render dependency |
| `MeshletsComputePassTemplate` | Builds the index buffer per visible meshlet |
| `MeshletsRenderPassTemplate` | Rasterizes the rebuilt index buffer |

## Architecture

SP1 focuses on the asset pipeline (build-time pack generation) and basic
runtime loading. See `docs/superpowers/specs/2026-05-08-meshlets-architecture.md`
for the full north-star design.

```
Asset Pipeline (SP1)
  Source FBX
    ↓
  MeshletPackRule (scenemanifest) or *.meshletpack (JSON sidecar)
    ↓
  MeshletsBuilders (builds triangle encoding, descriptors)
    ↓
  .azmeshletpack (binary asset, product)
    ↓
  AssetCatalog (runtime resolution)
    ↓
  MeshletsRenderObject (runtime loading + compute dispatch)

Runtime (SP1)
  MeshletsFeatureProcessor
    ├── owns SharedBuffer (256 MB ring; meshlet index/vertex storage)
    ├── per-frame: dispatches one CS group per meshlet object
    └── per-frame: submits one DrawPacket per meshlet object

  MeshletsRenderObject
    ├── per-mesh meshlet data (from loaded pack)
    ├── ComputeSrg (writes index buffer)
    └── RenderSrg (reads vertex streams via SV_VertexID)

  MultiDispatchComputePass
    └── batches per-object dispatches; submit-range parallel command list build
```

## Smoke test (manual)

After enabling the gem in a project:

1. Drop a production-shaped FBX into your assets folder.
2. Add a `MeshletPackRule` via the Asset Editor (Add Modifier → Meshlet Pack Rule).
3. Wait for AssetProcessor to complete; confirm `<model>.azmeshletpack` appears in the cache.
4. Create an entity with a `MeshComponent` pointing to the FBX.
5. Toggle "Use Virtual Geometry (Meshlets)" on.
6. In the `MeshComponent` inspector, confirm the status field reads `"OK"`.
7. Launch the scene; verify it renders without DXGI errors or device-hung messages.
8. Confirm output uses the UV-as-color debug shader (expected through SP1).
9. Remove the Meshlet Pack rule and re-bake; status field should read `"No meshlet pack for this model..."`.

## Known limitations (Tier 2+ work)

- No frustum / cone / occlusion culling — every meshlet is dispatched and
  rasterized every frame
- No hierarchical LOD selection (always uses LOD 0)
- No instancing — N copies of a mesh produce N dispatches
- No PBR / material binding — debug shader only
- No indirect draw or dispatch
- No mesh shaders (rasterization only)
- Hard-coded 256 MB shared buffer; no growth strategy

## Dependencies

- `Gem::CommonFeaturesAtom.Static`
- `Gem::Atom_RPI.Public`
- `3rdParty::meshoptimizer` (builder-time dependency)
