### Glossary

- **Rendering Pipeline**: Sequence of Rendering-Passes, declared in e.g. `MainRenderPipeline.azasset` and `MainPipeline.pass`, or `LowEndRenderPipeline.azasset` and `LowEndPipeline.pass`
- **Materialtype**: A list of material properties, material functors and rasterpass - shaders needed to render a mesh with that material, e.g. `AutoBrick.materialtype`, or `basepbr_generated.materialtype`  
- **Abstract Materialtype**: A list of material properties and material-functors and several shader functions, but no actual shaders. Also specifies a lighting model needed for the material pipeline. E.g. `BasePBR.materialtype`
- **Material**: An instance of a MaterialType with specific property values, assigned to a mesh and rendered.
- **Material Instance**: This term is sometimes used in the Editor, but it is functionally the same as a Material, i.e. a set of property values for a material-type.
- **Material Property**: A generic property value for a material, either directly used by the shaders (e.g. `baseColor`, `baseColorMap`, and `baseColorFactor`), as shader-option (e.g. `enableShadows` or `enableIBL`), or as rasterizer setting (e.g. `doubleSided`) 
- **Material Shader Parameters**: Specific set of property values (e.g. a texture, or a color) used by shaders during rendering. 
- **Material Functors**: Generic Lua scripts or C++ functions that are executed when material properties are modified, e.g. `UseTexture` sets a shader-option and a texture-map - index when a texture is selected.
- **Material Pipeline**: A list of shader templates that are compatible with a Rendering Pipeline and that use the material-shadercode from the abstract materialtype. A material-pipeline generates a non-abstract materialtype and shaders, so that an abstract material can be rendered with a specific rendering pipeline, e.g (`MainPipeline.materialpipeline` or `LowEndPipeline.materialpipelin`).
- **Material Pipeline Script**: A lua - script that filters the shader templates of the material pipeline based on the lighting model of the abstract materialtype, e.g. `MainPipelineScript.lua` or `LowEndPipelineScript.lua`
- **Material Pipeline Functors**: Similar to material functors, but specified in the material pipeline: Generic lua - scripts that are executed when a material property is modified: e.g. `ShaderEnable.lua` enables or disables transparency - shaders based on the `isTransparent` - property of the material. 
- **Material Canvas**: A Graphical Node-based editor used to create custom shader code for material types. The graph is converted into shader code for abstract material-types, which in turn use materialpipelines to create shaders for specific render pipelines.

### MaterialPipeline basics

At the most basic level a (non-abstract) material in O3DE provides a set of shaders that are executed by the matching Raster-Passes for each mesh using that material. This means that the material-shaders and the Rendering - pipeline have to be closely linked: In order to render a mesh in a Forward+ - pipeline, the material has to provide at least a depth-prepass shader and an opaque shader, and all shaders have to use the same render-attachments as the corresponding Raster-Passes. Furthermore, the opaque shader for the Forward+ - pipeline expects a view-based tiled light-assignment, and a cascaded shadowmap for the directional lights, which consequently also forces the Rendering-Pipeline to provide these things.

In O3DE the rendering-pipeline is very configurable, and it is reasonably easy to set up a deferred rendering pipeline or a fully path-traced rendering pipeline. But since the material-shaders are closely linked to the rendering pipeline, they cannot be reused across different configurations. For example, in a deferred rendering pipeline, the opaque pass is executed as a fullscreen shader per materialtype, as opposed to a rasterpass per mesh. Or the default opaque-shader expects a cascaded shadowmap for the directional light, but that shadowmap is only valid in the view frustum of the camera, which becomes problematic for features like raytraced reflections, where a mesh outside of the frustum still needs a valid shadow value. 

While a materialtype could theoretically provide shaders for both Forward+ and deferred rendering pipelines, doing so would require adapting shaders each time the pipeline is modified or a new one is introduced. This means that every change to the rendering pipeline — such as adding or removing a render target in the opaque pass of the MainPipeline — necessitates updates to every opaque shader for every material in every project, including user-generated materials, or existing projects would break.

