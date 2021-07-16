/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/FilteredSearchWidget.h>
#include "SimulatedObjectSelectionWidget.h"
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <MCore/Source/StringConversions.h>
#include <MCore/Source/LogManager.h>
#include <EMotionFX/Source/SimulatedObjectSetup.h>
#include <QLabel>
#include <QSizePolicy>
#include <QTreeWidget>
#include <QPixmap>
#include <QPushButton>
#include <QVBoxLayout>
#include <QIcon>
#include <QHeaderView>


namespace EMStudio
{
    SimulatedObjectSelectionWidget::SimulatedObjectSelectionWidget(QWidget* parent)
        : QWidget(parent)
    {
        m_searchWidget = new AzQtComponents::FilteredSearchWidget(this);
        connect(m_searchWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged, this, &SimulatedObjectSelectionWidget::OnTextFilterChanged);

        m_treeWidget = new QTreeWidget();

        m_treeWidget->setColumnCount(1);
        const QStringList headerList { "Name" };
        m_treeWidget->setHeaderLabels(headerList);

        m_treeWidget->setSortingEnabled(false);
        m_treeWidget->setSelectionMode(QAbstractItemView::MultiSelection);
        m_treeWidget->setAlternatingRowColors(true);
        m_treeWidget->setExpandsOnDoubleClick(true);
        m_treeWidget->setAnimated(true);
        m_treeWidget->header()->setSectionsMovable(false);

        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setMargin(0);
        layout->addWidget(m_searchWidget);
        layout->addWidget(m_treeWidget);

        connect(m_treeWidget, &QTreeWidget::itemSelectionChanged, this, &SimulatedObjectSelectionWidget::UpdateSelection);
        connect(m_treeWidget, &QTreeWidget::itemDoubleClicked, this, &SimulatedObjectSelectionWidget::ItemDoubleClicked);
    }

    void SimulatedObjectSelectionWidget::Update(EMotionFX::Actor* actor, const AZStd::vector<AZStd::string>& selectedSimulatedObjects)
    {
        m_actor                           = actor;
        m_selectedSimulatedObjectNames    = selectedSimulatedObjects;
        m_oldSelectedSimulatedObjectNames = selectedSimulatedObjects;

        Update();
    }

    void SimulatedObjectSelectionWidget::AddSimulatedObjectToInterface(const EMotionFX::SimulatedObject* object)
    {
        // Make sure we only show the simulated objects that are wanted after the name are filtering
        if (m_searchWidgetText.empty() || AzFramework::StringFunc::Find(object->GetName(), m_searchWidgetText) != AZStd::string::npos)
        {
            QTreeWidgetItem* item = new QTreeWidgetItem(m_treeWidget);
            m_treeWidget->addTopLevelItem(item);

            item->setText(0, object->GetName().c_str());
            item->setExpanded(true);

            // Check if the given object is selected
            if (AZStd::find(m_oldSelectedSimulatedObjectNames.begin(), m_oldSelectedSimulatedObjectNames.end(), object->GetName()) != m_oldSelectedSimulatedObjectNames.end())
            {
                item->setSelected(true);
            }
        }
    }

    void SimulatedObjectSelectionWidget::Update()
    {
        m_treeWidget->clear();
        m_treeWidget->blockSignals(true);

        const AZStd::vector<EMotionFX::SimulatedObject*>& simulatedObjects = m_actor->GetSimulatedObjectSetup()->GetSimulatedObjects();
        for (const EMotionFX::SimulatedObject* simulatedObject : simulatedObjects)
        {
            AddSimulatedObjectToInterface(simulatedObject);
        }

        m_treeWidget->blockSignals(false);
        UpdateSelection();
    }

    AZStd::vector<AZStd::string>& SimulatedObjectSelectionWidget::GetSelectedSimulatedObjectNames()
    {
        UpdateSelection(); 
        return m_selectedSimulatedObjectNames;
    }

    void SimulatedObjectSelectionWidget::UpdateSelection()
    {
        QList<QTreeWidgetItem*> selectedItems = m_treeWidget->selectedItems();

        m_selectedSimulatedObjectNames.clear();
        const uint32 numSelectedItems = selectedItems.count();
        m_selectedSimulatedObjectNames.reserve(numSelectedItems);

        // Iterate through the selected items in the tree widget.
        AZStd::string itemName;
        for (uint32 i = 0; i < numSelectedItems; ++i)
        {
            QTreeWidgetItem* item = selectedItems[i];
            itemName = item->text(0).toUtf8().data();

            // Skip the object that we can't find as they also shouldn't be selectable.
            const EMotionFX::SimulatedObject* object = m_actor->GetSimulatedObjectSetup()->FindSimulatedObjectByName(itemName.c_str());
            if (!object)
            {
                continue;
            }

            // Check if the selected item is a simulated object
            if (AZStd::find(m_selectedSimulatedObjectNames.begin(), m_selectedSimulatedObjectNames.end(), itemName) == m_selectedSimulatedObjectNames.end())
            {
                m_selectedSimulatedObjectNames.emplace_back(itemName);
            }
        }
    }

    void SimulatedObjectSelectionWidget::ItemDoubleClicked(QTreeWidgetItem* item, int column)
    {
        AZ_UNUSED(item);
        AZ_UNUSED(column);

        UpdateSelection();
        if (!m_selectedSimulatedObjectNames.empty())
        {
            emit OnDoubleClicked(m_selectedSimulatedObjectNames[0]);
        }
    }

    void SimulatedObjectSelectionWidget::OnTextFilterChanged(const QString& text)
    {
        m_searchWidgetText = text.toUtf8().data();
        AZStd::to_lower(m_searchWidgetText.begin(), m_searchWidgetText.end());
        Update();
    }
} // namespace EMStudio
