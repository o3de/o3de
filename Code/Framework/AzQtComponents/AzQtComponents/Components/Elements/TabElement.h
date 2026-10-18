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
class QPalette;
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
    //!
    //! A plain element (m_basePalette) draws no tab and no pane: those are the base style's, over the
    //! palette the legacy sheet gave the widgets (the look of every tab widget no other element covers).
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
            QColor m_text;                      //!< invalid: the element leaves this face out (m_pressed and m_focus may be)
            Edges m_edges;
            int m_weight = 0;                   //!< QFont weight of the label; 0: the weight of the bar's font (its size is the bar's, a TextElement row)

            bool isSet() const { return m_text.isValid(); }
        };

        const char* m_name = "";                //!< Scope.Element.Look

        // ---- the bar
        QColor m_barFill;                       //!< invalid: the bar paints nothing
        Edges m_barEdges;
        int m_barMinHeight = 0;                 //!< 0: no rule sets one, the widget keeps its own

        // ---- one tab; an inactive window draws as an active one. All faces of an element share one box:
        // the widths of their edges are the same, as the stylesheets gave every state of a tab one size.
        TabFace m_idle;
        TabFace m_hover;
        TabFace m_selected;
        TabFace m_selectedHover;
        //! The two states of the current tab a rule can look at: pressed (Qt presses a tab as it selects it, while
        //! SH_TabBar_SelectMouseType is the press, qtabbar.cpp:2156-2165) and focused (the bar has the focus, qtabbar.cpp:139-148).
        //! Of the rules that apply at once the later one wins: focus is looked at before pressed, pressed
        //! before hover and selected. A face an element leaves out is drawn as it always was: pressed as
        //! hover, focus as idle or selected.
        TabFace m_pressed;
        TabFace m_focus;
        QMargins m_margin;                      //!< left, top, right, bottom: around the tab's box, inside its rect
        QMargins m_padding;                     //!< left, top, right, bottom
        int m_minContentWidth = 0;              //!< 0: none (the contents, inside margin, edges and padding; wins over the maximum)
        int m_maxContentWidth = 0;              //!< 0: none (applies before padding and edges)
        int m_contentHeight = 0;                //!< 0: Qt's (the text's); else the contents are this tall, whatever the text
        bool m_hideFocusRect = false;           //!< false: Qt draws its focus rectangle on the focused tab of a keyboard focus

        // ---- the close button of a tab (a dock tab's is a DockBarButton: only its hover fill is used)
        const char* m_closeIcon = "";
        QColor m_closeHoverFill;
        int m_closeRadius = 0;
        int m_closeMarginRight = 0;

        // ---- the tab widget around the bar (sites without one ignore these)
        QColor m_widgetFill;                    //!< invalid: the widget paints nothing
        Edges m_widgetEdges;
        QColor m_paneFill;                      //!< invalid: the pane paints no fill of its own (the widget's shows)
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

        // ---- plain tabs. Only the bar's fill and the maximum content width above apply to one; the rest of
        // the element is unused: the tabs, the pane and every metric are the base style's.
        QColor m_basePalette;                   //!< valid: plain; the colour of Base, Button and Window, the roles the sheet's background-color set

        // ---- use
        const TabFace& face(const QStyleOption* option) const;
        //! Tab size for Qt's contents size (text and tab buttons; the tab spacing metric is 0).
        QSize tabSize(const QSize& contents) const;
        QRect contentsRect(const QRect& tabRect) const;
        void drawTab(QPainter* painter, const QStyleOption* option) const;
        void drawClose(QPainter* painter, const QStyleOption* option) const;
        static void drawEdges(QPainter* painter, const QRect& rect, const Edges& edges);
        bool isPlain() const { return m_basePalette.isValid(); }
        //! The palette of a plain element's tab or pane, from the option the base style draws it with
        void applyPalette(QPalette& palette) const;

        // ---- resolution: once per widget at polish, kept on the widget. A module registering an element after
        // its windows were polished gets the bars they hold resolved (and polished) again.
        using Matcher = bool (*)(const QWidget* tabBar);
        static void registerElement(const TabElement* element, Matcher matcher);
        static const TabElement* resolve(const QWidget* tabBar);
        //! The element that draws the tabs of a bar, or of the tab widget around it; null for a plain one
        static const TabElement* forTabBar(const QWidget* tabBar);
        static const TabElement* forTabWidget(const QWidget* tabWidget);
        //! The plain element of a bar, or of the tab widget around it; null for one an element draws
        static const TabElement* plainForTabBar(const QWidget* tabBar);
        static const TabElement* plainForTabWidget(const QWidget* tabWidget);
        //! From Style::polish(QWidget*): what the element of a tab bar, or of a tab widget, sets on it once
        static void polish(QWidget* widget);
    };
} // namespace AzQtComponents
