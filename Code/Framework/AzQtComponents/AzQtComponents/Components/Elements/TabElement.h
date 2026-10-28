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
#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/ElementCommon.h>
#include <QColor>
#include <QMargins>
#include <QRect>
#include <QSize>
#endif

class QPainter;
class QPalette;
class QStyleOption;

namespace AzQtComponents
{
    //! The tab bars of one look, in every state: the bar, each tab and its close button, and the tab widget and pane around them.
    //! Values are what the legacy GUI renders for the sites the element covers, each citing its source. The Global elements are in
    //! TabElement.cpp; a window or feature module registers its own (registerElement), checked before them.
    //!
    //! A plain element (m_basePalette) draws no tab and no pane: those are the base style's, over the palette the legacy sheet gave
    //! the widgets (every tab widget no other element covers).
    struct AZ_QT_COMPONENTS_API TabElement
    {
        using Edge = BoxElement::Edge;
        using Edges = BoxElement::Edges;

        //! What one tab draws in one state
        struct TabFace
        {
            QColor m_fill;                      //!< invalid: no fill
            QColor m_text;                      //!< invalid: the element leaves this face out (m_pressed and m_focus may)
            Edges m_edges;
            int m_weight = 0;                   //!< QFont weight of the label; 0: the bar's font (its size is the bar's, a TextElement row)

            bool isSet() const { return m_text.isValid(); }
        };

        const char* m_name = "";                //!< Scope.Tab.Look

        // ---- The bar
        QColor m_barFill;                       //!< invalid: the bar paints nothing
        Edges m_barEdges;
        int m_barMinHeight = 0;                 //!< 0: no rule sets one, the widget keeps its own

        // ---- One tab. An inactive window draws as an active one. Every face has the edge widths of m_idle: the sheets gave every
        // state of a tab one size.
        TabFace m_idle;
        TabFace m_hover;
        TabFace m_selected;
        TabFace m_selectedHover;
        //! States of the current tab only: pressed (Qt selects a tab as it is pressed, QTabBar::mousePressEvent) and focus (the bar has
        //! the focus, QTabBarPrivate::initBasicStyleOption). Focus wins over pressed, pressed over hover and selected. Left out,
        //! pressed draws as hover, focus as idle or selected.
        TabFace m_pressed;
        TabFace m_focus;
        QMargins m_margin;                      //!< around the tab's box, inside its rect
        QMargins m_padding;
        // The contents (0: unset): the minimum width wins over the maximum, and a height replaces the text's
        int m_minContentWidth = 0;
        int m_maxContentWidth = 0;
        int m_contentHeight = 0;
        bool m_hideFocusRect = false;           //!< false: Qt draws its focus rect on the tab that has the keyboard focus

        // ---- The close button of a tab (a dock tab's is a DockBarButton: only the hover fill applies)
        const char* m_closeIcon = "";
        QColor m_closeHoverFill;
        int m_closeRadius = 0;
        int m_closeMarginRight = 0;

        // ---- The tab widget around the bar (a bar without one ignores these)
        QColor m_widgetFill;                    //!< invalid: the widget paints nothing
        Edges m_widgetEdges;
        QColor m_paneFill;                      //!< invalid: the pane paints nothing (the widget's fill shows)
        Edges m_paneEdges;
        QMargins m_panePadding;
        int m_paneEmptyOffset = 0;              //!< the pane's top offset while the widget has no tabs
        int m_barLeft = 0;                      //!< the bar's left offset (0: QCommonStyle's SE_TabWidgetTabBar)
        int m_barAreaMinHeight = 0;             //!< the minimum height of the bar's slot (0: the bar's own height)
        int m_widgetHeight = 0;                 //!< the tab widget's fixed height (0: free)
        QMargins m_widgetMargins;               //!< the tab widget's own box sits inside these

        // ---- The action toolbar container in the tab widget's top-right corner. It paints nothing: its fill is the tab widget's
        // beneath it.
        int m_cornerMinHeight = 0;              //!< the minimum height above its bottom margin (0: Qt's)
        int m_cornerMarginBottom = 0;           //!< empty space at its bottom, inside its height

        // ---- Motion
        int m_moveAnimationDuration = 0;        //!< ms a moved tab takes to slide into place (0: it snaps)

        // ---- A plain element: only the bar's fill and the maximum content width apply; the tabs, the pane and every metric are the
        // base style's
        QColor m_basePalette;                   //!< valid: plain; the colour of Base, Button and Window (the sheet's background-color)

        // ---- Geometry
        //! The size of a tab for Qt's contents size (the text and the tab buttons; the tab spacing metric is 0)
        QSize tabSize(const QSize& contents) const;
        QRect contentsRect(const QRect& tabRect) const;     //!< the tab's rect inside margin, edges and padding

        // ---- Drawing
        const TabFace& face(const QStyleOption* option) const;
        void drawTab(QPainter* painter, const QStyleOption* option) const;
        void drawClose(QPainter* painter, const QStyleOption* option) const;
        //! The box of a tab, the bar, the tab widget or the pane as its rule drew it (BoxElement::drawFrame, square corners); an
        //! invalid fill draws none
        static void drawFrame(QPainter* painter, const QRect& borderBox, const QColor& fill, const Edges& edges);
        bool isPlain() const { return m_basePalette.isValid(); }
        //! The palette of a plain element's tab or pane (QRenderRule::configurePalette)
        void applyPalette(QPalette& palette) const;

        // ---- Resolution
        //! The widgets of widgetClass the matcher accepts have this element: module rows before Global ones, each in registration order
        static void registerElement(const TabElement* element, const QMetaObject* widgetClass, ElementMatcher matcher,
            ElementScope scope = ElementScope::Module);
        //! From Style::polish: finds a tab bar's element, keeps it, and sets once what the element sets on the bar, or on the tab
        //! widget around it
        static void resolve(QWidget* widget);
        //! The kept element of a tab bar, else the one resolve would find (nothing is kept)
        static const TabElement* forWidget(const QWidget* tabBar);
        //! The element that draws the tabs of a bar, or of the tab widget around it; null for a plain one
        static const TabElement* forTabBar(const QWidget* tabBar);
        static const TabElement* forTabWidget(const QWidget* tabWidget);
        //! The plain element of a bar, or of the tab widget around it; null for one an element draws
        static const TabElement* plainForTabBar(const QWidget* tabBar);
        static const TabElement* plainForTabWidget(const QWidget* tabWidget);
    };
} // namespace AzQtComponents
