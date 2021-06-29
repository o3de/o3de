/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/std/containers/unordered_map.h>
#include <QGridLayout>
#include <QBoxLayout>
#include <QPointer>

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
        AZStd::pair<Qt::Alignment, QBoxLayout*> CreateSubLayout(
            QBoxLayout* layout, int row, int column, Qt::Alignment alignment);

        //! A mapping of each sub-layout to its corresponding alignment on the grid.
        AZStd::unordered_map<Qt::Alignment, QBoxLayout*> m_internalLayouts;
    };
} // namespace AzToolsFramework::ViewportUi::Internal
