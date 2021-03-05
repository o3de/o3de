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

#ifndef AZFRAMEWORK_NET_BINDING_COMPONENT_H
#define AZFRAMEWORK_NET_BINDING_COMPONENT_H

#include <AzCore/Component/Component.h>
#include <AzFramework/Network/NetBindingHandlerBus.h>

namespace AzFramework
{
    /**
    * NetBindingComponent enables network synchronization for the entity.
    * It works in conjunction with NetBindingComponentChunk and NetBindingSystemComponent
    * to perform network binding and notifies other components on the entity to bind
    * their ReplicaChunks via the NetBindable interface.
    *
    * Entities bound to proxy replicas will be automatically destroyed when they are
    * unbound from the network.
    */
    class NetBindingComponent
        : public AZ::Component
        , public NetBindingHandlerBus::Handler
    {
        friend class NetBindingComponentChunk;

    public:
        AZ_COMPONENT(NetBindingComponent, "{E9CA5D63-ED2D-4B59-B3C4-EBCD4A0013E4}", NetBindingHandlerInterface);

        NetBindingComponent();

    protected:

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("ReplicaChunkService", 0xf86b88a8));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("ReplicaChunkService", 0xf86b88a8));
        }

        ///////////////////////////////////////////////////////////////////////
        // AZ::Component
        static void Reflect(AZ::ReflectContext* reflection);
        void Activate() override;
        void Deactivate() override;
        ///////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////
        // NetBindingHandlerBus::Handler
        void BindToNetwork(GridMate::ReplicaPtr bindTo) override;
        void UnbindFromNetwork() override;
        bool IsEntityBoundToNetwork() override;
        bool IsEntityAuthoritative() override;
        void MarkAsLevelSliceEntity() override;
        void SetSliceInstanceId(const AZ::SliceComponent::SliceInstanceId& sliceInstanceId) override;
        void RequestEntityChangeOwnership(GridMate::PeerId peerId = GridMate::InvalidReplicaPeerId) override;

        void SetReplicaPriority(GridMate::ReplicaPriority replicaPriority) override;
        GridMate::ReplicaPriority GetReplicaPriority() const override;
        ///////////////////////////////////////////////////////////////////////

        //! Returns if the entity belongs to the level slice for binding purposes.
        bool IsLevelSliceEntity() const;

        //! Points to the NetBindingComponentChunk counterpart.
        GridMate::ReplicaChunkPtr m_chunk;
        bool m_isLevelSliceEntity;
        AZ::SliceComponent::SliceInstanceId m_sliceInstanceId;
    };
}   // namespace AzFramework

#endif // AZFRAMEWORK_NET_BINDING_COMPONENT_H
#pragma once
