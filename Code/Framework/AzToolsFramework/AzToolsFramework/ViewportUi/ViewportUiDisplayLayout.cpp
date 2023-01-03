/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Console/IConsole.h>
#include <AzToolsFramework/ViewportUi/ViewportUiDisplayLayout.h>

#include <QWidget>

namespace AzToolsFramework::ViewportUi::Internal
{
    AZ_CVAR(
        int,
        ViewportUiDisplayLayoutSpacing,
        5,
        nullptr,
        AZ::ConsoleFunctorFlags::Null,
        "The spacing between elements attached to the Viewport UI Display Layout");

    ViewportUiDisplayLayout::ViewportUiDisplayLayout(QWidget* parent)
        : QGridLayout(parent)
    {
        // set margins and spacing for internal contents
        setContentsMargins(
            ViewportUiOverlayMargin, ViewportUiOverlayMargin, ViewportUiOverlayMargin,
            ViewportUiOverlayMargin);
        setSpacing(ViewportUiDisplayLayoutSpacing);

        // create a 3x2 map of sub layouts which will stack widgets according to their mapped alignment
        m_internalLayouts = AZStd::unordered_map<Qt::Alignment, QBoxLayout*>{
            CreateSubLayout(new QVBoxLayout(), 0, 0, Qt::AlignTop | Qt::AlignLeft),
            CreateSubLayout(new QVBoxLayout(), 1, 0, Qt::AlignBottom | Qt::AlignLeft),
            CreateSubLayout(new QVBoxLayout(), 0, 1, Qt::AlignTop),
            CreateSubLayout(new QHBoxLayout(), 1, 1, Qt::AlignBottom),
            CreateSubLayout(new QVBoxLayout(), 0, 2, Qt::AlignTop | Qt::AlignRight),
            CreateSubLayout(new QHBoxLayout(), 1, 2, Qt::AlignBottom | Qt::AlignRight),
        };
    }

    void ViewportUiDisplayLayout::AddAnchoredWidget(QPointer<QWidget> widget, const Qt::Alignment alignment)
    {
        if (!widget)
        {
            return;
        }

        // find the corresponding sub layout for the alignment and add the widget
        if (auto layoutForAlignment = m_internalLayouts.find(alignment); layoutForAlignment != m_internalLayouts.end())
        {
            // place the widget before or after the invisible spacer
            // depending on the layout alignment
            int index = 0;
            switch (alignment)
            {
            case Qt::AlignTop | Qt::AlignLeft:
            case Qt::AlignTop:
                index = layoutForAlignment->second->count() - 1;
                break;
            case Qt::AlignBottom | Qt::AlignRight:
            case Qt::AlignBottom:
                index = layoutForAlignment->second->count();
                break;
            // TopRight and BottomLeft are special cases
            // place the spacer differently according to whether it's a vertical or horizontal layout
            case Qt::AlignTop | Qt::AlignRight:
                if (qobject_cast<QVBoxLayout*>(layoutForAlignment->second))
                {
                    index = layoutForAlignment->second->count() - 1;
                }
                else if (qobject_cast<QHBoxLayout*>(layoutForAlignment->second))
                {
                    index = layoutForAlignment->second->count();
                }
                break;
            case Qt::AlignBottom | Qt::AlignLeft:
                if (qobject_cast<QVBoxLayout*>(layoutForAlignment->second))
                {
                    index = layoutForAlignment->second->count();
                }
                else if (qobject_cast<QHBoxLayout*>(layoutForAlignment->second))
                {
                    index = layoutForAlignment->second->count() - 1;
                }
                break;
            }
            layoutForAlignment->second->insertWidget(index, widget);
        }
    }

    AZStd::pair<Qt::Alignment, QBoxLayout*> ViewportUiDisplayLayout::CreateSubLayout(
        QBoxLayout* layout, const int row, const int column, const Qt::Alignment alignment)
    {
        layout->setAlignment(alignment);

        // add an invisible spacer (stretch) to occupy empty space
        // without this, alignment and resizing within the sublayouts becomes difficult
        layout->addStretch(1);

        addLayout(layout, row, column, /*rowSpan=*/1, /*colSpan=*/1, alignment);

        return { alignment, layout };
    }
} // namespace AzToolsFramework::ViewportUi::Internal
