/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Interface/Interface.h>
#include <AzCore/std/containers/vector.h>

#include <AzFramework/Network/IRemoteTools.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

#include <Editor/View/Widgets/LoggingPanel/LiveWindowSession/LiveLoggingDataAggregator.h>
#include <ScriptCanvas/Debugger/API.h>
#include <ScriptCanvas/Asset/ExecutionLogAssetBus.h>
#include <ScriptCanvas/Core/ExecutionNotificationsBus.h>
#include <ScriptCanvas/Execution/RuntimeComponent.h>
#include <ScriptCanvas/Utils/ScriptCanvasConstants.h>

namespace ScriptCanvasEditor
{
    //////////////////////////////
    // LiveLoggingDataAggregator
    //////////////////////////////
    
    LiveLoggingDataAggregator::LiveLoggingDataAggregator()
        : m_captureType(CaptureType::Editor)
        , m_isCapturingData(false)
        , m_ignoreRegistrations(false)
    {
        ScriptCanvas::Debugger::ClientUINotificationBus::Handler::BusConnect();

        OnCurrentTargetChanged();
    }
    
    LiveLoggingDataAggregator::~LiveLoggingDataAggregator()
    {
    }
    
    void LiveLoggingDataAggregator::OnCurrentTargetChanged()
    {
        ResetData();

        bool isConnected = false;
        ScriptCanvas::Debugger::ClientRequestsBus::BroadcastResult(isConnected, &ScriptCanvas::Debugger::ClientRequests::HasValidConnection);
        
        if (isConnected)
        {
            EditorLoggingComponentNotificationBus::Handler::BusDisconnect();

            if (!ScriptCanvas::Debugger::ServiceNotificationsBus::Handler::BusIsConnected())
            {
                ScriptCanvas::Debugger::ServiceNotificationsBus::Handler::BusConnect();
            }

            bool isSelf = false;
            ScriptCanvas::Debugger::ClientRequestsBus::BroadcastResult(isSelf, &ScriptCanvas::Debugger::ClientRequests::IsConnectedToSelf);

            if (!isSelf)
            {
                m_captureType = CaptureType::External;
                m_staticRegistrations.clear();
            }
        }
        else
        {
            if (!EditorLoggingComponentNotificationBus::Handler::BusIsConnected())
            {
                EditorLoggingComponentNotificationBus::Handler::BusConnect();
            }

            ScriptCanvas::Debugger::ServiceNotificationsBus::Handler::BusDisconnect();

            m_captureType = CaptureType::Editor;
            SetupEditorEntities();
        }
    }

    bool LiveLoggingDataAggregator::CanCaptureData() const
    {
        return true;
    }

    bool LiveLoggingDataAggregator::IsCapturingData() const
    {
        return m_isCapturingData;
    }

    void LiveLoggingDataAggregator::OnEditorScriptCanvasComponentActivated(const AZ::NamedEntityId& namedEntityId, const ScriptCanvas::GraphIdentifier& graphIdentifier)
    {
        if (graphIdentifier.m_assetId.IsValid())
        {
            RegisterScriptCanvas(namedEntityId, graphIdentifier);
        }
    }
    
    void LiveLoggingDataAggregator::OnEditorScriptCanvasComponentDeactivated(const AZ::NamedEntityId& namedEntityId, const ScriptCanvas::GraphIdentifier& graphIdentifier)
    {
        UnregisterScriptCanvas(namedEntityId, graphIdentifier);
    }
    
    void LiveLoggingDataAggregator::OnAssetSwitched(const AZ::NamedEntityId& namedEntityId, const ScriptCanvas::GraphIdentifier& newGraphIdentifier, const ScriptCanvas::GraphIdentifier& oldGraphIdentifier)
    {
        if (newGraphIdentifier == oldGraphIdentifier)
        {
            return;
        }

        if (newGraphIdentifier.m_assetId.IsValid())
        {
            RegisterScriptCanvas(namedEntityId, newGraphIdentifier);
        }

        UnregisterScriptCanvas(namedEntityId, oldGraphIdentifier);
        RemoveStaticRegistration(namedEntityId, oldGraphIdentifier);
    }

    void LiveLoggingDataAggregator::Connected([[maybe_unused]] const ScriptCanvas::Debugger::Target& target)
    {
        AZStd::lock(m_notificationMutex);
        SetupExternalEntities();
    }

    void LiveLoggingDataAggregator::GraphActivated(const ScriptCanvas::GraphActivation& activationSignal)
    {
        AZStd::lock(m_notificationMutex);
        RegisterScriptCanvas(activationSignal.m_runtimeEntity, activationSignal.m_graphIdentifier);
        RegisterEntityName(activationSignal.m_runtimeEntity, activationSignal.m_runtimeEntity.GetName());
        LoggingDataNotificationBus::Event(GetDataId(), &LoggingDataNotifications::OnEnabledStateChanged, activationSignal.m_entityIsObserved, activationSignal.m_runtimeEntity, activationSignal.m_graphIdentifier);
    }

