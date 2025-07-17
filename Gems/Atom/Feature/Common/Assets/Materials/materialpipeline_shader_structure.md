### Glossary

- **Render Pipeline**: Sequence of Render-Passes, declared in e.g. `MainRenderPipeline.azasset` and `MainPipeline.pass`, or `LowEndRenderPipeline.azasset` and `LowEndPipeline.pass`
- **Materialtype**: A list of material properties, material functors and rasterpass shaders needed to render a mesh with that material, e.g. `AutoBrick.materialtype`, or `basepbr_generated.materialtype`  
- **Abstract Materialtype**: A list of material properties and material-functors and several shader functions, but no actual shaders. Also specifies a lighting model needed for the material pipeline. E.g. `BasePBR.materialtype`
- **Material**: An instance of a MaterialType with specific property values, assigned to a mesh and rendered.
- **Material Instance**: This term is sometimes used in the Editor, but it is functionally the same as a Material, i.e. a set of property values for a material-type.
- **Material Property**: A generic property value for a material, either directly used by the shaders (e.g. `baseColor`, `baseColorMap`, and `baseColorFactor`), as shader-option (e.g. `enableShadows` or `enableIBL`), or as rasterizer setting (e.g. `doubleSided`) 
- **Material Shader Parameters**: Specific set of property values (e.g. a texture, or a color) used by shaders during rendering. 
- **Material Functors**: Generic Lua scripts or C++ functions that are executed when material properties are modified, e.g. `UseTexture` sets a shader-option and a texture-map - index when a texture is selected.
- **Material Pipeline**: A list of shader templates that are compatible with a Render Pipeline and that use the material-shadercode from the abstract materialtype. A material-pipeline generates a non-abstract materialtype and shaders, so that an abstract material can be rendered with a specific render pipeline, e.g (`MainPipeline.materialpipeline` or `LowEndPipeline.materialpipelin`).
- **Material Pipeline Script**: A lua script that filters the shader templates of the material pipeline based on the lighting model of the abstract materialtype, e.g. `MainPipelineScript.lua` or `LowEndPipelineScript.lua`
- **Material Pipeline Functors**: Similar to material functors, but specified in the material pipeline: Generic lua scripts that are executed when a material property is modified: e.g. `ShaderEnable.lua` enables or disables transparency - shaders based on the `isTransparent` - property of the material. 
- **Material Canvas**: A graphical node-based editor used to create custom shader code for material types. The graph is converted into shader code for abstract material-types, which in turn use materialpipelines to create shaders for specific render pipelines.

### MaterialPipeline basics

At the most basic level a (non-abstract) material in O3DE provides a set of shaders that are executed by the matching raster passes for each mesh using that material. This means that the material shaders and the render pipeline have to be closely linked: In order to render a mesh in a Forward+ pipeline, the material has to provide at least a depth pre-pass shader and an opaque shader, and all shaders have to use the same render attachments as the corresponding raster passes. Furthermore, the opaque shader for the Forward+ pipeline expects a view-based tiled light assignment, and a cascaded shadowmap for the directional lights, which consequently also forces the render pipeline to provide these things.

In O3DE, the render pipeline is very configurable. It is reasonably easy to set up a deferred render pipeline or a fully path-traced render pipeline. But since the material shaders are closely linked to the render pipeline, they cannot be reused across different configurations. For example, in a deferred render pipeline, the opaque pass is executed as a fullscreen shader per material type---as opposed to a raster pass, which is executed per mesh. As a different example, the default opaque shader expects a cascaded shadowmap for the directional light, but that shadowmap is only valid in the view frustum of the camera. This becomes problematic for features like ray traced reflections, where a mesh outside of the frustum still needs a valid shadow value. 

While a material type could theoretically provide shaders for both, Forward+ and deferred render pipelines, doing so would require adapting shaders each time a pipeline is modified or a new one is introduced. This means that every change to the render pipeline—such as adding or removing a render target in the opaque pass of the MainPipeline—necessitates updates to every opaque shader, for every material, in every project, including user-generated materials—or existing projects would break.

