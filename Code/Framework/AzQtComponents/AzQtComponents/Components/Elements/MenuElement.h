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
#include <QPalette>
#include <QRect>
#include <QSize>
#endif

class QPainter;
class QStyleOption;
class QStyleOptionMenuItem;

namespace AzQtComponents
{
    //! The popup menus of one look: the surface, its items and its separators. The legacy GUI drew the items with QWindowsStyle:
    //! the QMenu rule has a background colour and no -qt-style-features, so QRenderRule::baseStyleCanDraw is false and
    //! QStyleSheetStyle::drawControl (CE_MenuItem) hands an item to its parent style, over a palette made from the rule's colours;
    //! the sizes stay the base style's. The element does the same. Nothing is read from the palettes of the menu, its class or the
    //! application (a class palette set through QApplication::setPalette comes out of Style::polish(QPalette&) as the base one).
    //!
    //! The Global element is in MenuElement.cpp; a window or feature module registers its own (registerElement), checked before it.
    struct AZ_QT_COMPONENTS_API MenuElement
    {
        const char* m_name = "";                //!< Scope.Menu.Look

        // ---- The popup
        QColor m_surface;                       //!< fills the menu
        int m_paddingH = 0;                     //!< between the menu's edge and its items, left and right (PM_MenuHMargin)
        int m_paddingV = 0;                     //!< top and bottom (PM_MenuVMargin)

        // ---- An item. A widget a QWidgetAction hosts in the menu, tagged WidgetAction, takes the selected fill while hovered.
        QColor m_text;
        QColor m_selectedFill;                  //!< under the item the pointer or the keyboard is on
        QColor m_disabledText;

        // ---- A separator: margins around a border at its bottom, with no content height
        int m_separatorMarginTop = 0;
        int m_separatorBorder = 0;
        int m_separatorMarginBottom = 0;
        QColor m_separatorLine;

        // ---- Geometry
        int separatorHeight() const;

        // ---- Drawing
        //! The palette the sheet style handed QWindowsStyle for an item in this state, from the menu's own
        QPalette itemPalette(const QPalette& menuPalette, bool selected, bool enabled) const;
        //! Draws an item: a separator, or QWindowsStyle's item over the item palette. False: nothing was drawn (this Qt has no
        //! Windows style, warned once), and the caller draws the item as it would without an element.
        bool drawItem(QPainter* painter, const QStyleOptionMenuItem& option, const QWidget* menu) const;
        void drawSeparator(QPainter* painter, const QRect& rect) const;
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
