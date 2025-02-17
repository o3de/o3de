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

    ActionManager/Action/ActionManager.cpp
    ActionManager/Action/ActionManager.h
    ActionManager/Action/EditorAction.cpp
    ActionManager/Action/EditorAction.h
    ActionManager/Action/EditorActionContext.cpp
    ActionManager/Action/EditorActionContext.h
    ActionManager/Action/EditorWidgetAction.cpp
    ActionManager/Action/EditorWidgetAction.h
    ActionManager/HotKey/HotKeyManager.cpp
    ActionManager/HotKey/HotKeyManager.h
    ActionManager/HotKey/HotKeyWidgetRegistrationHelper.cpp
    ActionManager/HotKey/HotKeyWidgetRegistrationHelper.h
    ActionManager/Menu/EditorMenu.cpp
    ActionManager/Menu/EditorMenu.h
    ActionManager/Menu/EditorMenuBar.cpp
    ActionManager/Menu/EditorMenuBar.h
    ActionManager/Menu/MenuManager.cpp
    ActionManager/Menu/MenuManager.h
    ActionManager/ToolBar/EditorToolBar.cpp
    ActionManager/ToolBar/EditorToolBar.h
    ActionManager/ToolBar/EditorToolBarArea.cpp
    ActionManager/ToolBar/EditorToolBarArea.h
    ActionManager/ToolBar/ToolBarManager.cpp
    ActionManager/ToolBar/ToolBarManager.h
    ActionManager/ActionManagerSystemComponent.cpp
    ActionManager/ActionManagerSystemComponent.h

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
