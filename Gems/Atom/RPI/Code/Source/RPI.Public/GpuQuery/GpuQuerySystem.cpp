/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI/RHISystemInterface.h>

#include <Atom/RPI.Public/GpuQuery/GpuQuerySystem.h>
#include <Atom/RPI.Public/GpuQuery/TimestampQueryPool.h>

#include <Atom/RPI.Reflect/GpuQuerySystemDescriptor.h>

#include <AzCore/Interface/Interface.h>

namespace AZ
{
    namespace RPI
    {
        GpuQuerySystemInterface* GpuQuerySystemInterface::Get()
        {
            return Interface<GpuQuerySystemInterface>::Get();
        }

        void GpuQuerySystem::Init(const GpuQuerySystemDescriptor& desc)
        {
            // Cache the feature support for QueryTypes.
            CacheFeatureSupport();

            // Create the Timestamp QueryPool.
            if (IsQueryTypeSupported(RHI::QueryType::Timestamp))
            {
                // The limit of RPI Queries that is able to be created with this pool.
                const uint32_t TimestampQueryCount = desc.m_timestampQueryCount;

                // Create the Timestamp QueryPool.
                QueryPoolPtr timestampQueryPool = TimestampQueryPool::CreateTimestampQueryPool(TimestampQueryCount);
                m_queryPoolArray[static_cast<uint32_t>(RHI::QueryType::Timestamp)] = AZStd::move(timestampQueryPool);
            }

            // Create the Pipeline Statistics QueryPool.
            if (IsQueryTypeSupported(RHI::QueryType::PipelineStatistics))
            {
                // The limit of RPI Queries that is able to be created with this pool.
                const uint32_t PipelineStatisticsQueryCount = desc.m_statisticsQueryCount;
                // The amount of RHI Queries required to calculate a single result.
                const uint32_t RhiQueriesPerPipelineStatisticsResult = 1u;

                // Create the PipelineStatistics QueryPool.
                QueryPoolPtr pipelineStatisticsQueryPool = QueryPool::CreateQueryPool(PipelineStatisticsQueryCount, RhiQueriesPerPipelineStatisticsResult, RHI::QueryType::PipelineStatistics, desc.m_statisticsQueryFlags);
                m_queryPoolArray[static_cast<uint32_t>(RHI::QueryType::PipelineStatistics)] = AZStd::move(pipelineStatisticsQueryPool);
            }

            // Register the system to the interface.
            Interface<GpuQuerySystemInterface>::Register(this);
        }

        void GpuQuerySystem::Shutdown()
        {
            // Ensure all the query related resource are released before RHI System is shutdown.
            for (auto& queryPool : m_queryPoolArray)
            {
                queryPool = nullptr;
            }

            // Unregister the system to the interface.
            Interface<GpuQuerySystemInterface>::Unregister(this);
        }

        void GpuQuerySystem::Update()
        {
            AZ_PROFILE_SCOPE(RPI, "GpuQuerySystem: Update");
            for (auto& queryPool : m_queryPoolArray)
            {
                if (queryPool)
                {
                    queryPool->Update();
                }
            }
        }

        RHI::Ptr<Query> GpuQuerySystem::CreateQuery(RHI::QueryType queryType, RHI::QueryPoolScopeAttachmentType attachmentType, RHI::ScopeAttachmentAccess attachmentAccess)
        {
            RPI::QueryPool* queryPool = GetQueryPoolByType(queryType);
            if (queryPool)
            {
                return queryPool->CreateQuery(attachmentType, attachmentAccess);
            }

            return nullptr;
        }

        void GpuQuerySystem::CacheFeatureSupport()
        {
            // Use the device that is registered with the RHISystemInterface
            RHI::Device* device = RHI::RHISystemInterface::Get()->GetDevice();

            for (RHI::QueryTypeFlags commandQueueQueryTypeFlags : device->GetFeatures().m_queryTypesMask)
            {
                m_queryTypeSupport |= commandQueueQueryTypeFlags;
            }
        }

        bool GpuQuerySystem::IsQueryTypeValid(RHI::QueryType queryType)
        {
            const uint32_t queryTypeIndex = static_cast<uint32_t>(queryType);
            return queryTypeIndex < static_cast<uint32_t>(RHI::QueryType::Count);
        }

        bool GpuQuerySystem::IsQueryTypeSupported(RHI::QueryType queryType)
        {
            AZ_Assert(IsQueryTypeValid(queryType), "Provided QueryType is invalid");

            return static_cast<uint32_t>(m_queryTypeSupport) & AZ_BIT(static_cast<uint32_t>(queryType));
        }

        RPI::QueryPool* GpuQuerySystem::GetQueryPoolByType(RHI::QueryType queryType)
        {
            const uint32_t queryTypeIndex = static_cast<uint32_t>(queryType);
            const bool validQueryType = IsQueryTypeValid(queryType);

            // Only return the QueryPool if the QueryType is valid and if the QueryPool is initialized
            if (validQueryType && m_queryPoolArray[queryTypeIndex])
            {
                return m_queryPoolArray[queryTypeIndex].get();
            }

            return nullptr;
        }

    }; // namespace RPI
}; // namespace AZ
