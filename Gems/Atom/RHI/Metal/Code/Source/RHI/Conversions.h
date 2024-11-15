/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <Atom/RHI.Reflect/Format.h>
#include <Atom/RHI/DeviceBuffer.h>
#include <Atom/RHI/DeviceBufferView.h>
#include <Atom/RHI/DeviceImage.h>
#include <Atom/RHI/DeviceImageView.h>
#include <Atom/RHI/DevicePipelineState.h>
#include <Atom/RHI/DeviceDrawItem.h>
#include <Atom/RHI/DeviceBufferPool.h>
#include <Atom/RHI/DeviceQuery.h>
#include <Atom/RHI.Reflect/InputStreamLayout.h>
#include <Atom/RHI.Reflect/ShaderResourceGroupLayoutDescriptor.h>
#include <Metal/Metal.h>


#define METAL_MAX_ENTRIES_BUFFER_ARG_TABLE        31

namespace AZ
{
    namespace Metal
    {
        class Image;
        struct RasterizerState;
        
        struct ResourceDescriptor
        {
            const char* m_name = "";
            uint32_t m_width = 1;
            uint32_t m_height = 1;
            uint32_t m_depthOrArraySize = 1;
            uint16_t m_mipLevels = 1;
            uint16_t m_sampleCount = 1;
            MTLPixelFormat  m_mtlFormat = MTLPixelFormatInvalid;
            MTLTextureType  m_mtlTextureType = MTLTextureType2D;
            MTLTextureUsage m_mtlUsageFlags = MTLTextureUsageUnknown;
            
            MTLStorageMode m_mtlStorageMode = MTLStorageModeShared;
            MTLCPUCacheMode m_mtlCPUCacheMode = MTLCPUCacheModeDefaultCache;
            MTLHazardTrackingMode m_mtlHazardTrackingMode = MTLHazardTrackingModeTracked;
        };

        MTLPixelFormat ConvertPixelFormat(RHI::Format format);
        RHI::Format  ConvertPixelFormat(MTLPixelFormat format);
        MTLTextureUsage ConvertTextureUsageFlags(RHI::BufferBindFlags flags, RHI::Format format);
        MTLTextureType ConvertTextureType(RHI::ImageDimension dimension, int arraySize, bool isCubeMap, bool isViewArray=false);
        MTLPixelFormat ConvertImageViewFormat( const Image& image, const RHI::ImageViewDescriptor& imageViewDescriptor);
        MTLBlendOperation ConvertBlendOp(RHI::BlendOp op);
        MTLBlendFactor ConvertBlendFactor(RHI::BlendFactor factor);
        MTLVertexFormat ConvertVertexFormat(RHI::Format format);
        MTLColorWriteMask ConvertColorWriteMask(AZ::u8 writeMask);
        MTLSamplerMinMagFilter ConvertFilterMode(RHI::FilterMode mode);
        MTLSamplerMipFilter ConvertMipFilterMode(RHI::FilterMode mode);
        MTLSamplerAddressMode ConvertAddressMode(RHI::AddressMode addressMode);
#if AZ_TRAIT_ATOM_METAL_SAMPLER_BORDERCOLOR_SUPPORT
        MTLSamplerBorderColor ConvertBorderColor(RHI::BorderColor color);
#endif
        MTLCompareFunction ConvertComparisonFunc(RHI::ComparisonFunc func);
        MTLPrimitiveType ConvertPrimitiveTopology(RHI::PrimitiveTopology primTopology);
        MTLStencilOperation ConvertStencilOp(RHI::StencilOp op);
        MTLCullMode ConvertCullMode(RHI::CullMode mode);
        MTLTriangleFillMode ConvertFillMode(RHI::FillMode mode);

        MTLStorageMode ConvertTextureStorageMode(RHI::ImageBindFlags imageFlags, const bool isMsaa);
        MTLCPUCacheMode ConvertTextureCPUCacheMode();
        MTLHazardTrackingMode ConvertTextureHazardTrackingMode();

        MTLStorageMode ConvertBufferStorageMode(const RHI::BufferDescriptor& descriptor, RHI::HeapMemoryLevel heapMemoryLevel);
        MTLCPUCacheMode ConvertBufferCPUCacheMode();
        MTLHazardTrackingMode ConvertBufferHazardTrackingMode();

