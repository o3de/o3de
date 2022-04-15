#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    Source/RecastNavigationModuleInterface.h
    
    Include/RecastNavigation/RecastNavigationMeshBus.h
    Include/RecastNavigation/RecastNavigationSurveyorBus.h    
    
    Source/Components/RecastNavigationMeshComponent.cpp
    Source/Components/RecastNavigationMeshComponent.h
    Source/Components/RecastNavigationSurveyorComponent.cpp
    Source/Components/RecastNavigationSurveyorComponent.h
    Source/Components/RecastNavigationMeshConfig.cpp
    Source/Components/RecastNavigationMeshConfig.h
    Source/Components/RecastNavigationDebugDraw.cpp
    Source/Components/RecastNavigationDebugDraw.h
    Source/Components/RecastSmartPointer.h
    Source/Components/RecastHelpers.h
)
