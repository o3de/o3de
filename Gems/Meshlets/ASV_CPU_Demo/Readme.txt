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


Overview
========
This gem demonstrates the creation of meshlets as a way to generate mesh render on the fly.
The example component (via ASV) implements meshlets reference mesh creation, rendered via the regular mesh feature processor, and a second identical mesh generated using meshlet compute pass followed by meshlets direct draw pass - both managed by a new dedicated feature processor.
The POC is using the external mesh optimizer open source library (https://github.com/zeux/meshoptimizer) to generate the meshlets data, and demonstrates how to reconstruct and render the original mesh based on the added meshlets data.
It is created as a Gem, with added directories under the Gem including files demonstrating how to attach it and add an ASV sample.

The POC is a first step toward possible GPU driven pipeline ala Epic Nanite / Ubisoft:
To fulfill this direction the following steps will be required:

Turn the registration and GPU work into indirect dispatch and draw (to eliminate the CPU per frame side - should be easy)
Add culling data (easy - already supported by the external library) and add to the meshlets data
Populate frame required meshes on the GPU
Add culling pre-pass (Hi-Z generation per previous meshlets / largest occluders / other.. will be a large time saver)
Enhance the Compute indices generation and the indirect draw to use the visible data (this is where the GPU gain lies)
Replace demo shader by proper lighting and shading shader

Intent
======
This POC intent is not to replace the existing render pipeline. The provided gem is a POC towards a GPU driven render pipeline and is an opportunity for the O3DE community to take this gem as reference and enhance.
Removing the CPU from the equation is considered by many the path to the future of rendering, and using meshlets techniques increases performance as they eliminate the majority of the invisible meshes in the scene that otherwise would have sent to render due to partial visibility of the meshes. All in all this technique represents a much more optimal rendering approach, one that was successfully used by Ubisoft and is a large part of Nanite today.
This approach is parallel to the mesh render pipeline that exists in Atom today and is an opportunity for O3DE community to look into and hopefully advance in the future.
In no way it comes to replace the existing mesh render pipeline in Atom that is state of the art and coming to maturity.

References:
===========
Ubisoft - Siggraph 2015: https://advances.realtimerendering.com/s2015/aaltonenhaar_siggraph2015_combined_final_footer_220dpi.pdf

Meshlets Intro - NVidia: https://developer.nvidia.com/blog/introduction-turing-mesh-shaders/

Cluster Culling 2016: https://gpuopen.com/learn/geometryfx-1-2-cluster-culling/

Epic Nanite 2021: https://quip-amazon.com/-/blob/RKb9AAL3zt8/G7A6Cxor0gvn_WPGltB-8g?name=Karis_Nanite_SIGGRAPH_Advances_2021_final.pdf&s=Gn3dAhbv9gN6

Activision - Geometry Rendering Pipeline architecture 2021: https://research.activision.com/publications/2021/09/geometry-rendering-pipeline-architecture


Demo Showcase
=============
MeshExampleComponent will generate a secondary meshlet model that will be displayed
along side the original one with UVs grouped by meshlet and can be associated with 
color - this is done by using the material DebugShaderMaterial_01 based on the 
shader DebugShaderPBR_ForwardPass - all copied to this folder.
 

Quick Build and Run Direction
=============================
1. Copy the files in this folder over the original files
2. Add the gem directory to the file engine.json:
    "external_subdirectories": [
        "Gems/Meshlets",
        ...
3. Run cmake again
4. Compile meshoptimizer.lib as static library and add it to the Meshlets gem
5. Build the ASV solution
6. Run the standalone ASV and choose the Meshlets demo from the RPI demos sub-menu 
7. When selecting a model, a secondary CPU meshlet model will be generated and 
    displayed alongside, and in the GPU demo, a third one created by the GPU will 
    be displayed as well.


Adding the meshoptimizer library to the Gem
===========================================
The meshoptimizer library is not included as part of the O3DE.
When adding and compiling this Gem, compile the meshoptimizer 
library and add it as part of Meshlets.Static project (for example, in 
Visual Studio you simply add the library file to the project).
Once this is done, the Gem should compile and link properly to allow you 
to run the ASV sample 'Meshlets' (created with the MeshletsExampleComponent)

The meshoptimizer library and source can be found in the following Github link:
https://github.com/zeux/meshoptimizer


Connecting the Gem to the project folder (AtomSamplesViewer for example):
=========================================================================
1. Add Meshlets Shader Assets directories to the file 
    <project_folder>/config/shader_global_build_options.json
    
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

2. You can choose to add Meshlets Assets in 
    <project_folder>/Registry/assets_scan_folders.setreg
            "Meshlets":
            {
                "SourcePaths":
                [
                    "Gems/Meshlets/Assets"
                ]
            }
    Remark: this is not required in this case as Meshlets can process regular Atom meshes

3. Enable Meshlets gem in <project_folder>/Gem/code/enabled_gems.cmake
            (
                set(ENABLED_GEMS
                ...
                Meshlets
            )
            
4. Add the current passes to both MainPipeline.pass and LowEndPipeline.pass - 
    these two pass files were added to this folder as a reference how to add the meshlets passes.

5. Add the meshoptimizer.lib compiled library (under Gems\Meshlets\External\Lib) 
    to your Meshlets.static project.


Including the Meshlets sample in ASV
====================================
1. Add the following two files under the directory [O3de dir]\AtomSampleViewer\Gem\Code\Source
    MeshletsExampleComponent.h
    MeshletsExampleComponent.cpp

2. Alter SampleComponentManager.cpp to include the following lines:
    In the header files section:
    #include <MeshletsExampleComponent.h>

    In method SampleComponentManager::GetSamples()
                NewRPISample<MeshletsExampleComponent>("Meshlets"),
            
3. Add the source files to the make file 'atomsampleviewergem_private_files.cmake'

