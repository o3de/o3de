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
#ifndef CRYINCLUE_CRYSCRIPTSYSTEM_NETBINDING_H
#define CRYINCLUE_CRYSCRIPTSYSTEM_NETBINDING_H

#include "../NetworkGridmateMarshaling.h"
#include "../Compatibility/GridMateRMI.h"
#include "../Compatibility/GridMateNetSerialize.h"
#include "../Compatibility/GridMateNetSerializeAspectProfiles.h"
#include "Serialization/NetScriptSerialize.h"

#include "EntityReplicaSpawnParams.h"

namespace GridMate
{
    typedef DataSet<NetSerialize::AspectSerializeState, NetSerialize::AspectSerializeState::Marshaler> EntityScriptDataSetType;

    class EntityScriptDataSet
        : public EntityScriptDataSetType
    {
    private:
        static const char* GetDataSetName();
    public:
        GM_CLASS_ALLOCATOR(EntityScriptDataSet);

        EntityScriptDataSet();

        void DispatchChangedEvent(const TimeContext& tc) override;

        void SetIsEnabled(bool isEnabled);
        bool IsEnabled() const;

        GridMate::PrepareDataResult PrepareData(GridMate::EndianType endianType, AZ::u32 marshalFlags);
        void SetDirty();

    private:

        bool m_isEnabled;
    };

    class EntityScriptReplicaChunk
        : public GridMate::ReplicaChunk
        , public Serialization::INetScriptMarshaler
    {
    public:
        static const int k_maxScriptableDataSets = GM_MAX_DATASETS_IN_CHUNK;
    private:

        friend class EntityScriptDataSet;
        friend class EntityReplica;

        typedef AZStd::unordered_map<AZStd::string, int> DataSetIndexMapping;

    public:
        GM_CLASS_ALLOCATOR(EntityScriptReplicaChunk);

        EntityScriptReplicaChunk();
        ~EntityScriptReplicaChunk() = default;

        //////////////////////////////////////////////////////////////////////
        //! GridMate::ReplicaChunk overrides.
        static const char* GetChunkName() { return "EntityScriptReplicaChunk"; }
        void UpdateChunk(const GridMate::ReplicaContext& rc) override { (void)rc; }
        void OnReplicaActivate(const GridMate::ReplicaContext& rc) override;
        void OnReplicaDeactivate(const GridMate::ReplicaContext& rc) override { (void)rc; }
        void UpdateFromChunk(const GridMate::ReplicaContext& rc) override { (void)rc; }
        bool IsReplicaMigratable() override { return false; }
        //////////////////////////////////////////////////////////////////////        

        TSerialize FindSerializer(const char* name) override;
        bool CommitSerializer(const char* name, TSerialize serializer) override;

        bool IsMarshaling() const;

        int GetMaxServerProperties() const override { return k_maxScriptableDataSets; }

    protected:

        AZ::u32 CalculateDirtyDataSetMask(MarshalContext& marshalContext);
        
    private:        

        void Synchronize();
        void OnPropertyUpdate(EntityScriptDataSet& dataSet);

        void SetLocalEntityId(EntityId localEntityId);

        EntityScriptDataSet* FindDataSet(const AZStd::string& valueName);
        void EnsureMapping(const AZStd::string& valueName, EntityScriptDataSet& dataSet);        
        
        EntityScriptDataSet m_scriptDataSets[k_maxScriptableDataSets];

        DataSetIndexMapping m_nameToIndex;

        AZStd::string m_serializationTarget;

        GridMate::WriteBufferDynamic m_masterDataSetScratchBuffer;
        NetSerialize::EntityNetSerializerCollectState m_serializerImpl;
        CSimpleSerialize<NetSerialize::EntityNetSerializerCollectState> m_masterWriteSerializer;

        EntityId m_localEntityId;
        AZ::u32 m_enabledDataSetMask;
    };
}

#endif
