/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "Atom_RHI_Vulkan_precompiled.h"
#include <AzCore/std/algorithm.h>
#include <AzCore/std/parallel/lock.h>
#include <Atom/RHI.Reflect/ShaderResourceGroupLayout.h>
#include <RHI/DescriptorSetAllocator.h>
#include <RHI/Device.h>

namespace AZ
{
    namespace Vulkan
    {
        namespace Internal
        {
            ///////////////////////////////////////////////////////////////////
            // DescriptorPoolFactory
            ///////////////////////////////////////////////////////////////////
            void DescriptorPoolFactory::Init(const Descriptor& descriptor)
            {
                m_descriptor = descriptor;
            }

            RHI::Ptr<DescriptorPool> DescriptorPoolFactory::CreateObject(const DescriptorPool::Descriptor& poolDescriptor)
            {
                RHI::Ptr<DescriptorPool> descriptorPool = DescriptorPool::Create();
                if (descriptorPool->Init(poolDescriptor) != RHI::ResultCode::Success)
                {
                    AZ_Printf("Vulkan", "Failed to initialize DescriptorSet");
                    return nullptr;
                }

                return descriptorPool;
            }

            void DescriptorPoolFactory::ResetObject(DescriptorPool& descriptorPool, const DescriptorPool::Descriptor& poolDescriptor)
            {
                AZ_UNUSED(poolDescriptor);
                descriptorPool.Reset();
            }

            void DescriptorPoolFactory::ShutdownObject(DescriptorPool& descriptorPool, bool isPoolShutdown)
            {
                AZ_UNUSED(isPoolShutdown);
                descriptorPool.Shutdown();
            }

            bool DescriptorPoolFactory::CollectObject(DescriptorPool& descriptorPool)
            {
                AZ_UNUSED(descriptorPool);
                return true;
            }

            const DescriptorPoolFactory::Descriptor& DescriptorPoolFactory::GetDescriptor() const
            {
                return m_descriptor;
            }    
            
            void DescriptorSetSubAllocator::Init(DescriptorPoolAllocator& descriptorPoolAllocator, Device& device, const DescriptorPool::Descriptor& poolDescriptor)
            {
                m_device = &device;
                m_descriptorPoolAllocator = &descriptorPoolAllocator;
                m_poolDescriptor = poolDescriptor;
            }

            RHI::Ptr<DescriptorSetSubAllocator::ObjectType> DescriptorSetSubAllocator::Allocate(DescriptorSetLayout& layout)
            {
                // Look for a pool that can allocate the descriptor set
                for (DescriptorPool* pool : m_pools)
                {
                    // Check that we don't get over the max descriptor sets count.
                    // In theory the pool would return a VK_ERROR_OUT_OF_POOL_MEMORY result but that would
                    // trigger a validation layer error that we want to avoid.
                    if ((pool->GetTotalObjectCount() + 1) > m_poolDescriptor.m_maxSets)
                    {
                        continue;
                    }

                    auto result = pool->Allocate(layout);
                    VkResult vkResult = result.first;
                    if (vkResult == VK_SUCCESS)
                    {
                        return result.second;
                    }
                    else if (vkResult != VK_ERROR_FRAGMENTED_POOL && vkResult != VK_ERROR_OUT_OF_POOL_MEMORY)
                    {
                        AZ_Assert(false, "Failed to Allocate descriptor set");
                        return nullptr;
                    }
                }

                DescriptorPool* newPool = m_descriptorPoolAllocator->Allocate(m_poolDescriptor);
                auto result = newPool->Allocate(layout);
                if (result.first != VK_SUCCESS)
                {
                    AZ_Assert(false, "Failed to Allocate descriptor set");
                    return nullptr;
                }
                m_pools.push_front(newPool);
                return result.second;
            }

            void DescriptorSetSubAllocator::DeAllocate(RHI::Ptr<ObjectType> descriptorSet)
            {
                DescriptorPool* descriptorPool = const_cast<DescriptorPool*>(descriptorSet->GetDescriptor().m_descriptorPool);
                descriptorPool->DeAllocate(descriptorSet);
            }

            void DescriptorSetSubAllocator::Reset()
            {
                for (DescriptorPool* pool : m_pools)
                {
                    m_descriptorPoolAllocator->DeAllocate(pool);
                }

                m_descriptorPoolAllocator = nullptr;
            }

            void DescriptorSetSubAllocator::Collect()
            {
                auto it = m_pools.begin();
                while (it != m_pools.end())
                {
                    DescriptorPool* pool = *it;
                    pool->Collect();
                    if (pool->GetTotalObjectCount() == 0)
                    {
                        m_descriptorPoolAllocator->DeAllocate(pool);
                        it = m_pools.erase(it);
                    }
                    else
                    {
                        ++it;
                    }
                }
            }
        }

        RHI::ResultCode DescriptorSetAllocator::Init(const Descriptor& descriptor)
        {
            AZ_Assert(m_isInitialized == false, "DescriptorSetAllocator already initialized!");
            m_descriptor = descriptor;
            Base::Init(*m_descriptor.m_device);

            Internal::DescriptorPoolAllocator::Descriptor poolAllocatorDescriptor;
            poolAllocatorDescriptor.m_device = m_descriptor.m_device;
            poolAllocatorDescriptor.m_collectLatency = descriptor.m_frameCountMax;
            m_poolAllocator.Init(poolAllocatorDescriptor);

            DescriptorPool::Descriptor poolDescriptor;
            poolDescriptor.m_device = m_descriptor.m_device;
            poolDescriptor.m_maxSets = m_descriptor.m_poolSize;
            poolDescriptor.m_constantDataPool = m_descriptor.m_constantDataPool;
            poolDescriptor.m_collectLatency = descriptor.m_frameCountMax;
            AZStd::unordered_map<VkDescriptorType, VkDescriptorPoolSize> sizesByType;
            for (const auto& layoutBinding : descriptor.m_layout->GetNativeLayoutBindings())
            {
                sizesByType[layoutBinding.descriptorType].descriptorCount += layoutBinding.descriptorCount * m_descriptor.m_poolSize;
            }
            poolDescriptor.m_descriptorPoolSizes.reserve(sizesByType.size());
            AZStd::transform(sizesByType.begin(), sizesByType.end(), AZStd::back_inserter(poolDescriptor.m_descriptorPoolSizes), [](auto &it) 
            {
                it.second.type = it.first;
                return it.second; 
            });
            m_subAllocator.Init(m_poolAllocator, *m_descriptor.m_device, poolDescriptor);
            
            m_isInitialized = true;
            return RHI::ResultCode::Success;
        }

        RHI::Ptr<DescriptorSetAllocator::ObjectType> DescriptorSetAllocator::Allocate(DescriptorSetLayout& layout)
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_subAllocatorMutex);
            return m_subAllocator.Allocate(layout);
        }

        void DescriptorSetAllocator::DeAllocate(RHI::Ptr<ObjectType> descriptorSet)
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_subAllocatorMutex);
            m_subAllocator.DeAllocate(descriptorSet);
        }

        void DescriptorSetAllocator::Collect()
        {
            m_subAllocator.Collect();
            m_poolAllocator.Collect();
        }

        void DescriptorSetAllocator::Shutdown()
        {
            if (m_isInitialized)
            {
                m_subAllocator.Reset();
                m_poolAllocator.Shutdown();
                m_isInitialized = false;
            }
        }    
    }
}
