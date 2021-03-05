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

#include <ScriptCanvas/CodeGen/CodeGen.h>

#include <Include/ScriptCanvas/Libraries/Core/EBusEventHandler.generated.h>

#include <AzCore/std/parallel/mutex.h>
#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/Graph.h>
#include <AzCore/std/containers/map.h>

#include <ScriptCanvas/Core/EBusNodeBus.h>

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
            struct EBusEventEntry
            {
                AZ_TYPE_INFO(EBusEventEntry, "{92A20C1B-A54A-4583-97DB-A894377ACE21}");

                bool IsExpectingResult() const 
                {
                    return m_resultSlotId.IsValid();
                }

                AZStd::string m_eventName;
                EBusEventId   m_eventId;
                SlotId m_eventSlotId;
                SlotId m_resultSlotId;
                AZStd::vector<SlotId> m_parameterSlotIds;
                int m_numExpectedArguments = {};
                bool m_resultEvaluated = {};

                bool m_shouldHandleEvent = false;
                bool m_isHandlingEvent = false;

                static bool EBusEventEntryVersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement);
                static void Reflect(AZ::ReflectContext* context);
            };

            class EBusEventHandler 
                : public Node
                , public EBusHandlerNodeRequestBus::Handler
            {
            public:

                static const char* c_busIdName;
                static const char* c_busIdTooltip;

                using Events = AZStd::vector<EBusEventEntry>;
                using EventMap = AZStd::map<AZ::Crc32, EBusEventEntry>;

                static bool EBusEventHandlerVersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement);

                ScriptCanvas_Node(EBusEventHandler,
                    ScriptCanvas_Node::Name("Event Handler", "Allows you to handle a event.")
                    ScriptCanvas_Node::Uuid("{33E12915-EFCA-4AA7-A188-D694DAD58980}")
                    ScriptCanvas_Node::Icon("Editor/Icons/ScriptCanvas/Bus.png")
                    ScriptCanvas_Node::EventHandler("SerializeContextEventHandlerDefault<EBusEventHandler>")
                    ScriptCanvas_Node::Version(4, EBusEventHandler::EBusEventHandlerVersionConverter)
                    ScriptCanvas_Node::GraphEntryPoint(true)
                    ScriptCanvas_Node::EditAttributes(AZ::Script::Attributes::ExcludeFrom(AZ::Script::Attributes::ExcludeFlags::All))
                );

                EBusEventHandler() 
                    : m_autoConnectToGraphOwner(true)
                {}

                ~EBusEventHandler() override;

                void OnInit() override;
                void OnActivate() override;
                void OnPostActivate() override;
                void OnDeactivate() override;

                void OnGraphSet() override;
                
                void CollectVariableReferences(AZStd::unordered_set< ScriptCanvas::VariableId >& variableIds) const override;
                bool ContainsReferencesToVariables(const AZStd::unordered_set< ScriptCanvas::VariableId >& variableIds) const override;

                // EBusHandlerNodeRequestBus
                void SetAddressId(const Datum& datumValue) override;
                ////

                void InitializeBus(const AZStd::string& ebusName);

                void InitializeEvent(int eventIndex);
                
                bool CreateHandler(AZStd::string_view ebusName);

                void Connect();
                
                void Disconnect();

                const EBusEventEntry* FindEvent(const AZStd::string& name) const;
                AZ_INLINE const char* GetEBusName() const { return m_ebusName.c_str(); }
                AZ_INLINE ScriptCanvas::EBusBusId GetEBusId() const { return m_busId; }
                AZ_INLINE const EventMap& GetEvents() const { return m_eventMap; }
                AZStd::vector<SlotId> GetEventSlotIds() const;
                AZStd::vector<SlotId> GetNonEventSlotIds() const;
                bool IsEventSlotId(const SlotId& slotId) const;

                bool IsEventHandler() const override;
                bool IsEventConnected(const EBusEventEntry& entry) const;
                bool IsValid() const;
                
                AZ_INLINE bool IsIDRequired() const { return m_ebus ? !m_ebus->m_idParam.m_typeId.IsNull() : false; }
                void SetAutoConnectToGraphOwner(bool enabled);

                void OnWriteEnd();

                AZStd::string GetNodeName() const override
                {
                    return GetDebugName();
                }

                AZStd::string GetDebugName() const override
                {
                    return AZStd::string::format("%s Handler", GetEBusName());
                }

                NodeTypeIdentifier GetOutputNodeType(const SlotId& slotId) const override;

            protected:
                AZ_INLINE bool IsConfigured() const { return !m_eventMap.empty(); }
                
                void OnEvent(const char* eventName, const int eventIndex, AZ::BehaviorValueParameter* result, const int numParameters, AZ::BehaviorValueParameter* parameters);

                void OnInputSignal(const SlotId&) override;
                void OnInputChanged(const Datum& input, const SlotId& slotID) override;

            private:
                EBusEventHandler(const EBusEventHandler&) = delete;
                static void OnEventGenericHook(void* userData, const char* eventName, int eventIndex, AZ::BehaviorValueParameter* result, int numParameters, AZ::BehaviorValueParameter* parameters);


                // Inputs
                ScriptCanvas_In(ScriptCanvas_In::Name("Connect", "Connect this event handler to the specified entity."));
                ScriptCanvas_In(ScriptCanvas_In::Name("Disconnect", "Disconnect this event handler."));

                // Outputs
                ScriptCanvas_Out(ScriptCanvas_Out::Name("OnConnected", "Signaled when a connection has taken place."));
                ScriptCanvas_Out(ScriptCanvas_Out::Name("OnDisconnected", "Signaled when this event handler is disconnected."));
                ScriptCanvas_Out(ScriptCanvas_Out::Name("OnFailure", "Signaled when it is not possible to connect this handler."));
                
                ScriptCanvas_SerializeProperty(EventMap, m_eventMap);
                ScriptCanvas_SerializeProperty(AZStd::string, m_ebusName);
                ScriptCanvas_SerializeProperty(ScriptCanvas::EBusBusId, m_busId);

                ScriptCanvas_SerializeProperty(bool, m_autoConnectToGraphOwner);

                AZ::BehaviorEBusHandler* m_handler = nullptr;
                AZ::BehaviorEBus* m_ebus = nullptr;
                AZStd::recursive_mutex m_mutex; // post-serialization
            };
        } // namespace Core
    } // namespace Nodes
} // namespace ScriptCanvas
