/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : Implements the resource related functions

#include "RenderDll_precompiled.h"
#include "GLResource.hpp"
#include "METALContext.hpp"
#include "MetalDevice.hpp"
#include <DriverD3D.h>

namespace NCryMetal
{
    enum
    {
        MIN_MAPPED_RESOURCE_ALIGNMENT = 64, // DX10+ mapped resources are 16-aligned but GL_ARB_map_buffer_alignment ensures 64-alignment for AVX
    };

    MemRingBufferStorage GetMemAllocModeBasedOnSize(const size_t size)
    {
        MemRingBufferStorage memAllocMode = MEM_SHARED_RINGBUFFER;
#if defined(AZ_PLATFORM_MAC)
        if (size > FASTBUFFER_SIZE_THRESHHOLD)
        {
            memAllocMode = MEM_MANAGED_RINGBUFFER;
        }
#endif
        return memAllocMode;
    }

    id<MTLBuffer> GetMtlBufferBasedOnSize(const SBuffer* pBuffer)
    {
        if (!pBuffer)
        {
            return NULL;
        }
        id<MTLBuffer> buffer = pBuffer->m_BufferShared;
#if defined(AZ_PLATFORM_MAC)
        if (pBuffer->m_BufferManaged && GetMemAllocModeBasedOnSize(pBuffer->m_uMapSize) == MEM_MANAGED_RINGBUFFER)
        {
            buffer = pBuffer->m_BufferManaged;
        }
#endif
        return buffer;
    }

    uint32 GetRowPitch(uint32 uWidth, uint32 uRowBytes, const SGIFormatInfo* pFormatInfo)
    {
        uint32 uNumElementsPerRow(uRowBytes * pFormatInfo->m_pTexture->m_uBlockWidth / pFormatInfo->m_pTexture->m_uNumBlockBytes);
        return uNumElementsPerRow == uWidth ? 0 : uNumElementsPerRow;
    }

    uint32 GetImagePitch(uint32 uHeight, uint32 uImageBytes, uint32 uRowBytes)
    {
        uint32 uNumRowsPerImage(uImageBytes / uRowBytes);
        return uNumRowsPerImage == uHeight ? 0 : uNumRowsPerImage;
    }

    struct STexBox
    {
        STexPos m_kOffset;
        STexSize m_kSize;
    };

    struct SPackedLayout
    {
        uint32 m_uRowPitch;
        uint32 m_uImagePitch;
        uint32 m_uTextureSize;
    };

    static GLint GetMaxMipLevels(const D3D11_TEXTURE1D_DESC& kTexDesc)
    {
        return (GLint)IntegerLog2(kTexDesc.Width);
    }

    static GLint GetMaxMipLevels(const D3D11_TEXTURE2D_DESC& kTexDesc)
    {
        return (GLint)max(IntegerLog2(kTexDesc.Width), IntegerLog2(kTexDesc.Height));
    }

    static GLint GetMaxMipLevels(const D3D11_TEXTURE3D_DESC& kTexDesc)
    {
        return (GLint)max(max(IntegerLog2(kTexDesc.Width), IntegerLog2(kTexDesc.Height)), IntegerLog2(kTexDesc.Depth));
    }

    template <typename TextureDesc>
    static GLint GetNumMipLevels(const TextureDesc& kTexDesc)
    {
        return kTexDesc.MipLevels != 0 ? kTexDesc.MipLevels : GetMaxMipLevels(kTexDesc);
    }

    static STexSize GetMipSize(STexture* pTexture, GLint iLevel, const SGIFormatInfo* pFormat, bool bClampToBlockSize)
    {
        STexSize kMinSize(1, 1, 1);
        if (bClampToBlockSize && pFormat->m_pTexture->m_bCompressed)
        {
            kMinSize = STexSize(
                    pFormat->m_pTexture->m_uBlockWidth,
                    pFormat->m_pTexture->m_uBlockHeight,
                    pFormat->m_pTexture->m_uBlockDepth);
        }

        return STexSize(
            max(kMinSize.x, pTexture->m_iWidth >> iLevel),
            max(kMinSize.y, pTexture->m_iHeight >> iLevel),
            max(kMinSize.z, pTexture->m_iDepth >> iLevel));
    }

    void GetTextureBox(STexBox& kTexBox, STexture* pTexture, GLint iLevel, const SGIFormatInfo* pFormat, bool bClampToBlockSize)
    {
        kTexBox.m_kOffset = STexPos(0, 0, 0);
        kTexBox.m_kSize = GetMipSize(pTexture, iLevel, pFormat, bClampToBlockSize);
    }

    void GetTextureBox(STexBox& kTexBox, STexture* pTexture, GLint iLevel, const D3D11_BOX* pBox, const SGIFormatInfo* pFormat, bool bClampToBlockSize)
    {
        if (pBox != NULL)
        {
            kTexBox.m_kOffset = STexPos(pBox->left, pBox->top, pBox->front);
            kTexBox.m_kSize = STexSize(pBox->right - pBox->left, pBox->bottom - pBox->top, pBox->back - pBox->front);
        }
        else
        {
            GetTextureBox(kTexBox, pTexture, iLevel, pFormat, bClampToBlockSize);
        }
    }

    struct STex1DBase
    {
        static GLsizei GetBCImageSize(STexSize kSize, const STextureFormat* pTexFormat)
        {
            return pTexFormat->m_uNumBlockBytes *
                   ((kSize.x + pTexFormat->m_uBlockWidth - 1) / pTexFormat->m_uBlockWidth);
        }
    };

    struct STex2DBase
    {
        static GLsizei GetBCImageSize(STexSize kSize, const STextureFormat* pTexFormat)
        {
            return pTexFormat->m_uNumBlockBytes *
                   ((kSize.x + pTexFormat->m_uBlockWidth  - 1) / pTexFormat->m_uBlockWidth) *
                   ((kSize.y + pTexFormat->m_uBlockHeight - 1) / pTexFormat->m_uBlockHeight);
        }
    };

    struct STex3DBase
    {
        static GLsizei GetBCImageSize(STexSize kSize, const STextureFormat* pTexFormat)
        {
            return pTexFormat->m_uNumBlockBytes *
                   ((kSize.x + pTexFormat->m_uBlockWidth  - 1) / pTexFormat->m_uBlockWidth) *
                   ((kSize.y + pTexFormat->m_uBlockHeight - 1) / pTexFormat->m_uBlockHeight) *
                   ((kSize.z + pTexFormat->m_uBlockDepth  - 1) / pTexFormat->m_uBlockDepth);
        }
    };

    struct SDefaultTex1DBase
        : STex1DBase
    {
        static void TexStorage(STexture* pTexture, STexSize kSize, GLsizei iLevels, const SGIFormatInfo* pFormat, id<MTLDevice> mtlDevice, const uint32 uBindFlags)
        {
            assert(pFormat->m_pTexture);
            DXMETAL_NOT_IMPLEMENTED
        }
    };

    struct SDefaultTex2DBase
        : STex2DBase
    {
        typedef SDefaultTex1DBase TArrayElement;

        static void TexStorage(STexture* pTexture, STexSize kSize, GLsizei iLevels, const SGIFormatInfo* pFormat, id<MTLDevice> mtlDevice, const uint32 uBindFlags)
        {
            const STextureFormat* pTexFormat = pFormat->m_pTexture;
            assert(pTexFormat);
            //  Confetti BEGIN: Igor Lobanchikov
            bool bHasStecnilAttachment = false;
            MTLPixelFormat  eMetalFormat = pTexFormat->m_eMetalFormat;

            bool isDepthStencilBuffer = pFormat->m_eTypelessFormat == eGIF_R32G8X24_TYPELESS || pFormat->m_eDXGIFormat == DXGI_FORMAT_R32G8X24_TYPELESS || pFormat->m_eDXGIFormat == DXGI_FORMAT_R16_TYPELESS || pFormat->m_eDXGIFormat == DXGI_FORMAT_R32_TYPELESS ;
            
            //  Igor: Special handling for the texture which is actually 2 textures
            if (isDepthStencilBuffer)
            {
                bHasStecnilAttachment = true;
#if defined(AZ_PLATFORM_MAC)
                //The OSX_GPUFamily1_v1 feature set does not support separate depth and stencil render targets.
                //If these render targets are needed, use the following newly introduced depth/stencil pixel formats
                //to set the same texture as both the depth and stencil render target
                eMetalFormat = MTLPixelFormatDepth32Float_Stencil8;
#else
                eMetalFormat = MTLPixelFormatDepth32Float;
#endif
            }

            if (eMetalFormat != MTLPixelFormatInvalid)
            {
                MTLTextureDescriptor* Desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:eMetalFormat
                                              width:kSize.x
                                              height:kSize.y
                                              mipmapped:(iLevels > 1)];

                switch (pTexture->m_eTextureType)
                {
                case MTLTextureTypeCube:
                    assert(pTexture->m_uNumElements == 6);
                    Desc.textureType = MTLTextureTypeCube;
                    break;
                case MTLTextureType2D:
                    assert(pTexture->m_uNumElements == 1);
                    break;
                case MTLTextureType2DArray:
                    Desc.textureType = MTLTextureType2DArray;
                    Desc.arrayLength = pTexture->m_uNumElements;
                    break;
                default:
                    DXGL_NOT_IMPLEMENTED;
                }

                Desc.mipmapLevelCount = iLevels;
                
                if (uBindFlags & D3D11_BIND_RENDER_TARGET)
                {
                    Desc.usage = MTLTextureUsageRenderTarget;
                }
                
                if (uBindFlags & D3D11_BIND_SHADER_RESOURCE)
                {
                    Desc.usage |= MTLTextureUsageShaderRead;
                }
                
                
                if (isDepthStencilBuffer)
                {
                    //MTLStorageModePrivate on OS X makes it so that this resource is stored
                    //in video memory for the GPU.
                    Desc.storageMode = MTLStorageModePrivate;
                    //Depth stencil buffer gets written into and sampled from.
                    Desc.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;

#if defined(AZ_PLATFORM_MAC)
                    //On osx the depth/stencil texture is merged. You need to create a different
                    //texture view to access stencil data. Hence we need this flag.
                    Desc.usage |= MTLTextureUsagePixelFormatView;
#endif
                }
                else if(CTexture::IsDeviceFormatTypeless(pFormat->m_eDXGIFormat))
                {
                    //Apple recommendation -> For sRGB variant views, you don't need the PFV flag when: - running on
                    //iOS/tvOS 12.0 or newer - running on macOS 10.15 or newer
                    //However, on older OSs (and in macOS case, older GPUs) you are still required to set the flag.
#if defined(AZ_COMPILER_CLANG) && AZ_COMPILER_CLANG >= 9    //@available was added in Xcode 9
                    if (@available(macOS 10.15, iOS 12.0, *))
                    {
                        //No need to do anything but if we want to add non sRGB view related flags it would go here.
                    }
                    else
                    {
                        Desc.usage |= MTLTextureUsagePixelFormatView;
                    }
#endif
                }

                pTexture->m_Texture = [mtlDevice newTextureWithDescriptor:Desc];

                if (!pTexture->m_Texture)
                {
                    LOG_METAL_SHADER_ERRORS(@ "Failed to create texture: %@", Desc);
                }
                else if (isDepthStencilBuffer)
                {
#if defined(AZ_PLATFORM_MAC)
                    pTexture->m_StencilTexture = pTexture->m_Texture;
#else
                    Desc.pixelFormat = MTLPixelFormatStencil8;
                    pTexture->m_StencilTexture = [mtlDevice newTextureWithDescriptor: Desc];

                    if (!pTexture->m_StencilTexture)
                    {
                        LOG_METAL_SHADER_ERRORS(@ "Failed to create stencil attachment: %@", Desc);
                    }
#endif
                }
            }
            //  Confetti End: Igor Lobanchikov
        }