To solve this issue, O3DE uses Material Pipelines (e.g. `MainPipeline.materialpipeline`) and abstract materialtypes to separate the material shaders from the rendering pipeline to some extend. The Material Pipeline is strictly related to the rendering pipeline, and defines a list of template shaders that are compatible with the render passes of the rendering pipeline. Each of these shaders call specific material functions at specific locations, and the abstract material has to provide these functions. This allows O3DE to generate the actual material shaders needed to render a mesh with any rendering-pipeline.

Note that Material Pipelines do not explicitly specifiy the material functions or the shader structure, and the formal requirements to both the shaders and the abstract materials are fairly minimal: The abstract materialtype specifies two .azsli - files in the `materialShaderDefines` and `materialShaderCode` fields, and these are provided to the shader template with the `MATERIAL_TYPE_DEFINES_AZSLI_FILE_PATH` and `MATERIAL_TYPE_AZSLI_FILE_PATH` respectively. Additionally, the define file referenced in `MATERIAL_PARAMETERS_AZSLI_FILE_PATH` contains a `MaterialParameters` struct, which is genereated from the material shader parameters, although we will gloss over this in this document.

That being said, the abstract material-types provided by the engine (`BasePBR,` `StandardPBR`, etc) and the shader templates of the Material pipelines (`MainPipeline`, `LowEndPipeline`, `MobilePipeline`, etc) do follow a fairly strict interface, which we will describe in this file. And if you want to integrate your own custom material-types into existing rendering-pipelines, or if you want to create a new rendering pipeline that uses the existing materials, we suggest you stick to something very close to this.

The concrete example used in this file:
`MainPipeline.materialpipeline` defines the shader-template `ForwardPass_BaseLighting`, and the `pipelineScript` `MainPipelineScript.lua`.
The `BasePBR.materialtype` defines `lightingModel`: `Base`, `materialShaderDefines`: `BasePBR_Defines.azsli` and `materialShaderCode`: `BasePBR.azsli`. This means the `MainPipelineScript.lua` selects the `ForwardPass_BaseLighting` shader-template, and instantiates an actual shader by combining the `ForwardPass_BaseLighting.azsli` with `BasePBR_Defines.azsli` and `BasePBR.azsli`. That instantiated shader can be found in `<project>/Cache/Intermediate Assets/materials/types/basepbr_mainpipeline_forwardpass_baselighting.azsl`, together with `basepbr_generated.materialtype`.

### Material Interface Basics

- `VsOutput EvaluateVertexGeometry(VsInput IN, VsSystemValues SV, const MaterialParameters params);`
  - Generally called in the vertex-shader, and is supposed to provide the vertices in normalized device coordinates.
  - The structs `VsInput` and `VsOutput` are defined by the shader template, but can be controlled with various defines, e.g. `PIPELINE_VERTEX_POSITION` or `PIPELINE_VERTEX_NORMAL`.
  - The struct `MaterialParameters` is defined by the material.
  - The struct `VsSystemValues` is defined by the shader template, and should be treated as opaque by the material.
- `PixelGeometryData EvaluatePixelGeometry(VsOutput IN, VsSystemValues SV, bool isFrontFace, const MaterialParameters params);`
  - Generally called in the pixel shader, forwards values from the `VsOutput` struct, and constructs the per-pixel tangent frame. 
  - Used to decouple the pipeline-specific Vertex-Data from the more generic  be used to procedurally generate geometry, or to prepare input for a more generic `EvaluateSurface`.
  - The struct `PixelGeometryData` is defined by the material, and opaque to the shader template.
- `Surface EvaluateSurface(VsOutput IN, VsSystemValues SV, PixelGeometryData geoData);`
  - Generally called in the pixel shader, mostly samples textures.
  - The struct `Surface` is defined by the material, but needs to provide a few members, e.g. `position`.