To solve this issue, O3DE uses material pipelines (e.g., `MainPipeline.materialpipeline`) and abstract material types to separate the material shaders from the render pipeline to some extend. The material pipeline is strictly related to the render pipeline, and defines a list of template shaders that are compatible with the render passes of the render pipeline. Each of these shaders call specific material functions at specific locations, and the abstract material has to provide these functions. This allows O3DE to generate the actual material shaders needed to render a mesh with any render pipeline.

_Note:_ Material pipelines do not explicitly specifiy the material functions or the shader structure, and the formal requirements to both the shaders and the abstract materials are fairly minimal: The abstract material type specifies two `.azsli` files in the `materialShaderDefines` and `materialShaderCode` fields. These are provided to the shader template via `MATERIAL_TYPE_DEFINES_AZSLI_FILE_PATH` and `MATERIAL_TYPE_AZSLI_FILE_PATH`, respectively. For additional background information: The define file referenced in `MATERIAL_PARAMETERS_AZSLI_FILE_PATH` contains a `struct MaterialParameters`, which is generated from the material shader parameters.

That being said, the abstract material types provided by the engine (namely `BasePBR`, `StandardPBR`, etc.) and the shader templates of the material pipelines (namely `MainPipeline`, `LowEndPipeline`, `MobilePipeline`, etc.) do follow a fairly strict interface, which is described below. In order to integrate custom material types into existing render pipelines, or to create a new render pipeline that uses the existing materials, it is suggested to stick to something very close to this.

**Example:**
- `MainPipeline.materialpipeline` defines the shader template `ForwardPass_BaseLighting` and the `pipelineScript` `MainPipelineScript.lua`
- `BasePBR.materialtype` defines `lightingModel`: `Base`, `materialShaderDefines`: `BasePBR_Defines.azsli` and `materialShaderCode`: `BasePBR.azsli`.
- This means that the `MainPipelineScript.lua` script selects the `ForwardPass_BaseLighting` shader template, and instantiates an actual shader by combining the `ForwardPass_BaseLighting.azsli` with `BasePBR_Defines.azsli` and `BasePBR.azsli`. 
    - That instantiated shader can be found in `<project>/Cache/Intermediate Assets/materials/types/basepbr_mainpipeline_forwardpass_baselighting.azsl`, together with `basepbr_generated.materialtype`.

### Material Interface Basics

- `VsOutput EvaluateVertexGeometry(VsInput IN, VsSystemValues SV, const MaterialParameters params);`
  - Generally called in the vertex shader, and is supposed to provide the vertices in normalized device coordinates.
  - The structs `VsInput` and `VsOutput` are defined by the shader template, but can be controlled with various defines, e.g., `MATERIAL_USES_VERTEX_POSITION` or `MATERIAL_USES_VERTEX_NORMAL`.
  - The struct `MaterialParameters` is defined by the material.
  - The struct `VsSystemValues` is defined by the shader template, and should be treated as opaque by the material.
- `PixelGeometryData EvaluatePixelGeometry(VsOutput IN, VsSystemValues SV, bool isFrontFace, const MaterialParameters params);`
  - Generally called in the pixel shader, forwards values from the `VsOutput` struct, and constructs the per-pixel tangent frame. 
  - Can be used to procedurally generate geometry, or to prepare input for a more generic `EvaluateSurface`.
  - The struct `PixelGeometryData` is defined by the material, and opaque to the shader template.
- `Surface EvaluateSurface(VsOutput IN, VsSystemValues SV, PixelGeometryData geoData);`
  - Generally called in the pixel shader. It mostly samples textures.
  - The struct `Surface` is defined by the material. It needs to provide a few fixed members, e.g., `position`.
- `LightingData EvaluateLighting(Surface surface, IN.position, ViewSrg::m_worldPosition.xyz);`
  - Generally called in the pixel shader, and is supposed to calculate the final shading.
  - The struct `LightingData` is defined by the material. It needs to provide a few fixed members, e.g., `diffuseLighting`.
  - Defined by the shader template. It iterates over the assigned lights and evaluates the shadows.
  - Uses several material-defined functions:
    - `void InitializeLightingData(Surface surface, float3 viewPosition, inout LightingData lightingData);`
    - Expects a `<LightType>Util` class for each light type with several functions that are specific to each light type, but mostly boil down to:
      - `static <LightType>Util Init(<LightType> light, Surface surface, float3 cameraPositionWS);`
      - `real GetFalloff();`
        - The falloff value is used to skip a light before potentially costly shadow evaluations are performed.
      - `void Apply(<LightType> light, Surface surface, real litRatio, inout LightingData lightingData);`
    - `void FinalizeLightingData(Surface surface, inout LightingData lightingData);`


