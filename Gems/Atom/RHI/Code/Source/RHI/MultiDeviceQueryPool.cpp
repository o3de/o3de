/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/MultiDeviceQuery.h>
#include <Atom/RHI/MultiDeviceQueryPool.h>
#include <Atom/RHI/RHISystemInterface.h>

#include <AzCore/std/parallel/lock.h>

namespace AZ::RHI
{
    ResultCode MultiDeviceQueryPool::Init(MultiDevice::DeviceMask deviceMask, const QueryPoolDescriptor& descriptor)
    {
        if (Validation::IsEnabled())
        {
            if (!descriptor.m_queriesCount)
            {
                AZ_Error("RHI", false, "MultiDeviceQueryPool size can't be zero");
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

        auto resultCode = MultiDeviceResourcePool::Init(
            deviceMask,
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

                        m_deviceQueryPools[deviceIndex] = Factory::Get().CreateQueryPool();
                        resultCode = m_deviceQueryPools[deviceIndex]->Init(*device, descriptor);
                        return resultCode == ResultCode::Success;
                    });

                return resultCode;
            });

        if (resultCode != ResultCode::Success)
        {
            // Reset already initialized device-specific QueryPools and set deviceMask to 0
            m_deviceQueryPools.clear();
            MultiDeviceObject::Init(static_cast<MultiDevice::DeviceMask>(0u));
        }

        return resultCode;
    }

    ResultCode MultiDeviceQueryPool::InitQuery(MultiDeviceQuery* query)
    {
        return InitQuery(&query, 1);
    }

    ResultCode MultiDeviceQueryPool::InitQuery(MultiDeviceQuery** queries, uint32_t queryCount)
    {
        AZ_Assert(queries, "Null queries");
        auto resultCode{ ResultCode::Success };

        for (auto& [deviceIndex, deviceQueryPool] : m_deviceQueryPools)
        {
            AZStd::vector<RHI::Ptr<Query>> deviceQueries(queryCount);
            AZStd::vector<Query*> rawDeviceQueries(queryCount);
            for (auto index{ 0u }; index < queryCount; ++index)
            {
                deviceQueries[index] = RHI::Factory::Get().CreateQuery();
                rawDeviceQueries[index] = deviceQueries[index].get();
            }

            resultCode = deviceQueryPool->InitQuery(rawDeviceQueries.data(), queryCount);

            for (auto index{ 0u }; index < queryCount; ++index)
            {
                queries[index]->m_deviceQueries[deviceIndex] = deviceQueries[index];
            }

            if (resultCode != ResultCode::Success)
            {
                break;
            }
        }

        if (resultCode == ResultCode::Success)
        {
            for (auto index{ 0u }; index < queryCount; ++index)
            {
                MultiDeviceResourcePool::InitResource(
                    queries[index],
                    []()
                    {
                        return ResultCode::Success;
                    });
            }
        }

        return resultCode;
    }

    ResultCode MultiDeviceQueryPool::ValidateQueries(MultiDeviceQuery** queries, uint32_t queryCount)
    {
        if (!queryCount)
        {
            AZ_Error("RHI", false, "MultiDeviceQuery count is 0");
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
                AZ_Error("RHI", false, "MultiDeviceQuery does not belong to this pool");
                return RHI::ResultCode::InvalidArgument;
            }
        }
        return ResultCode::Success;
    }

    uint32_t MultiDeviceQueryPool::CalculateResultsCount(uint32_t queryCount)
    {
        auto deviceCount{RHISystemInterface::Get()->GetDeviceCount()};

        return CalculatePerDeviceResultsCount(queryCount) * deviceCount;
    }

    ResultCode MultiDeviceQueryPool::GetResults(MultiDeviceQuery* query, uint64_t* result, uint32_t resultsCount, QueryResultFlagBits flags)
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

        auto resultCode{ ResultCode::Success };

        auto perDeviceResultCount{CalculatePerDeviceResultsCount(1)};

        for (auto& [deviceIndex, deviceQueryPool] : m_deviceQueryPools)
        {
            auto deviceQuery{ query->m_deviceQueries[deviceIndex].get() };
            auto deviceResult{ result + (deviceIndex * perDeviceResultCount) };

            resultCode = deviceQueryPool->GetResults(&deviceQuery, 1, deviceResult, perDeviceResultCount, flags);
            if (resultCode != ResultCode::Success)
            {
                break;
            }
        }

        return resultCode;
    }

    ResultCode MultiDeviceQueryPool::GetResults(
        MultiDeviceQuery** queries, uint32_t queryCount, uint64_t* results, uint32_t resultsCount, QueryResultFlagBits flags)
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

        auto resultCode{ ResultCode::Success };

        auto perDeviceResultCount{CalculatePerDeviceResultsCount(queryCount)};

        for (auto& [deviceIndex, deviceQueryPool] : m_deviceQueryPools)
        {
            AZStd::vector<Query*> deviceQueries(queryCount);
            for (auto index{ 0u }; index < queryCount; ++index)
            {
                deviceQueries[index] = queries[index]->m_deviceQueries[deviceIndex].get();
            }

            auto deviceResults{ results + (deviceIndex * perDeviceResultCount) };
            resultCode = deviceQueryPool->GetResults(deviceQueries.data(), queryCount, deviceResults, perDeviceResultCount, flags);

            if (resultCode != ResultCode::Success)
            {
                break;
            }
        }

        return resultCode;
    }

    ResultCode MultiDeviceQueryPool::GetResults(uint64_t* results, uint32_t resultsCount, QueryResultFlagBits flags)
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

        auto resultCode{ ResultCode::Success };

        auto perDeviceResultCount{CalculatePerDeviceResultsCount(0)};

        for (auto& [deviceIndex, deviceQueryPool] : m_deviceQueryPools)
        {
            auto deviceResults{ results + (deviceIndex * perDeviceResultCount) };
            resultCode = deviceQueryPool->GetResults(deviceResults, perDeviceResultCount, flags);

            if (resultCode != ResultCode::Success)
            {
                break;
            }
        }

        return resultCode;
    }

    const QueryPoolDescriptor& MultiDeviceQueryPool::GetDescriptor() const
    {
        return m_descriptor;
    }

    void MultiDeviceQueryPool::Shutdown()
    {
        for (auto [_, pool] : m_deviceQueryPools)
        {
            pool->Shutdown();
        }

        MultiDeviceResourcePool::Shutdown();
    }

    uint32_t MultiDeviceQueryPool::CalculatePerDeviceResultsCount(uint32_t queryCount)
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