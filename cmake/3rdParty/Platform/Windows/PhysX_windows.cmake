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

if(LY_MONOLITHIC_GAME)
    
    set(PATH_TO_LIBS ${BASE_PATH}/physx/bin/win.x86_64.vc141.md/$<CONFIG>)
    set(PHYSX_LIBS
        ${PATH_TO_LIBS}/PhysX_static_64.lib
        ${PATH_TO_LIBS}/PhysXCooking_static_64.lib
        ${PATH_TO_LIBS}/PhysXFoundation_static_64.lib
        ${PATH_TO_LIBS}/PhysXCommon_static_64.lib
        ${PATH_TO_LIBS}/PhysXVehicle_static_64.lib
        ${PATH_TO_LIBS}/PhysXPvdSDK_static_64.lib
        ${PATH_TO_LIBS}/PhysXCharacterKinematic_static_64.lib
        ${PATH_TO_LIBS}/PhysXExtensions_static_64.lib
    )
    set(PHYSX_RUNTIME_DEPENDENCIES
        ${PATH_TO_LIBS}/PhysXDevice64.dll
        ${PATH_TO_LIBS}/PhysXGpu_64.dll
    )

else()
    
    set(PATH_TO_LIBS ${BASE_PATH}/physx/bin/win.x86_64.vc141.md.shared/$<CONFIG>)
    set(PHYSX_LIBS
        ${PATH_TO_LIBS}/PhysX_64.lib
        ${PATH_TO_LIBS}/PhysXCooking_64.lib
        ${PATH_TO_LIBS}/PhysXFoundation_64.lib
        ${PATH_TO_LIBS}/PhysXCommon_64.lib
        ${PATH_TO_LIBS}/PhysXVehicle_static_64.lib
        ${PATH_TO_LIBS}/PhysXPvdSDK_static_64.lib
        ${PATH_TO_LIBS}/PhysXCharacterKinematic_static_64.lib
        ${PATH_TO_LIBS}/PhysXExtensions_static_64.lib
        ${PATH_TO_LIBS}/LowLevel_static_64.lib
        ${PATH_TO_LIBS}/LowLevelAABB_static_64.lib
        ${PATH_TO_LIBS}/LowLevelDynamics_static_64.lib
        ${PATH_TO_LIBS}/SimulationController_static_64.lib
        ${PATH_TO_LIBS}/PhysXTask_static_64.lib
        ${PATH_TO_LIBS}/SceneQuery_static_64.lib
    )
    set(PHYSX_RUNTIME_DEPENDENCIES
        ${PATH_TO_LIBS}/PhysX_64.dll
        ${PATH_TO_LIBS}/PhysXCooking_64.dll
        ${PATH_TO_LIBS}/PhysXFoundation_64.dll
        ${PATH_TO_LIBS}/PhysXCommon_64.dll
        ${PATH_TO_LIBS}/PhysXDevice64.dll
        ${PATH_TO_LIBS}/PhysXGpu_64.dll
    )

endif()
