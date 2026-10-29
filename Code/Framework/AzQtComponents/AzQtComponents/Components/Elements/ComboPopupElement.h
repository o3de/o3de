/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/AzQtComponentsAPI.h>
#include <QColor>
#include <QPalette>
#endif

class QPainter;
class QStyleOptionViewItem;
class QWidget;

namespace AzQtComponents
{
    //! The popup list of a combo box (`QComboBox QAbstractItemView` and its `::item` rules). The popup reads it two ways, neither from
    //! the palette Qt passes down: the view and its container take the element's palette once, at polish (palette()), and every item
    //! is painted from the element (prepareItem). A combo box hands its own palette to its container whenever it changes
    //! (QComboBoxPrivate::updateViewContainerPaletteAndOpacity), which replaces the container's palette whole; a role the view set on
    //! itself stays, and the items never ask the palette.
    struct AZ_QT_COMPONENTS_API ComboPopupElement
    {
        const char* m_name = "";                //!< Scope.ComboPopup.Look
        QColor m_surface;                       //!< the list's background
        QColor m_text;                          //!< the text of an item
        QColor m_selectedFill;                  //!< the background of the selected (current) item
        QColor m_selectedText;                  //!< the text of the selected item
        QColor m_disabledText;                  //!< the text of a disabled item
        int m_itemPaddingLeft = 0;              //!< room for the check mark the delegate draws
        int m_itemPaddingRight = 0;

        // ---- Drawing
        //! The element's palette over a base: surface, text and selection roles in every colour group
        QPalette palette(const QPalette& base) const;
        //! One item, about to be drawn by the base style: the selected fill is painted here, the text colour goes into the option's
        //! palette, and the focus outline and the hover leave its state
        void prepareItem(QStyleOptionViewItem& option, QPainter* painter) const;

        // ---- Resolution: one Global element, no registry
        static const ComboPopupElement& global();
        //! A view inside a combo box: its popup list
        static bool coversView(const QWidget* widget);
        //! The popup container of a combo box, or a view inside one
        static bool covers(const QWidget* widget);
    };
} // namespace AzQtComponents
