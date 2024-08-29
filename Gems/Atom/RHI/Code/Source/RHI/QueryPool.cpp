/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/Query.h>
#include <Atom/RHI/QueryPool.h>
#include <Atom/RHI/RHISystemInterface.h>

#include <AzCore/std/parallel/lock.h>

namespace AZ::RHI
{
    ResultCode QueryPool::Init(const QueryPoolDescriptor& descriptor)
    {
        if (Validation::IsEnabled())
        {
            if (!descriptor.m_queriesCount)
            {
                AZ_Error("RHI", false, "QueryPool size can't be zero");
                return ResultCode::InvalidArgument;
            }

            if (descriptor.m_type == QueryType::PipelineStatistics && descriptor.m_pipelineStatisticsMask == PipelineStatisticsFlags::None)
            {
                AZ_Error("RHI", false, "Missing pipeline statistics flags");
                return ResultCode::InvalidArgument;
            }

            if (descriptor.m_type != QueryType::PipelineStatistics && descriptor.m_pipelineStatisticsMask != PipelineStatisticsFlags::None)
            {
                AZ_Warning(
                    "RHI",
                    false,
                    "Pipeline statistics flags are only valid for PipelineStatistics pools. Ignoring m_pipelineStatisticsMask");
            }
        }

        auto resultCode = ResourcePool::Init(
            descriptor.m_deviceMask,
            [this, &descriptor]()
            {
                // Assign the descriptor prior to initialization. Technically, the descriptor is undefined
                // for uninitialized pools, so it's okay if initialization fails. Doing this removes the
                // possibility that users will get garbage values from GetDescriptor().
                m_descriptor = descriptor;

                auto resultCode{ ResultCode::Success };

                IterateDevices(
                    [this, &descriptor, &resultCode](int deviceIndex)
                    {
                        auto* device = RHISystemInterface::Get()->GetDevice(deviceIndex);

                        m_deviceObjects[deviceIndex] = Factory::Get().CreateQueryPool();

                        if (const auto& name = GetName(); !name.IsEmpty())
                        {
                            m_deviceObjects[deviceIndex]->SetName(name);
                        }

                        resultCode = GetDeviceQueryPool(deviceIndex)->Init(*device, descriptor);
                        return resultCode == ResultCode::Success;
                    });

                return resultCode;
            });

        if (resultCode != ResultCode::Success)
        {
            // Reset already initialized device-specific QueryPools and set deviceMask to 0
            m_deviceObjects.clear();
            MultiDeviceObject::Init(static_cast<MultiDevice::DeviceMask>(0u));
        }

        return resultCode;
    }

    ResultCode QueryPool::InitQuery(Query* query)
    {
        return InitQuery(&query, 1);
    }

    ResultCode QueryPool::InitQuery(Query** queries, uint32_t queryCount)
    {
        AZ_Assert(queries, "Null queries");
        auto resultCode = IterateObjects<DeviceQueryPool>([&](auto deviceIndex, auto deviceQueryPool)
        {
            AZStd::vector<RHI::Ptr<DeviceQuery>> deviceQueries(queryCount);
            AZStd::vector<DeviceQuery*> rawDeviceQueries(queryCount);
            for (auto index{ 0u }; index < queryCount; ++index)
            {
                deviceQueries[index] = RHI::Factory::Get().CreateQuery();
                rawDeviceQueries[index] = deviceQueries[index].get();
            }

            auto resultCode = deviceQueryPool->InitQuery(rawDeviceQueries.data(), queryCount);

            if (resultCode != ResultCode::Success)
            {
                return resultCode;
            }

            for (auto index{ 0u }; index < queryCount; ++index)
            {
                queries[index]->m_deviceObjects[deviceIndex] = deviceQueries[index];

                if (const auto& name = queries[index]->GetName(); !name.IsEmpty())
                {
                    queries[index]->m_deviceObjects[deviceIndex]->SetName(name);
                }
            }

            return resultCode;
        });

        if (resultCode == ResultCode::Success)
        {
            for (auto index{ 0u }; index < queryCount; ++index)
            {
                ResourcePool::InitResource(
                    queries[index],
                    []()
                    {
                        return ResultCode::Success;
                    });
            }
        }

        return resultCode;
    }

