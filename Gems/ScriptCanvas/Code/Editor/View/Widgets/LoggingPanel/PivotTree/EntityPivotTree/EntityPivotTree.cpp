/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/View/Widgets/LoggingPanel/PivotTree/EntityPivotTree/EntityPivotTree.h>

namespace ScriptCanvasEditor
{
    /////////////////////////////
    // EntityPivotTreeGraphItem
    /////////////////////////////

    EntityPivotTreeGraphItem::EntityPivotTreeGraphItem(const ScriptCanvas::GraphIdentifier& graphIdentifier)
        : PivotTreeGraphItem(graphIdentifier.m_assetId)
        , m_checkState(Qt::CheckState::Unchecked)
        , m_graphIdentifier(graphIdentifier)
    {
    }

    Qt::CheckState EntityPivotTreeGraphItem::GetCheckState() const
    {
        return m_checkState;
    }

    void EntityPivotTreeGraphItem::SetCheckState(Qt::CheckState checkState)
    {
        m_checkState = checkState;

        SignalDataChanged();
    }

    const ScriptCanvas::GraphIdentifier& EntityPivotTreeGraphItem::GetGraphIdentifier() const
    {
        return m_graphIdentifier;
    }

    //////////////////////////////
    // EntityPivotTreeEntityItem
    //////////////////////////////

    EntityPivotTreeEntityItem::EntityPivotTreeEntityItem(const AZ::NamedEntityId& entityId)
        : PivotTreeEntityItem(entityId)
        , m_checkState(Qt::CheckState::Unchecked)
    {
        const bool isPivotElement = true;
        SetIsPivotedElement(isPivotElement);
    }

    void EntityPivotTreeEntityItem::RegisterGraphIdentifier(const ScriptCanvas::GraphIdentifier& graphIdentifier)
    {
        auto mapIter = m_pivotItems.find(graphIdentifier);

        if (mapIter == m_pivotItems.end())
        {
            EntityPivotTreeGraphItem* graphItem = CreateChildNode<EntityPivotTreeGraphItem>(graphIdentifier);
            m_pivotItems[graphIdentifier] = graphItem;

            if (m_checkState != Qt::CheckState::PartiallyChecked)
            {
                graphItem->SetCheckState(m_checkState);
            }
        }
    }

    void EntityPivotTreeEntityItem::OnChildDataChanged(GraphCanvasTreeItem* treeItem)
    {
        EntityPivotTreeGraphItem* graphItem = static_cast<EntityPivotTreeGraphItem*>(treeItem);

        ScriptCanvas::GraphIdentifier graphIdentifier = graphItem->GetGraphIdentifier();

        if (graphItem->GetCheckState() == Qt::CheckState::Checked)
        {
            LoggingDataRequestBus::Event(GetLoggingDataId(), &LoggingDataRequests::EnableRegistration, GetNamedEntityId(), graphIdentifier);
        }
        else
        {
            LoggingDataRequestBus::Event(GetLoggingDataId(), &LoggingDataRequests::DisableRegistration, GetNamedEntityId(), graphIdentifier);
        }

        CalculateCheckState();
        SignalDataChanged();
    }

    void EntityPivotTreeEntityItem::UnregisterGraphIdentifier(const ScriptCanvas::GraphIdentifier& graphIdentifier)
    {
        auto mapIter = m_pivotItems.find(graphIdentifier);

        if (mapIter != m_pivotItems.end())
        {
            RemoveChild(mapIter->second);
            m_pivotItems.erase(mapIter);
        }
    }

    Qt::CheckState EntityPivotTreeEntityItem::GetCheckState() const
    {
        return m_checkState;
    }

    void EntityPivotTreeEntityItem::SetCheckState(Qt::CheckState checkState)
    {
        if (m_checkState != checkState)
        {
            m_checkState = checkState;
            for (const auto& mapIter : m_pivotItems)
            {
                mapIter.second->SetCheckState(checkState);
            }

            SignalDataChanged();
        }
    }

    EntityPivotTreeGraphItem* EntityPivotTreeEntityItem::FindGraphTreeItem(const ScriptCanvas::GraphIdentifier& graphIdentifier)
    {
        auto mapIter = m_pivotItems.find(graphIdentifier);

        if (mapIter != m_pivotItems.end())
        {
            return mapIter->second;
        }
        else
        {
            return nullptr;
        }
    }

