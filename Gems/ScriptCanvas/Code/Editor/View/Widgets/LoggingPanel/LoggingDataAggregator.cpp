/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/View/Widgets/LoggingPanel/LoggingDataAggregator.h>

namespace ScriptCanvasEditor
{
    //////////////////////////
    // LoggingDataAggregator
    //////////////////////////

    LoggingDataAggregator::LoggingDataAggregator()
        : m_id(AZ::Entity::MakeId())
        , m_ignoreRegistrations(false)
        , m_hasAnchor(false)
        , m_anchorTimeStamp(0)
    {
        LoggingDataRequestBus::Handler::BusConnect(m_id);

        m_debugLogRoot = aznew DebugLogRootItem();
    }

    LoggingDataAggregator::~LoggingDataAggregator()
    {
    }

    const LoggingDataId& LoggingDataAggregator::GetDataId() const
    {
        return m_id;
    }

    const LoggingDataAggregator* LoggingDataAggregator::FindLoggingData() const
    {
        return this;
    }

    void LoggingDataAggregator::EnableRegistration(const AZ::NamedEntityId& namedEntityId, const ScriptCanvas::GraphIdentifier& graphIdentifier)
    {
        if (m_ignoreRegistrations)
        {
            return;
        }

        bool signalAddition = false;

        if (graphIdentifier.m_componentId == k_dynamicallySpawnedControllerId)
        {
            auto insertResult = m_loggedAssetSet.insert(graphIdentifier);
            signalAddition = insertResult.second;
        }
        else
        {
            signalAddition = true;

            auto equalRange = m_loggingEntityMapping.equal_range(namedEntityId);

            for (auto mapIter = equalRange.first; mapIter != equalRange.second; ++mapIter)
            {
                if (mapIter->second == graphIdentifier)
                {
                    signalAddition = false;
                    break;
                }
            }

            if (signalAddition)
            {
                m_loggingEntityMapping.insert(AZStd::make_pair(namedEntityId, graphIdentifier));
            }
        }

        if (signalAddition)
        {
            m_ignoreRegistrations = true;
            LoggingDataNotificationBus::Event(GetDataId(), &LoggingDataNotifications::OnEnabledStateChanged, true, namedEntityId, graphIdentifier);
            m_ignoreRegistrations = false;

            OnRegistrationEnabled(namedEntityId, graphIdentifier);
        }
    }

    void LoggingDataAggregator::DisableRegistration(const AZ::NamedEntityId& namedEntityId, const ScriptCanvas::GraphIdentifier& graphIdentifier)
    {
        if (m_ignoreRegistrations)
        {
            return;
        }

        bool signalErase = false;

        if (graphIdentifier.m_componentId == k_dynamicallySpawnedControllerId)
        {
            size_t eraseCount = m_loggedAssetSet.erase(graphIdentifier);
            signalErase = eraseCount > 0;
        }
        else
        {
            auto equalRange = m_loggingEntityMapping.equal_range(namedEntityId);

            for (auto mapIter = equalRange.first; mapIter != equalRange.second; ++mapIter)
            {
                if (mapIter->second == graphIdentifier)
                {
                    signalErase = true;
                    m_loggingEntityMapping.erase(mapIter);
                    break;
                }
            }
        }

        if (signalErase)
        {
            m_ignoreRegistrations = true;
            LoggingDataNotificationBus::Event(GetDataId(), &LoggingDataNotifications::OnEnabledStateChanged, false, namedEntityId, graphIdentifier);
            m_ignoreRegistrations = false;

            OnRegistrationDisabled(namedEntityId, graphIdentifier);
        }
    }

    AZ::NamedEntityId LoggingDataAggregator::FindNamedEntityId(const AZ::EntityId& entityId)
    {
        auto cacheIter = m_entityNameCache.find(entityId);

        if (cacheIter != m_entityNameCache.end())
        {
            return AZ::NamedEntityId(entityId, cacheIter->second);
        }

        return AZ::NamedEntityId(entityId, "<unknown>");
    }

    const EntityGraphRegistrationMap& LoggingDataAggregator::GetEntityGraphRegistrationMap() const
    {
        return m_registrationMap;
    }

    const LoggingEntityMap& LoggingDataAggregator::GetLoggingEntityMap() const
    {
        return m_loggingEntityMapping;
    }

    const LoggingAssetSet& LoggingDataAggregator::GetLoggingAssetSet() const
    {
        return m_loggedAssetSet;
    }

