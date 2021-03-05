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
#include "Atom_RHI_Vulkan_precompiled.h"
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <RHI/Conversion.h>
#include <RHI/Device.h>
#include <RHI/QueryPool.h>
#include <RHI/ReleaseContainer.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::Ptr<QueryPool> QueryPool::Create()
        {
            return aznew QueryPool();
        }

        VkQueryPool QueryPool::GetNativeQueryPool() const
        {
            return m_nativeQueryPool;
        }

        void QueryPool::ResetQueries(CommandList& commandList, const RHI::Interval& interval)
        {
            vkCmdResetQueryPool(
                commandList.GetNativeCommandBuffer(),
                m_nativeQueryPool,
                interval.m_min,
                interval.m_max - interval.m_min + 1
            );
        }

        RHI::ResultCode QueryPool::InitInternal(RHI::Device& baseDevice, const RHI::QueryPoolDescriptor& descriptor)
        {
            auto& device = static_cast<Device&>(baseDevice);
            RHI::ResultCode result = BuildNativeQueryPool(device, descriptor);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);
            return RHI::ResultCode::Success;
        }

        RHI::ResultCode QueryPool::InitQueryInternal([[maybe_unused]] RHI::Query& query)
        {
            // Nothing to initialize
            return RHI::ResultCode::Success;
        }

        RHI::ResultCode QueryPool::GetResultsInternal(uint32_t startIndex, uint32_t queryCount, uint64_t* results, uint32_t resultsCount, RHI::QueryResultFlagBits flags)
        {
            VkQueryResultFlags vkFlags = VK_QUERY_RESULT_64_BIT;
            vkFlags |= RHI::CheckBitsAll(flags, RHI::QueryResultFlagBits::Wait) ? VK_QUERY_RESULT_WAIT_BIT : 0;
            VkResult vkResult = vkGetQueryPoolResults(
                static_cast<Device&>(GetDevice()).GetNativeDevice(),
                m_nativeQueryPool,
                startIndex,
                queryCount,
                resultsCount * sizeof(uint64_t),
                results,
                sizeof(uint64_t),
                vkFlags);

            return ConvertResult(vkResult);
        }

        RHI::ResultCode QueryPool::BuildNativeQueryPool(Device& device, const RHI::QueryPoolDescriptor& descriptor)
        {
            VkQueryPoolCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
            createInfo.queryType = ConvertQueryType(descriptor.m_type);
            createInfo.queryCount = descriptor.m_queriesCount;
            createInfo.pipelineStatistics = ConvertQueryPipelineStatisticMask(descriptor.m_pipelineStatisticsMask);

            auto vkResult = vkCreateQueryPool(
                device.GetNativeDevice(),
                &createInfo,
                nullptr,
                &m_nativeQueryPool);

            return ConvertResult(vkResult);
        }

        void QueryPool::ShutdownInternal()
        {
            Base::ShutdownInternal();
            if (m_nativeQueryPool)
            {
                auto& device = static_cast<Device&>(GetDevice());
                device.QueueForRelease(new ReleaseContainer<VkQueryPool>(device.GetNativeDevice(), m_nativeQueryPool, vkDestroyQueryPool));
                m_nativeQueryPool = VK_NULL_HANDLE;
            }
        }
    }
}
