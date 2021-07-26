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

namespace AzToolsFramework::ViewportUi::Internal
{
    // margin for the Viewport UI Overlay in pixels
    constexpr int ViewportUiOverlayMargin = 5;
    // padding to make space for ImGui
    constexpr int ViewportUiOverlayTopMarginPadding = 20;

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
