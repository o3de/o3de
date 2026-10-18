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
class QWidget;
struct QMetaObject;

namespace AzQtComponents
{
    //! An item view element of the flattened GUI: how the legacy GUI finally rendered the items, the branches and the header of a
    //! QTreeView, a QTableView or a QListView (and the classes derived from them). The legacy sheets set the same things on every
    //! view, in TableView.qss (the "view" rules, the `::item`, `::branch` and `QHeaderView::section` rules), and a module's sheet
    //! changed some of their colours for its own views (AssetProcessor.qss and AssetsTab.qss for its tables, lists and asset trees,
    //! AssetBundler.qss for its headers, EntityOutliner.qss for the outliner, ProjectManager.qss for the header of its gem list).
    //!
    //! An element is the whole of that, one definition per kind of view. A member left out of a designated initializer keeps the
    //! value of TableView.qss: the one every view of the engine had, so a module element lists only what its own sheet changed
    //! (the legacy cascade layered a module's rule over TableView.qss the same way). The Global elements (ItemViewElement.cpp) are
    //! that base for a QTreeView, an AzQtComponents::TableView, a QTableView and a QListView; a module registers its own with the
    //! class of the views and a matcher (registerElement), checked before the Global ones.
    //!
    //! The legacy item rule is Qt's QSS box model (qstylesheetstyle.cpp:1061-1122): `::item { height: 24px; padding-left: 4px;
    //! border: none }` sizes an item as the base style does, 24px high, plus the padding (CT_ItemViewItem, :5557-5564), and lays
    //! its check box, icon and text out in the rect the box leaves (SE_ItemViewItem*, :6227-6256). The rules of the states fill the
    //! item and the branch cell of its row (PE_PanelItemViewItem, PE_PanelItemViewRow, PE_IndicatorBranch, :4777-4790, 4852-4884).
    //! The view's own roles (`alternate-background-color`, `selection-background-color`, `selection-color`) are the view's palette.
    //!
    //! A view resolves its element once, when it is polished, and keeps it. Elements are static, as BoxElement's are.
    struct AZ_QT_COMPONENTS_API ItemViewElement
    {
        //! The value of an integer member that is not set
        static constexpr int Unset = -1;

        //! The pseudo-states of an item or a branch cell that a face stands for: the legacy :hover, :selected, :active, :!active
        //! and :disabled (qstylesheetstyle.cpp:1757-1804). A disabled item is never hovered.
        enum State : unsigned
        {
            Hover = 1,                          //!< enabled and under the pointer (QStyle::State_MouseOver)
            Selected = 2,                       //!< QStyle::State_Selected
            Active = 4,                         //!< the window of the view is active (QStyle::State_Active)
            Inactive = 8,                       //!< ... is not
            Disabled = 16,                      //!< not QStyle::State_Enabled
        };

        //! What the rule of a state declares. A property a rule does not declare is invalid (an alpha of 0 is declared and paints nothing).
        struct Face
        {
            QColor m_fill;                      //!< background (the item, and the branch cell of its row)
            QColor m_text;                      //!< color: the colour of the item's text (Text and WindowText, and HighlightedText of a selected item)
            QColor m_selectedText;              //!< selection-color: the colour of the text of a selected item
        };

        //! Which cells of a row a face is for: the rule of the item (`::item`: the item, and the part of the row's panel that lies under the
        //! branches), the rule of the branch (`::branch`: the cell of the caret), or both (the sheets mostly give them the same colours)
        enum Scope : unsigned
        {
            Both = 0,
            OnItem = 1,
            OnBranch = 2,
        };

        //! A face that applies while all of its states hold (0: always, as a rule with no pseudo-class). The property of a state is
        //! the one of the LAST entry whose states hold and that declares it, which is the legacy cascade when the entries are
        //! written in the order the sheets declare them: an equal-specificity rule declared later wins (TableView.qss declares
        //! :selected before :hover, so a hovered selected row is hover's colour), and a compound pseudo-class such as
        //! `:selected:!active` is an entry of its own after the entries of its parts (it outranks them by specificity).
        struct StateFace
        {
            unsigned m_states = 0;
            Face m_face;
            Scope m_scope = Both;
        };

