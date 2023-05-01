/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

Atom Meshlets Gem POC
=====================

Author: Adi Bar-Lev, May 2022
Contributors: 
Dependencies: meshlets generation is done by an excellent open source library - 
    the MeshOptimizer library: https://github.com/zeux/meshoptimizer


Demo Show Case
--------------
MeshletsExampleComponent will generate two meshlet models that will be displayed
along side the original one with UVs grouped by meshlet and can be associated with 
color.
The first colored meshlets model is the reference model done by generating the meshlets 
in the CPU at run time model load and by using the material DebugShaderMaterial_01 based on the 
shader DebugShaderPBR_ForwardPass - all copied to this folder.
The second Meshlets model is using the meshlets data from the CPU and processing it 
on the fly using dispatch compute that creates the index buffer and the UV coloring - this
is then sent to a direct draw render that will displays it.
The latest is the POC that can be enhance to become a parallel GPU driven pipeline.

Short Intro
===========
The Meshlets Gem is a POC for rendering meshes solely on the GPU using 
meshlets / mesh clusters.
This is done to promote maximum parallelism in processing relative small pieces 
of the mesh and gain performance by doind fast culling that due to the size and locality
of these clusters is much more efficient than culling the entire mesh.
The work is inspired by Ubisoft 2015 presentation and recently Nanity by Epic.
The current POC does not do the indirect dispatch and render and lacks the culling 
stage - these are the main two steps missing to make it beyond a POC.

What does it do?
================
- Splits the mesh and creates the data structures requires for meshlets
- Prepares the GPU, buffers and data required by the GPU for such render pipeline
- Dispatch Compute groups that generates the index buffer and debug color UVs
- Direct draw the output of the compute using direct buffer access for the vertex streams

Known Bug:
==========
- In the latest version there seem to be an occasional missing meshlet group, hence creating 
    an area with missing polygons, this might be a very simple counting bug. Will be fixed in
    future versions.

Moving Forward
==============
- Create and prepare culling data compute culling data
- Create a new compute stage that will cull the meshlets based on one of the following:
    - Previous meshlets reprojected Z (requires pre-pass and adding delta objects)
    - Previous Hi-Z (cracks can occur - needs to compensate)
    - Best occluders
    - other..
- Based on the culling create new indexing table (into the index buffer) for the visible meshlets
- Run the current meshlet compute on visible meshlets only 
    - Calculate the indices and populate the index buffer
- Add actual PBR render connected to Atom's lighting
- Change the mesh loader and processing as an AssetProcessor builder job to be done offline

Remark: The POC seem to still have a small bug - in some meshes, small amount of polygons 
    might not render. Due to the selective nature of how it looks, it might be as simple 
    as missing a meshlet group.

Intent
======
This POC intent is not to replace the existing render pipeline. The provided gem is a POC towards a GPU 
driven render pipeline and is an opportunity for the O3DE community to take this gem as reference and enhance.
Removing the CPU from the equation is considered by many the path to the future of rendering, and using 
meshlets techniques increases performance as they eliminate the majority of the invisible meshes in the scene 
that otherwise would have sent to render due to partial visibility of the meshes. All in all this technique 
represents a much more optimal rendering approach, one that was successfully used by Ubisoft and is a large part of Nanite today.
This approach is parallel to the mesh render pipeline that exists in Atom today and is an opportunity for 
O3DE community to look into and hopefully advance in the future.
In no way it comes to replace the existing mesh render pipeline in Atom that is state of the art and coming to maturity.

References:
===========
Ubisoft - Siggraph 2015: https://advances.realtimerendering.com/s2015/aaltonenhaar_siggraph2015_combined_final_footer_220dpi.pdf

Meshlets Intro - NVidia: https://developer.nvidia.com/blog/introduction-turing-mesh-shaders/

Cluster Culling 2016: https://gpuopen.com/learn/geometryfx-1-2-cluster-culling/

Epic Nanite 2021: https://quip-amazon.com/-/blob/RKb9AAL3zt8/G7A6Cxor0gvn_WPGltB-8g?name=Karis_Nanite_SIGGRAPH_Advances_2021_final.pdf&s=Gn3dAhbv9gN6

Activision - Geometry Rendering Pipeline architecture 2021: https://research.activision.com/publications/2021/09/geometry-rendering-pipeline-architecture
 

Quick Build and Run Directions
==============================
This is a summary of the steps you'll need to follow, see the additional sections below for details.

1. Go over the files in this directory - you'd need to follow the instructions below and copy 
    the files to various locations. Create a Meshlets branch so you'd do it once only.
