#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    Source/main.cpp
    Source/MaterialCanvasApplication.cpp
    Source/MaterialCanvasApplication.h

    Source/Document/MaterialCanvasDocumentRequestBus.h
    Source/Document/MaterialCanvasDocument.cpp
    Source/Document/MaterialCanvasDocument.h

    Source/Viewport/MaterialCanvasViewportModule.h
    Source/Viewport/MaterialCanvasViewportModule.cpp
    Source/Viewport/InputController/MaterialCanvasViewportInputControllerBus.h
    Source/Viewport/MaterialCanvasViewportSettings.h
    Source/Viewport/MaterialCanvasViewportRequestBus.h
    Source/Viewport/MaterialCanvasViewportNotificationBus.h
    Source/Viewport/InputController/MaterialCanvasViewportInputController.cpp
    Source/Viewport/InputController/MaterialCanvasViewportInputController.h
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
    Source/Viewport/MaterialCanvasViewportSettings.cpp
    Source/Viewport/MaterialCanvasViewportComponent.cpp
    Source/Viewport/MaterialCanvasViewportComponent.h
    Source/Viewport/MaterialCanvasViewportWidget.cpp
    Source/Viewport/MaterialCanvasViewportWidget.h
    Source/Viewport/MaterialCanvasViewportWidget.ui

    Source/Window/MaterialCanvasBrowserInteractions.h
    Source/Window/MaterialCanvasBrowserInteractions.cpp
    Source/Window/MaterialCanvasMainWindow.h
    Source/Window/MaterialCanvasMainWindow.cpp
    Source/Window/MaterialCanvasMainWindowSettings.h
    Source/Window/MaterialCanvasMainWindowSettings.cpp
    Source/Window/MaterialCanvas.qrc
    Source/Window/ToolBar/MaterialCanvasToolBar.h
    Source/Window/ToolBar/MaterialCanvasToolBar.cpp
    Source/Window/ToolBar/ModelPresetComboBox.h
    Source/Window/ToolBar/ModelPresetComboBox.cpp
    Source/Window/ToolBar/LightingPresetComboBox.h
    Source/Window/ToolBar/LightingPresetComboBox.cpp
    Source/Window/Inspector/MaterialCanvasInspector.h
    Source/Window/Inspector/MaterialCanvasInspector.cpp
)
