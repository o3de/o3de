/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzQtComponents/AzQtComponentsAPI.h>

#include <QColor>
#include <QRect>
#include <QSize>
#include <QStyle>
#include <QVector>

class QPainter;
class QStyleOption;
class QStyleOptionProgressBar;
class QWidget;

namespace AzQtComponents
{
    //! The elements of the small controls: the frame of the buttons beside a dock's tabs (TitleBar.qss), a filter tag's close button
    //! (FilteredSearchWidget.qss), a toolbar's panel and handle (ToolBar.qss), the buttons of a segment bar (SegmentControl.qss) and the
    //! frame of a floating dock (StyledDockWidget.qss). Every function is "not ours" (false, -1, an invalid size or rect) for a widget
    //! it does not cover, and in the legacy GUI.
    namespace ControlElements
    {
        // ---- The fills of two widgets that set a sheet of their own. The legacy GUI's sheet and the elements read the same value.
        //! ToastNotification.ui:59 (before the flattening) `background-color: rgba(255, 255, 255, 20)` of the toast's icon frame.
        //! TOKENIZE: ToastIconFrameFill
        inline constexpr QColor ToastIconFrameFill(255, 255, 255, 20);
        //! OverlayWidgetLayer.cpp:35 (LayerStyle) `background-color:rgba(0, 0, 0, 179)` of an overlay layer that dims what is behind it,
        //! and of every widget below it. TOKENIZE: OverlayDimFill
        inline constexpr QColor OverlayDimFill(0, 0, 0, 179);
        //! The class tag of a layer that dims (OverlayWidgetLayer sets it in the flattened GUI, where the legacy GUI sets the sheet): the
        //! elements of the layer and of the widgets below it are matched by it
        inline constexpr const char* OverlayDimmedTag = "OverlayDimmed";

        //! From Style::polish(QApplication*): registers the Global box elements, once (a module's own element is checked first)
        AZ_QT_COMPONENTS_API void registerGlobal();
        //! From OverlayWidgetLayer, the first time a layer dims: registers the elements of a layer that dims and of the widgets below
        //! it, once. Not at startup: their label row tests the ancestors of every label, and only the importer's windows dim a layer.
        AZ_QT_COMPONENTS_API void registerOverlayLayer();

        // ---- Toolbars: the sheet drew the panel and the handle itself and sized the handle (CE_ToolBar, PE_IndicatorToolBarHandle,
        // PM_ToolBarHandleExtent)
        //! The panel: a module's element of the toolbar when it has one (`EditorWindow QToolBar` in Editor.qss), else the fill and, under a
        //! MainWindow outside a DockMainWindow, the line at its bottom
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

        // ---- Progress bars (ProgressBar.qss): the sheet drew a square track from the rule of the bar, a flat chunk from the rule of
        // ::chunk, and the label only where the bar shows its text (QStyleSheetStyle::drawControl, CE_ProgressBar)
        //! CE_ProgressBar: the track of the bar's element (its fill, border and margin), then the chunk, then the label. False: the bar
        //! has no element, and the base style draws it.
        AZ_QT_COMPONENTS_API bool drawProgressBar(
            const QStyle* style, const QStyleOptionProgressBar* option, QPainter* painter, const QWidget* widget);
        //! The rects the chunk fills in `contents`, in the order the sheet drew them (QStyleSheetStyle::drawControl,
        //! CE_ProgressBarContents): one from the starting edge for a value, none for a bar at its minimum; for a bar with neither a range
        //! nor a value, a chunk of half the contents that `busyStep` steps of the busy animation moved along it, and the part that wraps
        //! around its far edge. The painter clips them to `contents`.
        AZ_QT_COMPONENTS_API QVector<QRect> progressChunkRects(const QStyleOptionProgressBar* option, const QRect& contents, int busyStep);
    } // namespace ControlElements
} // namespace AzQtComponents
