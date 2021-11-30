/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/ImageDescriptor.h>
#include <Atom/RHI.Reflect/Bits.h>
#include <RHI/Conversions.h>
#include <RHI/Conversions_Platform.h>
#include <RHI/Image.h>
#include <RHI/PipelineState.h>

namespace AZ
{
    namespace Metal
    {
        namespace Platform
        {
            MTLPixelFormat ConvertPixelFormat(RHI::Format format);
            MTLBlitOption GetBlitOption(RHI::Format format);
            MTLSamplerAddressMode ConvertAddressMode(RHI::AddressMode addressMode);
            MTLResourceOptions CovertStorageMode(MTLStorageMode storageMode);
            MTLStorageMode GetCPUGPUMemoryMode();
            bool IsDepthStencilMerged(MTLPixelFormat mtlFormat);
            bool IsFilteringSupported(id<MTLDevice> mtlDevice, RHI::Format format);
            bool IsWriteSupported(id<MTLDevice> mtlDevice, RHI::Format format);
            bool IsColorRenderTargetSupported(id<MTLDevice> mtlDevice, RHI::Format format);
            bool IsBlendingSupported(id<MTLDevice> mtlDevice, RHI::Format format);
            bool IsMSAASupported(id<MTLDevice> mtlDevice, RHI::Format format);
            bool IsResolveTargetSupported(id<MTLDevice> mtlDevice, RHI::Format format);
            bool IsDepthStencilSupported(id<MTLDevice> mtlDevice, RHI::Format format);
        }
    
        MTLPixelFormat ConvertPixelFormat(RHI::Format format)
        {
            switch (format)
            {
            case RHI::Format::Unknown:
                return MTLPixelFormatInvalid;
            case RHI::Format::R32G32B32A32_FLOAT:
                return MTLPixelFormatRGBA32Float;
            case RHI::Format::R32G32B32A32_UINT:
                return MTLPixelFormatRGBA32Uint;
            case RHI::Format::R32G32B32A32_SINT:
                return MTLPixelFormatRGBA32Sint;
            case RHI::Format::R16G16B16A16_FLOAT:
                return MTLPixelFormatRGBA16Float;
            case RHI::Format::R16G16B16A16_UNORM:
                return MTLPixelFormatRGBA16Unorm;
            case RHI::Format::R16G16B16A16_UINT:
                return MTLPixelFormatRGBA16Uint;
            case RHI::Format::R16G16B16A16_SNORM:
                return MTLPixelFormatRGBA16Snorm;
            case RHI::Format::R16G16B16A16_SINT:
                return MTLPixelFormatRGBA16Sint;
            case RHI::Format::R32G32_FLOAT:
                return MTLPixelFormatRG32Float;
            case RHI::Format::R32G32_UINT:
                return MTLPixelFormatRG32Uint;
            case RHI::Format::R32G32_SINT:
                return MTLPixelFormatRG32Sint;
            case RHI::Format::D32_FLOAT_S8X24_UINT:
                return MTLPixelFormatDepth32Float_Stencil8;
            case RHI::Format::R10G10B10A2_UNORM:
                return MTLPixelFormatRGB10A2Unorm;
            case RHI::Format::R10G10B10A2_UINT:
                return MTLPixelFormatRGB10A2Uint;
            case RHI::Format::R11G11B10_FLOAT:
                return MTLPixelFormatRG11B10Float;
            case RHI::Format::R8G8B8A8_UNORM:
                return MTLPixelFormatRGBA8Unorm;
            case RHI::Format::R8G8B8A8_UNORM_SRGB:
                return MTLPixelFormatRGBA8Unorm_sRGB;
            case RHI::Format::R8G8B8A8_UINT:
                return MTLPixelFormatRGBA8Uint;
            case RHI::Format::R8G8B8A8_SNORM:
                return MTLPixelFormatRGBA8Snorm;
            case RHI::Format::R8G8B8A8_SINT:
                return MTLPixelFormatRGBA8Sint;
            case RHI::Format::R16G16_FLOAT:
                return MTLPixelFormatRG16Float;
            case RHI::Format::R16G16_UNORM:
                return MTLPixelFormatRG16Unorm;
            case RHI::Format::R16G16_UINT:
                return MTLPixelFormatRG16Uint;
            case RHI::Format::R16G16_SNORM:
                return MTLPixelFormatRG16Snorm;
            case RHI::Format::R16G16_SINT:
                return MTLPixelFormatRG16Sint;
            case RHI::Format::D32_FLOAT:
                return MTLPixelFormatDepth32Float;
            case RHI::Format::R32_FLOAT:
                return MTLPixelFormatR32Float;
            case RHI::Format::R32_UINT:
                return MTLPixelFormatR32Uint;
            case RHI::Format::R32_SINT:
                return MTLPixelFormatR32Sint;
            case RHI::Format::R8G8_UNORM:
                return MTLPixelFormatRG8Unorm;
            case RHI::Format::R8G8_UINT:
                return MTLPixelFormatRG8Uint;
            case RHI::Format::R8G8_SNORM:
                return MTLPixelFormatRG8Snorm;
            case RHI::Format::R8G8_SINT:
                return MTLPixelFormatRG8Sint;
            case RHI::Format::R16_FLOAT:
                return MTLPixelFormatR16Float;
            case RHI::Format::R16_UNORM:
                return MTLPixelFormatR16Unorm;
            case RHI::Format::R16_UINT:
                return MTLPixelFormatR16Uint;
            case RHI::Format::R16_SNORM:
                return MTLPixelFormatR16Snorm;
            case RHI::Format::R16_SINT:
                return MTLPixelFormatR16Sint;
            case RHI::Format::R8_UNORM:
                return MTLPixelFormatR8Unorm;
            case RHI::Format::R8_UINT:
                return MTLPixelFormatR8Uint;
            case RHI::Format::R8_SNORM:
                return MTLPixelFormatR8Snorm;
            case RHI::Format::R8_SINT:
                return MTLPixelFormatR8Sint;
            case RHI::Format::A8_UNORM:
                return MTLPixelFormatA8Unorm;
            case RHI::Format::R9G9B9E5_SHAREDEXP:
                return MTLPixelFormatRGB9E5Float;
            case RHI::Format::B8G8R8A8_UNORM:
                return MTLPixelFormatBGRA8Unorm;
            case RHI::Format::B8G8R8X8_UNORM:
                return MTLPixelFormatBGRA8Unorm;
            case RHI::Format::B8G8R8A8_UNORM_SRGB:
                return MTLPixelFormatBGRA8Unorm_sRGB;
            default:
                //Check for platform specific formats
                return Platform::ConvertPixelFormat(format);
            }
            
        }
        
        MTLStorageMode ConvertTextureStorageMode(RHI::ImageBindFlags imageFlags, const bool isMsaa)
        {
            MTLStorageMode storageModeFlags = GetCPUGPUMemoryMode();

            if (RHI::CheckBitsAll(imageFlags, RHI::ImageBindFlags::DepthStencil) || isMsaa)
            {
                storageModeFlags = MTLStorageModePrivate;
            }
            
            if (RHI::CheckBitsAll(imageFlags, RHI::ImageBindFlags::ShaderRead))
            {
                storageModeFlags = MTLStorageModePrivate;
            }
            
            return storageModeFlags;
        }
        
        MTLCPUCacheMode ConvertTextureCPUCacheMode()
        {
            return MTLCPUCacheModeDefaultCache;
        }
    
