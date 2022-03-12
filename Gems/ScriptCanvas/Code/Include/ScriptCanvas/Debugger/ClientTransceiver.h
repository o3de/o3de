/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/Component/TickBus.h>
#include <ScriptCanvas/Core/Core.h>
#include <AzCore/EBus/EBus.h>
#include <Debugger/Bus.h>
#include <Debugger/Messages/Notify.h>

#include "APIArguments.h"
#include "Logger.h"
#include "LogReader.h"

namespace ScriptCanvas
{
    namespace Debugger
    {
        /**
         * ClientTransceiver 
         * listens to debugger service messages on the target manager bus, and translates them
         * to ServiceNotifications
         *
         * listens to client requests, and translates them to debugger client request messages
         *
         * listens to target manager client messages and translated them to service notifications
         */
        class ClientTransceiver
            : public Message::NotificationVisitor
            , public AzFramework::TargetManagerClient::Bus::Handler
            , public AzFramework::TmMsgBus::Handler
            , public AZ::SystemTickBus::Handler
            , public ClientRequestsBus::Handler
            , public ClientUIRequestBus::Handler
        {
            using Mutex = AZStd::recursive_mutex;
            using Lock = AZStd::lock_guard<Mutex>;

        public:
            AZ_CLASS_ALLOCATOR(ClientTransceiver, AZ::SystemAllocator, 0);
            AZ_RTTI(ClientTransceiver, "{C6F5ACDC-5415-48FE-A7C3-E6398FDDED33}");
            
            ClientTransceiver();
            ~ClientTransceiver();
            
            //////////////////////////////////////////////////////////////////////////
            // ClientRequests
            AzFramework::TargetContainer EnumerateAvailableNetworkTargets() override;

            bool HasValidConnection() const override;
            bool IsConnected(const AzFramework::TargetInfo&) const override;
            bool IsConnectedToSelf() const override;
            AzFramework::TargetInfo GetNetworkTarget() override;

            void AddBreakpoint(const Breakpoint&) override;
            void AddVariableChangeBreakpoint(const VariableChangeBreakpoint&) override;
            void Break() override;
            void Continue() override;
            void RemoveBreakpoint(const Breakpoint&) override;
            void RemoveVariableChangeBreakpoint(const VariableChangeBreakpoint&) override;

            void SetVariableValue() override;
            void StepOver() override;

            void GetAvailableScriptTargets() override;
            void GetActiveEntities() override;
            void GetActiveGraphs() override;
            void GetVariableValue() override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // TargetManagerClient
            void DesiredTargetConnected(bool connected) override;
            void DesiredTargetChanged(AZ::u32 newId, AZ::u32 oldId) override;
            void TargetJoinedNetwork(AzFramework::TargetInfo info) override;
            void TargetLeftNetwork(AzFramework::TargetInfo info) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // TmMsgBus
            void OnReceivedMsg(AzFramework::TmMsgPtr msg) override;
            //////////////////////////////////////////////////////////////////////////
            
            //////////////////////////////////////////////////////////////////////////
            // Message processing
            void Visit(Message::ActiveEntitiesResult& notification) override;
            void Visit(Message::ActiveGraphsResult& notification) override;
            void Visit(Message::AnnotateNode& notification) override;
            void Visit(Message::AvailableScriptTargetsResult& notification) override;
            void Visit(Message::BreakpointAdded& notification) override;
            void Visit(Message::BreakpointHit& notification) override;
            void Visit(Message::Connected& notification) override;
            void Visit(Message::Disconnected& notification) override;
            void Visit(Message::Continued& notification) override;
            void Visit(Message::GraphActivated& notification) override;
            void Visit(Message::GraphDeactivated& notification) override;
            void Visit(Message::SignaledInput& notification) override;
            void Visit(Message::SignaledOutput& notification) override;
            void Visit(Message::VariableChanged& notification) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // SystemTickBus
            void OnSystemTick() override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // ClientUIRequestBus
            void StartEditorSession() override;
            void StopEditorSession() override;

            void StartLogging(ScriptTarget& initialTargets) override;
            void StopLogging() override;

            void AddEntityLoggingTarget(const AZ::EntityId& entityId, const ScriptCanvas::GraphIdentifier& assetId) override;
            void RemoveEntityLoggingTarget(const AZ::EntityId& entityId, const ScriptCanvas::GraphIdentifier& assetId) override;

            void AddGraphLoggingTarget(const AZ::Data::AssetId& assetId) override;
            void RemoveGraphLoggingTarget(const AZ::Data::AssetId& assetId) override;
            //////////////////////////////////////////////////////////////////////////

        protected:
            void DiscoverNetworkTargets();
            void BreakpointAdded(const Breakpoint& breakpoint);
            void ClearMessages();
            void ProcessMessages();
            
        private:

            void DisconnectFromTarget();
            void CleanupConnection();

            Mutex m_mutex;
            bool m_breakOnNext = true;

            AzFramework::TargetInfo m_selfTarget;

            bool m_resetDesiredTarget = false;
            AzFramework::TargetInfo m_previousDesiredInfo;

            AzFramework::TargetInfo m_currentTarget;
            ScriptTarget m_connectionState;

            AzFramework::TargetContainer m_networkTargets;
            AZStd::unordered_set<Breakpoint> m_breakpointsActive;
            AZStd::unordered_set<Breakpoint> m_breakpointsInactive;

            ScriptTarget m_addCache;
            ScriptTarget m_removeCache;
            
            Mutex m_msgMutex;
            AzFramework::TmMsgQueue m_msgQueue;
        };
    }
}
