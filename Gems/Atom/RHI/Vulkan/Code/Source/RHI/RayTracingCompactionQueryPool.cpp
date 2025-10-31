/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/VkAllocator.h>
#include <Atom/RHI.Reflect/Vulkan/Conversion.h>
#include <Atom/RHI/DeviceQueryPool.h>
#include <RHI/Device.h>
#include <RHI/RayTracingCompactionQueryPool.h>

namespace AZ
{
    namespace Vulkan
    {
        RayTracingCompactionQuery::~RayTracingCompactionQuery()
        {
            if (m_indexInPool)
            {
                auto vulkanPool = static_cast<RayTracingCompactionQueryPool*>(m_pool);
                vulkanPool->Deallocate(m_indexInPool.value());
            }
        }

        RHI::Ptr<RayTracingCompactionQuery> RayTracingCompactionQuery::Create()
        {
            return aznew RayTracingCompactionQuery;
        }

        int RayTracingCompactionQuery::GetIndexInPool()
        {
            return m_indexInPool.value();
        }

        void RayTracingCompactionQuery::Allocate()
        {
            auto vulkanPool = static_cast<RayTracingCompactionQueryPool*>(m_pool);
            m_indexInPool = vulkanPool->Allocate();
        }

        uint64_t RayTracingCompactionQuery::GetResult()
        {
            auto vulkanPool = static_cast<RayTracingCompactionQueryPool*>(m_pool);
            return vulkanPool->GetResult(m_indexInPool.value());
        }

        RHI::ResultCode RayTracingCompactionQuery::InitInternal(RHI::DeviceRayTracingCompactionQueryPool* pool)
        {
            m_pool = pool;
            return RHI::ResultCode::Success;
        }

        RHI::Ptr<RayTracingCompactionQueryPool> RayTracingCompactionQueryPool::Create()
        {
            return aznew RayTracingCompactionQueryPool;
        }

        int RayTracingCompactionQueryPool::Allocate()
        {
            auto result = m_freeList.back();
            m_freeList.pop_back();
            return result;
        }

        void RayTracingCompactionQueryPool::Deallocate(int index)
        {
            m_freeList.push_back(index);
            m_queriesEnqueuedForReset.push_back(index);
        }

        uint64_t RayTracingCompactionQueryPool::GetResult(int index)
        {
            uint64_t result;
            VkQueryResultFlags vkFlags = VK_QUERY_RESULT_64_BIT;
            auto& device = static_cast<Device&>(GetDevice());
            VkResult vkResult = device.GetContext().GetQueryPoolResults(
                device.GetNativeDevice(), m_nativeQueryPool, index, 1, sizeof(uint64_t), &result, sizeof(uint64_t), vkFlags);
            [[maybe_unused]] auto resultCode = ConvertResult(vkResult);
            AZ_Assert(resultCode == RHI::ResultCode::Success, "RayTracingCompactionQuery::GetResult: Result not ready");
            return result;
        }

        VkQueryPool RayTracingCompactionQueryPool::GetNativeQueryPool()
        {
            return m_nativeQueryPool;
        }

        void RayTracingCompactionQueryPool::ResetFreedQueries(CommandList* commandList)
        {
            AZStd::sort(m_queriesEnqueuedForReset.begin(), m_queriesEnqueuedForReset.end());
            for (int i = 0; i < m_queriesEnqueuedForReset.size();)
            {
                // Find continuous interval after current element
                int count = 1;
                while (i + count < m_queriesEnqueuedForReset.size() &&
                       m_queriesEnqueuedForReset[i + count] <= m_queriesEnqueuedForReset[i + count - 1] + 1)
                {
                    count++;
                }

                int startIndex = m_queriesEnqueuedForReset[i];
                auto& device = static_cast<Device&>(GetDevice());
                device.GetContext().CmdResetQueryPool(commandList->GetNativeCommandBuffer(), m_nativeQueryPool, startIndex, count);
                i += count;
            }
            m_queriesEnqueuedForReset.clear();
        }

        RHI::ResultCode RayTracingCompactionQueryPool::InitInternal(RHI::RayTracingCompactionQueryPoolDescriptor desc)
        {
            int queryPoolSize = desc.m_budget * RHI::Limits::Device::FrameCountMax;
            VkQueryPoolCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
            createInfo.queryType = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR;
            createInfo.queryCount = queryPoolSize;

            auto& device = static_cast<Device&>(GetDevice());

            auto vkResult =
                device.GetContext().CreateQueryPool(device.GetNativeDevice(), &createInfo, VkSystemAllocator::Get(), &m_nativeQueryPool);
            [[maybe_unused]] auto resultCode = ConvertResult(vkResult);
            AZ_Assert(
                resultCode == RHI::ResultCode::Success, "RayTracingCompactionQuery::InitInternal: Could not initialize vulkan query pool");

            m_freeList.resize(queryPoolSize);
            for (int i = 0; i < m_freeList.size(); i++)
            {
                m_freeList[i] = i;
            }
            m_queriesEnqueuedForReset = m_freeList;

            return resultCode;
        }
    } // namespace Vulkan
} // namespace AZ
