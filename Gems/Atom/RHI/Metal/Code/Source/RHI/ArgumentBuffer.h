/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceObject.h>
#include <Atom/RHI.Reflect/Metal/PipelineLayoutDescriptor.h>
#include <Atom/RHI.Reflect/SamplerState.h>
#include <AzCore/Debug/Trace.h>
#include <Metal/Metal.h>
#include <RHI/BufferMemoryAllocator.h>
#include <RHI/Conversions.h>
#include <RHI/ShaderResourceGroupPool.h>

//Disable this for better gpu captures. Xcode tooling is not good at handling Argument buffers that are sub-allocated
//from a big buffer (i.e Page) and hence the recommendaiton is to disable this to allocate individual
//buffers for ABs. Having individual buffer means that buffer labelling will also work making it easier
//to travers SRGs within the capture. 
#define ARGUMENTBUFFER_PAGEALLOCATOR

struct ResourceBindingData
{
    id<MTLResource> m_resourcPtr = nil;
    AZ::Metal::ResourceType m_rescType = AZ::Metal::ResourceType::MtlUndefined;
    union
    {
        AZ::RHI::ShaderInputImageAccess m_imageAccess;
        AZ::RHI::ShaderInputBufferAccess m_bufferAccess;
    };

    bool operator==(const ResourceBindingData& other) const
    {
        return this->m_resourcPtr == other.m_resourcPtr;
    };

    size_t GetHash() const
    {
        AZ_Assert(m_resourcPtr, "m_resourcPtr is null");
        return m_resourcPtr.hash;
    }
};

namespace AZStd
{
   template<>
   struct hash<ResourceBindingData>
   {
       size_t operator()(const ResourceBindingData& resourceBindingData) const noexcept
       {
           return resourceBindingData.GetHash();
       }
   };
}

//! Function which returns the native hash instead of trying to re-calculate a new hash. Used in data structures that require hash calculation below
struct MetalResourceHash
{
    size_t operator()(const id<MTLResource> m_resourcPtr) const
    {
        return m_resourcPtr.hash;
    }
};

namespace AZ
{
    namespace Metal
    {
        class Device;
        class BufferMemoryAllocator;
        class ShaderResourceGroup;
        struct ShaderResourceGroupCompiledData;

        class ArgumentBuffer final
            : public RHI::DeviceObject
        {
            using Base = RHI::DeviceObject;

        public:
            AZ_CLASS_ALLOCATOR(ArgumentBuffer, AZ::SystemAllocator, 0);
            AZ_RTTI(ArgumentBuffer, "FEFE8823-7772-4EA0-9241-65C49ADFF6B3", Base);

            ArgumentBuffer() = default;

            static RHI::Ptr<ArgumentBuffer> Create();
            void Init(Device* device,
                      RHI::ConstPtr<RHI::ShaderResourceGroupLayout> srgLayout,
                      ShaderResourceGroup& group,
                      ShaderResourceGroupPool* srgPool);

            void UpdateImageViews(const RHI::ShaderInputImageDescriptor& shaderInputImage,
                                  const AZStd::span<const RHI::ConstPtr<RHI::ImageView>>& imageViews);

            void UpdateSamplers(const RHI::ShaderInputSamplerDescriptor& shaderInputSampler,
                                const AZStd::span<const RHI::SamplerState>& samplerStates);

            void UpdateBufferViews(const RHI::ShaderInputBufferDescriptor& shaderInputBuffer,
                                   const AZStd::span<const RHI::ConstPtr<RHI::BufferView>>& bufferViews);

            void UpdateConstantBufferViews(AZStd::span<const uint8_t> rawData);

            id<MTLBuffer> GetArgEncoderBuffer() const;
            size_t GetOffset() const;

            //Map to cache all the resources based on the usage as we can batch all the resources for a given usage.
            using ResourcesForCompute = AZStd::unordered_set<id <MTLResource>, MetalResourceHash>;
            //Map to cache all the resources based on the usage and shader stage as we can batch all the resources for a given usage/shader usage.
            using ResourcesPerStageForGraphics = AZStd::array<AZStd::unordered_set<id <MTLResource>, MetalResourceHash>, RHI::ShaderStageGraphicsCount>;
              
            //Cache untracked resources we want to make resident for this argument buffer for graphics work
            void CollectUntrackedResources(const ShaderResourceGroupVisibility& srgResourcesVisInfo,
                                           ResourcesPerStageForGraphics& untrackedResourcesRead,
                                           ResourcesPerStageForGraphics& untrackedResourcesReadWrite) const;

            //Cache untracked resources we want to make resident for this argument buffer for compute work
            void CollectUntrackedResources(const ShaderResourceGroupVisibility& srgResourcesVisInfo,
                                           ResourcesForCompute& untrackedResourceComputeRead,
                                           ResourcesForCompute& untrackedResourceComputeReadWrite) const;

            bool IsNullHeapNeededForVertexStage(const ShaderResourceGroupVisibility& srgResourcesVisInfo) const;
            bool IsNullDescHeapNeeded() const;

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceObject
            void Shutdown() override;
            //////////////////////////////////////////////////////////////////////////

        private:

            static const int MaxEntriesInArgTable = 31;
            using ResourceBindingsSet = AZStd::unordered_set<ResourceBindingData>;
            using ResourceBindingsMap =  AZStd::unordered_map<AZ::Name, ResourceBindingsSet>;
            ResourceBindingsMap m_resourceBindings;
            
            //Helper functions that help cache untracked resources for compute and graphics work
            void CollectResourcesForCompute(const ResourceBindingsSet& resourceBindingData,
                                            ResourcesForCompute& untrackedResourceComputeRead,
                                            ResourcesForCompute& untrackedResourceComputeReadWrite) const;
            void CollectResourcesForGraphics(RHI::ShaderStageMask visShaderMask,
                                             const ResourceBindingsSet& resourceBindingDataSet,
                                             ResourcesPerStageForGraphics& untrackedResourcesRead,
                                             ResourcesPerStageForGraphics& untrackedResourcesReadWrite) const;
            void AddUntrackedResource(MTLRenderStages mtlRenderStages,
                                      id<MTLResource> resourcPtr,
                                      ResourcesPerStageForGraphics& resourceSet) const;
            
            //! Populate all the descriptors related to entries within a SRG
            bool CreateArgumentDescriptors(NSMutableArray * argBufferDecriptors);
            
            //! Create and attach a static sampler within the argument buffer
            void AttachStaticSamplers();
            
            //! Create and attach a constant buffer related to all the loose data within a SRG.
            void AttachConstantBuffer();
            
            //! Bind null samplers in case we dont bind a proper sampler.
            void BindNullSamplers(uint32_t registerId, uint32_t samplerCount);
            
            // Use a cache to store and retrieve samplers
            id<MTLSamplerState> GetMtlSampler(MTLSamplerDescriptor* samplerDesc);

            Device* m_device = nullptr;
            RHI::ConstPtr<RHI::ShaderResourceGroupLayout> m_srgLayout;

            id <MTLArgumentEncoder> m_argumentEncoder;
            uint32_t m_constantBufferSize = 0;
            bool m_useNullDescriptorHeap = false;
#if defined(ARGUMENTBUFFER_PAGEALLOCATOR)
            BufferMemoryView m_argumentBuffer;
            BufferMemoryView m_constantBuffer;
#else
            //We are keeping the non-page allocation implementation for GPU captures
            //GPU captures dont work well with argument buffers with offsets
            MemoryView m_argumentBuffer;
            MemoryView m_constantBuffer;
#endif
        };
    }
}
