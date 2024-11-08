/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <qpoint.h>

#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Math/Vector2.h>

#include <GraphCanvas/Types/Endpoint.h>
#include <GraphCanvas/Types/Types.h>

class QGraphicsLayoutItem;

namespace AZ
{
    class any;
}

namespace GraphCanvas
{
    //! The type of connections that the slot specifies
    //! Currently we only support input/output for creating connections.
    enum ConnectionType
    {
        CT_None = 0,    //!< Generally used by EBus handlers to indicate when a slot accepts no input
        CT_Input,       //!< Indicates an input slot
        CT_Output,      //!< Indicates an output slot

        CT_Invalid = -1 //!< Generally used as the default value, to detect when something didn't respond an EBus message
    };

    typedef AZ::Crc32 SlotType;

    namespace SlotTypes
    {
        static const SlotType Invalid = AZ::Crc32();

        static const SlotType DataSlot = AZ_CRC_CE("SlotType_Data");

        static const SlotType ExecutionSlot = AZ_CRC_CE("SlotType_Execution");

        static const SlotType ExtenderSlot = AZ_CRC_CE("SlotType_Extender");

        static const SlotType PropertySlot = AZ_CRC_CE("SlotType_Property");
    }

    // Visual Identification of how the Slot should be grouped for display
    typedef AZ::Crc32 SlotGroup;

    namespace SlotGroups
    {
        static const SlotGroup Invalid = AZ::Crc32();

        // Slot Group used by default for Data Slots
        static const SlotGroup DataGroup = AZ_CRC_CE("SlotGroup_Data");

        // Slot Group used by default for Execution Slots
        static const SlotGroup ExecutionGroup = AZ_CRC_CE("SlotGroup_Execution");

        // Slot Group used by default for Extender Slots
        static const SlotGroup ExtenderGroup = AZ_CRC_CE("SlotGroup_Extender");

        // Slot Group used by default for Property Slots
        static const SlotGroup PropertyGroup = AZ_CRC_CE("SlotGroup_Property");

        // Slot Group used by default for Variable Slots
        static const SlotGroup VariableReferenceGroup = AZ_CRC_CE("SlotGroup_VariableReference");

        // Slot Group used by default for Variable Slots
        static const SlotGroup VariableSourceGroup = AZ_CRC_CE("SlotGroup_VariableSource");
    }

    struct SlotConfiguration
    {
    public:
        AZ_RTTI(SlotConfiguration, "{E080FC05-EEB6-47A6-B939-F62A45C2B1D2}");
        AZ_CLASS_ALLOCATOR(SlotConfiguration, AZ::SystemAllocator);

        virtual ~SlotConfiguration() = default;

        ConnectionType m_connectionType = ConnectionType::CT_Invalid;

        AZStd::string m_tooltip;
        AZStd::string m_name;

        bool m_isNameHidden = false;

        SlotGroup m_slotGroup = SlotGroups::Invalid;

        AZStd::string m_textDecoration;
        AZStd::string m_textDecorationToolTip;
    };

    struct ExecutionSlotConfiguration
        : public SlotConfiguration
    {
    public:
        AZ_RTTI(ExecutionSlotConfiguration, "{1129A6DE-CF46-4E87-947F-D2EB432EEA2E}", SlotConfiguration);
        AZ_CLASS_ALLOCATOR(ExecutionSlotConfiguration, AZ::SystemAllocator);

        ExecutionSlotConfiguration() = default;

        ExecutionSlotConfiguration(const SlotConfiguration& slotConfiguration)
            : SlotConfiguration(slotConfiguration)
        {
        }
    };

    struct SlotGroupConfiguration
    {
    public:
        AZ_TYPE_INFO(SlotGroupConfiguration, "{88F7AB93-9F26-4059-BD37-FFBD41E38AF6}");
        AZ_CLASS_ALLOCATOR(SlotGroupConfiguration, AZ::SystemAllocator);

        struct ExtendabilityConfig
        {
            bool          m_isExtendable;
            AZStd::string m_name;
            AZStd::string m_tooltip;
        };

        SlotGroupConfiguration()
            : m_layoutOrder(0)
        {
        }

        SlotGroupConfiguration(int layoutOrder)
            : m_layoutOrder(layoutOrder)
        {
        }

        void SetInputExtendable(const ExtendabilityConfig& configuration)
        {
            m_extendability[ConnectionType::CT_Input] = configuration;
        }

        void SetOutputExtendable(const ExtendabilityConfig& configuration)
        {
            m_extendability[ConnectionType::CT_Output] = configuration;
        }

        int m_layoutOrder = 0;
        bool m_visible = true;

        AZStd::unordered_map< ConnectionType, ExtendabilityConfig > m_extendability;
    };

