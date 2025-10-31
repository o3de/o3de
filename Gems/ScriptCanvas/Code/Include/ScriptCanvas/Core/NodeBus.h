/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "Core.h"

#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/EBus/EBus.h>
#include <ScriptCanvas/Data/Data.h>

#include <ScriptCanvas/Core/SlotConfigurations.h>

namespace ScriptCanvas
{
    struct VariableId;
    class Datum;
    class Slot;

    class ModifiableDatumView;

    enum class NodeDisabledFlag : int
    {
        None = 0,
        User = 1 << 0,
        ErrorInUpdate = 1 << 1,

        // Appending non user flag here
        NonUser = ErrorInUpdate
    };

    class NodeRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = ID;

        virtual Slot* GetSlot(const SlotId& slotId) const = 0;
        virtual size_t GetSlotIndex(const SlotId& slotId) const = 0;

        //! Gets all of the slots on the node.
        //! Name is funky to avoid a mismatch with typing with another function
        //! that returns a better version of this information that cannot be used with
        //! EBuses because of references.
        virtual AZStd::vector<const Slot*> GetAllSlots() const  { return {}; }

        virtual AZStd::vector<Slot*> ModAllSlots() { return {}; }

        //! Retrieves a slot id that matches the supplied name
        //! There can be multiple slots with the same name on a node
        //! Therefore this should only be used when a slot's name is unique within the node
        virtual SlotId GetSlotId(AZStd::string_view slotName) const = 0;

        virtual SlotId FindSlotIdForDescriptor(AZStd::string_view slotName, const SlotDescriptor& descriptor) const = 0;

        //! Retrieves a slot id that matches the supplied name and the supplied slot type
        virtual SlotId GetSlotIdByType(AZStd::string_view slotName, CombinedSlotType slotType) const
        {
            return FindSlotIdForDescriptor(slotName, SlotDescriptor(slotType));
        }

        //! Retrieves all slot ids for slots with the specific name
        virtual AZStd::vector<SlotId> GetSlotIds(AZStd::string_view slotName) const = 0;

        virtual const ScriptCanvasId& GetOwningScriptCanvasId() const = 0;

        //! Get the Datum for the specified slot.
        virtual const Datum* FindDatum(const SlotId& slotId) const = 0;
        
        const Datum* GetInput(const SlotId& slotId) const
        {
            AZ_Warning("ScriptCanvas", false, "Using Deprecated GetInput method call. Please switch to FindDatum call instead, this method will be removed in a future update.");
            return FindDatum(slotId);
        }

        virtual bool FindModifiableDatumView(const SlotId& slotId, ModifiableDatumView& datumView) = 0;

        //! Determines whether the slot on this node with the specified slot id can accept values of the specified type
        virtual AZ::Outcome<void, AZStd::string> SlotAcceptsType(const SlotId&, const Data::Type&) const = 0;

        //! Gets the input for the given SlotId
        virtual Data::Type GetSlotDataType(const SlotId& slotId) const = 0;

        // Retrieves the variable id which is represents the current variable associated with the specified slot
        virtual VariableId GetSlotVariableId(const SlotId& slotId) const = 0;
        // Sets the variable id parameter as the current variable for the specified slot
        virtual void SetSlotVariableId(const SlotId& slotId, const VariableId& variableId) = 0;
        // Reset the variable id value to the original variable id that was associated with the slot
        // when the slot was created by a call to AddInputDatumSlot().
        // The reset variable Id is not associated Variable Manager and is owned by this node
        virtual void ClearSlotVariableId(const SlotId& slotId) = 0;

        virtual int FindSlotIndex(const SlotId& slotId) const = 0;

        virtual bool IsOnPureDataThread(const SlotId& slotId) const = 0;

        virtual AZ::Outcome<void, AZStd::string> IsValidTypeForSlot(const SlotId& slotId, const Data::Type& dataType) const = 0;
        virtual AZ::Outcome<void, AZStd::string> IsValidTypeForGroup(const AZ::Crc32& dynamicGroup, const Data::Type& dataType) const = 0;

        virtual void SignalBatchedConnectionManipulationBegin() = 0;
        virtual void SignalBatchedConnectionManipulationEnd() = 0;

        virtual void AddNodeDisabledFlag(NodeDisabledFlag disabledFlag) = 0;
        virtual void RemoveNodeDisabledFlag(NodeDisabledFlag disabledFlag) = 0;

        virtual bool IsNodeEnabled() const = 0;
        virtual bool HasNodeDisabledFlag(NodeDisabledFlag disabledFlag) const = 0;

        virtual bool RemoveVariableReferences(const AZStd::unordered_set< ScriptCanvas::VariableId >& variableIds) = 0;
    };

    using NodeRequestBus = AZ::EBus<NodeRequests>;

    class LogNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = ScriptCanvasId;

        virtual void LogMessage([[maybe_unused]] const AZStd::string& log) {}
    };
    using LogNotificationBus = AZ::EBus<LogNotifications>;
            
    class NodeNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual void OnSlotInputChanged(const SlotId& /*slotId*/) {}

        //! Events signaled when a slot is added or removed from a node
        virtual void OnSlotAdded(const SlotId& /*slotId*/) {}
        virtual void OnSlotRemoved(const SlotId& /*slotId*/) {}
        virtual void OnSlotRenamed(const SlotId& /*slotId*/, AZStd::string_view /*newName*/) {}        

        virtual void OnSlotDisplayTypeChanged(const SlotId& /*slotId*/, const ScriptCanvas::Data::Type& /*slotType*/) {}

        virtual void OnSlotActiveVariableChanged(const SlotId& /*slotId*/, [[maybe_unused]] const VariableId& oldVariableId, [[maybe_unused]] const VariableId& newVariableId) {}

        virtual void OnSlotsReordered() {}

        virtual void OnNodeDisabled() {};
        virtual void OnNodeEnabled() {};
    };

    using NodeNotificationsBus = AZ::EBus<NodeNotifications>;
}
