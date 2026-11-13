/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QString>

namespace O3DE::ProjectManager
{
    //! Registers the elements that carry ProjectManager.qss in the flattened GUI (AzQtComponents' frame, box, text and tab
    //! elements). StyleManager runs it when the sheet is first set on a widget (StyleManager::registerFlatSheet); calling
    //! it again does nothing.
    void RegisterFlatElements();

    //! The properties through which GemInspector::CreateStyledLabel hands the two values of its sheet (the font size in pixels and the
    //! colour, as the caller wrote it) to the elements of the flattened GUI, which sets no sheet there. RegisterFlatElements reads them
    //! back: one element for each pair of values the inspector uses.
    constexpr const char* StyledLabelFontSizeProperty = "styledLabelFontSize";
    constexpr const char* StyledLabelColorProperty = "styledLabelColor";

    // The legacy GUI's sheet takes any value a caller writes; the flattened GUI has a text element for the values in use today and
    // shows any other as a plain label. These tell whether a set of values has an element: LinkLabel's constructor and
    // GemInspector::CreateStyledLabel warn about a set that has none (flattened GUI only), so a new value is found when it is first
    // used, and not by a link that turns white.

    //! True when the flattened GUI has a text element for a LinkLabel of this font size in pixels
    bool HasFlatLinkLabelText(int fontSize);

    //! True when it has one for a styled label of this font size in pixels and this colour, as the caller writes it (case included)
    bool HasFlatStyledLabelText(int fontSize, const QString& color);
} // namespace O3DE::ProjectManager
