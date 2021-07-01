/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/FilteredSearchWidget.h>
#include "NodeHierarchyWidget.h"
#include "EMStudioManager.h"
#include <EMotionFX/Source/Mesh.h>
#include <EMotionFX/Source/ActorManager.h>
#include <MCore/Source/StringConversions.h>
#include <QLabel>
#include <QSizePolicy>
#include <QTreeWidget>
#include <QPixmap>
#include <QPushButton>
#include <QVBoxLayout>
#include <QIcon>
#include <QLineEdit>
#include <QMenu>
#include <QHeaderView>

EMotionFX::Node* SelectionItem::GetNode() const
{
    EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().FindActorInstanceByID(mActorInstanceID);
    if (!actorInstance)
    {
        return nullptr;
    }

    EMotionFX::Actor* actor = actorInstance->GetActor();
    return actor->GetSkeleton()->FindNodeByName(GetNodeName());
}

namespace EMStudio
{
    NodeHierarchyWidget::NodeHierarchyWidget(QWidget* parent, bool useSingleSelection, bool useDefaultMinWidth)
        : QWidget(parent)
    {
        const auto iconFilename = [](const QString& name) {
            return QStringLiteral("%1/Images/Icons/%2").arg(MysticQt::GetDataDir().c_str()).arg(name);
        };
        const auto boneIconFilename = iconFilename("Bone.svg");
        const auto nodeIconFilename = iconFilename("Node.svg");
        const auto meshIconFilename = iconFilename("Mesh.svg");
        mBoneIcon       = new QIcon(boneIconFilename);
        mNodeIcon       = new QIcon(nodeIconFilename);
        mMeshIcon       = new QIcon(meshIconFilename);
        mCharacterIcon  = new QIcon(iconFilename("Character.svg"));

        mActorInstanceIDs.SetMemoryCategory(MEMCATEGORY_EMSTUDIOSDK);

        QVBoxLayout* layout = new QVBoxLayout();
        layout->setMargin(0);

        m_searchWidget = new AzQtComponents::FilteredSearchWidget(this);
        m_searchWidget->setEnabledFiltersVisible(false);
        m_searchWidget->setTextFilterFillsWidth(true);
        const auto addFilter = [this](const QString& name, const QString& iconFilename, FilterType type) {
            AzQtComponents::SearchTypeFilter filter(tr("Node"), name);
            filter.extraIconFilename = iconFilename;
            filter.enabled = true;
            filter.metadata = static_cast<int>(type);
            m_searchWidget->AddTypeFilter(filter);
        };
        addFilter(tr("Meshes"), meshIconFilename, FilterType::Meshes);
        addFilter(tr("Nodes"), nodeIconFilename, FilterType::Nodes);
        addFilter(tr("Bones"), boneIconFilename, FilterType::Bones);
        mFilterState = {FilterType::Meshes, FilterType::Nodes, FilterType::Bones};
        connect(m_searchWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged, this, &NodeHierarchyWidget::OnTextFilterChanged);
        connect(m_searchWidget, &AzQtComponents::FilteredSearchWidget::TypeFilterChanged, this, [this](const auto& filters) {
            FilterTypes filterState;
            for (const auto& filter : filters)
            {
                filterState.setFlag(static_cast<FilterType>(filter.metadata.toInt()));
            }
            if (filterState == mFilterState)
            {
                return;
            }
            mFilterState = filterState;
            Update();
            emit FilterStateChanged(filterState);
        });
        layout->addWidget(m_searchWidget);

        // create the tree widget
        mHierarchy = new QTreeWidget();

        // create header items
        mHierarchy->setColumnCount(1);

        // set optical stuff for the tree
        mHierarchy->header()->setVisible(false);
        mHierarchy->header()->setStretchLastSection(true);
        mHierarchy->setSortingEnabled(false);
        mHierarchy->setSelectionMode(QAbstractItemView::SingleSelection);

        if (useDefaultMinWidth)
        {
            mHierarchy->setMinimumWidth(500);
        }

        mHierarchy->setMinimumHeight(400);
        mHierarchy->setExpandsOnDoubleClick(true);
        mHierarchy->setAnimated(true);

        // disable the move of section to have column order fixed
        mHierarchy->header()->setSectionsMovable(false);

        if (useSingleSelection == false)
        {
            mHierarchy->setContextMenuPolicy(Qt::CustomContextMenu);
            connect(mHierarchy, &QTreeWidget::customContextMenuRequested, this, &NodeHierarchyWidget::TreeContextMenu);
        }

        layout->addWidget(mHierarchy);
        setLayout(layout);

        connect(mHierarchy, &QTreeWidget::itemSelectionChanged, this, &NodeHierarchyWidget::UpdateSelection);
        connect(mHierarchy, &QTreeWidget::itemDoubleClicked, this, &NodeHierarchyWidget::ItemDoubleClicked);
        connect(mHierarchy, &QTreeWidget::itemSelectionChanged, this, &NodeHierarchyWidget::OnSelectionChanged);

        // connect the window activation signal to refresh if reactivated
        //connect( this, SIGNAL(visibilityChanged(bool)), this, SLOT(OnVisibilityChanged(bool)) );

        // set the selection mode
        SetSelectionMode(useSingleSelection);
    }