        template <typename T>
        static void SetLayerComponent(STexVec<T>& kVec, T kLayer)
        {
            kVec.y = kLayer;
        }
    };

    struct SDefaultTex3DBase
        : STex3DBase
    {
        typedef SDefaultTex2DBase TArrayElement;

        static void TexStorage(STexture* pTexture, STexSize kSize, GLsizei iLevels, const SGIFormatInfo* pFormat, id<MTLDevice> mtlDevice, const uint32 uBindFlags)
        {
            const STextureFormat* pTexFormat = pFormat->m_pTexture;
            assert(pTexFormat);
            //  Confetti BEGIN: Igor Lobanchikov
            MTLPixelFormat  eMetalFormat = pTexFormat->m_eMetalFormat;

            assert(pTexture->m_eTextureType == MTLTextureType3D || pTexture->m_eTextureType == MTLTextureType2DArray);
            assert(pTexture->m_uNumElements == 1 || pTexture->m_eTextureType != MTLTextureType3D);

            if (eMetalFormat != MTLPixelFormatInvalid)
            {
                MTLTextureDescriptor* Desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:eMetalFormat
                                              width:kSize.x
                                              height:kSize.y
                                              mipmapped:(iLevels > 1)];
                Desc.depth = kSize.z;
                Desc.textureType = MTLTextureType3D;
                Desc.mipmapLevelCount = iLevels;

                pTexture->m_Texture = [mtlDevice newTextureWithDescriptor:Desc];

                if (!pTexture->m_Texture)
                {
                    LOG_METAL_SHADER_ERRORS(@ "Failed to create texture: %@", Desc);
                }
            }
            //  Confetti End: Igor Lobanchikov
        }


        template <typename T>
        static void SetLayerComponent(STexVec<T>& kVec, T kLayer)
        {
            kVec.z = kLayer;
        }
    };

    struct STexCompressed
    {
        static bool GetPackedRange(const STexBox& kPixels, STexBox* pPackedRange, const SGIFormatInfo* pFormat)
        {
            const STextureFormat* pTexFormat(pFormat->m_pTexture);
            if ((kPixels.m_kOffset.x % pTexFormat->m_uBlockWidth)  != 0 ||
                (kPixels.m_kOffset.y % pTexFormat->m_uBlockHeight) != 0 ||
                (kPixels.m_kOffset.z % pTexFormat->m_uBlockDepth)  != 0 ||
                (kPixels.m_kSize.x   % pTexFormat->m_uBlockWidth)  != 0 ||
                (kPixels.m_kSize.y   % pTexFormat->m_uBlockHeight) != 0 ||
                (kPixels.m_kSize.z   % pTexFormat->m_uBlockDepth)  != 0)
            {
                return false;
            }

            pPackedRange->m_kOffset.x = pTexFormat->m_uNumBlockBytes * kPixels.m_kOffset.x / pTexFormat->m_uBlockWidth;
            pPackedRange->m_kSize.x   = pTexFormat->m_uNumBlockBytes * kPixels.m_kSize.x   / pTexFormat->m_uBlockWidth;

            pPackedRange->m_kOffset.y = kPixels.m_kOffset.y / pTexFormat->m_uBlockHeight;
            pPackedRange->m_kSize.y   = kPixels.m_kSize.y   / pTexFormat->m_uBlockHeight;

            pPackedRange->m_kOffset.z = kPixels.m_kOffset.z / pTexFormat->m_uBlockDepth;
            pPackedRange->m_kSize.z   = kPixels.m_kSize.z   / pTexFormat->m_uBlockDepth;
            return true;
        }
    };

    struct STexUncompressed
    {
        static bool GetPackedRange(const STexBox& kPixels, STexBox* pPackedRange, const SGIFormatInfo* pFormat)
        {
            uint32 uPixelBytes(pFormat->m_pTexture->m_uNumBlockBytes);

            pPackedRange->m_kOffset.x = kPixels.m_kOffset.x * uPixelBytes;
            pPackedRange->m_kSize.x   = kPixels.m_kSize.x   * uPixelBytes;

            pPackedRange->m_kOffset.y = kPixels.m_kOffset.y;
            pPackedRange->m_kSize.y   = kPixels.m_kSize.y;

            pPackedRange->m_kOffset.z = kPixels.m_kOffset.z;
            pPackedRange->m_kSize.z   = kPixels.m_kSize.z;
            return true;
        }
    };

    struct STex1DUncompressed
        : SDefaultTex1DBase
        , STexUncompressed
    {
        //  Confetti BEGIN: Igor Lobanchikov
        static void TexSubImage(STexture* pTexture, uint32 slice, GLint iLevel, STexBox kBox, const STextureFormat* pTexFormat, const GLvoid* pData, uint32 uRowPitch, uint32 uImagePitch)
        {
            MTLRegion Region = MTLRegionMake3D(kBox.m_kOffset.x, 0, 0, kBox.m_kSize.x, 1, 1);
            assert(kBox.m_kSize.y < 2);

            [pTexture->m_Texture replaceRegion: Region mipmapLevel: iLevel slice: slice withBytes: pData bytesPerRow: uRowPitch bytesPerImage: uImagePitch];
        }
        //  Confetti End: Igor Lobanchikov

        static void GetPackedLayout(STexSize kRect, const SGIFormatInfo* pFormat, SPackedLayout* pLayout)
        {
            pLayout->m_uRowPitch    = kRect.x * pFormat->m_pTexture->m_uNumBlockBytes;
            pLayout->m_uImagePitch  = pLayout->m_uRowPitch;
            pLayout->m_uTextureSize = pLayout->m_uRowPitch;
        }
    };

    struct STex2DUncompressed
        : SDefaultTex2DBase
        , STexUncompressed
    {
        //  Confetti BEGIN: Igor Lobanchikov
        static void TexSubImage(STexture* pTexture, uint32 slice, GLint iLevel, STexBox kBox, const STextureFormat* pTexFormat, const GLvoid* pData, uint32 uRowPitch, uint32 uImagePitch)
        {
            MTLRegion Region = MTLRegionMake3D(kBox.m_kOffset.x, kBox.m_kOffset.y, 0, kBox.m_kSize.x, kBox.m_kSize.y, 1);
            assert(kBox.m_kSize.z < 2);

            [pTexture->m_Texture replaceRegion: Region mipmapLevel: iLevel slice: slice withBytes: pData bytesPerRow: uRowPitch bytesPerImage: uImagePitch];
        }
        //  Confetti End: Igor Lobanchikov

        static void GetPackedLayout(STexSize kRect, const SGIFormatInfo* pFormat, SPackedLayout* pLayout)
        {
            pLayout->m_uRowPitch    = kRect.x * pFormat->m_pTexture->m_uNumBlockBytes;
            pLayout->m_uImagePitch  = kRect.y * pLayout->m_uRowPitch;
            pLayout->m_uTextureSize = pLayout->m_uImagePitch;
        }
    };

    struct STex3DUncompressed
        : SDefaultTex3DBase
        , STexUncompressed
    {
        //  Confetti BEGIN: Igor Lobanchikov
        static void TexSubImage(STexture* pTexture, uint32 slice, GLint iLevel, STexBox kBox, const STextureFormat* pTexFormat, const GLvoid* pData, uint32 uRowPitch, uint32 uImagePitch)
        {
            MTLRegion Region = MTLRegionMake3D(kBox.m_kOffset.x, kBox.m_kOffset.y, kBox.m_kOffset.z, kBox.m_kSize.x, kBox.m_kSize.y, kBox.m_kSize.z);
            assert(slice == 0);

            [pTexture->m_Texture replaceRegion: Region mipmapLevel: iLevel slice: slice withBytes: pData bytesPerRow: uRowPitch bytesPerImage: uImagePitch];
        }
        //  Confetti End: Igor Lobanchikov

        static void GetPackedLayout(STexSize kRect, const SGIFormatInfo* pFormat, SPackedLayout* pLayout)
        {
            pLayout->m_uRowPitch    = kRect.x * pFormat->m_pTexture->m_uNumBlockBytes;
            pLayout->m_uImagePitch  = kRect.y * pLayout->m_uRowPitch;
            pLayout->m_uTextureSize = kRect.z * pLayout->m_uImagePitch;
        }
    };

    struct STex1DCompressed
        : SDefaultTex1DBase
        , STexCompressed
    {
        //  Confetti BEGIN: Igor Lobanchikov
        static void TexSubImage(STexture* pTexture, uint32 slice, GLint iLevel, STexBox kBox, const STextureFormat* pTexFormat, const GLvoid* pData, uint32 uRowPitch, uint32 uImagePitch)
        {
            MTLRegion Region = MTLRegionMake3D(kBox.m_kOffset.x, 0, 0, kBox.m_kSize.x, 1, 1);
            assert(kBox.m_kSize.y < 2);

            //MTLPixelFormatPVRTC_RGB_2BPP/MTLPixelFormatPVRTC_RGBA_4BPP_sRGB not supported on OSX GPUs
#if !defined AZ_PLATFORM_MAC
            //  Igor: Metal requires these to be 0 for PVR formats
            if (pTexFormat->m_eMetalFormat >= MTLPixelFormatPVRTC_RGB_2BPP && pTexFormat->m_eMetalFormat <= MTLPixelFormatPVRTC_RGBA_4BPP_sRGB)
            {
                uRowPitch = 0;
                uImagePitch = 0;
            }
#endif

            [pTexture->m_Texture replaceRegion: Region mipmapLevel: iLevel slice: slice withBytes: pData bytesPerRow: uRowPitch bytesPerImage: uImagePitch];
        }
        //  Confetti End: Igor Lobanchikov

        static void GetPackedLayout(STexSize kRect, const SGIFormatInfo* pFormat, SPackedLayout* pLayout)
        {
            pLayout->m_uRowPitch    = GetBCImageSize(kRect, pFormat->m_pTexture);
            pLayout->m_uImagePitch  = pLayout->m_uRowPitch;
            pLayout->m_uTextureSize = pLayout->m_uRowPitch;
        }
    };

    struct STex2DCompressed
        : SDefaultTex2DBase
        , STexCompressed
    {
        //  Confetti BEGIN: Igor Lobanchikov
        static void TexSubImage(STexture* pTexture, uint32 slice, GLint iLevel, STexBox kBox, const STextureFormat* pTexFormat, const GLvoid* pData, uint32 uRowPitch, uint32 uImagePitch)
        {
            MTLRegion Region = MTLRegionMake3D(kBox.m_kOffset.x, kBox.m_kOffset.y, 0, kBox.m_kSize.x, kBox.m_kSize.y, 1);
            assert(kBox.m_kSize.z < 2);

            //MTLPixelFormatPVRTC_RGB_2BPP/MTLPixelFormatPVRTC_RGBA_4BPP_sRGB not supported on OSX GPUs
#if !defined AZ_PLATFORM_MAC
            //  Igor: Metal requires these to be 0 for PVR formats
            if (pTexFormat->m_eMetalFormat >= MTLPixelFormatPVRTC_RGB_2BPP && pTexFormat->m_eMetalFormat <= MTLPixelFormatPVRTC_RGBA_4BPP_sRGB)
            {
                uRowPitch = 0;
                uImagePitch = 0;
            }
#endif
            [pTexture->m_Texture replaceRegion: Region mipmapLevel: iLevel slice: slice withBytes: pData bytesPerRow: uRowPitch bytesPerImage: uImagePitch];
        }
        //  Confetti End: Igor Lobanchikov

        static void GetPackedLayout(STexSize kRect, const SGIFormatInfo* pFormat, SPackedLayout* pLayout)
        {
            pLayout->m_uRowPitch    = GetBCImageSize(STexSize(kRect.x, 1, 1), pFormat->m_pTexture);
            pLayout->m_uImagePitch  = GetBCImageSize(kRect, pFormat->m_pTexture);
            pLayout->m_uTextureSize = pLayout->m_uImagePitch;
        }
    };

    struct STex3DCompressed
        : SDefaultTex3DBase
        , STexCompressed
    {
        //  Confetti BEGIN: Igor Lobanchikov
        static void TexSubImage(STexture* pTexture, uint32 slice, GLint iLevel, STexBox kBox, const STextureFormat* pTexFormat, const GLvoid* pData, uint32 uRowPitch, uint32 uImagePitch)
        {
            MTLRegion Region = MTLRegionMake3D(kBox.m_kOffset.x, kBox.m_kOffset.y, kBox.m_kOffset.z, kBox.m_kSize.x, kBox.m_kSize.y, kBox.m_kSize.z);
            assert(slice == 0);

            //MTLPixelFormatPVRTC_RGB_2BPP/MTLPixelFormatPVRTC_RGBA_4BPP_sRGB not supported on OSX GPUs
#if !defined AZ_PLATFORM_MAC
            //  Igor: Metal requires these to be 0 for PVR formats
            if (pTexFormat->m_eMetalFormat >= MTLPixelFormatPVRTC_RGB_2BPP && pTexFormat->m_eMetalFormat <= MTLPixelFormatPVRTC_RGBA_4BPP_sRGB)
            {
                uRowPitch = 0;
                uImagePitch = 0;
            }
#endif

            [pTexture->m_Texture replaceRegion: Region mipmapLevel: iLevel slice: slice withBytes: pData bytesPerRow: uRowPitch bytesPerImage: uImagePitch];
        }
        //  Confetti End: Igor Lobanchikov

        static void GetPackedLayout(STexSize kRect, const SGIFormatInfo* pFormat, SPackedLayout* pLayout)
        {
            pLayout->m_uRowPitch    = GetBCImageSize(STexSize(kRect.x, 1, 1), pFormat->m_pTexture);
            pLayout->m_uImagePitch  = GetBCImageSize(STexSize(kRect.x, kRect.y, 1), pFormat->m_pTexture);
            pLayout->m_uTextureSize = GetBCImageSize(kRect, pFormat->m_pTexture);
        }
    };

    template <typename Interface>
    struct SSingleTexImpl
        : Interface
    {
        static void InitializeStorage(STexture* pTexture, uint32, const SGIFormatInfo* pFormat, CDevice* pDevice, const uint32 uBindFlags)
        {
            Interface::TexStorage(pTexture, GetMipSize(pTexture, 0, pFormat, false), pTexture->m_uNumMipLevels, pFormat, pDevice->GetMetalDevice(), uBindFlags);
        }

        static void UploadImage(STexture* pTexture, STexSubresourceID kSubID, STexBox kBox, const void* pSrcData, uint32 uSrcRowPitch, uint32 uSrcDepthPitch, CContext* pContext, const SGIFormatInfo* pFormat)
        {
            assert((kSubID.m_uElement == 0) ||
                (pTexture->m_eTextureType == MTLTextureTypeCube && kSubID.m_uElement < 6));

            //  Confetti BEGIN: Igor Lobanchikov
            Interface::TexSubImage(pTexture, kSubID.m_uElement, kSubID.m_iMipLevel, kBox, pFormat->m_pTexture, pSrcData, uSrcRowPitch, uSrcDepthPitch);
            //  Confetti End: Igor Lobanchikov
        }

        static void DownloadImage(STexture* pTexture, STexSubresourceID kSubID, STexBox kBox, void* pDstData, uint32 uDstRowPitch, uint32 uDstDepthPitch, CContext* pContext, const SGIFormatInfo* pFormat)
        {
            DXMETAL_NOT_IMPLEMENTED
        }

        static void Map(STexture* pTexture, STexSubresourceID kSubID, bool bDownload, SMappedSubTexture& kMappedSubTex, CContext* pContext, const SGIFormatInfo* pFormat)
        {
            STexBox kBox;
            GetTextureBox(kBox, pTexture, kSubID.m_iMipLevel, pFormat, true);
            SPackedLayout kPackedLayout;
            Interface::GetPackedLayout(kBox.m_kSize, pFormat, &kPackedLayout);

            DXGL_TODO("Check if it's worth to keep an allocation pool");
            kMappedSubTex.m_pBuffer = static_cast<uint8*>(Memalign(kPackedLayout.m_uTextureSize, MIN_MAPPED_RESOURCE_ALIGNMENT));
            kMappedSubTex.m_uRowPitch = kPackedLayout.m_uRowPitch;
            kMappedSubTex.m_uImagePitch = kPackedLayout.m_uImagePitch;
            kMappedSubTex.m_uDataOffset = 0;

            if (bDownload)
            {
                DownloadImage(pTexture, kSubID, kBox, kMappedSubTex.m_pBuffer, kPackedLayout.m_uRowPitch, kPackedLayout.m_uImagePitch, pContext, pFormat);
            }
        }

        static void Unmap(STexture* pTexture, STexSubresourceID kSubID, const SMappedSubTexture& kMappedSubTex, CContext* pContext, const SGIFormatInfo* pFormat)
        {
            if (kMappedSubTex.m_bUpload)
            {
                STexBox kBox;
                GetTextureBox(kBox, pTexture, kSubID.m_iMipLevel, pFormat, true);

                UploadImage(pTexture, kSubID, kBox, kMappedSubTex.m_pBuffer, kMappedSubTex.m_uRowPitch, kMappedSubTex.m_uImagePitch, pContext, pFormat);
            }

            MemalignFree(kMappedSubTex.m_pBuffer);
        }
    };

    template <typename Interface>
    struct SArrayTexImpl
        : Interface
    {
        static void InitializeStorage(STexture* pTexture, uint32, const SGIFormatInfo* pFormat, CDevice* pDevice, const uint32 uBindFlags)
        {
            STexSize kTexSize(GetMipSize(pTexture, 0, pFormat, false));
            Interface::SetLayerComponent(kTexSize, (GLsizei)pTexture->m_uNumElements);
            Interface::TexStorage(pTexture, kTexSize, pTexture->m_uNumMipLevels, pFormat, pDevice->GetMetalDevice(), uBindFlags);
        }

        static void UploadImage(STexture* pTexture, STexSubresourceID kSubID, STexBox kBox, const void* pSrcData, uint32 uSrcRowPitch, uint32 uSrcDepthPitch, CContext* pContext, const SGIFormatInfo* pFormat)
        {
            assert(pTexture->m_eTextureType != MTLTextureTypeCube);

            Interface::SetLayerComponent(kBox.m_kOffset, (int)kSubID.m_uElement);
            Interface::SetLayerComponent(kBox.m_kSize, 1);

            //  Confetti BEGIN: Igor Lobanchikov
            Interface::TexSubImage(pTexture, kSubID.m_uElement, kSubID.m_iMipLevel, kBox, pFormat->m_pTexture, pSrcData, uSrcRowPitch, uSrcDepthPitch);
            //  Confetti End: Igor Lobanchikov
        }

        static void DownloadImage(STexture* pTexture, STexSubresourceID kSubID, STexBox kBox, void* pDstData, uint32 uDstRowPitch, uint32 uDstDepthPitch, CContext* pContext, const SGIFormatInfo* pFormat)
        {
            DXGL_NOT_IMPLEMENTED
        }

        static void Map(STexture* pTexture, STexSubresourceID kSubID, bool bDownload, SMappedSubTexture& kMappedSubTex, CContext* pContext, const SGIFormatInfo* pFormat)
        {
            DXMETAL_NOT_IMPLEMENTED
        }

        static void Unmap(STexture* pTexture, STexSubresourceID kSubID, const SMappedSubTexture& kMappedSubTex, CContext* pContext, const SGIFormatInfo* pFormat)
        {
            DXMETAL_NOT_IMPLEMENTED
        }
    };

    template <typename Interface>
    uint32 GetSystemMemoryTextureOffset(STexture* pTexture, const SGIFormatInfo* pFormat, STexSubresourceID kID)
    {
        uint32 uOffset(0);
        uint32 uTotSize(0);
        uint32 uLevel;
        for (uLevel = 0; uLevel < pTexture->m_uNumMipLevels; ++uLevel)
        {
            STexSize kLevelSize(GetMipSize(pTexture, (GLint)uLevel, pFormat, true));

            SPackedLayout kPackedLayout;
            Interface::GetPackedLayout(kLevelSize, pFormat, &kPackedLayout);

            uTotSize += kPackedLayout.m_uTextureSize;

            // Keep every subresource aligned so that it can be directly mapped
            uTotSize += MIN_MAPPED_RESOURCE_ALIGNMENT - 1;
            uTotSize -= (uTotSize % MIN_MAPPED_RESOURCE_ALIGNMENT);
            if (uLevel < kID.m_iMipLevel)
            {
                uOffset = uTotSize;
            }
        }

        return uTotSize * kID.m_uElement + uOffset;
    }

    template <typename Interface>
    struct SStagingTexImpl
        : Interface
    {
        static void InitializeStorage(STexture* pTexture, uint32 uCPUAccess, const SGIFormatInfo* pFormat, CDevice* pDevice, const uint32 uBindFlags)
        {
            Interface::TexStorage(pTexture, GetMipSize(pTexture, 0, pFormat, false), pTexture->m_uNumMipLevels, pFormat, pDevice->GetMetalDevice(), uBindFlags);
            {
                STexSubresourceID kEndID = {aznumeric_cast<int32>(pTexture->m_uNumMipLevels),
                                            static_cast<uint32>(pTexture->m_uNumElements)};
                uint32 uMappedSize(GetSystemMemoryTextureOffset<Interface>(pTexture, pFormat, kEndID));
                pTexture->m_pMapMemoryCopy = static_cast<uint8*>(Memalign(uMappedSize, MIN_MAPPED_RESOURCE_ALIGNMENT));
            }
        }

        static void UploadImage(STexture* pTexture, STexSubresourceID kSubID, STexBox kBox, const void* pSrcData, uint32 uSrcRowPitch, uint32 uSrcDepthPitch, CContext* pContext, const SGIFormatInfo* pFormat)
        {
            DXMETAL_NOT_IMPLEMENTED
        }

        static void DownloadImage(STexture* pTexture, STexSubresourceID kSubID, STexBox kBox, const void* pDstData, uint32 uSrcRowPitch, uint32 uSrcDepthPitch, CContext* pContext, const SGIFormatInfo* pFormat)
        {
            DXGL_NOT_IMPLEMENTED
        }

        static void Map(STexture* pTexture, STexSubresourceID kSubID, bool bDownload, SMappedSubTexture& kMappedSubTex, CContext* pContext, const SGIFormatInfo* pFormat)
        {
            STexSize kSubSize(GetMipSize(pTexture, kSubID.m_iMipLevel, pFormat, true));
            SPackedLayout kPackedLayout;
            Interface::GetPackedLayout(kSubSize, pFormat, &kPackedLayout);

            kMappedSubTex.m_pBuffer = pTexture->m_pMapMemoryCopy + GetSystemMemoryTextureOffset<Interface>(pTexture, pFormat, kSubID);
            if (bDownload)
            {
                static ICVar* isScreenshot = gEnv->pConsole->GetCVar("e_ScreenShot");
                static ICVar* isCaptureFrame = gEnv->pConsole->GetCVar("capture_frames");
                if (isScreenshot->GetIVal() || isCaptureFrame->GetIVal())
                {
                    //This will stall the GPU so be very careful when using it. Only use it when you absolutely need the
                    //work encoded by the current comman buffer.
                    pContext->FlushBlitEncoderAndWait();
                }
                
                MTLRegion region = {
                    {0, 0, 0}, {pTexture->m_Texture.width, pTexture->m_Texture.height, pTexture->m_Texture.depth}
                };
                [pTexture->m_Texture getBytes: kMappedSubTex.m_pBuffer
                                  bytesPerRow: kPackedLayout.m_uRowPitch
                                bytesPerImage: kPackedLayout.m_uImagePitch
                                   fromRegion: region
                                  mipmapLevel: kSubID.m_iMipLevel
                                        slice: kSubID.m_uElement];
            }

            kMappedSubTex.m_uRowPitch = kPackedLayout.m_uRowPitch;
            kMappedSubTex.m_uImagePitch = kPackedLayout.m_uImagePitch;
            kMappedSubTex.m_uDataOffset = 0;
        }

        static void Unmap(STexture* pTexture, STexSubresourceID kSubID, const SMappedSubTexture& kMappedSubTex, CContext*, const SGIFormatInfo*)
        {
            if (kMappedSubTex.m_bUpload)
            {
                MTLRegion region = {
                    {0, 0, 0}, {pTexture->m_Texture.width, pTexture->m_Texture.height, pTexture->m_Texture.depth}
                };
                [pTexture->m_Texture replaceRegion: region
                                       mipmapLevel: kSubID.m_iMipLevel
                                             slice: kSubID.m_uElement
                                         withBytes: kMappedSubTex.m_pBuffer
                                       bytesPerRow: kMappedSubTex.m_uRowPitch
                                     bytesPerImage: kMappedSubTex.m_uImagePitch];
            }
        }
    };

    inline STexSubresourceID GetTexSubresourceID(STexture* pTexture, uint32 uSubresource)
    {
        STexSubresourceID kID;
        kID.m_iMipLevel = uSubresource % pTexture->m_uNumMipLevels;
        kID.m_uElement = uSubresource / pTexture->m_uNumMipLevels;
        assert(kID.m_uElement < pTexture->m_uNumElements);
        return kID;
    }

    template <typename Impl>
    void UpdateTexSubresource(SResource* pResource, uint32 uSubresource, const D3D11_BOX* pDstBox, const void* pSrcData, uint32 uSrcRowPitch, uint32 uSrcDepthPitch, CContext* pContext)
    {
        DXGL_SCOPED_PROFILE("UpdateTexSubresource")

        STexture * pTexture(static_cast<STexture*>(pResource));

        const SGIFormatInfo* pFormatInfo(GetGIFormatInfo(pTexture->m_eFormat));
        assert(pFormatInfo != NULL);
        assert(pFormatInfo->m_pTexture != NULL);

        STexSubresourceID kSubID(GetTexSubresourceID(pTexture, uSubresource));

        STexBox kTexBox;
        GetTextureBox(kTexBox, pTexture, kSubID.m_iMipLevel, pDstBox, pFormatInfo, false);
        Impl::UploadImage(pTexture, kSubID, kTexBox, pSrcData, uSrcRowPitch, uSrcDepthPitch, pContext, pFormatInfo);
    }

    template <typename Impl>
    bool MapTexSubresource(SResource* pResource, uint32 uSubresource, D3D11_MAP eMapType, uint32 uMapFlags, D3D11_MAPPED_SUBRESOURCE* pMappedResource, CContext* pContext)
    {
        DXGL_SCOPED_PROFILE("MapTexSubresource")
        
        STexture * pTexture(static_cast<STexture*>(pResource));

        const SGIFormatInfo* pFormatInfo(GetGIFormatInfo(pTexture->m_eFormat));
        assert(pFormatInfo != NULL);
        assert(pFormatInfo->m_pTexture != NULL);

        if (uSubresource >= pTexture->m_kMappedSubTextures.size())
        {
            pTexture->m_kMappedSubTextures.resize(uSubresource + 1);
        }
        SMappedSubTexture& kMappedSubTexture(pTexture->m_kMappedSubTextures.at(uSubresource));

        if (kMappedSubTexture.m_pBuffer != NULL)
        {
            DXGL_ERROR("Texture subresource is already mapped");
            return false;
        }

        bool bDownload = (eMapType == D3D11_MAP_READ || eMapType == D3D11_MAP_READ_WRITE);
        Impl::Map(pTexture, GetTexSubresourceID(pTexture, uSubresource), bDownload, kMappedSubTexture, pContext, pFormatInfo);
        kMappedSubTexture.m_bUpload = (eMapType != D3D11_MAP_READ);

        pMappedResource->pData = kMappedSubTexture.m_pBuffer + kMappedSubTexture.m_uDataOffset;
        pMappedResource->RowPitch = kMappedSubTexture.m_uRowPitch;
        pMappedResource->DepthPitch = kMappedSubTexture.m_uImagePitch;

        return kMappedSubTexture.m_pBuffer != NULL;
    }

    template <typename Impl>
    void UnmapTexSubresource(SResource* pResource, UINT uSubresource, CContext* pContext)
    {
        DXGL_SCOPED_PROFILE("UnmapTexSubresource")

        STexture * pTexture(static_cast<STexture*>(pResource));

        const SGIFormatInfo* pFormatInfo(GetGIFormatInfo(pTexture->m_eFormat));
        assert(pFormatInfo != NULL);
        assert(pFormatInfo->m_pTexture != NULL);

        if (uSubresource >= pTexture->m_kMappedSubTextures.size())
        {
            pTexture->m_kMappedSubTextures.resize(uSubresource + 1);
        }
        SMappedSubTexture& kMappedSubTexture(pTexture->m_kMappedSubTextures.at(uSubresource));

        if (kMappedSubTexture.m_pBuffer == NULL)
        {
            DXGL_ERROR("Texture subresource is not mapped");
            return;
        }

        Impl::Unmap(pTexture, GetTexSubresourceID(pTexture, uSubresource), kMappedSubTexture, pContext, pFormatInfo);
        kMappedSubTexture.m_pBuffer = NULL;
    }

    template <typename Impl>
    void UnpackTexData(STexture* pTexture, STexSubresourceID kSubID, STexPos kOffset, STexSize kSize, const SMappedSubTexture& kDataLocation, CContext* pContext)
    {
        DXGL_SCOPED_PROFILE("UnpackTexData")

        STexBox kBox;
        kBox.m_kOffset = kOffset;
        kBox.m_kSize = kSize;
        const SGIFormatInfo* pFormat(GetGIFormatInfo(pTexture->m_eFormat));

        Impl::UploadImage(pTexture, kSubID, kBox, kDataLocation.m_pBuffer + kDataLocation.m_uDataOffset, kDataLocation.m_uRowPitch, kDataLocation.m_uImagePitch, pContext, pFormat);
    }

    template <typename Impl>
    void PackTexData(STexture* pTexture, STexSubresourceID kSubID, STexPos kOffset, STexSize kSize, const SMappedSubTexture& kDataLocation, CContext* pContext)
    {
        DXGL_SCOPED_PROFILE("PackTexData")

        STexBox kBox;
        kBox.m_kOffset = kOffset;
        kBox.m_kSize = kSize;
        const SGIFormatInfo* pFormat(GetGIFormatInfo(pTexture->m_eFormat));

        Impl::DownloadImage(pTexture, kSubID, kBox, kDataLocation.m_pBuffer + kDataLocation.m_uDataOffset, kDataLocation.m_uRowPitch, kDataLocation.m_uImagePitch, pContext, pFormat);
    }

    template <typename Impl>
    void InitializeTexture(STexture* pTexture, const D3D11_SUBRESOURCE_DATA* pInitialData, uint32 uCPUAccess, CDevice* pDevice /*CContext* pContext*/, const SGIFormatInfo* pFormatInfo, const uint32 uBindFlags)
    {
        pTexture->m_pfUpdateSubresource = &UpdateTexSubresource<Impl>;
        pTexture->m_pfMapSubresource = &MapTexSubresource<Impl>;
        pTexture->m_pfUnmapSubresource = &UnmapTexSubresource<Impl>;

        Impl::InitializeStorage(pTexture, uCPUAccess, pFormatInfo, pDevice, uBindFlags);

        if (pInitialData)
        {
            STexBox kMipBox;
            kMipBox.m_kOffset = STexPos(0, 0, 0);

            STexSubresourceID kSubID;
            for (kSubID.m_uElement = 0; kSubID.m_uElement < pTexture->m_uNumElements; ++kSubID.m_uElement)
            {
                for (kSubID.m_iMipLevel = 0; kSubID.m_iMipLevel < (GLint)pTexture->m_uNumMipLevels; ++kSubID.m_iMipLevel)
                {
                    kMipBox.m_kSize = GetMipSize(pTexture, kSubID.m_iMipLevel, pFormatInfo, false);

                    Impl::UploadImage(pTexture, kSubID, kMipBox, pInitialData->pSysMem, pInitialData->SysMemPitch, pInitialData->SysMemSlicePitch, /*pContext*/ 0, pFormatInfo);

                    ++pInitialData;
                }
            }
        }
    }

    SResource::SResource()
        : m_pfUpdateSubresource(NULL)
        , m_pfMapSubresource(NULL)
        , m_pfUnmapSubresource(NULL)
    {
    }

    SResource::SResource(const SResource& kOther)
        : m_pfUpdateSubresource(kOther.m_pfUpdateSubresource)
        , m_pfMapSubresource(kOther.m_pfMapSubresource)
        , m_pfUnmapSubresource(kOther.m_pfUnmapSubresource)
    {
    }

    SResource::~SResource()
    {
    }

    STexture::STexture(GLsizei iWidth, GLsizei iHeight, GLsizei iDepth, MTLTextureType eTextureType, EGIFormat eFormat, uint32 uNumMipLevels, uint32 uNumElements)
        : m_eTextureType(eTextureType)
        , m_eFormat(eFormat)
        , m_uNumMipLevels(uNumMipLevels)
        , m_uNumElements(uNumElements)
        , m_iWidth(iWidth)
        , m_iHeight(iHeight)
        , m_iDepth(iDepth)
        , m_pShaderViewsHead(NULL)
        , m_pOutputMergerViewsHead(NULL)
        , m_pBoundModifier(NULL)
        //  Confetti BEGIN: Igor Lobanchikov
        , m_Texture(nil)
        , m_StencilTexture(nil)
        , m_bClearDepth(false)
        , m_bClearStencil(false)
        , m_bBackBuffer(false)
        , m_pMapMemoryCopy(NULL)
        //  Confetti End: Igor Lobanchikov
    {
#if DXGL_FULL_EMULATION
        m_uNumElements = max(1u, m_uNumElements);
#endif //DXGL_FULL_EMULATION
        ResetDontCareActionFlags();
    }

    STexture::~STexture()
    {
        //  Confetti BEGIN: Igor Lobanchikov
        if (m_pMapMemoryCopy)
        {
            MemalignFree(m_pMapMemoryCopy);
        }

        if (m_Texture)
        {
            [m_Texture release];
        }

        bool isDepthStencilTexSeparate = m_StencilTexture != m_Texture;
        if (m_StencilTexture && isDepthStencilTexSeparate)
        {
            [m_StencilTexture release];
        }
        //  Confetti End: Igor Lobanchikov
    }

    SShaderTextureViewPtr STexture::CreateShaderView(const SShaderTextureViewConfiguration& kConfiguration, CDevice* pDevice)
    {
        DXGL_TODO("This is not thread-safe, as multiple threads can create shader views for the same texture. Add synchronization primitive.");

        SShaderTextureViewPtr spView(new SShaderTextureView(this, kConfiguration));
        if (!spView->Init(pDevice))
        {
            return NULL;
        }
        return spView;
    }

    SShaderBufferViewPtr SBuffer::CreateShaderView(const SShaderBufferViewConfiguration& kConfiguration, CDevice* pDevice)
    {
        DXGL_TODO("This is not thread-safe, as multiple threads can create shader views for the same buffer. Add synchronization primitive.");

        SShaderBufferViewPtr spView(new SShaderBufferView(this, kConfiguration));
        if (!spView->Init(pDevice))
        {
            return NULL;
        }
        return spView;
    }

    SOutputMergerTextureViewPtr STexture::CreateOutputMergerView(const SOutputMergerTextureViewConfiguration& kConfiguration, CDevice* pDevice)
    {
        SOutputMergerTextureViewPtr spView(new SOutputMergerTextureView(this, kConfiguration));
        if (!spView->Init(pDevice))
        {
            return NULL;
        }
        return spView;
    }

    SOutputMergerTextureViewPtr STexture::GetCompatibleOutputMergerView(const SOutputMergerTextureViewConfiguration& kConfiguration, CDevice* pDevice)
    {
        DXGL_TODO("This is not thread-safe, as multiple threads can create output merger views for the same texture. Add synchronization primitive.");

        SOutputMergerTextureView* pExistingView(m_pOutputMergerViewsHead);
        while (pExistingView != NULL)
        {
            if (pExistingView->m_kConfiguration == kConfiguration)
            {
                return pExistingView;
            }
            pExistingView = pExistingView->m_pNextView;
        }

        return CreateOutputMergerView(kConfiguration, pDevice);
    }

    void STexture::ResetDontCareActionFlags()
    {
        m_bColorLoadDontCare =
            m_bDepthLoadDontCare =
                m_bStencilLoadDontCare =
                    m_bColorStoreDontCare =
                        m_bDepthStoreDontCare =
                            m_bStencilStoreDontCare = false;
    }


    SShaderResourceView::SShaderResourceView(EGIFormat eFormat)
        : m_eFormat(eFormat)
    {
    }

    SShaderResourceView::~SShaderResourceView()
    {
    }

    bool SShaderResourceView::GenerateMipmaps(CContext*)
    {
        DXGL_ERROR("Cannot create mipmaps from a generic shader resource view");
        return false;
    }

    SShaderBufferView::SShaderBufferView(SBuffer* pBuffer, const SShaderBufferViewConfiguration& kConfiguration)
        : SShaderResourceView(kConfiguration.m_eFormat)
        , m_kConfiguration(kConfiguration)
        , m_pBuffer(pBuffer)
        , m_BufferView(0)
    {
    }

    SShaderBufferView::~SShaderBufferView()
    {
        if (m_BufferView)
        {
            [m_BufferView release];
        }
    }

    bool SShaderBufferView::Init(CDevice* pDevice)
    {
        if (!m_BufferView)
        {
            m_BufferView = GetMtlBufferBasedOnSize(m_pBuffer);
            [m_BufferView retain];
        }

        return true;
    }

    id<MTLBuffer> SShaderBufferView::GetMetalBuffer()
    {
        id<MTLBuffer>  mtlBuffer = GetMtlBufferBasedOnSize(m_pBuffer);
        return m_BufferView ? m_BufferView : mtlBuffer;
    }

    bool SShaderBufferView::GenerateMipmaps(CContext* pContext)
    {
        DXGL_ERROR("Cannot create mipmaps from a buffer shader resource view");
        return false;
    }

    SShaderTextureView::SShaderTextureView(STexture* pTexture, const SShaderTextureViewConfiguration& kConfiguration)
        : SShaderResourceView(kConfiguration.m_eFormat)
        , m_kConfiguration(kConfiguration)
        , m_pTexture(pTexture)
        //  Confetti BEGIN: Igor Lobanchikov
        , m_TextureView(0)
        //  Confetti End: Igor Lobanchikov
    {
        m_pNextView = pTexture->m_pShaderViewsHead;
        pTexture->m_pShaderViewsHead = this;
    }

    SShaderTextureView::~SShaderTextureView()
    {
        //  Confetti BEGIN: Igor Lobanchikov
        if (m_TextureView)
        {
            [m_TextureView release];
        }
        //  Confetti End: Igor Lobanchikov
        SShaderTextureView** pLink = &m_pTexture->m_pShaderViewsHead;
        while (*pLink != NULL)
        {
            if (*pLink == this)
            {
                *pLink = m_pNextView;
                break;
            }
            pLink = &((*pLink)->m_pNextView);
        }
    }

    bool SShaderTextureView::Init(CDevice* pDevice)
    {
        const SGIFormatInfo* pFormatInfo;
        if (m_kConfiguration.m_eFormat == eGIF_NUM ||
            (pFormatInfo = GetGIFormatInfo(m_kConfiguration.m_eFormat)) == NULL)
        {
            DXGL_ERROR("Invalid format for shader resource view");
            return false;
        }

        if (m_kConfiguration.m_uMinMipLevel != 0
            || m_kConfiguration.m_uNumMipLevels != m_pTexture->m_uNumMipLevels)
        {
            DXGL_ERROR("Metal doesn't support SRV which map to a part of resource.");
        }

        bool bFormatRequiresUniqueView(false);
        if (m_kConfiguration.m_eFormat != m_pTexture->m_eFormat)
        {
            if (pFormatInfo->m_eTypelessFormat != m_pTexture->m_eFormat)
            {
                DXGL_ERROR("Shader resource view format is not compatible with texture format");
                return false;
            }

            switch (pFormatInfo->m_eTypelessConversion)
            {
            case eGIFC_DEPTH_TO_RED:
                break;
            case eGIFC_STENCIL_TO_RED:
#if defined AZ_PLATFORM_MAC
                //Need a new texture view to access the stencil data. x32_stencil8 or x24_stencil8
                bFormatRequiresUniqueView = true;
#else
                m_TextureView = m_pTexture->m_StencilTexture;
                [m_TextureView retain];
#endif
                break;
            case eGIFC_TEXTURE_VIEW:
                bFormatRequiresUniqueView = true;
                break;
            case eGIFC_UNSUPPORTED:
                DXGL_ERROR("Shader resource view conversion not supported for the requested format");
                return false;
            }
        }

        if (m_kConfiguration.m_eViewType != m_pTexture->m_eTextureType)
        {
            bFormatRequiresUniqueView = true;
        }

        if (bFormatRequiresUniqueView ||
            m_kConfiguration.m_uMinLayer > 0 ||
            m_kConfiguration.m_uNumLayers != m_pTexture->m_uNumElements)
        {
            //  Confetti BEGIN: Igor Lobanchikov
            assert(!m_TextureView);

            assert(!m_pTexture->m_bBackBuffer);

            if (m_pTexture->m_bBackBuffer)
            {
                DXGL_ERROR("Back buffer doesn't support views other than native format view");
                return false;
            }
            //  Confetti End: Igor Lobanchikov
            if (!CreateUniqueView(pDevice))
            {
                return false;
            }
        }

        //  Confetti BEGIN: Igor Lobanchikov
        if (!m_TextureView && !m_pTexture->m_bBackBuffer)
        {
            m_TextureView = m_pTexture->m_Texture;
            [m_TextureView retain];
        }
        //  Confetti End: Igor Lobanchikov

        return true;
    }

    bool SShaderTextureView::CreateUniqueView(CDevice* pDevice)
    {
        const SGIFormatInfo* pFormatInfo(GetGIFormatInfo(m_kConfiguration.m_eFormat));

        //  Confetti BEGIN: Igor Lobanchikov
        if (m_kConfiguration.m_uMinLayer > 0 ||
            m_kConfiguration.m_uNumLayers != m_pTexture->m_uNumElements)
        {
            DXGL_ERROR("Not implemented. Metal doesn't support this functionality.");
        }


        if (pFormatInfo->m_eTypelessFormat != m_pTexture->m_eFormat)
        {
            DXGL_ERROR("Texture view format is not compatible with texture format");
            return false;
        }

        m_TextureView = [m_pTexture->m_Texture newTextureViewWithPixelFormat:pFormatInfo->m_pTexture->m_eMetalFormat];

        if (!m_TextureView)
        {
            DXGL_ERROR("Couldn't create output merger or depth-stencil view");
            return false;
        }
        else
        {
            return true;
        }

        //  Confetti End: Igor Lobanchikov
    }

    bool SShaderTextureView::GenerateMipmaps(CContext* pContext)
    {
        id<MTLBlitCommandEncoder> blitCommandEncoder = pContext->GetBlitCommandEncoder();
        [blitCommandEncoder generateMipmapsForTexture: GetMetalTexture()];

        return true;
    }

    SOutputMergerView::SOutputMergerView(EGIFormat eFormat)
        : m_eFormat(eFormat)
    {
    }

    SOutputMergerView::~SOutputMergerView()
    {
    }

    const int32 SOutputMergerTextureView::INVALID_LAYER = -1;

    SOutputMergerTextureView::SOutputMergerTextureView(STexture* pTexture, const SOutputMergerTextureViewConfiguration& kConfiguration)
        : SOutputMergerView(kConfiguration.m_eFormat)
        , m_kConfiguration(kConfiguration)
        , m_pTexture(pTexture)
        //  Confetti BEGIN: Igor Lobanchikov
        , m_RTView(0)
        //  Confetti End: Igor Lobanchikov
    {
        m_pNextView = pTexture->m_pOutputMergerViewsHead;
        pTexture->m_pOutputMergerViewsHead = this;
    }

    SOutputMergerTextureView::~SOutputMergerTextureView()
    {
        SOutputMergerTextureView*& pLink = m_pTexture->m_pOutputMergerViewsHead;
        while (pLink != NULL)
        {
            if (pLink == this)
            {
                pLink = m_pNextView;
            }
            else
            {
                pLink = pLink->m_pNextView;
            }
        }

        //  Confetti BEGIN: Igor Lobanchikov
        if (m_RTView)
        {
            [m_RTView release];
        }
        //  Confetti End: Igor Lobanchikov
    }

    //  Confetti BEGIN: Igor Lobanchikov
    bool    IsMetalRenderable(MTLPixelFormat format)
    {
        switch (format)
        {
#if !defined AZ_PLATFORM_MAC
        case MTLPixelFormatB5G6R5Unorm:
        case MTLPixelFormatR8Unorm_sRGB:
        case MTLPixelFormatRG8Unorm_sRGB:
        case MTLPixelFormatA1BGR5Unorm:
        case MTLPixelFormatABGR4Unorm:
#endif
        case MTLPixelFormatR8Unorm:
        case MTLPixelFormatRG8Unorm:
        case MTLPixelFormatRGBA8Unorm:
        case MTLPixelFormatRG32Float:
        case MTLPixelFormatRGBA32Float:
        case MTLPixelFormatR32Float:
        case MTLPixelFormatBGRA8Unorm:
        case MTLPixelFormatRGBA8Unorm_sRGB:
        case MTLPixelFormatBGRA8Unorm_sRGB:
        case MTLPixelFormatR8Uint:
        case MTLPixelFormatR8Sint:
        case MTLPixelFormatRG8Uint:
        case MTLPixelFormatRG8Sint:
        case MTLPixelFormatRGBA8Uint:
        case MTLPixelFormatRGBA8Sint:
        case MTLPixelFormatR16Uint:
        case MTLPixelFormatR16Sint:
        case MTLPixelFormatRG16Uint:
        case MTLPixelFormatRG16Sint:
        case MTLPixelFormatRGBA16Uint:
        case MTLPixelFormatRGBA16Sint:
        case MTLPixelFormatR16Float:
        case MTLPixelFormatRG16Float:
        case MTLPixelFormatRGBA16Float:
        case MTLPixelFormatR32Uint:
        case MTLPixelFormatR32Sint:
        case MTLPixelFormatRG32Uint:
        case MTLPixelFormatRG32Sint:
        case MTLPixelFormatRGBA32Uint:
        case MTLPixelFormatRGBA32Sint:
        case MTLPixelFormatRGB10A2Unorm:
        case MTLPixelFormatRG11B10Float:
        case MTLPixelFormatRGB9E5Float:
        case MTLPixelFormatRGB10A2Uint:
        case MTLPixelFormatRG8Snorm:
            return true;
        default:
            return false;
        }
    }

    bool IsMetalDepthRenderable(const SGIFormatInfo& formatInfo)
    {
        if (!formatInfo.m_pTexture)
        {
            return false;
        }

        bool isMetalDepthRenderable = formatInfo.m_pTexture->m_eMetalFormat == MTLPixelFormatDepth32Float ||
            formatInfo.m_eTypelessFormat == eGIF_R32G8X24_TYPELESS || formatInfo.m_eTypelessFormat == eGIF_R16_TYPELESS ;

#if defined(AZ_PLATFORM_MAC)
        isMetalDepthRenderable = isMetalDepthRenderable ||
            formatInfo.m_pTexture->m_eMetalFormat == MTLPixelFormatDepth32Float_Stencil8 ||
            formatInfo.m_pTexture->m_eMetalFormat == MTLPixelFormatDepth24Unorm_Stencil8;
#endif
        return isMetalDepthRenderable;
    }
    //  Confetti End: Igor Lobanchikov

    bool SOutputMergerTextureView::Init(CDevice* pDevice)
    {
        m_iMipLevel = (GLint)m_kConfiguration.m_uMipLevel;
        if (m_kConfiguration.m_uMinLayer == 0 && m_kConfiguration.m_uNumLayers == m_pTexture->m_uNumElements)
        //  Confetti BEGIN: Igor Lobanchikov
        {
            if (m_kConfiguration.m_uNumLayers != 1)
            {
                //  Igor: for obvoiuse reasons iOS supports only 2D RT views, no arrays, no 3D RT, only a slice of it
                DXGL_NOT_IMPLEMENTED;
            }
            else
            {
                m_iLayer = m_kConfiguration.m_uMinLayer;
            }
        }
        //  Confetti End: Igor Lobanchikov
        else if (m_kConfiguration.m_uNumLayers == 1)
        {
            m_iLayer = m_kConfiguration.m_uMinLayer;
        }
        else
        {
            DXGL_NOT_IMPLEMENTED;
        }

        //  Confetti BEGIN: Igor Lobanchikov
        {
            const SGIFormatInfo* pFormatInfo;
            if (m_kConfiguration.m_eFormat == eGIF_NUM ||
                (pFormatInfo = GetGIFormatInfo(m_kConfiguration.m_eFormat)) == NULL ||
                pFormatInfo->m_pTexture == NULL ||
                (!IsMetalRenderable(pFormatInfo->m_pTexture->m_eMetalFormat) && !IsMetalDepthRenderable(*pFormatInfo)))
            {
                DXGL_ERROR("Invalid format for output merger view");
                return false;
            }

            assert(m_pTexture);
            assert(m_pTexture->m_Texture);

            if ((m_kConfiguration.m_eFormat != m_pTexture->m_eFormat) && !IsMetalDepthRenderable(*pFormatInfo))
            {
                assert(!m_pTexture->m_bBackBuffer);

                if (m_pTexture->m_bBackBuffer)
                {
                    DXGL_ERROR("Back buffer doesn't support views other than native format view");
                    return false;
                }

                return CreateUniqueView(pFormatInfo, pDevice);
            }
            else if (!m_pTexture->m_bBackBuffer)
            {
                m_RTView = m_pTexture->m_Texture;
                [m_RTView retain];
            }

            return true;
        }
        //  Confetti End: Igor Lobanchikov

        if (m_kConfiguration.m_eFormat != m_pTexture->m_eFormat)
        {
            const SGIFormatInfo* pFormatInfo;
            if (m_kConfiguration.m_eFormat == eGIF_NUM ||
                (pFormatInfo = GetGIFormatInfo(m_kConfiguration.m_eFormat)) == NULL ||
                pFormatInfo->m_pTexture == NULL)
            {
                DXGL_ERROR("Invalid format for output merger view");
                return false;
            }

            if (pFormatInfo->m_pTexture->m_eMetalFormat == GetGIFormatInfo(m_pTexture->m_eFormat)->m_pTexture->m_eMetalFormat)
            {
                return true;
            }

            // Frame buffer attachment does not support any kind of in-place conversion - texture view is required unless no conversion is needed at all
            return CreateUniqueView(pFormatInfo, pDevice);
        }

        return true;
    }

    bool SOutputMergerTextureView::CreateUniqueView(const SGIFormatInfo* pFormatInfo, CDevice* pDevice)
    {
        //  Confetti BEGIN: Igor Lobanchikov
        if (pFormatInfo->m_eTypelessFormat != m_pTexture->m_eFormat)
        {
            DXGL_ERROR("Output merger view format is not compatible with texture format");
            return false;
        }

        m_RTView = [m_pTexture->m_Texture newTextureViewWithPixelFormat:pFormatInfo->m_pTexture->m_eMetalFormat];

        if (!m_RTView)
        {
            DXGL_ERROR("Couldn't create output merger or depth-stencil view");
            return false;
        }

        return true;
        //  Confetti End: Igor Lobanchikov
    }

    SBuffer::SBuffer()
        : m_pSystemMemoryCopy(NULL)
        , m_bMapped(false)
        //  Confetti BEGIN: Igor Lobanchikov
        , m_BufferShared(0)
