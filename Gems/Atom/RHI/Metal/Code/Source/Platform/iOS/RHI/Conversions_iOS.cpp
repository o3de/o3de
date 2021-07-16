/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/Conversions.h>

namespace AZ
{
    namespace Metal
    {
        namespace Platform
        {
            bool IsFormatAvailable(RHI::Format format)
            {
                switch (format)
                {
                    case RHI::Format::D24_UNORM_S8_UINT:
                    case RHI::Format::D16_UNORM:
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
                        return false;
                    default:
                        return true;
                }
            }
        
            bool IsFilteringSupported(id<MTLDevice> mtlDevice, RHI::Format format)
            {
                AZ_UNUSED(mtlDevice);
                
                //Assumes min spec of MTLGPUFamilyApple4
                if(!IsFormatAvailable(format))
                {
                    return false;
                }
                   
                switch(format)
                {
                    case RHI::Format::R32G32_FLOAT:
                    case RHI::Format::R32G32B32A32_FLOAT:
                    case RHI::Format::D32_FLOAT:
                    case RHI::Format::D32_FLOAT_S8X24_UINT:
                        return false;
                    default:
                        return true;
                }
            }
        
            bool IsWriteSupported(id<MTLDevice> mtlDevice, RHI::Format format)
            {
                AZ_UNUSED(mtlDevice);
                
                //Assumes min spec of MTLGPUFamilyApple4
                if(!IsFormatAvailable(format))
                {
                    return false;
                }
                
                switch(format)
                {
                    case RHI::Format::B5G6R5_UNORM:
                    case RHI::Format::B5G5R5A1_UNORM:
                    case RHI::Format::B4G4R4A4_UNORM:
                    case RHI::Format::A1B5G5R5_UNORM:
                        return false;
                    default:
                        return true;
                }
            }
        
            bool IsColorRenderTargetSupported(id<MTLDevice> mtlDevice, RHI::Format format)
            {
                AZ_UNUSED(mtlDevice);
                
                //Assumes min spec of MTLGPUFamilyApple4
                return IsFormatAvailable(format);
            }
        
            bool IsBlendingSupported(id<MTLDevice> mtlDevice, RHI::Format format)
            {
                AZ_UNUSED(mtlDevice);
                
                //Assumes min spec of MTLGPUFamilyApple4
                if(!IsFormatAvailable(format))
                {
                    return false;
                }
                
                switch(format)
                {
                    case RHI::Format::R32G32B32A32_FLOAT:
                        return false;
                    default:
                        return true;
                }
            }

            bool IsMSAASupported(id<MTLDevice> mtlDevice, RHI::Format format)
            {
                //Assumes min spec of MTLGPUFamilyApple4
                if(!IsFormatAvailable(format))
                {
                    return false;
                }
                
                bool isGPUFamily4 = [mtlDevice supportsFeatureSet: MTLFeatureSet_iOS_GPUFamily4_v1];
                
                if(!isGPUFamily4)
                {
                    //Not supported for GPUFamily3 and below
                    switch(format)
                    {
                        case RHI::Format::R32_UINT:
                        case RHI::Format::R32_SINT:
                        case RHI::Format::R32G32_UINT:
                        case RHI::Format::R32G32_SINT:
                        case RHI::Format::R32G32_FLOAT:
                        case RHI::Format::R32G32B32A32_UINT:
                        case RHI::Format::R32G32B32A32_SINT:
                        case RHI::Format::R32G32B32A32_FLOAT:
                            return false;
                        default:
                            return true;
                    }
                }
                return true;
            }
        
            bool IsResolveTargetSupported(id<MTLDevice> mtlDevice, RHI::Format format)
            {
                AZ_UNUSED(mtlDevice);
                return IsFormatAvailable(format);
            }
        
            bool IsDepthStencilSupported(id<MTLDevice> mtlDevice, RHI::Format format)
            {
                return IsFormatAvailable(format);
            }
        
