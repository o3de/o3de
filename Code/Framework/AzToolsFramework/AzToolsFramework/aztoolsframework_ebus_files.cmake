#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
ComponentMode/EditorComponentModeBus.h
Entity/EditorEntityContextBus.h
    Entity/EditorEntityInfoBus.h
    Prefab/PrefabPublicNotificationBus.h
    AzToolsFrameworkEBus.cpp    
)

# Prevent the following files from being grouped in UNITY builds
set(SKIP_UNITY_BUILD_INCLUSION_FILES
)
