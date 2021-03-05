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

#ifndef AZFRAMEWORK_NET_BINDING_COMPONENT_CHUNK_H
#define AZFRAMEWORK_NET_BINDING_COMPONENT_CHUNK_H

#include <AzCore/Component/ComponentBus.h>
#include <AzFramework/Network/NetBindingSystemBus.h>
#include <GridMate/Replica/ReplicaChunk.h>
#include <GridMate/Serialize/ContainerMarshal.h>
#include <GridMate/Serialize/DataMarshal.h>
#include <GridMate/Serialize/CompressionMarshal.h>
#include <AzFramework/Network/NetBindingSystemImpl.h>

namespace AzFramework
{
    class NetBindingComponent;
    class NetBindingComponentChunkDescriptor;

    /**
    * NetBindingComponentChunk is the counterpart of NetBindingComponent on the network side.
    * It contains entity spawn data. It is created by NetBindingComponent during network
    * binding on the master and initiates entity creation and binding on the proxy side.
    */
    class NetBindingComponentChunk
        : public GridMate::ReplicaChunk
    {
        friend NetBindingComponent;
        friend NetBindingComponentChunkDescriptor;

    public:
        AZ_CLASS_ALLOCATOR(NetBindingComponentChunk, AZ::SystemAllocator, 0);

        static const char* GetChunkName() { return "NetBindingComponentChunk"; }

        NetBindingComponentChunk();

        void SetBinding(NetBindingComponent* bindingComponent) { m_bindingComponent = bindingComponent; }
        NetBindingComponent* GetBinding() const { return m_bindingComponent; }

    protected:
        ///////////////////////////////////////////////////////////////////////
        // ReplicaChunk
        bool IsReplicaMigratable() override { return true; }
        void OnReplicaActivate(const GridMate::ReplicaContext& rc) override;
        void OnReplicaDeactivate(const GridMate::ReplicaContext& rc) override;
        bool AcceptChangeOwnership(GridMate::PeerId requestor, const GridMate::ReplicaContext& rc) override;
        void OnReplicaChangeOwnership(const GridMate::ReplicaContext& rc) override;
        ///////////////////////////////////////////////////////////////////////

        NetBindingComponent* m_bindingComponent;

        class SpawnInfo
        {
        public:
            class Marshaler
            {
            public:
                void Marshal(GridMate::WriteBuffer& wb, const SpawnInfo& data);
                void Unmarshal(SpawnInfo& data, GridMate::ReadBuffer& rb);
            };

            class Throttle
            {
            public:
                //! Always return true because SpawnInfo never changes
                bool WithinThreshold(const SpawnInfo&) const { return true; }
                void UpdateBaseline(const SpawnInfo& baseline) { (void)baseline; }
            };

            SpawnInfo();

            bool operator==(const SpawnInfo& rhs);

            bool ContainsSerializedState() const;

            /**
             * \brief Same as m_staticEntityId on authoritative entity with master replica
             */
            AZ::u64 m_runtimeEntityId;
            NetBindingContextSequence m_owningContextId;
            AZStd::vector<AZ::u8> m_serializedState;

            /**
             * \brief EntityId of authoritative entity with master replica
             */
            AZ::u64 m_staticEntityId;

            AZStd::pair<AZ::Uuid, AZ::u32> m_sliceAssetId;
            /**
            * \brief uniquely identifies the slice instance that this entity is being replicated from
            */
            AZ::SliceComponent::SliceInstanceId m_sliceInstanceId;
        };

        GridMate::DataSet<SpawnInfo, SpawnInfo::Marshaler, SpawnInfo::Throttle> m_spawnInfo;
        GridMate::DataSet<AZStd::vector<AZ::ComponentId> > m_bindMap;
    };
    typedef AZStd::intrusive_ptr<NetBindingComponentChunk> NetBindingComponentChunkPtr;
}   // namespace AZ

#endif // AZFRAMEWORK_NET_BINDING_COMPONENT_CHUNK_H
#pragma once