        MTLHazardTrackingMode ConvertTextureHazardTrackingMode()
        {
            return MTLHazardTrackingModeTracked;
        }
    
        MTLStorageMode ConvertBufferStorageMode(const RHI::BufferDescriptor& descriptor, RHI::HeapMemoryLevel heapMemoryLevel)
        {
            //! Apple guidelines 
            //! Use MTLStorageModePrivate - If a buffer is Accessed exclusively by the GPU
            //! Use MTLStorageModeShared - If a buffer changes frequently, is relatively small, and is accessed by both the CPU and the GPU. This is slow memory.
            //! Use MTLStorageModeManaged (Mac only) - If a buffer is populated once by the CPU and accessed frequently by the GPU.
            //! Use MTLStorageModeManaged (Mac only) - If a buffer changes frequently , is relatively large, and is accessed by both the CPU and the GPU. 

            if (RHI::CheckBitsAny(descriptor.m_bindFlags, RHI::BufferBindFlags::Constant))
            {
                return MTLStorageModeShared;
            }
            
            //Only HeapMemoryLevel::Device contains a BufferPoolresolver to transfer memory from staging to GPU
            if(heapMemoryLevel == RHI::HeapMemoryLevel::Device)
            {
                if (RHI::CheckBitsAny(descriptor.m_bindFlags, RHI::BufferBindFlags::ShaderWrite))
                {
                    return MTLStorageModePrivate;
                }
                
                if (RHI::CheckBitsAny(descriptor.m_bindFlags, RHI::BufferBindFlags::CopyWrite))
                {
                    return MTLStorageModePrivate;
                }
                
                if (RHI::CheckBitsAny(descriptor.m_bindFlags, RHI::BufferBindFlags::Indirect))
                {
                    return MTLStorageModePrivate;
                }
            }
            
            if (RHI::CheckBitsAll(descriptor.m_bindFlags, RHI::BufferBindFlags::InputAssembly))
            {
                return GetCPUGPUMemoryMode();
            }
            
            //This flag is used for IA buffers that is updated frequently and hence shared mmory is the best fit
            if (RHI::CheckBitsAll(descriptor.m_bindFlags, RHI::BufferBindFlags::DynamicInputAssembly))
            {
                return MTLStorageModeShared;
            }
                 
            return GetCPUGPUMemoryMode();
        }
    
        MTLCPUCacheMode ConvertBufferCPUCacheMode()
        {
            return MTLCPUCacheModeDefaultCache;
        }
    
        MTLHazardTrackingMode ConvertBufferHazardTrackingMode()
        {
            return MTLHazardTrackingModeTracked;
        }
    
        MTLTextureUsage ConvertTextureUsageFlags(RHI::ImageBindFlags imageFlags, RHI::Format format)
        {
            MTLTextureUsage usageFlags = MTLTextureUsageUnknown;
            //Enables using this texture as a color, depth, or stencil render target in a render pass descriptor
            if (RHI::CheckBitsAll(imageFlags, RHI::ImageBindFlags::Color))
            {
                usageFlags |= MTLTextureUsageRenderTarget;
            }
            if (RHI::CheckBitsAll(imageFlags, RHI::ImageBindFlags::DepthStencil))
            {
                usageFlags |= MTLTextureUsageRenderTarget;
            }
            
            //Enables loading or sampling from the texture in any shader stage.
            if (RHI::CheckBitsAll(imageFlags, RHI::ImageBindFlags::ShaderRead))
            {
                usageFlags |= MTLTextureUsageShaderRead;
            }
            
            //Enables writing to the texture from compute shaders
            if (RHI::CheckBitsAll(imageFlags, RHI::ImageBindFlags::ShaderReadWrite))
            {
                usageFlags |= MTLTextureUsageShaderRead;
                usageFlags |= MTLTextureUsageShaderWrite;
            }
            
            //There is currently no way of knowing that the texture view for this texture will have a different pixel format.
            //Hence we assume every texture will use a texture view different from its own even though it may or may not.
            //Need to check performance impact for this assumption.
            usageFlags |= MTLTextureUsagePixelFormatView;
            
            return usageFlags;
        }
        
        
        MTLTextureType ConvertTextureType(RHI::ImageDimension dimension, int arraySize, bool isCubeMap)
        {
            if(isCubeMap)
            {
                AZ_Assert(arraySize % RHI::ImageDescriptor::NumCubeMapSlices == 0, "Incorrect array layers for Cube or CubeArray.");
                int numCubeMaps = arraySize / RHI::ImageDescriptor::NumCubeMapSlices;
                if(numCubeMaps>1)
                {
                    return MTLTextureTypeCubeArray;
                }
                else
                {
                    return MTLTextureTypeCube;
                }
            }
            switch (dimension)
            {
                case RHI::ImageDimension::Image1D:
                {
                    if(arraySize>1)
                    {
                        return MTLTextureType1DArray;
                    }
                    else
                    {
                        return MTLTextureType1D;
                    }
                }
                case RHI::ImageDimension::Image2D:
                {
                    if(arraySize>1)
                    {
                        return MTLTextureType2DArray;
                    }
                    else
                    {
                        return MTLTextureType2D;
                    }
                }
                case RHI::ImageDimension::Image3D:
                {
                    AZ_Assert(arraySize == 1, "3d textures cant have an array size of more than 1");
                    return MTLTextureType3D;
                }
                default:
                {
                    AZ_Assert(false, "failed to convert image type");
                    return MTLTextureType2D;
                }
            }
        }
        
        uint32_t GetArrayLength(int arraySize, bool isCubeMap)
        {            
            if(arraySize>1)
            {
                if(isCubeMap)
                {
                    return arraySize/RHI::ImageDescriptor::NumCubeMapSlices;
                }
                else
                {
                    return arraySize;
                }
            }
            
            AZ_Assert(arraySize==1, "If the texture type is not an array, this value should be 1.");
            return arraySize;
        }
            
        ResourceDescriptor ConvertImageResourceDescriptor(const RHI::ImageDescriptor& descriptor)
        {
            ResourceDescriptor resourceDesc;
            resourceDesc.m_width = descriptor.m_size.m_width;
            resourceDesc.m_height = descriptor.m_size.m_height;
            resourceDesc.m_depthOrArraySize = descriptor.m_dimension == RHI::ImageDimension::Image3D ? descriptor.m_size.m_depth : GetArrayLength(descriptor.m_arraySize, descriptor.m_isCubemap);
            resourceDesc.m_mipLevels = descriptor.m_mipLevels;
            resourceDesc.m_mtlFormat = ConvertPixelFormat(descriptor.m_format);
            resourceDesc.m_sampleCount = descriptor.m_multisampleState.m_samples;
            const bool isMsaa = descriptor.m_multisampleState.m_samples>1;
            if(isMsaa)
            {
                resourceDesc.m_mtlTextureType = MTLTextureType2DMultisample;
            }
            else
            {
                AZ_Assert(resourceDesc.m_sampleCount==1, "Non-MSAA testures should have a sample count of 1");
                resourceDesc.m_mtlTextureType = ConvertTextureType(descriptor.m_dimension, descriptor.m_arraySize, descriptor.m_isCubemap);
            }
            resourceDesc.m_mtlUsageFlags = ConvertTextureUsageFlags(descriptor.m_bindFlags, descriptor.m_format);
            
            
            resourceDesc.m_mtlStorageMode = ConvertTextureStorageMode(descriptor.m_bindFlags, isMsaa);
            resourceDesc.m_mtlCPUCacheMode = ConvertTextureCPUCacheMode();
            resourceDesc.m_mtlHazardTrackingMode = ConvertTextureHazardTrackingMode();
            
            return resourceDesc;
        }
        
