/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/optional.h>
#include <AzQtComponents/AzQtComponentsAPI.h>
#include <AzQtComponents/Components/Elements/ElementCommon.h>
#include <QColor>
#include <QMargins>
#include <QPalette>
#include <QRect>
#include <QSize>
#include <QString>
#endif

class QMenu;
class QPainter;
class QStyleOption;
class QStyleOptionMenuItem;

namespace AzQtComponents
{
    //! The popup menus of one look: the surface, its items and its separators. The legacy GUI drew the items with QWindowsStyle:
    //! the QMenu rule has a background colour and no -qt-style-features, so QRenderRule::baseStyleCanDraw is false and
    //! QStyleSheetStyle::drawControl (CE_MenuItem) hands an item to its parent style, over a palette made from the rule's colours;
    //! the sizes stay the base style's. The element does the same, except where it has an item margin: an ::item rule with a box
    //! (Menu.qss:42-45) made the sheet style size and draw the plain text items itself, which boxesItem, boxedItemSize and
    //! drawBoxedItem carry. Nothing is read from the palettes of the menu, its class or the application (a class palette set through
    //! QApplication::setPalette comes out of Style::polish(QPalette&) as the base one).
    //!
    //! The Global elements are in MenuElement.cpp; a window or feature module registers its own (registerElement), checked before it.
    struct AZ_QT_COMPONENTS_API MenuElement
    {
        const char* m_name = "";                //!< Scope.Menu.Look

        // ---- The popup
        QColor m_surface;                       //!< fills the menu
        //! The menu rule's margin. The sheet style took its left one for the panel width on all four sides (PM_MenuPanelWidth) and
        //! filled the menu inside the margin on the sides the rule has one (QRenderRule::drawRule)
        QMargins m_margin;
        int m_paddingH = 0;                     //!< between the menu's edge and its items, left and right (PM_MenuHMargin)
        int m_paddingV = 0;                     //!< top and bottom (PM_MenuVMargin)

        // ---- An item. A widget a QWidgetAction hosts in the menu, tagged WidgetAction, takes the selected fill while hovered.
        QColor m_text;
        QColor m_selectedFill;                  //!< under the item the pointer or the keyboard is on
        QColor m_disabledText;
        //! The ::item rule's margin. A rule with a box made the sheet style size and draw the menu's text items itself, not the base
        //! style: boxesItem says which items the element carries that way
        QMargins m_itemMargin;

        // ---- The icon of an item
        //! The sides of `QMenu::icon { left; top; right; bottom }`, the offsets a rule for the icon declares (0: not declared)
        struct IconOffsets
        {
            int m_left = 0;
            int m_top = 0;
            int m_right = 0;
            int m_bottom = 0;
        };

        //! What `QMenu::icon { left; top; right; bottom }` gave the icon, in px. A menu with a rule for the icon has the sheet style draw
        //! the icon of an item itself, apart from the text (QStyleSheetStyle::drawControl, CE_MenuItem): the icon sits at the left of the
        //! item, centered vertically (the rule's default position), where the base style puts it in the check column. The offsets move it
        //! from there (QStyleSheetStyle::positionRect): a `left` of 0 takes the `right` one in its place, mirrored, so `left: 0px;
        //! right: 6px` moves the icon 6px towards the left edge, and a `top` of 0 takes the `bottom`. Unset: the menu has no rule for the
        //! icon, and the base style draws it. For a menu with no item margin: an item with a margin was sized and drawn by the sheet style
        //! as a whole, which boxesItem carries for the plain text items only.
        AZStd::optional<IconOffsets> m_iconOffsets;

        // ---- A separator: margins around a border at its bottom, with no content height
        int m_separatorMarginTop = 0;
        int m_separatorBorder = 0;
        int m_separatorMarginBottom = 0;
        QColor m_separatorLine;

        // ---- Geometry
        int separatorHeight() const;
        int panelWidth() const;                 //!< PM_MenuPanelWidth: the left margin, on all four sides
        QRect panelRect(const QRect& menuRect) const;   //!< what the surface fills: the menu inside its margin
        //! Whether the sheet style's own item code, not the base style's, laid out this menu's item: the item rule has a box and the
        //! item is a plain text one. An item with an icon or a submenu arrow is the base style's here (no menu with an item margin
        //! holds one).
        bool boxesItem(const QStyleOptionMenuItem& option) const;
        //! The size of a boxed item for the size of its text, with the room of the check column the menu has for a checkable item
        //! (checkWidth: the base style's indicator width) and the item's margin
        QSize boxedItemSize(const QStyleOptionMenuItem& option, const QSize& contents, int checkWidth) const;

