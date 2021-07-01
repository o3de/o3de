/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "Atom_RHI_Metal_precompiled.h"
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
                case RHI::Format::R8_UNORM_SRGB:
                case RHI::Format::R8G8_UNORM_SRGB:
                case RHI::Format::B5G6R5_UNORM:
                case RHI::Format::B5G5R5A1_UNORM:
                case RHI::Format::A1B5G5R5_UNORM:
                case RHI::Format::B4G4R4A4_UNORM:
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
                    return false;
                default:
                    return true;
                }
            }
        
            bool IsDepthStencilMerged(MTLPixelFormat mtlFormat)
            {
                if(mtlFormat == MTLPixelFormatDepth24Unorm_Stencil8)
                {
                    return true;
                }
                return false;
            }
        
            MTLPixelFormat ConvertPixelFormat(RHI::Format format)
            {
                switch (format)
                {
                case RHI::Format::D24_UNORM_S8_UINT:
                    return MTLPixelFormatDepth24Unorm_Stencil8;
                case RHI::Format::D16_UNORM:
                    return MTLPixelFormatDepth16Unorm;
                case RHI::Format::BC1_UNORM:
                    return MTLPixelFormatBC1_RGBA;
                case RHI::Format::BC1_UNORM_SRGB:
                    return MTLPixelFormatBC1_RGBA_sRGB;
                case RHI::Format::BC2_UNORM:
                    return MTLPixelFormatBC2_RGBA;
                case RHI::Format::BC2_UNORM_SRGB:
                    return MTLPixelFormatBC2_RGBA_sRGB;
                case RHI::Format::BC3_UNORM:
                    return MTLPixelFormatBC3_RGBA;
                case RHI::Format::BC3_UNORM_SRGB:
                    return MTLPixelFormatBC3_RGBA_sRGB;
                case RHI::Format::BC4_UNORM:
                    return MTLPixelFormatBC4_RUnorm;
                case RHI::Format::BC4_SNORM:
                    return MTLPixelFormatBC4_RSnorm;
                case RHI::Format::BC5_UNORM:
                    return MTLPixelFormatBC5_RGUnorm;
                case RHI::Format::BC5_SNORM:
                    return MTLPixelFormatBC5_RGSnorm;
                case RHI::Format::BC6H_UF16:
                    return MTLPixelFormatBC6H_RGBUfloat;
                case RHI::Format::BC6H_SF16:
                    return MTLPixelFormatBC6H_RGBFloat;
                case RHI::Format::BC7_UNORM:
                    return MTLPixelFormatBC7_RGBAUnorm;
                case RHI::Format::BC7_UNORM_SRGB:
                    return MTLPixelFormatBC7_RGBAUnorm_sRGB;
                default:
                    AZ_Assert(false, "unhandled conversion in ConvertPixelFormat");
                    return MTLPixelFormatInvalid;
                }
            }
        
            bool IsFilteringSupported(id<MTLDevice> mtlDevice, RHI::Format format)
            {
                AZ_UNUSED(mtlDevice);
                //Assumes min spec of MTLGPUFamilyMac2
                return IsFormatAvailable(format);
            }
        
            bool IsWriteSupported(id<MTLDevice> mtlDevice, RHI::Format format)
            {
                AZ_UNUSED(mtlDevice);
                //Assumes min spec of MTLGPUFamilyMac2
                if(!IsFormatAvailable(format))
                {
                    return false;
                }
                
                switch(format)
                {
                    case RHI::Format::R8G8B8A8_UNORM_SRGB:
                    case RHI::Format::B8G8R8A8_UNORM_SRGB:
                    case RHI::Format::R9G9B9E5_SHAREDEXP:
                        return false;
                    default:
                        return true;
                }
            }
        
            bool IsColorRenderTargetSupported(id<MTLDevice> mtlDevice, RHI::Format format)
            {
                AZ_UNUSED(mtlDevice);
                //Assumes min spec of MTLGPUFamilyMac2
                if(!IsFormatAvailable(format))
                {
                    return false;
                }
                
                switch(format)
                {
                    case RHI::Format::R9G9B9E5_SHAREDEXP:
                        return false;
                    default:
                        return true;
                }
            }
        
            bool IsBlendingSupported(id<MTLDevice> mtlDevice, RHI::Format format)
            {
                AZ_UNUSED(mtlDevice);
                //Assumes min spec of MTLGPUFamilyMac2
                return IsFormatAvailable(format);
            }
        
            bool IsMSAASupported(id<MTLDevice> mtlDevice, RHI::Format format)
            {
                AZ_UNUSED(mtlDevice);
                //Assumes min spec of MTLGPUFamilyMac2
                return IsFormatAvailable(format);
            }
        
            bool IsResolveTargetSupported(id<MTLDevice> mtlDevice, RHI::Format format)
            {
                AZ_UNUSED(mtlDevice);
                //Assumes min spec of MTLGPUFamilyMac2
                return IsFormatAvailable(format);
            }
        
            bool IsDepthStencilSupported(id<MTLDevice> mtlDevice, RHI::Format format)
            {
                if(!IsFormatAvailable(format))
                {
                    return false;
                }
                
                if(format == RHI::Format::D24_UNORM_S8_UINT)
                {
                    return mtlDevice.depth24Stencil8PixelFormatSupported;
                }                
                return true;
            }
        
            MTLBlitOption GetBlitOption(RHI::Format format)
            {
                return MTLBlitOptionNone;
            }
        
            MTLSamplerAddressMode ConvertAddressMode(RHI::AddressMode addressMode)
            {
                switch (addressMode)
                {
                    case RHI::AddressMode::MirrorOnce:
                        return MTLSamplerAddressModeMirrorClampToEdge;
                    case RHI::AddressMode::Border:
                        return MTLSamplerAddressModeClampToBorderColor;
                    default:
                        AZ_Assert(false, "Unsupported  addressMode in ConvertAddressMode");
                        return MTLSamplerAddressModeRepeat;
                }
            }
        
            MTLResourceOptions CovertStorageMode(MTLStorageMode storageMode)
            {
                switch(storageMode)
                {
                    case MTLStorageModeManaged:
                        return MTLResourceStorageModeManaged;
                    default:
                        AZ_Assert(false, "Type not supported");
                        return MTLResourceStorageModeShared;
                }
            }
        
            MTLStorageMode GetCPUGPUMemoryMode()
            {
                return  MTLStorageModeManaged;
            }
        
        } //Platform
    } // Metal
} // AZ
