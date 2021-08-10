/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/FilteredSearchWidget.h>
#include "ParameterWidget.h"
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include <MCore/Source/StringConversions.h>
#include <MCore/Source/LogManager.h>
#include <QLabel>
#include <QSizePolicy>
#include <QTreeWidget>
#include <QPixmap>
#include <QPushButton>
#include <QVBoxLayout>
#include <QIcon>
#include <QLineEdit>
#include <QHeaderView>


namespace EMStudio
{
    // constructor
    ParameterWidget::ParameterWidget(QWidget* parent, bool useSingleSelection)
        : QWidget(parent)
    {
        // create the display button group
        m_searchWidget = new AzQtComponents::FilteredSearchWidget(this);
        connect(m_searchWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged, this, &ParameterWidget::OnTextFilterChanged);

        QVBoxLayout* layout = new QVBoxLayout();
        layout->setMargin(0);

        // create the tree widget
        m_treeWidget = new QTreeWidget();

        // create header items
        m_treeWidget->setColumnCount(1);
        QStringList headerList;
        headerList.append("Name");
        m_treeWidget->setHeaderLabels(headerList);

        // set optical stuff for the tree
        m_treeWidget->setSortingEnabled(false);
        m_treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
        m_treeWidget->setMinimumWidth(620);
        m_treeWidget->setMinimumHeight(500);
        m_treeWidget->setAlternatingRowColors(true);
        m_treeWidget->setExpandsOnDoubleClick(true);
        m_treeWidget->setAnimated(true);

        // disable the move of section to have column order fixed
        m_treeWidget->header()->setSectionsMovable(false);

        layout->addWidget(m_searchWidget);
        layout->addWidget(m_treeWidget);
        setLayout(layout);

        connect(m_treeWidget, &QTreeWidget::itemSelectionChanged, this, &ParameterWidget::UpdateSelection);
        connect(m_treeWidget, &QTreeWidget::itemDoubleClicked, this, &ParameterWidget::ItemDoubleClicked);

        // set the selection mode
        SetSelectionMode(useSingleSelection);
    }


    ParameterWidget::~ParameterWidget()
    {
    }


    void ParameterWidget::Update(EMotionFX::AnimGraph* animGraph, const AZStd::vector<AZStd::string>& selectedParameters)
    {
        m_animGraph                = animGraph;
        m_selectedParameters       = selectedParameters;
        m_oldSelectedParameters    = selectedParameters;

        Update();
    }


    void ParameterWidget::AddParameterToInterface([[maybe_unused]] EMotionFX::AnimGraph* animGraph, const EMotionFX::Parameter* parameter, QTreeWidgetItem* groupParameterItem)
    {
        // make sure we only show the parameters that are wanted after the name are filtering
        AZStd::string loweredParameter = parameter->GetName();
        AZStd::to_lower(loweredParameter.begin(), loweredParameter.end());
        const bool textFilterPassed = (m_searchWidgetText.empty() || loweredParameter.find(m_searchWidgetText) != AZStd::string::npos);

        const bool typeFilterPassed = (m_filterTypes.empty() || AZStd::find(m_filterTypes.begin(), m_filterTypes.end(), parameter->RTTI_GetType()) != m_filterTypes.end());

        if (textFilterPassed && typeFilterPassed)
        {
            QTreeWidgetItem* item = nullptr;
            if (groupParameterItem)
            {
                item = new QTreeWidgetItem(groupParameterItem);
                groupParameterItem->addChild(item);
            }
            else
            {
                item = new QTreeWidgetItem(m_treeWidget);
                m_treeWidget->addTopLevelItem(item);
            }

            item->setText(0, parameter->GetName().c_str());
            item->setExpanded(true);

            // check if the given parameter is selected
            if (AZStd::find(m_oldSelectedParameters.begin(), m_oldSelectedParameters.end(), parameter->GetName()) != m_oldSelectedParameters.end())
            {
                item->setSelected(true);
            }
        }
    }