    typedef AZStd::unordered_map< SlotGroup, SlotGroupConfiguration > SlotGroupConfigurationMap;

    class SlotGroupConfigurationComparator
    {
    public:
        SlotGroupConfigurationComparator()
            : m_slotConfigurationMap(nullptr)
        {
            AZ_Assert(false, "GraphCanvas: Invalid Slot Group Configuration Comparator");
        }

        explicit SlotGroupConfigurationComparator(const SlotGroupConfigurationMap* slotConfigurationMap)
            : m_slotConfigurationMap(slotConfigurationMap)
        {
        }

        bool operator()(const SlotGroup& a, const SlotGroup& b)
        {
            const SlotGroupConfiguration& configA = m_slotConfigurationMap->at(a);
            const SlotGroupConfiguration& configB = m_slotConfigurationMap->at(b);

            if (configA.m_layoutOrder == configB.m_layoutOrder)
            {
                return a < b;
            }
            else
            {
                return configA.m_layoutOrder < configB.m_layoutOrder;
            }
        }

    private:
        const SlotGroupConfigurationMap* m_slotConfigurationMap;
    };

    static const AZ::Crc32 kSlotServiceProviderId = AZ_CRC_CE("GraphCanvas_SlotService");

    //! SlotRequests
    //! Requests to retrieve or modify the current state of a slot.
    class SlotRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        //! Get the entity ID of the node that owns this slot, if any.
        virtual const AZ::EntityId& GetNode() const = 0;

        //! Set the entity ID of the node that owns this slot, if any.
        virtual void SetNode(const AZ::EntityId&) = 0;

        //! Returns the endpoint represented by this SlotId
        virtual Endpoint GetEndpoint() const = 0;

        //! Get the name, or label, of the slot.
        //! These generally appear as a label against \ref Input or \ref Output slots.
        virtual const AZStd::string GetName() const = 0;

        //! Set the slot's name.
        virtual void SetName(const AZStd::string&) = 0;

        //! Set the slot's name & tooltip.
        virtual void SetDetails(const AZStd::string& name, const AZStd::string& tooltip) = 0;

        //! Get the tooltip for the slot.
        virtual const AZStd::string GetTooltip() const = 0;

        //! Set the tooltip this slot should display.
        virtual void SetTooltip(const AZStd::string&) = 0;

        //! Get the group of the slot
        virtual SlotGroup GetSlotGroup() const = 0;

        //! Get the type of the slot.
        virtual SlotType GetSlotType() const = 0;

        //! Get the connection type of the slot is in.
        virtual ConnectionType GetConnectionType() const = 0;

        // Used by the layout to set the ordering for the slot after it's been display. Will not be respected during the layout phase.
        virtual void SetDisplayOrdering(int ordering) = 0;

        //! Returns the ordering index of the slot within it's given group.
        virtual int GetDisplayOrdering() const = 0;

        virtual bool IsConnectedTo(const Endpoint& endpoint) const = 0;

        virtual void FindConnectionsForEndpoints(const AZStd::unordered_set<Endpoint>& searchEndpoints, AZStd::unordered_set<ConnectionId>& connectedEndpoints) = 0;

        virtual bool CanDisplayConnectionTo(const Endpoint& endpoint) const = 0;
        virtual bool CanCreateConnectionTo(const Endpoint& endpoint) const = 0;

        //! Returns the connection to be used when trying to create a connection from this object.
        //! Will create a connection with the underlying data model.
        virtual AZ::EntityId CreateConnectionWithEndpoint(const Endpoint& endpoint) = 0;

        //! Returns the connection to be used when trying to create a connection from this object.
        virtual AZ::EntityId DisplayConnection() = 0;

        //! Returns the connection to be used when trying to create a connection from this object.
        //! Will not create a connection with the underlying data model.
        virtual AZ::EntityId DisplayConnectionWithEndpoint(const Endpoint& endpoint) = 0;

        //! Displays the proposed connection on the slot
        virtual void DisplayProposedConnection(const AZ::EntityId& connectionId, const Endpoint& endpoint) = 0;

        //! Restores the connection display to the previous state.
        virtual void RemoveProposedConnection(const AZ::EntityId& connectionId, const Endpoint& endpoint) = 0;

        //! Adds the given connection to the slot
        virtual void AddConnectionId(const AZ::EntityId& connectionId, const Endpoint& endpoint) = 0;

        //! Remove the specified connection from the slot.
        virtual void RemoveConnectionId(const AZ::EntityId& connectionId, const Endpoint& endpoint) = 0;

        //! Gets the UserData on the slot.
        virtual AZStd::any* GetUserData() = 0;

