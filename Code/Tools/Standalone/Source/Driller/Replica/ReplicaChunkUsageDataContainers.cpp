/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

#include "ReplicaChunkUsageDataContainers.h"

#include "Source/Driller/StripChart.hxx"
#include "ReplicaDataEvents.h"

#include "Source/Driller/Replica/ReplicaDisplayHelpers.h"

namespace Driller
{
    //////////////////////////
    // ReplicaBandwidthUsage
    //////////////////////////

    ReplicaBandwidthUsage::ReplicaBandwidthUsage(const char* replicaName, AZ::u64 replicaId)
        : m_replicaName(replicaName)
        , m_replicaId(replicaId)
    {
    }

    AZ::u64 ReplicaBandwidthUsage::GetReplicaId() const
    {
        return m_replicaId;
    }

    const char* ReplicaBandwidthUsage::GetReplicaName() const
    {
        return m_replicaName.c_str();
    }

    //////////////////////////////////
    // ReplicaChunkTypeDataContainer
    //////////////////////////////////

    ReplicaChunkTypeDataContainer::ReplicaChunkTypeDataContainer(const char* replicaType, const QColor& displayColor)
        : ReplicaBandwidthChartData(displayColor)
        , m_replicaType(replicaType)
    {
    }

    const char* ReplicaChunkTypeDataContainer::GetChunkType() const
    {
        return m_replicaType.c_str();
    }

    const char* ReplicaChunkTypeDataContainer::GetAxisName() const
    {
        return GetChunkType();
    }

    BandwidthUsageContainer* ReplicaChunkTypeDataContainer::CreateBandwidthUsage(const ReplicaChunkEvent* dataEvent)
    {
        return aznew ReplicaBandwidthUsage(dataEvent->GetReplicaName(), dataEvent->GetReplicaId());
    }

    AZ::u64 ReplicaChunkTypeDataContainer::GetKeyFromEvent(const ReplicaChunkEvent* dataEvent) const
    {
        return dataEvent->GetReplicaId();
    }
}