#if defined(AZ_PLATFORM_MAC)
        , m_BufferManaged(0)
#endif
        , m_pMappedData(0)
        , m_uMapOffset(0)
        , m_uMapSize(0)
        , m_pfMapBufferRange(0)
        //  Confetti End: Igor Lobanchikov
    {
    }

    SBuffer::~SBuffer()
    {
        if (m_pSystemMemoryCopy != NULL)
        {
            MemalignFree(m_pSystemMemoryCopy);
        }

        //  Confetti BEGIN: Igor Lobanchikov
        if (m_BufferShared)
        {
            [m_BufferShared release];
        }
#if defined(AZ_PLATFORM_MAC)
        if (m_BufferManaged)
        {
            [m_BufferManaged release];
        }
#endif
        //  Confetti End: Igor Lobanchikov
    }

    bool SBuffer::GetBufferAndOffset(CContext& context, uint32 uInputBufferOffset, uint32 uBaseOffset, uint32 uBaseStride, id<MTLBuffer>& tmpBuffer, uint32& offset, bool const popTransientMappedDataQueue)
    {
        offset = uInputBufferOffset;
        tmpBuffer = GetMtlBufferBasedOnSize(this);

        void* tmpMappedData = m_pMappedData;

        //  Igor: for now vertex buffers always store data in Metal buffer
        //  Dynamic updates are expected to use direct CPU access
        //  Other updates are handled using GPU copies.
        assert(tmpBuffer);

        if (m_eUsage == eBU_MapInRingBufferTTLOnce)
        {
            assert(offset == uInputBufferOffset);
            //  We assume that tmpMappedData has already map offest applied. Don't want to do it twice.
            offset = 0;

            // David: set appropriate mapped data if the buffer was mapped multiple times
            //        Caller of this function is responsible to pop the m_pTransientMappedData list.
            if (!m_pTransientMappedData.empty())
            {
                tmpMappedData = m_pTransientMappedData.front();

                if (popTransientMappedDataQueue)
                {
                    m_pTransientMappedData.pop_front();
                }
            }
        }

        //  Igor: this is used to calculate offset correctly when ring buffer is used
        if (tmpMappedData)
        {
            //  Check if offset in this situation is always 0
            assert(!offset);
            offset += (uint8*)(tmpMappedData) - (uint8*)(tmpBuffer.contents);
        }

        {
            offset += uBaseOffset * uBaseStride;
        }

        return true;
    }

    SQuery::~SQuery()
    {
    }

    //  Confetti BEGIN: Igor Lobanchikov
    void SQuery::Begin(CContext* pContext)
    {
    }

    void SQuery::End(CContext* pContext)
    {
    }
    //  Confetti End: Igor Lobanchikov

    uint32 SQuery::GetDataSize()
    {
        return 0;
    }

    SPlainQuery::SPlainQuery()
    {
    }

    SPlainQuery::~SPlainQuery()
    {
    }

    //  Confetti BEGIN: Igor Lobanchikov
    void SPlainQuery::Begin(CContext* pContext)
    {
        DXMETAL_NOT_IMPLEMENTED
    }

    void SPlainQuery::End(CContext* pContext)
    {
        DXMETAL_NOT_IMPLEMENTED
    }

    SOcclusionQuery::SOcclusionQuery()
        : m_pEventHelper(NULL)
        , m_pQueryData(NULL)
    {
    }

    void SOcclusionQuery::Begin(CContext* pContext)
    {
        assert(pContext);

        m_pEventHelper = 0;
        m_pQueryData = 0;
        pContext->BeginOcclusionQuery(this);
    }

    void SOcclusionQuery::End(CContext* pContext)
    {
        pContext->EndOcclusionQuery(this);
    }

    bool SOcclusionQuery::GetData(void* pData, uint32 uDataSize, bool bFlush)
    {
        //TODO: m_pEventHelper is null sometimes
        if (!m_pEventHelper)
        {
            return false;
        }
        //assert(m_pEventHelper != nullptr);
        assert(m_pQueryData);
        //  Igor: If the command buffer is not submitted event will never be triggered.
        //  Flushing of the command buffer is expensive (resolve/restore all RTs bound)
        //  As the result we don't support flush operation.
        //  This will lead to stall, if the command buffer is not submitted and application loops until
        //  the event will be triggered which will never happen.
        //  Assert chacks for this situation. If you are here, it is a good idea to reconsider
        //  using the event the way you want to.
        //  note: when the application is just initialized frame throttle events will assert
        //  if there are no resources since command buffer has never been committed.
        assert(!(bFlush) || (bFlush && m_pEventHelper->bCommandBufferSubmitted));
        if (!(!(bFlush) || (bFlush && m_pEventHelper->bCommandBufferSubmitted)))
        {
            int i = 0;
            ++i;
            AZ_TracePrintf("Metal", "Potential dead lock is possible! Event is not triggered. Continue to prevent a deadlock");
            *reinterpret_cast<UINT64*>(pData) = 0;
            return true;
        }

        DXMETAL_TODO("Consider returning something big for true instead of one for occlusion query.");
        //  Igor: Motivation: Cryengine uses threshold to determine visibility. 1 is usually below that threshold.
        if (m_pEventHelper->bTriggered)
        {
            *reinterpret_cast<UINT64*>(pData) = *m_pQueryData;
            return true;
        }
        else
        {
            return false;
        }
    }
    //  Confetti End: Igor Lobanchikov

    uint32 SOcclusionQuery::GetDataSize()
    {
        return sizeof(UINT64);
    }

    //  Confetti BEGIN: Igor Lobanchikov
    SFenceSync::SFenceSync()
        : m_pEventHelper(NULL)
    {
    }

    SFenceSync::~SFenceSync()
    {
    }

    void SFenceSync::End(CContext* pContext)
    {
        assert(pContext);
        m_pEventHelper = pContext->GetCurrentEventHelper();
        assert(m_pEventHelper);
    }

    bool SFenceSync::GetData(void* pData, uint32 uDataSize, bool bFlush)
    {
        assert(m_pEventHelper != NULL);
        //  Igor: If the command buffer is not submitted event will never be triggered.
        //  Flushing of the command buffer is expensive (resolve/restore all RTs bound)
        //  As the result we don't support flush operation.
        //  This will lead to stall, if the command buffer is not submitted and application loops until
        //  the event will be triggered which will never happen.
        //  Assert chacks for this situation. If you are here, it is a good idea to reconsider
        //  using the event the way you want to.
        //  note: when the application is just initialized frame throttle events will assert
        //  if there are no resources since command buffer has never been committed.
        assert(!(bFlush) || (bFlush && m_pEventHelper->bCommandBufferSubmitted));
        if (!(!(bFlush) || (bFlush && m_pEventHelper->bCommandBufferSubmitted)))
        {
            int i = 0;
            ++i;
            AZ_TracePrintf("Metal", "Potential dead lock is possible! Event is not triggered. Continue to prevent a deadlock");
            *reinterpret_cast<BOOL*>(pData) = TRUE;
            return true;
        }

        if (m_pEventHelper->bTriggered)
        {
            *reinterpret_cast<BOOL*>(pData) = TRUE;
            return true;
        }
        else
        {
            return false;
        }
    }
    //  Confetti End: Igor Lobanchikov

    uint32 SFenceSync::GetDataSize()
    {
        return sizeof(BOOL);
    }

    struct SDefaultFrameBufferOutputMergerView
        : SOutputMergerTextureView
    {
        SDefaultFrameBufferOutputMergerView(SDefaultFrameBufferTexture* pTexture, const SOutputMergerTextureViewConfiguration& kConfiguration)
            : SOutputMergerTextureView(pTexture, kConfiguration)
        {
        }

        virtual bool CreateUniqueView(const SGIFormatInfo* pFormatInfo, CDevice* pDevice)
        {
            return SOutputMergerTextureView::CreateUniqueView(pFormatInfo, 0);
        }
    };

    struct SDefaultFrameBufferShaderView
        : SShaderTextureView
    {
        SDefaultFrameBufferShaderView(SDefaultFrameBufferTexture* pTexture, const SShaderTextureViewConfiguration& kConfiguration)
            : SShaderTextureView(pTexture, kConfiguration)
        {
        }
    };

    SDefaultFrameBufferTexture::SDefaultFrameBufferTexture(int32 iWidth, int32 iHeight, EGIFormat eFormat)
        : STexture(iWidth, iHeight, 1, MTLTextureType2D, eFormat, 1, 1)
