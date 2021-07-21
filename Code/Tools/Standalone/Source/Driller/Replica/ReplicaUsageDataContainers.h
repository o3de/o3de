/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef DRILLER_REPLICA_USAGE_DATA_CONTAINER_H
#define DRILLER_REPLICA_USAGE_DATA_CONTAINER_H

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/string/string.h>

#include <QColor>

#include "ReplicaBandwidthChartData.h"

namespace Driller
{
    class ReplicaDataAggregator;
    class ReplicaDataView;

    class ReplicaChunkBandwidthUsage
        : public BandwidthUsageContainer
    {
    public:
        AZ_CLASS_ALLOCATOR(ReplicaChunkBandwidthUsage, AZ::SystemAllocator, 0);

        ReplicaChunkBandwidthUsage(const char* chunkTypeName, AZ::u32 chunkIndex);

        AZ::u32 GetChunkIndex() const;
        const char* GetChunkTypeName() const;

    private:
        AZStd::string m_chunkTypeName;
        AZ::u32 m_chunkIndex;
    };

    class ReplicaDataContainer
        : public ReplicaBandwidthChartData<AZ::u32>
    {
    public:
        AZ_CLASS_ALLOCATOR(ReplicaDataContainer, AZ::SystemAllocator, 0);

        ReplicaDataContainer(const char* replicaName, AZ::u64 replicaId, const QColor& displayColor);

        const char* GetReplicaName() const;
        AZ::u64 GetReplicaId() const;

        const char* GetAxisName() const override;        

    protected:
        BandwidthUsageContainer* CreateBandwidthUsage(const ReplicaChunkEvent* dataEvent) override;
        AZ::u32 GetKeyFromEvent(const ReplicaChunkEvent* dataEvent) const override;

    private:

        AZStd::string       m_replicaName;
        AZ::u64             m_replicaId;       
    };
}

#endif
