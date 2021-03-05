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

#include "ScriptEventBase.h"

#include <AzCore/std/parallel/mutex.h>

#include <ScriptCanvas/CodeGen/CodeGen.h>
#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptEvents/ScriptEventsBus.h>

#include <Include/ScriptCanvas/Libraries/Core/SendScriptEvent.generated.h>

namespace AZ
{
    class BehaviorMethod;
}

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            using Namespaces = AZStd::vector<AZStd::string>;

            class SendScriptEvent
                : public Internal::ScriptEventBase
                , ScriptEvents::ScriptEventNotificationBus::Handler
            {
            public:

                ScriptCanvas_Node(SendScriptEvent,
                    ScriptCanvas_Node::Name("Send Script Event", "Allows you to send an event.")
                    ScriptCanvas_Node::Uuid("{64A97CC3-2BEA-4B47-809B-6C7DA34FD00F}")
                    ScriptCanvas_Node::Icon("Editor/Icons/ScriptCanvas/Bus.png")
                    ScriptCanvas_Node::Version(4)
                    ScriptCanvas_Node::DynamicSlotOrdering(true)
                    ScriptCanvas_Node::EditAttributes(AZ::Script::Attributes::ExcludeFrom(AZ::Script::Attributes::ExcludeFlags::All))
                );

                ScriptCanvas_SerializeProperty(Namespaces, m_namespaces);
                ScriptCanvas_SerializeProperty(ScriptCanvas::EBusBusId, m_busId);
                ScriptCanvas_SerializeProperty(ScriptCanvas::EBusEventId, m_eventId);

                ~SendScriptEvent() override;

                ScriptCanvas_In(ScriptCanvas_In::Name("In", "Fires the specified ScriptEvent when signaled"));
                ScriptCanvas_Out(ScriptCanvas_Out::Name("Out", "Trigged after the ScriptEvent has been signaled and returns"));

                const AZStd::string& GetEventName() const { return m_eventName; }

                bool HasBusID() const { return (m_method == nullptr) ? false : m_method->HasBusId(); }
                bool HasResult() const { return (m_method == nullptr) ? false : m_method->HasResult(); }

                ScriptCanvas::EBusBusId GetBusId() const;
                ScriptCanvas::EBusEventId GetEventId() const;

                SlotId GetBusSlotId() const;

                void ConfigureNode(const AZ::Data::AssetId& assetId, const ScriptCanvas::EBusEventId& eventId);

                // NodeVersioning
                bool IsOutOfDate() const override;
                UpdateResult OnUpdateNode() override;
                AZStd::string GetUpdateString() const override;
                ////

            protected:

                void BuildNode(const AZ::Data::AssetId& assetId, const ScriptCanvas::EBusEventId& eventId, SlotIdMapping& populationMapping);
                void InitializeResultSlotId();

                void OnScriptEventReady(const AZ::Data::Asset<ScriptEvents::ScriptEventsAsset>&) override;
                bool CreateSender(AZ::Data::Asset<ScriptEvents::ScriptEventsAsset>);

                void OnDeactivate() override;

                bool IsConfigured() const { return m_method != nullptr; }
                void ConfigureMethod(AZ::BehaviorMethod& method);
                bool RegisterScriptEvent(AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> asset);

                bool FindEvent(AZ::BehaviorMethod*& outMethod, const Namespaces& namespaces, AZStd::string_view methodName);
                void OnInputSignal(const SlotId&) override;

                void AddInputSlot(size_t slotIndex, size_t argIndex, const AZStd::string_view argName, const AZStd::string_view tooltip, AZ::BehaviorMethod* method, const AZ::BehaviorParameter* argument, AZ::Uuid slotKey, SlotIdMapping& populationMapping);

                void OnRegistered(const ScriptEvents::ScriptEvent&) override;

                SlotId m_resultSlotID;
                AZ::BehaviorMethod* m_method = nullptr;
                AZStd::recursive_mutex m_mutex;

                AZStd::string m_eventName;
                AZStd::string m_busName;

                bool m_ignoreReadyEvent = false;
            };
        }
    }
}
