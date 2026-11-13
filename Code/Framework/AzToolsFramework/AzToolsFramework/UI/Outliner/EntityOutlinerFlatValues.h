/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QColor>

namespace AzToolsFramework
{
    // =======================================================================
    // Outliner Paint Colours
    // =======================================================================

    //! The colours the Entity Outliner paints with itself (its tree view and the delegate of its list model), named once, so the
    //! legacy and the flattened GUI read one value. The colours of the prefab rows are PrefabUiHandlerFlat's (PrefabUiHandler.h). A named
    //! namespace, not an anonymous one: a header is read by many files, and a unity build joins files into one translation unit, where
    //! anonymous namespaces merge.
    namespace EntityOutlinerFlat
    {
        //! The wash over a selected row: EntityOutlinerTreeView.hxx:99 (m_selectedColor), EntityOutlinerListModel.hxx:373
        //! (s_selectedColor). TOKENIZE: OutlinerSelectedWash
        inline constexpr QColor SelectedWash(255, 255, 255, 45);

        //! The wash over the row the pointer is on: EntityOutlinerTreeView.hxx:100 (m_hoverColor), EntityOutlinerListModel.hxx:374
        //! (s_hoverColor). TOKENIZE: OutlinerHoverWash
        inline constexpr QColor HoverWash(255, 255, 255, 30);

        //! The fill of the rectangle a drag selection draws: EntityOutlinerTreeView.hxx:101. TOKENIZE: OutlinerDragSelectFill
        inline constexpr QColor DragSelectFill(255, 255, 255, 20);

        //! The outline of the rectangle a drag selection draws: EntityOutlinerTreeView.hxx:102. TOKENIZE: OutlinerDragSelectBorder
        inline constexpr QColor DragSelectBorder(255, 255, 255);

        //! The disc behind the read-only badge of a row: EntityOutlinerListModel.hxx:376. TOKENIZE: OutlinerReadOnlyBadgeBackground
        inline constexpr QColor ReadOnlyBadgeBackground(0x44, 0x44, 0x44);
    } // namespace EntityOutlinerFlat
} // namespace AzToolsFramework
