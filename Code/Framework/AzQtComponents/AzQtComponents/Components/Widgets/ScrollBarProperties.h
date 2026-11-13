/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

namespace AzQtComponents
{
    //! The class and the properties through which ScrollBar.cpp (the watcher, ScrollBar::setBackgroundColor) and the flattened style
    //! (Style.cpp) talk about the same scroll area, named once so that the two sides cannot drift apart
    namespace ScrollBarProperties
    {
        //! The class ScrollBar::applyDarkStyle gives a scroll area (ScrollBar.qss:34-39, :84-88, :96-100)
        inline constexpr const char* DarkStyleClass = "DarkScrollBar";

        //! The property of the corner widget a scroll area is given, which the watcher toggles while a bar is under the pointer
        //! (ScrollBar.qss:90-91 `[ShowBackground=true]`)
        inline constexpr const char* ShowBackground = "ShowBackground";

        //! The property ScrollBar::setBackgroundColor gives a scroll area in the flattened GUI, in place of the inline sheet
        //! `background: <colour>` of the legacy GUI
        inline constexpr const char* BackgroundColor = "_o3de_scrollAreaBackgroundColor";
    } // namespace ScrollBarProperties
} // namespace AzQtComponents
