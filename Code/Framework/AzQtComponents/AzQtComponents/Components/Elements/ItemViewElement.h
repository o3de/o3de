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
#include <AzQtComponents/Components/Elements/ElementCommon.h>
#include <QColor>
#include <QMargins>
#include <QRect>
#include <QSize>
#include <QStyle>
#endif

class QAbstractItemView;
class QHeaderView;
class QPainter;
class QStyleOption;
class QStyleOptionViewItem;

namespace AzQtComponents
{
    //! The items, branches and header of a QTreeView, a QTableView or a QListView (and the classes derived from them): TableView.qss
    //! set them on every view, and a module's sheet changed some of them for its own views. A member left out of a designated
    //! initializer keeps TableView.qss's value; the faces of a module's element are layered after the Global faces of its view's kind.
    //!
    //! The item rule is Qt's style sheet box model: `::item { height: 24px; padding-left: 4px; border: none }` sizes an item as the
    //! base style does, at the rule's height, plus the box (QStyleSheetStyle::sizeFromContents, CT_ItemViewItem), and lays its check
    //! box, icon and text out in what the box leaves. The rules of the states fill the item and the branch cell of its row. The view's
    //! own roles (`alternate-background-color`, `selection-background-color`, `selection-color`) are the view's palette.
    struct AZ_QT_COMPONENTS_API ItemViewElement
    {
        //! An integer member no rule sets
        static constexpr int Unset = -1;

        //! The pseudo-states a face stands for (pseudoClass, qstylesheetstyle.cpp); a disabled item is never hovered
        enum State : unsigned
        {
            Hover = 1,                          //!< enabled and under the pointer (QStyle::State_MouseOver)
            Selected = 2,                       //!< QStyle::State_Selected
            Active = 4,                         //!< the view's window is active (QStyle::State_Active)
            Inactive = 8,                       //!< ... is not
            Disabled = 16,                      //!< not QStyle::State_Enabled
        };

        //! What the rule of a state declares; a property it does not declare is invalid (an alpha of 0 is declared and paints nothing)
        struct Face
        {
            QColor m_fill;                      //!< background (the item, and the branch cell of its row)
            QColor m_text;                      //!< color: the item's text (Text and WindowText, and HighlightedText of a selected item)
            QColor m_selectedText;              //!< selection-color: the text of a selected item
        };

        //! The cells of a row a face is for: the item (`::item`, with the part of the row's panel under the branches), the branch cell
        //! (`::branch`), or both
        enum Scope : unsigned
        {
            Both = 0,
            OnItem = 1,
            OnBranch = 2,
        };

        //! A face that applies while all of its states hold (0: always). A property of a state is the one of the last entry whose
        //! states hold and that declares it: the cascade, when the entries follow the order the sheets declare them in.
        struct StateFace
        {
            unsigned m_states = 0;
            Face m_face;
            Scope m_scope = Both;
        };

        //! The image of a `::branch` rule and its padding: the image fitted into the cell less the padding, centered
        //! (QRenderRule::drawImage, with QIcon::paint). No image: no caret.
        struct Branch
        {
            const char* m_image = "";           //!< resource of the image ("": none)
            QMargins m_padding;                 //!< left, top, right, bottom
        };

        const char* m_name = "";                //!< Scope.ItemView.Look

        // ---- The roles of the view: its palette
        QColor m_alternate = QColor(0x4D, 0x4D, 0x4D);      //!< TableView.qss:21 $fill.view.idle.alternate-fill.Alt1
        QColor m_selection = QColor(0x65, 0x65, 0x65);      //!< TableView.qss:23 $fill.view.idle.selection-fill.Alt1
        QColor m_selectionText = QColor(0xFF, 0xFF, 0xFF);  //!< TableView.qss:24 $text.view.idle

        // ---- The item's box: the item is m_itemHeight high where the base style would make it taller or lower, grown by the box
        int m_itemHeight = 24;                              //!< TableView.qss:36 $size.item.idle (Unset: the base style's)
        QMargins m_itemMargin;                              //!< the fill leaves it free
        QMargins m_itemPadding = QMargins(4, 0, 0, 0);      //!< TableView.qss:38 $space.item.idle.padding-left.Alt1

