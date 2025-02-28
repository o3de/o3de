#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    ActionManager/ActionManagerRegistrationNotificationBus.h
    ActionManager/Action/ActionManagerNotificationBus.h
    API/AssetDatabaseBus.h
    API/EditorCameraBus.cpp
    API/EditorCameraBus.h
    API/ComponentEntityObjectBus.h
    API/ComponentEntitySelectionBus.h 
    API/EditorAnimationSystemRequestBus.h
    API/EditorLevelNotificationBus.h
    API/EditorPythonConsoleBus.h
    API/EditorPythonRunnerRequestsBus.h
    API/EditorPythonScriptNotificationsBus.h
    API/EditorWindowRequestBus.h
    API/EntityCompositionNotificationBus.h
    API/EntityCompositionRequestBus.h
    API/EntityPropertyEditorNotificationBus.h
    API/EntityPropertyEditorRequestsBus.h
    API/ToolsApplicationBus.cpp
    API/ToolsApplicationBus.h
    API/ViewportEditorModeTrackerNotificationBus.cpp
    API/ViewportEditorModeTrackerNotificationBus.h
    AssetBrowser/AssetBrowserBus.cpp
    AssetBrowser/AssetBrowserBus.h
    AssetEditor/AssetEditorBus.cpp
    AssetEditor/AssetEditorBus.h
    AssetCatalog/PlatformAddressedAssetCatalogBus.cpp
    AssetCatalog/PlatformAddressedAssetCatalogBus.h
    ComponentMode/ComponentModeDelegateBus.h
    ComponentMode/EditorComponentModeBus.h
    Entity/EditorEntityContextBus.h
    Entity/EditorEntityInfoBus.h
    Prefab/PrefabPublicNotificationBus.h
    Thumbnails/SourceControlThumbnailBus.cpp
    Thumbnails/SourceControlThumbnailBus.h
    ToolsComponents/EditorComponentBus.h
#    ToolsComponents/EditorVisibilityBus.h
    ToolsComponents/EditorLockComponentBus.h
    Viewport/ViewportMessagesBus.h
    AzToolsFrameworkEBus.cpp    
)

# Prevent the following files from being grouped in UNITY builds
set(SKIP_UNITY_BUILD_INCLUSION_FILES
)
