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
#include "CryNetwork_precompiled.h"

#include "EntityScriptReplicaChunk.h"
#include "EntityReplica.h"
#include "../NetworkGridMateSystemEvents.h"
#include "../NetworkGridmateMarshaling.h"
#include "../Compatibility/GridMateNetSerialize.h"
#include "../Compatibility/GridMateNetSerializeAspectProfiles.h"

namespace GridMate
{
    ////////////////////////
    // EntityScriptDataSet
    ////////////////////////
    const char* EntityScriptDataSet::GetDataSetName()
    {
        static int          s_chunkIndex = 0;
        static const char*  s_nameArray[] = {
            "DataSet1","DataSet2","DataSet3","DataSet4","DataSet5",
            "DataSet6","DataSet7","DataSet8","DataSet9","DataSet10",
            "DataSet11","DataSet12","DataSet13","DataSet14","DataSet15",
            "DataSet16","DataSet17","DataSet18","DataSet19","DataSet20",
            "DataSet21","DataSet22","DataSet23","DataSet24","DataSet25",
            "DataSet26","DataSet27","DataSet28","DataSet29","DataSet30",
            "DataSet31","DataSet32"
        };

        static_assert(EntityScriptReplicaChunk::k_maxScriptableDataSets <= AZ_ARRAY_SIZE(s_nameArray),"Insufficient number of names supplied to EntityScriptDataSet::GetDataSetName()");

        if (s_chunkIndex > EntityScriptReplicaChunk::k_maxScriptableDataSets && EntityScriptReplicaChunk::k_maxScriptableDataSets >= 0)
        {
            s_chunkIndex = s_chunkIndex%EntityScriptReplicaChunk::k_maxScriptableDataSets;
        }

        return s_nameArray[s_chunkIndex++];
    }

    EntityScriptDataSet::EntityScriptDataSet()
        : EntityScriptDataSetType(GetDataSetName())
        , m_isEnabled(false)
    {
        /*
         * ReplicaChunk has to declare all of its dataset at compile, however, scripts decide that at runtime.
         * Thus, we create the maximum possible datasets and disable those that aren't being used.
         *
         * So a lot of time these datasets aren't used in scripts and thus they don't have any useful value in them.
         * Marking them as default achieves not sending them on the network.
         */
        MarkAsDefaultValue();
    }

    void EntityScriptDataSet::DispatchChangedEvent([[maybe_unused]] const TimeContext& tc)
    {
        SetIsEnabled(true);
        static_cast<EntityScriptReplicaChunk*>(m_replicaChunk)->OnPropertyUpdate((*this));
    }

    void EntityScriptDataSet::SetIsEnabled(bool isEnabled)
    {
        m_isEnabled = isEnabled;
    }

    bool EntityScriptDataSet::IsEnabled() const
    {
        return m_isEnabled;
    }

    GridMate::PrepareDataResult EntityScriptDataSet::PrepareData(GridMate::EndianType endianType, AZ::u32 marshalFlags)
    {
        if (!IsEnabled())
        {
            return PrepareDataResult(false, false, false, false);
        }

        return EntityScriptDataSetType::PrepareData(endianType, marshalFlags);
    }

    void EntityScriptDataSet::SetDirty()
    {
        if (!IsEnabled())
        {
            return;
        }

        EntityScriptDataSetType::SetDirty();
    }

    /////////////////////////////
    // EntityScriptReplicaChunk
    /////////////////////////////
    EntityScriptReplicaChunk::EntityScriptReplicaChunk()
        : m_masterDataSetScratchBuffer(EndianType::BigEndian)
        , m_serializerImpl(m_masterDataSetScratchBuffer)
        , m_masterWriteSerializer(m_serializerImpl)
        , m_localEntityId(kInvalidEntityId)
        , m_enabledDataSetMask(0)
    {
    }

    void EntityScriptReplicaChunk::OnReplicaActivate([[maybe_unused]] const GridMate::ReplicaContext& rc)
    {
    }

    TSerialize EntityScriptReplicaChunk::FindSerializer(const char* name)
    {
        AZ_Error("EntityScriptReplicaChunk",!IsMarshaling(),"Trying to marshal two data sets at once.");
        FRAME_PROFILER("StartMarshal",GetISystem(), PROFILE_NETWORK);

        TSerialize retVal = nullptr;

        if (!IsMarshaling() && name)
        {
            m_serializationTarget = name;

            // Clear out our buffer and return the master serializer
            m_masterDataSetScratchBuffer.Clear();
            retVal = &m_masterWriteSerializer;
        }

        return retVal;
    }

    bool EntityScriptReplicaChunk::IsMarshaling() const
    {
        return !m_serializationTarget.empty();
    }