        MTLTextureDescriptor* ConvertImageDescriptor(const RHI::ImageDescriptor& descriptor)
        {
            ResourceDescriptor resourceDescriptor = ConvertImageResourceDescriptor(descriptor);
            
            AZ_Assert(resourceDescriptor.m_mtlFormat != MTLPixelFormatInvalid, "Invalid Texture format");
            MTLTextureDescriptor* mtlTextureDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat : resourceDescriptor.m_mtlFormat
                                                                                            width : resourceDescriptor.m_width
                                                                                           height : resourceDescriptor.m_height
                                                                                        mipmapped : (resourceDescriptor.m_mipLevels > 1)];
            mtlTextureDesc.textureType = resourceDescriptor.m_mtlTextureType;
            if(resourceDescriptor.m_mtlTextureType == MTLTextureType3D)
            {
                mtlTextureDesc.depth = resourceDescriptor.m_depthOrArraySize;
            }
            else
            {
                mtlTextureDesc.arrayLength = resourceDescriptor.m_depthOrArraySize;
            }
            mtlTextureDesc.mipmapLevelCount = resourceDescriptor.m_mipLevels;
            mtlTextureDesc.usage = resourceDescriptor.m_mtlUsageFlags;
            mtlTextureDesc.storageMode = resourceDescriptor.m_mtlStorageMode;
            mtlTextureDesc.cpuCacheMode = resourceDescriptor.m_mtlCPUCacheMode;
            mtlTextureDesc.hazardTrackingMode = resourceDescriptor.m_mtlHazardTrackingMode;
            
            mtlTextureDesc.sampleCount = resourceDescriptor.m_sampleCount;
            return mtlTextureDesc;
        }
    
        
        ResourceDescriptor ConvertBufferDescriptor(const RHI::BufferDescriptor& descriptor, RHI::HeapMemoryLevel heapMemoryLevel)
        {
            ResourceDescriptor resourceDesc;
            resourceDesc.m_width = descriptor.m_byteCount;
            resourceDesc.m_mtlStorageMode = ConvertBufferStorageMode(descriptor, heapMemoryLevel);
            resourceDesc.m_mtlCPUCacheMode = ConvertBufferCPUCacheMode();
            resourceDesc.m_mtlHazardTrackingMode = ConvertBufferHazardTrackingMode();
            return resourceDesc;
        }
        
        MTLPixelFormat ConvertImageViewFormat(const Image& image, const RHI::ImageViewDescriptor& imageViewDescriptor)
        {
            /**
             * The format of the image is pulled from the base image or the view depending
             * on if the user overrides it in the view. It is legal for a view to have an unknown
             * format, which just means we're falling back to the image format.
             */
            return imageViewDescriptor.m_overrideFormat != RHI::Format::Unknown ?
            ConvertPixelFormat(imageViewDescriptor.m_overrideFormat) :
            ConvertPixelFormat(image.GetDescriptor().m_format);
        }
        
        MTLBlendOperation ConvertBlendOp(RHI::BlendOp op)
        {
            static const MTLBlendOperation table[] =
            {
                MTLBlendOperationAdd,
                MTLBlendOperationSubtract,
                MTLBlendOperationReverseSubtract,
                MTLBlendOperationMin,
                MTLBlendOperationMax
            };
            
            return table[(uint32_t)op];
        }
        
        MTLBlendFactor ConvertBlendFactor(RHI::BlendFactor factor)
        {
            static const MTLBlendFactor table[] =
            {
                MTLBlendFactorZero,
                MTLBlendFactorOne,
                MTLBlendFactorSourceColor,
                MTLBlendFactorOneMinusSourceColor,
                MTLBlendFactorSourceAlpha,
                MTLBlendFactorOneMinusSourceAlpha,
                MTLBlendFactorDestinationAlpha,
                MTLBlendFactorOneMinusDestinationAlpha,
                MTLBlendFactorDestinationColor,
                MTLBlendFactorOneMinusDestinationColor,
                MTLBlendFactorSourceAlphaSaturated,
                MTLBlendFactorBlendColor,  //todo: could be MTLBlendFactorBlendAlpha
                MTLBlendFactorOneMinusBlendColor, //todo: could be MTLBlendFactorOneMinusBlendAlpha
                MTLBlendFactorSource1Color,
                MTLBlendFactorOneMinusSource1Color,
                MTLBlendFactorSource1Alpha,
                MTLBlendFactorOneMinusSource1Alpha
            };
            return table[(uint32_t)factor];
        }

        MTLColorWriteMask ConvertColorWriteMask(AZ::u8 writeMask)
        {
            MTLColorWriteMask colorMask = MTLColorWriteMaskNone;
            if(writeMask == 0)
            {
                return colorMask;
            }
            
            if(RHI::CheckBitsAll(writeMask, static_cast<uint8_t>(RHI::WriteChannelMask::ColorWriteMaskAll)))
            {
                return MTLColorWriteMaskAll;
            }
                        
            if (RHI::CheckBitsAny(writeMask, static_cast<uint8_t>(RHI::WriteChannelMask::ColorWriteMaskRed)))
            {
                colorMask |= MTLColorWriteMaskRed;
            }
            if (RHI::CheckBitsAny(writeMask, static_cast<uint8_t>(RHI::WriteChannelMask::ColorWriteMaskGreen)))
            {
                colorMask |= MTLColorWriteMaskGreen;
            }
            if (RHI::CheckBitsAny(writeMask, static_cast<uint8_t>(RHI::WriteChannelMask::ColorWriteMaskBlue)))
            {
                colorMask |= MTLColorWriteMaskBlue;
            }
            if (RHI::CheckBitsAny(writeMask, static_cast<uint8_t>(RHI::WriteChannelMask::ColorWriteMaskAlpha)))
            {
                colorMask |= MTLColorWriteMaskAlpha;
            }
            
            return colorMask;
        }
        