    void EntityPivotTreeEntityItem::OnEnabledStateChanged(bool isEnabled, const AZ::NamedEntityId& namedEntityId, const ScriptCanvas::GraphIdentifier& graphIdentifier)
    {
        if (namedEntityId == GetEntityId())
        {
            EntityPivotTreeGraphItem* item = FindGraphTreeItem(graphIdentifier);

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

    void EntityPivotTreeEntityItem::OnLoggingDataIdSet()
    {
        LoggingDataNotificationBus::Handler::BusDisconnect();

        LoggingDataNotificationBus::Handler::BusConnect(GetLoggingDataId());
    }

    void EntityPivotTreeEntityItem::CalculateCheckState()
    {
        bool isChecked = false;
        bool isUnchecked = false;

        for (const auto& mapIter : m_pivotItems)
        {
            if (mapIter.second->GetCheckState() == Qt::CheckState::Checked)
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

        m_checkState = Qt::CheckState::Unchecked;

        if (isChecked && isUnchecked)
        {
            m_checkState = Qt::CheckState::PartiallyChecked;
        }
        else if (isChecked)
        {
            m_checkState = Qt::CheckState::Checked;
        }
    }

    ////////////////////////
    // EntityPivotTreeRoot
    ////////////////////////

    EntityPivotTreeRoot::EntityPivotTreeRoot()
        : m_capturingData(false)
    {
    }

    void EntityPivotTreeRoot::OnDataSourceChanged(const LoggingDataId& aggregateDataSource)
    {
        ClearData();

        LoggingDataNotificationBus::Handler::BusDisconnect();

        m_dataSource = aggregateDataSource;

        const LoggingDataAggregator* dataAggregator = nullptr;

        LoggingDataRequestBus::EventResult(dataAggregator, m_dataSource, &LoggingDataRequests::FindLoggingData);

        if (dataAggregator)
        {
            const EntityGraphRegistrationMap& registrationMap = dataAggregator->GetEntityGraphRegistrationMap();

            for (const auto& mapIter : registrationMap)
            {
                OnEntityGraphRegistered(mapIter.first, mapIter.second);
            }

            if (dataAggregator->IsCapturingData())
            {
                OnDataCaptureBegin();
            }
        }

        LoggingDataNotificationBus::Handler::BusConnect(m_dataSource);
    }

    void EntityPivotTreeRoot::OnDataCaptureBegin()
    {
        m_capturingData = true;
    }

    void EntityPivotTreeRoot::OnDataCaptureEnd()
    {
        m_capturingData = false;

        for (const auto& unregistrationPair : m_delayedUnregistrations)
        {
            OnEntityGraphUnregistered(unregistrationPair.first, unregistrationPair.second);
        }

        m_delayedUnregistrations.clear();
    }

    void EntityPivotTreeRoot::OnEntityGraphRegistered(const AZ::NamedEntityId& namedEntityId, const ScriptCanvas::GraphIdentifier& graphIdentifier)
    {
        EntityPivotTreeEntityItem* pivotItem = nullptr;

        auto mapIter = m_entityTreeItemMapping.find(namedEntityId);

        if (mapIter != m_entityTreeItemMapping.end())
        {
            pivotItem = mapIter->second;
        }
        else
        {
            pivotItem = CreateChildNode<EntityPivotTreeEntityItem>(namedEntityId);
            m_entityTreeItemMapping[namedEntityId] = pivotItem;
        }

        if (pivotItem)
        {
            pivotItem->RegisterGraphIdentifier(graphIdentifier);
        }
    }

    void EntityPivotTreeRoot::OnEntityGraphUnregistered(const AZ::NamedEntityId& namedEntityId, const ScriptCanvas::GraphIdentifier& graphIdentifier)
    {
        auto mapIter = m_entityTreeItemMapping.find(namedEntityId);

        if (mapIter != m_entityTreeItemMapping.end())
        {
            EntityPivotTreeEntityItem* pivotItem = mapIter->second;

            if (!m_capturingData)
            {
                pivotItem->UnregisterGraphIdentifier(graphIdentifier);

                if (pivotItem->GetChildCount() == 0)
                {
                    RemoveChild(pivotItem);
                    m_entityTreeItemMapping.erase(mapIter);
                }
            }
            else
            {
                m_delayedUnregistrations.emplace_back(namedEntityId, graphIdentifier);
            }
        }
    }

    void EntityPivotTreeRoot::OnEnabledStateChanged(bool isEnabled, const AZ::NamedEntityId& namedEntityId, const ScriptCanvas::GraphIdentifier& graphIdentifier)
    {
        if (graphIdentifier.m_componentId == k_dynamicallySpawnedControllerId)
        {
            return;
        }

        auto mapIter = m_entityTreeItemMapping.find(namedEntityId);

        if (mapIter != m_entityTreeItemMapping.end())
        {
            EntityPivotTreeGraphItem* pivotTreeItem = mapIter->second->FindGraphTreeItem(graphIdentifier);

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

    void EntityPivotTreeRoot::ClearData()
   {
        ClearChildren();
        m_entityTreeItemMapping.clear();
    }

    //////////////////////////
    // EntityPivotTreeWidget
    //////////////////////////

    EntityPivotTreeWidget::EntityPivotTreeWidget(QWidget* parent)
        : PivotTreeWidget(aznew EntityPivotTreeRoot(), AZ_CRC_CE("EntityPivotTreeId"), parent)
    {
    }

#include <Editor/View/Widgets/LoggingPanel/PivotTree/EntityPivotTree/moc_EntityPivotTree.cpp>
}
