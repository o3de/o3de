/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/Entries/ProductAssetBrowserEntry.h>
#include <Editor/View/Widgets/LoggingPanel/PivotTree/GraphPivotTree/GraphPivotTree.h>
#include <ScriptCanvas/Asset/SubgraphInterfaceAsset.h>

namespace ScriptCanvasEditor
{
    /////////////////////////////
    // GraphPivotTreeEntityItem
    /////////////////////////////
    
    GraphPivotTreeEntityItem::GraphPivotTreeEntityItem(const AZ::NamedEntityId& namedEntityId, const ScriptCanvas::GraphIdentifier& graphIdentifier)
        : PivotTreeEntityItem(namedEntityId)
        , m_checkState(Qt::CheckState::Unchecked)
        , m_graphIdentifier(graphIdentifier)
    {
    }

    Qt::CheckState GraphPivotTreeEntityItem::GetCheckState() const
    {
        return m_checkState;
    }

    void GraphPivotTreeEntityItem::SetCheckState(Qt::CheckState checkState)
    {
        m_checkState = checkState;
        SignalDataChanged();
    }

    const ScriptCanvas::GraphIdentifier& GraphPivotTreeEntityItem::GetGraphIdentifier() const
    {
        return m_graphIdentifier;
    }

    ////////////////////////////
    // GraphPivotTreeGraphItem
    ////////////////////////////

    GraphPivotTreeGraphItem::GraphPivotTreeGraphItem(const AZ::Data::AssetId& assetId)
        : PivotTreeGraphItem(assetId)
        , m_checkState(Qt::CheckState::Unchecked)
    {
        const bool isPivotedElement = true;
        SetIsPivotedElement(isPivotedElement);

        const bool isChecked = true;
        SetupDynamicallySpawnedElementItem(isChecked);
    }

    void GraphPivotTreeGraphItem::OnDataSwitch()
    {
        bool isChecked = true;
        auto pivotIter = m_pivotItems.find(AZ::EntityId());

        if (pivotIter != m_pivotItems.end())
        {
            GraphPivotTreeEntityItem* entityItem = pivotIter->second;
            isChecked = entityItem->GetCheckState() == Qt::CheckState::Checked;
        }

        m_pivotItems.clear();
        ClearChildren();

        SetupDynamicallySpawnedElementItem(isChecked);
    }

    void GraphPivotTreeGraphItem::RegisterEntity(const AZ::NamedEntityId& entityId, const ScriptCanvas::GraphIdentifier& graphIdentifier)
    {
        auto pivotRange = m_pivotItems.equal_range(entityId);

        bool foundElement = false;

        for (auto mapIter = pivotRange.first; mapIter != pivotRange.second; ++mapIter)
        {
            if (mapIter->second->GetGraphIdentifier() == graphIdentifier)
            {
                foundElement = true;
                break;
            }
        }

        if (!foundElement)
        {
            GraphPivotTreeEntityItem* entityItem = CreateChildNode<GraphPivotTreeEntityItem>(entityId, graphIdentifier);
            entityItem->SetCheckState(Qt::CheckState::Unchecked);

            m_pivotItems.insert(AZStd::make_pair(entityId,entityItem));
        }
    }

    void GraphPivotTreeGraphItem::UnregisterEntity(const AZ::NamedEntityId& entityId, const ScriptCanvas::GraphIdentifier& graphIdentifier)
    {
        auto pivotRange = m_pivotItems.equal_range(entityId);

        for (auto pivotIter = pivotRange.first; pivotIter != pivotRange.second; ++pivotIter)
        {
            if (pivotIter->second->GetGraphIdentifier() == graphIdentifier)
            {
                RemoveChild(pivotIter->second);
                m_pivotItems.erase(pivotIter);
                break;
            }
        }
    }

    void GraphPivotTreeGraphItem::OnChildDataChanged(GraphCanvasTreeItem* treeItem)
    {
        GraphPivotTreeEntityItem* graphItem = static_cast<GraphPivotTreeEntityItem*>(treeItem);

        if (graphItem->GetCheckState() == Qt::CheckState::Checked)
        {
            LoggingDataRequestBus::Event(GetLoggingDataId(), &LoggingDataRequests::EnableRegistration, graphItem->GetNamedEntityId(), graphItem->GetGraphIdentifier());
        }
        else
        {
            LoggingDataRequestBus::Event(GetLoggingDataId(), &LoggingDataRequests::DisableRegistration, graphItem->GetNamedEntityId(), graphItem->GetGraphIdentifier());
        }

        CalculateCheckState();
        SignalDataChanged();
    }

