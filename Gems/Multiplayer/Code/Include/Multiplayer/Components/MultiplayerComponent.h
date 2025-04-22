/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzNetworking/Serialization/ISerializer.h>
#include <AzNetworking/DataStructures/FixedSizeBitsetView.h>
#include <Multiplayer/NetworkEntity/NetworkEntityHandle.h>
#include <Multiplayer/MultiplayerStats.h>
#include <Multiplayer/MultiplayerTypes.h>
#include <Multiplayer/IMultiplayer.h>

//! Macro to declare bindings for a multiplayer component inheriting from MultiplayerComponent
#define AZ_MULTIPLAYER_COMPONENT(ComponentClass, Guid, Base) \
    AZ_RTTI(ComponentClass, Guid, Base)                      \
    AZ_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(ComponentClass)   \
    AZ_COMPONENT_BASE(ComponentClass)                        \
    AZ_CLASS_ALLOCATOR(ComponentClass, AZ::ComponentAllocator)

namespace Multiplayer
{
    class NetworkEntityRpcMessage;
    class ReplicationRecord;
    class NetBindComponent;
    class MultiplayerController;

    class MultiplayerComponent
        : public AZ::Component
    {
    public:
        AZ_CLASS_ALLOCATOR(MultiplayerComponent, AZ::SystemAllocator);
        AZ_RTTI(MultiplayerComponent, "{B7F5B743-CCD3-4981-8F1A-FC2B95CE22D7}", AZ::Component);

        static void Reflect(AZ::ReflectContext* context);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        MultiplayerComponent();
        ~MultiplayerComponent() override = default;

        //! Returns the NetBindComponent responsible for network binding for this entity.
        //! @return the NetBindComponent responsible for network binding for this entity
        //! @{
        const NetBindComponent* GetNetBindComponent() const;
        NetBindComponent* GetNetBindComponent();
        //! @}

        //! Linearly searches the components attached to the entity and returns the requested component.
        //! @return the requested component, or nullptr if the component does not exist on the entity
        //! @{
        template <typename ComponentType>
        const ComponentType* FindComponent() const;
        template <typename ComponentType>
        ComponentType* FindComponent();
        //! @}

        NetEntityId GetNetEntityId() const;
        bool IsNetEntityRoleAuthority() const;
        bool IsNetEntityRoleAutonomous() const;
        bool IsNetEntityRoleServer() const;
        bool IsNetEntityRoleClient() const;
        ConstNetworkEntityHandle GetEntityHandle() const;
        NetworkEntityHandle GetEntityHandle();
        void MarkDirty();

        //! Override to run component logic when the NetworkEntity has completed network activation
        //! Invoked when the NetworkEntity is attached and has RPCs bound via m_networkActivatedHandler
        //! Requires m_networkActivatedHandler be registered via NetBindComponent::AddNetworkActivatedEventHandler 
        virtual void OnNetworkActivated(){};

        virtual void SetOwningConnectionId(AzNetworking::ConnectionId connectionId) = 0;
        virtual NetComponentId GetNetComponentId() const = 0;

        virtual bool HandleRpcMessage(AzNetworking::IConnection* invokingConnection, NetEntityRole netEntityRole, NetworkEntityRpcMessage& rpcMessage) = 0;
        virtual bool SerializeStateDeltaMessage(ReplicationRecord& replicationRecord, AzNetworking::ISerializer& serializer) = 0;
        virtual void NotifyStateDeltaChanges(ReplicationRecord& replicationRecord) = 0;
        virtual bool HasController() const = 0;
        virtual MultiplayerController* GetController() = 0;
        virtual const MultiplayerController* GetController() const = 0;

    protected:
        virtual void ConstructController() = 0;
        virtual void DestructController() = 0;
        virtual void ActivateController(EntityIsMigrating entityIsMigrating) = 0;
        virtual void DeactivateController(EntityIsMigrating entityIsMigrating) = 0;
        virtual void NetworkAttach(NetBindComponent* netBindComponent, ReplicationRecord& currentEntityRecord, ReplicationRecord& predictableEntityRecord) = 0;

        mutable NetBindComponent* m_netBindComponent = nullptr;
        AZ::Event<>::Handler m_networkActivatedHandler;

        friend class NetworkEntityHandle;
        friend class NetBindComponent;
        friend class MultiplayerController;
    };

    template <typename ComponentType>
    inline const ComponentType* MultiplayerComponent::FindComponent() const
    {
        return GetEntity()->FindComponent<ComponentType>();
    }

    template <typename ComponentType>
    inline ComponentType* MultiplayerComponent::FindComponent()
    {
        return GetEntity()->FindComponent<ComponentType>();
    }

