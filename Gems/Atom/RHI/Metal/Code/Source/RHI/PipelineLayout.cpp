/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/Metal/PipelineLayoutDescriptor.h>
#include <Atom/RHI.Reflect/ShaderStages.h>
#include <RHI/Conversions.h>
#include <RHI/Device.h>
#include <RHI/PipelineLayout.h>

namespace AZ
{
    namespace Metal
    {
        AZ::HashValue64 PipelineLayout::GetHash() const
        {
            return m_hash;
        }

        PipelineLayout::PipelineLayout(PipelineLayoutCache& parentCache)
        : m_parentCache{&parentCache}
        {}
        
        PipelineLayout::~PipelineLayout()
        {
        }
        
        void PipelineLayout::add_ref() const
        {
            AZ_Assert(m_useCount >= 0, "PipelineLayout has been deleted");
            ++m_useCount;
        }
        
        void PipelineLayout::release() const
        {
            AZ_Assert(m_useCount > 0, "Usecount is already 0!");
            if (m_useCount.fetch_sub(1) == 1)
            {
                // The cache has ownership.
                if (m_parentCache)
                {
                    m_parentCache->TryReleasePipelineLayout(this);
                }
                // We were orphaned by the cache, just delete.
                else
                {
                    delete this;
                }
            }
        }

        void PipelineLayout::Init(id<MTLDevice> metalDevice, const RHI::PipelineLayoutDescriptor& descriptor)
        {
            m_hash = descriptor.GetHash();
            const uint32_t groupLayoutCount = static_cast<uint32_t>(descriptor.GetShaderResourceGroupLayoutCount());
            AZStd::vector<id<MTLSamplerState>> resourceSamplers;
            
            const PipelineLayoutDescriptor& rhiLayoutDescriptor = static_cast<const PipelineLayoutDescriptor&>(descriptor);
            m_slotToIndexTable = rhiLayoutDescriptor.GetSlotToIndexTable();
            m_indexToSlotTable = rhiLayoutDescriptor.GetIndexToSlotTable();
            
            m_srgVisibilities.resize(RHI::Limits::Pipeline::ShaderResourceGroupCountMax);
            m_srgResourcesVisibility.resize(RHI::Limits::Pipeline::ShaderResourceGroupCountMax);
            m_srgResourcesVisibilityHash.resize(RHI::Limits::Pipeline::ShaderResourceGroupCountMax);
            for (uint32_t srgLayoutIdx = 0; srgLayoutIdx < groupLayoutCount; ++srgLayoutIdx)
            {
                const RHI::ShaderResourceGroupLayout& srgLayout = *descriptor.GetShaderResourceGroupLayout(srgLayoutIdx);
                uint32_t bindingSlot = srgLayout.GetBindingSlot();
                RHI::ShaderStageMask mask = RHI::ShaderStageMask::None;
                
                //srgIndex that respects the frequencyId
                uint32_t srgIndex = m_slotToIndexTable[bindingSlot];
                const ShaderResourceGroupVisibility& srgVis = rhiLayoutDescriptor.GetShaderResourceGroupVisibility(srgIndex);
                mask |= srgVis.m_constantDataStageMask;
                
                for (const RHI::ShaderInputBufferDescriptor& shaderInputBuffer : srgLayout.GetShaderInputListForBuffers())
                {
                    auto findStageMaskIt = srgVis.m_resourcesStageMask.find(shaderInputBuffer.m_name);
                    AZ_Error("PipelineLayout", findStageMaskIt != srgVis.m_resourcesStageMask.end(), "No Visibility information related to %s", shaderInputBuffer.m_name.GetCStr());
                    mask |= findStageMaskIt->second;
                }
                
                for (const RHI::ShaderInputImageDescriptor& shaderInputImage : srgLayout.GetShaderInputListForImages())
                {
                    auto findStageMaskIt = srgVis.m_resourcesStageMask.find(shaderInputImage.m_name);
                    AZ_Error("PipelineLayout", findStageMaskIt != srgVis.m_resourcesStageMask.end(), "No Visibility information related to %s", shaderInputImage.m_name.GetCStr());
                    mask |= findStageMaskIt->second;
                }
                
                for (const RHI::ShaderInputSamplerDescriptor& shaderInputSampler : srgLayout.GetShaderInputListForSamplers())
                {
                    auto findStageMaskIt = srgVis.m_resourcesStageMask.find(shaderInputSampler.m_name);
                    AZ_Error("PipelineLayout", findStageMaskIt != srgVis.m_resourcesStageMask.end(), "No Visibility information related to %s", shaderInputSampler.m_name.GetCStr());
                    mask |= findStageMaskIt->second;
                }
                
                for (const RHI::ShaderInputStaticSamplerDescriptor& shaderInputStaticSampler : srgLayout.GetStaticSamplers())
                {
                    auto findStageMaskIt = srgVis.m_resourcesStageMask.find(shaderInputStaticSampler.m_name);
                    AZ_Error("PipelineLayout", findStageMaskIt != srgVis.m_resourcesStageMask.end(), "No Visibility information related to %s", shaderInputStaticSampler.m_name.GetCStr());
                    mask |= findStageMaskIt->second;
                }

                m_srgVisibilities[srgIndex] = mask;
                m_srgResourcesVisibility[srgIndex] = srgVis;
                m_srgResourcesVisibilityHash[srgIndex] = srgVis.GetHash();
            }           
            
            // Cache the inline constant size and slot index
            const RootConstantBinding& rootConstantBinding = rhiLayoutDescriptor.GetRootConstantBinding();
            m_rootConstantsSize = rhiLayoutDescriptor.GetRootConstantsLayout()->GetDataSize();
            m_rootConstantSlotIndex = rootConstantBinding.m_constantRegisterSpace;
           
            m_isCompiled = true;
        }
        