        //! Returns whether or not the slot has any connections
        virtual bool HasConnections() const = 0;

        //! Returns the last connection connected to the slot
        //! Returns an invalid EntityId if the slot has no connections
        virtual AZ::EntityId GetLastConnection() const = 0;

        //! Returns the list of connections connected to this slot
        virtual AZStd::vector<AZ::EntityId> GetConnections() const = 0;

        //! Sets the specified display state onto all of the connected connections.
        virtual void SetConnectionDisplayState(RootGraphicsItemDisplayState displayState) = 0;

        //! Sets the specified display state onto all of the connected connections.
        virtual void ReleaseConnectionDisplayState() = 0;

        //! Clears all of the connections currently attached to this slot.
        virtual void ClearConnections() = 0;

        // Returns the slot configuration for the slot.
        virtual const SlotConfiguration& GetSlotConfiguration() const = 0;

        // Clones the configurations in use by a slot
        virtual SlotConfiguration* CloneSlotConfiguration() const = 0;

        // Mainly used by Grouping.
        // As a way of remapping the virtual slots that are created down to the correct
        // underlying model

        // Adds a Endpoint that this connection wants to remap to for use with the underlying model
        virtual void RemapSlotForModel(const Endpoint& endpoint) = 0;

        // Signals whether or not the Endpoint needs to be remapped for the model
        virtual bool HasModelRemapping() const = 0;

        // Returns the list of slot remapping
        virtual AZStd::vector< Endpoint > GetRemappedModelEndpoints() const = 0;

        // Returns the layout priority of the slot. Higher priority means higher up on the list.
        virtual int GetLayoutPriority() const {
            return 10;
        }

        virtual void SetLayoutPriority(int layoutPriority) = 0;
    };

    using SlotRequestBus = AZ::EBus<SlotRequests>;
    
    struct SlotLayoutInfo
    {
        SlotLayoutInfo(SlotId slotId)
            : m_slotId(slotId)
        {
            SlotRequestBus::EventResult(m_priority, slotId, &SlotRequests::GetLayoutPriority);
        }

        SlotId m_slotId;
        int m_priority;
    };

    class SlotUIRequests
        : public AZ::EBusTraits
    {
    public:
        //! BusId is the entity id of the slot object.
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = SlotId;

        virtual QPointF GetPinCenter() const = 0;
        virtual QPointF GetConnectionPoint() const = 0;
        virtual QPointF GetJutDirection() const = 0;
    };

    using SlotUIRequestBus = AZ::EBus<SlotUIRequests>;

    class SlotUINotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual void OnSlotLayoutPriorityChanged(int layoutPriority) = 0;
    };

    using SlotUINotificationBus = AZ::EBus<SlotUINotifications>;

    //! SlotNotifications
    //! Notifications that indicate changes to a slot's state.
    class SlotNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = SlotId;

        //! When the name of the slot changes, the new name is signaled.
        virtual void OnNameChanged(const AZStd::string&) {}

        //! When the tooltip of the slot changes, the new tooltip value is emitted.
        virtual void OnTooltipChanged(const AZStd::string&) {}

        virtual void OnRegisteredToNode(const AZ::EntityId&) {}
        
        //! When the slot configuration changes, then this event is signaled.
        virtual void OnSlotConfigChanged() {}

        //! When the slot becomes an end of a new connection, it provides a notification of the connection and the
        //! other slot, in that order.
        virtual void OnConnectedTo(const AZ::EntityId& connectionId, const Endpoint& endpoint) { (void)connectionId; (void)endpoint; }

        //! When the slot ceases to be an end of a connection, it provides a notification of the connection and the
        //! other slot, in that order.
        virtual void OnDisconnectedFrom(const AZ::EntityId& connectionId, const Endpoint& endpoint) { (void)connectionId; (void)endpoint; }
    };

    using SlotNotificationBus = AZ::EBus<SlotNotifications>;

    class SlotLayoutRequests
        : public AZ::EBusTraits
    {
    public:
        // Id here is the ID of the node that contains the SlotLayout.
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual void SetDividersEnabled(bool enabled) = 0;
        virtual void ConfigureSlotGroup(SlotGroup group, SlotGroupConfiguration configuration) = 0;
        virtual int GetSlotGroupDisplayOrder(SlotGroup group) const = 0;

        virtual bool IsSlotGroupVisible(SlotGroup group) const = 0;
        virtual void SetSlotGroupVisible(SlotGroup group, bool visible) = 0;
        virtual void ClearSlotGroup(SlotGroup group) = 0;
    };

    using SlotLayoutRequestBus = AZ::EBus<SlotLayoutRequests>;
}

DECLARE_EBUS_EXTERN(GraphCanvas::SlotRequests);
