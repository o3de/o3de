/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

#include "ReplicaUsageDataContainers.h"

#include "Source/Driller/StripChart.hxx"
#include "ReplicaDataEvents.h"

namespace Driller
{
    ///////////////////////////////
    // ReplicaChunkBandwidthUsage
    ///////////////////////////////

    ReplicaChunkBandwidthUsage::ReplicaChunkBandwidthUsage(const char* chunkTypeName, AZ::u32 chunkIndex)
        : m_chunkIndex(chunkIndex)
        , m_chunkTypeName(chunkTypeName)
    {
    }

    AZ::u32 ReplicaChunkBandwidthUsage::GetChunkIndex() const
    {
        return m_chunkIndex;
    }

    const char* ReplicaChunkBandwidthUsage::GetChunkTypeName() const
    {
        return m_chunkTypeName.c_str();
    }

    /////////////////////////
    // ReplicaDataContainer
    /////////////////////////

    ReplicaDataContainer::ReplicaDataContainer(const char* replicaName, AZ::u64 replicaId, const QColor& displayColor)
        : ReplicaBandwidthChartData(displayColor)
        , m_replicaName(replicaName)
        , m_replicaId(replicaId)
    {
    }

    const char* ReplicaDataContainer::GetReplicaName() const
    {
        return m_replicaName.c_str();
    }

    AZ::u64 ReplicaDataContainer::GetReplicaId() const
    {
        return m_replicaId;
    }

    const char* ReplicaDataContainer::GetAxisName() const
    {
        return GetReplicaName();
    }    

    BandwidthUsageContainer* ReplicaDataContainer::CreateBandwidthUsage(const ReplicaChunkEvent* chunkEvent)
    {
        return aznew ReplicaChunkBandwidthUsage(chunkEvent->GetChunkTypeName(), chunkEvent->GetReplicaChunkIndex());
    }

    AZ::u32 ReplicaDataContainer::GetKeyFromEvent(const ReplicaChunkEvent* chunkEvent) const
    {
        return chunkEvent->GetReplicaChunkIndex();
    }
}
