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

#include <QMenu>
#include <QScrollArea>
#include <QScrollBar>
#include <QSizePolicy>

#include <AtomToolsFramework/Inspector/InspectorGroupHeaderWidget.h>
#include <AtomToolsFramework/Inspector/InspectorGroupWidget.h>
#include <AtomToolsFramework/Inspector/InspectorWidget.h>
#include <Source/Inspector/ui_InspectorWidget.h>

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
        m_layout->setContentsMargins(0, 0, 0, 0);
        m_layout->setSpacing(0);
        m_headers.clear();
        m_groups.clear();
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
        m_ui->m_propertyScrollArea->verticalScrollBar()->setValue(m_ui->m_propertyScrollArea->verticalScrollBar()->minimum());

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
        groupHeader->setObjectName(groupNameId.c_str());
        m_layout->addWidget(groupHeader);
        m_headers.push_back(groupHeader);

        groupWidget->setObjectName(groupNameId.c_str());
        groupWidget->setParent(m_ui->m_propertyContent);
        m_layout->addWidget(groupWidget);
        m_groups.push_back(groupWidget);

        connect(groupHeader, &InspectorGroupHeaderWidget::clicked, this, [this, groupHeader, groupWidget](QMouseEvent* event) {
            OnHeaderClicked(event, groupHeader, groupWidget);
        });
    }
    
    void InspectorWidget::SetGroupVisible(const AZStd::string& groupNameId, bool visible)
    {
        for (size_t i = 0; i < m_groups.size(); ++i)
        {
            if (m_groups[i]->objectName() == groupNameId.c_str())
            {
                m_headers[i]->setVisible(visible);
                m_groups[i]->setVisible(visible && m_headers[i]->IsExpanded());
                break;
            }
        }
    }
    
    bool InspectorWidget::IsGroupVisible(const AZStd::string& groupNameId) const
    {
        for (auto& header : m_headers)
        {
            if (header->objectName() == groupNameId.c_str())
            {
                return header->isVisible();
            }
        }

        return false;
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

    void InspectorWidget::ExpandAll()
    {
        for (auto headerWidget : m_headers)
        {
            headerWidget->SetExpanded(true);
        }
        for (auto groupWidget : m_groups)
        {
            groupWidget->setVisible(true);
        }
    }

    void InspectorWidget::CollapseAll()
    {
        for (auto headerWidget : m_headers)
        {
            headerWidget->SetExpanded(false);
        }
        for (auto groupWidget : m_groups)
        {
            groupWidget->setVisible(false);
        }
    }

    void InspectorWidget::OnHeaderClicked(QMouseEvent* event, InspectorGroupHeaderWidget* groupHeader, QWidget* groupWidget)
    {
        if (event->button() == Qt::MouseButton::LeftButton)
        {
            groupHeader->SetExpanded(!groupHeader->IsExpanded());
            groupWidget->setVisible(groupHeader->IsExpanded());
            return;
        }

        if (event->button() == Qt::MouseButton::RightButton)
        {
            QMenu menu;
            menu.addAction("Expand", [groupHeader, groupWidget]() {
                groupHeader->SetExpanded(true);
                groupWidget->setVisible(true);
            })->setEnabled(!groupHeader->IsExpanded());
            menu.addAction("Collapse", [groupHeader, groupWidget]() {
                groupHeader->SetExpanded(false);
                groupWidget->setVisible(false);
            })->setEnabled(groupHeader->IsExpanded());
            menu.addAction("Expand All", [this]() { ExpandAll(); });
            menu.addAction("Collapse All", [this]() { CollapseAll(); });
            menu.exec(event->globalPos());
            return;
        }
    }
} // namespace AtomToolsFramework

#include <AtomToolsFramework/Inspector/moc_InspectorWidget.cpp>
