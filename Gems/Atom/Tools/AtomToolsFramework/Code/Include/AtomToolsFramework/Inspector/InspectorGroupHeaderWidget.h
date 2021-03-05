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

#if !defined(Q_MOC_RUN)
#include <AzCore/Memory/SystemAllocator.h>
#include <AzQtComponents/Components/ExtendedLabel.h>
#include <QPaintEvent>
#endif

namespace AtomToolsFramework
{
    class InspectorGroupHeaderWidget
        : public AzQtComponents::ExtendedLabel
    {
        Q_OBJECT //AUTOMOC
    public:
        AZ_CLASS_ALLOCATOR(InspectorGroupHeaderWidget, AZ::SystemAllocator, 0);

        explicit InspectorGroupHeaderWidget(QWidget* parent = nullptr);
        void SetExpanded(bool expanded);
        bool IsExpanded() const;

    protected:
        void paintEvent(QPaintEvent* event) override;

    private:
        QPixmap m_iconExpanded = QPixmap(":/Icons/group_open.png");
        QPixmap m_iconCollapsed = QPixmap(":/Icons/group_closed.png");
        bool m_expanded = true;
    };
}
