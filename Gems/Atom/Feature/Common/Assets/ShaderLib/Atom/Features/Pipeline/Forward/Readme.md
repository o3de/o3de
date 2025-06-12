## Light-evaluation for the Forward pass

The files in this folder are intended to be used with the Forward+ - pipeline, or something very similar to it.

The main entry point is in `ForwardPassEvaluateLighting.azsli`, with the function `LightingData EvaluateLighting(Surface surface, float4 screenPosition, float3 viewPosition, VsSystemValues SV);`

The main assumptions are:
- The light assignment is tile-based in screenspace, and uses the `LightCullingTileIterator`
  - The variables `PassSrg::m_lightListRemapped` and `PassSrg::m_tileLightData` are accessible
  - The define `ENABLE_LIGHT_CULLING` can be used if this is not the case, and no culling should be used.
  - The lights are stored in the `ViewSrg`

- Following light-types with various different methods of shadows are supported:
  - Decals
    - Not strictly speaking lights, but they use the same light assignment method
    - Decal data is from the `ViewSrg`
  - CapsuleLight
    - No Shadows
    - Can be analytical (ltc) or sampled, although sampling is slow and exists mostly to verify the analytical solution
  - DirectionalLight
    - only one directional light can cast shadows
    - Uses Cascaded shadowmaps from the `PassSrg` (via the `DirectionalLightShadow` class)
  - DiskLight (aka spot-light with area)
    - Uses projected shadows from the `ViewSrg`
    - Can be analytical (ltc) or sampled, although sampling is slow
  - IBL
    - Uses environment maps from the `SceneSrg` and reflection probes from the `ObjectSrg`
  - PointLight (aka sphere light)
    - Uses 6 projected shadows from the `ViewSrg` as a cube-map
    - Can be analytical (ltc) or sampled, although sampling is slow
  - PolygonLight
    - No Shadows
    - Analytical evaluation with linearly transformed cosines (LTC)
  - QuadLight
    - No Shadows
    - Can be analytical (ltc) or sampled, although sampling is slow
  - SimplePointLights (withouth volume)
    - No shadows
  - SimpleSpotLights (aka spot-light without diameter)
    - uses projected shadows from the `ViewSrg`