    Qt::CheckState GraphPivotTreeGraphItem::GetCheckState() const
    {
        return m_checkState;
    }

    void GraphPivotTreeGraphItem::SetCheckState(Qt::CheckState checkState)
    {
        if (checkState != m_checkState)
        {
            m_checkState = checkState;

            for (int i = 0; i < GetChildCount(); ++i)
            {
                PivotTreeItem* treeItem = static_cast<PivotTreeItem*>(FindChildByRow(i));

                if (treeItem)
                {
                    treeItem->SetCheckState(checkState);
                }
            }

            SignalDataChanged();
        }
    }

    GraphPivotTreeEntityItem* GraphPivotTreeGraphItem::FindDynamicallySpawnedTreeItem() const
    {
        return FindEntityTreeItem(AZ::NamedEntityId(AZ::EntityId(), ""), ScriptCanvas::GraphIdentifier(GetAssetId(), k_dynamicallySpawnedControllerId));
    }

    GraphPivotTreeEntityItem* GraphPivotTreeGraphItem::FindEntityTreeItem(const AZ::NamedEntityId& namedEntityId, [[maybe_unused]] const ScriptCanvas::GraphIdentifier& graphIdentifier) const
    {
        auto pivotIter = m_pivotItems.find(namedEntityId);

        if (pivotIter != m_pivotItems.end())
        {
            return pivotIter->second;
        }

        return nullptr;
    }

    void GraphPivotTreeGraphItem::OnEnabledStateChanged(bool isEnabled, const AZ::NamedEntityId& namedEntityId, const ScriptCanvas::GraphIdentifier& graphIdentifier)
    {
        if (graphIdentifier.m_assetId == GetAssetId())
        {
            GraphPivotTreeEntityItem* item = FindEntityTreeItem(namedEntityId, graphIdentifier);

            if (item)
            {
                if (isEnabled)
                {
                    item->SetCheckState(Qt::CheckState::Checked);
                }
                else
                {
                    item->SetCheckState(Qt::CheckState::Unchecked);
                }
            }
        }
    }

    void GraphPivotTreeGraphItem::OnLoggingDataIdSet()
    {
        LoggingDataNotificationBus::Handler::BusDisconnect();

        LoggingDataNotificationBus::Handler::BusConnect(GetLoggingDataId());
    }

    void GraphPivotTreeGraphItem::SetupDynamicallySpawnedElementItem([[maybe_unused]] bool isChecked)
    {
        AZ::NamedEntityId dynamicEntityId(AZ::EntityId(), "All Graph Instances");
        RegisterEntity(dynamicEntityId, ScriptCanvas::GraphIdentifier(GetAssetId(), k_dynamicallySpawnedControllerId));
    }

    void GraphPivotTreeGraphItem::CalculateCheckState()
    {
        bool isChecked = false;
        bool isUnchecked = false;
        for (int i = 0; i < GetChildCount(); ++i)
        {
            PivotTreeItem* treeItem = static_cast<PivotTreeItem*>(FindChildByRow(i));

            if (treeItem)
            {
                if (treeItem->GetCheckState() == Qt::CheckState::Checked)
                {
                    isChecked = true;
                }
                else
                {
                    isUnchecked = true;
                }

                if (isChecked && isUnchecked)
                {
                    break;
                }
            }
        }

        if (isChecked && isUnchecked)
        {
            m_checkState = Qt::CheckState::PartiallyChecked;
        }
        else if (isChecked)
        {
            m_checkState = Qt::CheckState::Checked;
        }
        else
        {
            m_checkState = Qt::CheckState::Unchecked;
        }
    }

    /////////////////////////
    // GraphPivotTreeFolder
    /////////////////////////

    GraphPivotTreeFolder::GraphPivotTreeFolder(AZStd::string_view folder)
        : m_folderName(folder)
        , m_checkState(Qt::CheckState::Unchecked)
    {
        const bool isPivotElement = true;
        SetIsPivotedElement(isPivotElement);
    }

