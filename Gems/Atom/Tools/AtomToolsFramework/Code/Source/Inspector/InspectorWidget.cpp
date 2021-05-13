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

        groupWidget->setObjectName(groupNameId.c_str());
        groupWidget->setParent(m_ui->m_propertyContent);
        m_layout->addWidget(groupWidget);

        m_groups[groupNameId] = AZStd::make_pair(groupHeader, groupWidget);

        connect(groupHeader, &InspectorGroupHeaderWidget::clicked, this, [this, groupNameId](QMouseEvent* event) {
            OnHeaderClicked(groupNameId, event);
        });
        connect(groupHeader, &InspectorGroupHeaderWidget::expanded, this, [this, groupNameId]() { OnGroupExpanded(groupNameId); });
        connect(groupHeader, &InspectorGroupHeaderWidget::collapsed, this, [this, groupNameId]() { OnGroupCollapsed(groupNameId); });

        if (ShouldGroupAutoExpanded(groupNameId))
        {
            ExpandGroup(groupNameId);
        }
        else
        {
            CollapseGroup(groupNameId);
        }
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

    void InspectorWidget::ExpandGroup(const AZStd::string& groupNameId)
    {
        auto groupItr = m_groups.find(groupNameId);
        if (groupItr != m_groups.end())
        {
            groupItr->second.first->SetExpanded(true);
            groupItr->second.second->setVisible(true);
        }
    }

    void InspectorWidget::CollapseGroup(const AZStd::string& groupNameId)
    {
        auto groupItr = m_groups.find(groupNameId);
        if (groupItr != m_groups.end())
        {
            groupItr->second.first->SetExpanded(false);
            groupItr->second.second->setVisible(false);
        }
    }

    bool InspectorWidget::IsGroupExpanded(const AZStd::string& groupNameId) const
    {
        auto groupItr = m_groups.find(groupNameId);
        return groupItr != m_groups.end() ? groupItr->second.first->IsExpanded() : false;
    }

    void InspectorWidget::ExpandAll()
    {
        for (auto& groupPair : m_groups)
        {
            groupPair.second.first->SetExpanded(true);
            groupPair.second.second->setVisible(true);
        }
    }

    void InspectorWidget::CollapseAll()
    {
        for (auto& groupPair : m_groups)
        {
            groupPair.second.first->SetExpanded(false);
            groupPair.second.second->setVisible(false);
        }
    }

    bool InspectorWidget::ShouldGroupAutoExpanded(const AZStd::string& groupNameId) const
    {
        AZ_UNUSED(groupNameId);
        return true;
    }

    void InspectorWidget::OnGroupExpanded(const AZStd::string& groupNameId)
    {
        AZ_UNUSED(groupNameId);
    }

    void InspectorWidget::OnGroupCollapsed(const AZStd::string& groupNameId)
    {
        AZ_UNUSED(groupNameId);
    }

    void InspectorWidget::OnHeaderClicked(const AZStd::string& groupNameId, QMouseEvent* event)
    {
        if (event->button() == Qt::MouseButton::LeftButton)
        {
            if (!IsGroupExpanded(groupNameId))
            {
                ExpandGroup(groupNameId);
            }
            else
            {
                CollapseGroup(groupNameId);
            }
            return;
        }

        if (event->button() == Qt::MouseButton::RightButton)
        {
            QMenu menu;
            menu.addAction("Expand", [this, groupNameId]() { ExpandGroup(groupNameId); })->setEnabled(!IsGroupExpanded(groupNameId));
            menu.addAction("Collapse", [this, groupNameId]() { CollapseGroup(groupNameId); })->setEnabled(IsGroupExpanded(groupNameId));
            menu.addAction("Expand All", [this]() { ExpandAll(); });
            menu.addAction("Collapse All", [this]() { CollapseAll(); });
            menu.exec(event->globalPos());
            return;
        }
    }
} // namespace AtomToolsFramework

#include <AtomToolsFramework/Inspector/moc_InspectorWidget.cpp>