            bool IsTextureAsDepthStencilSupported(id<MTLDevice> mtlDevice, RHI::Format format)
            {
                AZ_UNUSED(mtlDevice);
                return IsFormatAvailable(format);
            }
        
            bool IsDepthStencilMerged(MTLPixelFormat mtlFormat)
            {
                AZ_UNUSED(mtlFormat);
                return false;
            }
        
            MTLBlitOption GetBlitOption(RHI::Format format)
            {
                switch(format)
                {
                    case RHI::Format::PVRTC4_UNORM:
                    case RHI::Format::PVRTC4_UNORM_SRGB:
                        return MTLBlitOptionRowLinearPVRTC;
                    default:
                        return MTLBlitOptionNone;
                }
            }
        
            MTLSamplerAddressMode ConvertAddressMode(RHI::AddressMode addressMode)
            {
                //No iOS specific addressMode.
                AZ_UNUSED(addressMode);
                AZ_Assert(false, "Unsupported  addressMode in ConvertAddressMode");
                return MTLSamplerAddressModeRepeat;
            }
        
            MTLResourceOptions CovertStorageMode(MTLStorageMode storageMode)
            {
                //No iOS specific storageMode.
                AZ_UNUSED(storageMode);
                AZ_Assert(false, "storageMode not supported");
                return MTLResourceStorageModeShared;
            }
        
            MTLStorageMode GetCPUGPUMemoryMode()
            {
                //ios has unified memory
                return MTLStorageModeShared;
            }
        
