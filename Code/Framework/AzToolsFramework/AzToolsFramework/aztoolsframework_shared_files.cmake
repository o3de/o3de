#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    Application/ToolsApplication.cpp
    Application/ToolsApplication.h
    AzToolsFrameworkModule.cpp
    AzToolsFrameworkModule.h
)

# Prevent the following files from being grouped in UNITY builds
set(SKIP_UNITY_BUILD_INCLUSION_FILES
    # The following files are skipped from unity to avoid duplicated symbols related to an ebus
    AzToolsFrameworkModule.cpp
    Application/ToolsApplication.cpp
#    UI/PropertyEditor/PropertyEntityIdCtrl.cpp
#    UI/PropertyEditor/PropertyManagerComponent.cpp
#    ViewportSelection/EditorDefaultSelection.cpp
#    ViewportSelection/EditorInteractionSystemComponent.cpp
#    ViewportSelection/EditorTransformComponentSelection.cpp
)
