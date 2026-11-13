#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    Tests/CommentTextEditFlatSheetTests.cpp
    Tests/GraphCanvasTesting.cpp

    Tests/GraphCanvasTranslation.cpp

    # The home of the looks of the comment editor, which the twins of the tests compile: it depends on Qt and AzQtComponents only
    Source/Components/Nodes/Comment/CommentTextEditFlat.cpp
    Source/Components/Nodes/Comment/CommentTextEditFlat.h
)