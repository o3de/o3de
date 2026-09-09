# Preview-only shader templates

These are copies of the `.shader.template` files in `Atom_Feature_Common/Assets/Materials/Pipelines` that the Material Canvas
preview pipeline references. Each is a verbatim copy of the original apart from four added fields:

```json
"DisabledRHIBackends": ["null", "vulkan"],
"SkipIncludeFileDependencies": true,
"AddBuildArguments": { "dxc": ["-O1"] },
"Definitions": [ "ENABLE_AREA_LIGHTS=0", ... ]
```

They exist only so those can be set without editing the shared engine templates, which every material type in the project builds
from. `MaterialPipelineSourceData::ShaderTemplate` has no field for any of them, so a local copy is the only place to put them.

Each has its own section below: the backends under "Disabled RHI backends", the include dependencies under "Skipped include
file dependencies", the optimisation level under "Investigated and rejected" (`-O1` is a saving; `-O0` is slower, which is not
the obvious way round), and the definitions under "Preview fidelity cuts".

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
- `MainPipeline/TintedTransparent_StandardLighting.shader.template`
- `MainPipeline/TintedTransparent_EnhancedLighting.shader.template`

That is all ten files in this folder. Check the count against `ls *.shader.template` rather than trusting the list: an earlier
version of it named eight, having silently dropped both TintedTransparent templates, which is exactly the drift this section is
supposed to catch.

The file names must not change. `MaterialPipelineScriptRunner` builds its lookup table by stripping the folder and the
`.shader.template` extensions, and `PreviewPipelineScript.lua` calls `IncludeShader` with those stems.

The `azsli` half of each `shaderTemplates` entry in `MainPipeline.materialpipeline` still points at Atom_Feature_Common. Only the
`.shader` JSON needed copying; the shader code itself is untouched.

## Preview fidelity cuts

Every forward and transparent template carries a `Definitions` block that compiles six features out of the preview shaders. They
are not new switches: `LightingOptions.azsli` declares all of them, `#ifndef` guarded, and its own header comment says they exist
"for customizing and optimizing material types". `ShaderQualityOptions.azsli` sets the same combination for
`QUALITY_LOW_END_TIER2`, so this is a configuration the engine already supports.

What matters is that they gate `#include` lines and not merely function bodies. `QuadLight.azsli` opens with
`#if ENABLE_QUAD_LIGHTS` above its own `#include <Ltc.azsli>`, so with the option off the file never reaches the preprocessor
output at all. Anything removed this way is gone before azslc reads a byte, rather than being parsed and then discarded.

### Why it is worth doing

Measured on a Material Canvas graph in this project, the preprocessed input azslc receives for one Standard PBR forward shader is
13,399 lines. Attributing every line through the `#line` directives the preprocessor leaves behind (the figures in this table
predate the cuts; see "Verifying this table" below for what the shader looks like now):

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

The six defines together remove roughly 4,240 of those 13,399 lines, taking the preview transparent shader to its current
5,469 content lines:

| Define | Removes | Lines |
| --- | --- | ---: |
| `ENABLE_AREA_LIGHTS=0` | Ltc, Quad, Disk, Capsule and Polygon lights | ~1,394 |
| `ENABLE_DECALS=0` | Aces (its decal route only -- see below) | ~808 |
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

**`ENABLE_DECALS=0` removes `Aces.azsli` and nothing else.** An earlier version of this table credited it with
`AcesColorSpaceConversion.azsli` and `TransformColor.azsli` as well. Both are still in the output, at 126 and 124 lines, and for
the reason the paragraph above already gives: `sample_texture_2d` pulls `TransformColor` in directly, so turning decals off
removes only the decal route. Attribution says so plainly -- the two files are still listed, `Aces.azsli` is not.

`ENABLE_SHADER_DEBUGGING=0` removes about 15 lines, not the whole of `Debug.azsli` (309 lines when this was written, 212 in the
current preview shader -- the file is still there either way). The define guards the code
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
crystal_shader_materialcanvaspreview_transparent_standardlighting_dx12.azslin   5,469 content lines, 113 files
  Aces.azsli                     absent      ENABLE_ACESCC_COLOR_SPACE / ENABLE_DECALS worked
  Ltc.azsli                      absent      ENABLE_AREA_LIGHTS worked
  QuadLight / DiskLight /
    CapsuleLight.azsli           absent      ENABLE_AREA_LIGHTS worked
  DirectionalLightShadowCalculator,
    ProjectedShadow.azsli        absent      ENABLE_SHADOWS worked
  NVLC.azsli                     absent      ENABLE_LIGHT_CULLING, once its include was guarded
  ParallaxDepth.azsli            absent      already off; see below
  Debug.azsli                    212 lines   ENABLE_SHADER_DEBUGGING does not remove it
  AcesColorSpaceConversion.azsli 126 lines   ENABLE_DECALS does not remove it
  TransformColor.azsli           124 lines   ENABLE_DECALS does not remove it