    bool EntityScriptReplicaChunk::CommitSerializer(const char* name, TSerialize serializer)
    {
        (void)name;
        (void)serializer;

        AZ_Error("EntityScriptReplicaChunk",IsMarshaling(),"Committing a serializer without finding it first.");

        bool didMarshal = false;

        if (IsMarshaling())
        {
            EntityScriptDataSet* scriptDataSet = FindDataSet(m_serializationTarget);
            m_serializationTarget.clear();

            AZ_Error("EntityScriptReplicaChunk",scriptDataSet,"Invalid SerializationTarget");
            if (scriptDataSet)
            {
                didMarshal = true;

                NetSerialize::AspectSerializeState::Marshaler& marshaler = scriptDataSet->GetMarshaler();

                if (marshaler.GetStorageSize() < m_masterDataSetScratchBuffer.Size())
                {
                    marshaler.AllocateAspectSerializationBuffer(m_masterDataSetScratchBuffer.Size());
                }

                if (m_masterDataSetScratchBuffer.Size() > 0)
                {
                    FRAME_PROFILER("AspectBufferCopy",GetISystem(), PROFILE_NETWORK);
                    WriteBufferType writeBuffer = marshaler.GetWriteBuffer();
                    writeBuffer.Clear();
                    writeBuffer.WriteRaw(m_masterDataSetScratchBuffer.Get(),m_masterDataSetScratchBuffer.Size());
                }

                // Store update contents & hash. Any change willr esult in a downstream update.
                NetSerialize::AspectSerializeState updatedState = scriptDataSet->Get();

                bool changed = false;

                {
                    FRAME_PROFILER("AspectBufferHash", GetISystem(), PROFILE_NETWORK);
                    changed = updatedState.UpdateHash(NetSerialize::HashBuffer(m_masterDataSetScratchBuffer.Get(), m_masterDataSetScratchBuffer.Size()), m_masterDataSetScratchBuffer.Size());
                }

                {
                    FRAME_PROFILER("AspectUpdate", GetISystem(), PROFILE_NETWORK);
                    scriptDataSet->Set(updatedState);
                }

                if (changed)
                {
                    FRAME_PROFILER("AspectSentEvent", GetISystem(), PROFILE_NETWORK);
                    EBUS_EVENT(NetworkSystemEventBus, AspectSent, m_localEntityId, BIT(eEA_Script), m_masterDataSetScratchBuffer.Size());
                }
            }
        }

        return didMarshal;
    }

    void EntityScriptReplicaChunk::Synchronize()
    {
        // Only want to synchronize on the proxy(otherwise we might be overwriting
        // good data inside of the script table that we actually want to pull out.
        if (IsProxy())
        {
            for (int i=0; i < k_maxScriptableDataSets; ++i)
            {
                ReadBufferType rb = m_scriptDataSets[i].GetMarshaler().GetReadBuffer();

                if (rb.Get())
                {
                    m_enabledDataSetMask |= (1 << i);
                    OnPropertyUpdate(m_scriptDataSets[i]);
                }
            }
        }
    }

    void EntityScriptReplicaChunk::OnPropertyUpdate(EntityScriptDataSet& dataSet)
    {
        ReadBufferType rb = dataSet.GetMarshaler().GetReadBuffer();

        if (rb.Get())
        {
            {
                FRAME_PROFILER(Debug::GetAspectNameByBitIndex(eEA_Script),GetISystem(),PROFILE_NETWORK);
                EBUS_EVENT(NetworkSystemEventBus, AspectReceived, m_localEntityId, eEA_Script, rb.Size().GetSizeInBytesRoundUp());
            }
        }
    }

    void EntityScriptReplicaChunk::SetLocalEntityId(EntityId localEntityId)
    {
        m_localEntityId = localEntityId;

        Synchronize();
    }

    EntityScriptDataSet* EntityScriptReplicaChunk::FindDataSet(const AZStd::string& valueName)
    {
        EntityScriptDataSet* retVal = nullptr;
        DataSetIndexMapping::iterator indexIter = m_nameToIndex.find(valueName);

        if (indexIter == m_nameToIndex.end())
        {
            for (int i = 0; i < k_maxScriptableDataSets; ++i)
            {
                if (!m_scriptDataSets[i].IsEnabled())
                {
                    m_enabledDataSetMask |= (1 << i);

                    m_scriptDataSets[i].SetIsEnabled(true);
                    AZStd::pair<DataSetIndexMapping::iterator, bool> insertResult = m_nameToIndex.insert(DataSetIndexMapping::value_type(valueName,i));

                    indexIter = insertResult.first;
                    break;
                }
            }
        }

        if (indexIter != m_nameToIndex.end())
        {
            retVal = &m_scriptDataSets[indexIter->second];
        }

        return retVal;
    }

    void EntityScriptReplicaChunk::EnsureMapping(const AZStd::string& valueName, EntityScriptDataSet& dataSet)
    {
        if (!dataSet.IsEnabled())
        {
            DataSetIndexMapping::iterator nameIter = m_nameToIndex.find(valueName);

            AZ_Error("EntityScriptReplicaChunk",nameIter == m_nameToIndex.end(),"Trying to create two script values with the same name.");
            if (nameIter == m_nameToIndex.end())
            {
                for (int i=0; i < k_maxScriptableDataSets; ++i)
                {
                    if (&m_scriptDataSets[i] == &dataSet)
                    {
                        m_nameToIndex.insert(DataSetIndexMapping::value_type(valueName,i));
                        break;
                    }
                }
            }
        }

        AZ_Error("EntityScriptReplicaChunk",m_nameToIndex.find(valueName) != m_nameToIndex.end() && &m_scriptDataSets[m_nameToIndex.find(valueName)->second] == &dataSet,"Given invalid DataSet for mapping to name");
    }

    AZ::u32 EntityScriptReplicaChunk::CalculateDirtyDataSetMask(MarshalContext& marshalContext)
    {
        return (m_enabledDataSetMask & GridMate::ReplicaChunk::CalculateDirtyDataSetMask(marshalContext));
    }
}