    void LiveLoggingDataAggregator::GraphDeactivated(const ScriptCanvas::GraphDeactivation& deactivationSignal)
    {
        AZStd::lock(m_notificationMutex);
        UnregisterScriptCanvas(deactivationSignal.m_runtimeEntity, deactivationSignal.m_graphIdentifier);
    }

    void LiveLoggingDataAggregator::NodeStateChanged(const ScriptCanvas::NodeStateChange& nodeStateChangeSignal)
    {
        AZStd::lock(m_notificationMutex);
        ProcessNodeStateChanged(nodeStateChangeSignal);
    }

    void LiveLoggingDataAggregator::SignaledInput(const ScriptCanvas::InputSignal& inputSignal)
    {
        AZStd::lock(m_notificationMutex);
        ProcessInputSignal(inputSignal);
    }

    void LiveLoggingDataAggregator::SignaledOutput(const ScriptCanvas::OutputSignal& outputSignal)
    {
        AZStd::lock(m_notificationMutex);
        ProcessOutputSignal(outputSignal);
    }

    void LiveLoggingDataAggregator::AnnotateNode(const ScriptCanvas::AnnotateNodeSignal& annotateNode)
    {
        AZStd::lock(m_notificationMutex);
        ProcessAnnotateNode(annotateNode);
    }

    void LiveLoggingDataAggregator::VariableChanged(const ScriptCanvas::VariableChange& variableChangeSignal)
    {
        AZStd::lock(m_notificationMutex);
        ProcessVariableChangedSignal(variableChangeSignal);
    }

    void LiveLoggingDataAggregator::GetActiveEntitiesResult(const ScriptCanvas::ActiveEntityStatusMap& activeEntities)
    {
        AZStd::lock(m_notificationMutex);
        m_ignoreRegistrations = true;

        for (const auto& activeEntityPair : activeEntities)
        {
            const AZ::NamedEntityId& namedEntityId = activeEntityPair.first;
            RegisterEntityName(namedEntityId, namedEntityId.GetName());

            const auto& activeEntityStatus = activeEntityPair.second;
            
            for (const auto& activeGraphStatus : activeEntityStatus.m_activeGraphs)
            {
                RegisterScriptCanvas(namedEntityId, activeGraphStatus.first);
                LoggingDataNotificationBus::Event(GetDataId(), &LoggingDataNotifications::OnEnabledStateChanged, activeGraphStatus.second.m_isObserved, namedEntityId, activeGraphStatus.first);
            }
        }

        m_ignoreRegistrations = false;
    }

    const AZStd::unordered_multimap<AZ::NamedEntityId, ScriptCanvas::GraphIdentifier>& LiveLoggingDataAggregator::GetStaticRegistrations() const
    {
        return m_staticRegistrations;
    }

    void LiveLoggingDataAggregator::OnRegistrationEnabled(const AZ::NamedEntityId& namedEntityId, const ScriptCanvas::GraphIdentifier& graphIdentifier)
    {
        if (m_ignoreRegistrations)
        {
            return;
        }

        if (IsCapturingData() || m_captureType == External)
        {
            if (graphIdentifier.m_componentId == k_dynamicallySpawnedControllerId)
            {
                ScriptCanvas::Debugger::ClientUIRequestBus::Broadcast(&ScriptCanvas::Debugger::ClientUIRequests::AddGraphLoggingTarget, graphIdentifier.m_assetId);
            }
            else
            {
                ScriptCanvas::Debugger::ClientUIRequestBus::Broadcast(&ScriptCanvas::Debugger::ClientUIRequests::AddEntityLoggingTarget, namedEntityId, graphIdentifier);

                bool gotResult = false;
                AZ::EntityId editorId;

                AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(gotResult, &AzToolsFramework::EditorEntityContextRequests::MapRuntimeIdToEditorId, namedEntityId, editorId);

                if (gotResult)
                {
                    AZ::NamedEntityId namedEditorId(editorId, namedEntityId.GetName());

                    AddStaticRegistration(namedEditorId, graphIdentifier);
                }
            }

            return;
        }

        AddStaticRegistration(namedEntityId, graphIdentifier);
    }

    void LiveLoggingDataAggregator::OnRegistrationDisabled(const AZ::NamedEntityId& namedEntityId, const ScriptCanvas::GraphIdentifier& graphIdentifier)
    {
        if (m_ignoreRegistrations)
        {
            return;
        }

        if (IsCapturingData() || m_captureType == External)
        {
            if (graphIdentifier.m_componentId == k_dynamicallySpawnedControllerId)
            {
                ScriptCanvas::Debugger::ClientUIRequestBus::Broadcast(&ScriptCanvas::Debugger::ClientUIRequests::RemoveGraphLoggingTarget, graphIdentifier.m_assetId);
            }
            else
            {
                ScriptCanvas::Debugger::ClientUIRequestBus::Broadcast(&ScriptCanvas::Debugger::ClientUIRequests::RemoveEntityLoggingTarget, namedEntityId, graphIdentifier);

                if (m_captureType == Editor)
                {
                    bool gotResult = false;
                    AZ::EntityId editorId;

                    AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(gotResult, &AzToolsFramework::EditorEntityContextRequests::MapRuntimeIdToEditorId, namedEntityId, editorId);

                    if (gotResult)
                    {
                        AZ::NamedEntityId namedEditorId(editorId, namedEntityId.GetName());
                        RemoveStaticRegistration(namedEditorId, graphIdentifier);
                    }
                }
            }

            return;
        }

        RemoveStaticRegistration(namedEntityId, graphIdentifier);
    }

