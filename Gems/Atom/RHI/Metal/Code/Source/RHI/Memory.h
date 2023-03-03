/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <Atom/RHI/Object.h>
#include <metal/metal.h>

namespace AZ
{
    namespace Metal
    {
        enum class ResourceType
        {
            MtlUndefined = 0,
            MtlTextureType,
            MtlBufferType,            
        };
        
        struct MetalResourceDescriptor
        {
            void* m_resourcePtr = nullptr;
            ResourceType m_resourceType = ResourceType::MtlUndefined;
            bool m_isSwapChainResource = false;
        };
    
        class MetalResource final
            : public RHI::Object
        {
        public:
            
            AZ_CLASS_ALLOCATOR(MetalResource, AZ::SystemAllocator);
            AZ_RTTI(MetalResource, "{ED5953FB-6B4B-4A3B-9566-7561EC284687}", RHI::Object);

            static RHI::Ptr<MetalResource> Create(const MetalResourceDescriptor& metalResourceDescriptor)
            {
                RHI::Ptr<MetalResource> resource = aznew MetalResource();
                resource->m_resourcePtr = metalResourceDescriptor.m_resourcePtr;
                resource->m_resourceType = metalResourceDescriptor.m_resourceType;
                resource->m_isSwapChainResource = metalResourceDescriptor.m_isSwapChainResource;
                return resource;
            }
            
            ~MetalResource()
            {
                switch(m_resourceType)
                {
                    case ResourceType::MtlTextureType:
                    {
                        id<MTLTexture> mtlTexture = GetGpuAddress<id<MTLTexture>>();
                        //Swapchain textures are not created by the engine and are owned by the drivers hence we cant release them
                        if(mtlTexture && !m_isSwapChainResource)
                        {
                            [mtlTexture release];
                            mtlTexture = nil;
                        }
                        break;
                    }
                    case ResourceType::MtlBufferType:
                    {
                        id<MTLBuffer> mtlBuffer = GetGpuAddress<id<MTLBuffer>>();
                        if (mtlBuffer)
                        {
                            [mtlBuffer release];
                            mtlBuffer = nil;
                        }
                        break;
                    }
                    default:
                    {
                        AZ_Assert(false, "Undefined Resource type");
                    }
                }
                m_resourcePtr = nullptr;
            }
            
            ResourceType GetResourceType() const
            {
                return m_resourceType;
            }
            
            void* GetCpuAddress() const
            {
                AZ_Assert(m_resourcePtr, "m_resourcePtr can not be null");
                if(m_resourceType == ResourceType::MtlBufferType)
                {
                    id<MTLBuffer> mtlBuffer = static_cast<id<MTLBuffer>>(m_resourcePtr);
                    return mtlBuffer.contents;
                }
                AZ_Error("MetalResource", false, "Resource Type is incorrect");
                return nullptr;
            }            
            
            template <typename T>
            T GetGpuAddress() const
            {
                AZ_Assert(m_resourcePtr, "m_resourcePtr can not be null");
                return static_cast<T>(m_resourcePtr);
            }
            
            uint64_t GetHash() const
            {
                switch(m_resourceType)
                {
                    case ResourceType::MtlTextureType:
                    {
                        id<MTLTexture> mtlTexture = GetGpuAddress<id<MTLTexture>>();
                        return mtlTexture.hash;
                    }
                    case ResourceType::MtlBufferType:
                    {
                        id<MTLBuffer> mtlBuffer = GetGpuAddress<id<MTLBuffer>>();
                        return mtlBuffer.hash;
                    }
                    default:
                    {
                        AZ_Assert(false, "Undefined Resource type");
                        return 0;
                    }
                }
            }
            
            //! This function is setup for swapchain texture to override the native pointer as for metal we
            //! get the swapchain texture from the drivers at the end of every frame by requesting the nextdrawable from the CAMetalLayer.
            void OverrideResource(id<MTLTexture> mtlTexture)
            {
                AZ_Assert(m_isSwapChainResource, "Only the swapchain texture should be overriden due to the way swapchain works for metal");
                if(m_isSwapChainResource)
                {
                    m_resourcePtr = static_cast<void*>(mtlTexture);
                }
            }

        private:
            MetalResource() = default;
            void* m_resourcePtr = nullptr;
            ResourceType m_resourceType = ResourceType::MtlUndefined;
            bool m_isSwapChainResource = false;
        };
        
        using Memory = MetalResource;
    }
}
