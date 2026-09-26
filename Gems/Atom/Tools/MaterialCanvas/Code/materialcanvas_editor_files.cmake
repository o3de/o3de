#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Sources for the Editor-hosted pane; the standalone window/app files are excluded, the compiler and viewport content shared.

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