    // destructor
    NodeHierarchyWidget::~NodeHierarchyWidget()
    {
        delete mBoneIcon;
        delete mMeshIcon;
        delete mNodeIcon;
        delete mCharacterIcon;
    }


    void NodeHierarchyWidget::Update(const MCore::Array<uint32>& actorInstanceIDs, CommandSystem::SelectionList* selectionList)
    {
        mActorInstanceIDs = actorInstanceIDs;
        ConvertFromSelectionList(selectionList);

        Update();
    }


    void NodeHierarchyWidget::Update(uint32 actorInstanceID, CommandSystem::SelectionList* selectionList)
    {
        mActorInstanceIDs.Clear();

        if (actorInstanceID == MCORE_INVALIDINDEX32)
        {
            // get the number actor instances and iterate over them
            const uint32 numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
            for (uint32 i = 0; i < numActorInstances; ++i)
            {
                // add the actor to the node hierarchy widget
                EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().GetActorInstance(i);

                if (actorInstance->GetIsOwnedByRuntime())
                {
                    continue;
                }

                mActorInstanceIDs.Add(actorInstance->GetID());
            }
        }
        else
        {
            mActorInstanceIDs.Add(actorInstanceID);
        }

        Update(mActorInstanceIDs, selectionList);
    }


    void NodeHierarchyWidget::Update()
    {
        mHierarchy->blockSignals(true);

        // clear the whole thing (don't put this before blockSignals() else we have a bug in the skeletal LOD choosing, before also doesn't make any sense cause the OnNodesChanged() gets called and resets the selection!)
        mHierarchy->clear();

        // get the number actor instances and iterate over them
        const uint32 numActorInstances = mActorInstanceIDs.GetLength();
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            // get the actor instance by its id
            EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().FindActorInstanceByID(mActorInstanceIDs[i]);
            if (actorInstance)
            {
                AddActorInstance(actorInstance);
            }
        }

        mHierarchy->blockSignals(false);