### Excursion: Inheritance in HLSL

Inheritance is not supported in [HLSL](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl). Sometimes, some "define magic" is used to achieve a similar effect like, e.g., the following in `BasePBR_VertexEval.azsli`:

```cpp
#ifndef EvaluateVertexGeometry
#define EvaluateVertexGeometry EvaluateVertexGeometry_BasePBR
#endif

VsOutput EvaluateVertexGeometry_BasePBR() 
...
```

This way, `#define EvaluateVertexGeometry EvaluateVertexGeometry_StandardPBR` can be used in `StandardPBR_VertexEval.azsli` before including the BasePBR file.
This allows to call `EvaluateVertexGeometry_BasePBR()` from within the `EvaluateVertexGeometry_StandardPBR()` function, in addition to our own shader code. 
In the rest of the shader code, `EvaluateVertexGeometry()` can be called in an agnostic way w.r.t. which function is actually called in these places.

### ForwardPass_BaseLighting.azsli as a specific example for the Material Interface

`Gems/Atom/Feature/Common/Assets/Materials/Pipelines/MainPipeline/ForwardPass_BaseLighting.azsli` with `BasePBR_Defines.azsli` and `BasePBR.azsli`.

```cpp
#pragma once

#include <Atom/Features/SrgSemantics.azsli>

// The following defines provide information about the current shader's capabilities to the material shader code.
// E.g., the depth-shader (without custom z) has no pixel shader stage, so the material shader only needs to define 'EvaluatePixelGeometry()' and related structs.
#define MATERIALPIPELINE_SHADER_HAS_PIXEL_STAGE 1
#define OUTPUT_DEPTH 0
#define FORCE_OPAQUE 1
#define OUTPUT_SUBSURFACE 0
#define ENABLE_TRANSMISSION 0
#define ENABLE_SHADER_DEBUGGING 1
#define ENABLE_CLEAR_COAT 0

//////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef MATERIAL_TYPE_DEFINES_AZSLI_FILE_PATH
#include MATERIAL_TYPE_DEFINES_AZSLI_FILE_PATH
// -------------------------------------------------------------------------------------------------
// This is declared in the file 'BasePBR.materialtype'---specifically in the field "materialShaderDefines", which has the value "BasePBR_Defines.azsli" in this example.
// 
// The material needs to define the following SRGs and structs in this file:

// #if PIPELINE_HAS_OBJECT_SRG
// ShaderResourceGroup ObjectSrg : SRG_PerObject {};
// #endif /* PIPELINE_HAS_OBJECT_SRG */
//
// #if PIPELINE_HAS_DRAW_SRG
// ShaderResourceGroup DrawSrg : SRG_PerDraw {};
// #endif /* PIPELINE_HAS_DRAW_SRG */
//
// Usually these are declared by including DefaultDrawSrg.azsli and DefaultObjectSrg.azsli. When using a custom SRG, however, at least the same fields should 
// be present, since they are filled by the feature processors during creation of the draw items.
// NOTE: A deferred pipeline or a raytracing pipeline cannot have an ObjectSRG or a DrawSRG, meaning the relevant defines will be set to 0, and the information
// has to be provided in a different way.
// 
// struct MaterialParameters{}; 
// Parameter inputs for the material. This is generally generated by the material pipeline, and works by using '#include MATERIAL_PARAMETERS_AZSLI_FILE_PATH'.
// When this struct is defined manually, SceneMaterialSrg cannot be used, since then the layout of the struct is not known in C++ and the parameters cannot be stored in a structured buffer.
// 
// ShaderResourceGroup MaterialSrg : SRG_PerMaterial {};
// The definition of MaterialSrg is fairly strict, and usually it is the easiest to use '#include <Atom/Features/Materials/MaterialSrg.azsli>', which uses the MaterialParameters struct.
// While a custom MaterialSrg can be used, the same limitations apply as with the MaterialParameters struct above: SceneMaterialSrg cannot be used.
// 
// The SceneMaterialSrg is required for accessing more than one set of material parameters at once, like, e.g., in a deferred pipeline.
// 
// These are usually not fully defined by the material, but are instead controlled by several defines: See 'BasePBR_VertexData.azsli' in conjunction with 'ForwardPassVertexData.azsli'.
// struct VsInput{};            // Inputs to the vertex shader
// struct VsOutput{};           // Output of the vertex shader, input to the pixel shader
// struct VsSystemValues{};     // Values specific to the render pipeline (e.g. `instanceId` for Forward+), usually treated as an opaque struct that is passed to various utility functions, and never declared by the material directly.
//   How everything fits together can become complex. E.g., the InstanceId needs to be passed from the vertex shader to the pixel shader and therefore, needs to be part of the VsOutput struct in a Forward+ pipeline only.

// struct PixelGeometryData{}; // Converts the VsOutput to input for the surface data; 
                               // Ideally, this does parallax handling and alpha clipping, so that the custom z pass can stop after this.
                               // In practice, alpha clipping currently happens with the surface.
// See 'BasePBR_PixelGeometryData.azsli' 

// struct SurfaceData{};       // Contains all the surface data needed to evaluate the lighting on a pixel. 
                               // Generally textures are sampled when creating this structure.
// See 'BasePBR_SurfaceData.azsli' 

// struct LightingData{};      // Contains input and output for the actual lighting evaluation.
                               // The lighting evaluation functions use this during the evaluation of the separate light types.
                               // The shader converts this to the actual output of the pixel shader (which depends on the render pipeline).
                               // This means the struct needs to have least the members which are expected by these functions and shaders.
// See 'BasePBR_LightingData.azsli'

// -------------------------------------------------------------------------------------------------
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////

#include <viewsrg_all.srgi>  // collects the 'partial ShaderResourceGroup ViewSrg {};'
#include <scenesrg_all.srgi> // collects the 'partial ShaderResourceGroup SceneSrg {};'
#include <Atom/Features/Pipeline/Forward/ForwardPassSrg.azsli>  // Defines the 'ShaderResourceGroup PassSrg : SRG_PerPass {};' as suitable for the forward pass in the main pipeline.
#include <Atom/Features/Pipeline/Forward/ForwardPassVertexData.azsli>  // Uses the defines created by the material and actually declares the structs VsInput, VsOutput and VsSystemValues.
#include <Atom/Features/Pipeline/Forward/ForwardPassPipelineCallbacks.azsli> // Wrapper functions to access the DrawSrg/ObjectSrg/MaterialSrg/etc.
                                                                             // Currently, the following functions are defined:
                                                                             // 'uint GetLightingChannelMask(VsSystemValues SV);'
                                                                             // 'float4x4 GetObjectToWorldMatrix(VsSystemValues SV);'
                                                                             // 'float3x3 GetObjectToWorldMatrixInverseTranspose(VsSystemValues SV);'
                                                                             // 'float4x4 GetViewProjectionMatrix(VsSystemValues SV);'
                                                                             // 'bool HasGoboTexture(int index);'
                                                                             // 'Texture2D<float> GetGoboTexture(int index);'
                                                                             // And when MATERIALPIPELINE_USES_PREV_VERTEX_POSITION is defined:
                                                                             // 'float4x4 GetObjectToWorldMatrixPrev(VsSystemValues SV);'
                                                                             // 'float4x4 GetViewProjectionPrevMatrix(VsSystemValues SV);'
 
//////////////////////////////////////////////////////////////////////////////////////////////////

#include MATERIAL_TYPE_AZSLI_FILE_PATH
// -------------------------------------------------------------------------------------------------
// This is declared in the file 'BasePBR.materialtype', specifically in the field "materialShaderCode", which has the value "BasePBR.azsli" for this example
// 
// The material needs to define at least the following functions:
// 
// - 'VsOutput EvaluateVertexGeometry(VsInput IN, VsSystemValues SV, const MaterialParameters params);'
//     Called in the vertex shader (in the Forward+ pipeline at least), and converts VsInput into VsOutput, i.e., performs the view-projection transformation.
//     If the material doesn't demand a custom z shader, only this function will be called for the depth pass.
// 
// - 'PixelGeometryData EvaluatePixelGeometry(VsOutput IN, VsSystemValues SV, bool isFrontFace, const MaterialParameters params);'
//     This is called in the pixel shader, and converts VsOutput IN into PixelGeometryData.
//
// - 'Surface EvaluateSurface(VsOutput IN, PixelGeometryData geoData, const MaterialParameters params);'
//     Called in the pixel shader. It converts PixelGeometryData to surface data. 
//     Most texture sampling happens in this functions, and the materials generally evaluate most of the material parameters (and shader options) here.
//     For convenience, the material inputs should also be used here, e.g., 'BaseColorInput.azsli' provides 'GetBaseColorInput()' and 'BlendBaseColor()', which can be used together 
//     with the shader options defined with 'COMMON_OPTIONS_BASE_COLOR', and it uses the material parameters defined in 'BaseColorPropertyGroup.json'.
//     NOTE: The 'COMMON_SRG_INPUTS_BASE_COLOR' are obsolete; They were used to manually create the MaterialSRG before struct MaterialParameters existed.
// 
// - 'real3 GetDiffuseLighting(Surface surface, LightingData lightingData, real3 lightIntensity, real3 dirToLight);'
// - 'real3 GetSpecularLighting(Surface surface, LightingData lightingData, const real3 lightIntensity, const real3 dirToLight);'
//     Called during the (default) lighting evaluation. it allows the material to provide its own BRDF:
// 
// - 'void InitializeLightingData(inout Surface surface, float3 viewPosition, inout LightingData lightingData);'
// - 'void FinalizeLightingData(Surface surface, inout LightingData lightingData);'
//     Called before and after the lighting evaluation.
//
// - class CapsuleLightUtil 
//   {
//     static bool CheckLightChannel(CapsuleLight light, LightingData lightingData);
//     static CapsuleLightUtil Init(CapsuleLight light, Surface surface, float3 cameraPositionWS);
//     real GetFalloff();
//     void Apply(CapsuleLight light, Surface surface, [real litRatio], inout LightingData lightingData);
//     static void ApplySampled(CapsuleLight light, Surface surface, inout LightingData lightingData, const uint sampleCount); // optional
//   };
//     Used during the lighting evaluation for each light type (capsule, directional, disk, point, polygon, quad, simple point light, simple spot light).
//     If the default PBR implementation shall be used in a custom shader, one has to include "Atom/Features/PBR/Lights/Lights.azsli". It provides a default
//     <Type>LightUtil class for each supported light type.
//
// -------------------------------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////////////////////////

// Lighting eval functions from the forward pass that use functions and classes defined by the material
#include <Atom/Features/Pipeline/Forward/ForwardPassEvaluateLighting.azsli>
// - 'EvaluateLighting(Surface surface, float4 screenPosition, float3 viewPosition);'
//     This function is specific to the Forward+ pipeline. It uses the TileLightingIterator from the PassSrg to iterate over each assigned light,
//     evaluates the shadows (if applicable), and uses the '<Type>LightUtil' class to apply light to the surface (or skip it).

// The actual vertex and pixel shader code:
#include "ForwardPassVertexAndPixel.azsli"
// The vertex shader calls:
//    'VsOutput OUT = EvaluateVertexGeometry(VsInput IN, VsSystemValues SV, MaterialParameters params);'
// The pixel shader subsequently calls:
//    'PixelGeometryData geoData = EvaluatePixelGeometry(VsOutput IN, VsSystemValues SV, bool isFrontFace, MaterialParameters params);'
//    'Surface surface = EvaluateSurface(VsOutput IN, PixelGeometryData geoData, MaterialParameters paramn);'
//    'LightingData lightingData = EvaluateLighting(Surface surface, float4 IN.position, float3 ViewSrg::m_worldPosition.xyz);'
// and then converts the 'lightingData' into whatever output the current pass requires.
```
