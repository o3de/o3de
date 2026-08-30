#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# The gem module itself. Everything of substance lives in MaterialCanvas.Editor.Static; this target only exists to expose
# it to the Editor as a loadable gem module, which is the same split Landscape Canvas uses.

set(FILES
    Source/Editor/MaterialCanvasEditorModule.cpp
)
