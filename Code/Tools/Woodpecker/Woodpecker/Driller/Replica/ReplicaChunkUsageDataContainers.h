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

#ifndef DRILLER_REPLICA_CHUNK_USAGE_DATA_CONTAINER_H
#define DRILLER_REPLICA_CHUNK_USAGE_DATA_CONTAINER_H

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/string/string.h>

#include <QColor>

#include "ReplicaBandwidthChartData.h"

namespace Driller
{
    class ReplicaDataAggregator;
    class ReplicaChunkEvent;
    class ReplicaDataView;

    class BaseDetailDisplayHelper;

    class ReplicaBandwidthUsage
        : public BandwidthUsageContainer
    {
    public:
        AZ_CLASS_ALLOCATOR(ReplicaBandwidthUsage, AZ::SystemAllocator, 0);
        ReplicaBandwidthUsage(const char* replicaName, AZ::u64 replicaId);

        AZ::u64 GetReplicaId() const;
        const char* GetReplicaName() const;

    private:
        AZStd::string   m_replicaName;
        AZ::u64         m_replicaId;
    };

    class ReplicaChunkTypeDataContainer
        : public ReplicaBandwidthChartData<AZ::u64>
    {
    public:
        AZ_CLASS_ALLOCATOR(ReplicaChunkTypeDataContainer, AZ::SystemAllocator, 0);

        ReplicaChunkTypeDataContainer(const char* replicaType, const QColor& displayColor);

        const char* GetChunkType() const;
        const char* GetAxisName() const override;

    protected:
        BandwidthUsageContainer* CreateBandwidthUsage(const ReplicaChunkEvent* dataEvent) override;
        AZ::u64 GetKeyFromEvent(const ReplicaChunkEvent* dataEvent) const override;

    private:

        ReplicaBandwidthUsage* GetReplicaForFrame(AZ::u64 frameId, const ReplicaChunkEvent* dataEvent);

        AZStd::string   m_replicaType;
    };
}

#endif