        MTLResourceOptions CovertStorageMode(MTLStorageMode storageMode);
        MTLResourceOptions CovertHazardTrackingMode(MTLHazardTrackingMode hazardTrackingMode);
        MTLResourceOptions CovertCPUCahceMode(MTLCPUCacheMode cpuCahceMode);
        MTLResourceOptions CovertToResourceOptions(MTLStorageMode storageMode, MTLCPUCacheMode cpuCahceMode, MTLHazardTrackingMode hazardTrackingMode);
        MTLStorageMode GetCPUGPUMemoryMode();
        MTLBindingAccess GetBindingAccess(RHI::ShaderInputImageAccess accessType);
        MTLBindingAccess GetBindingAccess(RHI::ShaderInputBufferAccess accessType);
    
        void ConvertSamplerState(const RHI::SamplerState& state, MTLSamplerDescriptor* samplerDesc);
        void ConvertDepthStencilState(const RHI::DepthStencilState& depthStencil, MTLDepthStencilDescriptor* mtlDepthStencilDesc);
        void ConvertStencilState(const RHI::StencilState& stencilState, MTLDepthStencilDescriptor* mtlDepthStencilDesc);
        void ConvertInputElements(const RHI::InputStreamLayout& layout, MTLVertexDescriptor * vertDesc);
        void ConvertRasterState(const RHI::RasterState& raster, RasterizerState& rasterizerState);
        void ConvertImageArgumentDescriptor(MTLArgumentDescriptor* imgArgDescriptor, const RHI::ShaderInputImageDescriptor& shaderInputImage);
        void ConvertBufferArgumentDescriptor(MTLArgumentDescriptor* bufferArgDescriptor, const RHI::ShaderInputBufferDescriptor& shaderInputBuffer);
        
        bool IsDepthStencilMerged(RHI::Format format);
        bool IsDepthStencilMerged(MTLPixelFormat mtlFormat);
        bool GetVertexFormatSizeInBytes(const MTLVertexFormat vertexFormat, uint32_t& size);
        bool GetIndexTypeSizeInBytes(const MTLIndexType indexType, uint32_t& size);
        bool IsTextureTypeAnArray(MTLTextureType textureType);
        uint32_t GetArrayLength(int arraySize, bool isCubeMap);
        
        ResourceDescriptor ConvertBufferDescriptor(const RHI::BufferDescriptor& descriptor, RHI::HeapMemoryLevel heapMemoryLevel = RHI::HeapMemoryLevel::Device);
        ResourceDescriptor ConvertImageResourceDescriptor(const RHI::ImageDescriptor& descriptor);
        RHI::ShaderInputImageAccess GetImageAccess(RHI::ShaderInputBufferAccess bufferAccess);
        MTLTextureDescriptor* ConvertImageDescriptor(const RHI::ImageDescriptor& descriptor);
        MTLSamplePosition ConvertSampleLocation(const RHI::SamplePosition& position);
        MTLVisibilityResultMode ConvertVisibilityResult(RHI::QueryControlFlags flags);
        MTLBlitOption GetBlitOption(RHI::Format format, RHI::ImageAspect imageAspect);
        MTLResourceUsage GetImageResourceUsage(RHI::ShaderInputImageAccess imageAccess);
        MTLResourceUsage GetBufferResourceUsage(RHI::ShaderInputBufferAccess bufferAccess);
        MTLRenderStages GetRenderStages(RHI::ShaderStageMask shaderMask);
    
        //Use https://developer.apple.com/metal/Metal-Feature-Set-Tables.pdf for these functions
        bool IsFilteringSupported(id<MTLDevice> mtlDevice, RHI::Format format);
        bool IsWriteSupported(id<MTLDevice> mtlDevice, RHI::Format format);
        bool IsColorRenderTargetSupported(id<MTLDevice> mtlDevice, RHI::Format format);
        bool IsBlendingSupported(id<MTLDevice> mtlDevice, RHI::Format format);
        bool IsMSAASupported(id<MTLDevice> mtlDevice, RHI::Format format);
        bool IsResolveTargetSupported(id<MTLDevice> mtlDevice, RHI::Format format);
        bool IsDepthStencilSupported(id<MTLDevice> mtlDevice, RHI::Format format);
        bool IsCompressedFormat(RHI::Format format);
    }
}
