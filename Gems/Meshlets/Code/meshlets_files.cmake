#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    Include/Meshlets/MeshletsBus.h
    Source/MeshletsModuleInterface.h
    Source/MeshletsSystemComponent.cpp
    Source/MeshletsSystemComponent.h

    ../Assets/Passes/MeshletsPassTemplates.azasset
    ../Assets/Passes/MeshletsPassRequest.azasset
    ../Assets/Passes/MeshletsParent.pass
    ../Assets/Passes/MeshletsCompute.pass
    ../Assets/Passes/MeshletsRender.pass
    
    ../Assets/Shaders/MeshletsCompute.shader
    ../Assets/Shaders/MeshletsCompute.azsl
    ../Assets/Shaders/MeshletsDebugRenderShader.shader
    ../Assets/Shaders/MeshletsDebugRenderShader.azsl
    ../Assets/Shaders/MeshletsPerObjectRenderSrg.azsli
    
    Source/Meshlets/MeshletsRenderPass.h
    Source/Meshlets/MeshletsRenderPass.cpp
    Source/Meshlets/MultiDispatchComputePass.h
    Source/Meshlets/MultiDispatchComputePass.cpp
    Source/Meshlets/MeshletsData.h
    Source/Meshlets/MeshletsDispatchItem.h
    Source/Meshlets/MeshletsDispatchItem.cpp
    Source/Meshlets/SharedBufferInterface.h
    Source/Meshlets/MeshletsUtilities.h
    Source/Meshlets/MeshletsUtilities.cpp
    Source/Meshlets/SharedBuffer.h
    Source/Meshlets/SharedBuffer.cpp
    Source/Meshlets/MeshletsRenderObject.h
    Source/Meshlets/MeshletsRenderObject.cpp
    
    Source/Meshlets/MeshletsFeatureProcessor.h
    Source/Meshlets/MeshletsFeatureProcessor.cpp
    
    Source/Meshlets/MeshletsAssets.h
    Source/Meshlets/MeshletsAssets.cpp
)
