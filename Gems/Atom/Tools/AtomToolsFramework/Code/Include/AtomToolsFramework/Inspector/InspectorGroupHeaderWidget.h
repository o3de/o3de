/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Memory/SystemAllocator.h>
#include <AzQtComponents/Components/ExtendedLabel.h>
#include <QMouseEvent>
#include <QPaintEvent>
#endif

namespace AtomToolsFramework
{
    class InspectorGroupHeaderWidget
        : public AzQtComponents::ExtendedLabel
    {
        Q_OBJECT //AUTOMOC
    public:
        AZ_CLASS_ALLOCATOR(InspectorGroupHeaderWidget, AZ::SystemAllocator);

        explicit InspectorGroupHeaderWidget(QWidget* parent = nullptr);
        void SetExpanded(bool expand);
        bool IsExpanded() const;

    Q_SIGNALS:
        void clicked(QMouseEvent* event);
        void expanded();
        void collapsed();

    protected:
        void mousePressEvent(QMouseEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        QPixmap m_iconExpanded = QPixmap(":/Icons/group_open.png");
        QPixmap m_iconCollapsed = QPixmap(":/Icons/group_closed.png");
        bool m_expanded = true;
    };
}
