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
    Source/Viewport/InputController/MaterialEditorViewportInputControllerBus.h
    Source/Viewport/MaterialViewportSettings.h
    Source/Viewport/MaterialViewportRequestBus.h
    Source/Viewport/MaterialViewportNotificationBus.h
    Source/Viewport/InputController/MaterialEditorViewportInputController.cpp
    Source/Viewport/InputController/MaterialEditorViewportInputController.h
    Source/Viewport/InputController/Behavior.cpp
    Source/Viewport/InputController/Behavior.h
    Source/Viewport/InputController/DollyCameraBehavior.cpp
    Source/Viewport/InputController/DollyCameraBehavior.h
    Source/Viewport/InputController/IdleBehavior.cpp
    Source/Viewport/InputController/IdleBehavior.h
    Source/Viewport/InputController/MoveCameraBehavior.cpp
    Source/Viewport/InputController/MoveCameraBehavior.h
    Source/Viewport/InputController/PanCameraBehavior.cpp
    Source/Viewport/InputController/PanCameraBehavior.h
    Source/Viewport/InputController/OrbitCameraBehavior.cpp
    Source/Viewport/InputController/OrbitCameraBehavior.h
    Source/Viewport/InputController/RotateEnvironmentBehavior.cpp
    Source/Viewport/InputController/RotateEnvironmentBehavior.h
    Source/Viewport/InputController/RotateModelBehavior.cpp
    Source/Viewport/InputController/RotateModelBehavior.h
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