        MTLVertexFormat ConvertVertexFormat(RHI::Format format)
        {
            switch (format)
            {
                case RHI::Format::R32G32B32A32_FLOAT:
                    return MTLVertexFormatFloat4;
                case RHI::Format::R32G32B32A32_UINT:
                    return MTLVertexFormatUInt4;
                case RHI::Format::R32G32B32A32_SINT:
                    return MTLVertexFormatInt4;
                case RHI::Format::R32G32B32_FLOAT:
                    return MTLVertexFormatFloat3;
                case RHI::Format::R32G32B32_UINT:
                    return MTLVertexFormatUInt3;
                case RHI::Format::R32G32B32_SINT:
                    return MTLVertexFormatInt3;
                case RHI::Format::R16G16B16A16_FLOAT:
                    return MTLVertexFormatHalf4;
                case RHI::Format::R16G16B16A16_UNORM:
                    return MTLVertexFormatUShort4Normalized;
                case RHI::Format::R16G16B16A16_UINT:
                    return MTLVertexFormatUShort4;
                case RHI::Format::R16G16B16A16_SNORM:
                    return MTLVertexFormatShort4Normalized;
                case RHI::Format::R16G16B16A16_SINT:
                    return MTLVertexFormatShort4;
                case RHI::Format::R32G32_FLOAT:
                    return MTLVertexFormatFloat2;
                case RHI::Format::R32G32_UINT:
                    return MTLVertexFormatUInt2;
                case RHI::Format::R32G32_SINT:
                    return MTLVertexFormatInt2;
                case RHI::Format::R10G10B10A2_UNORM:
                    return MTLVertexFormatUInt1010102Normalized;
                case RHI::Format::R8G8B8A8_UNORM:
                    return MTLVertexFormatUChar4Normalized;
                case RHI::Format::R8G8B8A8_UNORM_SRGB:
                    return MTLVertexFormatUChar4Normalized;
                case RHI::Format::R8G8B8A8_UINT:
                    return MTLVertexFormatUChar4;
                case RHI::Format::R8G8B8A8_SNORM:
                    return MTLVertexFormatChar4Normalized;
                case RHI::Format::R8G8B8A8_SINT:
                    return MTLVertexFormatChar4;
                case RHI::Format::R16G16_FLOAT:
                    return MTLVertexFormatHalf2;
                case RHI::Format::R16G16_UNORM:
                    return MTLVertexFormatUShort2Normalized;
                case RHI::Format::R16G16_UINT:
                    return MTLVertexFormatUShort2;
                case RHI::Format::R16G16_SNORM:
                    return MTLVertexFormatShort2Normalized;
                case RHI::Format::R16G16_SINT:
                    return MTLVertexFormatShort2;
                case RHI::Format::R32_FLOAT:
                    return MTLVertexFormatFloat;
                case RHI::Format::R32_UINT:
                    return MTLVertexFormatUInt;
                case RHI::Format::R32_SINT:
                    return MTLVertexFormatInt;
                case RHI::Format::D24_UNORM_S8_UINT:
                    return MTLVertexFormatUInt;
                case RHI::Format::R8G8_UNORM:
                    return MTLVertexFormatUChar2Normalized;
                case RHI::Format::R8G8_UINT:
                    return MTLVertexFormatUChar2;
                case RHI::Format::R8G8_SNORM:
                    return MTLVertexFormatChar2Normalized;
                case RHI::Format::R8G8_SINT:
                    return MTLVertexFormatChar2;
                case RHI::Format::R16_FLOAT:
                    return MTLVertexFormatHalf;
                case RHI::Format::R16_UNORM:
                    return MTLVertexFormatUShortNormalized;
                case RHI::Format::R16_UINT:
                    return MTLVertexFormatUShort;
                case RHI::Format::R16_SNORM:
                    return MTLVertexFormatShortNormalized;
                case RHI::Format::R16_SINT:
                    return MTLVertexFormatShort;
                case RHI::Format::R8_UNORM:
                    return MTLVertexFormatUCharNormalized;
                case RHI::Format::R8_UINT:
                    return MTLVertexFormatUChar;
                case RHI::Format::R8_SNORM:
                    return MTLVertexFormatCharNormalized;
                case RHI::Format::R8_SINT:
                    return MTLVertexFormatChar;
                case RHI::Format::A8_UNORM:
                    return MTLVertexFormatUCharNormalized;
                case RHI::Format::B8G8R8A8_UNORM:
                    return MTLVertexFormatUChar4Normalized;
                case RHI::Format::B8G8R8X8_UNORM:
                    return MTLVertexFormatUChar4Normalized;
                case RHI::Format::B8G8R8A8_UNORM_SRGB:
                    return MTLVertexFormatUChar4Normalized;
                default:
                    AZ_Assert(false, "unhandled conversion in ConvertVertexFormat");
                    return MTLVertexFormatInvalid;
            }
        }
        
        bool GetIndexTypeSizeInBytes(const MTLIndexType indexType, uint32_t& size)
        {
            switch (indexType)
            {
                case MTLIndexTypeUInt16:
                {
                    size = 2;
                    break;
                }
                case MTLIndexTypeUInt32:
                {
                    size = 4;
                    break;
                }
                default:
                {
                    AZ_Assert(false, "Unknown metal index type");
                    return false;
                }
            }
            return true;
        }
        
        bool GetVertexFormatSizeInBytes(const MTLVertexFormat vertexFormat, uint32_t& size)
        {
            switch (vertexFormat)
            {
                case MTLVertexFormatInvalid:
                    return false;
                    
                case MTLVertexFormatUChar2:
                case MTLVertexFormatChar2:
                case MTLVertexFormatUChar2Normalized:
                case MTLVertexFormatChar2Normalized:
                    size = 2;
                    break;
                    
                case MTLVertexFormatUChar3:
                case MTLVertexFormatChar3:
                case MTLVertexFormatUChar3Normalized:
                case MTLVertexFormatChar3Normalized:
                    size = 3;
                    break;
                    
                case MTLVertexFormatUChar4:
                case MTLVertexFormatChar4:
                case MTLVertexFormatUChar4Normalized:
                case MTLVertexFormatChar4Normalized:
                case MTLVertexFormatUShort2:
                case MTLVertexFormatShort2:
                case MTLVertexFormatUShort2Normalized:
                case MTLVertexFormatShort2Normalized:
                case MTLVertexFormatHalf2:
                case MTLVertexFormatFloat:
                case MTLVertexFormatInt:
                case MTLVertexFormatUInt:
                case MTLVertexFormatInt1010102Normalized:
                case MTLVertexFormatUInt1010102Normalized:
                    size = 4;
                    break;
                    
                case MTLVertexFormatUShort3:
                case MTLVertexFormatShort3:
                case MTLVertexFormatUShort3Normalized:
                case MTLVertexFormatShort3Normalized:
                case MTLVertexFormatHalf3:
                    size = 6;
                    break;
                    
                case MTLVertexFormatUShort4:
                case MTLVertexFormatShort4:
                case MTLVertexFormatUShort4Normalized:
                case MTLVertexFormatShort4Normalized:
                case MTLVertexFormatHalf4:
                case MTLVertexFormatFloat2:
                case MTLVertexFormatInt2:
                case MTLVertexFormatUInt2:
                    size = 8;
                    break;
                    
                case MTLVertexFormatFloat3:
                case MTLVertexFormatInt3:
                case MTLVertexFormatUInt3:
                    size = 12;
                    break;
                    
                case MTLVertexFormatFloat4:
                case MTLVertexFormatInt4:
                case MTLVertexFormatUInt4:
                    size = 16;
                    break;
                    
                default:
                    AZ_Assert(false, "Unknown metal vertex format");
                    return false;
            }
            return true;
        }

        MTLVertexStepFunction ConvertStepFunction(RHI::StreamStepFunction stepFunction)
        {
            static const MTLVertexStepFunction table[] =
            {
                MTLVertexStepFunctionConstant,
                MTLVertexStepFunctionPerVertex,
                MTLVertexStepFunctionPerInstance,
                MTLVertexStepFunctionPerPatch,
                MTLVertexStepFunctionPerPatchControlPoint
            };
            return table[static_cast<uint32_t>(stepFunction)];
        }

