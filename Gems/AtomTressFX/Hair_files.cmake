#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
    Code/Passes/HairSkinningComputePass.h
    Code/Passes/HairSkinningComputePass.cpp
    Code/Passes/HairPPLLRasterPass.h
    Code/Passes/HairPPLLRasterPass.cpp
    Code/Passes/HairPPLLResolvePass.h
    Code/Passes/HairPPLLResolvePass.cpp
#)
#
#set(atom_hair_interfaces
    Code/Rendering/HairBuffersSemantics.h
    Code/Rendering/SharedBufferInterface.h
    Code/Rendering/HairRenderObjectInterface.h
    Code/Rendering/HairFeatureProcessorBus.h
    Code/Components/HairBus.h
    
    Code/Assets/HairAsset.h
    Code/Assets/HairAsset.cpp
#)
#set(shaders_sources
    # Srgs and Utility files
    Assets/Shaders/HairSrgs.azsli
    Assets/Shaders/HairSimulationSrgs.azsli
    Assets/Shaders/HairRenderingSrgs.azsli
    Assets/Shaders/HairSimulationCommon.azsli
    Assets/Shaders/HairStrands.azsli
    Assets/Shaders/HairUtilities.azsli
    Assets/Shaders/HairLighting.azsli    
    Assets/Shaders/HairLightingEquations.azsli    
    Assets/Shaders/HairLightTypes.azsli
    Assets/Shaders/HairSurface.azsli

    # Simulation Compute shaders
    Assets/Shaders/HairSimulation.azsl          # Skinning only for testing without physics 
    Assets/Shaders/HairSimulationCompute.azsl   
    
    # Collision shaders - to be included soon
#    Assets/Shaders/HairCollisionPrepareSDF.azsl
#    Assets/Shaders/HairCollisionWithSDF.azsl

    # Rendering shaders
    Assets/Shaders/HairRenderingFillPPLL.azsl
    Assets/Shaders/HairRenderingResolvePPLL.azsl  

    # Simulation .shader files
    Assets/Shaders/HairSkinningCompute.shader   # Skinning only for testing without physics 
    Assets/Shaders/HairGlobalShapeConstraintsCompute.shader  
    Assets/Shaders/HairCalculateStrandLevelDataCompute.shader
    Assets/Shaders/HairVelocityShockPropagationCompute.shader
    Assets/Shaders/HairLocalShapeConstraintsCompute.shader
    Assets/Shaders/HairLengthConstraintsWindAndCollisionCompute.shader  
    Assets/Shaders/HairUpdateFollowHairCompute.shader
    
    # Rendering .shader file
    Assets/Shaders/HairRenderingFillPPLL.shader
    Assets/Shaders/HairRenderingResolvePPLL.shader
    
    # Colisions .shader files - to be included soon
#    Assets/Shaders/HairCollisionInitializeSDF.shader 
#    Assets/Shaders/HairCollisionConstructSDF.shader
#    Assets/Shaders/HairCollisionFinalizeSDF.shader
#    Assets/Shaders/HairCollisionWithSDF.shader 
#)
#
#set(atom_hair_passes
    Assets/Passes/HairParentPass.pass
    Assets/Passes/HairSkinningCompute.pass
    Assets/Passes/HairGlobalShapeConstraintsCompute.pass
    Assets/Passes/HairCalculateStrandLevelDataCompute.pass
    Assets/Passes/HairVelocityShockPropagationCompute.pass
    Assets/Passes/HairLocalShapeConstraintsCompute.pass
    Assets/Passes/HairLengthConstraintsWindAndCollisionCompute.pass
    Assets/Passes/HairUpdateFollowHairCompute.pass
    Assets/Passes/HairFillPPLL.pass
    Assets/Passes/HairResolvePPLL.pass
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