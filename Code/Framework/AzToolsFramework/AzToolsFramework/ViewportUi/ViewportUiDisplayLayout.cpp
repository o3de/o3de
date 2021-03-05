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

#include "AzToolsFramework_precompiled.h"

#include <AzCore/Console/IConsole.h>
#include <AzToolsFramework/ViewportUi/ViewportUiDisplayLayout.h>

namespace AzToolsFramework::ViewportUi::Internal
{
    AZ_CVAR(
        int, ViewportUiDisplayLayoutSpacing, 5, nullptr, AZ::ConsoleFunctorFlags::Null,
        "The spacing between elements attached to the Viewport UI Display Layout");

    ViewportUiDisplayLayout::ViewportUiDisplayLayout(QWidget* parent)
        : QGridLayout(parent)
    {
        // set margins and spacing for internal contents
        setContentsMargins(0, 0, 0, 0);
        setSpacing(ViewportUiDisplayLayoutSpacing);

        // create a 3x2 map of sub layouts which will stack widgets according to their mapped alignment
        m_internalLayouts = AZStd::unordered_map<Qt::Alignment, QBoxLayout*> {
            CreateSubLayout(new QHBoxLayout(), 0, 0, Qt::AlignTop | Qt::AlignLeft),
            CreateSubLayout(new QHBoxLayout(), 1, 0, Qt::AlignBottom | Qt::AlignLeft),
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
        if (auto layoutForAlignment = m_internalLayouts.find(alignment);
            layoutForAlignment != m_internalLayouts.end())
        {
            // place the widget before the invisible spacer
            // spacer must be last item in layout to not interfere with positioning
            int index = layoutForAlignment->second->count() - 1;
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

        addLayout(layout, row, column, /*rowSpan=*/ 1, /*colSpan=*/ 1, alignment);

        return { alignment, layout };
    }
} // namespace AzToolsFramework::ViewportUi::Internal