    ResultCode QueryPool::ValidateQueries(Query** queries, uint32_t queryCount)
    {
        if (!queryCount)
        {
            AZ_Error("RHI", false, "Query count is 0");
            return RHI::ResultCode::InvalidArgument;
        }

        for (uint32_t i = 0; i < queryCount; ++i)
        {
            auto* query = queries[i];
            if (!query)
            {
                AZ_Error("RHI", false, "Null query %i", i);
                return RHI::ResultCode::InvalidArgument;
            }

            if (query->GetQueryPool() != this)
            {
                AZ_Error("RHI", false, "Query does not belong to this pool");
                return RHI::ResultCode::InvalidArgument;
            }
        }
        return ResultCode::Success;
    }

    uint32_t QueryPool::CalculateResultsCount(uint32_t queryCount)
    {
        auto deviceCount{RHISystemInterface::Get()->GetDeviceCount()};

        return CalculatePerDeviceResultsCount(queryCount) * deviceCount;
    }

    ResultCode QueryPool::GetResults(Query* query, uint64_t* result, uint32_t resultsCount, QueryResultFlagBits flags)
    {
        if (Validation::IsEnabled())
        {
            auto targetResultsCount = CalculateResultsCount(1);

            if (targetResultsCount > resultsCount)
            {
                AZ_Error("RHI", false, "Results count is too small. Needed at least %d", targetResultsCount);
                return RHI::ResultCode::InvalidArgument;
            }
        }

        auto perDeviceResultCount{CalculatePerDeviceResultsCount(1)};

        return IterateObjects<DeviceQueryPool>([&](auto deviceIndex, auto deviceQueryPool)
        {
            auto deviceQuery{ query->GetDeviceQuery(deviceIndex).get() };
            auto deviceResult{ result + (deviceIndex * perDeviceResultCount) };

            return deviceQueryPool->GetResults(&deviceQuery, 1, deviceResult, perDeviceResultCount, flags);
        });
    }

    ResultCode QueryPool::GetResults(
        Query** queries, uint32_t queryCount, uint64_t* results, uint32_t resultsCount, QueryResultFlagBits flags)
    {
        AZ_Assert(queries && queryCount, "Null queries");
        AZ_Assert(results && resultsCount, "Null results");

        if (Validation::IsEnabled())
        {
            auto validationResult = ValidateQueries(queries, queryCount);
            if (validationResult != ResultCode::Success)
            {
                return validationResult;
            }

            auto targetResultsCount = CalculateResultsCount(queryCount);

            if (targetResultsCount > resultsCount)
            {
                AZ_Error("RHI", false, "Results count is too small. Needed at least %d", targetResultsCount);
                return RHI::ResultCode::InvalidArgument;
            }
        }

        auto perDeviceResultCount{CalculatePerDeviceResultsCount(queryCount)};

        return IterateObjects<DeviceQueryPool>([&](auto deviceIndex, auto deviceQueryPool)
        {
            AZStd::vector<DeviceQuery*> deviceQueries(queryCount);
            for (auto index{ 0u }; index < queryCount; ++index)
            {
                deviceQueries[index] = queries[index]->GetDeviceQuery(deviceIndex).get();
            }

            auto deviceResults{ results + (deviceIndex * perDeviceResultCount) };
            return deviceQueryPool->GetResults(deviceQueries.data(), queryCount, deviceResults, perDeviceResultCount, flags);
        });
    }

    ResultCode QueryPool::GetResults(uint64_t* results, uint32_t resultsCount, QueryResultFlagBits flags)
    {
        if (Validation::IsEnabled())
        {
            auto targetResultsCount = CalculateResultsCount();

            if (targetResultsCount > resultsCount)
            {
                AZ_Error("RHI", false, "Results count is too small. Needed at least %d", targetResultsCount);
                return RHI::ResultCode::InvalidArgument;
            }
        }

        auto perDeviceResultCount{CalculatePerDeviceResultsCount(0)};

        return IterateObjects<DeviceQueryPool>([&](auto deviceIndex, auto deviceQueryPool)
        {
            auto deviceResults{ results + (deviceIndex * perDeviceResultCount) };
            return deviceQueryPool->GetResults(deviceResults, perDeviceResultCount, flags);
        });
    }

    const QueryPoolDescriptor& QueryPool::GetDescriptor() const
    {
        return m_descriptor;
    }

    void QueryPool::Shutdown()
    {
        ResourcePool::Shutdown();
    }

    uint32_t QueryPool::CalculatePerDeviceResultsCount(uint32_t queryCount)
    {
        uint32_t perResultSize = m_descriptor.m_type == QueryType::PipelineStatistics
            ? CountBitsSet(static_cast<uint64_t>(m_descriptor.m_pipelineStatisticsMask))
            : 1;

        if(queryCount == 0)
        {
            queryCount = m_descriptor.m_queriesCount;
        }

        return perResultSize * queryCount;
    }
} // namespace AZ::RHI