        // after we refilled everything, update the selection
        UpdateSelection();
    }


    void NodeHierarchyWidget::AddActorInstance(EMotionFX::ActorInstance* actorInstance)
    {
        EMotionFX::Actor*   actor       = actorInstance->GetActor();
        AZStd::string       actorName;
        AzFramework::StringFunc::Path::GetFileName(actor->GetFileNameString().c_str(), actorName);
        const uint32        numNodes    = actor->GetNumNodes();

        // extract the bones from the actor
        actor->ExtractBoneList(actorInstance->GetLODLevel(), &mBoneList);

        // calculate the number of polygons and indices
        uint32 numPolygons, numVertices, numIndices;
        actor->CalcMeshTotals(actorInstance->GetLODLevel(), &numPolygons, &numVertices, &numIndices);

        QTreeWidgetItem* rootItem = new QTreeWidgetItem(mHierarchy);

        // select the item in case the actor
        if (CheckIfActorInstanceSelected(actorInstance->GetID()))
        {
            rootItem->setSelected(true);
        }

        rootItem->setText(0, actorName.c_str());
        rootItem->setText(1, "Character");
        rootItem->setText(2, AZStd::to_string(numNodes).c_str());
        rootItem->setText(3, AZStd::to_string(numIndices / 3).c_str());
        rootItem->setText(4, "");
        rootItem->setExpanded(true);
        rootItem->setIcon(0, *mCharacterIcon);
        QString whatsthis = AZStd::to_string(actorInstance->GetID()).c_str();
        rootItem->setWhatsThis(0, whatsthis);

        mHierarchy->addTopLevelItem(rootItem);

        // get the number of root nodes and iterate through them
        const uint32 numRootNodes = actor->GetSkeleton()->GetNumRootNodes();
        for (uint32 i = 0; i < numRootNodes; ++i)
        {
            // get the root node index and the corresponding node
            const uint32        rootNodeIndex   = actor->GetSkeleton()->GetRootNodeIndex(i);
            EMotionFX::Node*    rootNode        = actor->GetSkeleton()->GetNode(rootNodeIndex);

            // recursively add all the nodes to the hierarchy
            RecursivelyAddChilds(rootItem, actor, actorInstance, rootNode);
        }
    }


    bool NodeHierarchyWidget::CheckIfNodeVisible(EMotionFX::ActorInstance* actorInstance, EMotionFX::Node* node)
    {
        if (node == nullptr)
        {
            return false;
        }

        const uint32        nodeIndex   = node->GetNodeIndex();
        AZStd::string       nodeName = node->GetNameString();
        AZStd::to_lower(nodeName.begin(), nodeName.end());
        EMotionFX::Mesh*    mesh        = actorInstance->GetActor()->GetMesh(actorInstance->GetLODLevel(), nodeIndex);
        const bool          isMeshNode  = (mesh);
        const bool          isBone      = (mBoneList.Find(nodeIndex) != MCORE_INVALIDINDEX32);
        const bool          isNode      = (isMeshNode == false && isBone == false);

        return CheckIfNodeVisible(nodeName, isMeshNode, isBone, isNode);
    }


    bool NodeHierarchyWidget::CheckIfNodeVisible(const AZStd::string& nodeName, bool isMeshNode, bool isBone, bool isNode)
    {
        if (((GetDisplayMeshes() && isMeshNode) ||
            (GetDisplayBones() && isBone) ||
            (GetDisplayNodes() && isNode)) &&
            (m_searchWidgetText.empty() || nodeName.find(m_searchWidgetText) != AZStd::string::npos))
        {
            return true;
        }

        return false;
    }


    void NodeHierarchyWidget::RecursivelyAddChilds(QTreeWidgetItem* parent, EMotionFX::Actor* actor, EMotionFX::ActorInstance* actorInstance, EMotionFX::Node* node)
    {
        const uint32        nodeIndex   = node->GetNodeIndex();
        AZStd::string       nodeName = node->GetNameString();
        AZStd::to_lower(nodeName.begin(), nodeName.end());
        const uint32        numChildren = node->GetNumChildNodes();
        EMotionFX::Mesh*    mesh        = actor->GetMesh(actorInstance->GetLODLevel(), nodeIndex);
        const bool          isMeshNode  = (mesh);
        const bool          isBone      = (mBoneList.Find(nodeIndex) != MCORE_INVALIDINDEX32);
        const bool          isNode      = (isMeshNode == false && isBone == false);

        if (CheckIfNodeVisible(nodeName, isMeshNode, isBone, isNode))
        {
            QTreeWidgetItem* item = new QTreeWidgetItem(parent);

            // check if the node is selected
            if (CheckIfNodeSelected(node->GetName(), actorInstance->GetID()))
            {
                item->setSelected(true);
            }

            item->setText(0, node->GetName());
            item->setText(2, QString::number(numChildren));
            item->setExpanded(true);
            item->setWhatsThis(0, QString::number(actorInstance->GetID()));

            // set the correct icon and the type
            if (isMeshNode)
            {
                item->setIcon(0, *mMeshIcon);
                item->setText(1, "Mesh");
                item->setText(3, QString::number(mesh->GetNumIndices() / 3));
            }
            else if (isBone)
            {
                item->setIcon(0, *mBoneIcon);
                item->setText(1, "Bone");
            }
            else if (isNode)
            {
                item->setIcon(0, *mNodeIcon);
                item->setText(1, "Node");
            }
            else
            {
                item->setText(1, "Undefined");
            }


            // the mirrored node
            const bool hasMirrorInfo = actor->GetHasMirrorInfo();
            if (hasMirrorInfo == false || actor->GetNodeMirrorInfo(nodeIndex).mSourceNode == MCORE_INVALIDINDEX16 || actor->GetNodeMirrorInfo(nodeIndex).mSourceNode == nodeIndex)
            {
                item->setText(4, "");
            }
            else
            {
                item->setText(4, actor->GetSkeleton()->GetNode(actor->GetNodeMirrorInfo(nodeIndex).mSourceNode)->GetName());
            }

            parent->addChild(item);

            // iterate through all children
            for (uint32 i = 0; i < numChildren; ++i)
            {
                // get the node index and the corresponding node
                const uint32        childIndex  = node->GetChildIndex(i);
                EMotionFX::Node*    child       = actor->GetSkeleton()->GetNode(childIndex);

                // recursively add all the nodes to the hierarchy
                RecursivelyAddChilds(item, actor, actorInstance, child);
            }
        }
        else
        {
            // iterate through all children
            for (uint32 i = 0; i < numChildren; ++i)
            {
                // get the node index and the corresponding node
                const uint32        childIndex  = node->GetChildIndex(i);
                EMotionFX::Node*    child       = actor->GetSkeleton()->GetNode(childIndex);

                // recursively add all the nodes to the hierarchy
                RecursivelyAddChilds(parent, actor, actorInstance, child);
            }
        }
    }


    // remove the selected item with the given node name from the selected nodes
    void NodeHierarchyWidget::RemoveNodeFromSelectedNodes(const char* nodeName, uint32 actorInstanceID)
    {
        // generate the string id for the given node name
        const uint32 nodeNameID = MCore::GetStringIdPool().GenerateIdForString(nodeName);

        // iterate through the selected nodes and remove the given ones
        for (size_t i = 0; i < m_selectedNodes.size(); )
        {
            // check if this is our node, if yes remove it
            if (nodeNameID == m_selectedNodes[i].mNodeNameID && actorInstanceID == m_selectedNodes[i].mActorInstanceID)
            {
                m_selectedNodes.erase(m_selectedNodes.begin() + i);
                //LOG("Removing: %s", nodeName);
            }
            else
            {
                i++;
            }
        }
    }


    void NodeHierarchyWidget::RemoveActorInstanceFromSelectedNodes(uint32 actorInstanceID)
    {
        const uint32 emptyStringID = MCore::GetStringIdPool().GenerateIdForString("");

        // iterate through the selected nodes and remove the given ones
        for (size_t i = 0; i < m_selectedNodes.size(); )
        {
            // check if this is our node, if yes remove it
            if (emptyStringID == m_selectedNodes[i].mNodeNameID && actorInstanceID == m_selectedNodes[i].mActorInstanceID)
            {
                m_selectedNodes.erase(m_selectedNodes.begin() + i);
                //LOG("Removing: %s", nodeName);
            }
            else
            {
                i++;
            }
        }
    }


    // add the given node from the given actor instance to the selected nodes
    void NodeHierarchyWidget::AddNodeToSelectedNodes(const char* nodeName, uint32 actorInstanceID)
    {
        AddNodeToSelectedNodes(SelectionItem(actorInstanceID, nodeName));
    }

    void NodeHierarchyWidget::AddNodeToSelectedNodes(const SelectionItem& item)
    {

        // Make sure this node is not already in our selection list
        for (const SelectionItem& selectedItem : m_selectedNodes)
        {
            if (item.mNodeNameID == selectedItem.mNodeNameID && item.mActorInstanceID == selectedItem.mActorInstanceID)
            {
                return;
            }
        }

        if (mUseSingleSelection)
        {
            m_selectedNodes.clear();
        }

        m_selectedNodes.emplace_back(item);
    }


    // remove all unselected child items from the currently selected nodes
    void NodeHierarchyWidget::RecursiveRemoveUnselectedItems(QTreeWidgetItem* item)
    {
        // check if this item is unselected, if yes remove it from the selected nodes
        if (item->isSelected() == false)
        {
            // get the actor instance id to which this item belongs to
            mActorInstanceIDString = FromQtString(item->whatsThis(0));
            int actorInstanceID;
            const bool validConversion = AzFramework::StringFunc::LooksLikeInt(mActorInstanceIDString.c_str(), &actorInstanceID);
            MCORE_ASSERT(validConversion);

            // remove the node from the selected nodes
            RemoveNodeFromSelectedNodes(FromQtString(item->text(0)).c_str(), actorInstanceID);

            // remove the selected actor
            QTreeWidgetItem* parent = item->parent();
            if (parent == nullptr)
            {
                RemoveActorInstanceFromSelectedNodes(actorInstanceID);
            }
        }

        // get the number of children and iterate through them
        const uint32 numChilds = item->childCount();
        for (uint32 i = 0; i < numChilds; ++i)
        {
            RecursiveRemoveUnselectedItems(item->child(i));
        }
    }


    void NodeHierarchyWidget::UpdateSelection()
    {
        uint32 i;

        //LOG("================================Update Selection!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
        //LOG("NumSelectedNodes=%i", mSelectedNodes.GetLength());
        //String debugString;
        //debugString.Reserve(10000);
        //for (uint32 s=0; s<mSelectedNodes.GetLength(); ++s)
        //  debugString += AZStd::string::format("%s,", mSelectedNodes[s].GetNodeName());
        //LOG(debugString.AsChar());

        // get the selected items and the number of them
        QList<QTreeWidgetItem*> selectedItems = mHierarchy->selectedItems();
        const uint32 numSelectedItems = selectedItems.count();

        // remove the unselected tree widget items from the selected nodes
        const uint32 numTopLevelItems = mHierarchy->topLevelItemCount();
        for (i = 0; i < numTopLevelItems; ++i)
        {
            RecursiveRemoveUnselectedItems(mHierarchy->topLevelItem(i));
        }

        // iterate through all selected items
        for (i = 0; i < numSelectedItems; ++i)
        {
            QTreeWidgetItem* item = selectedItems[i];

            // get the item name
            FromQtString(item->text(0), &mItemName);
            FromQtString(item->whatsThis(0), &mActorInstanceIDString);

            int actorInstanceID;
            const bool validConversion = AzFramework::StringFunc::LooksLikeInt(mActorInstanceIDString.c_str(), &actorInstanceID);
            MCORE_ASSERT(validConversion);

            EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().FindActorInstanceByID(actorInstanceID);
            if (actorInstance == nullptr)
            {
                //LogWarning("Cannot add node '%s' from actor instance %i to the selection.", itemName.AsChar(), actorInstanceID);
                continue;
            }

            // check if the item name is actually a valid node
            EMotionFX::Actor* actor = actorInstance->GetActor();
            if (actor->GetSkeleton()->FindNodeByName(mItemName.c_str()))
            {
                AddNodeToSelectedNodes(mItemName.c_str(), actorInstanceID);
            }

            // check if we are dealing with an actor instance
            QTreeWidgetItem* parent = item->parent();
            if (parent == nullptr)
            {
                AddNodeToSelectedNodes("", actorInstanceID);
            }
        }
    }


    void NodeHierarchyWidget::SetSelectionMode(bool useSingleSelection)
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


    void NodeHierarchyWidget::ItemDoubleClicked(QTreeWidgetItem* item, int column)
    {
        MCORE_UNUSED(item);
        MCORE_UNUSED(column);

        UpdateSelection();

        emit OnDoubleClicked(m_selectedNodes);
        emit OnDoubleClicked(GetSelectedItemsAsMCoreArray());
    }


    void NodeHierarchyWidget::OnSelectionChanged()
    {
        emit SelectionChanged();
    }


    void NodeHierarchyWidget::TreeContextMenu(const QPoint& pos)
    {
        // show the menu if at least one selected
        const size_t numSelectedNodes = m_selectedNodes.size();
        if (numSelectedNodes == 0 ||
            (numSelectedNodes == 1 && m_selectedNodes[0].GetNodeNameString().empty()))
        {
            return;
        }

        // show the menu
        QMenu menu(this);
        menu.addAction("Add all towards root to selection");

        AZStd::vector<SelectionItem> itemsToAdd;
        if (menu.exec(mHierarchy->mapToGlobal(pos)))
        {
            // Collect the list of items to select. Actual adding to the
            // selection has to happen in a separate loop so that the iterators
            // stay valid.
            for (const SelectionItem& selectedItem : m_selectedNodes)
            {
                // Ensure the actor instance is still valid before looking at
                // its skeleton
                const EMotionFX::ActorInstance* actorInstance =
                    EMotionFX::GetActorManager().FindActorInstanceByID(selectedItem.mActorInstanceID);
                if (!actorInstance)
                {
                    continue;
                }

                // Walk up the parent hierarchy
                const EMotionFX::Node* parentNode =
                    actorInstance->GetActor()->GetSkeleton()->FindNodeByName(selectedItem.GetNodeName());
                for(; parentNode; parentNode = parentNode->GetParentNode())
                {
                    itemsToAdd.emplace_back(selectedItem.mActorInstanceID, parentNode->GetName());
                }
            }

            for (const SelectionItem& item : itemsToAdd)
            {
                AddNodeToSelectedNodes(item);
            }
            Update();
        }
    }


    void NodeHierarchyWidget::OnTextFilterChanged(const QString& text)
    {
        //mFindString = String(text.toAscii().data()).Lowered();
        FromQtString(text, &m_searchWidgetText);
        AZStd::to_lower(m_searchWidgetText.begin(), m_searchWidgetText.end());
        Update();
    }


    void NodeHierarchyWidget::FireSelectionDoneSignal()
    {
        emit OnSelectionDone(m_selectedNodes);
        emit OnSelectionDone(GetSelectedItemsAsMCoreArray());
    }


    AZStd::vector<SelectionItem>& NodeHierarchyWidget::GetSelectedItems()
    {
        UpdateSelection();
        return m_selectedNodes;
    }


    MCore::Array<SelectionItem> NodeHierarchyWidget::GetSelectedItemsAsMCoreArray()
    {
        AZStd::vector<SelectionItem>& selectedItems = GetSelectedItems();
        MCore::Array<SelectionItem> result;

        const AZ::u32 numSelectedItems = static_cast<AZ::u32>(selectedItems.size());
        result.Resize(numSelectedItems);

        for (AZ::u32 i = 0; i < numSelectedItems; ++i)
        {
            result[i] = selectedItems[i];
        }

        return result;
    }


    // check if the node with the given name is selected in the window
    bool NodeHierarchyWidget::CheckIfNodeSelected(const char* nodeName, uint32 actorInstanceID)
    {
        for (const SelectionItem& selectedItem : m_selectedNodes)
        {
            if (selectedItem.mActorInstanceID == actorInstanceID && selectedItem.GetNodeNameString() == nodeName)
            {
                return true;
            }
        }

        // failure, not found in the selected nodes array
        return false;
    }


    // check if the actor instance with the given id is selected in the window
    bool NodeHierarchyWidget::CheckIfActorInstanceSelected(uint32 actorInstanceID)
    {
        for (const SelectionItem& selectedItem : m_selectedNodes)
        {
            if (selectedItem.mActorInstanceID == actorInstanceID && selectedItem.GetNodeNameString().empty())
            {
                return true;
            }
        }

        // failure, not found in the selected nodes array
        return false;
    }


    // sync the selection list with the selected nodes
    void NodeHierarchyWidget::ConvertFromSelectionList(CommandSystem::SelectionList* selectionList)
    {
        // if the selection list is not valid get the global from emstudio
        if (selectionList == nullptr)
        {
            selectionList = &(GetCommandManager()->GetCurrentSelection());
        }

        m_selectedNodes.clear();

        // get the number actor instances and iterate over them
        const uint32 numActorInstances = mActorInstanceIDs.GetLength();
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            // add the actor to the node hierarchy widget
            const uint32 actorInstanceID = mActorInstanceIDs[i];

            // get the number of selected nodes and iterate through them
            const uint32 numSelectedNodes = selectionList->GetNumSelectedNodes();
            for (uint32 n = 0; n < numSelectedNodes; ++n)
            {
                const EMotionFX::Node* joint = selectionList->GetNode(n);
                if (joint)
                {
                    SelectionItem selectionItem;
                    selectionItem.mActorInstanceID = actorInstanceID;
                    selectionItem.SetNodeName(joint->GetName());
                    m_selectedNodes.emplace_back(selectionItem);
                }
            }
        }
    }


    bool NodeHierarchyWidget::GetDisplayMeshes() const
    {
        return mFilterState.testFlag(FilterType::Meshes);
    }


    bool NodeHierarchyWidget::GetDisplayNodes() const
    {
        return mFilterState.testFlag(FilterType::Nodes);
    }


    bool NodeHierarchyWidget::GetDisplayBones() const
    {
        return mFilterState.testFlag(FilterType::Bones);
    }

    /*
    void NodeHierarchyWidget::OnVisibilityChanged(bool isVisible)
    {
        if (isVisible)
            Update();
    }*/
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/moc_NodeHierarchyWidget.cpp>
