/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        m_currentSelectionList = selectionList;
        if (selectionList == nullptr)
        {
            m_currentSelectionList = &(GetCommandManager()->GetCurrentSelection());
        }

        QVBoxLayout* layout = new QVBoxLayout();
        layout->setMargin(0);

        // create the display button group
        m_searchWidget = new AzQtComponents::FilteredSearchWidget(this);
        connect(m_searchWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged, this, &MotionSetHierarchyWidget::OnTextFilterChanged);

        // create the tree widget
        m_hierarchy = new QTreeWidget();

        // create header items
        m_hierarchy->setColumnCount(2);
        QStringList headerList;
        headerList.append("ID");
        headerList.append("FileName");
        m_hierarchy->setHeaderLabels(headerList);

        // set optical stuff for the tree
        m_hierarchy->setColumnWidth(0, 400);
        m_hierarchy->setSortingEnabled(false);
        m_hierarchy->setSelectionMode(QAbstractItemView::SingleSelection);
        m_hierarchy->setMinimumWidth(620);
        m_hierarchy->setMinimumHeight(500);
        m_hierarchy->setAlternatingRowColors(true);
        m_hierarchy->setExpandsOnDoubleClick(true);
        m_hierarchy->setAnimated(true);

        // disable the move of section to have column order fixed
        m_hierarchy->header()->setSectionsMovable(false);

        layout->addWidget(m_searchWidget);
        layout->addWidget(m_hierarchy);
        setLayout(layout);

        connect(m_hierarchy, &QTreeWidget::itemSelectionChanged, this, &MotionSetHierarchyWidget::UpdateSelection);
        connect(m_hierarchy, &QTreeWidget::itemDoubleClicked, this, &MotionSetHierarchyWidget::ItemDoubleClicked);

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
        m_motionSet = motionSet;
        m_currentSelectionList = selectionList;

        if (selectionList == nullptr)
        {
            m_currentSelectionList = &(GetCommandManager()->GetCurrentSelection());
        }

        Update();
    }


    // update the widget
    void MotionSetHierarchyWidget::Update()
    {
        m_hierarchy->clear();

        m_hierarchy->blockSignals(true);
        if (m_motionSet)
        {
            AddMotionSetWithParents(m_motionSet);
        }
        else
        {
            // add all root motion sets
            const size_t numMotionSets = EMotionFX::GetMotionManager().GetNumMotionSets();
            for (size_t i = 0; i < numMotionSets; ++i)
            {
                EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().GetMotionSet(i);

                if (motionSet->GetIsOwnedByRuntime())
                {
                    continue;
                }

                if (motionSet->GetParentSet() == nullptr)
                {
                    RecursiveAddMotionSet(nullptr, EMotionFX::GetMotionManager().GetMotionSet(i), m_currentSelectionList);
                }
            }
        }

        m_hierarchy->blockSignals(false);
        UpdateSelection();
    }


    void MotionSetHierarchyWidget::RecursiveAddMotionSet(QTreeWidgetItem* parent, EMotionFX::MotionSet* motionSet, CommandSystem::SelectionList* selectionList)
    {
        // create the root item
        QTreeWidgetItem* motionSetItem;
        if (parent == nullptr)
        {
            motionSetItem = new QTreeWidgetItem(m_hierarchy);
            m_hierarchy->addTopLevelItem(motionSetItem);
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
        const size_t numChildSets = motionSet->GetNumChildSets();
        for (size_t i = 0; i < numChildSets; ++i)
        {
            RecursiveAddMotionSet(motionSetItem, motionSet->GetChildSet(i), selectionList);
        }
    }


    void MotionSetHierarchyWidget::AddMotionSetWithParents(EMotionFX::MotionSet* motionSet)
    {
        // create the motion set item
        QTreeWidgetItem* motionSetItem = new QTreeWidgetItem(m_hierarchy);

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
            QTreeWidgetItem* parentMotionSetItem = new QTreeWidgetItem(m_hierarchy);

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
            parentMotionSetItem->addChild(m_hierarchy->takeTopLevelItem(m_hierarchy->indexOfTopLevelItem(motionSetItem)));
            motionSetItem = parentMotionSetItem;

            // set the next parent motion set
            parentMotionSet = parentMotionSet->GetParentSet();
        }

        // expand all to show all items
        m_hierarchy->expandAll();
    }


    void MotionSetHierarchyWidget::Select(const AZStd::vector<MotionSetSelectionItem>& selectedItems)
    {
        m_selected = selectedItems;

        for (const MotionSetSelectionItem& selectionItem : selectedItems)
        {
            const AZStd::string& motionId = selectionItem.m_motionId;

            QTreeWidgetItemIterator itemIterator(m_hierarchy);
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
        QList<QTreeWidgetItem*> selectedItems = m_hierarchy->selectedItems();

        // Reset the selection.
        m_selected.clear();
        m_selected.reserve(selectedItems.size());

        AZStd::string motionId;
        for (const QTreeWidgetItem* item : selectedItems)
        {
             motionId = item->text(0).toUtf8().data();

            // Extract the motion set id.
            QString motionSetIdAsString = item->whatsThis(0);
            const uint32 motionSetId = AzFramework::StringFunc::ToInt(motionSetIdAsString.toUtf8().data());

            // Find the motion set based on the id.
            EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().FindMotionSetByID(motionSetId);
            if (!motionSet)
            {
                continue;
            }

            MotionSetSelectionItem selectionItem(motionId, motionSet);
            m_selected.push_back(selectionItem);
        }
    }


    void MotionSetHierarchyWidget::SetSelectionMode(bool useSingleSelection)
    {
        if (useSingleSelection)
        {
            m_hierarchy->setSelectionMode(QAbstractItemView::SingleSelection);
        }
        else
        {
            m_hierarchy->setSelectionMode(QAbstractItemView::ExtendedSelection);
        }

        m_useSingleSelection = useSingleSelection;
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
        emit SelectionChanged(m_selected);
    }


    AZStd::vector<MotionSetSelectionItem>& MotionSetHierarchyWidget::GetSelectedItems()
    {
        UpdateSelection();
        return m_selected;
    }


    AZStd::vector<AZStd::string> MotionSetHierarchyWidget::GetSelectedMotionIds(EMotionFX::MotionSet* motionSet)
    {
        const AZStd::vector<MotionSetSelectionItem>& selectedItems = GetSelectedItems();

        AZStd::vector<AZStd::string> result;
        result.reserve(selectedItems.size());

        for (const MotionSetSelectionItem& selectedItem : selectedItems)
        {
            if (selectedItem.m_motionSet == motionSet)
            {
                result.push_back(selectedItem.m_motionId);
            }
        }

        return result;    
    }

    void MotionSetHierarchyWidget::SelectItemsWithText(QString text)
    {
        QList<QTreeWidgetItem*> items = m_hierarchy->findItems(text, Qt::MatchWrap | Qt::MatchWildcard | Qt::MatchRecursive);

        m_hierarchy->clearSelection();

        for (QTreeWidgetItem* item : items)
        {
            item->setSelected(true);
        }
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/moc_MotionSetHierarchyWidget.cpp>
