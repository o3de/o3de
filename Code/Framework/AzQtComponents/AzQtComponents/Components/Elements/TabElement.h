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
#endif

class QPainter;
class QStyleOption;
class QWidget;

namespace AzQtComponents
{
    //! A tab element of the flattened GUI: one complete definition of what a tab bar of this look
    //! renders, in every state. Values are the ones the legacy GUI finally renders for the sites
    //! the element covers (the winners of its stylesheet cascade, or Qt's own behavior where no
    //! rule sets one); each definition names where its values come from. Sites that render
    //! identically in every state share one element, and no element is built from another.
    //!
    //! Global elements are defined in TabElement.cpp; a window or feature module defines its own
    //! and registers it with a matcher (registerElement), checked before the Global ones.
    struct AZ_QT_COMPONENTS_API TabElement
    {
        //! One side of a box; a width of 0 draws nothing.
        struct Edge
        {
            int m_width = 0;
            QColor m_color;
        };

        struct Edges
        {
            Edge m_top;
            Edge m_right;
            Edge m_bottom;
            Edge m_left;
        };

        //! What one tab draws in one state.
        struct TabFace
        {
            QColor m_fill;                      //!< invalid: no fill
            QColor m_text;
            Edges m_edges;
        };

        const char* m_name = "";                //!< Scope.Element.Look

        // ---- the bar
        QColor m_barFill;                       //!< invalid: the bar paints nothing
        Edges m_barEdges;
        int m_barMinHeight = 0;

        // ---- one tab; focus and an inactive window draw as idle, pressed draws as hover
        TabFace m_idle;
        TabFace m_hover;
        TabFace m_selected;
        TabFace m_selectedHover;
        QMargins m_padding;                     //!< left, top, right, bottom
        int m_maxContentWidth = 0;              //!< 0: none (applies before padding and edges)

        // ---- the close button of a tab (a dock tab's is a DockBarButton: only its hover fill is used)
        const char* m_closeIcon = "";
        QColor m_closeHoverFill;
        int m_closeRadius = 0;
        int m_closeMarginRight = 0;

        // ---- the tab widget around the bar (sites without one ignore these)
        QColor m_widgetFill;                    //!< invalid: the widget paints nothing
        Edges m_widgetEdges;
        Edges m_paneEdges;
        QMargins m_panePadding;
        int m_paneEmptyOffset = 0;              //!< pane top offset while the widget has no tabs
        int m_barLeft = 0;                      //!< left offset of the bar (0: Qt's, QCommonStyle SE_TabWidgetTabBar)
        int m_barAreaMinHeight = 0;             //!< minimum height of the bar's slot (0: the bar's own height)
        int m_widgetHeight = 0;                 //!< fixed height of the tab widget (0: free)
        QMargins m_widgetMargins;               //!< the tab widget's own box sits inside these

        // ---- the action toolbar container in the tab widget's top-right corner. Its fill is the
        // tab widget's own beneath it (#111111 over #111111, or transparent), so it paints nothing.
        int m_cornerMinHeight = 0;              //!< minimum height above its bottom margin (0: as Qt sizes it)
        int m_cornerMarginBottom = 0;           //!< empty space at its bottom, inside its height

        // ---- motion
        int m_moveAnimationDuration = 0;        //!< ms a moved tab takes to slide into place (0: it snaps)

        // ---- use
        const TabFace& face(const QStyleOption* option) const;
        //! Tab size for Qt's contents size (text and tab buttons; the tab spacing metric is 0).
        QSize tabSize(const QSize& contents) const;
        QRect contentsRect(const QRect& tabRect) const;
        void drawTab(QPainter* painter, const QStyleOption* option) const;
        void drawClose(QPainter* painter, const QStyleOption* option) const;
        static void drawEdges(QPainter* painter, const QRect& rect, const Edges& edges);

        // ---- resolution: once per widget at polish, kept on the widget
        using Matcher = bool (*)(const QWidget* tabBar);
        static void registerElement(const TabElement* element, Matcher matcher);
        static const TabElement* resolve(const QWidget* tabBar);
        static const TabElement* forTabBar(const QWidget* tabBar);
        static const TabElement* forTabWidget(const QWidget* tabWidget);
    };
} // namespace AzQtComponents