        //! The image of a `::branch` rule and its padding: the image is fitted into the branch cell less the padding and centred
        //! (QRenderRule::drawImage, qstylesheetstyle.cpp:1379-1384, with QIcon::paint). No image: the branch cell shows no caret.
        struct Branch
        {
            const char* m_image = "";           //!< resource of the image ("": none)
            QMargins m_padding;                 //!< left, top, right, bottom
        };

        const char* m_name = "";                //!< Scope.ItemView.Look

        // ---- the roles of the view: its palette (TableView.qss:21-24)
        QColor m_alternate = QColor(77, 77, 77);            //!< alternate-background-color, QPalette::AlternateBase. TOKENIZE: ItemViewAlternate
        QColor m_selection = QColor(101, 101, 101);         //!< selection-background-color, QPalette::Highlight. TOKENIZE: ItemViewSelection
        QColor m_selectionText = QColor(255, 255, 255);     //!< selection-color, QPalette::HighlightedText. TOKENIZE: ItemViewSelectionText

        // ---- the item (TableView.qss:31-40): its box, in px. The item is `m_itemHeight` high where the base style would make it
        // taller or lower (the rule's height is the contents' exact height); the margin, padding and border (none) grow it.
        int m_itemHeight = 24;                              //!< height; Unset: the base style's
        QMargins m_itemMargin;                              //!< margin: the fill leaves it free
        QMargins m_itemPadding = QMargins(4, 0, 0, 0);      //!< padding: left, top, right, bottom. TOKENIZE: ItemViewPaddingLeft

        // ---- the check box of an item (`::indicator`): CheckBox.qss:23-28 `QAbstractItemView::indicator`, over TableView.qss:78-85 for a tree
        QSize m_indicatorSize = QSize(16, 16);              //!< width, height of the image
        QMargins m_indicatorMargin = QMargins(2, 2, 2, 2);  //!< margin around it: the box the layout makes room for is the image's size plus these

        // ---- the branches of a tree (TableView.qss:87-108) and its indentation (TreeView.h:39)
        Branch m_closed;                                    //!< `::branch:closed:has-children` (a right caret); no image: none
        Branch m_open;                                      //!< `::branch:open:has-children` (a down caret)
        int m_indentation = Unset;                          //!< setIndentation at polish and again when the tree is first shown; Unset: not set

        // ---- the faces of the item and of the branch cell of its row, in the order the sheets declare them (capacity 10: the Global
        // element's, then a module's)
        StateFace m_faces[10] = {
            { Selected, { QColor(101, 101, 101), QColor(), QColor() } },    // TableView.qss:42-50 :selected { background: rgb(101,101,101) }. TOKENIZE: ItemViewSelection
            { Hover, { QColor(90, 90, 90), QColor(), QColor() } },          // :52-60 :hover { background: rgb(90,90,90) }. TOKENIZE: ItemViewHover.
                                                                            // UNVERIFIED: no Native capture holds a hovered item of a view with this look
        };

        // ---- the header (TableView.qss:62-76 `QHeaderView::section`): the fill of its sections, and so of the area of the header
        // beyond the last section (the legacy header's viewport showed the colour of the section rule). An alpha of 0 paints
        // nothing and lets the window show through, as a section declared `background: transparent` did.
        QColor m_headerFill = QColor(0x2D, 0x2D, 0x2D);     //!< TOKENIZE: HeaderSectionFill

