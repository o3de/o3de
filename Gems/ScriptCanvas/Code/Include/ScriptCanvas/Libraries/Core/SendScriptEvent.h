/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "ScriptEventBase.h"

#include <AzCore/std/parallel/mutex.h>

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
            //! Provides a node to send a Script Event
            class SendScriptEvent
                : public Internal::ScriptEventBase
                , ScriptEvents::ScriptEventNotificationBus::Handler
            {
            public:

                SCRIPTCANVAS_NODE(SendScriptEvent);

                NamespacePath m_namespaces;
                ScriptCanvas::EBusBusId m_busId;
                ScriptCanvas::EBusEventId m_eventId;

                ~SendScriptEvent() override;

                const AZStd::string& GetEventName() const { return m_eventName; }

                bool HasBusID() const { return (m_method == nullptr) ? false : m_method->HasBusId(); }
                bool HasResult() const { return (m_method == nullptr) ? false : m_method->HasResult(); }

                ScriptCanvas::EBusBusId GetBusId() const;
                ScriptCanvas::EBusEventId GetEventId() const;

                SlotId GetBusSlotId() const;

                void ConfigureNode(const AZ::Data::AssetId& assetId, const ScriptCanvas::EBusEventId& eventId);

                // NodeVersioning...
                bool IsOutOfDate(const VersionData& graphVersion) const override;
                UpdateResult OnUpdateNode() override;
                AZStd::string GetUpdateString() const override;
                ////

                AZ::Outcome<Grammar::LexicalScope, void> GetFunctionCallLexicalScope(const Slot* /*slot*/) const override;
                AZ::Outcome<AZStd::string, void> GetFunctionCallName(const Slot* /*slot*/) const override;
                EventType GetFunctionEventType(const Slot* /*slot*/) const override;

            protected:

                bool FindEvent(AZ::BehaviorMethod*& outMethod, const NamespacePath& namespaces, AZStd::string_view eventName);
                void BuildNode(const AZ::Data::AssetId& assetId, const ScriptCanvas::EBusEventId& eventId, SlotIdMapping& populationMapping);
                void InitializeResultSlotId();

                void OnScriptEventReady(const AZ::Data::Asset<ScriptEvents::ScriptEventsAsset>&) override;
                bool CreateSender(AZ::Data::Asset<ScriptEvents::ScriptEventsAsset>);

                void OnDeactivate() override;

                bool IsConfigured() const { return m_method != nullptr; }
                void ConfigureMethod(AZ::BehaviorMethod& method);
                bool RegisterScriptEvent(AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> asset);

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
