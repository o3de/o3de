#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Sources for the Editor-hosted Material Canvas pane.
#
# Note what is NOT here. MaterialCanvasMainWindow belongs to the standalone application: it derives from
# AtomToolsMainWindow, which wraps itself in a top-level window, runs its own FancyDocking, and builds its own asset
# browser, python terminal and log panel. The Editor already provides all of that, so the pane composes the underlying
# widgets itself in MaterialCanvasPaneWindow instead, the way Script Canvas does. main.cpp, MaterialCanvasApplication and
# MaterialCanvasTestData are likewise absent -- those are the standalone process entry point and its application object.
#
# The graph compiler and viewport content ARE shared with the standalone target, compiled into both binaries.

set(FILES
    Source/Editor/MaterialCanvasEditorSystemComponent.cpp
    Source/Editor/MaterialCanvasEditorSystemComponent.h
    Source/Editor/MaterialCanvasPaneWindow.cpp
    Source/Editor/MaterialCanvasPaneWindow.h

    Source/Document/InMemoryShaderCompiler.cpp
    Source/Document/InMemoryShaderCompiler.h
    Source/Document/MaterialGraphCompiler.cpp
    Source/Document/MaterialGraphCompiler.h

    Source/Window/MaterialCanvas.qrc
    Source/Window/MaterialCanvasViewportContent.cpp
    Source/Window/MaterialCanvasViewportContent.h
)
