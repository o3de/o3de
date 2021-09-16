/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/EntityUtils.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <ScriptCanvas/Core/Endpoint.h>
#include <ScriptCanvas/Core/ExecutionNotificationsBus.h>
#include <ScriptCanvas/Variable/VariableCore.h>

#include <Editor/View/Widgets/LoggingPanel/LoggingWindowTreeItems.h>

#include <Editor/View/Widgets/LoggingPanel/LoggingTypes.h>

namespace ScriptCanvasEditor
{
    class LoggingDataAggregator;

    class LoggingDataRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        
        using BusIdType = LoggingDataId;
        
        virtual bool IsCapturingData() const = 0;

        // Return the object to allow for certain large data elements to be passed by reference instead of by value.
        virtual const LoggingDataAggregator* FindLoggingData() const = 0;

        virtual void EnableRegistration(const AZ::NamedEntityId& namedEntityId, const ScriptCanvas::GraphIdentifier& graphIdentifier) = 0;
        virtual void DisableRegistration(const AZ::NamedEntityId& namedEntityId, const ScriptCanvas::GraphIdentifier& graphIdentifier) = 0;

        virtual AZ::NamedEntityId FindNamedEntityId(const AZ::EntityId& entityId) = 0;
    };
    
    using LoggingDataRequestBus = AZ::EBus<LoggingDataRequests>;
    
    class LoggingDataNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = LoggingDataId;

        virtual void OnDataCaptureBegin() {};
        virtual void OnDataCaptureEnd() {};
        
        virtual void OnEntityGraphRegistered([[maybe_unused]] const AZ::NamedEntityId& entityId, [[maybe_unused]] const ScriptCanvas::GraphIdentifier& assetId) {};
        virtual void OnEntityGraphUnregistered([[maybe_unused]] const AZ::NamedEntityId& entityId, [[maybe_unused]] const ScriptCanvas::GraphIdentifier& assetId) {}

        virtual void OnEnabledStateChanged([[maybe_unused]] bool isEnabled, [[maybe_unused]] const AZ::NamedEntityId& namedEntityId, [[maybe_unused]] const ScriptCanvas::GraphIdentifier& graphIdentifier) {}

        // TODO: Find a better spot for this
        virtual void OnTreeItemAdded() {}
    };
    
    using LoggingDataNotificationBus = AZ::EBus<LoggingDataNotifications>;

    // Container class for all of the local elements
    class LoggingDataAggregator
        : public LoggingDataRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(LoggingDataAggregator, AZ::SystemAllocator,0);

        LoggingDataAggregator();
        ~LoggingDataAggregator();

        const LoggingDataId& GetDataId() const;
        
        // LoggedDataRequests
        const LoggingDataAggregator* FindLoggingData() const override;
        
        void EnableRegistration(const AZ::NamedEntityId& namedEntityId, const ScriptCanvas::GraphIdentifier& graphIdentifier) override;
        void DisableRegistration(const AZ::NamedEntityId& namedEntityId, const ScriptCanvas::GraphIdentifier& graphIdentifier) override;

        AZ::NamedEntityId FindNamedEntityId(const AZ::EntityId& entityId) override;
        ////

        bool IsCapturingData() const override = 0;
        virtual bool CanCaptureData() const = 0;
        
        // Should be bus methods, but don't want to copy data
        const EntityGraphRegistrationMap& GetEntityGraphRegistrationMap() const;

        const LoggingEntityMap& GetLoggingEntityMap() const;
        const LoggingAssetSet& GetLoggingAssetSet() const;
        ////

        //
        void ProcessSignal(const ScriptCanvas::Signal& signal);

        void ProcessNodeStateChanged(const ScriptCanvas::NodeStateChange& stateChangeSignal);
        void ProcessInputSignal(const ScriptCanvas::InputSignal& inputSignal);
        void ProcessOutputSignal(const ScriptCanvas::OutputSignal& outputSignal);
        void ProcessAnnotateNode(const ScriptCanvas::AnnotateNodeSignal& annotateNodeSignal);

        void ProcessVariableChangedSignal(const ScriptCanvas::VariableChange& variableChangeSignal);
        ////

        DebugLogRootItem* GetTreeRoot() const;

        void RegisterEntityName(const AZ::EntityId& entityId, AZStd::string_view entityName);
        void UnregisterEntityName(const AZ::EntityId& entityId);

    protected:

        // Methods here for child elements to do something with the data.
        virtual void OnRegistrationEnabled(const AZ::NamedEntityId& namedEntityId, const ScriptCanvas::GraphIdentifier& graphIdentifier);
        virtual void OnRegistrationDisabled(const AZ::NamedEntityId& namedEntityId, const ScriptCanvas::GraphIdentifier& graphIdentifier);

        void ResetData();
        void ResetLog();
        void RegisterScriptCanvas(const AZ::NamedEntityId& entityId, const ScriptCanvas::GraphIdentifier& graphIdentifier);
        void UnregisterScriptCanvas(const AZ::NamedEntityId& entityId, const ScriptCanvas::GraphIdentifier& graphIdentifier);        
        
        // Parsed Data Information
        //
        // Debug Context Information
        //
        // Will be used for visually displaying the data once we get to it.
        AZStd::unordered_map< ScriptCanvas::Endpoint, AZStd::string > m_endpointData;
        AZStd::unordered_map< ScriptCanvas::VariableId, AZStd::string > m_variableData;
        ////

        AZStd::unordered_map< AZ::EntityId, AZStd::string >                         m_entityNameCache;

        AZStd::unordered_map< ScriptCanvas::GraphInfo, ExecutionLogTreeItem* > m_lastAggregateItemMap;
        AZStd::unordered_map< ScriptCanvas::GraphInfo, AZStd::vector<ExecutionIdentifier>> m_lastExecutionThreadMap;
    
    private:

        DebugLogRootItem*               m_debugLogRoot;
    
        // State Information
        LoggingDataId     m_id;

        bool m_ignoreRegistrations;

        bool m_hasAnchor;
        ScriptCanvas::Timestamp         m_anchorTimeStamp;

        // TODO: Consider wrapping the three of these up into a single struct.
        EntityGraphRegistrationMap      m_registrationMap;

        LoggingEntityMap    m_loggingEntityMapping;
        LoggingAssetSet     m_loggedAssetSet;
    };
}
