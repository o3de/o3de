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

#include <QScrollArea>
#include <QScrollBar>
#include <QSizePolicy>

#include <AtomToolsFramework/Inspector/InspectorGroupWidget.h>
#include <AtomToolsFramework/Inspector/InspectorGroupHeaderWidget.h>
#include <AtomToolsFramework/Inspector/InspectorWidget.h>
#include "Inspector/ui_InspectorWidget.h"

namespace AtomToolsFramework
{
    InspectorWidget::InspectorWidget(QWidget* parent)
        : QWidget(parent)
        , m_ui(new Ui::InspectorWidget)
    {
        m_ui->setupUi(this);
    }

    InspectorWidget::~InspectorWidget()
    {
    }

    void InspectorWidget::Reset()
    {
        qDeleteAll(m_ui->m_propertyContent->children());
        m_layout = new QVBoxLayout(m_ui->m_propertyContent);
        // 5 pixels added on the right to fix occlusion by the scrollbar. Remove after switching to UI 2.0.
        m_layout->setContentsMargins(0, 0, 5, 0);
        m_layout->setSpacing(0);
    }

    void InspectorWidget::AddGroupsBegin()
    {
        setUpdatesEnabled(false);

        Reset();
    }

    void InspectorWidget::AddGroupsEnd()
    {
        m_layout->addStretch();

        // Scroll to top whenever there is new content
        m_ui->m_propertyScrollArea->verticalScrollBar()->setValue(
            m_ui->m_propertyScrollArea->verticalScrollBar()->minimum());

        setUpdatesEnabled(true);
    }

    void InspectorWidget::AddGroup(
        const AZStd::string& groupNameId,
        const AZStd::string& groupDisplayName,
        const AZStd::string& groupDescription,
        QWidget* groupWidget)
    {
        InspectorGroupHeaderWidget* groupHeader = new InspectorGroupHeaderWidget(m_ui->m_propertyContent);
        groupHeader->setText(groupDisplayName.c_str());
        groupHeader->setToolTip(groupDescription.c_str());
        m_layout->addWidget(groupHeader);

        groupWidget->setObjectName(groupNameId.c_str());
        groupWidget->setParent(m_ui->m_propertyContent);
        m_layout->addWidget(groupWidget);

        connect(groupHeader, &AzQtComponents::ExtendedLabel::clicked, this, [groupHeader, groupWidget]()
        {
            groupHeader->SetExpanded(!groupHeader->IsExpanded());
            groupWidget->setVisible(groupHeader->IsExpanded());
        });
    }

    void InspectorWidget::RefreshGroup(const AZStd::string& groupNameId)
    {
        for (auto groupWidget : m_ui->m_propertyContent->findChildren<InspectorGroupWidget*>(groupNameId.c_str()))
        {
            groupWidget->Refresh();
        }
    }

    void InspectorWidget::RebuildGroup(const AZStd::string& groupNameId)
    {
        for (auto groupWidget : m_ui->m_propertyContent->findChildren<InspectorGroupWidget*>(groupNameId.c_str()))
        {
            groupWidget->Rebuild();
        }
    }

    void InspectorWidget::RefreshAll()
    {
        for (auto groupWidget : m_ui->m_propertyContent->findChildren<InspectorGroupWidget*>())
        {
            groupWidget->Refresh();
        }
    }

    void InspectorWidget::RebuildAll()
    {
        for (auto groupWidget : m_ui->m_propertyContent->findChildren<InspectorGroupWidget*>())
        {
            groupWidget->Rebuild();
        }
    }
} // namespace AtomToolsFramework

#include <AtomToolsFramework/Inspector/moc_InspectorWidget.cpp>
