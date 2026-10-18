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
#include <QRect>
#include <QSize>
#endif

class QPainter;
class QStyleOption;
class QStyleOptionMenuItem;
class QWidget;

namespace AzQtComponents
{
    //! A popup menu element of the flattened GUI: one complete definition of what a menu of this look
    //! renders. Values are the ones the legacy GUI finally renders for the sites the element covers (the
    //! winners of its stylesheet cascade, or Qt's own behavior where no rule sets one); each definition
    //! names where its values come from.
    //!
    //! The legacy GUI drew the items of a popup menu with QWindowsStyle, not with the base style: the
    //! QMenu rule has a background colour and no -qt-style-features, so the sheet style cannot hand the
    //! drawing to the base (qstylesheetstyle.cpp:521-529, 3980-3989), and the sizes stay the base's. The
    //! element does the same: a private QWindowsStyle draws the items, over a palette made from the
    //! element's colours the way the sheet style made it. Nothing is read from the palettes of the menu,
    //! its class or the application (a class palette set through QApplication::setPalette goes through
    //! Style::polish(QPalette&) and comes out as the base palette, qapplication.cpp:1190-1198).
    //!
    //! Global elements are defined in MenuElement.cpp; a window or feature module defines its own and
    //! registers it with a matcher (registerElement), checked before the Global one.
    struct AZ_QT_COMPONENTS_API MenuElement
    {
        const char* m_name = "";                //!< Scope.Menu.Look

        // ---- the popup
        QColor m_surface;                       //!< fills the menu
        int m_paddingH = 0;                     //!< between the menu's edge and its items, left and right (PM_MenuHMargin)
        int m_paddingV = 0;                     //!< top and bottom (PM_MenuVMargin)

        // ---- an item (a widget a QWidgetAction puts in the menu and tags WidgetAction gets the selected fill while hovered)
        QColor m_text;
        QColor m_selectedFill;                  //!< under the item the pointer or the keyboard is on
        QColor m_disabledText;

        // ---- a separator: a box of margins around a border at its bottom, of no content height
        int m_separatorMarginTop = 0;
        int m_separatorBorder = 0;
        int m_separatorMarginBottom = 0;
        QColor m_separatorLine;

        // ---- use
        int separatorHeight() const;
        //! The palette the legacy sheet style handed the base style for an item in this state, from the menu's own
        QPalette itemPalette(const QPalette& menuPalette, bool selected, bool enabled) const;
        //! Draws an item (a separator, or QWindowsStyle's item over the item palette). False: nothing was drawn
        //! (this Qt has no Windows style, warned once), the caller draws the item as it would without an element.
        bool drawItem(QPainter* painter, const QStyleOptionMenuItem& option, const QWidget* menu) const;
        void drawSeparator(QPainter* painter, const QRect& rect) const;
        //! The fill of a widget tagged WidgetAction inside a menu while the pointer is on it. False: not such a widget, or
        //! the pointer is not on it (it draws nothing then).
        static bool drawHostedWidget(QPainter* painter, const QStyleOption* option, const QWidget* widget);

        // ---- resolution: once per menu at polish, kept on the menu. A module registers before its menus are polished, and its
        // element lives for the whole run (a static const, as the Global ones are): menus keep a pointer to it.
        using Matcher = bool (*)(const QWidget* menu);
        static void registerElement(const MenuElement* element, Matcher matcher);
        static const MenuElement& resolve(const QWidget* menu);
        //! The element of a menu: the one polish resolved, else resolved now. The Global one for a widget that is no menu.
        static const MenuElement& forMenu(const QWidget* menu);
        static const MenuElement& popup();      //!< Global.Menu.Popup
    };

    //! The menu bar element of the flattened GUI: what the legacy rules of QMenuBar and its items finally render.
    //! Each item is as high as the bar's content, the bar has a border under that content, and the bar itself is
    //! transparent over its window (the universal rule's background, BaseStyleSheet.qss:17).
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

        // ---- use
        //! The height Qt keeps the bar at: its content and its border
        int minHeight() const;
        //! An item's size for the size of its contents. Contents that are not there (an item with no title) stay not
        //! there, which makes Qt leave the item out.
        QSize itemSize(const QSize& contents) const;
        //! The fill under an item in this state; invalid for an idle one
        QColor itemFill(const QStyleOption* option) const;
        //! The item as its label is drawn: in the box inside the item's padding, with the bar's text colour, on a
        //! transparent window colour (so that nothing of the base style's idle look shows but the label)
        QStyleOptionMenuItem labelOption(const QStyleOptionMenuItem& option) const;
        void drawBorder(QPainter* painter, const QRect& barRect) const;

        static const MenuBarElement& global();  //!< Global.Menu.Bar
    };
} // namespace AzQtComponents
