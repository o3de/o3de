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
    
    set(PATH_TO_LIBS ${BASE_PATH}/physx/bin/linux.clang/$<CONFIG>)
    set(PHYSX_LIBS                                         # Order here is important
        ${PATH_TO_LIBS}/libPhysX_static_64.a
        ${PATH_TO_LIBS}/libPhysXCooking_static_64.a
        ${PATH_TO_LIBS}/libPhysXFoundation_static_64.a
        ${PATH_TO_LIBS}/libPhysXCommon_static_64.a
        ${PATH_TO_LIBS}/libPhysXVehicle_static_64.a
        ${PATH_TO_LIBS}/libPhysXPvdSDK_static_64.a
        ${PATH_TO_LIBS}/libPhysXCharacterKinematic_static_64.a
        ${PATH_TO_LIBS}/libPhysXExtensions_static_64.a
        ${PATH_TO_LIBS}/libPhysX_static_64.a
        ${PATH_TO_LIBS}/libPhysXCooking_static_64.a
    )
    set(PHYSX_RUNTIME_DEPENDENCIES
        ${PATH_TO_LIBS}/libPhysXGpu_64.so
    )

else()
    
    set(PATH_TO_LIBS ${BASE_PATH}/physx/bin/linux.clang.shared/$<CONFIG>)
    set(PHYSX_LIBS
        ${PATH_TO_LIBS}/libPhysX_64.so
        ${PATH_TO_LIBS}/libPhysXCooking_64.so
        ${PATH_TO_LIBS}/libPhysXFoundation_64.so
        ${PATH_TO_LIBS}/libPhysXCommon_64.so
        ${PATH_TO_LIBS}/libPhysXVehicle_static_64.a
        ${PATH_TO_LIBS}/libPhysXPvdSDK_static_64.a
        ${PATH_TO_LIBS}/libPhysXCharacterKinematic_static_64.a
        ${PATH_TO_LIBS}/libPhysXExtensions_static_64.a
    )
    set(PHYSX_RUNTIME_DEPENDENCIES
        ${PATH_TO_LIBS}/libPhysX_64.so
        ${PATH_TO_LIBS}/libPhysXCooking_64.so
        ${PATH_TO_LIBS}/libPhysXFoundation_64.so
        ${PATH_TO_LIBS}/libPhysXCommon_64.so
        ${PATH_TO_LIBS}/libPhysXGpu_64.so # Runtime only
    )

endif()

