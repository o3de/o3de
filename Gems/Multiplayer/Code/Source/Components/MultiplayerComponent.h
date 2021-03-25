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

#include <AzCore/Component/Component.h>
#include <AzNetworking/Serialization/ISerializer.h>
#include <Source/NetworkEntity/NetworkEntityHandle.h>
#include <Source/MultiplayerTypes.h>

//! Macro to declare bindings for a multiplayer component inheriting from MultiplayerComponent
#define AZ_MULTIPLAYER_COMPONENT(ComponentClass, Guid)     \
    AZ_RTTI(ComponentClass, Guid, AZ::Component)           \
    AZ_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(ComponentClass) \
    AZ_COMPONENT_BASE(ComponentClass, Guid, Multiplayer::MultiplayerComponent)

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
        AZ_CLASS_ALLOCATOR(MultiplayerComponent, AZ::SystemAllocator, 0);
        AZ_RTTI(MultiplayerComponent, "{B7F5B743-CCD3-4981-8F1A-FC2B95CE22D7}", AZ::Component);

        static void Reflect(AZ::ReflectContext* reflection);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        MultiplayerComponent() = default;
        ~MultiplayerComponent() override = default;

        //! Returns the NetBindComponent responsible for network binding for this entity.
        //! @return the NetBindComponent responsible for network binding for this entity
        //! @{
        const NetBindComponent* GetNetBindComponent() const;
        NetBindComponent* GetNetBindComponent();
        //! @}

        //! Linearly searches the components attached to the entity and returns the requested component. .
        //! @return the requested component, or nullptr if the component does not exist on the entity
        //! @{
        template <typename ComponentType>
        const ComponentType* FindComponent() const;
        template <typename ComponentType>
        ComponentType* FindComponent();
        //! @}

        NetEntityId GetNetEntityId() const;
        ConstNetworkEntityHandle GetEntityHandle() const;
        NetworkEntityHandle GetEntityHandle();

        virtual NetComponentId GetNetComponentId() const = 0;

        virtual bool HandleRpcMessage(NetEntityRole netEntityRole, NetworkEntityRpcMessage& rpcMessage) = 0;
        virtual bool SerializeStateDeltaMessage(ReplicationRecord& replicationRecord, AzNetworking::ISerializer& serializer, ComponentSerializationType componentSerializationType) = 0;
        virtual void NotifyStateDeltaChanges(ReplicationRecord& replicationRecord, ComponentSerializationType componentSerializationType) = 0;

    protected:

        virtual bool HasController() const = 0;
        virtual MultiplayerController* GetController() = 0;

        virtual bool Migrate(AzNetworking::ISerializer& serializer) = 0;
        virtual void ConstructController() = 0;
        virtual void DestructController() = 0;
        virtual void ActivateController(EntityIsMigrating entityIsMigrating) = 0;
        virtual void DeactivateController(EntityIsMigrating entityIsMigrating) = 0;
        virtual void NetworkAttach(NetBindComponent& netBindComponent, ReplicationRecord& currentEntityRecord, ReplicationRecord& predictableEntityRecord) = 0;
        virtual void NetworkDetach() = 0;

        mutable NetBindComponent* m_netBindComponent = nullptr;

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
}
