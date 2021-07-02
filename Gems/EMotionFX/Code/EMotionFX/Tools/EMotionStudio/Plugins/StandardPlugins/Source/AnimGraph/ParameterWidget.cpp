/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        mTreeWidget = new QTreeWidget();

        // create header items
        mTreeWidget->setColumnCount(1);
        QStringList headerList;
        headerList.append("Name");
        mTreeWidget->setHeaderLabels(headerList);

        // set optical stuff for the tree
        mTreeWidget->setSortingEnabled(false);
        mTreeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
        mTreeWidget->setMinimumWidth(620);
        mTreeWidget->setMinimumHeight(500);
        mTreeWidget->setAlternatingRowColors(true);
        mTreeWidget->setExpandsOnDoubleClick(true);
        mTreeWidget->setAnimated(true);

        // disable the move of section to have column order fixed
        mTreeWidget->header()->setSectionsMovable(false);

        layout->addWidget(m_searchWidget);
        layout->addWidget(mTreeWidget);
        setLayout(layout);

        connect(mTreeWidget, &QTreeWidget::itemSelectionChanged, this, &ParameterWidget::UpdateSelection);
        connect(mTreeWidget, &QTreeWidget::itemDoubleClicked, this, &ParameterWidget::ItemDoubleClicked);

        // set the selection mode
        SetSelectionMode(useSingleSelection);
    }


    ParameterWidget::~ParameterWidget()
    {
    }


    void ParameterWidget::Update(EMotionFX::AnimGraph* animGraph, const AZStd::vector<AZStd::string>& selectedParameters)
    {
        mAnimGraph                = animGraph;
        mSelectedParameters       = selectedParameters;
        mOldSelectedParameters    = selectedParameters;

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
                item = new QTreeWidgetItem(mTreeWidget);
                mTreeWidget->addTopLevelItem(item);
            }

            item->setText(0, parameter->GetName().c_str());
            item->setExpanded(true);

            // check if the given parameter is selected
            if (AZStd::find(mOldSelectedParameters.begin(), mOldSelectedParameters.end(), parameter->GetName()) != mOldSelectedParameters.end())
            {
                item->setSelected(true);
            }
        }
    }


    void ParameterWidget::Update()
    {
        mTreeWidget->clear();
        mTreeWidget->blockSignals(true);

        // add all parameters that belong to no group parameter
        const EMotionFX::ValueParameterVector childValueParameters = mAnimGraph->GetChildValueParameters();
        for (const EMotionFX::ValueParameter* parameter : childValueParameters)
        {
            AddParameterToInterface(mAnimGraph, parameter, nullptr);
        }

        // get all group parameters and iterate through them
        AZStd::string tempString;
        const EMotionFX::GroupParameterVector groupParameters = mAnimGraph->RecursivelyGetGroupParameters();
        for (const EMotionFX::GroupParameter* groupParameter : groupParameters)
        {
            // add the group item to the tree widget
            QTreeWidgetItem* groupItem = new QTreeWidgetItem(mTreeWidget);
            groupItem->setText(0, groupParameter->GetName().c_str());

            groupItem->setExpanded(true);
            const EMotionFX::ValueParameterVector childValueParameters2 = groupParameter->GetChildValueParameters();
            tempString = AZStd::string::format("%zu Parameters", childValueParameters2.size());
            groupItem->setToolTip(1, tempString.c_str());
            mTreeWidget->addTopLevelItem(groupItem);

            // add all parameters that belong to the given group
            bool groupSelected = !childValueParameters2.empty();
            for (const EMotionFX::ValueParameter* valueParameter : childValueParameters2)
            {
                AddParameterToInterface(mAnimGraph, valueParameter, groupItem);

                // check if the given parameter is selected
                if (groupSelected && AZStd::find(mOldSelectedParameters.begin(), mOldSelectedParameters.end(), valueParameter->GetName()) == mOldSelectedParameters.end())
                {
                    groupSelected = false;
                }
            }

            groupItem->setSelected(groupSelected);
        }

        mTreeWidget->blockSignals(false);
        UpdateSelection();
    }


    void ParameterWidget::UpdateSelection()
    {
        QList<QTreeWidgetItem*> selectedItems = mTreeWidget->selectedItems();

        mSelectedParameters.clear();
        const uint32 numSelectedItems = selectedItems.count();
        mSelectedParameters.reserve(numSelectedItems);

        // Iterate through the selected items in the tree widget.
        AZStd::string itemName;
        for (uint32 i = 0; i < numSelectedItems; ++i)
        {
            QTreeWidgetItem* item = selectedItems[i];
            itemName = item->text(0).toUtf8().data();

            // Get the parameter by name.
            // Skip elements that we can't find as they also shouldn't be selectable.
            const EMotionFX::Parameter* parameter = mAnimGraph->FindParameterByName(itemName);
            if (!parameter)
            {
                continue;
            }

            // check if the selected item is a parameter
            if (azrtti_typeid(parameter) != azrtti_typeid<EMotionFX::GroupParameter>())
            {
                if (AZStd::find(mSelectedParameters.begin(), mSelectedParameters.end(), itemName) == mSelectedParameters.end())
                {
                    mSelectedParameters.emplace_back(itemName);
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
                    if (AZStd::find(mSelectedParameters.begin(), mSelectedParameters.end(), valueParameter->GetName()) == mSelectedParameters.end())
                    {
                        mSelectedParameters.emplace_back(valueParameter->GetName());
                    }
                }
            }
        }
    }


    void ParameterWidget::SetSelectionMode(bool useSingleSelection)
    {
        if (useSingleSelection)
        {
            mTreeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
        }
        else
        {
            mTreeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
        }

        mUseSingleSelection = useSingleSelection;
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
        if (!mSelectedParameters.empty())
        {
            emit OnDoubleClicked(mSelectedParameters[0]);
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
        emit OnSelectionDone(mSelectedParameters);
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/moc_ParameterWidget.cpp>