    Qt::CheckState GraphPivotTreeFolder::GetCheckState() const
    {
        return m_checkState;
    }

    void GraphPivotTreeFolder::SetCheckState(Qt::CheckState checkState)
    {
        if (m_checkState != checkState)
        {
            m_checkState = checkState;

            for (int i = 0; i < GetChildCount(); ++i)
            {
                PivotTreeItem* pivotTreeItem = static_cast<PivotTreeItem*>(FindChildByRow(i));
                pivotTreeItem->SetCheckState(checkState);
            }

            SignalDataChanged();
        }
    }

    AZStd::string GraphPivotTreeFolder::GetDisplayName() const
    {
        return m_folderName;
    }

    void GraphPivotTreeFolder::OnChildDataChanged([[maybe_unused]] GraphCanvasTreeItem* treeItem)
    {
        CalculateCheckState();

        SignalDataChanged();
    }

    void GraphPivotTreeFolder::CalculateCheckState()
    {
        bool isChecked = false;
        bool isUnchecked = false;

        for (int i = 0; i < GetChildCount(); ++i)
        {
            const PivotTreeItem* pivotTreeItem = static_cast<const PivotTreeItem*>(FindChildByRow(i));

            if (pivotTreeItem)
            {
                Qt::CheckState checkState = pivotTreeItem->GetCheckState();

                if (checkState == Qt::CheckState::PartiallyChecked)
                {
                    isChecked = true;
                    isUnchecked = true;
                }
                else if (checkState == Qt::CheckState::Checked)
                {
                    isChecked = true;
                }
                else
                {
                    isUnchecked = true;
                }

                if (isChecked && isUnchecked)
                {
                    break;
                }
            }
        }    

        if (isChecked && isUnchecked)
        {
            m_checkState = Qt::CheckState::PartiallyChecked;
        }
        else if (isChecked)
        {
            m_checkState = Qt::CheckState::Checked;
        }
        else
        {
            m_checkState = Qt::CheckState::Unchecked;
        }

        SignalDataChanged();
    }

    ///////////////////////
    // GraphPivotTreeRoot
    ///////////////////////

    GraphPivotTreeRoot::GraphPivotTreeRoot()
        : m_categorizer((*this))
    {
        AzToolsFramework::AssetBrowser::AssetBrowserModel* assetBrowserModel = nullptr;
        AzToolsFramework::AssetBrowser::AssetBrowserComponentRequestBus::BroadcastResult(assetBrowserModel, &AzToolsFramework::AssetBrowser::AssetBrowserComponentRequests::GetAssetBrowserModel);

        m_assetModel = new AzToolsFramework::AssetBrowser::AssetBrowserFilterModel();

        AzToolsFramework::AssetBrowser::AssetGroupFilter* assetFilter = new AzToolsFramework::AssetBrowser::AssetGroupFilter();
        assetFilter->SetAssetGroup(SourceDescription::GetGroup());

        assetFilter->SetFilterPropagation(AzToolsFramework::AssetBrowser::AssetBrowserEntryFilter::PropagateDirection::Down);

        QObject::connect(m_assetModel, &QAbstractItemModel::rowsInserted, this, &GraphPivotTreeRoot::OnScriptCanvasGraphAssetAdded);
        QObject::connect(m_assetModel, &QAbstractItemModel::rowsAboutToBeRemoved, this, &GraphPivotTreeRoot::OnScriptCanvasGraphAssetRemoved);

        m_assetModel->setSourceModel(assetBrowserModel);

        SetAllowPruneOnEmpty(false);
    }

