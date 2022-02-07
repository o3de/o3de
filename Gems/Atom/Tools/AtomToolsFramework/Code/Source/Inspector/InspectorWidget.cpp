/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

    void InspectorWidget::AddHeading(QWidget* headingWidget)
    {
        headingWidget->setParent(this);
        m_ui->m_headingContentsLayout->addWidget(headingWidget);
    }

    void InspectorWidget::ClearHeading()
    {
        while (QLayoutItem* child = m_ui->m_headingContentsLayout->takeAt(0))
        {
            delete child->widget();
            delete child;
        }
    }

    void InspectorWidget::Reset()
    {
        while (QLayoutItem* child = m_ui->m_groupContentsLayout->takeAt(0))
        {
            delete child->widget();
            delete child;
        }
        m_groups.clear();
    }

    void InspectorWidget::AddGroupsBegin()
    {
        setVisible(false);

        Reset();
    }

    void InspectorWidget::AddGroupsEnd()
    {
        m_ui->m_groupContentsLayout->addStretch();

        // Scroll to top whenever there is new content
        m_ui->m_groupScrollArea->verticalScrollBar()->setValue(m_ui->m_groupScrollArea->verticalScrollBar()->minimum());

        setVisible(true);
    }

    void InspectorWidget::AddGroup(
        const AZStd::string& groupName,
        const AZStd::string& groupDisplayName,
        const AZStd::string& groupDescription,
        QWidget* groupWidget)
    {
        InspectorGroupHeaderWidget* groupHeader = new InspectorGroupHeaderWidget(m_ui->m_groupContents);
        groupHeader->setText(groupDisplayName.c_str());
        groupHeader->setToolTip(groupDescription.c_str());
        m_ui->m_groupContentsLayout->addWidget(groupHeader);

        groupWidget->setObjectName(groupName.c_str());
        groupWidget->setParent(m_ui->m_groupContents);
        m_ui->m_groupContentsLayout->addWidget(groupWidget);

        m_groups[groupName] = {groupHeader, groupWidget};

        connect(groupHeader, &InspectorGroupHeaderWidget::clicked, this, [this, groupName](QMouseEvent* event) {
            OnHeaderClicked(groupName, event);
        });
        connect(groupHeader, &InspectorGroupHeaderWidget::expanded, this, [this, groupName]() { OnGroupExpanded(groupName); });
        connect(groupHeader, &InspectorGroupHeaderWidget::collapsed, this, [this, groupName]() { OnGroupCollapsed(groupName); });

        if (ShouldGroupAutoExpanded(groupName))
        {
            ExpandGroup(groupName);
        }
        else
        {
            CollapseGroup(groupName);
        }
    }
    
    void InspectorWidget::SetGroupVisible(const AZStd::string& groupName, bool visible)
    {
        auto groupItr = m_groups.find(groupName);
        if (groupItr != m_groups.end())
        {
            groupItr->second.m_header->setVisible(visible);
            groupItr->second.m_panel->setVisible(visible && groupItr->second.m_header->IsExpanded());
        }
    }
    
    bool InspectorWidget::IsGroupVisible(const AZStd::string& groupName) const
    {
        auto groupItr = m_groups.find(groupName);
        return groupItr != m_groups.end() ? groupItr->second.m_header->isVisible() : false;
    }
    
    bool InspectorWidget::IsGroupHidden(const AZStd::string& groupName) const
    {
        auto groupItr = m_groups.find(groupName);
        return groupItr != m_groups.end() ? groupItr->second.m_header->isHidden() : false;
    }

    void InspectorWidget::RefreshGroup(const AZStd::string& groupName)
    {
        for (auto groupWidget : m_ui->m_groupContents->findChildren<InspectorGroupWidget*>(groupName.c_str()))
        {
            groupWidget->Refresh();
        }
    }

    void InspectorWidget::RebuildGroup(const AZStd::string& groupName)
    {
        for (auto groupWidget : m_ui->m_groupContents->findChildren<InspectorGroupWidget*>(groupName.c_str()))
        {
            groupWidget->Rebuild();
        }
    }

    void InspectorWidget::RefreshAll()
    {
        for (auto groupWidget : m_ui->m_groupContents->findChildren<InspectorGroupWidget*>())
        {
            groupWidget->Refresh();
        }
    }

    void InspectorWidget::RebuildAll()
    {
        for (auto groupWidget : m_ui->m_groupContents->findChildren<InspectorGroupWidget*>())
        {
            groupWidget->Rebuild();
        }
    }

    void InspectorWidget::ExpandGroup(const AZStd::string& groupName)
    {
        auto groupItr = m_groups.find(groupName);
        if (groupItr != m_groups.end())
        {
            groupItr->second.m_header->SetExpanded(true);
            groupItr->second.m_panel->setVisible(true);
        }
    }

    void InspectorWidget::CollapseGroup(const AZStd::string& groupName)
    {
        auto groupItr = m_groups.find(groupName);
        if (groupItr != m_groups.end())
        {
            groupItr->second.m_header->SetExpanded(false);
            groupItr->second.m_panel->setVisible(false);
        }
    }

    bool InspectorWidget::IsGroupExpanded(const AZStd::string& groupName) const
    {
        auto groupItr = m_groups.find(groupName);
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

    bool InspectorWidget::ShouldGroupAutoExpanded(const AZStd::string& groupName) const
    {
        AZ_UNUSED(groupName);
        return true;
    }

    void InspectorWidget::OnGroupExpanded(const AZStd::string& groupName)
    {
        AZ_UNUSED(groupName);
    }

    void InspectorWidget::OnGroupCollapsed(const AZStd::string& groupName)
    {
        AZ_UNUSED(groupName);
    }

    void InspectorWidget::OnHeaderClicked(const AZStd::string& groupName, QMouseEvent* event)
    {
        if (event->button() == Qt::MouseButton::LeftButton)
        {
            if (!IsGroupExpanded(groupName))
            {
                ExpandGroup(groupName);
            }
            else
            {
                CollapseGroup(groupName);
            }
            return;
        }

        if (event->button() == Qt::MouseButton::RightButton)
        {
            QMenu menu;
            menu.addAction("Expand", [this, groupName]() { ExpandGroup(groupName); })->setEnabled(!IsGroupExpanded(groupName));
            menu.addAction("Collapse", [this, groupName]() { CollapseGroup(groupName); })->setEnabled(IsGroupExpanded(groupName));
            menu.addAction("Expand All", [this]() { ExpandAll(); });
            menu.addAction("Collapse All", [this]() { CollapseAll(); });
            menu.exec(event->globalPos());
            return;
        }
    }
} // namespace AtomToolsFramework

#include <AtomToolsFramework/Inspector/moc_InspectorWidget.cpp>
