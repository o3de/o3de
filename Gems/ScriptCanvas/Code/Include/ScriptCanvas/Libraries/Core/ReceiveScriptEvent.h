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

#include <ScriptCanvas/Libraries/Core/ScriptEventBase.h>

#include <ScriptCanvas/CodeGen/CodeGen.h>
#include <Include/ScriptCanvas/Libraries/Core/ReceiveScriptEvent.generated.h>

#include <AzCore/std/containers/map.h>

#include <ScriptEvents/ScriptEventsBus.h>
#include <ScriptEvents/ScriptEventsAssetRef.h>

namespace AZ
{
    class BehaviorEBus;
    class BehaviorEBusHandler;
}

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            class ReceiveScriptEvent
                : public Internal::ScriptEventBase
            {
            public:

                ScriptCanvas_Node(ReceiveScriptEvent,
                    ScriptCanvas_Node::Name("Receive Script Event", "Allows you to handle a event.")
                    ScriptCanvas_Node::Uuid("{76CF9938-4A7E-4CDA-8DF3-77C10239D99C}")
                    ScriptCanvas_Node::Icon("Editor/Icons/ScriptCanvas/Bus.png")
                    ScriptCanvas_Node::Version(2)
                    ScriptCanvas_Node::GraphEntryPoint(true)
                    ScriptCanvas_Node::EditAttributes(AZ::Script::Attributes::ExcludeFrom(AZ::Script::Attributes::ExcludeFlags::All))
                );

                ScriptCanvas_SerializeProperty(ScriptCanvas::EBusBusId, m_busId);

                ReceiveScriptEvent();
                ~ReceiveScriptEvent() override;

                void OnActivate() override;
                void OnPostActivate() override;
                void OnDeactivate() override;

                // Inputs
                ScriptCanvas_In(ScriptCanvas_In::Name("Connect", "Connect this event handler to the specified entity."));
                ScriptCanvas_In(ScriptCanvas_In::Name("Disconnect", "Disconnect this event handler."));

                // Outputs
                ScriptCanvas_Out(ScriptCanvas_Out::Name("OnConnected", "Signaled when a connection has taken place."));
                ScriptCanvas_Out(ScriptCanvas_Out::Name("OnDisconnected", "Signaled when this event handler is disconnected."));
                ScriptCanvas_Out(ScriptCanvas_Out::Name("OnFailure", "Signaled when it is not possible to connect this handler."));

                const AZ::Data::AssetId GetAssetId() const { return m_scriptEventAssetId; }
                ScriptCanvas::EBusBusId GetBusId() const { return ScriptCanvas::EBusBusId(GetAssetId().ToString<AZStd::string>().c_str()); }

                AZStd::vector<SlotId> GetEventSlotIds() const;
                AZStd::vector<SlotId> GetNonEventSlotIds() const;
                bool IsEventSlotId(const SlotId& slotId) const;

                bool IsIDRequired() const;

                // NodeVersioning
                bool IsOutOfDate() const override;
                UpdateResult OnUpdateNode() override;
                AZStd::string GetUpdateString() const override;
                ////

                using Events = AZStd::vector<Internal::ScriptEventEntry>;

                void SetAutoConnectToGraphOwner(bool enabled);

            protected:

                void OnScriptEventReady(const AZ::Data::Asset<ScriptEvents::ScriptEventsAsset>&) override;

            private:

                void Connect();
                void Disconnect(bool queueDisconnect = true);
                void CompleteDisconnection();

                bool CreateEbus();
                bool SetupHandler();

                void OnInputSignal(const SlotId& slotId) override;
                void OnInputChanged(const Datum& input, const SlotId& slotId) override;
                
                AZ::BehaviorEBusHandler* m_handler = nullptr;
                AZ::BehaviorEBus* m_ebus = nullptr;
                AZStd::recursive_mutex m_mutex; // post-serialization

                bool IsConfigured() const { return !m_eventMap.empty(); }

                void InitializeEvent(AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> asset, int eventIndex, SlotIdMapping& populationMapping);

                static void OnEventGenericHook(void* userData, const char* eventName, int eventIndex, AZ::BehaviorValueParameter* result, int numParameters, AZ::BehaviorValueParameter* parameters);
                void OnEvent(const char* eventName, const int eventIndex, AZ::BehaviorValueParameter* result, const int numParameters, AZ::BehaviorValueParameter* parameters);

                bool IsEventConnected(const Internal::ScriptEventEntry& entry) const;

                Internal::ScriptEventEntry ConfigureEbusEntry(const ScriptEvents::Method& methodDefinition, const AZ::BehaviorEBusHandler::BusForwarderEvent& event, SlotIdMapping& populationMapping);

                bool CreateHandler(AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> asset);
                void CompleteInitialize(AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> asset);
                void PopulateAsset(AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> asset, SlotIdMapping& populationMapping);
                bool m_eventInitComplete = false;

                static const char* c_busIdName;
                static const char* c_busIdTooltip;

                struct EventHookUserData
                {
                    ReceiveScriptEvent* m_handler;
                    const ScriptEvents::Method* m_methodDefinition;
                };

                EventHookUserData m_userData;

                ScriptCanvas_SerializePropertyWithDefaults(bool, m_autoConnectToGraphOwner, true);

                bool m_connected;

            };
        } // namespace Core
    } // namespace Nodes
} // namespace ScriptCanvas
