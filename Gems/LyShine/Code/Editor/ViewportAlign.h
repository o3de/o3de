/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