    void GraphPivotTreeRoot::OnDataSourceChanged(const LoggingDataId& aggregateDataSource)
    {
        if (LoggingDataNotificationBus::Handler::BusIsConnected())
        {
            LoggingDataNotificationBus::Handler::BusDisconnect();
        }

        if (m_loggedDataId.IsValid())
        {
            const LoggingDataAggregator* previousDataAggregator = nullptr;
            LoggingDataRequestBus::EventResult(previousDataAggregator, m_loggedDataId, &LoggingDataRequests::FindLoggingData);

            if (previousDataAggregator)
            {
                const EntityGraphRegistrationMap& entityPivoting = previousDataAggregator->GetEntityGraphRegistrationMap();

                for (const auto& registrationMap : entityPivoting)
                {
                    OnEntityGraphUnregistered(registrationMap.first, registrationMap.second);
                }
            }
        }

        m_loggedDataId = aggregateDataSource;
        for (auto assetPair : m_graphTreeItemMapping)
        {
            assetPair.second->OnDataSwitch();
        }

        const LoggingDataAggregator* dataAggregator = nullptr;
        LoggingDataRequestBus::EventResult(dataAggregator, m_loggedDataId, &LoggingDataRequests::FindLoggingData);

        if (dataAggregator)
        {
            const EntityGraphRegistrationMap& entityPivoting = dataAggregator->GetEntityGraphRegistrationMap();

            for (const auto& registrationMap : entityPivoting)
            {
                OnEntityGraphRegistered(registrationMap.first, registrationMap.second);
            }

            if (dataAggregator->IsCapturingData())
            {
                OnDataCaptureBegin();
            }
        }

        LoggingDataNotificationBus::Handler::BusConnect(m_loggedDataId);
    }

    void GraphPivotTreeRoot::OnDataCaptureBegin()
    {
    }

    void GraphPivotTreeRoot::OnDataCaptureEnd()
    {
    }

    void GraphPivotTreeRoot::OnEntityGraphRegistered(const AZ::NamedEntityId& entityId, const ScriptCanvas::GraphIdentifier& graphIdentifier)
    {
        GraphPivotTreeGraphItem* graphItem = nullptr;

        auto mapPairIter = m_graphTreeItemMapping.find(graphIdentifier.m_assetId);

        if (mapPairIter == m_graphTreeItemMapping.end())
        {
            AZStd::string fullPath;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(fullPath, &AZ::Data::AssetCatalogRequests::GetAssetPathById, graphIdentifier.m_assetId);

            GraphCanvasTreeItem* parentItem = m_categorizer.GetCategoryNode(fullPath.c_str(), this);
            graphItem = parentItem->CreateChildNode<GraphPivotTreeGraphItem>(graphIdentifier.m_assetId);

            m_graphTreeItemMapping[graphIdentifier.m_assetId] = graphItem;
        }
        else
        {
            graphItem = mapPairIter->second;
        }

        if (entityId.IsValid())
        {
            graphItem->RegisterEntity(entityId, graphIdentifier);
        }
    }

    void GraphPivotTreeRoot::OnEntityGraphUnregistered(const AZ::NamedEntityId& entityId, const ScriptCanvas::GraphIdentifier& graphIdentifier)
    {
        auto mapPairIter = m_graphTreeItemMapping.find(graphIdentifier.m_assetId);

        if (mapPairIter != m_graphTreeItemMapping.end())
        {
            mapPairIter->second->UnregisterEntity(entityId, graphIdentifier);

            if (mapPairIter->second->GetChildCount() == 0)
            {
                m_categorizer.PruneEmptyNodes();
            }
        }
    }

    void GraphPivotTreeRoot::OnEnabledStateChanged(bool isEnabled, const AZ::NamedEntityId& namedEntityId, const ScriptCanvas::GraphIdentifier& graphIdentifier)
    {
        auto mapIter = m_graphTreeItemMapping.find(graphIdentifier.m_assetId);

        if (mapIter != m_graphTreeItemMapping.end())
        {
            GraphPivotTreeEntityItem* pivotTreeItem = nullptr;
            if (graphIdentifier.m_componentId == k_dynamicallySpawnedControllerId)
            {
                pivotTreeItem = mapIter->second->FindDynamicallySpawnedTreeItem();
            }
            else
            {
                pivotTreeItem = mapIter->second->FindEntityTreeItem(namedEntityId, graphIdentifier);
            }

            if (pivotTreeItem)
            {
                if (isEnabled)
                {
                    pivotTreeItem->SetCheckState(Qt::CheckState::Checked);
                }
                else
                {
                    pivotTreeItem->SetCheckState(Qt::CheckState::Unchecked);
                }

            }
        }
    }

