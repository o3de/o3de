/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

////////////////////////////////////////////////////////////////////////////////////////////////////
//! ViewportAlign contains static function that implement alignment operations on elements.
class ViewportAlign
{
public: //types

    enum class AlignType
    {
        HorizontalLeft,
        HorizontalCenter,
        HorizontalRight,
        VerticalTop,
        VerticalCenter,
        VerticalBottom
    };

public: //static functions

    //! Align the selected elements using the given type of alignment operation
    static void AlignSelectedElements(EditorWindow* editorWindow, AlignType alignType);

    //! Check if alignment is allowed given the current selection
    static bool IsAlignAllowed(EditorWindow* editorWindow);
};