#if CRY_DXGL_FULL_EMULATION
        , m_kCustomWindowContext(NULL)
#endif //DXGL_FULL_EMULATION
    {
        m_pfUpdateSubresource = &UpdateSubresource;
        m_pfMapSubresource    = &MapSubresource;
        m_pfUnmapSubresource  = &UnmapSubresource;
    }

    SShaderTextureViewPtr SDefaultFrameBufferTexture::CreateShaderView(const SShaderTextureViewConfiguration& kConfiguration, CDevice* pDevice)
    {
        SShaderTextureViewPtr spView(new SDefaultFrameBufferShaderView(this, kConfiguration));
        if (!spView->Init(pDevice))
        {
            return NULL;
        }
        return spView;
    }

    SOutputMergerTextureViewPtr SDefaultFrameBufferTexture::CreateOutputMergerView(const SOutputMergerTextureViewConfiguration& kConfiguration, CDevice* pDevice)
    {
        SOutputMergerTextureViewPtr spView(new SDefaultFrameBufferOutputMergerView(this, kConfiguration));
        if (!spView->Init(pDevice))
        {
            return NULL;
        }
        return spView;
    }

#if DXGL_FULL_EMULATION

    void SDefaultFrameBufferTexture::SetCustomWindowContext(const TWindowContext& kCustomWindowContext)
    {
        m_kCustomWindowContext = kCustomWindowContext;
    }

