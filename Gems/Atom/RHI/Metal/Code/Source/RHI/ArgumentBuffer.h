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

namespace AZ
{
    namespace Metal
    {
        class Device;
        class BufferMemoryAllocator;
        class ShaderResourceGroup;
        struct ShaderResourceGroupCompiledData;

        //! This class manages all the native objects associated with a ShaderResourceGroup
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
                      ShaderResourceGroupPool* srgPool);
            
            void Init(Device* device,
                      AZStd::vector<MTLArgumentDescriptor*> argBufferDescriptors,
                      AZStd::string argBufferName);
            
            void UpdateImageViews(const RHI::ShaderInputImageDescriptor& shaderInputImage,
                                  const AZStd::span<const RHI::ConstPtr<RHI::ImageView>>& imageViews);

            void UpdateSamplers(const RHI::ShaderInputSamplerDescriptor& shaderInputSampler,
                                const AZStd::span<const RHI::SamplerState>& samplerStates);

            void UpdateBufferViews(const RHI::ShaderInputBufferDescriptor& shaderInputBuffer,
                                   const AZStd::span<const RHI::ConstPtr<RHI::BufferView>>& bufferViews);

            void UpdateConstantBufferViews(AZStd::span<const uint8_t> rawData);

            //! Return the native MTLBuffer that holds SRG data
            id<MTLBuffer> GetArgEncoderBuffer() const;
            
            //! Return the native Argument buffer encoder is used to write into the native MTLBuffer
            const id<MTLArgumentEncoder> GetArgEncoder() const;
            
            //! Return the offset associated with the native MTLBuffer for this argument buffer
            size_t GetOffset() const;

            //Map to cache all the resources based on the usage as we can batch all the resources for a given usage.
            using ComputeResourcesToMakeResidentMap = AZStd::unordered_map<MTLResourceUsage, AZStd::unordered_set<id <MTLResource>>>;
            //Map to cache all the resources based on the usage and shader stage as we can batch all the resources for a given usage/shader usage.
            using GraphicsResourcesToMakeResidentMap = AZStd::unordered_map<AZStd::pair<MTLResourceUsage,MTLRenderStages>, AZStd::unordered_set<id <MTLResource>>>;

            //Cache all the resources bound to the SRG that needs to be made resident beofre the draw call
            void CollectUntrackedResources(const ShaderResourceGroupVisibility& srgResourcesVisInfo,
                                           ComputeResourcesToMakeResidentMap& resourcesToMakeResidentCompute,
                                           GraphicsResourcesToMakeResidentMap& resourcesToMakeResidentGraphics) const;

            bool IsNullHeapNeededForVertexStage(const ShaderResourceGroupVisibility& srgResourcesVisInfo) const;
            bool IsNullDescHeapNeeded() const;

            //! Update the texture related descriptor at a specific id index within the Argument buffer
            void UpdateTextureView(id <MTLTexture> mtlTexture, uint32_t index);
            
            //! Update the buffer related descriptor at a specific id index within the Argument buffer
            void UpdateBufferView(id <MTLBuffer> mtlBuffer, uint32_t offset, uint32_t index);
            
            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceObject
            void Shutdown() override;
            //////////////////////////////////////////////////////////////////////////

        private:

            bool CreateArgumentDescriptors(NSMutableArray * argBufferDecriptors);
            void AttachStaticSamplers();
            void AttachConstantBuffer();

            // Use a cache to store and retrieve samplers
            id<MTLSamplerState> GetMtlSampler(MTLSamplerDescriptor* samplerDesc);

            using ResourceBindingsSet = AZStd::unordered_set<ResourceBindingData>;
            using ResourceBindingsMap =  AZStd::unordered_map<AZ::Name, ResourceBindingsSet>;
            ResourceBindingsMap m_resourceBindings;

            static const int MaxEntriesInArgTable = 31;

            void CollectResourcesForCompute(const ResourceBindingsSet& resourceBindingData,
                                            ComputeResourcesToMakeResidentMap& resourcesToMakeResidentMap) const;
            void CollectResourcesForGraphics(RHI::ShaderStageMask visShaderMask,
                                             const ResourceBindingsSet& resourceBindingDataSet,
                                             GraphicsResourcesToMakeResidentMap& resourcesToMakeResidentMap) const;
    
            void BindNullSamplers(uint32_t registerId, uint32_t samplerCount);
            void SetArgumentBuffer(NSMutableArray* argBufferDecriptors, AZStd::string argBufferName);
            
            Device* m_device = nullptr;
            RHI::ConstPtr<RHI::ShaderResourceGroupLayout> m_srgLayout;

            id <MTLArgumentEncoder> m_argumentEncoder;
            uint32_t m_constantBufferSize = 0;

#if defined(ARGUMENTBUFFER_PAGEALLOCATOR)
            BufferMemoryView m_argumentBuffer;
            BufferMemoryView m_constantBuffer;
#else
            //We are keeping the non-page allocation implementation for GPU captures
            //GPU captures dont work well with argument buffers with offsets
            MemoryView m_argumentBuffer;
            MemoryView m_constantBuffer;
#endif
            bool m_useNullDescriptorHeap = false;
        };
    }
}
