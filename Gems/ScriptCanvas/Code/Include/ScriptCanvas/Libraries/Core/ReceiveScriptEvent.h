/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Libraries/Core/ScriptEventBase.h>

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
            //! Provides a node to handle a Script Event
            class ReceiveScriptEvent
                : public Internal::ScriptEventBase
            {
            public:

                SCRIPTCANVAS_NODE(ReceiveScriptEvent);

                ScriptCanvas::EBusBusId m_busId;

                ReceiveScriptEvent();
                ~ReceiveScriptEvent() override;

                void OnActivate() override;
                void OnDeactivate() override;

                const AZ::Data::AssetId GetAssetId() const { return m_scriptEventAssetId; }
                ScriptCanvas::EBusBusId GetBusId() const { return ScriptCanvas::EBusBusId(GetAssetId().ToString<AZStd::string>().c_str()); }

                const Internal::ScriptEventEntry* FindEventWithSlot(const Slot& slot) const;
                AZ::Outcome<AZStd::string> GetInternalOutKey(const Slot& slot) const override;
                const Slot* GetEBusConnectSlot() const override;
                const Slot* GetEBusDisconnectSlot() const override;
                AZStd::optional<size_t> GetEventIndex(AZStd::string eventName) const override;
                AZStd::vector<SlotId> GetEventSlotIds() const override;
                AZStd::vector<SlotId> GetNonEventSlotIds() const override;

                bool IsIDRequired() const;

                bool IsEventSlotId(const SlotId& slotId) const;

                // NodeVersioning...
                bool IsOutOfDate(const VersionData& graphVersion) const override;
                UpdateResult OnUpdateNode() override;
                AZStd::string GetUpdateString() const override;
                ////

                using Events = AZStd::vector<Internal::ScriptEventEntry>;

                void SetAutoConnectToGraphOwner(bool enabled);

                AZStd::string GetEBusName() const override;
                bool IsAutoConnected() const override;
                bool IsEBusAddressed() const override;
                bool IsEventHandler() const override;
                const Datum* GetHandlerStartAddress() const override;
                const Slot* GetEBusConnectAddressSlot() const override;
                AZ::Outcome<AZStd::string, void> GetFunctionCallName(const Slot* /*slot*/) const override;
                AZStd::vector<const Slot*> GetOnVariableHandlingDataSlots() const override;
                AZStd::vector<const Slot*> GetOnVariableHandlingExecutionSlots() const override;

            protected:
                ConstSlotsOutcome GetSlotsInExecutionThreadByTypeImpl(const Slot& executionSlot, CombinedSlotType targetSlotType, const Slot* /*executionChildSlot*/) const override;

                void OnScriptEventReady(const AZ::Data::Asset<ScriptEvents::ScriptEventsAsset>&) override;

            private:

                bool CreateEbus();

                AZ::BehaviorEBusHandler* m_handler = nullptr;
                AZ::BehaviorEBus* m_ebus = nullptr;
                AZStd::recursive_mutex m_mutex; // post-serialization

                bool IsConfigured() const { return !m_eventMap.empty(); }

                void InitializeEvent(AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> asset, int eventIndex, SlotIdMapping& populationMapping);

                bool IsEventConnected(const Internal::ScriptEventEntry& entry) const;

                Internal::ScriptEventEntry ConfigureEbusEntry(const ScriptEvents::Method& methodDefinition, const AZ::BehaviorEBusHandler::BusForwarderEvent& event, SlotIdMapping& populationMapping);

                bool InitializeDefinition(AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> asset);
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

                bool m_autoConnectToGraphOwner = true;

                bool m_connected;
            };
        }
    }
}