#endif //DXGL_FULL_EMULATION

    void SDefaultFrameBufferTexture::UpdateSubresource(SResource* pResource, uint32 uSubresource, const D3D11_BOX* pDstBox, const void* pSrcData, uint32 uSrcRowPitch, uint32 uSrcDepthPitch, CContext* pContext)
    {
        if (uSubresource > 0)
        {
            DXGL_ERROR("The only valid subresource index for the default frame buffer is 0 - cannot update subresource");
            return;
        }

        SDefaultFrameBufferTexture* pDefaultFramebufferTexture(static_cast<SDefaultFrameBufferTexture*>(pResource));
        UpdateTexSubresource<SSingleTexImpl<STex2DUncompressed> >(pDefaultFramebufferTexture, uSubresource, pDstBox, pSrcData, uSrcRowPitch, uSrcDepthPitch, pContext);
    }

    bool SDefaultFrameBufferTexture::MapSubresource(SResource* pResource, uint32 uSubresource, D3D11_MAP MapType, UINT MapFlags, D3D11_MAPPED_SUBRESOURCE* pMappedResource, CContext* pContext)
    {
        if (uSubresource > 0)
        {
            DXGL_ERROR("The only valid subresource index for the default frame buffer is 0 - cannot map subresource");
            return false;
        }

        SDefaultFrameBufferTexture* pDefaultFramebufferTexture(static_cast<SDefaultFrameBufferTexture*>(pResource));
        switch (MapType)
        {
        case D3D11_MAP_READ:
        case D3D11_MAP_READ_WRITE:
        case D3D11_MAP_WRITE:
            break;
        default:
            DXGL_ERROR("Unsupported map operation type for default frame buffer");
            return false;
        }

        return MapTexSubresource<SSingleTexImpl<STex2DUncompressed> >(pDefaultFramebufferTexture, uSubresource, MapType, MapFlags, pMappedResource, pContext);
    }

    void SDefaultFrameBufferTexture::UnmapSubresource(SResource* pResource, uint32 uSubresource, CContext* pContext)
    {
        if (uSubresource > 0)
        {
            DXGL_ERROR("The only valid subresource index for the default frame buffer is 0 - cannot unmap subresource");
            return;
        }

        SDefaultFrameBufferTexture* pDefaultFramebufferTexture(static_cast<SDefaultFrameBufferTexture*>(pResource));

        UnmapTexSubresource<SSingleTexImpl<STex2DUncompressed> >(pDefaultFramebufferTexture, uSubresource, pContext);
    }

    const SGIFormatInfo* GetCompatibleTextureFormatInfo(EGIFormat* peGIFormat)
    {
        const SGIFormatInfo* pFormatInfo(GetGIFormatInfo(*peGIFormat));
        if (pFormatInfo->m_pTexture != NULL)
        {
            return pFormatInfo;
        }
        if (pFormatInfo->m_eTypelessFormat != eGIF_NUM && pFormatInfo->m_eTypelessFormat != *peGIFormat)
        {
            *peGIFormat = pFormatInfo->m_eTypelessFormat;
            pFormatInfo = GetGIFormatInfo(pFormatInfo->m_eTypelessFormat);
            if (pFormatInfo->m_pTexture != NULL)
            {
                return pFormatInfo;
            }
        }
        *peGIFormat = eGIF_NUM;
        return NULL;
    }

    STexturePtr CreateTexture1D(const D3D11_TEXTURE1D_DESC& kDesc, const D3D11_SUBRESOURCE_DATA* pInitialData, CDevice* pDevice /*CContext* pContext*/)
    {
        DXGL_SCOPED_PROFILE("CreateTexture1D")

        EGIFormat eGIFormat(GetGIFormat(kDesc.Format));
        const SGIFormatInfo* pFormatInfo(NULL);
        if (eGIFormat == eGIF_NUM ||
            (pFormatInfo = GetCompatibleTextureFormatInfo(&eGIFormat)) == NULL)
        {
            DXGL_ERROR("Invalid format for 1D texture");
            return NULL;
        }

        uint32 uNumElements(kDesc.ArraySize);
        bool bArray(kDesc.ArraySize > 1 || (kDesc.MiscFlags & D3D11_RESOURCE_MISC_DXGL_FORCE_ARRAY));

        STexturePtr spTexture(
            new STexture(
                kDesc.Width, 1, 1,
                bArray ? MTLTextureType1DArray : MTLTextureType1D,
                eGIFormat, GetNumMipLevels(kDesc), kDesc.ArraySize));

        if (kDesc.Usage == D3D11_USAGE_STAGING)
        {
            if (pFormatInfo->m_pTexture->m_bCompressed)
            {
                InitializeTexture<SStagingTexImpl<STex1DCompressed> >(spTexture, pInitialData, kDesc.CPUAccessFlags, pDevice, pFormatInfo, kDesc.BindFlags);
            }
            else
            {
                InitializeTexture<SStagingTexImpl<STex1DUncompressed> >(spTexture, pInitialData, kDesc.CPUAccessFlags, pDevice, pFormatInfo, kDesc.BindFlags);
            }
        }
        else
        {
            if (pFormatInfo->m_pTexture->m_bCompressed)
            {
                if (bArray)
                {
                    InitializeTexture<SArrayTexImpl<STex2DCompressed> >(spTexture, pInitialData, kDesc.CPUAccessFlags, pDevice, pFormatInfo, kDesc.BindFlags);
                }
                else
                {
                    InitializeTexture<SSingleTexImpl<STex1DCompressed> >(spTexture, pInitialData, kDesc.CPUAccessFlags, pDevice, pFormatInfo, kDesc.BindFlags);
                }
            }
            else
            {
                if (bArray)
                {
                    InitializeTexture<SArrayTexImpl<STex2DUncompressed> >(spTexture, pInitialData, kDesc.CPUAccessFlags, pDevice, pFormatInfo, kDesc.BindFlags);
                }
                else
                {
                    InitializeTexture<SSingleTexImpl<STex1DUncompressed> >(spTexture, pInitialData, kDesc.CPUAccessFlags, pDevice, pFormatInfo, kDesc.BindFlags);
                }
            }
        }

        return spTexture;
    }

    void InitializeTexture2D(STexture* pTexture, bool bArray, bool bStaging, const D3D11_SUBRESOURCE_DATA* pInitialData, uint32 uCPUAccess, CDevice* pDevice /*CContext* pContext*/, const SGIFormatInfo* pFormatInfo, const uint32 uBindFlags)
    {
        if (bStaging)
        {
            if (pFormatInfo->m_pTexture->m_bCompressed)
            {
                InitializeTexture<SStagingTexImpl<STex2DCompressed> >(pTexture, pInitialData, uCPUAccess, pDevice, pFormatInfo, uBindFlags);
            }
            else
            {
                InitializeTexture<SStagingTexImpl<STex2DUncompressed> >(pTexture, pInitialData, uCPUAccess, pDevice, pFormatInfo, uBindFlags);
            }
        }
        else
        {
            if (pFormatInfo->m_pTexture->m_bCompressed)
            {
                if (bArray)
                {
                    InitializeTexture<SArrayTexImpl<STex3DCompressed> >(pTexture, pInitialData, uCPUAccess, pDevice, pFormatInfo, uBindFlags);
                }
                else
                {
                    InitializeTexture<SSingleTexImpl<STex2DCompressed> >(pTexture, pInitialData, uCPUAccess, pDevice, pFormatInfo, uBindFlags);
                }
            }
            else
            {
                if (bArray)
                {
                    InitializeTexture<SArrayTexImpl<STex3DUncompressed> >(pTexture, pInitialData, uCPUAccess, pDevice, pFormatInfo, uBindFlags);
                }
                else
                {
                    InitializeTexture<SSingleTexImpl<STex2DUncompressed> >(pTexture, pInitialData, uCPUAccess, pDevice, pFormatInfo, uBindFlags);
                }
            }
        }
    }

    STexturePtr CreateTexture2D(const D3D11_TEXTURE2D_DESC& kDesc, const D3D11_SUBRESOURCE_DATA* pInitialData, CDevice* pDevice /*CContext* pContext*/)
    {
        DXGL_SCOPED_PROFILE("CreateTexture2D")

        EGIFormat eGIFormat(GetGIFormat(kDesc.Format));
        const SGIFormatInfo* pFormatInfo(NULL);
        if (eGIFormat == eGIF_NUM ||
            (pFormatInfo = GetCompatibleTextureFormatInfo(&eGIFormat)) == NULL)
        {
            DXGL_ERROR("Invalid format for 2D texture");
            return NULL;
        }

        bool bStaging(kDesc.Usage == D3D11_USAGE_STAGING);

        if (kDesc.SampleDesc.Count > 1)
        {
            DXGL_NOT_IMPLEMENTED
            return NULL;
        }
        else
        {
            if ((kDesc.MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE) != 0)
            {
                bool bArray(kDesc.ArraySize > 6 || (kDesc.MiscFlags & D3D11_RESOURCE_MISC_DXGL_FORCE_ARRAY));

                if (bArray)
                {
                    DXGL_NOT_IMPLEMENTED;
                    return NULL;
                }
                STexturePtr spTexture(
                    new STexture(
                        kDesc.Width, kDesc.Height, 1,
                        MTLTextureTypeCube,
                        eGIFormat, GetNumMipLevels(kDesc), kDesc.ArraySize));

                InitializeTexture2D(spTexture, bArray, bStaging, pInitialData, kDesc.CPUAccessFlags, pDevice, pFormatInfo, kDesc.BindFlags);

                return spTexture;
            }
            else
            {
                bool bArray(kDesc.ArraySize > 1 || (kDesc.MiscFlags & D3D11_RESOURCE_MISC_DXGL_FORCE_ARRAY));

                STexturePtr spTexture(
                    new STexture(
                        kDesc.Width, kDesc.Height, 1,
                        bArray ? MTLTextureType2DArray : MTLTextureType2D,
                        eGIFormat, GetNumMipLevels(kDesc), kDesc.ArraySize));

                InitializeTexture2D(spTexture, bArray, bStaging, pInitialData, kDesc.CPUAccessFlags, pDevice, pFormatInfo, kDesc.BindFlags);

                return spTexture;
            }
        }
    }

    STexturePtr CreateTexture3D(const D3D11_TEXTURE3D_DESC& kDesc, const D3D11_SUBRESOURCE_DATA* pInitialData, CDevice* pDevice /*CContext* pContext*/)
    {
        DXGL_SCOPED_PROFILE("CreateTexture3D")

        EGIFormat eGIFormat(GetGIFormat(kDesc.Format));
        const SGIFormatInfo* pFormatInfo(NULL);
        if (eGIFormat == eGIF_NUM ||
            (pFormatInfo = GetCompatibleTextureFormatInfo(&eGIFormat)) == NULL)
        {
            DXGL_ERROR("Invalid format for 3D texture");
            return NULL;
        }

        STexturePtr spTexture(
            new STexture(
                kDesc.Width, kDesc.Height, kDesc.Depth,
                MTLTextureType3D,
                eGIFormat, GetNumMipLevels(kDesc), 1));

        if (kDesc.Usage == D3D11_USAGE_STAGING)
        {
            if (pFormatInfo->m_pTexture->m_bCompressed)
            {
                InitializeTexture<SStagingTexImpl<STex3DCompressed> >(spTexture, pInitialData, kDesc.CPUAccessFlags, pDevice, pFormatInfo, kDesc.BindFlags);
            }
            else
            {
                InitializeTexture<SStagingTexImpl<STex3DUncompressed> >(spTexture, pInitialData, kDesc.CPUAccessFlags, pDevice, pFormatInfo, kDesc.BindFlags);
            }
        }
        else
        {
            if (pFormatInfo->m_pTexture->m_bCompressed)
            {
                InitializeTexture<SSingleTexImpl<STex3DCompressed> >(spTexture, pInitialData, kDesc.CPUAccessFlags, pDevice, pFormatInfo, kDesc.BindFlags);
            }
            else
            {
                InitializeTexture<SSingleTexImpl<STex3DUncompressed> >(spTexture, pInitialData, kDesc.CPUAccessFlags, pDevice, pFormatInfo, kDesc.BindFlags);
            }
        }

        return spTexture;
    }

    void UpdateRingBuffer(SBuffer* pBuffer, CContext* pContext, bool checkIfBufferIsMapped)
    {
        MemRingBufferStorage memAllocMode = GetMemAllocModeBasedOnSize(pBuffer->m_uMapSize);
        id<MTLBuffer> mtlBuffer = GetMtlBufferBasedOnSize(pBuffer);

        assert(pBuffer->m_BufferShared);

        bool checkForMapping = checkIfBufferIsMapped ? pBuffer->m_bMapped : true;
        //  Igor: pBuffer->m_bMapped is checked here because we don't want to do anything if the buffer is not mapped.
        if ((pBuffer->m_eUsage == eBU_Default) && checkForMapping)
        {
            id<MTLBuffer> tmpBuffer = pContext->GetRingBuffer(memAllocMode);
            size_t unusedOffset = 0;
            void* pTempData = pContext->AllocateMemoryInRingBuffer(pBuffer->m_uMapSize, memAllocMode, unusedOffset);
            size_t tmpOffset = (uint8*)pTempData - (uint8*)tmpBuffer.contents;

            cryMemcpy(pTempData, pBuffer->m_pSystemMemoryCopy + pBuffer->m_uMapOffset, pBuffer->m_uMapSize);

            id<MTLBlitCommandEncoder> blitCommandEncoder = pContext->GetBlitCommandEncoder();
            [blitCommandEncoder copyFromBuffer: tmpBuffer
                                sourceOffset: tmpOffset
                                toBuffer: mtlBuffer
                                destinationOffset: pBuffer->m_uMapOffset
                                size: pBuffer->m_uMapSize];

            pBuffer->m_uMapOffset = 0;
            pBuffer->m_uMapSize = 0;
        }
#if defined(AZ_PLATFORM_MAC)
        else
        {
            //If this buffer was using the faster ring buffer synchronize with the GPU
            if (memAllocMode == MEM_MANAGED_RINGBUFFER)
            {
                [pBuffer->m_BufferManaged didModifyRange: NSMakeRange(pBuffer->m_uMapOffset, pBuffer->m_uMapSize)];
            }
        }
#endif
    }

    struct SDefaultBufferImpl
    {
        static void UpdateBufferSubresource(SResource* pResource, uint32 uSubresource, const D3D11_BOX* pDstBox, const void* pSrcData, uint32, uint32, CContext* pContext)
        {
            DXGL_SCOPED_PROFILE("SDefaultBufferImpl::UpdateBufferSubresource")

            SBuffer * pBuffer(static_cast<SBuffer*>(pResource));
            assert(pBuffer->m_BufferShared);
            size_t ringBufferOffsetOut = 0;
            
            id<MTLBuffer> tmpBuffer = pContext->GetRingBuffer(MEM_SHARED_RINGBUFFER);
            void* pTempData = pContext->AllocateMemoryInRingBuffer(pBuffer->m_uSize, MEM_SHARED_RINGBUFFER, ringBufferOffsetOut);
            size_t tmpOffset = (uint8*)pTempData - (uint8*)tmpBuffer.contents;
            
            cryMemcpy(pTempData, pSrcData, pDstBox ? pDstBox->right - pDstBox->left : pBuffer->m_BufferShared.length);
            
            id<MTLBlitCommandEncoder> blitCommandEncoder = pContext->GetBlitCommandEncoder();
            [blitCommandEncoder copyFromBuffer: tmpBuffer
                                  sourceOffset: tmpOffset
                                      toBuffer: pBuffer->m_BufferShared
                             destinationOffset: pDstBox ? pDstBox->left : 0
                                          size: pDstBox ? pDstBox->right - pDstBox->left : pBuffer->m_BufferShared.length];
        }
    };

    struct SDynamicBufferImpl
    {
        static bool MapBufferRange(SBuffer* pBuffer, size_t uOffset, size_t uSize, D3D11_MAP eMapType, uint32 uMapFlags, D3D11_MAPPED_SUBRESOURCE* pMappedResource, CContext* pContext)
        {
            DXGL_SCOPED_PROFILE("SDynamicBufferImpl::MapBufferRange")

            //  Confetti BEGIN: Igor Lobanchikov

            size_t ringBufferOffsetOut = 0;

            DXMETAL_TODO("Add more buffer usage types for optimization purposes.");
            //  Igor: Use local copy now, copy it to ring buffer when rendering the actual geometry
            //  Note: vertex buffer must be 256 bytes aligned when rendering.
            //  Igor: eBU_MapInRingBuffer is set for constant buffer. So use ring buffer for constant buffer updates
            switch (pBuffer->m_eUsage)
            {
            case eBU_Default:
            {
                assert(pBuffer->m_pSystemMemoryCopy);
                pMappedResource->pData = pBuffer->m_pSystemMemoryCopy + uOffset;

                assert(pBuffer->m_BufferShared);
                pBuffer->m_bMapped = true;
                break;
            }

            case eBU_MapInRingBufferTTLFrame:
            {
                MemRingBufferStorage memAllocMode = GetMemAllocModeBasedOnSize(uSize);

                if (!pBuffer->m_BufferShared)
                {
                    pBuffer->m_BufferShared = pContext->GetRingBuffer(MEM_SHARED_RINGBUFFER);
                    [pBuffer->m_BufferShared retain];
                }
#if defined(AZ_PLATFORM_MAC)
                if (!pBuffer->m_BufferManaged)
                {
                    pBuffer->m_BufferManaged = pContext->GetRingBuffer(MEM_MANAGED_RINGBUFFER);
                    [pBuffer->m_BufferManaged retain];
                }
#endif
                pBuffer->m_bMapped = true;

                if (eMapType == D3D11_MAP_WRITE_NO_OVERWRITE)
                {
                    if (!pBuffer->m_pMappedData)
                    {
                        pBuffer->m_pMappedData = pContext->AllocateMemoryInRingBuffer(pBuffer->m_uSize, memAllocMode, ringBufferOffsetOut);
                    }
                }
                else
                {
                    // The only other possible mode for dynamic buffers is D3D11_MAP_WRITE_DISCARD
                    assert(eMapType == D3D11_MAP_WRITE_DISCARD);
                    pBuffer->m_pMappedData = pContext->AllocateMemoryInRingBuffer(pBuffer->m_uSize, memAllocMode, ringBufferOffsetOut);
                }

                pMappedResource->pData = (uint8*)pBuffer->m_pMappedData;
                break;
            }

            default:
                DXMETAL_NOT_IMPLEMENTED;
            }
            //  Confetti End: Igor Lobanchikov

            pMappedResource->RowPitch = 0;  // Meaningless for buffers
            pMappedResource->DepthPitch = 0; // Meaningless for buffers

            pBuffer->m_uMapOffset = ringBufferOffsetOut; // This is needed to synchronize with the GPU (didModifyRange)
            pBuffer->m_uMapSize   = uSize;

            return true;
        }

        static bool MapBuffer(SResource* pResource, uint32 uSubresource, D3D11_MAP eMapType, uint32 uMapFlags, D3D11_MAPPED_SUBRESOURCE* pMappedResource, CContext* pContext)
        {
            DXGL_SCOPED_PROFILE("SDynamicBufferImpl::MapBuffer")

            SBuffer * pBuffer(static_cast<SBuffer*>(pResource));
            return MapBufferRange(pBuffer, (size_t)0, (size_t)pBuffer->m_uSize, eMapType, uMapFlags, pMappedResource, pContext);
        }

        static void UnmapBuffer(SResource* pResource, UINT Subresource, CContext* pContext)
        {
            DXGL_SCOPED_PROFILE("SDynamicBufferImpl::UnmapBuffer")

            SBuffer * pBuffer(static_cast<SBuffer*>(pResource));

            //Update the ring buffer with the final rendering data
            bool checkIfBufferIsMapped = true;//Check to make sure this buffer is mapped if it is eBU_Default
            UpdateRingBuffer(pBuffer, pContext, checkIfBufferIsMapped);

            pBuffer->m_bMapped = false;
        }

        static void UpdateBufferSubresource(SResource* pResource, uint32 uSubresource, const D3D11_BOX* pDstBox, const void* pSrcData, uint32, uint32, CContext* pContext)
        {
            DXGL_SCOPED_PROFILE("SDynamicBufferImpl::UpdateBufferSubresource")

            SBuffer * pBuffer(static_cast<SBuffer*>(pResource));

            assert(uSubresource == 0);

            uint32 uDstOffset, uDstSize;
            if (pDstBox != NULL)
            {
                uDstOffset = pDstBox->left;
                uDstSize   = pDstBox->right - pDstBox->left;
            }
            else
            {
                uDstOffset = 0;
                uDstSize   = pBuffer->m_uSize;
            }

            //Update the resource
            if (pBuffer->m_pSystemMemoryCopy)
            {
                cryMemcpy(pBuffer->m_pSystemMemoryCopy + uDstOffset, static_cast<const uint8*>(pSrcData), uDstSize);
            }

            assert(pBuffer->m_eUsage == eBU_Default);

            assert(pBuffer->m_BufferShared);

            //Update the ring buffer with the final rendering data
            bool checkIfBufferIsMapped = false;//Check to make sure this buffer is mapped if it is eBU_Default
            UpdateRingBuffer(pBuffer, pContext, checkIfBufferIsMapped);
        }
    };

    struct SStagingBufferImpl
    {
        static bool MapBufferRange(SBuffer* pBuffer, size_t uOffset, size_t uSize, D3D11_MAP, uint32, D3D11_MAPPED_SUBRESOURCE* pMappedResource, CContext*)
        {
            DXGL_SCOPED_PROFILE("SStagingBufferImpl::MapBufferRange")

            //  Confetti BEGIN: Igor Lobanchikov
            pMappedResource->pData = pBuffer->m_pSystemMemoryCopy + uOffset;
            //  Confetti End: Igor Lobanchikov
            pMappedResource->RowPitch = 0;  // Meaningless for buffers
            pMappedResource->DepthPitch = 0; // Meaningless for buffers

            return true;
        }

        static bool MapBuffer(SResource* pResource, uint32, D3D11_MAP, uint32, D3D11_MAPPED_SUBRESOURCE* pMappedResource, CContext*)
        {
            DXGL_SCOPED_PROFILE("SStagingBufferImpl::MapBuffer")

            SBuffer * pBuffer(static_cast<SBuffer*>(pResource));
            pMappedResource->pData = pBuffer->m_pSystemMemoryCopy;
            //  Confetti End: Igor Lobanchikov
            pMappedResource->RowPitch = 0;  // Meaningless for buffers
            pMappedResource->DepthPitch = 0; // Meaningless for buffers

            return true;
        }

        static void UnmapBuffer(SResource*, UINT, CContext*)
        {
        }

        static void UpdateBufferSubresource(SResource* pResource, uint32 uSubresource, const D3D11_BOX* pDstBox, const void* pSrcData, uint32, uint32, CContext*)
        {
            DXGL_SCOPED_PROFILE("SStagingBufferImpl::UpdateBufferSubresource")

            SBuffer * pBuffer(static_cast<SBuffer*>(pResource));

            assert(uSubresource == 0);

            //  Confetti BEGIN: Igor Lobanchikov
            if (pDstBox != NULL)
            {
                cryMemcpy(pBuffer->m_pSystemMemoryCopy + pDstBox->left, pSrcData, pDstBox->right - pDstBox->left);
            }
            else
            {
                cryMemcpy(pBuffer->m_pSystemMemoryCopy, pSrcData, pBuffer->m_uSize);
            }
            //  Confetti End: Igor Lobanchikov
        }
    };

    struct STransientBufferImpl
    {
        static bool MapBufferRange(SBuffer* pBuffer, size_t uOffset, size_t uSize, D3D11_MAP eMapType, uint32 uMapFlags, D3D11_MAPPED_SUBRESOURCE* pMappedResource, CContext* pContext)
        {
            DXGL_SCOPED_PROFILE("STransientBufferImpl::MapBufferRange")

            assert(pBuffer->m_eUsage == eBU_MapInRingBufferTTLOnce);
            assert(!pBuffer->m_pSystemMemoryCopy);

            size_t ringBufferOffsetOut = 0;
            MemRingBufferStorage memAllocMode = GetMemAllocModeBasedOnSize(uSize);

            if (!pBuffer->m_BufferShared)
            {
                pBuffer->m_BufferShared = pContext->GetRingBuffer(MEM_SHARED_RINGBUFFER);
                [pBuffer->m_BufferShared retain];
            }
#if defined(AZ_PLATFORM_MAC)
            if (!pBuffer->m_BufferManaged)
            {
                pBuffer->m_BufferManaged = pContext->GetRingBuffer(MEM_MANAGED_RINGBUFFER);
                [pBuffer->m_BufferManaged retain];
            }
#endif
            pBuffer->m_bMapped = true;

            if (eMapType == D3D11_MAP_WRITE_NO_OVERWRITE)
            {
                assert(uOffset + uSize <= pBuffer->m_uSize);
                pBuffer->m_pMappedData = pContext->AllocateMemoryInRingBuffer(uSize, memAllocMode, ringBufferOffsetOut);
            }
            else
            {
                // The only other possible mode for dynamic buffers is D3D11_MAP_WRITE_DISCARD
                assert(eMapType == D3D11_MAP_WRITE_DISCARD);
                assert(uSize <= pBuffer->m_uSize);
                pBuffer->m_pMappedData = pContext->AllocateMemoryInRingBuffer(uSize, memAllocMode, ringBufferOffsetOut);
            }

            pMappedResource->pData = (uint8*)pBuffer->m_pMappedData;

            pMappedResource->RowPitch = 0;  // Meaningless for buffers
            pMappedResource->DepthPitch = 0; // Meaningless for buffers

            pBuffer->m_uMapOffset = ringBufferOffsetOut; // This is needed to synchronize with the GPU (didModifyRange)
            pBuffer->m_uMapSize   = uSize;

            pBuffer->m_pTransientMappedData.push_back(pBuffer->m_pMappedData);

            return true;
        }

        static void UnmapBuffer(SResource* pResource, UINT Subresource, CContext* pContext)
        {
            DXGL_SCOPED_PROFILE("STransientBufferImpl::UnmapBuffer")

            SBuffer * pBuffer(static_cast<SBuffer*>(pResource));

            assert(pBuffer->m_eUsage == eBU_MapInRingBufferTTLOnce);
#if defined(AZ_PLATFORM_MAC)
            //If this buffer was using the faster ring buffer synchronize with the GPU
            MemRingBufferStorage memAllocMode = GetMemAllocModeBasedOnSize(pBuffer->m_uMapSize);
            if (memAllocMode == MEM_MANAGED_RINGBUFFER)
            {
                [pBuffer->m_BufferManaged didModifyRange: NSMakeRange(pBuffer->m_uMapOffset, pBuffer->m_uMapSize)];
            }
#endif
            pBuffer->m_bMapped = false;
        }
    };

    struct SDirectAccessBufferImpl
    {
        static bool MapBufferRange(SBuffer* pBuffer, size_t uOffset, size_t uSize, D3D11_MAP, uint32, D3D11_MAPPED_SUBRESOURCE* pMappedResource, CContext*)
        {
            DXGL_SCOPED_PROFILE("SDirectAccessBufferImpl::MapBufferRange")

            assert(pBuffer->m_BufferShared);
            assert(pBuffer->m_eUsage == eBU_DirectAccess);

            //DirectAccess buffers allocate from shared memory only.
            pMappedResource->pData = (uint8*)pBuffer->m_BufferShared.contents + uOffset;
            pMappedResource->RowPitch = 0;  // Meaningless for buffers
            pMappedResource->DepthPitch = 0; // Meaningless for buffers

            return true;
        }

        static bool MapBuffer(SResource* pResource, uint32, D3D11_MAP, uint32, D3D11_MAPPED_SUBRESOURCE* pMappedResource, CContext*)
        {
            DXGL_SCOPED_PROFILE("SDirectAccessBufferImpl::MapBuffer")

            SBuffer * pBuffer(static_cast<SBuffer*>(pResource));

            assert(pBuffer->m_BufferShared);
            assert(pBuffer->m_eUsage == eBU_DirectAccess);

            //DirectAccess buffers allocate from shared memory only.
            pMappedResource->pData = (uint8*)pBuffer->m_BufferShared.contents;

            pMappedResource->RowPitch = 0;  // Meaningless for buffers
            pMappedResource->DepthPitch = 0; // Meaningless for buffers

            return true;
        }

        static void UnmapBuffer(SResource*, UINT, CContext*)
        {
        }
    };

    SBufferPtr CreateBuffer(const D3D11_BUFFER_DESC& kDesc, const D3D11_SUBRESOURCE_DATA* pInitialData, CDevice* pDevice /*CContext* pContext*/)
    {
        DXGL_SCOPED_PROFILE("CreateBuffer")

        SBufferPtr spBuffer(new SBuffer());
        spBuffer->m_uSize = kDesc.ByteWidth;

        for (UINT uBindMask = 1; uBindMask != 0; uBindMask <<= 1)
        {
            switch (kDesc.BindFlags & uBindMask)
            {
            case 0:
                break;
            case D3D11_BIND_VERTEX_BUFFER:
                spBuffer->m_kBindings.Set(eBB_Array, true);
                break;
            case D3D11_BIND_INDEX_BUFFER:
                spBuffer->m_kBindings.Set(eBB_ElementArray, true);
                break;
            case D3D11_BIND_CONSTANT_BUFFER:
                spBuffer->m_kBindings.Set(eBB_UniformBuffer, true);
                break;
#if DXGL_SUPPORT_TEXTURE_BUFFERS
            case D3D11_BIND_SHADER_RESOURCE:
                spBuffer->m_kBindings.Set(eBB_Texture, true);
                break;
#endif //DXGL_SUPPORT_TEXTURE_BUFFERS
            case D3D11_BIND_UNORDERED_ACCESS:
                if ((kDesc.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED) != 0)
                {
#if DXGL_SUPPORT_SHADER_STORAGE_BLOCKS
                    spBuffer->m_kBindings.Set(eBB_ShaderStorage, true);
                    break;
#endif //DXGL_SUPPORT_SHADER_STORAGE_BLOCKS
                }
                else
                {
#if DXGL_SUPPORT_TEXTURE_BUFFERS
                    spBuffer->m_kBindings.Set(eBB_Texture, true);
                    break;
#endif //DXGL_SUPPORT_TEXTURE_BUFFERS
                }
            default:
                DXGL_TODO("Support more buffer bindings");
                DXGL_ERROR("Buffer binding not supported");
                return NULL;
            }
        }

        bool bVideoMemory = false, bAllocateSystemMemory = false;
        switch (kDesc.Usage)
        {
        case D3D11_USAGE_DEFAULT:
            spBuffer->m_pfUpdateSubresource = &SDefaultBufferImpl::UpdateBufferSubresource;
        case D3D11_USAGE_IMMUTABLE:
            if ((kDesc.CPUAccessFlags & D3D11_CPU_ACCESS_WRITE) == 0)
            {
                spBuffer->m_eUsage = eBU_Default;//GL_STATIC_READ;
            }
            else if ((kDesc.CPUAccessFlags & D3D11_CPU_ACCESS_READ) == 0)
            {
                spBuffer->m_eUsage = eBU_Default;//GL_STATIC_DRAW;
            }
            else
            {
                spBuffer->m_eUsage = eBU_Default;//GL_STATIC_COPY;
            }
            bVideoMemory = true;
            bAllocateSystemMemory = false;
            break;
        case D3D11_USAGE_DYNAMIC:
            spBuffer->m_pfMapSubresource    = &SDynamicBufferImpl::MapBuffer;
            spBuffer->m_pfUnmapSubresource  = &SDynamicBufferImpl::UnmapBuffer;
            spBuffer->m_pfUpdateSubresource = &SDynamicBufferImpl::UpdateBufferSubresource;
            spBuffer->m_pfMapBufferRange    = &SDynamicBufferImpl::MapBufferRange;
            if ((kDesc.CPUAccessFlags & D3D11_CPU_ACCESS_READ) != 0)
            {
                DXGL_ERROR("Cannot create a buffer with dynamic usage that is CPU readable");
                return NULL;
            }
            if ((kDesc.BindFlags & D3D11_BIND_CONSTANT_BUFFER) != 0)
            {
                // Assuming that constant buffers are accessed more frequently and usually discarded on updates
                spBuffer->m_eUsage = eBU_MapInRingBufferTTLFrame;//GL_DYNAMIC_DRAW;
            }
            else
            {
                spBuffer->m_eUsage = eBU_Default;//GL_STREAM_DRAW;
            }
            //  Confetti BEGIN: Igor Lobanchikov
            if (spBuffer->m_eUsage == eBU_Default)
            {
                bVideoMemory = true;
                //  Igor: we always map the system memory, then copy to ring buffer and use GPU to copy from ring buffer to the actual buffer.
                //  Slow but safe.
                //  This path should never be used. Consider using other approaches.
                bAllocateSystemMemory = true;
            }
            else
            {
                bVideoMemory = false;
                bAllocateSystemMemory = false;
            }
            //  Confetti End: Igor Lobanchikov
            break;
        case D3D11_USAGE_STAGING:
            spBuffer->m_pfMapSubresource    = &SStagingBufferImpl::MapBuffer;
            spBuffer->m_pfUnmapSubresource  = &SStagingBufferImpl::UnmapBuffer;
            spBuffer->m_pfUpdateSubresource = &SStagingBufferImpl::UpdateBufferSubresource;
            spBuffer->m_pfMapBufferRange    = &SStagingBufferImpl::MapBufferRange;
            spBuffer->m_eUsage = eBU_Default;//GL_NONE;
            AZ_Assert(!(kDesc.CPUAccessFlags & D3D11_CPU_ACCESS_WRITE), "The resource should not be writable by CPU");
            AZ_Assert((kDesc.CPUAccessFlags & D3D11_CPU_ACCESS_READ), "The resource should be readable by CPU");
            bVideoMemory = false;
            bAllocateSystemMemory = true;
            break;
        case D3D11_USAGE_TRANSIENT:
            spBuffer->m_pfUnmapSubresource  = &STransientBufferImpl::UnmapBuffer;
            spBuffer->m_pfMapBufferRange    = &STransientBufferImpl::MapBufferRange;
            spBuffer->m_eUsage = eBU_MapInRingBufferTTLOnce;
            //  Igor: this buffer never own memory but rather borrows it from the ring buffer
            bVideoMemory = false;
            bAllocateSystemMemory = false;
            if ((kDesc.CPUAccessFlags & D3D11_CPU_ACCESS_READ) != 0)
            {
                DXGL_ERROR("Cannot create a buffer with transient usage that is CPU readable");
                return NULL;
            }
            break;
        default:
        case D3D11_USAGE_DIRECT_ACCESS:
            spBuffer->m_pfUnmapSubresource  = &SDirectAccessBufferImpl::UnmapBuffer;
            spBuffer->m_pfMapBufferRange    = &SDirectAccessBufferImpl::MapBufferRange;
            spBuffer->m_pfMapSubresource    = &SDirectAccessBufferImpl::MapBuffer;
            spBuffer->m_eUsage = eBU_DirectAccess;
            bVideoMemory = true;
            bAllocateSystemMemory = false;
            break;
            DXGL_ERROR("Buffer usage not supported");
            return NULL;
        }

        if (bVideoMemory)
        {
            //  Confetti BEGIN: Igor Lobanchikov
            id<MTLDevice> Device = pDevice->GetMetalDevice();
            if (kDesc.BindFlags & D3D11_BIND_UNORDERED_ACCESS)
            {
                spBuffer->m_BufferShared = [Device newBufferWithLength:spBuffer->m_uSize options:MTLResourceStorageModeShared];
            }
            else
            {
                spBuffer->m_BufferShared = [Device newBufferWithLength:spBuffer->m_uSize options:MTLResourceCPUCacheModeWriteCombined | MTLResourceStorageModeShared];
            }
            if (pInitialData != NULL)
            {
                cryMemcpy(spBuffer->m_BufferShared.contents, pInitialData->pSysMem, kDesc.ByteWidth);
            }
            //  Confetti End: Igor Lobanchikov
        }

        if (bAllocateSystemMemory)
        {
            spBuffer->m_pSystemMemoryCopy = static_cast<uint8*>(Memalign(kDesc.ByteWidth, MIN_MAPPED_RESOURCE_ALIGNMENT));
        }

        if (spBuffer->m_pSystemMemoryCopy != NULL && pInitialData != NULL)
        {
            cryMemcpy(spBuffer->m_pSystemMemoryCopy, pInitialData->pSysMem, kDesc.ByteWidth);
        }

        return spBuffer;
    }

    template <typename ViewDesc, typename View>
    struct SResourceViewBaseImpl
    {
        typedef ViewDesc TViewDesc;
        typedef View TView;
        typedef _smart_ptr<TView> TViewPtr;
    };

    struct SShaderResourceViewImpl
        : SResourceViewBaseImpl<D3D11_SHADER_RESOURCE_VIEW_DESC, SShaderResourceView>
    {
        enum EViewDimension
        {
            DIMENSION_BUFFER           = D3D11_SRV_DIMENSION_BUFFER,
            DIMENSION_TEXTURE1D        = D3D11_SRV_DIMENSION_TEXTURE1D,
            DIMENSION_TEXTURE1DARRAY   = D3D11_SRV_DIMENSION_TEXTURE1DARRAY,
            DIMENSION_TEXTURE2D        = D3D11_SRV_DIMENSION_TEXTURE2D,
            DIMENSION_TEXTURE2DARRAY   = D3D11_SRV_DIMENSION_TEXTURE2DARRAY,
            DIMENSION_TEXTURE2DMS      = D3D11_SRV_DIMENSION_TEXTURE2DMS,
            DIMENSION_TEXTURE2DMSARRAY = D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY,
            DIMENSION_TEXTURE3D        = D3D11_SRV_DIMENSION_TEXTURE3D,
            DIMENSION_TEXTURECUBE      = D3D11_SRV_DIMENSION_TEXTURECUBE,
            DIMENSION_TEXTURECUBEARRAY = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY,
            DIMENSION_BUFFEREX         = D3D11_SRV_DIMENSION_BUFFEREX
        };

        static SResourceViewBaseImpl::TViewPtr GetView(STexture* pTexture, DXGI_FORMAT eDXGIFormat, MTLTextureType eViewType, uint32 uMinLevel, uint32 uNumLevels, uint32 uMinElement, uint32 uNumElements, CDevice* pDevice)
        {
            SShaderTextureViewConfiguration kConfiguration(GetGIFormat(eDXGIFormat), eViewType, uMinLevel, uNumLevels, uMinElement, uNumElements);

            SShaderTextureView* pExistingView(pTexture->m_pShaderViewsHead);
            while (pExistingView != NULL)
            {
                if (pExistingView->m_kConfiguration == kConfiguration)
                {
                    return pExistingView;
                }
                pExistingView = pExistingView->m_pNextView;
            }

            return pTexture->CreateShaderView(kConfiguration, pDevice);
        }

        static SResourceViewBaseImpl::TViewPtr GetBufferView(SBuffer* pBuffer, DXGI_FORMAT eDXGIFormat, MTLTextureType eViewType, uint32 uMinLevel, uint32 uNumLevels, uint32 uMinElement, uint32 uNumElements, CDevice* pDevice)
        {
            SShaderBufferViewConfiguration kConfiguration(GetGIFormat(eDXGIFormat), eViewType, uMinLevel, uNumLevels, uMinElement, uNumElements);
            return pBuffer->CreateShaderView(kConfiguration, pDevice);
        }

        template <typename DimDesc>
        static TViewPtr GetViewMip(STexture* pTexture, const DimDesc& kDimDesc, DXGI_FORMAT eDXGIFormat, MTLTextureType eViewType, uint32 uMinElement, uint32 uNumElements, CDevice* pDevice)
        {
            uint32 uNumMipLevels(kDimDesc.MipLevels == -1 ? (pTexture->m_uNumMipLevels - kDimDesc.MostDetailedMip) : kDimDesc.MipLevels);
            return GetView(pTexture, eDXGIFormat, eViewType, kDimDesc.MostDetailedMip, uNumMipLevels, uMinElement, uNumElements, pDevice);
        }

        template <typename DimDesc>
        static TViewPtr GetViewLayers(STexture* pTexture, const DimDesc& kDimDesc, DXGI_FORMAT eDXGIFormat, uint32 uMinLevel, uint32 uNumLevels, MTLTextureType eViewType, CDevice* pDevice)
        {
            return GetView(pTexture, eDXGIFormat, eViewType, uMinLevel, uNumLevels, kDimDesc.FirstArraySlice, kDimDesc.ArraySize, pDevice);
        }

        template <typename DimDesc>
        static TViewPtr GetViewMipLayers(STexture* pTexture, const DimDesc& kDimDesc, DXGI_FORMAT eDXGIFormat, MTLTextureType eViewType, CDevice* pDevice)
        {
            return GetViewMip(pTexture, kDimDesc, eDXGIFormat, eViewType, kDimDesc.FirstArraySlice, kDimDesc.ArraySize, pDevice);
        }
    };

    template <typename ViewDesc>
    struct SOutputMergerViewImpl
        : SResourceViewBaseImpl<ViewDesc, SOutputMergerView>
    {
        static typename SOutputMergerViewImpl::TViewPtr GetView(STexture* pTexture, DXGI_FORMAT eDXGIFormat, MTLTextureType eViewType, uint32 uMipLevel, uint32, uint32 uMinElement, uint32 uNumElements, CDevice* pDevice)
        {
            return pTexture->GetCompatibleOutputMergerView(SOutputMergerTextureViewConfiguration(GetGIFormat(eDXGIFormat), uMipLevel, uMinElement, uNumElements), pDevice);
        }
        static typename SOutputMergerViewImpl::TViewPtr GetBufferView(SBuffer* pBuffer, DXGI_FORMAT eDXGIFormat, MTLTextureType eViewType, uint32 uMinLevel, uint32 uNumLevels, uint32 uMinElement, uint32 uNumElements, CDevice* pDevice)
        {
            //not implemented
            CRY_ASSERT(0);
            return NULL;
        }
        template <typename DimDesc>
        static typename SOutputMergerViewImpl::TViewPtr GetViewMip(STexture* pTexture, const DimDesc& kDimDesc, DXGI_FORMAT eDXGIFormat, MTLTextureType eViewType, uint32 uMinElement, uint32 uNumElements, CDevice* pDevice)
        {
            return GetView(pTexture, eDXGIFormat, eViewType, kDimDesc.MipSlice, 1, uMinElement, uNumElements, pDevice);
        }

        template <typename DimDesc>
        static typename SOutputMergerViewImpl::TViewPtr GetViewLayers(STexture* pTexture, const DimDesc& kDimDesc, DXGI_FORMAT eDXGIFormat, MTLTextureType eViewType, uint32 uMinLevel, uint32 uNumLevels, CDevice* pDevice)
        {
            return GetView(pTexture, eDXGIFormat, eViewType, uMinLevel, uNumLevels, kDimDesc.FirstArraySlice, kDimDesc.ArraySize, pDevice);
        }

        template <typename DimDesc>
        static typename SOutputMergerViewImpl::TViewPtr GetViewMipLayers(STexture* pTexture, const DimDesc& kDimDesc, DXGI_FORMAT eDXGIFormat, MTLTextureType eViewType, CDevice* pDevice)
        {
            return GetViewMip(pTexture, kDimDesc, eDXGIFormat, eViewType, kDimDesc.FirstArraySlice, kDimDesc.ArraySize, pDevice);
        }
    };

    struct SRenderTargetViewImpl
        : SOutputMergerViewImpl<D3D11_RENDER_TARGET_VIEW_DESC>
    {
        enum EViewDimension
        {
            DIMENSION_BUFFER           = D3D11_RTV_DIMENSION_BUFFER,
            DIMENSION_TEXTURE1D        = D3D11_RTV_DIMENSION_TEXTURE1D,
            DIMENSION_TEXTURE1DARRAY   = D3D11_RTV_DIMENSION_TEXTURE1DARRAY,
            DIMENSION_TEXTURE2D        = D3D11_RTV_DIMENSION_TEXTURE2D,
            DIMENSION_TEXTURE2DARRAY   = D3D11_RTV_DIMENSION_TEXTURE2DARRAY,
            DIMENSION_TEXTURE2DMS      = D3D11_RTV_DIMENSION_TEXTURE2DMS,
            DIMENSION_TEXTURE2DMSARRAY = D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY,
            DIMENSION_TEXTURE3D        = D3D11_RTV_DIMENSION_TEXTURE3D
        };
    };

    struct SDepthStencilViewImpl
        : SOutputMergerViewImpl<D3D11_DEPTH_STENCIL_VIEW_DESC>
    {
        enum EViewDimension
        {
            DIMENSION_TEXTURE1D        = D3D11_DSV_DIMENSION_TEXTURE1D,
            DIMENSION_TEXTURE1DARRAY   = D3D11_DSV_DIMENSION_TEXTURE1DARRAY,
            DIMENSION_TEXTURE2D        = D3D11_DSV_DIMENSION_TEXTURE2D,
            DIMENSION_TEXTURE2DARRAY   = D3D11_DSV_DIMENSION_TEXTURE2DARRAY,
            DIMENSION_TEXTURE2DMS      = D3D11_DSV_DIMENSION_TEXTURE2DMS,
            DIMENSION_TEXTURE2DMSARRAY = D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY,
        };
    };

    template <typename Impl>
    typename Impl::TViewPtr GetTexture1DView(STexture* pTexture, const typename Impl::TViewDesc& kViewDesc, CDevice* pDevice)
    {
        using ViewDimensionType = decltype(kViewDesc.ViewDimension);

        switch (kViewDesc.ViewDimension)
        {
        case static_cast<ViewDimensionType>(Impl::DIMENSION_TEXTURE1D):
            return Impl::GetViewMip(pTexture,       kViewDesc.Texture1D,      kViewDesc.Format, MTLTextureType1D,       0, 1, pDevice);
        case static_cast<ViewDimensionType>(Impl::DIMENSION_TEXTURE1DARRAY):
            return Impl::GetViewMipLayers(pTexture, kViewDesc.Texture1DArray, kViewDesc.Format, MTLTextureType1DArray,       pDevice);
        }
        return NULL;
    }

    template <typename Impl>
    typename Impl::TViewPtr GetTexture2DView(STexture* pTexture, const typename Impl::TViewDesc& kViewDesc, CDevice* pDevice)
    {
        using ViewDimensionType = decltype(kViewDesc.ViewDimension);

        switch (kViewDesc.ViewDimension)
        {
        case static_cast<ViewDimensionType>(Impl::DIMENSION_TEXTURE2D):
            return Impl::GetViewMip(pTexture,       kViewDesc.Texture2D,        kViewDesc.Format, MTLTextureType2D,                   0, 1,       pDevice);
        case static_cast<ViewDimensionType>(Impl::DIMENSION_TEXTURE2DARRAY):
            return Impl::GetViewMipLayers(pTexture, kViewDesc.Texture2DArray,   kViewDesc.Format, MTLTextureType2DArray,                         pDevice);
#if DXGL_SUPPORT_MULTISAMPLED_TEXTURES
        case static_cast<ViewDimensionType>(Impl::DIMENSION_TEXTURE2DMS):
            return Impl::GetView(pTexture,                                      kViewDesc.Format, GL_TEXTURE_2D_MULTISAMPLE,       0, 1, 0, 1, pDevice);
#endif //DXGL_SUPPORT_MULTISAMPLED_TEXTURES
        }
        return NULL;
    }

    template <typename Impl>
    typename Impl::TViewPtr GetTextureCubeView(STexture* pTexture, const typename Impl::TViewDesc& kViewDesc, CDevice* pDevice)
    {
        using ViewDimensionType = decltype(kViewDesc.ViewDimension);

        switch (kViewDesc.ViewDimension)
        {
        case static_cast<ViewDimensionType>(D3D11_SRV_DIMENSION_TEXTURECUBE):
            DXGL_TODO("Check if 6 is correct");
            return Impl::GetViewMip(pTexture, kViewDesc.TextureCube, kViewDesc.Format, MTLTextureTypeCube, 0, 6, pDevice);
        }
        return NULL;
    }

    template <typename Impl>
    typename Impl::TViewPtr GetTexture3DView(STexture* pTexture, const typename Impl::TViewDesc& kViewDesc, CDevice* pDevice)
    {
        if ((uint32)kViewDesc.ViewDimension == (uint32)Impl::DIMENSION_TEXTURE3D)
        {
            return Impl::GetViewMip(pTexture, kViewDesc.Texture3D, kViewDesc.Format, MTLTextureType3D, 0, 1, pDevice);
        }
        return NULL;
    }

    SShaderResourceViewImpl a;

    //  Confetti BEGIN: Igor Lobanchikov
    template <>
    SRenderTargetViewImpl::TViewPtr GetTexture3DView<SRenderTargetViewImpl>(STexture* pTexture, const SRenderTargetViewImpl::TViewDesc& kViewDesc, CDevice* pDevice)
    {
        //  DX11 does not support the array 3D texture rendering so route 3D slice down as the arraiy slice
        //  iOS does not support rendering into the multiple slices at the same time, so the check is performed later in
        //  the pipeline.

        //  Igor: double check the following code when we meet this firts. This wasn't used hence wasn't tested.
        DXGL_NOT_IMPLEMENTED;

        //  Confetti End: Igor Lobanchikov
        if ((uint32)kViewDesc.ViewDimension == (uint32)SRenderTargetViewImpl::DIMENSION_TEXTURE3D)
        {
            return SRenderTargetViewImpl::GetViewMip(pTexture, kViewDesc.Texture3D, kViewDesc.Format, MTLTextureType3D,
                kViewDesc.Texture3D.FirstWSlice, kViewDesc.Texture3D.WSize, pDevice);
        }
        return NULL;
    }
    //  Confetti End: Igor Lobanchikov

    template <typename Impl>
    typename Impl::TViewPtr GetBufferView(SBuffer* pBuffer, const typename Impl::TViewDesc& kViewDesc, CDevice* pDevice)
    {
        return Impl::GetBufferView(pBuffer, kViewDesc.Format, MTLTextureType1D, 1, 1, 0, 1, pDevice);
    }

    template <typename Impl>
    typename Impl::TViewPtr GetBufferViewEx(SBuffer* pBuffer, const typename Impl::TViewDesc& kViewDesc, CDevice* pDevice)
    {
        DXMETAL_NOT_IMPLEMENTED
            CryLog("TODO: GetBufferViewEX");
        return NULL;
    }

    SShaderResourceViewPtr CreateShaderResourceView(SResource* pResource, D3D11_RESOURCE_DIMENSION eDimension, const D3D11_SHADER_RESOURCE_VIEW_DESC& kViewDesc, CDevice* pDevice /*CContext* pContext*/)
    {
        DXGL_SCOPED_PROFILE("CreateShaderResourceView")

        typedef SShaderResourceViewImpl TImpl;
        SShaderResourceViewPtr spView;
        switch (eDimension)
        {
        case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
            spView =   GetTexture1DView<TImpl>(static_cast<STexture*>(pResource), kViewDesc, pDevice);
            break;
        case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
            spView =   GetTexture2DView<TImpl>(static_cast<STexture*>(pResource), kViewDesc, pDevice);
            if (spView == NULL)
            {
                spView = GetTextureCubeView<TImpl>(static_cast<STexture*>(pResource), kViewDesc, pDevice);
            }
            break;
        case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
            spView =   GetTexture3DView<TImpl>(static_cast<STexture*>(pResource), kViewDesc, pDevice);
            break;
        case D3D11_RESOURCE_DIMENSION_BUFFER:
            spView =   GetBufferView<TImpl>(static_cast<SBuffer*>(pResource), kViewDesc, pDevice);
            if (spView == NULL)
            {
                spView = GetBufferViewEx<TImpl>(static_cast<SBuffer*>(pResource), kViewDesc, pDevice);
            }
            break;
        default:
            DXGL_ERROR("Invalid resource dimension for shader resource");
            return NULL;
        }

        if (spView == NULL)
        {
            DXGL_ERROR("Invalid shader resource view paramters");
        }
        return spView;
    }

    SOutputMergerViewPtr CreateRenderTargetView(SResource* pResource, D3D11_RESOURCE_DIMENSION eDimension, const D3D11_RENDER_TARGET_VIEW_DESC& kViewDesc, CDevice* pDevice /*CContext* pContext*/)
    {
        DXGL_SCOPED_PROFILE("CreateRenderTargetView")

        typedef SRenderTargetViewImpl TImpl;
        switch (eDimension)
        {
        case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
            return GetTexture1DView<TImpl>(static_cast<STexture*>(pResource), kViewDesc, pDevice);
        case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
            return GetTexture2DView<TImpl>(static_cast<STexture*>(pResource), kViewDesc, pDevice);
        case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
            return GetTexture3DView<TImpl>(static_cast<STexture*>(pResource), kViewDesc, pDevice);
        case D3D11_RESOURCE_DIMENSION_BUFFER:
            return GetBufferView<TImpl>(static_cast<SBuffer*>(pResource), kViewDesc, pDevice);
        }
        DXGL_ERROR("Invalid resource dimension for render target");
        return NULL;
    }

    SOutputMergerViewPtr CreateDepthStencilView(SResource* pResource, D3D11_RESOURCE_DIMENSION eDimension, const D3D11_DEPTH_STENCIL_VIEW_DESC& kViewDesc, CDevice* pDevice /*CContext* pContext*/)
    {
        DXGL_SCOPED_PROFILE("CreateDepthStencilView")

        typedef SDepthStencilViewImpl TImpl;
        switch (eDimension)
        {
        case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
            return GetTexture1DView<TImpl>(static_cast<STexture*>(pResource), kViewDesc, pDevice);
        case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
            return GetTexture2DView<TImpl>(static_cast<STexture*>(pResource), kViewDesc, pDevice);
        }
        DXGL_ERROR("Invalid resource dimension for render target");
        return NULL;
    }

    SQueryPtr CreateQuery(const D3D11_QUERY_DESC& kDesc, CDevice* pDevice /*CContext* pContext*/)
    {
        DXGL_SCOPED_PROFILE("CreateQuery")

        SQueryPtr spQuery(NULL);
        switch (kDesc.Query)
        {
        case D3D11_QUERY_OCCLUSION:
        {
            SOcclusionQuery* pOcclusionQuery(new SOcclusionQuery());
            spQuery = pOcclusionQuery;
        }
        break;
        case D3D11_QUERY_EVENT:
            spQuery = new SFenceSync();
            break;
        case D3D11_QUERY_TIMESTAMP:
        case D3D11_QUERY_TIMESTAMP_DISJOINT:
        case D3D11_QUERY_PIPELINE_STATISTICS:
        case D3D11_QUERY_OCCLUSION_PREDICATE:
        case D3D11_QUERY_SO_STATISTICS:
        case D3D11_QUERY_SO_OVERFLOW_PREDICATE:
        case D3D11_QUERY_SO_STATISTICS_STREAM0:
        case D3D11_QUERY_SO_OVERFLOW_PREDICATE_STREAM0:
        case D3D11_QUERY_SO_STATISTICS_STREAM1:
        case D3D11_QUERY_SO_OVERFLOW_PREDICATE_STREAM1:
        case D3D11_QUERY_SO_STATISTICS_STREAM2:
        case D3D11_QUERY_SO_OVERFLOW_PREDICATE_STREAM2:
        case D3D11_QUERY_SO_STATISTICS_STREAM3:
        case D3D11_QUERY_SO_OVERFLOW_PREDICATE_STREAM3:
            DXGL_NOT_IMPLEMENTED
            break;
        }

        return spQuery;
    }

    SDefaultFrameBufferTexturePtr CreateBackBufferTexture(const D3D11_TEXTURE2D_DESC& kDesc)
    {
        DXGL_SCOPED_PROFILE("CreateBackBufferTexture")

        EGIFormat eGIFormat(GetGIFormat(kDesc.Format));
        if (eGIFormat == eGIF_NUM)
        {
            return NULL;
        }

        return new SDefaultFrameBufferTexture(kDesc.Width, kDesc.Height, eGIFormat);
    }

    typedef void (* CopyTextureBoxFunc)(STexture*, STexPos, STexSubresourceID, STexture*, STexPos, STexSubresourceID, STexSize, CContext*);

    void CopySystemTextureBox(
        STexture* pDstTexture, STexPos kDstPos, STexSubresourceID kDstSubID,
        STexture* pSrcTexture, STexPos kSrcPos, STexSubresourceID kSrcSubID,
        STexSize kBoxSize, CContext* pContext)
    {
        UINT uDstSubresource(D3D11CalcSubresource(kDstSubID.m_iMipLevel, kDstSubID.m_uElement, pDstTexture->m_uNumMipLevels));
        UINT uSrcSubresource(D3D11CalcSubresource(kSrcSubID.m_iMipLevel, kSrcSubID.m_uElement, pSrcTexture->m_uNumMipLevels));

        D3D11_BOX kDstBox;
        kDstBox.left    = kDstPos.x;
        kDstBox.top     = kDstPos.y;
        kDstBox.front   = kDstPos.z;
        kDstBox.right   = kDstPos.x + kBoxSize.x;
        kDstBox.bottom  = kDstPos.y + kBoxSize.y;
        kDstBox.back    = kDstPos.z + kBoxSize.z;

        D3D11_MAPPED_SUBRESOURCE kSrcMapped;
        pSrcTexture->m_pfMapSubresource(pSrcTexture, uSrcSubresource, D3D11_MAP_READ, 0, &kSrcMapped, pContext);
        pDstTexture->m_pfUpdateSubresource(pDstTexture, uDstSubresource, &kDstBox, kSrcMapped.pData, kSrcMapped.RowPitch, kSrcMapped.DepthPitch, pContext);
        pSrcTexture->m_pfUnmapSubresource(pSrcTexture, uSrcSubresource, pContext);
    }

    void CopyTextureWithBlitCommandEncoder(
        STexture* pDstTexture, STexPos kDstPos, STexSubresourceID kDstSubID,
        STexture* pSrcTexture, STexPos kSrcPos, STexSubresourceID kSrcSubID,
        STexSize kBoxSize, CContext* pContext)
    {
        MTLOrigin sourceOrigin = MTLOriginMake(kSrcPos.x, kSrcPos.y, kSrcPos.z);
        MTLSize sourceSize = MTLSizeMake(kBoxSize.x, kBoxSize.y, kBoxSize.z);
        MTLOrigin destinationOrigin = {static_cast<NSUInteger>(kDstPos.x),
                                       static_cast<NSUInteger>(kDstPos.y),
                                       static_cast<NSUInteger>(kDstPos.z)};

        id<MTLBlitCommandEncoder> blitCommandEncoder = pContext->GetBlitCommandEncoder();
        [blitCommandEncoder copyFromTexture: pSrcTexture->m_Texture
                                sourceSlice: kSrcSubID.m_uElement
                                sourceLevel: kSrcSubID.m_iMipLevel
                               sourceOrigin: sourceOrigin
                                 sourceSize: sourceSize
                                  toTexture: pDstTexture->m_Texture
                           destinationSlice: kDstSubID.m_uElement
                           destinationLevel: kDstSubID.m_iMipLevel
                          destinationOrigin: destinationOrigin];
        
#if defined AZ_PLATFORM_MAC
        if (pDstTexture->m_Texture.storageMode == MTLStorageModeManaged)
        {
            // Need to synchronize the CPU/GPU versions of the texture if it is
            // in manged storage mode otherwise the CPU may not see any of the
            // writes the GPU does to the texture
            [blitCommandEncoder synchronizeTexture: pDstTexture->m_Texture
                                             slice: kDstSubID.m_uElement
                                             level: kDstSubID.m_iMipLevel];
        }
#endif
    }

    template <CopyTextureBoxFunc pfCopyTextureBox>
    void CopyTextureImpl(STexture* pDstTexture, STexture* pSrcTexture, CContext* pContext)
    {
        DXGL_TODO("Check if there's a better way to do this");
        STexSubresourceID kSubID;
        for (kSubID.m_iMipLevel = 0; kSubID.m_iMipLevel < (GLint)pDstTexture->m_uNumMipLevels; ++kSubID.m_iMipLevel)
        {
            STexBox kBox;
            GetTextureBox(kBox, pDstTexture, kSubID.m_iMipLevel, GetGIFormatInfo(pDstTexture->m_eFormat), true);

            for (kSubID.m_uElement = 0; kSubID.m_uElement < pDstTexture->m_uNumElements; ++kSubID.m_uElement)
            {
                pfCopyTextureBox(
                    pDstTexture, kBox.m_kOffset, kSubID,
                    pSrcTexture, kBox.m_kOffset, kSubID,
                    kBox.m_kSize, pContext);
            }
        }
    }

    void CopyTexture(STexture* pDstTexture, STexture* pSrcTexture, CContext* pContext)
    {
        DXGL_SCOPED_PROFILE("CopyTexture")

        if (pSrcTexture->m_uNumMipLevels    != pDstTexture->m_uNumMipLevels ||
            pSrcTexture->m_uNumElements     != pDstTexture->m_uNumElements ||
            pSrcTexture->m_iWidth           != pDstTexture->m_iWidth ||
            pSrcTexture->m_iHeight          != pDstTexture->m_iHeight ||
            pSrcTexture->m_iDepth           != pDstTexture->m_iDepth)
        {
            DXGL_ERROR("Source and destination textures to copy don't match");
            return;
        }

        //  Confetti BEGIN: Igor Lobanchikov
        CopyTextureImpl<CopyTextureWithBlitCommandEncoder>(pDstTexture, pSrcTexture, pContext);
        //  Confetti End: Igor Lobanchikov
    }

    template <CopyTextureBoxFunc pfCopyTextureBox>
    void CopySubTextureImpl(
        STexture* pDstTexture, uint32 uDstSubresource, uint32 uDstX, uint32 uDstY, uint32 uDstZ,
        STexture* pSrcTexture, uint32 uSrcSubresource, const D3D11_BOX* pSrcBox, CContext* pContext)
    {
        STexSubresourceID kDstSubID;
        kDstSubID.m_iMipLevel = uDstSubresource % pDstTexture->m_uNumMipLevels;
        kDstSubID.m_uElement  = uDstSubresource / pDstTexture->m_uNumMipLevels;

        STexSubresourceID kSrcSubID;
        kSrcSubID.m_iMipLevel = uSrcSubresource % pSrcTexture->m_uNumMipLevels;
        kSrcSubID.m_uElement  = uSrcSubresource / pSrcTexture->m_uNumMipLevels;

        STexBox kSrcBox;
        GetTextureBox(kSrcBox, pSrcTexture, kSrcSubID.m_iMipLevel, pSrcBox, GetGIFormatInfo(pSrcTexture->m_eFormat), true);

        STexPos kDstPos(uDstX, uDstY, uDstZ);

        pfCopyTextureBox(
            pDstTexture, kDstPos, kDstSubID,
            pSrcTexture, kSrcBox.m_kOffset, kSrcSubID,
            kSrcBox.m_kSize, pContext);
    }

    void CopySubTexture(
        STexture* pDstTexture, uint32 uDstSubresource, uint32 uDstX, uint32 uDstY, uint32 uDstZ,
        STexture* pSrcTexture, uint32 uSrcSubresource, const D3D11_BOX* pSrcBox, CContext* pContext)
    {
        DXGL_SCOPED_PROFILE("CopySubTexture")

        //  Confetti BEGIN: Igor Lobanchikov
        assert(pDstTexture->m_Texture);
        assert(pSrcTexture->m_Texture);

        if (pDstTexture->m_eFormat != pSrcTexture->m_eFormat)
        {
            pContext->TrySlowCopySubresource(pDstTexture, uDstSubresource, uDstX, uDstY, uDstZ, pSrcTexture, uSrcSubresource, pSrcBox);
            return;
        }

        CopySubTextureImpl<CopyTextureWithBlitCommandEncoder>(pDstTexture, uDstSubresource, uDstX, uDstY, uDstZ, pSrcTexture, uSrcSubresource, pSrcBox, pContext);
    }

    void CopySubBufferInternal(SBuffer* pDstBuffer, SBuffer* pSrcBuffer, uint32 uDstOffset, uint32 uSrcOffset, uint32 uSize, CContext* pContext)
    {
        if (pSrcBuffer->m_pSystemMemoryCopy != NULL && pDstBuffer->m_pSystemMemoryCopy != NULL)
        {
            cryMemcpy(pDstBuffer->m_pSystemMemoryCopy + uDstOffset, pSrcBuffer->m_pSystemMemoryCopy + uSrcOffset, uSize);
        }

        //  Confetti BEGIN: Igor Lobanchikov
        assert(pDstBuffer->m_BufferShared);
        //    assert(pSrcBuffer->m_Buffer);
        size_t ringBufferOffsetOut = 0;
        MemRingBufferStorage memAllocMode = GetMemAllocModeBasedOnSize(uSize);
        id<MTLBuffer> tmpBuffer = pSrcBuffer->m_BufferShared;

        if (!tmpBuffer)
        {
            assert(pSrcBuffer->m_pSystemMemoryCopy);
            tmpBuffer = pContext->GetRingBuffer(memAllocMode);
            void* pTempData = pContext->AllocateMemoryInRingBuffer(uSize, memAllocMode, ringBufferOffsetOut);
            size_t tmpOffset = (uint8*)pTempData - (uint8*)tmpBuffer.contents;

            cryMemcpy(pTempData, pSrcBuffer->m_pSystemMemoryCopy + uSrcOffset, uSize);

            uSrcOffset = tmpOffset;
        }

        id<MTLBlitCommandEncoder> blitCommandEncoder = pContext->GetBlitCommandEncoder();
        [blitCommandEncoder copyFromBuffer: tmpBuffer sourceOffset: uSrcOffset toBuffer: pDstBuffer->m_BufferShared destinationOffset: uDstOffset size: uSize];


        //  Confetti End: Igor Lobanchikov
    }

    void CopyBuffer(SBuffer* pDstBuffer, SBuffer* pSrcBuffer, CContext* pContext)
    {
        DXGL_SCOPED_PROFILE("CopyBuffer")

        if (pSrcBuffer->m_uSize != pDstBuffer->m_uSize)
        {
            DXGL_ERROR("Source and destination buffers to copy don't match");
            return;
        }

        CopySubBufferInternal(pDstBuffer, pSrcBuffer, 0, 0, pSrcBuffer->m_uSize, pContext);
    }

    void CopySubBuffer(
        SBuffer* pDstBuffer, uint32 uDstSubresource, uint32 uDstX, uint32, uint32,
        SBuffer* pSrcBuffer, uint32 uSrcSubresource, const D3D11_BOX* pSrcBox, CContext* pContext)
    {
        DXGL_SCOPED_PROFILE("CopySubBuffer")

        uint32 uSrcBegin, uSrcEnd;
        if (pSrcBox != NULL)
        {
            uSrcBegin = pSrcBox->left;
            uSrcEnd   = pSrcBox->right;
        }
        else
        {
            uSrcBegin = 0;
            uSrcEnd   = pSrcBuffer->m_uSize;
        }
        uint32 uSize(uSrcEnd - uSrcBegin);

        if (uSrcBegin > pSrcBuffer->m_uSize ||
            uSrcEnd > pSrcBuffer->m_uSize ||
            uDstX + uSize > pDstBuffer->m_uSize)
        {
            DXGL_ERROR("Source or destination range out of bounds");
            return;
        }

        if (pDstBuffer != pSrcBuffer)
        {
            CopySubBufferInternal(pDstBuffer, pSrcBuffer, uDstX, uSrcBegin, uSize, pContext);
        }
        else
        {
            //  Igor: check if this works as expected once triggered.
            assert(!"Not tested");
            //  Igor: don't want to copy
            if (uDstX == uSrcBegin)
            {
                return;
            }
            //  copy forward
            else if (uSrcBegin > uDstX && uSrcBegin < uDstX + uSize)
            {
                uint32 chunkSize = uSrcBegin - uDstX;
                while (uSize)
                {
                    chunkSize = min(chunkSize, uSize);
                    CopySubBufferInternal(pDstBuffer, pSrcBuffer, uDstX, uSrcBegin, chunkSize, pContext);
                    pDstBuffer += chunkSize;
                    pSrcBuffer += chunkSize;
                    uSize -= chunkSize;
                }
            }
            //  copy backward
            else if (uDstX > uSrcBegin && uDstX < uSrcBegin + uSize)
            {
                uint32 chunkSize = uDstX - uSrcBegin;
                uDstX = uSrcBegin + uSize;
                uSrcBegin = uDstX - chunkSize;
                while (uSize)
                {
                    chunkSize = min(chunkSize, uSize);
                    CopySubBufferInternal(pDstBuffer, pSrcBuffer, uDstX, uSrcBegin, chunkSize, pContext);
                    pDstBuffer -= chunkSize;
                    pSrcBuffer -= chunkSize;
                    uSize -= chunkSize;
                }
            }
            //  default behaviour
            else
            {
                CopySubBufferInternal(pDstBuffer, pSrcBuffer, uDstX, uSrcBegin, uSize, pContext);
            }
        }
    }
}
