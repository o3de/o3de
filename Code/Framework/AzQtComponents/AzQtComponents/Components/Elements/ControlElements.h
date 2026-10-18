/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzQtComponents/AzQtComponentsAPI.h>

#include <QRect>
#include <QSize>
#include <QStyle>

class QPainter;
class QStyleOption;
class QWidget;

namespace AzQtComponents
{
    //! The elements of the small controls of the flattened GUI: the margin of the buttons beside a dock's tabs (TitleBar.qss),
    //! the box of a filter tag's close button (FilteredSearchWidget.qss), the panel and the handle of a toolbar (ToolBar.qss), the
    //! buttons of a segment bar (SegmentControl.qss) and the frame of a floating dock (StyledDockWidget.qss). Each takes the values
    //! the legacy GUI finally rendered for the widgets it covers; ControlElements.cpp names the declaration that grants every one.
    namespace ControlElements
    {
        // ---- Global box elements: registered with BoxElement as Global ones, so a module's own element for the same widget is
        // checked first
        //! Registers the elements. From Style::polish(QApplication*), in the flattened GUI only; a call in the legacy GUI
        //! does nothing, and so does every call after the first.
        AZ_QT_COMPONENTS_API void registerGlobal();

        // ---- toolbars. The sheet drew a toolbar's panel and handle itself (CE_ToolBar, PE_IndicatorToolBarHandle) and sized the
        // handle (PM_ToolBarHandleExtent); the base style's bevel, dots and extent are not Native's. Each is false (-1) for a widget
        // that is not a toolbar, and in the legacy GUI.
        //! The panel of a toolbar: the fill and, under a MainWindow, the line at its bottom. Draws as a rule draws a background
        //! and a border (antialiased edges: at a fractional device ratio the end rows of the line blend)
        AZ_QT_COMPONENTS_API bool drawToolBarPanel(const QStyleOption* option, QPainter* painter, const QWidget* widget);
        //! The extent of the handle of a movable toolbar along the toolbar: the image's size and the padding of the winning
        //! handle rule. -1: not ours
        AZ_QT_COMPONENTS_API int toolBarHandleExtent(const QStyleOption* option, const QWidget* widget);
        //! The handle's image, centred inside the padding of the rule, in the rect of the handle (QToolBarLayout::handleRect)
        AZ_QT_COMPONENTS_API bool drawToolBarHandle(const QStyleOption* option, QPainter* painter, const QWidget* widget);

        // ---- segment bars. The buttons of a SegmentBar are flagged to ignore the style (SegmentBar::createButton), so Style asks for them
        // ahead of its gate. SegmentControl.qss:14-77 gave every button one rule: a 1px #222222 border, the fill of its state, 16px of padding
        // at the sides, 28px of height, a negative margin that makes the border of its neighbour its own, and in a horizontal bar round
        // outer corners. Each is "not ours" (an invalid size, a null rect, false) for a widget that is not a button of a bar, and in the
        // legacy GUI.
        //! CT_PushButton: the size the rule gave a button whose label asks for `contents`
        AZ_QT_COMPONENTS_API QSize segmentButtonSize(const QSize& contents, const QWidget* widget);
        //! The contents rect of the rule in the rect of the option (inside its margin, border and padding), as SE_PushButtonContents gave
        //! it, mirrored in a right to left layout as the sheet's was
        AZ_QT_COMPONENTS_API QRect segmentButtonContents(const QStyleOption* option, const QWidget* widget);
        //! CE_PushButton, CE_PushButtonBevel and CE_PushButtonLabel of a button of a segment bar. The bevel is the rule drawn over the whole
        //! rect (QRenderRule::drawRule: the fill of the state, antialiased where a corner is round, and the border as qDrawBorder draws it,
        //! antialiased). CE_PushButton is QCommonStyle's (the bevel, then the label in the contents rect of the rule): Fusion asks its own
        //! SE_PushButtonContents for that rect, not the sheet's, so the style does it here. The label is the base style's, but in a vertical
        //! bar (`text-align: left`) it starts at the left of the contents. False: nothing drawn, let the base style.
        AZ_QT_COMPONENTS_API bool drawSegmentButton(const QStyle* style, QStyle::ControlElement element, const QStyleOption* option,
            QPainter* painter, const QWidget* widget);
        //! From Style::polish(QWidget*): what the sheet set when it polished a button of a bar (its minimum width: the rule's `min-width` and
        //! the box around it, QStyleSheetStyle::setGeometry; the hover of a rule with :hover), and, for the bar itself, the same for each of its
        //! buttons again where the bar, or a widget above it, was moved since they were last polished (see the .cpp: the sheet repolished
        //! them at that reparent, and only then)
        AZ_QT_COMPONENTS_API void polishSegmentBar(QWidget* widget);

        // ---- floating docks. StyledDockWidget.qss:9-18 gave a StyledDockWidget no border, and 1px solid black while it was floating. The sheet
        // read `floating` when it matched its rules to the dock (at its polish: qstylesheetstyle.cpp:2873-2889, 1620-1696) and kept them: a dock
        // polished docked and floated later (the Gallery floats its docks to capture them) keeps the rule without a border, no frame is drawn
        // and the layout reserves none. A dock polished while it was floating (FancyDocking::createFloatingMainWindow) has the border.
        // Both are "not ours" (-1, false) for a widget that is no StyledDockWidget, and in the legacy GUI.
        //! PM_DockWidgetFrameWidth: the width of the border of the rule the dock was polished with (QDockWidgetLayout reserves it around a
        //! floating dock that has no native frame)
        AZ_QT_COMPONENTS_API int dockFrameWidth(const QWidget* widget);
        //! PE_FrameDockWidget: the border of that rule, antialiased as QRenderRule::drawBorder draws it (nothing where the rule has none)
        AZ_QT_COMPONENTS_API bool drawDockFrame(const QStyleOption* option, QPainter* painter, const QWidget* widget);
    } // namespace ControlElements
} // namespace AzQtComponents