        void ConvertInputElements(const RHI::InputStreamLayout& layout, MTLVertexDescriptor* vertDesc)
        {
            uint32_t channelIdx = 0;
            for (const RHI::StreamChannelDescriptor& streamChannel : layout.GetStreamChannels())
            {
                AZ_Assert(streamChannel.m_bufferIndex < METAL_MAX_ENTRIES_BUFFER_ARG_TABLE, "Can not exceed the max slots allowed");
                uint32_t remappedBufferIdx = (METAL_MAX_ENTRIES_BUFFER_ARG_TABLE - 1) - streamChannel.m_bufferIndex;

                vertDesc.attributes[channelIdx].offset = streamChannel.m_byteOffset;
                vertDesc.attributes[channelIdx].format = ConvertVertexFormat(streamChannel.m_format);
                vertDesc.attributes[channelIdx].bufferIndex = remappedBufferIdx;
                channelIdx++;
            }

            uint32_t bufferIdx = 0;
            for (const RHI::StreamBufferDescriptor& streamBuffer : layout.GetStreamBuffers())
            {
                AZ_Assert(bufferIdx < METAL_MAX_ENTRIES_BUFFER_ARG_TABLE, "Can not exceed the max slots allowed");
                uint32_t remappedBufferIdx = (METAL_MAX_ENTRIES_BUFFER_ARG_TABLE - 1) - bufferIdx;

                vertDesc.layouts[remappedBufferIdx].stepFunction = ConvertStepFunction(streamBuffer.m_stepFunction);
                vertDesc.layouts[remappedBufferIdx].stepRate = streamBuffer.m_stepRate;
                vertDesc.layouts[remappedBufferIdx].stride = streamBuffer.m_byteStride;
                bufferIdx++;
            }
        }
        
        MTLSamplerMinMagFilter ConvertFilterMode(RHI::FilterMode mode)
        {
            switch (mode)
            {
                case RHI::FilterMode::Point:
                    return MTLSamplerMinMagFilterNearest;
                case RHI::FilterMode::Linear:
                    return MTLSamplerMinMagFilterLinear;
            }
            
            AZ_Assert(false, "bad conversion in ConvertFilterMode");
            return MTLSamplerMinMagFilterNearest;
        }
        
        MTLSamplerMipFilter ConvertMipFilterMode(RHI::FilterMode mode)
        {
            switch (mode)
            {
                case RHI::FilterMode::Point:
                    return MTLSamplerMipFilterNearest;
                case RHI::FilterMode::Linear:
                    return MTLSamplerMipFilterLinear;
            }
            return MTLSamplerMipFilterNotMipmapped;
        }
        
        MTLSamplerAddressMode ConvertAddressMode(RHI::AddressMode addressMode)
        {
            switch (addressMode)
            {
                case RHI::AddressMode::Wrap:
                    return MTLSamplerAddressModeRepeat;
                case RHI::AddressMode::Clamp:
                    return MTLSamplerAddressModeClampToEdge;
                case RHI::AddressMode::Mirror:
                    return MTLSamplerAddressModeMirrorRepeat;
                default:
                    return Platform::ConvertAddressMode(addressMode);
            }
        }
        
        MTLCompareFunction ConvertComparisonFunc(RHI::ComparisonFunc func)
        {
            static const MTLCompareFunction table[] =
            {
                MTLCompareFunctionNever,
                MTLCompareFunctionLess,
                MTLCompareFunctionEqual,
                MTLCompareFunctionLessEqual,
                MTLCompareFunctionGreater,
                MTLCompareFunctionNotEqual,
                MTLCompareFunctionGreaterEqual,
                MTLCompareFunctionAlways
            };
            return table[static_cast<uint32_t>(func)];
        }

#if AZ_TRAIT_ATOM_METAL_SAMPLER_BORDERCOLOR_SUPPORT
    MTLSamplerBorderColor ConvertBorderColor(RHI::BorderColor color)
    {
        switch (color)
        {
            case RHI::BorderColor::OpaqueBlack:
                return MTLSamplerBorderColorOpaqueBlack;
            case RHI::BorderColor::TransparentBlack:
                return MTLSamplerBorderColorTransparentBlack;
            case RHI::BorderColor::OpaqueWhite:
                return MTLSamplerBorderColorOpaqueWhite;
            default:
                AZ_Assert(false, "Unsupported Border Color");
        }
        return MTLSamplerBorderColorOpaqueBlack;
    }
#endif

        void ConvertSamplerState(const RHI::SamplerState& state, MTLSamplerDescriptor* samplerDesc)
        {
            samplerDesc.sAddressMode = ConvertAddressMode(state.m_addressU);
            samplerDesc.tAddressMode = ConvertAddressMode(state.m_addressV);
            samplerDesc.rAddressMode = ConvertAddressMode(state.m_addressW);
            samplerDesc.compareFunction = ConvertComparisonFunc(state.m_comparisonFunc);
    
            samplerDesc.minFilter = ConvertFilterMode(state.m_filterMin);
            samplerDesc.magFilter = ConvertFilterMode(state.m_filterMag);
            samplerDesc.mipFilter = ConvertMipFilterMode(state.m_filterMip);
            samplerDesc.supportArgumentBuffers = YES;
            
            samplerDesc.maxAnisotropy = AZ::u8(state.m_anisotropyMax);
            if (!state.m_anisotropyEnable)
            {
                samplerDesc.maxAnisotropy = 1;
            }
            samplerDesc.lodMaxClamp = AZ::u8(state.m_mipLodMax);
            samplerDesc.lodMinClamp = AZ::u8(state.m_mipLodMin);
            
#if AZ_TRAIT_ATOM_METAL_SAMPLER_BORDERCOLOR_SUPPORT
            samplerDesc.borderColor = ConvertBorderColor(state.m_borderColor);
#endif            
            if(state.m_mipLodBias)
            {
                AZ_Warning("ConvertSamplerState", false, "Metal sampler: MipLODBias is not supported.");
            }
        }
        
        MTLPrimitiveType ConvertPrimitiveTopology(RHI::PrimitiveTopology primTopology)
        {
            switch (primTopology)
            {
                case RHI::PrimitiveTopology::PointList:
                    return  MTLPrimitiveTypePoint;
                case RHI::PrimitiveTopology::LineList:
                    return MTLPrimitiveTypeLine;
                case RHI::PrimitiveTopology::LineStrip:
                    return  MTLPrimitiveTypeLineStrip;
                case RHI::PrimitiveTopology::TriangleList:
                    return MTLPrimitiveTypeTriangle;
                case RHI::PrimitiveTopology::TriangleStrip:
                    return  MTLPrimitiveTypeTriangleStrip;
                default:
                    AZ_Assert(false, "Invalid primitive topology");
                break;
            }
            return MTLPrimitiveTypePoint;
        }
        
        MTLStencilOperation ConvertStencilOp(RHI::StencilOp op)
        {
            static const MTLStencilOperation table[] =
            {
                MTLStencilOperationKeep,
                MTLStencilOperationZero,
                MTLStencilOperationReplace,
                MTLStencilOperationIncrementClamp,
                MTLStencilOperationDecrementClamp,
                MTLStencilOperationInvert,
                MTLStencilOperationIncrementWrap,
                MTLStencilOperationDecrementWrap
            };
            return table[static_cast<uint32_t>(op)];
        }
        