    void LoggingDataAggregator::ProcessSignal([[maybe_unused]] const ScriptCanvas::Signal& signal)
    {
        //GraphIdentifier identifier;
        //identifier.m_entityId = signal.m_runtimeEntity;
        //identifier.m_assetId = signal.m_graphCount.m_assetId;
        //identifier.m_sequenceId = signal.m_graphCount.m_count;

        //auto aggregateIter = m_lastAggregateItemMap.find(identifier);

        //if (aggregateIter != m_lastAggregateItemMap.end())
        //{
        //    /*
        //    if (aggregateIter->second->TryProcessSignal(signal))
        //    {
        //        return;
        //    }
        //    */

        //    m_lastAggregateItemMap.erase(identifier);
        //}

        // Signal events are ambiguous on their own. Will need a secondary source of information to be able to disambiguate them.
    }

    void LoggingDataAggregator::ProcessNodeStateChanged([[maybe_unused]] const ScriptCanvas::NodeStateChange& stateChangeSignal)
    {
    }

    void LoggingDataAggregator::ProcessInputSignal(const ScriptCanvas::InputSignal& inputSignal)
    {
        if (!m_hasAnchor)
        {
            m_hasAnchor = true;
            m_anchorTimeStamp = inputSignal.GetTimestamp();
        }

        // For every input we always want to make a new element.
        ScriptCanvas::Timestamp relativeTimeStamp = inputSignal.GetTimestamp() - m_anchorTimeStamp;
        ExecutionLogTreeItem* treeItem = m_debugLogRoot->CreateExecutionItem(GetDataId(), inputSignal.m_nodeType, inputSignal, inputSignal.m_endpoint.GetNamedNodeId());

        m_lastAggregateItemMap[inputSignal] = treeItem;

        treeItem->RegisterExecutionInput(ScriptCanvas::Endpoint(), inputSignal.m_endpoint.GetSlotId(), inputSignal.m_endpoint.GetSlotName(), AZStd::chrono::milliseconds(relativeTimeStamp));

        for (auto dataMap : inputSignal.m_data)
        {
            AZStd::string valueString = dataMap.second.m_datum.ToString();
            treeItem->RegisterDataInput(ScriptCanvas::Endpoint(), dataMap.first, dataMap.first.m_name, valueString, GetTreeRoot()->GetUpdatePolicy() == DebugLogRootItem::UpdatePolicy::RealTime);
        }
    }

    void LoggingDataAggregator::ProcessOutputSignal(const ScriptCanvas::OutputSignal& outputSignal)
    {
        if (!m_hasAnchor)
        {
            m_hasAnchor = true;
            m_anchorTimeStamp = outputSignal.GetTimestamp();
        }

        // For the output we want to correlate it with the appropriate starting node
        auto lastAggregateIter = m_lastAggregateItemMap.find(outputSignal);

        ExecutionLogTreeItem* treeItem = nullptr;

        if (lastAggregateIter != m_lastAggregateItemMap.end())
        {
            treeItem = lastAggregateIter->second;

            if (treeItem->HasExecutionOutput()
                || treeItem->GetNodeId() != outputSignal.m_endpoint.GetNodeId())
            {
                treeItem = nullptr;
            }
        }

        if (treeItem == nullptr)
        {
            treeItem = m_debugLogRoot->CreateExecutionItem(GetDataId(), outputSignal.m_nodeType, outputSignal, outputSignal.m_endpoint.GetNamedNodeId());
            m_lastAggregateItemMap[outputSignal] = treeItem;
        }

        ScriptCanvas::Timestamp relativeTimeStamp = outputSignal.GetTimestamp() - m_anchorTimeStamp;
        treeItem->RegisterExecutionOutput(outputSignal.m_endpoint.GetSlotId(), outputSignal.m_endpoint.GetSlotName(), AZStd::chrono::milliseconds(relativeTimeStamp));

        for (auto dataMap : outputSignal.m_data)
        {
            AZStd::string valueString = dataMap.second.m_datum.ToString();
            treeItem->RegisterDataOutput(dataMap.first, dataMap.first.m_name, valueString, GetTreeRoot()->GetUpdatePolicy() == DebugLogRootItem::UpdatePolicy::RealTime);
        }
    }