        // ---- Drawing
        //! The palette the sheet style handed QWindowsStyle for an item in this state, from the menu's own. An idle item has no fill of
        //! its own to show (transparent window and button colours), except for an item of a menu with a rule for the icon, which the sheet
        //! style handed over without that (`clearIdle` false).
        QPalette itemPalette(const QPalette& menuPalette, bool selected, bool enabled, bool clearIdle = true) const;
        //! Draws an item: a separator, or QWindowsStyle's item over the item palette. An item with an icon, in a menu with a rule for it
        //! (m_iconOffsets), has its text drawn that way and its icon drawn apart. False: nothing was drawn (this Qt has no Windows
        //! style, warned once), and the caller draws the item as it would without an element.
        bool drawItem(QPainter* painter, const QStyleOptionMenuItem& option, const QWidget* menu) const;
        //! The rect of an item's icon: left aligned and centered vertically in the item's rect, moved by m_iconOffsets, for an icon
        //! of the pixmap's logical size (QStyleSheetStyle::positionRect, PseudoElement_MenuIcon)
        QRect iconRect(const QRect& itemRect, const QSize& iconSize, Qt::LayoutDirection direction) const;
        void drawSeparator(QPainter* painter, const QRect& rect) const;
        //! Draws a boxed item as the sheet style's own code did: the selected fill inside the item's margin, the text and the shortcut
        //! in the contents rect, the room of the check column before them (checkWidth as for boxedItemSize)
        void drawBoxedItem(QPainter* painter, const QStyleOptionMenuItem& option, const QWidget* menu, int checkWidth) const;
        //! The fill of a widget tagged WidgetAction in a menu while the pointer is on it. False: no such widget, or the pointer is
        //! not on it (it draws nothing then).
        static bool drawHostedWidget(QPainter* painter, const QStyleOption* option, const QWidget* widget);

        // ---- Resolution
        //! The widgets of widgetClass the matcher accepts have this element: module rows before Global ones, each in registration order
        static void registerElement(const MenuElement* element, const QMetaObject* widgetClass, ElementMatcher matcher,
            ElementScope scope = ElementScope::Module);
        //! From Style::polish: finds the menu's element, keeps it, and gives the menu's palette its colours, for the widgets a
        //! QWidgetAction hosts in it
        static void resolve(QWidget* widget);
        //! The kept element, else the one resolve would find (nothing is kept); the Global one for a widget that is no menu
        static const MenuElement& forWidget(const QWidget* widget);

        // ---- The popup that fills itself
        //! The Global popup on the fill that four menus gave themselves with an inline sheet (`background-color: #333333`, with
        //! `border: none` for two of them). The sheet had no selector, so it reached its menu and every widget below it, the submenus
        //! among them; this element is found the same way, for a menu that markFilledPopup marked and for each menu below it. The
        //! items, the separators and the padding stay the popup's own: the sheet's rule has no pseudo-element, so it reaches the menu
        //! and none of its items or separators (rules with a pseudo-element do not cascade), and the menu rule has no border
        //! (Menu.qss:10-14) for `border: none` to take away.
        static const MenuElement& filledPopup();    //!< Global.Menu.FilledPopup
        //! The one line of a call site: sets the menu's legacy sheet (StyleManager::setLegacyStyleSheet, the legacy GUI only) and marks
        //! the menu (markFilledPopup, the flattened GUI only). Where the menu is made, before it is shown.
        static void setFilledPopupSheet(QMenu* menu, const QString& legacySheet);
        //! Marks a menu as one that fills itself: gives it the class tag FilledPopup (Style::addClass, which polishes the menu again).
        //! Before the menu is shown: the menus below it resolve their elements when they are polished. The flattened GUI only: the
        //! legacy GUI is left as it was.
        static void markFilledPopup(QMenu* menu);
    };

    //! The menu bar: what the rules of QMenuBar and its items render. Each item is as high as the bar's content, the bar has a
    //! border under that content, and the bar is transparent over its window (the universal rule's background, BaseStyleSheet.qss:17).
    struct AZ_QT_COMPONENTS_API MenuBarElement
    {
        const char* m_name = "";                //!< Scope.Menu.Look

        int m_height = 0;                       //!< of the bar's content, and of each item
        int m_borderWidth = 0;                  //!< under the content
        QColor m_borderColor;
        int m_itemPadding = 0;                  //!< left and right of the label of an item
        QColor m_text;
        QColor m_selectedFill;                  //!< under the item the pointer is on
        QColor m_pressedFill;                   //!< under the item whose menu is open

        // ---- Geometry
        int minHeight() const;                  //!< the height Qt keeps the bar at: its content and its border
        //! An item's size for its contents' size. Contents that are not there (an item with no title) stay not there, and Qt leaves
        //! the item out.
        QSize itemSize(const QSize& contents) const;

        // ---- Drawing
        QColor itemFill(const QStyleOption* option) const;      //!< the fill under an item in this state; invalid for an idle one
        //! The item as its label is drawn: in the box inside the item's padding, in the bar's text colour, on a transparent window
        //! colour (nothing of the base style's idle look shows but the label)
        QStyleOptionMenuItem labelOption(const QStyleOptionMenuItem& option) const;
        void drawBorder(QPainter* painter, const QRect& barRect) const;

        static const MenuBarElement& global();  //!< Global.Menu.Bar
    };
} // namespace AzQtComponents
