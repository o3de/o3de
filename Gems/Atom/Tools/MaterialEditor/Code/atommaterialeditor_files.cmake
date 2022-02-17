#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    Source/AtomMaterialEditorSystemComponent.cpp
    Source/AtomMaterialEditorSystemComponent.h

    Source/Document/MaterialDocumentRequestBus.h
    Source/Document/MaterialDocument.cpp
    Source/Document/MaterialDocument.h
    
    Source/Viewport/MaterialViewportSettings.h
    Source/Viewport/MaterialViewportRequestBus.h
    Source/Viewport/MaterialViewportNotificationBus.h
    Source/Viewport/MaterialViewportSettings.cpp
    Source/Viewport/MaterialViewportComponent.cpp
    Source/Viewport/MaterialViewportComponent.h
    Source/Viewport/MaterialViewportWidget.cpp
    Source/Viewport/MaterialViewportWidget.h
    Source/Viewport/MaterialViewportWidget.ui

    Source/Window/MaterialEditorWindowSettings.h
    Source/Window/MaterialEditorWindow.h
    Source/Window/MaterialEditorWindow.cpp
    Source/Window/MaterialEditorWindowSettings.cpp
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
    Source/Window/ToolBar/ModelPresetComboBox.h
    Source/Window/ToolBar/ModelPresetComboBox.cpp
    Source/Window/ToolBar/LightingPresetComboBox.h
    Source/Window/ToolBar/LightingPresetComboBox.cpp
    Source/Window/MaterialInspector/MaterialInspector.h
    Source/Window/MaterialInspector/MaterialInspector.cpp
    Source/Window/ViewportSettingsInspector/ViewportSettingsInspector.h
    Source/Window/ViewportSettingsInspector/ViewportSettingsInspector.cpp
)
