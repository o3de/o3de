/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Editor/Include/ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <Editor/View/Widgets/LoggingPanel/LoggingDataAggregator.h>

#include <ScriptCanvas/Debugger/Bus.h>
#include <ScriptCanvas/Debugger/Logger.h>

namespace ScriptCanvasEditor
{
    class LiveLoggingDataAggregator
        : public LoggingDataAggregator
        , public EditorLoggingComponentNotificationBus::Handler
        , public ScriptCanvas::Debugger::ServiceNotificationsBus::Handler
        , public ScriptCanvas::Debugger::ClientUINotificationBus::Handler
    {
        enum CaptureType
        {
            Editor,
            External
        };

    public:
        AZ_CLASS_ALLOCATOR(LiveLoggingDataAggregator, AZ::SystemAllocator);
        LiveLoggingDataAggregator();
        ~LiveLoggingDataAggregator();

        // ClientUINotificationBus
        void OnCurrentTargetChanged() override;
        ////

        bool CanCaptureData() const override;
        bool IsCapturingData() const override;

        void StartCaptureData();
        void StopCaptureData();
        
        // EditorLoggingComponentNotifications
        void OnEditorScriptCanvasComponentActivated(const AZ::NamedEntityId& namedEntityId, const ScriptCanvas::GraphIdentifier& graphIdentifier) override;
        void OnEditorScriptCanvasComponentDeactivated(const AZ::NamedEntityId& namedEntityId, const ScriptCanvas::GraphIdentifier& graphIdentifier) override;
        void OnAssetSwitched(const AZ::NamedEntityId& namedEntityId, const ScriptCanvas::GraphIdentifier& newAssetId, const ScriptCanvas::GraphIdentifier& oldAssetId) override;
        ////

        // ServiceNotifications
        //// Logging Notifications
        void Connected(const ScriptCanvas::Debugger::Target& target) override;
        void GraphActivated(const ScriptCanvas::GraphActivation& activatedSignal) override;
        void GraphDeactivated(const ScriptCanvas::GraphDeactivation& deactivatedSignal) override;
        
        void NodeStateChanged(const ScriptCanvas::NodeStateChange& stateChange) override;
        void SignaledInput(const ScriptCanvas::InputSignal& inputSignal) override;
        void SignaledOutput(const ScriptCanvas::OutputSignal& outputSignal) override;
        void AnnotateNode(const ScriptCanvas::AnnotateNodeSignal& annotateNode) override;

        void VariableChanged(const ScriptCanvas::VariableChange& variableChanged) override;

        //// Result Methods
        void GetActiveEntitiesResult(const ScriptCanvas::ActiveEntityStatusMap& activeEntityMap) override;
        ////

        const AZStd::unordered_multimap<AZ::NamedEntityId, ScriptCanvas::GraphIdentifier>& GetStaticRegistrations() const;

    protected:

        void OnRegistrationEnabled(const AZ::NamedEntityId& namedEntityId, const ScriptCanvas::GraphIdentifier& graphIdentifier) override;
        void OnRegistrationDisabled(const AZ::NamedEntityId& namedEntityId, const ScriptCanvas::GraphIdentifier& graphIdentifier) override;
        
    private:

        void AddStaticRegistration(const AZ::NamedEntityId& namedEntityId, const ScriptCanvas::GraphIdentifier& graphIdentifier);
        void RemoveStaticRegistration(const AZ::NamedEntityId& namedEntityId, const ScriptCanvas::GraphIdentifier& graphIdentifier);

        void SetupEditorEntities();
        void SetupExternalEntities();

        CaptureType m_captureType;
        bool m_isCapturingData;
        bool m_ignoreRegistrations;

        AZStd::recursive_mutex m_notificationMutex;

        ScriptCanvas::Debugger::Logger m_logger;
        AZStd::unordered_multimap<AZ::NamedEntityId, ScriptCanvas::GraphIdentifier> m_staticRegistrations;
    };
}