        // ---- what the style asks of an element (flattened mode, from Style.cpp)
        //! The states of an option, as the faces name them
        static unsigned statesOf(const QStyleOption* option);
        //! The background and the text colours that hold in a set of states for the item or for the branch (an invalid colour: the rule declares none)
        QColor fillOf(unsigned states, Scope scope = OnItem) const;
        QColor textOf(unsigned states) const;
        QColor selectedTextOf(unsigned states) const;
        //! CT_ItemViewItem: the size the base style gave the item, with the rule's height and the box around it (qstylesheetstyle.cpp:5557-5564)
        QSize itemSize(const QSize& base) const;
        //! The rect the item's box leaves for its contents (qstylesheetstyle.cpp:6246-6252): the layout of the check box, icon and text
        QRect itemContentsRect(const QRect& rect) const;
        //! What the check box asks of the layout (PM_IndicatorWidth and PM_IndicatorHeight of an item option, :5041-5050), and its image in that room
        QSize indicatorBox() const;
        QRect indicatorImageRect(const QRect& slot) const;
        //! The palette of an item as the rule of its state set it (qstylesheetstyle.cpp:4366-4370)
        void prepareItem(QStyleOptionViewItem* option) const;
        //! The fill of the item's rule in the state of the option, over the item's rect less its margin (QRenderRule::drawRule, :1349-1353)
        void drawFill(QPainter* painter, const QStyleOption* option) const;
        //! PE_IndicatorBranch of a view whose rule gives the branch an image or a fill (qstylesheetstyle.cpp:4777-4790): the row's panel
        //! (through the style, which draws what the base style draws), the face's fill, then the caret fitted into the cell less its padding.
        //! False: the rule has nothing for this cell (no fill in its state, no image), and the cell is the base style's, as the sheet style asked it
        bool drawBranch(const QStyle* style, const QStyleOptionViewItem* option, QPainter* painter, const QWidget* widget) const;

        // ---- resolution: once per view at polish, kept on the view
        using Matcher = bool (*)(const QWidget* widget);
        //! The element for the views of widgetClass (and its subclasses) that a matcher accepts, before the Global ones. Of the registrations that
        //! match a view the first registered wins. Views polished before the call are resolved again.
        //!
        //! A module's element lists only what its own sheet changes, as the legacy cascade layered a module's rule over TableView.qss. What it
        //! leaves out is the Global element's of the kind of the view (a plain tree, a TableView, a table, a list): a member it leaves at the value
        //! of the struct's defaults is the Global element's (the carets, the check box margin and the indentation of a tree, the disabled
        //! text), and its faces are layered AFTER the Global element's, so that a property a module's face declares replaces the Global face's
        //! for the states it names and every other state keeps the Global face. A module that wants a member to have exactly the value
        //! of the defaults where the Global element has another has no way to say so (none of the legacy sheets did).
        static void registerElement(const ItemViewElement* element, const QMetaObject* widgetClass, Matcher matcher);
        //! From Style::polish(QWidget*), after the base style (flattened mode): resolve the element of a view, keep it, and set what it
        //! gives the view: the palette roles, the indentation of a tree, hover tracking, and the header palette. A header resolves nothing.
        static void apply(QWidget* widget);
        //! The element kept on a view; null: none (not an item view, a view the legacy sheet did not cover: a plain QAbstractItemView
        //! subclass, or the list of a combo box popup, which ComboPopupElement owns), or not polished yet
        static const ItemViewElement* forWidget(const QWidget* widget);
        //! The element of the view a header belongs to, resolved now if the view was not polished yet; null for a header of no covered view
        static const ItemViewElement* forHeader(const QWidget* header);
        //! The fill of the sections of a header: its view's element, else TableView.qss's. A header whose `SectionBackground` property is
        //! "transparent" (the Gallery's log table) has none.
        static QColor headerFillOf(const QWidget* header);
        //! Called once when a tree is first shown: the legacy sheet style polished a tree again after its owner had built it (a sheet set on
        //! a parent, a new parent), and the indentation the element gives was set again then
        static void showTree(QWidget* tree);
    };
} // namespace AzQtComponents