    GraphCanvas::GraphCanvasTreeItem* GraphPivotTreeRoot::CreateCategoryNode([[maybe_unused]] AZStd::string_view categoryPath, AZStd::string_view categoryName, GraphCanvasTreeItem* parent) const
    {
        return parent->CreateChildNode<GraphPivotTreeFolder>(categoryName);
    }

    void GraphPivotTreeRoot::OnScriptCanvasGraphAssetAdded(const QModelIndex& parentIndex, int first, int last)
    {
        for (int i = first; i <= last; ++i)
        {
            QModelIndex modelIndex = m_assetModel->index(i, 0, parentIndex);
            QModelIndex sourceIndex = m_assetModel->mapToSource(modelIndex);

            AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry = reinterpret_cast<AzToolsFramework::AssetBrowser::AssetBrowserEntry*>(sourceIndex.internalPointer());

            ProcessEntry(entry);
        }
    }

    void GraphPivotTreeRoot::OnScriptCanvasGraphAssetRemoved(const QModelIndex& parentIndex, int first, int last)
    {
        // TODO: This likely needs to be handled better
        for (int i = first; i <= last; ++i)
        {
            QModelIndex modelIndex = m_assetModel->index(i, 0, parentIndex);
            QModelIndex sourceIndex = m_assetModel->mapToSource(modelIndex);

            AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry = reinterpret_cast<AzToolsFramework::AssetBrowser::AssetBrowserEntry*>(sourceIndex.internalPointer());

            if (entry && entry->GetEntryType() == AzToolsFramework::AssetBrowser::AssetBrowserEntry::AssetEntryType::Product)
            {
                const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry* productEntry = azrtti_cast<const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry*>(entry);

                if (productEntry->GetAssetType() == azrtti_typeid<ScriptCanvas::SubgraphInterfaceAsset>())
                {
                    auto mapIter = m_graphTreeItemMapping.find(productEntry->GetAssetId());

                    if (mapIter != m_graphTreeItemMapping.end() && mapIter->second)
                    {
                        GraphCanvas::GraphCanvasTreeItem* currentItem = mapIter->second;
                        currentItem->ClearChildren();

                        m_graphTreeItemMapping.erase(mapIter);

                        m_categorizer.PruneNode(currentItem);
                    }

                    OnEntityGraphUnregistered(AZ::NamedEntityId(), ScriptCanvas::GraphIdentifier(productEntry->GetAssetId(), k_dynamicallySpawnedControllerId));
                }
            }
        }
    }

    void GraphPivotTreeRoot::ProcessEntry(AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry)
    {
        if (entry && entry->GetEntryType() == AzToolsFramework::AssetBrowser::AssetBrowserEntry::AssetEntryType::Product)
        {
            const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry* productEntry = static_cast<const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry*>(entry);

            if (productEntry->GetAssetType() == azrtti_typeid<ScriptCanvas::SubgraphInterfaceAsset>())
            {
                OnEntityGraphRegistered(AZ::NamedEntityId(), ScriptCanvas::GraphIdentifier(productEntry->GetAssetId(), k_dynamicallySpawnedControllerId));
            }
        }
    }

    void GraphPivotTreeRoot::TraverseTree(QModelIndex index)
    {
        QModelIndex sourceIndex = m_assetModel->mapToSource(index);
        AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry = reinterpret_cast<AzToolsFramework::AssetBrowser::AssetBrowserEntry*>(sourceIndex.internalPointer());

        ProcessEntry(entry);

        int rowCount = m_assetModel->rowCount(index);

        for (int i = 0; i < rowCount; ++i)
        {
            QModelIndex nextIndex = m_assetModel->index(i, 0, index);
            TraverseTree(nextIndex);
        }
    }

    /////////////////////////
    // GraphPivotTreeWidget
    /////////////////////////

    GraphPivotTreeWidget::GraphPivotTreeWidget(QWidget* parent)
        : PivotTreeWidget(aznew GraphPivotTreeRoot(), AZ_CRC_CE("GraphPivotTreeId"), parent)
    {
        static_cast<GraphPivotTreeRoot*>(GetTreeRoot())->TraverseTree();
    }

#include <Editor/View/Widgets/LoggingPanel/PivotTree/GraphPivotTree/moc_GraphPivotTree.cpp>

}