    void LiveLoggingDataAggregator::AddStaticRegistration(const AZ::NamedEntityId& namedEntityId, const ScriptCanvas::GraphIdentifier& graphIdentifier)
    {
        if (graphIdentifier.m_componentId == k_dynamicallySpawnedControllerId
            || m_captureType != Editor)
        {
            return;
        }

        bool registerEvent = true;

        auto mapRange = m_staticRegistrations.equal_range(namedEntityId);

        for (auto mapIter = mapRange.first; mapIter != mapRange.second; ++mapIter)
        {
            if (mapIter->second == graphIdentifier)
            {
                registerEvent = false;
                break;
            }
        }

        if (registerEvent)
        {
            m_staticRegistrations.insert(AZStd::make_pair(namedEntityId, graphIdentifier));
        }
    }

    void LiveLoggingDataAggregator::RemoveStaticRegistration(const AZ::NamedEntityId& namedEntityId, const ScriptCanvas::GraphIdentifier& graphIdentifier)
    {
        if (graphIdentifier.m_componentId == k_dynamicallySpawnedControllerId
            || m_captureType != Editor)
        {
            return;
        }

        auto mapRange = m_staticRegistrations.equal_range(namedEntityId);

        for (auto mapIter = mapRange.first; mapIter != mapRange.second; ++mapIter)
        {
            if (mapIter->second == graphIdentifier)
            {
                m_staticRegistrations.erase(mapIter);
                break;
            }
        }
    }
    
    void LiveLoggingDataAggregator::SetupEditorEntities()
    {
        m_ignoreRegistrations = true;

        EditorScriptCanvasComponentLoggingBus::EnumerateHandlers([this](EditorScriptCanvasComponentLogging* loggingComponent)
        {
            ScriptCanvas::GraphIdentifier graphIdentifier = loggingComponent->GetGraphIdentifier();

            if (graphIdentifier.m_assetId.IsValid())
            {
                RegisterScriptCanvas(loggingComponent->FindNamedEntityId(), graphIdentifier);
            }
            
            return true;
        });

        for (const auto& mapPair : m_staticRegistrations)
        {
            LoggingDataNotificationBus::Event(GetDataId(), &LoggingDataNotifications::OnEnabledStateChanged, true, mapPair.first, mapPair.second);
        }

        m_ignoreRegistrations = false;
    }

    void LiveLoggingDataAggregator::SetupExternalEntities()
    {
        ScriptCanvas::Debugger::ClientRequestsBus::Broadcast(&ScriptCanvas::Debugger::ClientRequests::GetActiveEntities);
    }

    void LiveLoggingDataAggregator::StartCaptureData()
    {
        AZStd::lock(m_notificationMutex);
        m_isCapturingData = true;

        ResetLog();

        ScriptCanvas::Debugger::ServiceNotificationsBus::Handler::BusConnect();
    }

    void LiveLoggingDataAggregator::StopCaptureData()
    {
        AZStd::lock(m_notificationMutex);
        m_isCapturingData = false;

        ResetData();

        const AZStd::string name = AZStd::string::format("ScriptCanvasLog_%s", AZStd::to_string(AZStd::GetTimeUTCMilliSecond()).data());
        ScriptCanvas::ExecutionLogAssetEBus::Broadcast(&ScriptCanvas::ExecutionLogAssetBus::SaveToRelativePath, name);

        if (m_captureType == CaptureType::Editor)
        {
            bool isDesiredTargetConnected = false;
            AzFramework::IRemoteTools* remoteTools = AzFramework::RemoteToolsInterface::Get();
            if (remoteTools)
            {
                const AzFramework::RemoteToolsEndpointInfo& desiredTarget = remoteTools->GetDesiredEndpoint(ScriptCanvas::RemoteToolsKey);
                isDesiredTargetConnected = desiredTarget.IsOnline();
            }

            if (isDesiredTargetConnected)
            {
                SetupExternalEntities();
            }
            else
            {
                SetupEditorEntities();
            }
        }
        else
        {
            SetupExternalEntities();
        }

        ScriptCanvas::ExecutionLogAssetEBus::Broadcast(&ScriptCanvas::ExecutionLogAssetBus::ClearLog);

        ScriptCanvas::Debugger::ServiceNotificationsBus::Handler::BusDisconnect();
    }
}
