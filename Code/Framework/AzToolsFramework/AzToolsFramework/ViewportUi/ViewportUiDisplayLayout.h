/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/unordered_map.h>

#include <QBoxLayout>
#include <QGridLayout>
#include <QPointer>
#include <QMargins>

namespace AzToolsFramework::ViewportUi
{
    //! Margin for the Viewport UI Overlay edges (in pixels)
    constexpr int ViewportUiOverlayMargin = 5;
    //! Padding to make space for ImGui (in pixels)
    constexpr int ViewportUiOverlayTopMarginPadding = 20;
    //! Size of the top viewport border (in pixels)
    constexpr int ViewportUiTopBorderSize = 25;
    //! Size of the left, right and bottom viewport border (in pixels)
    constexpr int ViewportUiLeftRightBottomBorderSize = 5;
    //! Complete margin for the Viewport UI Overlay
    constexpr QMargins ViewportUiOverlayDefaultMargin =
        QMargins(ViewportUiOverlayMargin, ViewportUiOverlayMargin, ViewportUiOverlayMargin, ViewportUiOverlayMargin);
    //! Complete margin for Viewport UI Overlay with border
    constexpr QMargins ViewportUiOverlayBorderMargin = QMargins(
        ViewportUiLeftRightBottomBorderSize + ViewportUiOverlayMargin,
        ViewportUiOverlayMargin + ViewportUiTopBorderSize,
        ViewportUiLeftRightBottomBorderSize + ViewportUiOverlayMargin,
        ViewportUiLeftRightBottomBorderSize + ViewportUiOverlayMargin);
    //! Complete margin for Viewport UI Overlay with ImGui
    constexpr QMargins ViewportUiOverlayImGuiMargin = QMargins(
        ViewportUiOverlayMargin,
        ViewportUiOverlayMargin + ViewportUiOverlayTopMarginPadding,
        ViewportUiOverlayMargin,
        ViewportUiOverlayMargin);
    //! Complete margin for Viewport UI Overlay with ImGui + border
    constexpr QMargins ViewportUiOverlayImGuiBorderMargin = QMargins(
        ViewportUiLeftRightBottomBorderSize + ViewportUiOverlayMargin,
        ViewportUiTopBorderSize + ViewportUiOverlayMargin + ViewportUiOverlayTopMarginPadding,
        ViewportUiLeftRightBottomBorderSize + ViewportUiOverlayMargin,
        ViewportUiLeftRightBottomBorderSize + ViewportUiOverlayMargin);
} // namespace AzToolsFramework::ViewportUi

namespace AzToolsFramework::ViewportUi::Internal
{
    //! QGridLayout implementation that uses a grid of QVBox/QHBoxLayouts internally to stack widgets.
    class ViewportUiDisplayLayout : public QGridLayout
    {
    public:
        explicit ViewportUiDisplayLayout(QWidget* parent = nullptr);
        ~ViewportUiDisplayLayout() = default;

        //! Add a QWidget to the corresponding internal sub-layout.
        void AddAnchoredWidget(QPointer<QWidget> widget, Qt::Alignment alignment);

    private:
        //! Create a sub-layout to add to the grid of layouts.
        //! @return A pair of the new layout along with its alignment on the grid.
        AZStd::pair<Qt::Alignment, QBoxLayout*> CreateSubLayout(QBoxLayout* layout, int row, int column, Qt::Alignment alignment);

        //! A mapping of each sub-layout to its corresponding alignment on the grid.
        AZStd::unordered_map<Qt::Alignment, QBoxLayout*> m_internalLayouts;
    };
} // namespace AzToolsFramework::ViewportUi::Internal