- `LightingData EvaluateLighting(Surface surface, IN.position, ViewSrg::m_worldPosition.xyz);`
  - Generally called in the pixel shader, and is supposed to calculate the final shading.
  - The struct `LightingData` is defined by the material, but needs to provide a few members, e.g. `diffuseLighting`.
  - Defined by the shader template and iterates over the assigned lights and evaluates the shadows.
  - Uses several material-defined functions:
    - `void InitializeLightingData(Surface surface, float3 viewPosition, inout LightingData lightingData);`
    - Expects a `<LightType>Util` - class for each light-type with several functions that are specific to each light type, but mostly boil down to:
      - `static <LightType>Util Init(<LightType> light, Surface surface, float3 cameraPositionWS);`
      - `real GetFalloff();`
        - The Falloff - value is used to skip a light before potentially costly shadow evaluations are performed.
      - `void Apply(<LightType> light, Surface surface, real litRatio, inout LightingData lightingData);`
    - `void FinalizeLightingData(Surface surface, inout LightingData lightingData);`


### Inheritance in HLSL
Inheritance doesn't work in `hlsl`, so you can often see some define magic like this to archieve a similar effect:

in `BasePBR_VertexEval.azsli`
```cpp
#ifndef EvaluateVertexGeometry
#define EvaluateVertexGeometry EvaluateVertexGeometry_BasePBR
#endif

VsOutput EvaluateVertexGeometry_BasePBR() 
...
```

This way we can in e.g. `StandardPBR_VertexEval.azsli` use `#define EvaluateVertexGeometry EvaluateVertexGeometry_StandardPBR` before we including the BasePBR - file,
which allows us to call `EvaluateVertexGeometry_BasePBR()` from within the `EvaluateVertexGeometry_StandardPBR()` function, in addition to our own shader code. 
And in the rest of the shader code we can call `EvaluateVertexGeometry()` and don't need to pay attention to what function is actually called there.

### ForwardPass_BaseLighting.azsli as a specific example for the Material Interface

`Gems/Atom/Feature/Common/Assets/Materials/Pipelines/MainPipeline/ForwardPass_BaseLighting.azsli` with `BasePBR_Defines.azsli` and `BasePBR.azsli`.

