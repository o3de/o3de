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


Demo Showcase
=============
MeshExampleComponent will generate a secondary meshlet model that will be displayed
along side the original one with UVs grouped by meshlet and can be associated with 
color - this is done by using the material DebugShaderMaterial_01 based on the 
shader DebugShaderPBR_ForwardPass - all copied to this folder.
 
 
Quick Build and Run Direction
=============================
1. Copy the files in this folder over the original ASV files
2. Run cmake again
3. Compile meshoptimizer.lib as static library and add it to the Meshlets gem
3. Compile in VS
4. Run the standalone ASV and choose the MeshExampleComponent
5. When selecting a large enough model, a secondary meshlet model will be generated
and displayed along side.


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

