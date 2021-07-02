/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include <AzQtComponents/Components/FilteredSearchWidget.h>
#include "MotionSetHierarchyWidget.h"
#include "EMStudioManager.h"
#include <MCore/Source/LogManager.h>
#include <MCore/Source/StringConversions.h>
#include <EMotionFX/Source/Mesh.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/MotionSet.h>
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
    MotionSetHierarchyWidget::MotionSetHierarchyWidget(QWidget* parent, bool useSingleSelection, CommandSystem::SelectionList* selectionList)
        : QWidget(parent)
    {
        mCurrentSelectionList = selectionList;
        if (selectionList == nullptr)
        {
            mCurrentSelectionList = &(GetCommandManager()->GetCurrentSelection());
        }

        QVBoxLayout* layout = new QVBoxLayout();
        layout->setMargin(0);

        // create the display button group
        m_searchWidget = new AzQtComponents::FilteredSearchWidget(this);
        connect(m_searchWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged, this, &MotionSetHierarchyWidget::OnTextFilterChanged);

        // create the tree widget
        mHierarchy = new QTreeWidget();

        // create header items
        mHierarchy->setColumnCount(2);
        QStringList headerList;
        headerList.append("ID");
        headerList.append("FileName");
        mHierarchy->setHeaderLabels(headerList);

        // set optical stuff for the tree
        mHierarchy->setColumnWidth(0, 400);
        mHierarchy->setSortingEnabled(false);
        mHierarchy->setSelectionMode(QAbstractItemView::SingleSelection);
        mHierarchy->setMinimumWidth(620);
        mHierarchy->setMinimumHeight(500);
        mHierarchy->setAlternatingRowColors(true);
        mHierarchy->setExpandsOnDoubleClick(true);
        mHierarchy->setAnimated(true);

        // disable the move of section to have column order fixed
        mHierarchy->header()->setSectionsMovable(false);

        layout->addWidget(m_searchWidget);
        layout->addWidget(mHierarchy);
        setLayout(layout);

        connect(mHierarchy, &QTreeWidget::itemSelectionChanged, this, &MotionSetHierarchyWidget::UpdateSelection);
        connect(mHierarchy, &QTreeWidget::itemDoubleClicked, this, &MotionSetHierarchyWidget::ItemDoubleClicked);

        // connect the window activation signal to refresh if reactivated
        //connect( this, SIGNAL(visibilityChanged(bool)), this, SLOT(OnVisibilityChanged(bool)) );

        SetSelectionMode(useSingleSelection);
    }


    // destructor
    MotionSetHierarchyWidget::~MotionSetHierarchyWidget()
    {
    }


    // update from a motion set and selection list
    void MotionSetHierarchyWidget::Update(EMotionFX::MotionSet* motionSet, CommandSystem::SelectionList* selectionList)
    {
        mMotionSet = motionSet;
        mCurrentSelectionList = selectionList;

        if (selectionList == nullptr)
        {
            mCurrentSelectionList = &(GetCommandManager()->GetCurrentSelection());
        }

        Update();
    }


    // update the widget
    void MotionSetHierarchyWidget::Update()
    {
        mHierarchy->clear();

        mHierarchy->blockSignals(true);
        if (mMotionSet)
        {
            AddMotionSetWithParents(mMotionSet);
        }
        else
        {
            // add all root motion sets
            const uint32 numMotionSets = EMotionFX::GetMotionManager().GetNumMotionSets();
            for (uint32 i = 0; i < numMotionSets; ++i)
            {
                EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().GetMotionSet(i);

                if (motionSet->GetIsOwnedByRuntime())
                {
                    continue;
                }

                if (motionSet->GetParentSet() == nullptr)
                {
                    RecursiveAddMotionSet(nullptr, EMotionFX::GetMotionManager().GetMotionSet(i), mCurrentSelectionList);
                }
            }
        }

        mHierarchy->blockSignals(false);
        UpdateSelection();
    }


    void MotionSetHierarchyWidget::RecursiveAddMotionSet(QTreeWidgetItem* parent, EMotionFX::MotionSet* motionSet, CommandSystem::SelectionList* selectionList)
    {
        // create the root item
        QTreeWidgetItem* motionSetItem;
        if (parent == nullptr)
        {
            motionSetItem = new QTreeWidgetItem(mHierarchy);
            mHierarchy->addTopLevelItem(motionSetItem);
        }
        else
        {
            motionSetItem = new QTreeWidgetItem(parent);
        }

        // set the name
        motionSetItem->setText(0, motionSet->GetName());
        motionSetItem->setText(1, motionSet->GetFilename());
        motionSetItem->setWhatsThis(0, QString("%1").arg(MCORE_INVALIDINDEX32));
        motionSetItem->setExpanded(true);

        const EMotionFX::MotionSet::MotionEntries& motionEntries = motionSet->GetMotionEntries();
        for (const auto& item : motionEntries)
        {
            const EMotionFX::MotionSet::MotionEntry* motionEntry = item.second;

            // Check if the id is valid and don't put it to the list in case it is not.
            if (motionEntry->GetId().empty())
            {
                continue;
            }

            if (m_searchWidgetText.empty() ||
                AzFramework::StringFunc::Find(motionEntry->GetId().c_str(), m_searchWidgetText.c_str()) != AZStd::string::npos ||
                AzFramework::StringFunc::Find(motionEntry->GetFilename(), m_searchWidgetText.c_str()) != AZStd::string::npos)
            {
                QTreeWidgetItem* newItem = new QTreeWidgetItem(motionSetItem);

                newItem->setText(0, motionEntry->GetId().c_str());
                newItem->setText(1, motionEntry->GetFilename());

                newItem->setWhatsThis(0, QString("%1").arg(motionSet->GetID()));

                newItem->setExpanded(true);
            }
        }

        // add all child sets
        const uint32 numChildSets = motionSet->GetNumChildSets();
        for (uint32 i = 0; i < numChildSets; ++i)
        {
            RecursiveAddMotionSet(motionSetItem, motionSet->GetChildSet(i), selectionList);
        }
    }


    void MotionSetHierarchyWidget::AddMotionSetWithParents(EMotionFX::MotionSet* motionSet)
    {
        // create the motion set item
        QTreeWidgetItem* motionSetItem = new QTreeWidgetItem(mHierarchy);

        // set the name
        motionSetItem->setText(0, motionSet->GetName());
        motionSetItem->setText(1, motionSet->GetFilename());
        motionSetItem->setWhatsThis(0, QString("%1").arg(MCORE_INVALIDINDEX32));

        const EMotionFX::MotionSet::MotionEntries& motionEntries = motionSet->GetMotionEntries();
        for (const auto& item : motionEntries)
        {
            const EMotionFX::MotionSet::MotionEntry* motionEntry = item.second;

            // Check if the id is valid and don't put it to the list in case it is not.
            if (motionEntry->GetId().empty())
            {
                continue;
            }

            if (m_searchWidgetText.empty() ||
                AzFramework::StringFunc::Find(motionEntry->GetId().c_str(), m_searchWidgetText.c_str()) != AZStd::string::npos ||
                AzFramework::StringFunc::Find(motionEntry->GetFilename(), m_searchWidgetText.c_str()) != AZStd::string::npos)
            {
                QTreeWidgetItem* newItem = new QTreeWidgetItem(motionSetItem);

                newItem->setText(0, motionEntry->GetId().c_str());
                newItem->setText(1, motionEntry->GetFilename());

                newItem->setWhatsThis(0, QString("%1").arg(motionSet->GetID()));
            }
        }

        // add each parent
        EMotionFX::MotionSet* parentMotionSet = motionSet->GetParentSet();
        while (parentMotionSet)
        {
            // create the motion set item
            QTreeWidgetItem* parentMotionSetItem = new QTreeWidgetItem(mHierarchy);

            // set the name
            parentMotionSetItem->setText(0, parentMotionSet->GetName());
            parentMotionSetItem->setText(1, parentMotionSet->GetFilename());
            parentMotionSetItem->setWhatsThis(0, QString("%1").arg(MCORE_INVALIDINDEX32));

            const EMotionFX::MotionSet::MotionEntries& parentMotionEntries = parentMotionSet->GetMotionEntries();
            for (const auto& item : parentMotionEntries)
            {
                const EMotionFX::MotionSet::MotionEntry* motionEntry = item.second;

                // Check if the id is valid and don't put it to the list in case it is not.
                if (motionEntry->GetId().empty())
                {
                    continue;
                }

                if (m_searchWidgetText.empty() ||
                    AzFramework::StringFunc::Find(motionEntry->GetId().c_str(), m_searchWidgetText.c_str()) != AZStd::string::npos ||
                    AzFramework::StringFunc::Find(motionEntry->GetFilename(), m_searchWidgetText.c_str()) != AZStd::string::npos)
                {
                    QTreeWidgetItem* newItem = new QTreeWidgetItem(parentMotionSetItem);

                    newItem->setText(0, motionEntry->GetId().c_str());
                    newItem->setText(1, motionEntry->GetFilename());

                    newItem->setWhatsThis(0, QString("%1").arg(parentMotionSet->GetID()));
                }
            }

            // add the last motion set item as child and set this parent as last motion set item
            parentMotionSetItem->addChild(mHierarchy->takeTopLevelItem(mHierarchy->indexOfTopLevelItem(motionSetItem)));
            motionSetItem = parentMotionSetItem;

            // set the next parent motion set
            parentMotionSet = parentMotionSet->GetParentSet();
        }

        // expand all to show all items
        mHierarchy->expandAll();
    }


    void MotionSetHierarchyWidget::Select(const AZStd::vector<MotionSetSelectionItem>& selectedItems)
    {
        mSelected = selectedItems;

        for (const MotionSetSelectionItem& selectionItem : selectedItems)
        {
            const AZStd::string& motionId = selectionItem.mMotionId;

            QTreeWidgetItemIterator itemIterator(mHierarchy);
            while (*itemIterator)
            {
                QTreeWidgetItem* item = *itemIterator;
                if (item->text(0) == motionId.c_str())
                {
                    item->setSelected(true);
                    break;
                }
                ++itemIterator;
            }
        }
    }


    void MotionSetHierarchyWidget::UpdateSelection()
    {
        // Get the selected items in the tree widget.
        QList<QTreeWidgetItem*> selectedItems = mHierarchy->selectedItems();
        const uint32 numSelectedItems = selectedItems.count();

        // Reset the selection.
        mSelected.clear();
        mSelected.reserve(numSelectedItems);

        AZStd::string motionId;
        for (uint32 i = 0; i < numSelectedItems; ++i)
        {
            QTreeWidgetItem* item = selectedItems[i];
            motionId = item->text(0).toUtf8().data();

            // Extract the motion set id.
            QString motionSetIdAsString = item->whatsThis(0);
            const AZ::u32 motionSetId = AzFramework::StringFunc::ToInt(motionSetIdAsString.toUtf8().data());

            // Find the motion set based on the id.
            EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().FindMotionSetByID(motionSetId);
            if (!motionSet)
            {
                continue;
            }

            MotionSetSelectionItem selectionItem(motionId, motionSet);
            mSelected.push_back(selectionItem);
        }
    }


    void MotionSetHierarchyWidget::SetSelectionMode(bool useSingleSelection)
    {
        if (useSingleSelection)
        {
            mHierarchy->setSelectionMode(QAbstractItemView::SingleSelection);
        }
        else
        {
            mHierarchy->setSelectionMode(QAbstractItemView::ExtendedSelection);
        }

        mUseSingleSelection = useSingleSelection;
    }


    void MotionSetHierarchyWidget::ItemDoubleClicked(QTreeWidgetItem* item, int column)
    {
        MCORE_UNUSED(item);
        MCORE_UNUSED(column);

        UpdateSelection();
        FireSelectionDoneSignal();
    }


    void MotionSetHierarchyWidget::OnTextFilterChanged(const QString& text)
    {
        m_searchWidgetText = text.toUtf8().data();
        Update();
    }


    void MotionSetHierarchyWidget::FireSelectionDoneSignal()
    {
        emit SelectionChanged(mSelected);
    }


    AZStd::vector<MotionSetSelectionItem>& MotionSetHierarchyWidget::GetSelectedItems()
    {
        UpdateSelection();
        return mSelected;
    }


    AZStd::vector<AZStd::string> MotionSetHierarchyWidget::GetSelectedMotionIds(EMotionFX::MotionSet* motionSet)
    {
        const AZStd::vector<MotionSetSelectionItem>& selectedItems = GetSelectedItems();

        AZStd::vector<AZStd::string> result;
        result.reserve(selectedItems.size());

        for (const MotionSetSelectionItem& selectedItem : selectedItems)
        {
            if (selectedItem.mMotionSet == motionSet)
            {
                result.push_back(selectedItem.mMotionId);
            }
        }

        return result;    
    }

    void MotionSetHierarchyWidget::SelectItemsWithText(QString text)
    {
        QList<QTreeWidgetItem*> items = mHierarchy->findItems(text, Qt::MatchWrap | Qt::MatchWildcard | Qt::MatchRecursive);

        mHierarchy->clearSelection();

        for (QTreeWidgetItem* item : items)
        {
            item->setSelected(true);
        }
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/moc_MotionSetHierarchyWidget.cpp>
