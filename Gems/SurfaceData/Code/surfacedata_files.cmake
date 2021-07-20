#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
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
    Source/Components/SurfaceDataShapeComponent.cpp
    Source/Components/SurfaceDataShapeComponent.h
    Source/SurfaceDataUtility.cpp
)