    void LoggingDataAggregator::ProcessAnnotateNode(const ScriptCanvas::AnnotateNodeSignal& annotateNodeSignal)
    {
        auto lastAggregateIter = m_lastAggregateItemMap.find(annotateNodeSignal);

        if (lastAggregateIter != m_lastAggregateItemMap.end())
        {
            ExecutionLogTreeItem* treeItem = lastAggregateIter->second;

            treeItem->RegisterAnnotation(annotateNodeSignal, GetTreeRoot()->GetUpdatePolicy() == DebugLogRootItem::UpdatePolicy::RealTime);
        }
    }

    void LoggingDataAggregator::ProcessVariableChangedSignal([[maybe_unused]] const ScriptCanvas::VariableChange& variableChangeSignal)
    {

    }

    DebugLogRootItem* LoggingDataAggregator::GetTreeRoot() const
    {
        return m_debugLogRoot;
    }

    void LoggingDataAggregator::RegisterEntityName(const AZ::EntityId& entityId, AZStd::string_view entityName)
    {
        auto nameIter = m_entityNameCache.find(entityId);

        if (nameIter == m_entityNameCache.end())
        {
            m_entityNameCache[entityId] = entityName;
        }
    }

    void LoggingDataAggregator::UnregisterEntityName(const AZ::EntityId& entityId)
    {
        // While we are capturing, we never want to update this list.
        if (!IsCapturingData())
        {
            m_entityNameCache.erase(entityId);
        }
    }

    void LoggingDataAggregator::OnRegistrationEnabled(const AZ::NamedEntityId&, const ScriptCanvas::GraphIdentifier&)
    {

    }

    void LoggingDataAggregator::OnRegistrationDisabled(const AZ::NamedEntityId&, const ScriptCanvas::GraphIdentifier&)
    {

    }

    void LoggingDataAggregator::ResetLog()
    {
        m_debugLogRoot->ResetData();
    }

    void LoggingDataAggregator::ResetData()
    {
        m_endpointData.clear();
        m_variableData.clear();

        for (const auto& registrationPair : m_registrationMap)
        {
            LoggingDataNotificationBus::Event(GetDataId(), &LoggingDataNotifications::OnEntityGraphUnregistered, registrationPair.first, registrationPair.second);
        }

        m_registrationMap.clear();

        // Entity registrations are all transient. We need to clear them when we reset data.
        // The assets should be static, so we can persist them.
        m_loggingEntityMapping.clear();

        m_lastAggregateItemMap.clear();
        m_lastExecutionThreadMap.clear();

        if (!IsCapturingData())
        {
            m_entityNameCache.clear();
        }

        m_hasAnchor = false;
        m_anchorTimeStamp = ScriptCanvas::Timestamp(0);
    }

    void LoggingDataAggregator::RegisterScriptCanvas(const AZ::NamedEntityId& entityId, const ScriptCanvas::GraphIdentifier& graphIdentifier)
    {
         bool foundMatch = false;
 
         auto matchedRange = m_registrationMap.equal_range(entityId);
         for (auto mapIter = matchedRange.first; mapIter != matchedRange.second; ++mapIter)
         {
             if (mapIter->second == graphIdentifier)
             {
                 foundMatch = true;
                 AZ_Warning("ScriptCanvas", false, "Received a duplicated registration callback.");
                 break;
             }
         }
 
         if (!foundMatch)
         {
             m_registrationMap.insert(AZStd::make_pair(entityId, graphIdentifier));
             LoggingDataNotificationBus::Event(GetDataId(), &LoggingDataNotifications::OnEntityGraphRegistered, entityId, graphIdentifier);
         }
    }

    void LoggingDataAggregator::UnregisterScriptCanvas(const AZ::NamedEntityId& entityId, const ScriptCanvas::GraphIdentifier& graphIdentifier)
    {
        bool foundMatch = false;

        {
            auto matchedRange = m_registrationMap.equal_range(entityId);
            for (auto mapIter = matchedRange.first; mapIter != matchedRange.second; ++mapIter)
            {
                if (mapIter->second == graphIdentifier)
                {
                    foundMatch = true;
                    m_registrationMap.erase(mapIter);
                    break;
                }
            }
        }

        {
            auto matchedRange = m_loggingEntityMapping.equal_range(entityId);
            for (auto mapIter = matchedRange.first; mapIter != matchedRange.second; ++mapIter)
            {
                if (mapIter->second == graphIdentifier)
                {
                    m_loggingEntityMapping.erase(mapIter);
                    break;
                }
            }
        }

        if (foundMatch)
        {
            LoggingDataNotificationBus::Event(GetDataId(), &LoggingDataNotifications::OnEntityGraphUnregistered, entityId, graphIdentifier);
        }
    }
}
