#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    Include/SurfaceData/Components/SurfaceDataColliderComponent.h
    Include/SurfaceData/Components/SurfaceDataShapeComponent.h
    Include/SurfaceData/Components/SurfaceDataSystemComponent.h
    Include/SurfaceData/MixedStackHeapAllocator.h
    Include/SurfaceData/SurfaceDataConstants.h
    Include/SurfaceData/SurfaceDataTypes.h
    Include/SurfaceData/SurfaceDataSystemRequestBus.h
    Include/SurfaceData/SurfaceDataSystemNotificationBus.h
    Include/SurfaceData/SurfaceDataTagEnumeratorRequestBus.h
    Include/SurfaceData/SurfaceDataTagProviderRequestBus.h
    Include/SurfaceData/SurfaceDataProviderRequestBus.h
    Include/SurfaceData/SurfaceDataModifierRequestBus.h
    Include/SurfaceData/SurfacePointList.h
    Include/SurfaceData/SurfaceTag.h
    Include/SurfaceData/Utility/SurfaceDataUtility.h
    Source/SurfaceDataSystemComponent.cpp
    Source/SurfaceDataTypes.cpp
    Source/SurfacePointList.cpp
    Source/SurfaceTag.cpp
    Source/Components/SurfaceDataColliderComponent.cpp
    Source/Components/SurfaceDataShapeComponent.cpp
    Source/SurfaceDataUtility.cpp
)
