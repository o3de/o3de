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
    //! The elements of the small controls: the frame of the buttons beside a dock's tabs (TitleBar.qss), a filter tag's close button
    //! (FilteredSearchWidget.qss), a toolbar's panel and handle (ToolBar.qss), the buttons of a segment bar (SegmentControl.qss) and the
    //! frame of a floating dock (StyledDockWidget.qss). Every function is "not ours" (false, -1, an invalid size or rect) for a widget
    //! it does not cover, and in the legacy GUI.
    namespace ControlElements
    {
        //! From Style::polish(QApplication*): registers the Global box elements, once (a module's own element is checked first)
        AZ_QT_COMPONENTS_API void registerGlobal();

        // ---- Toolbars: the sheet drew the panel and the handle itself and sized the handle (CE_ToolBar, PE_IndicatorToolBarHandle,
        // PM_ToolBarHandleExtent)
        //! The panel: the fill and, under a MainWindow, the line at its bottom
        AZ_QT_COMPONENTS_API bool drawToolBarPanel(const QStyleOption* option, QPainter* painter, const QWidget* widget);
        //! The extent of a movable toolbar's handle along the toolbar: the image and the padding of the handle rule that wins
        AZ_QT_COMPONENTS_API int toolBarHandleExtent(const QStyleOption* option, const QWidget* widget);
        //! The handle's image, centered inside the rule's padding in the handle's rect
        AZ_QT_COMPONENTS_API bool drawToolBarHandle(const QStyleOption* option, QPainter* painter, const QWidget* widget);

        // ---- Segment bars: one rule for every button (a border, the fill of its state, padding at the sides, a height, a negative
        // margin that shares the border of its neighbour, round outer corners in a horizontal bar). The buttons are flagged to ignore
        // the style (SegmentBar::createButton), so Style asks for them ahead of its gate.
        //! CT_PushButton: the size of a button whose label asks for `contents`
        AZ_QT_COMPONENTS_API QSize segmentButtonSize(const QSize& contents, const QWidget* widget);
        //! The rule's contents rect in the option's rect, mirrored in a right to left layout
        AZ_QT_COMPONENTS_API QRect segmentButtonContents(const QStyleOption* option, const QWidget* widget);
        //! CE_PushButton, CE_PushButtonBevel and CE_PushButtonLabel of a segment bar's button: the rule over the whole rect, the label in
        //! the rule's contents (left in a vertical bar). False: the base style draws it.
        AZ_QT_COMPONENTS_API bool drawSegmentButton(const QStyle* style, QStyle::ControlElement element, const QStyleOption* option,
            QPainter* painter, const QWidget* widget);
        //! From Style::polish(QWidget*): what the sheet set when it polished a button (its minimum width, the hover of its :hover rule),
        //! and for the bar itself the same again for its buttons where a widget above the bar moved since they were last polished
        AZ_QT_COMPONENTS_API void polishSegmentBar(QWidget* widget);

        // ---- Floating docks: the frame of the rule the dock was polished with (a border while floating, none while docked)
        //! PM_DockWidgetFrameWidth: the width of that border, which QDockWidgetLayout reserves around a floating dock
        AZ_QT_COMPONENTS_API int dockFrameWidth(const QWidget* widget);
        //! PE_FrameDockWidget: that border (nothing where the rule has none)
        AZ_QT_COMPONENTS_API bool drawDockFrame(const QStyleOption* option, QPainter* painter, const QWidget* widget);
    } // namespace ControlElements
} // namespace AzQtComponents