        void ConvertStencilState(const RHI::StencilState& stencilState, MTLDepthStencilDescriptor* mtlDepthStencilDesc)
        {
            MTLStencilDescriptor* frontFaceStencil = mtlDepthStencilDesc.frontFaceStencil;
            MTLStencilDescriptor* backFaceStencil  = mtlDepthStencilDesc.backFaceStencil;
            
            backFaceStencil.readMask  = frontFaceStencil.readMask  = stencilState.m_readMask;
            backFaceStencil.writeMask = frontFaceStencil.writeMask = stencilState.m_writeMask;

            frontFaceStencil.stencilCompareFunction     = ConvertComparisonFunc(stencilState.m_frontFace.m_func);
            frontFaceStencil.stencilFailureOperation    = ConvertStencilOp(stencilState.m_frontFace.m_failOp);
            frontFaceStencil.depthFailureOperation      = ConvertStencilOp(stencilState.m_frontFace.m_depthFailOp);
            frontFaceStencil.depthStencilPassOperation  = ConvertStencilOp(stencilState.m_frontFace.m_passOp);

            
            backFaceStencil.stencilCompareFunction      = ConvertComparisonFunc(stencilState.m_backFace.m_func);
            backFaceStencil.stencilFailureOperation     = ConvertStencilOp(stencilState.m_backFace.m_failOp);
            backFaceStencil.depthFailureOperation       = ConvertStencilOp(stencilState.m_backFace.m_depthFailOp);
            backFaceStencil.depthStencilPassOperation   = ConvertStencilOp(stencilState.m_backFace.m_passOp);
        }
        
        void ConvertDepthStencilState(const RHI::DepthStencilState& depthStencil, MTLDepthStencilDescriptor* mtlDepthStencilDesc)
        {
            mtlDepthStencilDesc.depthWriteEnabled = depthStencil.m_depth.m_enable ? (depthStencil.m_depth.m_writeMask != RHI::DepthWriteMask::Zero): false;
            mtlDepthStencilDesc.depthCompareFunction = depthStencil.m_depth.m_enable ? ConvertComparisonFunc(depthStencil.m_depth.m_func)
                                                                                     : MTLCompareFunctionAlways;
            
            if(depthStencil.m_stencil.m_enable)
            {
                ConvertStencilState(depthStencil.m_stencil, mtlDepthStencilDesc);
            }
        }
        
        MTLCullMode ConvertCullMode(RHI::CullMode mode)
        {
            static const MTLCullMode table[] =
            {
                MTLCullModeNone,
                MTLCullModeFront,
                MTLCullModeBack
            };
            return table[static_cast<uint32_t>(mode)];
        }
        
        MTLTriangleFillMode ConvertFillMode(RHI::FillMode mode)
        {
            static const MTLTriangleFillMode table[] =
            {
                MTLTriangleFillModeFill,
                MTLTriangleFillModeLines
            };
            return table[static_cast<uint32_t>(mode)];
        }

        void ConvertRasterState(const RHI::RasterState& raster, RasterizerState& rasterizerState)
        {
            //todo::need support for MTLWindingClockwise
            rasterizerState.m_frontFaceWinding = MTLWindingCounterClockwise;
            rasterizerState.m_cullMode = ConvertCullMode(raster.m_cullMode);
            rasterizerState.m_depthBias = raster.m_depthBias;
            rasterizerState.m_depthBiasClamp = raster.m_depthBiasClamp;
            rasterizerState.m_depthSlopeScale = raster.m_depthBiasSlopeScale;
            rasterizerState.m_triangleFillMode = ConvertFillMode(raster.m_fillMode);
            rasterizerState.m_depthClipMode = raster.m_depthClipEnable ? MTLDepthClipModeClip:MTLDepthClipModeClamp;
        }
        
        bool IsDepthStencilMerged(RHI::Format format)
        {
            switch (format)
            {
                case RHI::Format::D32_FLOAT_S8X24_UINT:
                case RHI::Format::D24_UNORM_S8_UINT:
                    return true;
            }
            return false;
        }
        
        bool IsDepthStencilMerged(MTLPixelFormat mtlFormat)
        {
            switch (mtlFormat)
            {
                case MTLPixelFormatDepth32Float_Stencil8:
                    return true;
                default:
                    return Platform::IsDepthStencilMerged(mtlFormat);
            }
        }
        
        bool IsDepthStencilFormat(RHI::Format format)
        {
            switch (format)
            {
                case RHI::Format::D32_FLOAT_S8X24_UINT:
                case RHI::Format::D24_UNORM_S8_UINT:
                case RHI::Format::D32_FLOAT:
                case RHI::Format::D16_UNORM:
                case RHI::Format::D16_UNORM_S8_UINT:
                    return true;
                default:
                    return false;
            }
        }

        void ConvertImageArgumentDescriptor(MTLArgumentDescriptor* imgArgDescriptor, const RHI::ShaderInputImageDescriptor& shaderInputImage)
        {
            imgArgDescriptor.dataType = MTLDataTypeTexture;
            imgArgDescriptor.index = shaderInputImage.m_registerId;
            switch(shaderInputImage.m_access)
            {
                case RHI::ShaderInputImageAccess::Read:
                {
                    imgArgDescriptor.access = MTLArgumentAccessReadOnly;
                    break;
                }
                case RHI::ShaderInputImageAccess::ReadWrite:
                {
                    imgArgDescriptor.access = MTLArgumentAccessReadWrite;
                    break;
                }
                default:
                {
                    AZ_Assert(false, "Invalid usage type.");
                }
            }
            switch(shaderInputImage.m_type)
            {
                case RHI::ShaderInputImageType::Image1D:
                {
                    imgArgDescriptor.textureType = MTLTextureType1D;
                    break;
                }
                case RHI::ShaderInputImageType::Image1DArray:
                {
                    imgArgDescriptor.textureType = MTLTextureType1DArray;
                    break;
                }
                case RHI::ShaderInputImageType::Image2D:
                {
                    imgArgDescriptor.textureType = MTLTextureType2D;
                    break;
                }
                case RHI::ShaderInputImageType::Image2DArray:
                {
                    imgArgDescriptor.textureType = MTLTextureType2DArray;
                    break;
                }
                case RHI::ShaderInputImageType::Image2DMultisample:
                {
                    imgArgDescriptor.textureType = MTLTextureType2DMultisample;
                    break;
                }
                case RHI::ShaderInputImageType::Image3D:
                {
                    imgArgDescriptor.textureType = MTLTextureType3D;
                    break;
                }
                case RHI::ShaderInputImageType::ImageCube:
                {
                    imgArgDescriptor.textureType = MTLTextureTypeCube;
                    break;
                }
                case RHI::ShaderInputImageType::ImageCubeArray:
                {
                    imgArgDescriptor.textureType = MTLTextureTypeCubeArray;
                    break;
                }
                default:
                {
                    AZ_Assert(false, "Invalid texture type.");
                }
            }
            imgArgDescriptor.arrayLength = shaderInputImage.m_count;
        }
        