    inline void UpdateComponentMetrics
    (
        bool modifyRecord,
        uint32_t prevSerializerSize,
        uint32_t currSerializerSize,
        NetComponentId componentId,
        PropertyIndex propertyIndex,
        MultiplayerStats& stats
    )
    {
        const uint32_t updateSize = (currSerializerSize - prevSerializerSize);
        if (updateSize > 0)
        {
            if (modifyRecord)
            {
                stats.RecordPropertyReceived(componentId, propertyIndex, updateSize);
            }
            else
            {
                stats.RecordPropertySent(componentId, propertyIndex, updateSize);
            }
        }
    }

    template <typename TYPE>
    inline void SerializeNetworkPropertyHelper
    (
        AzNetworking::ISerializer& serializer,
        AzNetworking::FixedSizeBitsetView& bitset,
        int32_t bitIndex,
        TYPE& value,
        const char* name,
        NetComponentId componentId,
        PropertyIndex propertyIndex,
        MultiplayerStats& stats
    )
    {
        if (bitset.GetBit(bitIndex))
        {
            const bool modifyRecord = serializer.GetSerializerMode() == AzNetworking::SerializerMode::WriteToObject;
            const uint32_t prevUpdateSize = serializer.GetSize();
            serializer.ClearTrackedChangesFlag();
            serializer.Serialize(value, name);
            if (modifyRecord && !serializer.GetTrackedChangesFlag())
            {
                // If the serializer didn't change any values, then lower the flag so we don't unnecessarily notify
                bitset.SetBit(bitIndex, false);
            }
            const uint32_t postUpdateSize = serializer.GetSize();
            UpdateComponentMetrics(modifyRecord, prevUpdateSize, postUpdateSize, componentId, propertyIndex, stats);
        }
    }

    template <typename TYPE, AZStd::size_t SIZE>
    inline void SerializeNetworkPropertyHelperArray
    (
        AzNetworking::ISerializer& serializer,
        AzNetworking::FixedSizeBitsetView& bitset,
        AZStd::array<TYPE, SIZE>& value,
        NetComponentId componentId,
        PropertyIndex propertyIndex,
        MultiplayerStats& stats
    )
    {
        const bool modifyRecord = serializer.GetSerializerMode() == AzNetworking::SerializerMode::WriteToObject;
        const uint32_t prevUpdateSize = serializer.GetSize();
        for (uint32_t i = 0; i < SIZE; ++i)
        {
            if (bitset.GetBit(i))
            {
                serializer.ClearTrackedChangesFlag();
                serializer.Serialize(value[i], AzNetworking::GenerateIndexLabel<SIZE>(i).c_str());
                if (modifyRecord && !serializer.GetTrackedChangesFlag())
                {
                    bitset.SetBit(i, false);
                }
            }
        }
        const uint32_t postUpdateSize = serializer.GetSize();
        UpdateComponentMetrics(modifyRecord, prevUpdateSize, postUpdateSize, componentId, propertyIndex, stats);
    }

    template <typename TYPE, AZStd::size_t SIZE>
    inline void SerializeNetworkPropertyHelperVector
    (
        AzNetworking::ISerializer& serializer,
        AzNetworking::FixedSizeBitsetView& bitset,
        AZStd::fixed_vector<TYPE, SIZE>& value,
        NetComponentId componentId,
        PropertyIndex propertyIndex,
        MultiplayerStats& stats
    )
    {
        const bool modifyRecord = serializer.GetSerializerMode() == AzNetworking::SerializerMode::WriteToObject;
        const uint32_t prevUpdateSize = serializer.GetSize();
        if (bitset.GetBit(SIZE))
        {
            using SizeType = typename AZ::SizeType<AZ::RequiredBytesForValue<SIZE>(), false>::Type;
            SizeType origSize = aznumeric_cast<SizeType>(value.size());
            SizeType newSize = origSize;
            serializer.Serialize(newSize, "Size");
            value.resize(newSize);
            if (modifyRecord && origSize == newSize)
            {
                bitset.SetBit(SIZE, false);
            }
        }

        for (uint32_t i = 0; i < value.size(); ++i)
        {
            if (bitset.GetBit(i))
            {
                serializer.ClearTrackedChangesFlag();
                serializer.Serialize(value[i], AzNetworking::GenerateIndexLabel<SIZE>(i).c_str());
                if (modifyRecord && !serializer.GetTrackedChangesFlag())
                {
                    bitset.SetBit(i, false);
                }
            }
        }
        const uint32_t postUpdateSize = serializer.GetSize();
        UpdateComponentMetrics(modifyRecord, prevUpdateSize, postUpdateSize, componentId, propertyIndex, stats);
    }
}
