#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

#set(tressfx_sources
set(FILES
    External/Code/src/Math/Vector3D.h
    External/Code/src/Math/Vector3D.cpp
    External/Code/src/Math/Transform.h
    External/Code/src/Math/Transform.cpp
    External/Code/src/Math/Quaternion.h
    External/Code/src/Math/Quaternion.cpp
    External/Code/src/Math/Matrix33.h
    External/Code/src/Math/Matrix33.cpp

    External/Code/src/TressFX/AMD_Types.h
    External/Code/src/TressFX/AMD_TressFX.h
    External/Code/src/TressFX/TressFXCommon.h
    External/Code/src/TressFX/TressFXAsset.h
    External/Code/src/TressFX/TressFXAsset.cpp
    External/Code/src/TressFX/TressFXConstantBuffers.h
    External/Code/src/TressFX/TressFXFileFormat.h
    External/Code/src/TressFX/TressFXSettings.h
    External/Code/src/TressFX/TressFXSettings.cpp
#   External/Code/src/Math/Quaternion.h
#)
#
#set(atom_hair_sources
    Code/HairModule.h
    Code/Rendering/HairCommon.h
    Code/Rendering/HairCommon.cpp
    Code/Rendering/HairDispatchItem.h
    Code/Rendering/HairDispatchItem.cpp
    Code/Rendering/HairFeatureProcessor.cpp
    Code/Rendering/HairFeatureProcessor.h
    Code/Rendering/HairRenderObject.cpp
    Code/Rendering/HairRenderObject.h
    Code/Rendering/SharedBuffer.cpp
    Code/Rendering/SharedBuffer.h
    Code/Rendering/HairLightingModels.h
    Code/Rendering/HairGlobalSettings.h
    Code/Rendering/HairGlobalSettings.cpp
    Code/Rendering/HairGlobalSettingsBus.h
#)
#
#set(atom_hair_components
    Code/Components/HairSystemComponent.h
    Code/Components/HairSystemComponent.cpp
    Code/Components/EditorHairComponent.h
    Code/Components/EditorHairComponent.cpp
    Code/Components/HairComponent.h
    Code/Components/HairComponent.cpp
    Code/Components/HairComponentController.h
    Code/Components/HairComponentController.cpp
    Code/Components/HairComponentConfig.h
    Code/Components/HairComponentConfig.cpp
#)
#
#set(atom_hair_passes
    # The simulation pass class shared by all simulation / skinning compute passes
    Code/Passes/HairSkinningComputePass.h
    Code/Passes/HairSkinningComputePass.cpp
    # Base class of all geometry raster passes
    Code/Passes/HairGeometryRasterPass.h
    Code/Passes/HairGeometryRasterPass.cpp

    # ShortCut rendering technique - pass classes 
    Code/Passes/HairShortCutGeometryDepthAlphaPass.h
    Code/Passes/HairShortCutGeometryDepthAlphaPass.cpp
    Code/Passes/HairShortCutGeometryShadingPass.h
    Code/Passes/HairShortCutGeometryShadingPass.cpp

    # PPLL rendering technique - geometry raster pass
    Code/Passes/HairPPLLRasterPass.h
    Code/Passes/HairPPLLRasterPass.cpp
    # PP full screen resolve pass 
    Code/Passes/HairPPLLResolvePass.h
    Code/Passes/HairPPLLResolvePass.cpp
#)
#
#set(atom_hair_interfaces
    Code/Rendering/HairBuffersSemantics.h
    Code/Rendering/HairSharedBufferInterface.h
    Code/Components/HairBus.h
    
    Code/Assets/HairAsset.h
    Code/Assets/HairAsset.cpp
#)
#set(shaders_sources
    # Geometry and Full Screen azsl utility files
    Assets/Shaders/HairRenderingSrgs.azsli
    Assets/Shaders/HairStrands.azsli
    Assets/Shaders/HairUtilities.azsli
    Assets/Shaders/HairFullScreenUtils.azsli
    Assets/Shaders/HairLighting.azsli    
    Assets/Shaders/HairLightingEquations.azsli    
    Assets/Shaders/HairLightTypes.azsli
    Assets/Shaders/HairSurface.azsli

    # ShortCut technique shaders (using multiple RTs instead of PPLL for GPU memory reduction) 
    Assets/Shaders/HairShortCutGeometryDepthAlpha.azsl
    Assets/Shaders/HairShortCutResolveDepth.azsl
    Assets/Shaders/HairShortCutGeometryShading.azsl
    Assets/Shaders/HairShortCutResolveColor.azsl

    # Rendering azsl files
    Assets/Shaders/HairRenderingFillPPLL.azsl
    Assets/Shaders/HairRenderingResolvePPLL.azsl  

    # Simulation Compute azsl files
    Assets/Shaders/HairComputeSrgs.azsli
    Assets/Shaders/HairSimulationComputeSrgs.azsli
    Assets/Shaders/HairSimulationCommon.azsli
    Assets/Shaders/HairSimulationCompute.azsl   
    
    # Collision azsl files - to be included soon
#    Assets/Shaders/HairCollisionPrepareSDF.azsl
#    Assets/Shaders/HairCollisionWithSDF.azsl

    # Simulation Compute .shader files
    Assets/Shaders/HairGlobalShapeConstraintsCompute.shader  
    Assets/Shaders/HairCalculateStrandLevelDataCompute.shader
    Assets/Shaders/HairVelocityShockPropagationCompute.shader
    Assets/Shaders/HairLocalShapeConstraintsCompute.shader
    Assets/Shaders/HairLengthConstraintsWindAndCollisionCompute.shader  
    Assets/Shaders/HairUpdateFollowHairCompute.shader
    
    # PPLL Render .shader file
    Assets/Shaders/HairRenderingFillPPLL.shader
    Assets/Shaders/HairRenderingResolvePPLL.shader

    # ShortCut Render .shader file
    Assets/Shaders/HairShortCutGeometryDepthAlpha.shader
    Assets/Shaders/HairShortCutResolveDepth.shader
    Assets/Shaders/HairShortCutGeometryShading.shader
    Assets/Shaders/HairShortCutResolveColor.shader
    
    # Colisions .shader files - to be included soon
#    Assets/Shaders/HairCollisionInitializeSDF.shader 
#    Assets/Shaders/HairCollisionConstructSDF.shader
#    Assets/Shaders/HairCollisionFinalizeSDF.shader
#    Assets/Shaders/HairCollisionWithSDF.shader 
#)
#
#set(atom_hair_passes
    # Compute simulation and skinning passes
    Assets/Passes/HairParentPass.pass
    Assets/Passes/HairParentShortCutPass.pass
    Assets/Passes/HairGlobalShapeConstraintsCompute.pass
    Assets/Passes/HairCalculateStrandLevelDataCompute.pass
    Assets/Passes/HairVelocityShockPropagationCompute.pass
    Assets/Passes/HairLocalShapeConstraintsCompute.pass
    Assets/Passes/HairLengthConstraintsWindAndCollisionCompute.pass
    Assets/Passes/HairUpdateFollowHairCompute.pass

    # PPLL render passes
    Assets/Passes/HairFillPPLL.pass
    Assets/Passes/HairResolvePPLL.pass

    # Shortcut render passes
    Assets/Passes/HairShortCutGeometryDepthAlpha.pass
    Assets/Passes/HairShortCutResolveDepth.pass
    Assets/Passes/HairShortCutGeometryShading.pass
    Assets/Passes/HairShortCutResolveColor.pass
)

set(SKIP_UNITY_BUILD_INCLUSION_FILES
#Add files that are in the shared here.
)

#include_directories(src src/TressFX)
#add_subdirectory(src)

source_group("Math Sources"             FILES ${math_sources})
source_group("TressFX Sources"          FILES ${tressfx_sources})
source_group("Atom Hair Sources"        FILES ${atom_hair_sources})
source_group("Atom Hair Components"     FILES ${atom_hair_components})
source_group("Atom Hair Passes"         FILES ${atom_hair_passes})
source_group("Atom Hair Interfaces"     FILES ${atom_hair_interfaces})
source_group("Common Sources"           FILES ${common_sources})
source_group("Shader Sources"           FILES ${shaders_sources})