```

The largest single file left is `Atom/RPI/Math.azsli` at 368 lines, and the graph's own generated files come to about 770 --
roughly 14% of the shader. The rest is engine ShaderLib that is byte identical on every graph edit.

`z_attribute_shader_lines.ps1` in the project root does this:

```
powershell -File z_attribute_shader_lines.ps1 -AzslinPath <path to a .azslin in Cache/pc>
```

Run it after any change here. Absent files are the point: a define that worked removes its file from the listing entirely.

### Investigated and rejected

Four cuts look obvious and are not worth making. All were measured; this section exists so the next person does not spend the
afternoon rediscovering them.

**Guarding the four `Debug.azsli` includes** would recover the ~212 lines it still contributes, worth about **8 ms** at the
measured 0.037 ms marginal per line. (This used to read "30-50 ms at 0.10-0.17 ms per line", priced before the parser work; the
saving shrank with the parse, and the case against was already the stronger half of the argument.) `Debug.azsli` is
deliberately written to be included unconditionally: its predicates (`IsDirectLightingEnabled`,
`IsDiffuseLightingEnabled`, `AreNormalMapsEnabled`, `UseDebugLight` and six more) return the 'everything on' answer when
debugging is off, so ordinary lighting code calls them without guards. Twelve files across the engine depend on that, including
`ForwardPassVertexAndPixel.azsli`, which calls `DebugModifyOutput` on the main pixel path. Guarding the includes means shipping
a stub header that redeclares that whole surface and keeping it in sync with `Debug.azsli` forever, and a drift between the two
would show up as a miscompiled production shader rather than a build error. Not worth 8 ms.

**`ENABLE_PARALLAX=0`** saves nothing here. Material Canvas already defaults it to 0 in
`MaterialGraphName_Defines.azsli` for both BasePBR and StandardPBR -- parallax is disabled until the parallax depth functions
stop taking the heightmap and sampler from the material SRG. Setting it in these templates changes nothing for a graph that
does not use parallax, and silently breaks the preview for one that does. Measurements showing a saving for this define were
taken on a hand-authored StandardPBR material, where the default is 1.

**Lowering DXC's optimisation level to `-O0`** is slower than `-O1`, not faster. On the preview transparent pixel shader
(4,282 lines of generated HLSL) the pixel stage measures 209 ms at `-O1` and 240 ms at `-O0`, both with process spawn
subtracted. The object code explains it: `-O1` emits 37,252 bytes of DXIL and `-O0` emits 65,276. DXC still runs its front end
and DXIL emission at `-O0`, and writing, validating and signing 75% more module costs more than the optimiser passes it skipped.
The templates therefore keep `-O1`, which is already well below DXC's `-O3` default and is where the saving actually is.

**Dropping `-Fh`** saves 6 ms of 209, about 3%. DXC writes a full text disassembly of the object code to that file and
`ShaderPlatformInterface.cpp` reads it back for one thing: counting dynamic branches, reported as a job byproduct. It is pure
diagnostic and nothing in the built shader depends on it, so removing it is tempting. It is not worth losing: the dynamic branch
count is what showed the production pixel shader carrying 674 branches against the preview's 72, which is the single most useful
number for deciding whether a graph or the pipeline is responsible for a slow compile. If it is ever removed, make it conditional
rather than deleted.

Process spawn is cheap and is not worth attacking: a one-line shader compiles end to end in 22 ms, so the five compiler
invocations in a preview shader job cost about 100 ms of process creation between them. An earlier harness in this project
reported 1,017 ms for the same one-line shader; that number was an artefact of the harness and should be ignored.

azslc cost tracks input size and does so slightly worse than linearly: across seven shaders between 13.4k and 15.5k lines the
average is about 0.038 ms per line and the marginal cost about 0.037 ms per line, measured by `z_azslc_per_line.ps1` across nine
cached shaders from 56 to 10,647 content lines. dxc then parses the HLSL azslc emits, so a smaller input shortens both halves of
the job.

Use the marginal figure to price a cut, not the average: the average includes a fixed per-run cost that removing lines does not
touch (process spawn alone is about 25 ms).

**These replace an earlier 0.10 average / 0.17 marginal, which are now about 4.5x too high.** They were measured before the
parser changes in `z_PREVIEW_PERFORMANCE.md` section 2.5, which cut the parse -- two thirds of an azslc run -- by 57%. Anything
priced against the old figures was overvalued by roughly that factor, which is worth knowing before reviving a rejected cut on
the strength of its line count.

### What the preview gives up

This is a deliberate trade and it is visible. The preview shaderball loses area lights, decals, received shadows and the shader
debug views. It keeps the directional light, simple point and spot lights, IBL, clear coat and the full BRDF, which is what the
material being authored actually looks like.

Production shaders are unaffected. These templates are reachable only through the preview-only material pipeline, and only while
that pipeline is enabled.

### Re-measuring

`z_azslc_per_line.ps1` in the project root re-runs azslc on the `.azslin` products the Asset Processor leaves in the cache, with
the Asset Processor out of the picture, and reports cost against input size. `z_azslc_ab.ps1` compares two azslc builds, first
for identical output across every cached shader and then for speed. Run either before and after a change here to see what
actually moved.

(An earlier version of this section named `z_measure_shader_compilers.ps1`, which does not exist.)

## Skipped include file dependencies

`ShaderAssetBuilder::CreateJobs` normally walks a shader's `#include` tree and registers every file it reaches as a source
dependency, so that editing any of them reprocesses the shader. That is what keeps a shader in step with the library it is built
from, and it is right for every shader except these.

