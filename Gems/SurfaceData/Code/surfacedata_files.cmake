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

set(FILES
    Source/SurfaceData_precompiled.cpp
    Source/SurfaceData_precompiled.h
    Include/SurfaceData/SurfaceDataConstants.h
    Include/SurfaceData/SurfaceDataTypes.h
    Include/SurfaceData/SurfaceDataSystemRequestBus.h
    Include/SurfaceData/SurfaceDataSystemNotificationBus.h
    Include/SurfaceData/SurfaceDataTagEnumeratorRequestBus.h
    Include/SurfaceData/SurfaceDataTagProviderRequestBus.h
    Include/SurfaceData/SurfaceDataProviderRequestBus.h
    Include/SurfaceData/SurfaceDataModifierRequestBus.h
    Include/SurfaceData/SurfaceTag.h
    Include/SurfaceData/Utility/SurfaceDataUtility.h
    Source/SurfaceDataSystemComponent.cpp
    Source/SurfaceDataSystemComponent.h
    Source/TerrainSurfaceDataSystemComponent.cpp
    Source/TerrainSurfaceDataSystemComponent.h
    Source/SurfaceTag.cpp
    Source/Components/SurfaceDataColliderComponent.cpp
    Source/Components/SurfaceDataColliderComponent.h
    Source/Components/SurfaceDataMeshComponent.cpp
    Source/Components/SurfaceDataMeshComponent.h
    Source/Components/SurfaceDataShapeComponent.cpp
    Source/Components/SurfaceDataShapeComponent.h
)
