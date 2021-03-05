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

#include "Woodpecker_precompiled.h"

#include "ReplicaChunkUsageDataContainers.h"

#include "Woodpecker/Driller/StripChart.hxx"
#include "ReplicaDataEvents.h"

#include "Woodpecker/Driller/Replica/ReplicaDisplayHelpers.h"

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