    void ParameterWidget::Update()
    {
        m_treeWidget->clear();
        m_treeWidget->blockSignals(true);

        // add all parameters that belong to no group parameter
        const EMotionFX::ValueParameterVector childValueParameters = m_animGraph->GetChildValueParameters();
        for (const EMotionFX::ValueParameter* parameter : childValueParameters)
        {
            AddParameterToInterface(m_animGraph, parameter, nullptr);
        }

        // get all group parameters and iterate through them
        AZStd::string tempString;
        const EMotionFX::GroupParameterVector groupParameters = m_animGraph->RecursivelyGetGroupParameters();
        for (const EMotionFX::GroupParameter* groupParameter : groupParameters)
        {
            // add the group item to the tree widget
            QTreeWidgetItem* groupItem = new QTreeWidgetItem(m_treeWidget);
            groupItem->setText(0, groupParameter->GetName().c_str());

            groupItem->setExpanded(true);
            const EMotionFX::ValueParameterVector childValueParameters2 = groupParameter->GetChildValueParameters();
            tempString = AZStd::string::format("%zu Parameters", childValueParameters2.size());
            groupItem->setToolTip(1, tempString.c_str());
            m_treeWidget->addTopLevelItem(groupItem);

            // add all parameters that belong to the given group
            bool groupSelected = !childValueParameters2.empty();
            for (const EMotionFX::ValueParameter* valueParameter : childValueParameters2)
            {
                AddParameterToInterface(m_animGraph, valueParameter, groupItem);

                // check if the given parameter is selected
                if (groupSelected && AZStd::find(m_oldSelectedParameters.begin(), m_oldSelectedParameters.end(), valueParameter->GetName()) == m_oldSelectedParameters.end())
                {
                    groupSelected = false;
                }
            }

            groupItem->setSelected(groupSelected);
        }

        m_treeWidget->blockSignals(false);
        UpdateSelection();
    }


    void ParameterWidget::UpdateSelection()
    {
        QList<QTreeWidgetItem*> selectedItems = m_treeWidget->selectedItems();

        m_selectedParameters.clear();
        const uint32 numSelectedItems = selectedItems.count();
        m_selectedParameters.reserve(numSelectedItems);

        // Iterate through the selected items in the tree widget.
        AZStd::string itemName;
        for (uint32 i = 0; i < numSelectedItems; ++i)
        {
            QTreeWidgetItem* item = selectedItems[i];
            itemName = item->text(0).toUtf8().data();

            // Get the parameter by name.
            // Skip elements that we can't find as they also shouldn't be selectable.
            const EMotionFX::Parameter* parameter = m_animGraph->FindParameterByName(itemName);
            if (!parameter)
            {
                continue;
            }

            // check if the selected item is a parameter
            if (azrtti_typeid(parameter) != azrtti_typeid<EMotionFX::GroupParameter>())
            {
                if (AZStd::find(m_selectedParameters.begin(), m_selectedParameters.end(), itemName) == m_selectedParameters.end())
                {
                    m_selectedParameters.emplace_back(itemName);
                }
            }
            // selected item is a group
            else 
            {
                // get the number of parameters in the group and iterate through them
                const EMotionFX::GroupParameter* groupParameter = static_cast<const EMotionFX::GroupParameter*>(parameter);
                const EMotionFX::ValueParameterVector valueParameters = groupParameter->RecursivelyGetChildValueParameters();
                for (const EMotionFX::ValueParameter* valueParameter : valueParameters)
                {
                    if (AZStd::find(m_selectedParameters.begin(), m_selectedParameters.end(), valueParameter->GetName()) == m_selectedParameters.end())
                    {
                        m_selectedParameters.emplace_back(valueParameter->GetName());
                    }
                }
            }
        }
    }


    void ParameterWidget::SetSelectionMode(bool useSingleSelection)
    {
        if (useSingleSelection)
        {
            m_treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
        }
        else
        {
            m_treeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
        }

        m_useSingleSelection = useSingleSelection;
    }

    void ParameterWidget::SetFilterTypes(const AZStd::vector<AZ::TypeId>& filterTypes)
    {
        m_filterTypes = filterTypes;
    }

    void ParameterWidget::ItemDoubleClicked(QTreeWidgetItem* item, int column)
    {
        MCORE_UNUSED(item);
        MCORE_UNUSED(column);

        UpdateSelection();
        if (!m_selectedParameters.empty())
        {
            emit OnDoubleClicked(m_selectedParameters[0]);
        }
    }


    void ParameterWidget::OnTextFilterChanged(const QString& text)
    {
        m_searchWidgetText = text.toUtf8().data();
        AZStd::to_lower(m_searchWidgetText.begin(), m_searchWidgetText.end());
        Update();
    }


    void ParameterWidget::FireSelectionDoneSignal()
    {
        emit OnSelectionDone(m_selectedParameters);
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/moc_ParameterWidget.cpp>