        // ---- The check box of an item (`QAbstractItemView::indicator`)
        //! CheckBox.qss:25-26 $size.check-indicator.idle.width.Alt1 $size.check-indicator.idle.height
        QSize m_indicatorSize = QSize(16, 16);
        //! Around the image: the box the layout makes room for. CheckBox.qss:27 $space.check-indicator.idle.margin.Alt1
        QMargins m_indicatorMargin = QMargins(2, 2, 2, 2);

        // ---- The branches of a tree and its indentation
        Branch m_closed;                                    //!< `::branch:closed:has-children` (a right caret); no image: none
        Branch m_open;                                      //!< `::branch:open:has-children` (a down caret)
        int m_indentation = Unset;                          //!< set at polish, and again when the tree is first shown; Unset: not set

        // ---- The faces of the item and of the branch cell of its row, in the order the sheets declare them
        StateFace m_faces[16];

        //! The fill of a header's sections, and so of the header beyond the last section (the legacy header's viewport showed the
        //! section rule's colour); an alpha of 0 lets the window show through. TableView.qss:69 $fill.header.idle.Alt1
        QColor m_headerFill = QColor(0x2D, 0x2D, 0x2D);

        // ---- Geometry and drawing
        //! The states of an option, as the faces name them
        static unsigned statesOf(const QStyleOption* option);
        //! The background and text colours that hold in a set of states, for the item or for the branch (invalid: none declared)
        QColor fillOf(unsigned states, Scope scope = OnItem) const;
        QColor textOf(unsigned states) const;
        QColor selectedTextOf(unsigned states) const;
        //! CT_ItemViewItem: the size the base style gave the item, at the rule's height, with the box around it
        QSize itemSize(const QSize& base) const;
        //! The rect the item's box leaves for its contents: the layout of the check box, icon and text
        QRect itemContentsRect(const QRect& rect) const;
        //! What the check box asks of the layout (PM_IndicatorWidth and PM_IndicatorHeight of an item option), and its image in it
        QSize indicatorBox() const;
        QRect indicatorImageRect(const QRect& slot) const;
        //! The palette of an item as the rule of its state set it (QStyleSheetStyle::drawControl, CE_ItemViewItem)
        void prepareItem(QStyleOptionViewItem* option) const;
        //! The fill of the item's rule in the option's state, over the item's rect less its margin
        void drawFill(QPainter* painter, const QStyleOption* option) const;
        //! PE_IndicatorBranch of a rule with an image or a fill: the row's panel, the face's fill, then the caret. False: the rule has
        //! nothing for this cell, which is the base style's.
        bool drawBranch(const QStyle* style, const QStyleOptionViewItem* option, QPainter* painter, const QWidget* widget) const;

        // ---- Resolution
        //! The widgets of widgetClass the matcher accepts have this element: module rows before Global ones, each in registration order.
        //! A module's element lists only what its own sheet changes: a member left at the struct's default is the Global element's of
        //! the view's kind (a plain tree, a TableView, a table, a list), and its faces are layered after the Global faces.
        static void registerElement(const ItemViewElement* element, const QMetaObject* widgetClass, ElementMatcher matcher,
            ElementScope scope = ElementScope::Module);
        //! From Style::polish, after the base style: finds the view's element, keeps it, and sets the palette roles, the indentation of
        //! a tree, hover tracking and the header palette. A header resolves nothing.
        static void resolve(QWidget* widget);
        //! The kept element; none before the view is polished, or for a widget no element covers (not an item view, a plain
        //! QAbstractItemView subclass, or a combo box popup's list, which ComboPopupElement owns)
        static const ItemViewElement* forWidget(const QWidget* widget);
        //! The element of the view a header belongs to, looked up now if the view was not polished yet; none for a header of no
        //! covered view
        static const ItemViewElement* forHeader(const QWidget* header);
        //! The fill of a header's sections: its view's element, else TableView.qss's; none for a header whose `SectionBackground`
        //! property is "transparent"
        static QColor headerFillOf(const QWidget* header);
        //! Called once when a tree is first shown: the legacy sheet style polished a tree again after its owner had built it, and set
        //! the element's indentation again then
        static void showTree(QWidget* tree);
    };
} // namespace AzQtComponents