        size_t PipelineLayout::GetSlotByIndex(size_t index) const
        {
            return m_indexToSlotTable[index];
        }
        
        size_t PipelineLayout::GetIndexBySlot(size_t slot) const
        {
            return m_slotToIndexTable[slot];
        }
        
        const RHI::ShaderStageMask& PipelineLayout::GetSrgVisibility(uint32_t index) const
        {
            return m_srgVisibilities[index];
        }
    
        const ShaderResourceGroupVisibility& PipelineLayout::GetSrgResourcesVisibility(uint32_t index) const
        {
            return m_srgResourcesVisibility[index];
        }

        const AZ::HashValue64 PipelineLayout::GetSrgResourcesVisibilityHash(uint32_t index) const
        {
            return m_srgResourcesVisibilityHash[index];
        }
    
        uint32_t PipelineLayout::GetRootConstantsSize() const
        {
            return m_rootConstantsSize;
        }
    
        uint32_t PipelineLayout::GetRootConstantsSlotIndex() const
        {
            return m_rootConstantSlotIndex;
        }
    
        RHI::ConstPtr<PipelineLayout> PipelineLayoutCache::Allocate(const RHI::PipelineLayoutDescriptor& descriptor)
        {
            AZ_Assert(m_parentDevice, "Cache is not initialized.");
            
            const AZ::HashValue64 hashCode = descriptor.GetHash();
            bool isFirstCompile = false;
            
            RHI::Ptr<PipelineLayout> layout;
            
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
                auto iter = m_pipelineLayouts.find(hashCode);
                
                // Reserve space so the next inquiry will find that someone got here first.
                if (iter == m_pipelineLayouts.end())
                {
                    iter = m_pipelineLayouts.emplace(hashCode, aznew PipelineLayout(*this)).first;
                    isFirstCompile = true;
                }
                
                layout = iter->second;
            }
            
            if (isFirstCompile)
            {
                layout->Init(m_parentDevice->GetMtlDevice(), descriptor);
            }
            else
            {
                // If we are another thread that has requested an uncompiled root signature, but we lost the race
                // to compile it, we now have to wait until the other thread finishes compilation.
                while (!layout->m_isCompiled)
                {
                    AZStd::this_thread::yield();
                }
            }
            
            return layout;
        }
        
        void PipelineLayoutCache::Init(Device& device)
        {
            m_parentDevice = &device;
        }
        
        void PipelineLayoutCache::Shutdown()
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
            
            /**
             * Orphan any remaining pipeline layouts from the cache so they don't
             * de-reference a garbage parent pool pointer when they shutdown.
             */
            for (auto& keyValue : m_pipelineLayouts)
            {
                keyValue.second->m_parentCache = nullptr;
            }
            m_pipelineLayouts.clear();
            m_parentDevice = nullptr;
        }
        
        void PipelineLayoutCache::TryReleasePipelineLayout(const PipelineLayout* pipelineLayout)
        {
            if (!pipelineLayout)
            {
                return;
            }
            
            AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
            
            /**
             * need to check the count again in here in case
             * someone was trying to get the name on another thread
             * Set it to -1 so only this thread will attempt to clean up the
             * cache and delete the pipeline layout.
             */
            
            int32_t expectedRefCount = 0;
            if (pipelineLayout->m_useCount.compare_exchange_strong(expectedRefCount, -1))
            {
                auto iter = m_pipelineLayouts.find(pipelineLayout->GetHash());
                if (iter != m_pipelineLayouts.end())
                {
                    m_pipelineLayouts.erase(iter);
                }
                
                delete pipelineLayout;
            }
        }
    }
}