            MTLPixelFormat ConvertPixelFormat(RHI::Format format)
            {
                switch (format)
                {
                    case RHI::Format::R8_UNORM_SRGB:
                        return MTLPixelFormatR8Unorm;
                    case RHI::Format::R8G8_UNORM_SRGB:
                        return MTLPixelFormatRG8Unorm;
                    case RHI::Format::B5G6R5_UNORM:
                        return MTLPixelFormatB5G6R5Unorm;
                    case RHI::Format::B5G5R5A1_UNORM:
                        return MTLPixelFormatBGR5A1Unorm;
                    case RHI::Format::B4G4R4A4_UNORM:
                        return MTLPixelFormatABGR4Unorm;
                    case RHI::Format::EAC_R11_UNORM:
                        return MTLPixelFormatEAC_R11Unorm;
                    case RHI::Format::EAC_R11_SNORM:
                        return MTLPixelFormatEAC_R11Snorm;
                    case RHI::Format::EAC_RG11_UNORM:
                        return MTLPixelFormatEAC_RG11Unorm;
                    case RHI::Format::EAC_RG11_SNORM:
                        return MTLPixelFormatEAC_RG11Snorm;
                    case RHI::Format::ETC2_UNORM:
                        return MTLPixelFormatETC2_RGB8;
                    case RHI::Format::ETC2_UNORM_SRGB:
                        return MTLPixelFormatETC2_RGB8_sRGB;
                    case RHI::Format::ETC2A_UNORM:
                        return MTLPixelFormatETC2_RGB8A1;
                    case RHI::Format::ETC2A_UNORM_SRGB:
                        return MTLPixelFormatETC2_RGB8A1_sRGB;
                    case RHI::Format::PVRTC2_UNORM:
                        return MTLPixelFormatPVRTC_RGBA_2BPP;
                    case RHI::Format::PVRTC2_UNORM_SRGB:
                        return MTLPixelFormatPVRTC_RGBA_2BPP_sRGB;
                    case RHI::Format::PVRTC4_UNORM:
                        return MTLPixelFormatPVRTC_RGBA_4BPP;
                    case RHI::Format::PVRTC4_UNORM_SRGB:
                        return MTLPixelFormatPVRTC_RGBA_4BPP_sRGB;
                    case RHI::Format::ASTC_4x4_UNORM:
                        return MTLPixelFormatASTC_4x4_LDR;
                    case RHI::Format::ASTC_4x4_UNORM_SRGB:
                        return MTLPixelFormatASTC_4x4_sRGB;
                    case RHI::Format::ASTC_5x4_UNORM:
                        return MTLPixelFormatASTC_5x4_LDR;
                    case RHI::Format::ASTC_5x4_UNORM_SRGB:
                        return MTLPixelFormatASTC_5x4_sRGB;
                    case RHI::Format::ASTC_5x5_UNORM:
                        return MTLPixelFormatASTC_5x5_LDR;
                    case RHI::Format::ASTC_5x5_UNORM_SRGB:
                        return MTLPixelFormatASTC_5x5_sRGB;
                    case RHI::Format::ASTC_6x5_UNORM:
                        return MTLPixelFormatASTC_6x5_LDR;
                    case RHI::Format::ASTC_6x5_UNORM_SRGB:
                        return MTLPixelFormatASTC_6x5_sRGB;
                    case RHI::Format::ASTC_6x6_UNORM:
                        return MTLPixelFormatASTC_6x6_LDR;
                    case RHI::Format::ASTC_6x6_UNORM_SRGB:
                        return MTLPixelFormatASTC_6x6_sRGB;
                    case RHI::Format::ASTC_8x5_UNORM:
                        return MTLPixelFormatASTC_8x5_LDR;
                    case RHI::Format::ASTC_8x5_UNORM_SRGB:
                        return MTLPixelFormatASTC_8x5_sRGB;
                    case RHI::Format::ASTC_8x6_UNORM:
                        return MTLPixelFormatASTC_8x6_LDR;
                    case RHI::Format::ASTC_8x6_UNORM_SRGB:
                        return MTLPixelFormatASTC_8x6_sRGB;
                    case RHI::Format::ASTC_8x8_UNORM:
                        return MTLPixelFormatASTC_8x8_LDR;
                    case RHI::Format::ASTC_8x8_UNORM_SRGB:
                        return MTLPixelFormatASTC_8x8_sRGB;
                    case RHI::Format::ASTC_10x5_UNORM:
                        return MTLPixelFormatASTC_10x5_LDR;
                    case RHI::Format::ASTC_10x5_UNORM_SRGB:
                        return MTLPixelFormatASTC_10x5_sRGB;
                    case RHI::Format::ASTC_10x6_UNORM:
                        return MTLPixelFormatASTC_10x6_LDR;
                    case RHI::Format::ASTC_10x6_UNORM_SRGB:
                        return MTLPixelFormatASTC_10x6_sRGB;
                    case RHI::Format::ASTC_10x8_UNORM:
                        return MTLPixelFormatASTC_10x8_LDR;
                    case RHI::Format::ASTC_10x8_UNORM_SRGB:
                        return MTLPixelFormatASTC_10x8_sRGB;
                    case RHI::Format::ASTC_10x10_UNORM:
                        return MTLPixelFormatASTC_10x10_LDR;
                    case RHI::Format::ASTC_10x10_UNORM_SRGB:
                        return MTLPixelFormatASTC_10x10_sRGB;
                    case RHI::Format::ASTC_12x10_UNORM:
                        return MTLPixelFormatASTC_12x10_LDR;
                    case RHI::Format::ASTC_12x10_UNORM_SRGB:
                        return MTLPixelFormatASTC_12x10_sRGB;
                    case RHI::Format::ASTC_12x12_UNORM:
                        return MTLPixelFormatASTC_12x12_LDR;
                    case RHI::Format::ASTC_12x12_UNORM_SRGB:
                        return MTLPixelFormatASTC_12x12_sRGB;
                
                default:
                    AZ_Assert(false, "unhandled conversion in ConvertPixelFormat");
                    return MTLPixelFormatInvalid;
                }
            }
        } //Platform
    } //Metal
} //AZ
