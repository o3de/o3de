# Preview-only shader templates

These are copies of the `.shader.template` files in `Atom_Feature_Common/Assets/Materials/Pipelines` that the Material Canvas
preview pipeline references. Each is a verbatim copy of the original apart from one added field:

```json
"DisabledRHIBackends": ["null"]
```

They exist only so that field can be set without editing the shared engine templates, which every material type in the project
builds from. `MaterialPipelineSourceData::ShaderTemplate` has no field for this, so a local copy is the only place to put it.

## Why "null"

`ShaderAssetBuilder::ProcessJob` loops over every shader platform interface the Asset Processor's platform tags enable, and runs
MCPP and AZSLc once per interface before handing the result to that backend's compiler. The stock `pc` tags are
`tools,renderer,dx12,vulkan,null`, so a Standard PBR forward shader is preprocessed and transpiled three times.

`Null::ShaderPlatformInterface::CompilePlatformInternal` is a no-op that returns `true` — it produces a stub nothing ever draws
with. The DXC step is skipped, but the full preprocess and transpile pass in front of it is not, and on a Standard PBR shader
that pass flattens a large include tree. Listing `null` here skips it entirely while the preview pipeline is active.

`ShaderAssetBuilder` reads this through `DiscoverEnabledShaderPlatformInterfaces` in `ProcessJob`. `CreateJobs` still uses
`DiscoverValidShaderPlatformInterfaces`, so the job is created either way — the work inside it is what shrinks.

## Why dx12 and vulkan are not listed

Which of those two is redundant depends on the RHI the application is running, and a static file cannot know that. The
**Enable Faster Shader Builds** setting covers that case instead: it rewrites the Asset Processor platform tags at startup based
on `AZ::RHI::Factory::Get().GetName()`, leaving only the active backend.

The two settings compose. With **Enable Faster Shader Builds** off, these templates take the preview pipeline from three backends
to two. With it on, the tags already leave one backend and these files change nothing.

## Keeping them in sync

Nothing detects drift from the originals. If an upstream change touches one of the Atom_Feature_Common templates listed below,
the same change has to be made here by hand:

- `Common/DepthPass.shader.template`
- `Common/ShadowmapPass.shader.template`
- `MainPipeline/ForwardPass_BaseLighting.shader.template`
- `MainPipeline/ForwardPass_StandardLighting.shader.template`
- `MainPipeline/ForwardPass_EnhancedLighting.shader.template`
- `MainPipeline/ForwardPass_SkinLighting.shader.template`
- `MainPipeline/Transparent_StandardLighting.shader.template`
- `MainPipeline/Transparent_EnhancedLighting.shader.template`

The file names must not change. `MaterialPipelineScriptRunner` builds its lookup table by stripping the folder and the
`.shader.template` extensions, and `PreviewPipelineScript.lua` calls `IncludeShader` with those stems.

The `azsli` half of each `shaderTemplates` entry in `MainPipeline.materialpipeline` still points at Atom_Feature_Common. Only the
`.shader` JSON needed copying; the shader code itself is untouched.

## Preview fidelity cuts

Every forward and transparent template carries a `Definitions` block that compiles five features out of the preview shaders. They
are not new switches: `LightingOptions.azsli` declares all of them, `#ifndef` guarded, and its own header comment says they exist
"for customizing and optimizing material types". `ShaderQualityOptions.azsli` sets the same combination for
`QUALITY_LOW_END_TIER2`, so this is a configuration the engine already supports.

What matters is that they gate `#include` lines and not merely function bodies. `QuadLight.azsli` opens with
`#if ENABLE_QUAD_LIGHTS` above its own `#include <Ltc.azsli>`, so with the option off the file never reaches the preprocessor
output at all. Anything removed this way is gone before azslc reads a byte, rather than being parsed and then discarded.

### Why it is worth doing

Measured on a Material Canvas graph in this project, the preprocessed input azslc receives for one Standard PBR forward shader is
13,399 lines. Attributing every line through the `#line` directives the preprocessor leaves behind:

