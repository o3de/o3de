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

    Source/Viewport/MaterialCanvasViewportSettings.h
    Source/Viewport/MaterialCanvasViewportSettingsRequestBus.h
    Source/Viewport/MaterialCanvasViewportSettingsNotificationBus.h
    Source/Viewport/MaterialCanvasViewportSettings.cpp
    Source/Viewport/MaterialCanvasViewportSettingsSystem.cpp
    Source/Viewport/MaterialCanvasViewportSettingsSystem.h
    Source/Viewport/MaterialCanvasViewportWidget.cpp
    Source/Viewport/MaterialCanvasViewportWidget.h

    Source/Window/MaterialCanvasGraphView.h
    Source/Window/MaterialCanvasGraphView.cpp
    Source/Window/MaterialCanvasMainWindow.h
    Source/Window/MaterialCanvasMainWindow.cpp
    Source/Window/MaterialCanvas.qrc
    Source/Window/ToolBar/MaterialCanvasToolBar.h
    Source/Window/ToolBar/MaterialCanvasToolBar.cpp
    Source/Window/ViewportSettingsInspector/ViewportSettingsInspector.h
    Source/Window/ViewportSettingsInspector/ViewportSettingsInspector.cpp
)