2. Add the gem directory to the file engine.json:
    "external_subdirectories": [
        "Gems/Meshlets",
        ...
3. Compile meshoptimizer.lib as static library and add it to the Meshlets gem
4. Update several build configuration files to connect the gem to AtomSampleViewer (ASV)
5. Run cmake to apply the configuration changes
6. Update ASV code to add the Meshlets sample
7. Build the ASV standalone project
8. Run the standalone ASV and choose the Meshlets demo from the RPI demos sub-menu 
9. When selecting a model, a secondary CPU meshlet model will be generated and 
    displayed alongside, and in the GPU demo, a third one created by the GPU will 
    be displayed as well.


Adding the meshoptimizer library to the Gem
===========================================
The meshoptimizer library is not included as part of the O3DE.
When adding and compiling this Gem, compile the meshoptimizer 
library and add it as part of Meshlets.Static project.
Once this is done, the Gem should compile and link properly to allow you 
to run the ASV sample 'Meshlets' (created with the MeshletsExampleComponent).

The meshoptimizer library and source can be found in the following Github link:
https://github.com/zeux/meshoptimizer

Here are the specific steps to build and install the library, on Windows:
git clone https://github.com/zeux/meshoptimizer
cd meshoptimizer
mkdir build
cmake . -B build
cmake --build build --config Release
copy build\Release\meshoptimizer.lib [YourO3DEPath]\Gems\Meshlets\External\Lib

Finally, in Visual Studio, right-click the Meshlets.Static project 
    and click Add > Existing Item..., then select 
    [YourO3DEPath]\Gems\Meshlets\External\Lib\meshoptimizer.lib
    (Note that if you run cmake in O3DE again, this setting will be lost and you
    will have to add meshoptimizer.lib to the Meshlets.Static project again).

Connecting the Gem to the project folder (AtomSamplesViewer for example):
=========================================================================
In the following example I will use AtomSampleViewer as the active project. If this 
is not the case, simply replace with the directory name of your active project.
  
1. Add Meshlets Shader Assets directories to the file 
    AtomSampleViewer/config/shader_global_build_options.json
    
            "PreprocessorOptions" : {
            "predefinedMacros": ["AZSL=17"],
                // The root of the current project is always the first include path.
                // These include paths are already part of the automatic include folders list.
                // By specifying them here, we are boosting the priority of these folders above all the other automatic include folders.
                // (This is not necessary for the project, but just shown as a usage example.)
            "projectIncludePaths": [
                "Gems/Atom/RPI/Assets/ShaderLib",
                "Gems/Atom/Feature/Common/Assets/ShaderLib",
                "Gems/Meshlets/Assets/Shaders"
            ]

2. Not required: you can choose to add Meshlets Assets in 
    AtomSampleViewer/Registry/assets_scan_folders.setreg
            "Meshlets":
            {
                "SourcePaths":
                [
                    "Gems/Meshlets/Assets"
                ]
            }
    Remark: this is NOT required in this case as Meshlets can process regular Atom meshes

3. Enable Meshlets gem for the active project - AtomSampleViewer/project.json
            "gem_names": [
                ...
                "Meshlets"
            ]

4. Add a build dependency on the meshlets gem - AtomSampleViewer/Gem/Code/CMakeLists.txt
        ly_add_target(
            NAME AtomSampleViewer.Private.Static STATIC
            ...
            BUILD_DEPENDENCIES
                PUBLIC
                    Gem::Meshlets.Static

5. Copy the shader debug files (DebugShaderPBR_ForwardPass.*) to the material types directory:
    [o3de]\AtomSampleViewer\Materials\Types\*

6. Copy the material type file 'DebugShaderPBR.materialtype' to the material types directory:
    [o3de]\AtomSampleViewer\Materials\DebugShaderPBR.materialtype
    
7. Copy the material file 'debugshadermaterial_01.material' to the material directory:
    [o3de]\AtomSampleViewer\Materials\DebugShaderMaterial_01.material
        


Including the Meshlets sample in ASV
====================================
1. Add the following two files to the directory [o3de]\AtomSampleViewer\Gem\Code\Source
    MeshletsExampleComponent.h
    MeshletsExampleComponent.cpp

2. Alter SampleComponentManager.cpp to include the following lines:
    Add the following line to the header files section:
        #include <MeshletsExampleComponent.h>

    Add the following line to the method SampleComponentManager::GetSamples()
        NewRPISample<MeshletsExampleComponent>("Meshlets"),
        
    Set the pipeline descriptor to allow pass injection:
        pipelineDesc.m_allowModification = true;
            
3. Add the source files to the make file 'atomsampleviewergem_private_files.cmake'
    set(FILES
        Source/MeshletsExampleComponent.cpp
        Source/MeshletsExampleComponent.h
        ...
