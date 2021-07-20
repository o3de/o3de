/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        m_layout->addWidget(groupHeader);

        groupWidget->setObjectName(groupNameId.c_str());
        groupWidget->setParent(m_ui->m_propertyContent);
        m_layout->addWidget(groupWidget);

        m_groups[groupNameId] = {groupHeader, groupWidget};

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
        auto groupItr = m_groups.find(groupNameId);
        if (groupItr != m_groups.end())
        {
            groupItr->second.m_header->setVisible(visible);
            groupItr->second.m_panel->setVisible(visible && groupItr->second.m_header->IsExpanded());
        }
    }
    
    bool InspectorWidget::IsGroupVisible(const AZStd::string& groupNameId) const
    {
        auto groupItr = m_groups.find(groupNameId);
        if (groupItr != m_groups.end())
        {
            return groupItr->second.m_header->isVisible();
        }

        return false;
    }
    
    bool InspectorWidget::IsGroupHidden(const AZStd::string& groupNameId) const
    {
        auto groupItr = m_groups.find(groupNameId);
        if (groupItr != m_groups.end())
        {
            return groupItr->second.m_header->isHidden();
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
            groupItr->second.m_header->SetExpanded(true);
            groupItr->second.m_panel->setVisible(true);
        }
    }

    void InspectorWidget::CollapseGroup(const AZStd::string& groupNameId)
    {
        auto groupItr = m_groups.find(groupNameId);
        if (groupItr != m_groups.end())
        {
            groupItr->second.m_header->SetExpanded(false);
            groupItr->second.m_panel->setVisible(false);
        }
    }

    bool InspectorWidget::IsGroupExpanded(const AZStd::string& groupNameId) const
    {
        auto groupItr = m_groups.find(groupNameId);
        return groupItr != m_groups.end() ? groupItr->second.m_header->IsExpanded() : false;
    }

    void InspectorWidget::ExpandAll()
    {
        for (auto& groupPair : m_groups)
        {
            groupPair.second.m_header->SetExpanded(true);
            groupPair.second.m_panel->setVisible(true);
        }
    }

    void InspectorWidget::CollapseAll()
    {
        for (auto& groupPair : m_groups)
        {
            groupPair.second.m_header->SetExpanded(false);
            groupPair.second.m_panel->setVisible(false);
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