        void ConvertBufferArgumentDescriptor(MTLArgumentDescriptor* bufferArgDescriptor, const RHI::ShaderInputBufferDescriptor& shaderInputBuffer)
        {
            AZ_Assert(bufferArgDescriptor, "bufferArgDescriptor is null");
            
            bufferArgDescriptor.index = shaderInputBuffer.m_registerId;
            switch(shaderInputBuffer.m_access)
            {
                case RHI::ShaderInputBufferAccess::Constant:
                case RHI::ShaderInputBufferAccess::Read:
                {
                    bufferArgDescriptor.access = MTLArgumentAccessReadOnly;
                    break;
                }
                case RHI::ShaderInputBufferAccess::ReadWrite:
                {
                    bufferArgDescriptor.access = MTLArgumentAccessReadWrite;
                    break;
                }
                default:
                {
                    AZ_Assert(false, "Invalid usage type.");
                }
            }
            bufferArgDescriptor.arrayLength = shaderInputBuffer.m_count;
            
            if(shaderInputBuffer.m_type == RHI::ShaderInputBufferType::Typed)
            {
                //Typed buffers (Buffer/RWBuffer) become texture_buffer
                bufferArgDescriptor.dataType = MTLDataTypeTexture;
                bufferArgDescriptor.textureType = MTLTextureTypeTextureBuffer;
            }
            else
            {
                bufferArgDescriptor.dataType = MTLDataTypePointer;
            }
        }
    
        MTLSamplePosition ConvertSampleLocation(const RHI::SamplePosition& position)
        {
            const static float cellSize = 1.0f / RHI::Limits::Pipeline::MultiSampleCustomLocationGridSize;
            return MTLSamplePosition{ position.m_x * cellSize, position.m_y * cellSize };
        }
    
        MTLResourceOptions CovertStorageMode(MTLStorageMode storageMode)
        {
            switch(storageMode)
            {
                case MTLStorageModeShared:
                {
                    return MTLResourceStorageModeShared;
                }
                case MTLStorageModePrivate:
                {
                    return MTLResourceStorageModePrivate;
                }
                default:
                {
                    return Platform::CovertStorageMode(storageMode);
                }
            }
        }
    
        MTLResourceOptions CovertCPUCacheMode(MTLCPUCacheMode cpuCacheMode)
        {
            switch(cpuCacheMode)
            {
                case MTLCPUCacheModeDefaultCache:
                {
                    return MTLResourceCPUCacheModeDefaultCache;
                }
                case MTLCPUCacheModeWriteCombined:
                {
                    return MTLResourceCPUCacheModeWriteCombined;
                }
                default:
                {
                    AZ_Assert(false, "Type not supported");
                    return MTLResourceCPUCacheModeDefaultCache;
                }
            }
        }
    
        MTLResourceOptions CovertHazardTrackingMode(MTLHazardTrackingMode hazardTrackingMode)
        {
            switch(hazardTrackingMode)
            {
                case MTLHazardTrackingModeDefault:
                {
                    return MTLResourceHazardTrackingModeDefault;
                }
                case MTLHazardTrackingModeUntracked:
                {
                    return MTLResourceHazardTrackingModeUntracked;
                }
                case MTLHazardTrackingModeTracked:
                {
                    return MTLResourceHazardTrackingModeTracked;
                }
                default:
                {
                    AZ_Assert(false, "Type not supported");
                    return MTLResourceHazardTrackingModeDefault;
                }
            }
        }
    
        MTLResourceOptions CovertToResourceOptions(MTLStorageMode storageMode,
                                                   MTLCPUCacheMode cpuCacheMode,
                                                   MTLHazardTrackingMode hazardTrackingMode)
        {
            MTLResourceOptions resourceOptions;
            
            resourceOptions = CovertCPUCacheMode(cpuCacheMode) | CovertStorageMode(storageMode) | CovertHazardTrackingMode(hazardTrackingMode);
            return resourceOptions;
        }
          
        //This returns the available cpu/gpu memory for the platform. 
        MTLStorageMode GetCPUGPUMemoryMode()
        {
            return Platform::GetCPUGPUMemoryMode();
        }
    
        MTLVisibilityResultMode ConvertVisibilityResult(RHI::QueryControlFlags flags)
        {
            return RHI::CheckBitsAll(flags, RHI::QueryControlFlags::PreciseOcclusion) ? MTLVisibilityResultModeCounting : MTLVisibilityResultModeBoolean;
        }
    
        bool IsCompressedFormat(RHI::Format format)
        {
            switch (format)
            {
                case RHI::Format::BC1_UNORM:
                case RHI::Format::BC1_UNORM_SRGB:
                case RHI::Format::BC2_UNORM:
                case RHI::Format::BC2_UNORM_SRGB:
                case RHI::Format::BC3_UNORM:
                case RHI::Format::BC3_UNORM_SRGB:
                case RHI::Format::BC4_UNORM:
                case RHI::Format::BC4_SNORM:
                case RHI::Format::BC5_UNORM:
                case RHI::Format::BC5_SNORM:
                case RHI::Format::BC6H_UF16:
                case RHI::Format::BC6H_SF16:
                case RHI::Format::BC7_UNORM:
                case RHI::Format::BC7_UNORM_SRGB:
                case RHI::Format::EAC_R11_UNORM:
                case RHI::Format::EAC_R11_SNORM:
                case RHI::Format::EAC_RG11_UNORM:
                case RHI::Format::EAC_RG11_SNORM:
                case RHI::Format::ETC2_UNORM:
                case RHI::Format::ETC2_UNORM_SRGB:
                case RHI::Format::ETC2A_UNORM:
                case RHI::Format::ETC2A_UNORM_SRGB:
                case RHI::Format::PVRTC2_UNORM:
                case RHI::Format::PVRTC2_UNORM_SRGB:
                case RHI::Format::PVRTC4_UNORM:
                case RHI::Format::PVRTC4_UNORM_SRGB:
                case RHI::Format::ASTC_4x4_UNORM:
                case RHI::Format::ASTC_4x4_UNORM_SRGB:
                case RHI::Format::ASTC_5x4_UNORM:
                case RHI::Format::ASTC_5x4_UNORM_SRGB:
                case RHI::Format::ASTC_5x5_UNORM:
                case RHI::Format::ASTC_5x5_UNORM_SRGB:
                case RHI::Format::ASTC_6x5_UNORM:
                case RHI::Format::ASTC_6x5_UNORM_SRGB:
                case RHI::Format::ASTC_6x6_UNORM:
                case RHI::Format::ASTC_6x6_UNORM_SRGB:
                case RHI::Format::ASTC_8x5_UNORM:
                case RHI::Format::ASTC_8x5_UNORM_SRGB:
                case RHI::Format::ASTC_8x6_UNORM:
                case RHI::Format::ASTC_8x6_UNORM_SRGB:
                case RHI::Format::ASTC_8x8_UNORM:
                case RHI::Format::ASTC_8x8_UNORM_SRGB:
                case RHI::Format::ASTC_10x5_UNORM:
                case RHI::Format::ASTC_10x5_UNORM_SRGB:
                case RHI::Format::ASTC_10x6_UNORM:
                case RHI::Format::ASTC_10x6_UNORM_SRGB:
                case RHI::Format::ASTC_10x8_UNORM:
                case RHI::Format::ASTC_10x8_UNORM_SRGB:
                case RHI::Format::ASTC_10x10_UNORM:
                case RHI::Format::ASTC_10x10_UNORM_SRGB:
                case RHI::Format::ASTC_12x10_UNORM:
                case RHI::Format::ASTC_12x10_UNORM_SRGB:
                case RHI::Format::ASTC_12x12_UNORM:
                case RHI::Format::ASTC_12x12_UNORM_SRGB:
                    return true;
                default:
                    return false;
            }
        }
    
