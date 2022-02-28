#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    Source/main.cpp
    Source/MaterialEditorApplication.cpp
    Source/MaterialEditorApplication.h

    Source/Document/MaterialDocumentRequestBus.h
    Source/Document/MaterialDocument.cpp
    Source/Document/MaterialDocument.h

    Source/Viewport/MaterialViewportModule.h
    Source/Viewport/MaterialViewportModule.cpp
    Source/Viewport/MaterialViewportSettings.h
    Source/Viewport/MaterialViewportRequestBus.h
    Source/Viewport/MaterialViewportNotificationBus.h
    Source/Viewport/MaterialViewportSettings.cpp
    Source/Viewport/MaterialViewportComponent.cpp
    Source/Viewport/MaterialViewportComponent.h
    Source/Viewport/MaterialViewportWidget.cpp
    Source/Viewport/MaterialViewportWidget.h

    Source/Window/MaterialEditorWindow.h
    Source/Window/MaterialEditorWindow.cpp
    Source/Window/MaterialEditor.qrc
    Source/Window/MaterialEditor.qss
    Source/Window/SettingsDialog/SettingsDialog.cpp
    Source/Window/SettingsDialog/SettingsDialog.h
    Source/Window/SettingsDialog/SettingsWidget.cpp
    Source/Window/SettingsDialog/SettingsWidget.h
    Source/Window/CreateMaterialDialog/CreateMaterialDialog.cpp
    Source/Window/CreateMaterialDialog/CreateMaterialDialog.h
    Source/Window/CreateMaterialDialog/CreateMaterialDialog.ui
    Source/Window/ToolBar/MaterialEditorToolBar.h
    Source/Window/ToolBar/MaterialEditorToolBar.cpp
    Source/Window/ViewportSettingsInspector/ViewportSettingsInspector.h
    Source/Window/ViewportSettingsInspector/ViewportSettingsInspector.cpp
)
