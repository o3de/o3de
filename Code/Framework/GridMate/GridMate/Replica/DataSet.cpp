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

#include <GridMate/Replica/Replica.h>
#include <GridMate/Replica/ReplicaChunkDescriptor.h>
#include <GridMate/Replica/ReplicaMgr.h>
#include <GridMate/Replica/DataSet.h>

namespace GridMate
{
    DataSetBase::DataSetBase(const char* debugName)
        : m_maxIdleTicks(5.f)
        , m_streamCache(EndianType::IgnoreEndian, 64)
        , m_replicaChunk(nullptr)
        , m_lastUpdateTime(0)
        , m_isDefaultValue(true)
        , m_revision(0)  //null stamp
        , m_override(nullptr)
    {
        ReplicaChunkInitContext* initContext = ReplicaChunkDescriptorTable::Get().GetCurrentReplicaChunkInitContext();
        AZ_Assert(initContext, "Replica's context was NOT pushed on the stack! Call Replica::Descriptor::Push() before construction!");
        ReplicaChunkDescriptor* descriptor = initContext->m_descriptor;
        AZ_Assert(descriptor, "Replica's descriptor was not stored in InitContext!");
        descriptor->RegisterDataSet(debugName, this);
    }

    ReadBuffer DataSetBase::GetMarshalData() const
    {
        AZ_Assert(m_streamCache.Size() != 0, "The value was not written to the stream cache!");

        return ReadBuffer(m_streamCache.GetEndianType(), m_streamCache.Get(), m_streamCache.GetExactSize());
    }

    void DataSetBase::SetDirty()
    {
        m_isDefaultValue = false;

        if (m_replicaChunk)
        {
            m_replicaChunk->SignalDataSetChanged(*this);
        }
    }

    bool DataSetBase::CanSet() const
    {
        return m_replicaChunk ? m_replicaChunk->IsMaster() : true;
    }
}
