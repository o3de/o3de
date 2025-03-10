## Light-evaluation for the Forward pass

The files in this folder are intended to be used with a deferred pipeline, where no `ObjectSrg` or `DrawSrg` is available. 

The main entry point is in `DeferredPassEvaluateLighting.azsli`, with the function `LightingData EvaluateLighting(Surface surface, float4 screenPosition, float3 viewPosition, VsSystemValues SV);`

This behaves exactly the same as with the Forward+ - pipeline (See `ForwardPassEvaluateLighting.azsli`), i.e. light-assignment is in screenspace and uses the `LightCullingTileIterator`, and the lights use the same types of shadows.

The only exception is the IBL light, since the reflection-probe data from the `ObjectSrg` is not available.

