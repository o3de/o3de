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
MeshExampleComponent will generate two meshlet models that will be displayed
along side the original one with UVs grouped by meshlet and can be associated with 
color.
The fisrt colored meshlets model is the reference model done by generating the meshlets 
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


Moving Forward
==============
- Create and prepare culling data compute culling data
- Create a new compute stage that will cull the meshlets based on one of the following:
    - Previous meshlets reprojected Z (requires pre-pass and adding delta objects)
    - Previous Hi-Z (cracks can occure - needs to compensate)
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
    these two files were added to this folder as a reference.

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