| Source | Lines | Share |
| --- | ---: | ---: |
| `PostProcessing/Aces.azsli` | 808 | 6.3% |
| the graph's own generated files | 1,251 | 9.7% |
| `PBR/Lights/Ltc.azsli` | 709 | 5.5% |
| `Shadow/DirectionalLightShadowCalculator.azsli` | 417 | 3.2% |
| `Shadow/ProjectedShadow.azsli` | 348 | 2.7% |
| `Debug.azsli` | 309 | 2.4% |
| `LightCulling/NVLC.azsli` | 286 | 2.2% |
| Quad, Disk and Capsule lights | 685 | 5.3% |

About 90% of what azslc parses on every single graph edit is engine shader library that did not change. The 808 lines of ACES
colour science are the clearest example: they arrive through
`ForwardPassDecals` to `Decals.azsli` to `BlendUtility.azsli` to `TransformColor.azsli` to `AcesCcToAcesCg.azsli`. Decal colour
blending is the only reason a material shader contains a tone mapping library.

The six defines together remove roughly 4,240 of those 13,399 lines:

| Define | Removes | Lines |
| --- | --- | ---: |
| `ENABLE_AREA_LIGHTS=0` | Ltc, Quad, Disk, Capsule and Polygon lights | ~1,394 |
| `ENABLE_DECALS=0` | Aces, AcesColorSpaceConversion, TransformColor | ~1,123 |
| `ENABLE_SHADOWS=0` | the four shadow filtering files | ~1,127 |
| `ENABLE_LIGHT_CULLING=0` | NVLC, but only since its include was guarded -- see below | ~286 |
| `ENABLE_ACESCC_COLOR_SPACE=0` | Aces (via TransformColor's ACEScc conversion) | ~808 |
| `ENABLE_SHADER_DEBUGGING=0` | a handful of branches inside Debug, not the file | ~15 |

**These rows do not sum, and the last one is not what it looks like.** Both mistakes are easy to make from a table like this,
so they are worth stating outright.

`Aces.azsli` reaches the shader by two independent routes: decals pull it in through `BlendUtility` to `TransformColor`, and
`sample_texture_2d` pulls it in through `TransformColor` directly. Whichever define is measured first is charged the full 808
lines and the other appears to save them again. Measuring each define alone and adding the results gives about 4,750; the
combined figure is ~4,240. Only ever quote the combined number, measured with every define set at once.

`ENABLE_SHADER_DEBUGGING=0` removes about 15 lines, not the 309 that `Debug.azsli` contributes. The define guards the code
that *uses* the debug data; the file itself is `#include`d unconditionally from `LightingData.azsli`, `Ibl.azsli`,
`ForwardPassDirectLighting.azsli` and `DeferredPassEvaluateLighting.azsli`. An earlier version of this table credited it with
the full 309, which was wrong. It is kept in the `Definitions` block because 15 lines is still 15 lines and it costs nothing.

`ENABLE_LIGHT_CULLING=0` did not remove `NVLC.azsli` either, for the same reason, until this branch guarded it.
`LightCullingShared.azsli` included NVLC unconditionally and `LightCullingTileIterator.azsli` included that, so 285 lines
survived an option everyone assumed removed them. The iterator needed exactly two sentinel constants from the header on the
culling-disabled path, so it now defines those in an `#else` and skips the include. Note the include order there: the guard
has to sit below `LightingOptions.azsli`, because `#if` treats an undefined `ENABLE_LIGHT_CULLING` as 0 and the original
order would have dropped light culling from every shader in the engine.

### Verifying this table

Do not trust the estimates; read the preprocessor's own output. Every `.azslin` in `Cache/pc` carries `#line` directives, so
attributing its lines back to source files says exactly what survived. That is how both errors above were caught -- the
defines were all set correctly and the files were still there:

```
crystal_shader_materialcanvaspreview_transparent_standardlighting_dx12.azslin   8,343 content lines, 106 files
  Aces.azsli            absent      ENABLE_ACESCC_COLOR_SPACE / ENABLE_DECALS worked
  Ltc.azsli             absent      ENABLE_AREA_LIGHTS worked
  ParallaxDepth.azsli   absent      already off; see below
  Debug.azsli           304 lines   ENABLE_SHADER_DEBUGGING did not remove it
  NVLC.azsli            286 lines   ENABLE_LIGHT_CULLING did not remove it (fixed since)
```

`z_attribute_shader_lines.ps1` in the project root does this. Run it after any change here.

### Investigated and rejected

Two further cuts look obvious from the attribution table and are not worth making. Both were measured; this section exists so
the next person does not spend the afternoon rediscovering them.

**Guarding the four `Debug.azsli` includes** would recover the remaining ~294 lines, worth 30-50ms at the measured 0.10-0.17ms
per line. `Debug.azsli` is deliberately written to be included unconditionally: its predicates (`IsDirectLightingEnabled`,
`IsDiffuseLightingEnabled`, `AreNormalMapsEnabled`, `UseDebugLight` and six more) return the 'everything on' answer when
debugging is off, so ordinary lighting code calls them without guards. Twelve files across the engine depend on that, including
`ForwardPassVertexAndPixel.azsli`, which calls `DebugModifyOutput` on the main pixel path. Guarding the includes means shipping
a stub header that redeclares that whole surface and keeping it in sync with `Debug.azsli` forever, and a drift between the two
would show up as a miscompiled production shader rather than a build error. Not worth 40ms.

**`ENABLE_PARALLAX=0`** saves nothing here. Material Canvas already defaults it to 0 in
`MaterialGraphName_Defines.azsli` for both BasePBR and StandardPBR -- parallax is disabled until the parallax depth functions
stop taking the heightmap and sampler from the material SRG. Setting it in these templates changes nothing for a graph that
does not use parallax, and silently breaks the preview for one that does. Measurements showing a saving for this define were
taken on a hand-authored StandardPBR material, where the default is 1.

azslc cost tracks input size and does so slightly worse than linearly: across seven shaders between 13.4k and 15.5k lines the
average was about 0.10 ms per line while the marginal cost was about 0.17 ms per line. dxc then parses the HLSL azslc emits, so a
smaller input shortens both halves of the job.

### What the preview gives up

This is a deliberate trade and it is visible. The preview shaderball loses area lights, decals, received shadows and the shader
debug views. It keeps the directional light, simple point and spot lights, IBL, clear coat and the full BRDF, which is what the
material being authored actually looks like.

Production shaders are unaffected. These templates are reachable only through the preview-only material pipeline, and only while
that pipeline is enabled.

### Re-measuring

`z_measure_shader_compilers.ps1` in the project root re-runs azslc on the `.azslin` products the Asset Processor leaves in the
cache, with the Asset Processor out of the picture. Run it before and after a change here to see what actually moved.

## Disabled RHI backends

`ShaderAssetBuilder` runs its entire compile once per enabled `ShaderPlatformInterface` — MCPP, azslc and DXC, the whole chain — in
the loop over `DiscoverEnabledShaderPlatformInterfaces` (`ShaderAssetBuilder.cpp`). The `pc` platform is tagged
`"tools,renderer,dx12,vulkan,null"` in `Registry/AssetProcessorPlatformConfig.setreg`, so a shader that disables neither is built
twice over.

Measured on one preview shader for `crystal_shader`: the Shader Asset job took 2.319 s of a 3.4 s edit-to-viewport round trip,
against roughly 670 ms for azslc alone on the DX12 input. Two backends accounts for the difference.

The Material Canvas viewport binds one backend, so the second set of bytecode is never used. These templates therefore disable
`vulkan` as well as `null`. This is scoped to the preview pipeline: production shaders are built from Atom_Feature_Common's own
templates and still target every enabled backend.

**Remove `"vulkan"` from `DisabledRHIBackends` in all ten templates if you run the editor on Vulkan.** A preview material whose
shaders were built only for DX12 has nothing to draw with on a Vulkan device, and the shaderball will simply not appear. There is
no error for this — `IsRhiBackendDisabled` is an exact, case-sensitive string match against the backend's `APINameString`, and a
name that matches nothing is silently ignored, so a typo here fails the same quiet way.