The Material Canvas viewport compiles the preview shader **itself, in process**, from the same generated `.azsli` files the graph
rewrites on every edit (`InMemoryShaderCompiler.cpp`). With the dependency registered, an edit had the Asset Processor rebuild
the identical shader at the same moment, and the two compiles contended: 526 ms for the in-process compile alone against
726-861 ms racing each other.

`"SkipIncludeFileDependencies": true` turns that walk off for these shaders only. The dependency on the shader's own `.azsl` is
kept either way, so a change to the shader's own source still reprocesses it — only the library it includes stops doing so.

**Whoever sets this takes on responsibility for rebuilding.** Nothing else will notice the includes changed. The viewport does
that with `ClearFingerprintForAsset` whenever it cannot produce the shader itself: when the in-memory path is switched off, or
when a graph edit has changed the shader's interface and the cached asset is no longer safe to clone from. If the preview ever
goes stale and stays stale, that recovery path is the first thing to check.

The flag is `ShaderSourceData::m_skipIncludeFileDependencies` and it is general — any shader something else is responsible for
rebuilding can set it — but Material Canvas preview shaders are currently the only user.

## Disabled RHI backends

`ShaderAssetBuilder` runs its entire compile once per enabled `ShaderPlatformInterface` — MCPP, azslc and DXC, the whole chain — in
the loop over `DiscoverEnabledShaderPlatformInterfaces` (`ShaderAssetBuilder.cpp`). The `pc` platform is tagged
`"tools,renderer,dx12,vulkan,null"` in `Registry/AssetProcessorPlatformConfig.setreg`, so a shader that disables neither is built
twice over.

Measured on one preview shader for `crystal_shader` when this was written: the Shader Asset job took 2.319 s of a 3.4 s
edit-to-viewport round trip, against roughly 670 ms for azslc alone on the DX12 input. Two backends accounts for the difference.

Those absolute figures are long out of date -- the same job is now 0.530 s with azslc at about 234 ms, after the compiler work
in `z_PREVIEW_PERFORMANCE.md` sections 2.1 to 2.6. The ratio is what this section is about and it still holds: enabling a second
backend runs the whole MCPP to azslc to DXC chain a second time.

The Material Canvas viewport binds one backend, so the second set of bytecode is never used. These templates therefore disable
`vulkan` as well as `null`. This is scoped to the preview pipeline: production shaders are built from Atom_Feature_Common's own
templates and still target every enabled backend.

**Remove `"vulkan"` from `DisabledRHIBackends` in all ten templates if you run the editor on Vulkan.** A preview material whose
shaders were built only for DX12 has nothing to draw with on a Vulkan device, and the shaderball will simply not appear. There is
no error for this — `IsRhiBackendDisabled` is an exact, case-sensitive string match against the backend's `APINameString`, and a
name that matches nothing is silently ignored, so a typo here fails the same quiet way.
