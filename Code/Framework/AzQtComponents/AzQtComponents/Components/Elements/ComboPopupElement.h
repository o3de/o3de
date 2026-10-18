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
    //! The popup list of a combo box in the flattened GUI: what the legacy GUI finally renders for every item
    //! view inside the popup container of a QComboBox (the winners of ComboBox.qss:79-117). Each value cites the
    //! declaration that grants it. One element: the sites that render identically share it, and it is not built
    //! from another (the menu has its own look, with the same numbers).
    //!
    //! The popup reads it two ways, neither of them from the palette Qt passes down to the view: the view and its
    //! container take the element's palette once, when they are polished (palette()), and every item is painted
    //! from the element (prepareItem). A combo box hands its own palette to its container whenever it changes
    //! (qcombobox.cpp:1430-1448, 3295-3297), which replaces the container's palette whole; a role the view set on
    //! itself stays, and the items never asked the palette.
    struct AZ_QT_COMPONENTS_API ComboPopupElement
    {
        const char* m_name = "";                //!< Scope.Element.Look
        QColor m_surface;                       //!< the list's background
        QColor m_text;                          //!< the text of an item
        QColor m_selectedFill;                  //!< the background of the selected (current) item
        QColor m_selectedText;                  //!< the text of the selected item
        QColor m_disabledText;                  //!< the text of a disabled item
        int m_itemPaddingLeft = 0;              //!< room for the check mark the delegate draws
        int m_itemPaddingRight = 0;

        static const ComboPopupElement& global();

        // ---- selectors
        //! A view inside a combo box: its popup list
        static bool coversView(const QWidget* widget);
        //! The popup container of a combo box, or a view inside one
        static bool covers(const QWidget* widget);

        // ---- use
        //! The element's palette over a base: surface, text and selection roles in every color group
        QPalette palette(const QPalette& base) const;
        //! One item, about to be drawn by the base style: the selected fill is painted here, the text color goes
        //! into the option's palette, and the focus outline and hover are dropped from its state
        void prepareItem(QStyleOptionViewItem& option, QPainter* painter) const;
    };
} // namespace AzQtComponents