        bool IsFilteringSupported(id<MTLDevice> mtlDevice, RHI::Format format)
        {
            //https://developer.apple.com/metal/Metal-Feature-Set-Tables.pdf
            switch(format)
            {
                case RHI::Format::R8_UINT:
                case RHI::Format::R8_SINT:
                case RHI::Format::R16_UINT:
                case RHI::Format::R16_SINT:
                case RHI::Format::R8G8_UINT:
                case RHI::Format::R8G8_SINT:
                case RHI::Format::R32_UINT:
                case RHI::Format::R32_SINT:
                case RHI::Format::R16G16_UINT:
                case RHI::Format::R16G16_SINT:
                case RHI::Format::R8G8B8A8_UINT:
                case RHI::Format::R8G8B8A8_SINT:
                case RHI::Format::R10G10B10A2_UINT:
                case RHI::Format::R32G32_UINT:
                case RHI::Format::R32G32_SINT:
                case RHI::Format::R16G16B16A16_UINT:
                case RHI::Format::R16G16B16A16_SINT:
                case RHI::Format::R32G32B32A32_UINT:
                case RHI::Format::R32G32B32A32_SINT:
                {
                    return false;
                }
                default:
                {
                    return Platform::IsFilteringSupported(mtlDevice, format);
                }
            }
        }
    
        bool IsWriteSupported(id<MTLDevice> mtlDevice, RHI::Format format)
        {
            if(IsDepthStencilFormat(format) || IsCompressedFormat(format) || format == RHI::Format::A8_UNORM)
            {
                return false;
            }
                        
            return Platform::IsWriteSupported(mtlDevice, format);
        }
    
        bool IsColorRenderTargetSupported(id<MTLDevice> mtlDevice, RHI::Format format)
        {
            if(IsDepthStencilFormat(format) || IsCompressedFormat(format) || format == RHI::Format::A8_UNORM)
            {
                return false;
            }
            
            return Platform::IsColorRenderTargetSupported(mtlDevice, format);
        }
    
        bool IsBlendingSupported(id<MTLDevice> mtlDevice, RHI::Format format)
        {
            if(IsDepthStencilFormat(format) || IsCompressedFormat(format))
            {
                return false;
            }
            
            switch(format)
            {
                case RHI::Format::A8_UNORM:
                case RHI::Format::R8_UINT:
                case RHI::Format::R8_SINT:
                case RHI::Format::R16_UINT:
                case RHI::Format::R16_SINT:
                case RHI::Format::R8G8_UINT:
                case RHI::Format::R8G8_SINT:
                case RHI::Format::R32_UINT:
                case RHI::Format::R32_SINT:
                case RHI::Format::R16G16_UINT:
                case RHI::Format::R16G16_SINT:
                case RHI::Format::R8G8B8A8_UINT:
                case RHI::Format::R8G8B8A8_SINT:
                case RHI::Format::R10G10B10A2_UINT:
                case RHI::Format::R32G32_UINT:
                case RHI::Format::R32G32_SINT:
                case RHI::Format::R16G16B16A16_UINT:
                case RHI::Format::R16G16B16A16_SINT:
                case RHI::Format::R32G32B32A32_UINT:
                case RHI::Format::R32G32B32A32_SINT:
                {
                    return false;
                }
                default:
                {
                    return Platform::IsBlendingSupported(mtlDevice, format);
                }
            }
        }
    
        bool IsMSAASupported(id<MTLDevice> mtlDevice, RHI::Format format)
        {
            if(IsCompressedFormat(format) || format == RHI::Format::A8_UNORM)
            {
                return false;
            }
            return Platform::IsMSAASupported(mtlDevice, format);
        }
    
        bool IsResolveTargetSupported(id<MTLDevice> mtlDevice, RHI::Format format)
        {
            if(IsCompressedFormat(format))
            {
                return false;
            }
            
            switch(format)
            {
                case RHI::Format::A8_UNORM:
                case RHI::Format::R8_UINT:
                case RHI::Format::R8_SINT:
                case RHI::Format::R16_UNORM:
                case RHI::Format::R16_SNORM:
                case RHI::Format::R16_UINT:
                case RHI::Format::R16_SINT:
                case RHI::Format::R8G8_UINT:
                case RHI::Format::R8G8_SINT:
                case RHI::Format::R32_UINT:
                case RHI::Format::R32_SINT:
                case RHI::Format::R32_FLOAT:
                case RHI::Format::R16G16_UNORM:
                case RHI::Format::R16G16_UINT:
                case RHI::Format::R16G16_SNORM:
                case RHI::Format::R16G16_SINT:
                case RHI::Format::R8G8B8A8_UINT:
                case RHI::Format::R8G8B8A8_SINT:
                case RHI::Format::R10G10B10A2_UINT:
                case RHI::Format::R32G32_UINT:
                case RHI::Format::R32G32_SINT:
                case RHI::Format::R32G32_FLOAT:
                case RHI::Format::R16G16B16A16_UNORM:
                case RHI::Format::R16G16B16A16_SNORM:
                case RHI::Format::R16G16B16A16_UINT:
                case RHI::Format::R16G16B16A16_SINT:
                case RHI::Format::R32G32B32_FLOAT:
                case RHI::Format::R32G32B32_UINT:
                case RHI::Format::R32G32B32_SINT:
                {
                    return false;
                }
                default:
                {
                    return Platform::IsResolveTargetSupported(mtlDevice, format);
                }
            }
        }
     
        bool IsDepthStencilSupported(id<MTLDevice> mtlDevice, RHI::Format format)
        {
            if(!IsDepthStencilFormat(format) || IsCompressedFormat(format))
            {
                return false;
            }
            
            return Platform::IsDepthStencilSupported(mtlDevice, format);
        }
    
        MTLBlitOption GetBlitOption(RHI::Format format)
        {
            return Platform::GetBlitOption(format);
        }
    
        MTLResourceUsage GetImageResourceUsage(RHI::ShaderInputImageAccess imageAccess)
        {
            MTLResourceUsage mtlResourceUsage = MTLResourceUsageSample | MTLResourceUsageRead;
            if(imageAccess == RHI::ShaderInputImageAccess::ReadWrite)
            {
                mtlResourceUsage |= MTLResourceUsageWrite;
            }
            return mtlResourceUsage;
        }
        
        MTLResourceUsage GetBufferResourceUsage(RHI::ShaderInputBufferAccess bufferAccess)
        {
            MTLResourceUsage mtlResourceUsage = MTLResourceUsageRead;
            if(bufferAccess == RHI::ShaderInputBufferAccess::ReadWrite)
            {
                mtlResourceUsage |= MTLResourceUsageWrite;
            }
            return mtlResourceUsage;
        }
        
        MTLRenderStages GetRenderStages(RHI::ShaderStageMask shaderMask)
        {
            MTLRenderStages mtlRenderStages = 0;
            if(RHI::CheckBitsAny(shaderMask, RHI::ShaderStageMask::Vertex))
            {
                mtlRenderStages |= MTLRenderStageVertex;
            }
            if(RHI::CheckBitsAny(shaderMask, RHI::ShaderStageMask::Fragment))
            {
                mtlRenderStages |= MTLRenderStageFragment;
            }
            return mtlRenderStages;
        }
    
        RHI::ShaderInputImageAccess GetImageAccess(RHI::ShaderInputBufferAccess bufferAccess)
        {
            if(bufferAccess == RHI::ShaderInputBufferAccess::ReadWrite)
            {
                return RHI::ShaderInputImageAccess::ReadWrite;
            }
            return RHI::ShaderInputImageAccess::Read;
        }
    }
}