```cpp
#pragma once

#include <Atom/Features/SrgSemantics.azsli>

// Defines tell the material shader code what the current shader can do. 
// e.g. the depth-shader (without custom z) has no pixel shader stage, so the material-shader only needs to define 'EvaluatePixelGeometry()' and related structs.
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
// This is declared in the file 'BasePBR.materialtype', specifically in the field "materialShaderDefines", which has the value "BasePBR_Defines.azsli" in this example.
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
// Usually these are declared by including DefaultDrawSrg.azsli and DefaultObjectSrg.azsli, but if you are using a custom SRG, at least the same fields should be present 
// since they are filled by the feature processors during creation of the draw items.
// Also keep in mind that a deferred or raytracing pipeline can't have a Object or Draw SRG, meaning the relevant defines will be set to 0, and the information
// has to be provided in a different way.
// 
// struct MaterialParameters{}; 
// Parameter Inputs for the Material. This is generally generated by the MaterialPipeline, and works by using '#include MATERIAL_PARAMETERS_AZSLI_FILE_PATH'.
// If you are defining this struct manually you cannot use the SceneMaterialSrg, since then the Layout of the struct is not known in C++ and the parameters cannot be stored in a structured buffer.
// 
// ShaderResourceGroup MaterialSrg : SRG_PerMaterial {};
// The MaterialSrg is fairly strict, and usually its the easiest to use '#include <Atom/Features/Materials/MaterialSrg.azsli>', which uses the MaterialParameters struct.
// While you can use a custom MaterialSrg, you face the same limitations as with the MaterialParameters struct above, in that you cannot use the SceneMaterialSrg.
// 
// The SceneMaterialSrg is required if you want to access more than one set of material-parameters at once, e.g. in a deferred pipeline.
// 
// These are usually not fully defined by the material, but are instead controlled by several defines, see 'BasePBR_VertexData.azsli' in conjunction with 'ForwardPassVertexData.azsli'.
// struct VsInput{};            // inputs to the Vertex-Shader
// struct VsOutput{};           // Output of the Vertex-Shader, input to the Pixel-Shader
// struct VsSystemValues{};     // Values specific to the rendering-pipeline (e.g. instanceId for Forward+), usually treated as an opaque struct that is passed to various utility functions, and never declared by the material directly.
//   Note that this is a bit complicated, since e.g. the InstanceId needs to be passed from the Vertex-Shader to the Pixel-Shader, and therefore needs to be part of the VsOutput struct in a Forward+ - pipeline only.

// struct PixelGeometryData{}; // Converts the VsOutput to input for the Surface-Data; 
                               // Ideally this does parallax handling and Alpha-clipping, so the custom-z pass can stop after this. 
                               // In practice Alpha-clipping happens with the Surface. (TODO!)
// See 'BasePBR_PixelGeometryData.azsli' 

// struct SurfaceData{};       // Contains all the surface data needed to evaluate the lighting on a pixel. 
                               // Generally textures are sampled when creating this structure
// See 'BasePBR_SurfaceData.azsli' 

// struct LightingData{};      // Contains Input and Output for the actual lighting evaluation.
                               // The Lighting-Eval functions use this during the evaluation of the separate light types, and  
                               // the shader converts this to the actual output of the Pixel Shader (which depends on the Rendering Pipeline).
                               // This means the struct needs at least the members these functions and shaders expect
// See 'BasePBR_LightingData.azsli'

// -------------------------------------------------------------------------------------------------
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////

#include <viewsrg_all.srgi>  // collects the 'partial ShaderResourceGroup ViewSrg {};'
#include <scenesrg_all.srgi> // collects the 'partial ShaderResourceGroup SceneSrg {};'
#include <Atom/Features/Pipeline/Forward/ForwardPassSrg.azsli>  // defines the 'ShaderResourceGroup PassSrg : SRG_PerPass {};' as suitable for the Forward-Pass in the MainPipeline
#include <Atom/Features/Pipeline/Forward/ForwardPassVertexData.azsli>  // uses the defines created by the material and actually declares the struct VsInput, VsOutput and VsSystemValues
#include <Atom/Features/Pipeline/Forward/ForwardPassPipelineCallbacks.azsli> // Wrapper - functions to access the DrawSrg / ObjectSrg / MaterialSrg / etc.
                                                                             // Currently the following functions are defined:
                                                                             // 'uint GetLightingChannelMask(VsSystemValues SV);'
                                                                             // 'float4x4 GetObjectToWorldMatrix(VsSystemValues SV);'
                                                                             // 'float3x3 GetObjectToWorldMatrixInverseTranspose(VsSystemValues SV);'
                                                                             // 'float4x4 GetViewProjectionMatrix(VsSystemValues SV);'
                                                                             // 'bool HasGoboTexture(int index);'
                                                                             // 'Texture2D<float> GetGoboTexture(int index);'
                                                                             // when MATERIALPIPELINE_USES_PREV_VERTEX_POSITION is defined
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
//     Called in the Vertex-Shader (in the Forward+ pipeline at least), and converts VsInput into VsOutput, i.e. does the ViewProjection.
//     If the material doesn't demand the Custom-Z shader, only this function that will be called for the DepthPass.
// 
// - 'PixelGeometryData EvaluatePixelGeometry(VsOutput IN, VsSystemValues SV, bool isFrontFace, const MaterialParameters params);'
//     Called in the Pixel-Shader, and converts VsOutput IN into PixelGeometryData.
//     TODO: make alpha-clipping part of this, so we only need to call this for the CustomZ - passes. Otherwise this can be folded into the surface
//
// - 'Surface EvaluateSurface(VsOutput IN, PixelGeometryData geoData, const MaterialParameters params);'
//     Called in the Pixel-Shader, and converts PixelGeometryData to a surface. 
//     Most texture sampling happens in this functions, and the materials generally evaluate most of the MaterialParameters (and shader-options) here.
//     For convenience, the MaterialInputs should also be used here, e.g. 'BaseColorInput.azsli' provides 'GetBaseColorInput()' and 'BlendBaseColor()', which can be used together 
//     with the shader-options defined with 'COMMON_OPTIONS_BASE_COLOR', and it uses the Material-Parameters defined in 'BaseColorPropertyGroup.json'.
//     Note that the 'COMMON_SRG_INPUTS_BASE_COLOR' are obsolete; They were used to manually create the MaterialSRG before the MaterialParameters - struct existed.
// 
// - 'real3 GetDiffuseLighting(Surface surface, LightingData lightingData, real3 lightIntensity, real3 dirToLight);'
// - 'real3 GetSpecularLighting(Surface surface, LightingData lightingData, const real3 lightIntensity, const real3 dirToLight);'
//     Called during the (default) Lighting evaluation, and allows the material to provide its own BRDF - function:
// 
// - 'void InitializeLightingData(inout Surface surface, float3 viewPosition, inout LightingData lightingData);'
// - 'void FinalizeLightingData(Surface surface, inout LightingData lightingData);'
//     Called before and after the Lighting evaluation.
//
// - class CapsuleLightUtil 
//   {
//     static bool CheckLightChannel(CapsuleLight light, LightingData lightingData);
//     static CapsuleLightUtil Init(CapsuleLight light, Surface surface, float3 cameraPositionWS);
//     real GetFalloff();
//     void Apply(CapsuleLight light, Surface surface, [real litRatio], inout LightingData lightingData);
//     static void ApplySampled(CapsuleLight light, Surface surface, inout LightingData lightingData, const uint sampleCount); // optional
//   };
//     Used during the Lighting evaluation for each light type (Capsule, Directional, Disk, Point, Polygon, Quad, SimplePoint, SimpleSpot).
//     If you want to use the default PBR implementation, simply include "Atom/Features/PBR/Lights/Lights.azsli", which provides a default
//     <Type>LightUtil class for each supported light type.
//
// -------------------------------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////////////////////////

// Lighting eval functions from the forward-pass that use functions and classes defined by the material
#include <Atom/Features/Pipeline/Forward/ForwardPassEvaluateLighting.azsli>
// - 'EvaluateLighting(Surface surface, float4 screenPosition, float3 viewPosition);'
//     This function is specific to the Forward+ - Pipeline, and uses the TileLightingIterator from the PassSrg to iterate over each assigned light,
//     evaluates the shadows (if applicable), and uses the '<Type>LightUtil' - class to Apply the light to the surface (or skip it, if possible)

// The actual Vertex and Pixel shader code:
#include "ForwardPassVertexAndPixel.azsli"
// The Vertex Shader calls 
//    'VsOutput OUT = EvaluateVertexGeometry(VsInput IN, VsSystemValues SV, MaterialParameters params);'
// The Pixel Shader then calls 
//    'PixelGeometryData geoData = EvaluatePixelGeometry(VsOutput IN, VsSystemValues SV, bool isFrontFace, MaterialParameters params);'
//    'Surface surface = EvaluateSurface(VsOutput IN, PixelGeometryData geoData, MaterialParameters paramn);'
//    'LightingData lightingData = EvaluateLighting(Surface surface, float4 IN.position, float3 ViewSrg::m_worldPosition.xyz);'
// and then converts the 'lightingData' into whatever output the current pass requires.
```
