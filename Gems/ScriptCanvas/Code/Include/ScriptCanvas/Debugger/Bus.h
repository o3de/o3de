/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/Component/EntityId.h>
#include <ScriptCanvas/Core/Core.h>
#include <AzCore/EBus/EBus.h>

#include "APIArguments.h"

namespace ScriptCanvas
{
    namespace Debugger
    {
        class ServiceNotifications : public AZ::EBusTraits
        {
        public:
            virtual ~ServiceNotifications() = default;

            // Target Management Methods
            virtual void BecameUnavailable(const Target&) {}
            virtual void BecameAvailable(const Target&) {}

            virtual void Connected(const Target&) {}
            virtual void ConnectionRefused(const Target&) {}
            virtual void Disconnected() {}
            
            // Logging Notifications
            virtual void GraphActivated(const GraphActivation&) {}
            virtual void GraphDeactivated(const GraphDeactivation&) {}

            virtual void ExecutionThreadEnded(const ExecutionThreadEnd&) {}
            virtual void ExecutionThreadBegun(const ExecutionThreadBeginning&) {}

            virtual void NodeStateChanged(const NodeStateChange&) {}
            virtual void SignaledInput(const InputSignal&) {}
            virtual void SignaledOutput(const OutputSignal&) {}
            virtual void SignaledDataOutput(const OutputDataSignal&) {}
            virtual void AnnotateNode(const AnnotateNodeSignal&) {}
            virtual void VariableChanged(const VariableChange&) {}

            // Result Methods
            virtual void GetAvailableScriptTargetResult(const ActiveEntitiesAndGraphs&) {}
            virtual void GetActiveEntitiesResult(const ActiveEntityStatusMap&) {}
            virtual void GetActiveGraphsResult(const ActiveGraphStatusMap&) {}
            virtual void GetVariableValueResult(const DatumValue&) {}

            // Control Methods
            virtual void BreakPointAdded(const Breakpoint&) {}
            virtual void BreakPointHit(const Breakpoint&) {}
            virtual void BreakPointRemoved(const Breakpoint&) {}

            virtual void Continued(const Target&) {}

            virtual void VariableChangeBreakpointAdded(const VariableChangeBreakpoint&) {}
            virtual void VariableChangeBreakpointHit(const VariableChangeBreakpoint&) {}
            virtual void VariableChangeBreakpointRemoved(const VariableChangeBreakpoint&) {}
        };
        using ServiceNotificationsBus = AZ::EBus<ServiceNotifications>;

        class ClientRequests : public AZ::EBusTraits
        {
        public:
            virtual ~ClientRequests() = default;

            // Target Management Methods
            virtual AzFramework::TargetContainer EnumerateAvailableNetworkTargets() { return AzFramework::TargetContainer(); }

            virtual bool HasValidConnection() const { return false; }
            virtual bool IsConnected(const AzFramework::TargetInfo&) const { return false; }
            virtual bool IsConnectedToSelf() const { return false; }
            virtual AzFramework::TargetInfo GetNetworkTarget() { return AzFramework::TargetInfo(); }

            // Control Methods
            virtual void AddBreakpoint(const Breakpoint&) {}
            virtual void AddVariableChangeBreakpoint(const VariableChangeBreakpoint&) {}
            virtual void Break() {} // break on next execution signal of any kind
            virtual void Continue() {} // turns OFF unspecified data changes
            virtual void RemoveBreakpoint(const Breakpoint&) {}
            virtual void RemoveVariableChangeBreakpoint(const VariableChangeBreakpoint&) {}

            virtual void SetVariableValue() {}
            // virtual void StepInto() {}
            // virtual void StepOut() {}
            virtual void StepOver() {}

            // Data Requests
            virtual void GetAvailableScriptTargets() {}
            virtual void GetActiveEntities() {}
            virtual void GetActiveGraphs() {}
            virtual void GetVariableValue( /*need a variable argument here*/ ) {}
        };

        using ClientRequestsBus = AZ::EBus<ClientRequests>;

        class ClientUIRequests : public AZ::EBusTraits
        {
        public:

            virtual void StartEditorSession() = 0;
            virtual void StopEditorSession() = 0;

            virtual void StartLogging(ScriptTarget& initialTargets) = 0;
            virtual void StopLogging() = 0;

            virtual void AddEntityLoggingTarget(const AZ::EntityId& entityId, const ScriptCanvas::GraphIdentifier& graphIdentifier) = 0;
            virtual void RemoveEntityLoggingTarget(const AZ::EntityId& entityId, const ScriptCanvas::GraphIdentifier& graphIdentifier) = 0;

            virtual void AddGraphLoggingTarget(const AZ::Data::AssetId& assetId) = 0;
            virtual void RemoveGraphLoggingTarget(const AZ::Data::AssetId& assetId) = 0;
        };

        using ClientUIRequestBus = AZ::EBus<ClientUIRequests>;

        class ClientUINotifications : public AZ::EBusTraits
        {
        public:

            virtual void OnCurrentTargetChanged() {}
        };

        using ClientUINotificationBus = AZ::EBus<ClientUINotifications>;
    }
}
