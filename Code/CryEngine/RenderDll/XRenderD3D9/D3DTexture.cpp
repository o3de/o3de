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

// Description : Direct3D specific texture manager implementation.


#include "RenderDll_precompiled.h"
#include "DriverD3D.h"
#include "I3DEngine.h"
#include "StringUtils.h"
#include <ImageExtensionHelper.h>
#include "BitFiddling.h"        // ConvertBlock3DcToDXT5()
#include "D3DStereo.h"
#include "../Common/PostProcess/PostProcessUtils.h"
#include "D3DPostProcess.h"
#include "../Common/Textures/TextureHelpers.h"
#include "../Common/Textures/TextureManager.h"
#include "../Common/RenderCapabilities.h"
#include <AzCore/Debug/AssetTracking.h>
#include <Common/Memory/VRAMDrillerBus.h>

#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
#include <RTTBus.h>
#endif // AZ_RENDER_TO_TEXTURE_GEM_ENABLED

#undef min
#undef max
//===============================================================================
namespace {
    void SetShaderResourceViewDesc(const SResourceView& rv, uint32 texType, D3DFormat format, int arraySize, uint32 nSliceCount, D3D11_SHADER_RESOURCE_VIEW_DESC& desc)
    {
        const uint nMipCount = rv.m_Desc.nMipCount == SResourceView().m_Desc.nMipCount ? (uint) - 1 : (uint)rv.m_Desc.nMipCount;
        ZeroStruct(desc);
        desc.Format = CTexture::ConvertToShaderResourceFmt(format);
        if (rv.m_Desc.bSrgbRead)
        {
            desc.Format = CTexture::ConvertToSRGBFmt(desc.Format);
        }
        switch (texType)
        {
        case eTT_1D:
            if (arraySize > 1)
            {
                desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1DARRAY;
                desc.Texture1DArray.MostDetailedMip = rv.m_Desc.nMostDetailedMip;
                desc.Texture1DArray.MipLevels = nMipCount;
                desc.Texture1DArray.FirstArraySlice = rv.m_Desc.nFirstSlice;
                desc.Texture1DArray.ArraySize = nSliceCount;
            }
            else
            {
                desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
                desc.Texture1D.MostDetailedMip = rv.m_Desc.nMostDetailedMip;
                desc.Texture1D.MipLevels = nMipCount;
            }
            break;
        case eTT_2D:
            if (arraySize > 1)
            {
                if (rv.m_Desc.bMultisample)
                {
                    desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY;
                    desc.Texture2DMSArray.FirstArraySlice = rv.m_Desc.nFirstSlice;
                    desc.Texture2DMSArray.ArraySize = nSliceCount;
                }
                else
                {
                    desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
                    desc.Texture2DArray.MostDetailedMip = rv.m_Desc.nMostDetailedMip;
                    desc.Texture2DArray.MipLevels = nMipCount;
                    desc.Texture2DArray.FirstArraySlice = rv.m_Desc.nFirstSlice;
                    desc.Texture2DArray.ArraySize = nSliceCount;
                }
            }
            else
            {
                if (rv.m_Desc.bMultisample)
                {
                    desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
                }
                else
                {
                    desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                    desc.Texture2D.MostDetailedMip = rv.m_Desc.nMostDetailedMip;
                    desc.Texture2D.MipLevels = nMipCount;
                }
            }
            break;
        case eTT_3D:
            desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
            desc.Texture3D.MostDetailedMip = rv.m_Desc.nMostDetailedMip;
            desc.Texture3D.MipLevels = nMipCount;
            break;
        case eTT_Cube:
            desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
            desc.TextureCube.MostDetailedMip = rv.m_Desc.nMostDetailedMip;
            desc.TextureCube.MipLevels = nMipCount;
            break;
        }
    }

    void SetRenderTargetViewDesc(const SResourceView& rv, uint32 texType, D3DFormat format, int arraySize, uint32 nSliceCount, D3D11_RENDER_TARGET_VIEW_DESC& rtvDesc)
    {
        ZeroStruct(rtvDesc);
        rtvDesc.Format = format;

        switch (texType)
        {
        case eTT_1D:
            if (arraySize > 1)
            {
                rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE1DARRAY;
                rtvDesc.Texture1DArray.MipSlice = rv.m_Desc.nMostDetailedMip;
                rtvDesc.Texture1DArray.FirstArraySlice = rv.m_Desc.nFirstSlice;
                rtvDesc.Texture1DArray.ArraySize = nSliceCount;
            }
            else
            {
                rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE1D;
                rtvDesc.Texture1D.MipSlice = rv.m_Desc.nMostDetailedMip;
            }
            break;
        case eTT_2D:
            if (arraySize > 1)
            {
                if (rv.m_Desc.bMultisample)
                {
                    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
                    rtvDesc.Texture2DMSArray.FirstArraySlice = rv.m_Desc.nFirstSlice;
                    rtvDesc.Texture2DMSArray.ArraySize = nSliceCount;
                }
                else
                {
                    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                    rtvDesc.Texture2DArray.MipSlice = rv.m_Desc.nMostDetailedMip;
                    rtvDesc.Texture2DArray.FirstArraySlice = rv.m_Desc.nFirstSlice;
                    rtvDesc.Texture2DArray.ArraySize = nSliceCount;
                }
            }
            else
            {
                if (rv.m_Desc.bMultisample)
                {
                    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
                }
                else
                {
                    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                    rtvDesc.Texture2D.MipSlice = rv.m_Desc.nMostDetailedMip;
                }
            }
            break;
        case eTT_3D:
            rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
            rtvDesc.Texture3D.MipSlice = rv.m_Desc.nMostDetailedMip;
            rtvDesc.Texture3D.FirstWSlice = rv.m_Desc.nFirstSlice;
            rtvDesc.Texture3D.WSize = nSliceCount;
            break;
        case eTT_Cube:
            rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
            rtvDesc.Texture2DArray.MipSlice = rv.m_Desc.nMostDetailedMip;
            rtvDesc.Texture2DArray.FirstArraySlice = rv.m_Desc.nFirstSlice;
            rtvDesc.Texture2DArray.ArraySize = nSliceCount;
            break;
        }
    }

    void SetDepthStencilViewDesc(const SResourceView& rv, uint32 texType, D3DFormat format, int arraySize, uint32 nSliceCount, D3D11_DEPTH_STENCIL_VIEW_DESC& dsvDesc)
    {
        ZeroStruct(dsvDesc);
        dsvDesc.Format = (DXGI_FORMAT)CTexture::ConvertToDepthStencilFmt(format);

        switch (texType)
        {
        case eTT_1D:
            if (arraySize > 1)
            {
                dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE1DARRAY;
                dsvDesc.Texture1DArray.MipSlice = rv.m_Desc.nMostDetailedMip;
                dsvDesc.Texture1DArray.FirstArraySlice = rv.m_Desc.nFirstSlice;
                dsvDesc.Texture1DArray.ArraySize = nSliceCount;
            }
            else
            {
                dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE1D;
                dsvDesc.Texture1D.MipSlice = rv.m_Desc.nMostDetailedMip;
            }
            break;
        case eTT_2D:
            if (arraySize > 1)
            {
                if (rv.m_Desc.bMultisample)
                {
                    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY;
                    dsvDesc.Texture2DMSArray.FirstArraySlice = rv.m_Desc.nFirstSlice;
                    dsvDesc.Texture2DMSArray.ArraySize = nSliceCount;
                }
                else
                {
                    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
                    dsvDesc.Texture2DArray.MipSlice = rv.m_Desc.nMostDetailedMip;
                    dsvDesc.Texture2DArray.FirstArraySlice = rv.m_Desc.nFirstSlice;
                    dsvDesc.Texture2DArray.ArraySize = nSliceCount;
                }
            }
            else
            {
                if (rv.m_Desc.bMultisample)
                {
                    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
                }
                else
                {
                    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                    dsvDesc.Texture2D.MipSlice = rv.m_Desc.nMostDetailedMip;
                }
            }
            break;
        case eTT_Cube:
            {
                dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
                dsvDesc.Texture2DArray.MipSlice = rv.m_Desc.nMostDetailedMip;
                dsvDesc.Texture2DArray.FirstArraySlice = rv.m_Desc.nFirstSlice;
                dsvDesc.Texture2DArray.ArraySize = nSliceCount;
            }
            break;
        }
    }

    void SetUnorderedAccessViewDesc(const SResourceView& rv, uint32 texType, D3DFormat format, int arraySize, uint32 nSliceCount, D3D11_UNORDERED_ACCESS_VIEW_DESC& desc)
    {
        ZeroStruct(desc);
        desc.Format = format;

        switch (texType)
        {
        case eTT_1D:
            if (arraySize > 1)
            {
                desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE1DARRAY;
                desc.Texture1DArray.MipSlice = rv.m_Desc.nMostDetailedMip;
                desc.Texture1DArray.FirstArraySlice = rv.m_Desc.nFirstSlice;
                desc.Texture1DArray.ArraySize = nSliceCount;
            }
            else
            {
                desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE1D;
                desc.Texture1D.MipSlice = rv.m_Desc.nMostDetailedMip;
            }
            break;
        case eTT_2D:
            if (arraySize > 1)
            {
                AZ_Assert(rv.m_Desc.bMultisample == 0, "No MSAA in UAV Array");
                desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
                desc.Texture2DArray.MipSlice = rv.m_Desc.nMostDetailedMip;
                desc.Texture2DArray.FirstArraySlice = rv.m_Desc.nFirstSlice;
                desc.Texture2DArray.ArraySize = nSliceCount;
            }
            else
            {
                AZ_Assert(rv.m_Desc.bMultisample == 0, "No MSAA in UAV");
                desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
                desc.Texture2D.MipSlice = rv.m_Desc.nMostDetailedMip;
            }
            break;
        case eTT_3D:
            desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
            desc.Texture3D.MipSlice = rv.m_Desc.nMostDetailedMip;
            desc.Texture3D.FirstWSlice = rv.m_Desc.nFirstSlice;
            desc.Texture3D.WSize = nSliceCount;
            break;
        }
    }
}

RenderTargetData::~RenderTargetData()
{
    CDeviceTexture* pTexMSAA = (CDeviceTexture*)m_pDeviceTextureMSAA;
    SAFE_RELEASE(pTexMSAA);

    for (size_t i = 0; i < m_ResourceViews.size(); ++i)
    {
        const SResourceView& rv = m_ResourceViews[i];
        switch (rv.m_Desc.eViewType)
        {
        case SResourceView::eShaderResourceView:
        {
            D3DShaderResourceView* pView = static_cast<D3DShaderResourceView*>(rv.m_pDeviceResourceView);
            SAFE_RELEASE(pView);
            break;
        }
        case SResourceView::eRenderTargetView:
        {
            D3DSurface* pView = static_cast<D3DSurface*>(rv.m_pDeviceResourceView);
            SAFE_RELEASE(pView);
            break;
        }
        case SResourceView::eDepthStencilView:
        {
            D3DDepthSurface* pView = static_cast<D3DDepthSurface*>(rv.m_pDeviceResourceView);
            SAFE_RELEASE(pView);
            break;
        }
        case SResourceView::eUnorderedAccessView:
        {
            D3DUnorderedAccessView* pView = static_cast<D3DUnorderedAccessView*>(rv.m_pDeviceResourceView);
            SAFE_RELEASE(pView);
            break;
        }
        default:
            assert(false);
        }
    }
}
//===============================================================================


#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define D3DTEXTURE_CPP_SECTION_1 1
#define D3DTEXTURE_CPP_SECTION_2 2
#define D3DTEXTURE_CPP_SECTION_3 3
#define D3DTEXTURE_CPP_SECTION_4 4
#define D3DTEXTURE_CPP_SECTION_5 5
#define D3DTEXTURE_CPP_SECTION_6 6
#define D3DTEXTURE_CPP_SECTION_7 7
#define D3DTEXTURE_CPP_SECTION_8 8
#define D3DTEXTURE_CPP_SECTION_9 9
#define D3DTEXTURE_CPP_SECTION_10 10
#define D3DTEXTURE_CPP_SECTION_11 11
#define D3DTEXTURE_CPP_SECTION_12 12
#define D3DTEXTURE_CPP_SECTION_13 13
#define D3DTEXTURE_CPP_SECTION_14 14
#endif

#if defined(TEXTURE_GET_SYSTEM_COPY_SUPPORT)
byte* CTexture::Convert(const byte* sourceData, int nWidth, int nHeight, int sourceMipCount, ETEX_Format eTFSrc, ETEX_Format eTFDst, int& nOutSize, [[maybe_unused]] bool bLinear)
{
    nOutSize = 0;

    DXGI_FORMAT DeviceFormatSRC = (DXGI_FORMAT)DeviceFormatFromTexFormat(eTFSrc);
    DXGI_FORMAT DeviceFormatDST = (DXGI_FORMAT)DeviceFormatFromTexFormat(eTFDst);

    if (DeviceFormatSRC == DXGI_FORMAT_UNKNOWN || DeviceFormatDST == DXGI_FORMAT_UNKNOWN || nWidth <= 0 || nHeight <= 0)
    {
        AZ_Assert(false, "Invalid parameters to CTexture::Convert");
        return nullptr;
    }

    if (sourceMipCount <= 0)
    {
        sourceMipCount = 1;
    }

    int outputSize = 0;
    byte* outputData = nullptr;

    if (eTFSrc == eTF_BC5U && eTFDst == eTF_BC3)
    {
        int w = nWidth;
        int h = nHeight;

        outputSize = CTexture::TextureDataSize(w, h, 1, sourceMipCount, 1, eTF_BC3);
        outputData = new byte[outputSize];

        int nOffsDST = 0;
        int nOffsSRC = 0;
        for (int mip = 0; mip < sourceMipCount; mip++)
        {
            if (w <= 0)
            {
                w = 1;
            }
            if (h <= 0)
            {
                h = 1;
            }

            const void* outSrc = &sourceData[nOffsSRC];
            DWORD outSize = CTexture::TextureDataSize(w, h, 1, 1, 1, eTFDst);

            nOffsSRC += CTexture::TextureDataSize(w, h, 1, 1, 1, eTFSrc);

            {
                const byte* src = (const byte*)outSrc;

                for (uint32 n = 0; n < outSize / 16; n++)           // for each block
                {
                    const uint8* srcDataBlock = &src[n * 16];
                    uint8* outputDataBlock = &outputData[nOffsDST + n * 16];
                    ConvertBlock3DcToDXT5(outputDataBlock, srcDataBlock);
                }

                nOffsDST += outSize;
            }

            w >>= 1;
            h >>= 1;
        }
        nOutSize = outputSize;
        return outputData;
    }

    outputSize = CTexture::TextureDataSize(nWidth, nHeight, 1, sourceMipCount, 1, eTFSrc);
    outputData = new byte[outputSize];
    memcpy(outputData, sourceData, outputSize);
    nOutSize = outputSize;
    return outputData;
}
#endif //#if defined(WIN32) || defined(WIN64)

D3DSurface* CTexture::GetSurface(int nCMSide, int nLevel)
{
    if (!m_pDevTexture)
    {
        return NULL;
    }

    if (IsDeviceFormatTypeless(m_pPixelFormat->DeviceFormat))
    {
        iLog->Log("Error: CTexture::GetSurface: typeless formats can't be specified for RTVs, failed to create surface for the texture %s", GetSourceName());
        return NULL;
    }

    SCOPED_RENDERER_ALLOCATION_NAME_HINT(GetSourceName());

    HRESULT hr = S_OK;
    D3DTexture* pID3DTexture = NULL;
    D3DTexture* pID3DTexture3D = NULL;
    D3DTexture* pID3DTextureCube = NULL;
    D3DSurface* pTargSurf = m_pDeviceRTV;
    const bool bUseMultisampledRTV = ((m_nFlags & FT_USAGE_MSAA) && m_bUseMultisampledRTV) != 0;
    if (bUseMultisampledRTV)
    {
        pTargSurf = m_pDeviceRTVMS;
    }

    if (!pTargSurf)
    {
        int nMipLevel = 0;
        int nSlice = 0;
        int nSliceCount = -1;

        if (m_eTT == eTT_Cube)
        {
            nMipLevel = (m_nFlags & FT_FORCE_MIPS) ? min((int)(m_nMips - 1), nLevel) : 0;
            nSlice = nCMSide;
            nSliceCount = 1;
        }
        pTargSurf = (D3DSurface*) CreateDeviceResourceView(SResourceView::RenderTargetView(m_eTFDst, nSlice, nSliceCount, nMipLevel, bUseMultisampledRTV));

        if (bUseMultisampledRTV)
        {
            m_pDeviceRTVMS = pTargSurf;
        }
        else
        {
            m_pDeviceRTV = pTargSurf;
        }
    }
    assert(hr == S_OK);

    if (FAILED(hr))
    {
        pTargSurf = NULL;
    }

    return pTargSurf;
}


void CTexture::Readback(AZ::u32 subresourceIndex, StagingHook callback)
{
    if (m_pDevTexture)
    {
        m_pDevTexture->DownloadToStagingResource(subresourceIndex, callback);
    }
}

//===============================================================================

bool CTexture::IsDeviceFormatTypeless(D3DFormat nFormat)
{
    switch (nFormat)
    {
    //128 bits
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:
    case DXGI_FORMAT_R32G32B32_TYPELESS:

    //64 bits
    case DXGI_FORMAT_R16G16B16A16_TYPELESS:
    case DXGI_FORMAT_R32G32_TYPELESS:
    case DXGI_FORMAT_R32G8X24_TYPELESS:
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:

    //32 bits
    case DXGI_FORMAT_R10G10B10A2_TYPELESS:
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
    case DXGI_FORMAT_B8G8R8X8_TYPELESS:
    case DXGI_FORMAT_R16G16_TYPELESS:
    case DXGI_FORMAT_R32_TYPELESS:
    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:

    //16 bits
    case DXGI_FORMAT_R8G8_TYPELESS:
    case DXGI_FORMAT_R16_TYPELESS:

    //8 bits
    case DXGI_FORMAT_R8_TYPELESS:

    //block formats
    case DXGI_FORMAT_BC1_TYPELESS:
    case DXGI_FORMAT_BC2_TYPELESS:
    case DXGI_FORMAT_BC3_TYPELESS:
    case DXGI_FORMAT_BC4_TYPELESS:
    case DXGI_FORMAT_BC5_TYPELESS:

#if defined(OPENGL) || defined(CRY_USE_METAL)
    case DXGI_FORMAT_ETC2_TYPELESS:
    case DXGI_FORMAT_ETC2A_TYPELESS:
    case DXGI_FORMAT_EAC_R11_TYPELESS:
    case DXGI_FORMAT_EAC_RG11_TYPELESS:
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DTEXTURE_CPP_SECTION_1
    #include AZ_RESTRICTED_FILE(D3DTexture_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
    case DXGI_FORMAT_BC6H_TYPELESS:
#endif
    case DXGI_FORMAT_BC7_TYPELESS:

#ifdef CRY_USE_METAL
    case DXGI_FORMAT_PVRTC2_TYPELESS:
    case DXGI_FORMAT_PVRTC4_TYPELESS:
#endif
#if defined(ANDROID) || defined(CRY_USE_METAL)
    case DXGI_FORMAT_ASTC_4x4_TYPELESS:
    case DXGI_FORMAT_ASTC_5x4_TYPELESS:
    case DXGI_FORMAT_ASTC_5x5_TYPELESS:
    case DXGI_FORMAT_ASTC_6x5_TYPELESS:
    case DXGI_FORMAT_ASTC_6x6_TYPELESS:
    case DXGI_FORMAT_ASTC_8x5_TYPELESS:
    case DXGI_FORMAT_ASTC_8x6_TYPELESS:
    case DXGI_FORMAT_ASTC_8x8_TYPELESS:
    case DXGI_FORMAT_ASTC_10x5_TYPELESS:
    case DXGI_FORMAT_ASTC_10x6_TYPELESS:
    case DXGI_FORMAT_ASTC_10x8_TYPELESS:
    case DXGI_FORMAT_ASTC_10x10_TYPELESS:
    case DXGI_FORMAT_ASTC_12x10_TYPELESS:
    case DXGI_FORMAT_ASTC_12x12_TYPELESS:
#endif
        return true;

    default:
        break;
    }

    return false;
}

bool CTexture::IsDeviceFormatSRGBReadable(D3DFormat nFormat)
{
    switch (nFormat)
    {
    case DXGI_FORMAT_R8G8B8A8_UNORM:
        return true;
    case DXGI_FORMAT_B8G8R8A8_UNORM:
        return true;
    case DXGI_FORMAT_B8G8R8X8_UNORM:
        return true;

    case DXGI_FORMAT_BC1_UNORM:
        return true;
    case DXGI_FORMAT_BC2_UNORM:
        return true;
    case DXGI_FORMAT_BC3_UNORM:
        return true;
    case DXGI_FORMAT_BC7_UNORM:
        return true;

#if defined(OPENGL) || defined(CRY_USE_METAL)
    case DXGI_FORMAT_ETC2_UNORM:
        return true;
    case DXGI_FORMAT_ETC2A_UNORM:
        return true;
#endif //defined(OPENGL)

#ifdef CRY_USE_METAL
    case DXGI_FORMAT_PVRTC2_UNORM:
        return true;
    case DXGI_FORMAT_PVRTC4_UNORM:
        return true;
#endif
#if defined(ANDROID) || defined(CRY_USE_METAL)
    case DXGI_FORMAT_ASTC_4x4_UNORM:
        return true;
    case DXGI_FORMAT_ASTC_5x4_UNORM:
        return true;
    case DXGI_FORMAT_ASTC_5x5_UNORM:
        return true;
    case DXGI_FORMAT_ASTC_6x5_UNORM:
        return true;
    case DXGI_FORMAT_ASTC_6x6_UNORM:
        return true;
    case DXGI_FORMAT_ASTC_8x5_UNORM:
        return true;
    case DXGI_FORMAT_ASTC_8x6_UNORM:
        return true;
    case DXGI_FORMAT_ASTC_8x8_UNORM:
        return true;
    case DXGI_FORMAT_ASTC_10x5_UNORM:
        return true;
    case DXGI_FORMAT_ASTC_10x6_UNORM:
        return true;
    case DXGI_FORMAT_ASTC_10x8_UNORM:
        return true;
    case DXGI_FORMAT_ASTC_10x10_UNORM:
        return true;
    case DXGI_FORMAT_ASTC_12x10_UNORM:
        return true;
    case DXGI_FORMAT_ASTC_12x12_UNORM:
        return true;
#endif

    default:
        break;
    }

    return false;
}

// this function is valid for FT_USAGE_DEPTHSTENCIL textures only
D3DFormat CTexture::DeviceFormatFromTexFormat(ETEX_Format eTF)
{
    switch (eTF)
    {
    case eTF_R8G8B8A8:
        return DXGI_FORMAT_R8G8B8A8_UNORM;
    case eTF_R8G8B8A8S:
        return DXGI_FORMAT_R8G8B8A8_SNORM;

    case eTF_A8:
        return DXGI_FORMAT_A8_UNORM;
    case eTF_R8:
        return DXGI_FORMAT_R8_UNORM;
    case eTF_R8S:
        return DXGI_FORMAT_R8_SNORM;
    case eTF_R16:
        return DXGI_FORMAT_R16_UNORM;
    case eTF_R16U:
        return DXGI_FORMAT_R16_UINT;
    case eTF_R16G16U:
        return DXGI_FORMAT_R16G16_UINT;
    case eTF_R10G10B10A2UI:
        return DXGI_FORMAT_R10G10B10A2_UINT;
    case eTF_R16F:
        return DXGI_FORMAT_R16_FLOAT;
    case eTF_R32F:
        return DXGI_FORMAT_R32_FLOAT;
    case eTF_R8G8:
        return DXGI_FORMAT_R8G8_UNORM;
    case eTF_R8G8S:
        return DXGI_FORMAT_R8G8_SNORM;
    case eTF_R16G16:
        return DXGI_FORMAT_R16G16_UNORM;
    case eTF_R16G16S:
        return DXGI_FORMAT_R16G16_SNORM;
    case eTF_R16G16F:
        return DXGI_FORMAT_R16G16_FLOAT;
    case eTF_R11G11B10F:
        return DXGI_FORMAT_R11G11B10_FLOAT;
    case eTF_R10G10B10A2:
        return DXGI_FORMAT_R10G10B10A2_UNORM;
    case eTF_R16G16B16A16:
        return DXGI_FORMAT_R16G16B16A16_UNORM;
    case eTF_R16G16B16A16S:
        return DXGI_FORMAT_R16G16B16A16_SNORM;
    case eTF_R16G16B16A16F:
        return DXGI_FORMAT_R16G16B16A16_FLOAT;
    case eTF_R32G32B32A32F:
        return DXGI_FORMAT_R32G32B32A32_FLOAT;

    case eTF_BC1:
        return DXGI_FORMAT_BC1_UNORM;
    case eTF_BC2:
        return DXGI_FORMAT_BC2_UNORM;
    case eTF_BC3:
        return DXGI_FORMAT_BC3_UNORM;
    case eTF_BC4U:
        return DXGI_FORMAT_BC4_UNORM;
    case eTF_BC4S:
        return DXGI_FORMAT_BC4_SNORM;
    case eTF_BC5U:
        return DXGI_FORMAT_BC5_UNORM;
    case eTF_BC5S:
        return DXGI_FORMAT_BC5_SNORM;
    case eTF_BC6UH:
        return DXGI_FORMAT_BC6H_UF16;
    case eTF_BC6SH:
        return DXGI_FORMAT_BC6H_SF16;
    case eTF_BC7:
        return DXGI_FORMAT_BC7_UNORM;
    case eTF_R9G9B9E5:
        return DXGI_FORMAT_R9G9B9E5_SHAREDEXP;

    // hardware depth buffers
    case eTF_D16:
        return DXGI_FORMAT_D16_UNORM;
    case eTF_D24S8:
        return DXGI_FORMAT_D24_UNORM_S8_UINT;
    case eTF_D32F:
        return DXGI_FORMAT_D32_FLOAT;
    case eTF_D32FS8:
        return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

    // only available as hardware format under DX11.1 with DXGI 1.2
    case eTF_B5G6R5:
        return DXGI_FORMAT_B5G6R5_UNORM;
    case eTF_B5G5R5:
        return DXGI_FORMAT_B5G5R5A1_UNORM;
    //      case eTF_B4G4R4A4:                      return DXGI_FORMAT_B4G4R4A4_UNORM;
    case eTF_B4G4R4A4:
        return DXGI_FORMAT_UNKNOWN;

#if defined(OPENGL) || defined(CRY_USE_METAL)
    // only available as hardware format under OpenGL
    case eTF_EAC_R11:
        return DXGI_FORMAT_EAC_R11_UNORM;
    case eTF_EAC_RG11:
        return DXGI_FORMAT_EAC_RG11_UNORM;
    case eTF_ETC2:
        return DXGI_FORMAT_ETC2_UNORM;
    case eTF_ETC2A:
        return DXGI_FORMAT_ETC2A_UNORM;
#endif //defined(OPENGL)
#ifdef CRY_USE_METAL
    case eTF_PVRTC2:
        return DXGI_FORMAT_PVRTC2_UNORM;
    case eTF_PVRTC4:
        return DXGI_FORMAT_PVRTC4_UNORM;
#endif
#if defined(ANDROID) || defined(CRY_USE_METAL)
    case eTF_ASTC_4x4:
        return DXGI_FORMAT_ASTC_4x4_UNORM;
    case eTF_ASTC_5x4:
        return DXGI_FORMAT_ASTC_5x4_UNORM;
    case eTF_ASTC_5x5:
        return DXGI_FORMAT_ASTC_5x5_UNORM;
    case eTF_ASTC_6x5:
        return DXGI_FORMAT_ASTC_6x5_UNORM;
    case eTF_ASTC_6x6:
        return DXGI_FORMAT_ASTC_6x6_UNORM;
    case eTF_ASTC_8x5:
        return DXGI_FORMAT_ASTC_8x5_UNORM;
    case eTF_ASTC_8x6:
        return DXGI_FORMAT_ASTC_8x6_UNORM;
    case eTF_ASTC_8x8:
        return DXGI_FORMAT_ASTC_8x8_UNORM;
    case eTF_ASTC_10x5:
        return DXGI_FORMAT_ASTC_10x5_UNORM;
    case eTF_ASTC_10x6:
        return DXGI_FORMAT_ASTC_10x6_UNORM;
    case eTF_ASTC_10x8:
        return DXGI_FORMAT_ASTC_10x8_UNORM;
    case eTF_ASTC_10x10:
        return DXGI_FORMAT_ASTC_10x10_UNORM;
    case eTF_ASTC_12x10:
        return DXGI_FORMAT_ASTC_12x10_UNORM;
    case eTF_ASTC_12x12:
        return DXGI_FORMAT_ASTC_12x12_UNORM;
#endif

    // only available as hardware format under DX9
    case eTF_A8L8:
        return DXGI_FORMAT_UNKNOWN;
    case eTF_L8:
        return DXGI_FORMAT_UNKNOWN;
    case eTF_L8V8U8:
        return DXGI_FORMAT_UNKNOWN;
    case eTF_B8G8R8:
        return DXGI_FORMAT_UNKNOWN;
    case eTF_L8V8U8X8:
        return DXGI_FORMAT_UNKNOWN;
    case eTF_B8G8R8X8:
        return DXGI_FORMAT_B8G8R8X8_UNORM;
    case eTF_B8G8R8A8:
        return DXGI_FORMAT_B8G8R8A8_UNORM;
    }

    return DXGI_FORMAT_UNKNOWN;
}

D3DFormat CTexture::GetD3DLinFormat(D3DFormat nFormat)
{
    return nFormat;
}

D3DFormat CTexture::ConvertToSRGBFmt(D3DFormat fmt)
{
    switch (fmt)
    {
    case DXGI_FORMAT_R8G8B8A8_UNORM:
        return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    case DXGI_FORMAT_B8G8R8A8_UNORM:
        return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    case DXGI_FORMAT_B8G8R8X8_UNORM:
        return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;

    case DXGI_FORMAT_BC1_UNORM:
        return DXGI_FORMAT_BC1_UNORM_SRGB;
    case DXGI_FORMAT_BC2_UNORM:
        return DXGI_FORMAT_BC2_UNORM_SRGB;
    case DXGI_FORMAT_BC3_UNORM:
        return DXGI_FORMAT_BC3_UNORM_SRGB;
    case DXGI_FORMAT_BC7_UNORM:
        return DXGI_FORMAT_BC7_UNORM_SRGB;

#if defined(OPENGL) || defined(CRY_USE_METAL)
    case DXGI_FORMAT_ETC2_UNORM:
        return DXGI_FORMAT_ETC2_UNORM_SRGB;
    case DXGI_FORMAT_ETC2A_UNORM:
        return DXGI_FORMAT_ETC2A_UNORM_SRGB;
    case DXGI_FORMAT_EAC_RG11_UNORM:
        return DXGI_FORMAT_EAC_RG11_UNORM;
#endif //defined(OPENGL)

#ifdef CRY_USE_METAL
    case DXGI_FORMAT_PVRTC2_UNORM:
        return DXGI_FORMAT_PVRTC2_UNORM_SRGB;
    case DXGI_FORMAT_PVRTC4_UNORM:
        return DXGI_FORMAT_PVRTC4_UNORM_SRGB;
#endif
#if defined(ANDROID) || defined(CRY_USE_METAL)
    case DXGI_FORMAT_ASTC_4x4_UNORM:
        return DXGI_FORMAT_ASTC_4x4_UNORM_SRGB;
    case DXGI_FORMAT_ASTC_5x4_UNORM:
        return DXGI_FORMAT_ASTC_5x4_UNORM_SRGB;
    case DXGI_FORMAT_ASTC_5x5_UNORM:
        return DXGI_FORMAT_ASTC_5x5_UNORM_SRGB;
    case DXGI_FORMAT_ASTC_6x5_UNORM:
        return DXGI_FORMAT_ASTC_6x5_UNORM_SRGB;
    case DXGI_FORMAT_ASTC_6x6_UNORM:
        return DXGI_FORMAT_ASTC_6x6_UNORM_SRGB;
    case DXGI_FORMAT_ASTC_8x5_UNORM:
        return DXGI_FORMAT_ASTC_8x5_UNORM_SRGB;
    case DXGI_FORMAT_ASTC_8x6_UNORM:
        return DXGI_FORMAT_ASTC_8x6_UNORM_SRGB;
    case DXGI_FORMAT_ASTC_8x8_UNORM:
        return DXGI_FORMAT_ASTC_8x8_UNORM_SRGB;
    case DXGI_FORMAT_ASTC_10x5_UNORM:
        return DXGI_FORMAT_ASTC_10x5_UNORM_SRGB;
    case DXGI_FORMAT_ASTC_10x6_UNORM:
        return DXGI_FORMAT_ASTC_10x6_UNORM_SRGB;
    case DXGI_FORMAT_ASTC_10x8_UNORM:
        return DXGI_FORMAT_ASTC_10x8_UNORM_SRGB;
    case DXGI_FORMAT_ASTC_10x10_UNORM:
        return DXGI_FORMAT_ASTC_10x10_UNORM_SRGB;
    case DXGI_FORMAT_ASTC_12x10_UNORM:
        return DXGI_FORMAT_ASTC_12x10_UNORM_SRGB;
    case DXGI_FORMAT_ASTC_12x12_UNORM:
        return DXGI_FORMAT_ASTC_12x12_UNORM_SRGB;
    case DXGI_FORMAT_R8_UNORM:
        return DXGI_FORMAT_R8_UNORM;
    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
        return DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
#endif
    case DXGI_FORMAT_R10G10B10A2_UNORM:
        return DXGI_FORMAT_R10G10B10A2_UNORM;
    // AntonK: we don't need sRGB space for fp formats, because there is enough precision
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
        return DXGI_FORMAT_R16G16B16A16_FLOAT;
    case DXGI_FORMAT_R11G11B10_FLOAT:
        return DXGI_FORMAT_R11G11B10_FLOAT;
    // There is no SRGB format for BC4
    case DXGI_FORMAT_BC4_SNORM:
        return DXGI_FORMAT_BC4_SNORM;
    case DXGI_FORMAT_BC4_UNORM:
        return DXGI_FORMAT_BC4_UNORM;
    default:
        assert(0);
    }

    return fmt;
}

D3DFormat CTexture::ConvertToSignedFmt(D3DFormat fmt)
{
    switch (fmt)
    {
    case DXGI_FORMAT_R8_UNORM:
        return DXGI_FORMAT_R8_SNORM;
    case DXGI_FORMAT_R8G8_UNORM:
        return DXGI_FORMAT_R8G8_SNORM;
    case DXGI_FORMAT_R8G8B8A8_UNORM:
        return DXGI_FORMAT_R8G8B8A8_SNORM;
    case DXGI_FORMAT_R16_UNORM:
        return DXGI_FORMAT_R16_SNORM;
    case DXGI_FORMAT_R16G16_UNORM:
        return DXGI_FORMAT_R16G16_SNORM;
    case DXGI_FORMAT_R16G16B16A16_UNORM:
        return DXGI_FORMAT_R16G16B16A16_SNORM;
    case DXGI_FORMAT_BC4_UNORM:
        return DXGI_FORMAT_BC4_SNORM;
    case DXGI_FORMAT_BC5_UNORM:
        return DXGI_FORMAT_BC5_SNORM;
    case DXGI_FORMAT_BC6H_UF16:
        return DXGI_FORMAT_BC6H_SF16;

    case DXGI_FORMAT_R32G32B32A32_UINT:
        return DXGI_FORMAT_R32G32B32A32_SINT;
    case DXGI_FORMAT_R32G32B32_UINT:
        return DXGI_FORMAT_R32G32B32_SINT;
    case DXGI_FORMAT_R16G16B16A16_UINT:
        return DXGI_FORMAT_R16G16B16A16_SINT;
    case DXGI_FORMAT_R32G32_UINT:
        return DXGI_FORMAT_R32G32_SINT;
    case DXGI_FORMAT_R8G8B8A8_UINT:
        return DXGI_FORMAT_R8G8B8A8_SINT;
    case DXGI_FORMAT_R16G16_UINT:
        return DXGI_FORMAT_R16G16_SINT;
    case DXGI_FORMAT_R32_UINT:
        return DXGI_FORMAT_R32_SINT;
    case DXGI_FORMAT_R8G8_UINT:
        return DXGI_FORMAT_R8G8_SINT;
    case DXGI_FORMAT_R16_UINT:
        return DXGI_FORMAT_R16_SINT;
    case DXGI_FORMAT_R8_UINT:
        return DXGI_FORMAT_R8_SINT;

    default:
        assert(0);
    }

    return fmt;
}

ETEX_Format CTexture::TexFormatFromDeviceFormat(D3DFormat nFormat)
{
    switch (nFormat)
    {
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        return eTF_R8G8B8A8;
    case DXGI_FORMAT_R8G8B8A8_UNORM:
        return eTF_R8G8B8A8;
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        return eTF_R8G8B8A8;
    case DXGI_FORMAT_R8G8B8A8_SNORM:
        return eTF_R8G8B8A8S;

    case DXGI_FORMAT_A8_UNORM:
        return eTF_A8;
    case DXGI_FORMAT_R8_TYPELESS:
    case DXGI_FORMAT_R8_UNORM:
        return eTF_R8;
    case DXGI_FORMAT_R8_SNORM:
        return eTF_R8S;
    case DXGI_FORMAT_R16_UNORM:
        return eTF_R16;
    case DXGI_FORMAT_R16_UINT:
        return eTF_R16U;
    case DXGI_FORMAT_R16G16_UINT:
        return eTF_R16G16U;
    case DXGI_FORMAT_R10G10B10A2_UINT:
        return eTF_R10G10B10A2UI;
    case DXGI_FORMAT_R16_FLOAT:
        return eTF_R16F;
    case DXGI_FORMAT_R16_TYPELESS:
        return eTF_R16F;
    case DXGI_FORMAT_R32_FLOAT:
        return eTF_R32F;
    case DXGI_FORMAT_R32_TYPELESS:
        return eTF_R32F;
    case DXGI_FORMAT_R8G8_UNORM:
        return eTF_R8G8;
    case DXGI_FORMAT_R8G8_SNORM:
        return eTF_R8G8S;
    case DXGI_FORMAT_R16G16_UNORM:
        return eTF_R16G16;
    case DXGI_FORMAT_R16G16_SNORM:
        return eTF_R16G16S;
    case DXGI_FORMAT_R16G16_FLOAT:
        return eTF_R16G16F;
    case DXGI_FORMAT_R11G11B10_FLOAT:
        return eTF_R11G11B10F;
    case DXGI_FORMAT_R10G10B10A2_UNORM:
        return eTF_R10G10B10A2;
    case DXGI_FORMAT_R10G10B10A2_TYPELESS:
        return eTF_R10G10B10A2;
    case DXGI_FORMAT_R16G16B16A16_UNORM:
        return eTF_R16G16B16A16;
    case DXGI_FORMAT_R16G16B16A16_SNORM:
        return eTF_R16G16B16A16S;
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
        return eTF_R16G16B16A16F;
    case DXGI_FORMAT_R16G16B16A16_TYPELESS:
        return eTF_R16G16B16A16F;
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
        return eTF_R32G32B32A32F;
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:
        return eTF_R32G32B32A32F;

    case DXGI_FORMAT_BC1_TYPELESS:
        return eTF_BC1;
    case DXGI_FORMAT_BC1_UNORM:
        return eTF_BC1;
    case DXGI_FORMAT_BC1_UNORM_SRGB:
        return eTF_BC1;
    case DXGI_FORMAT_BC2_TYPELESS:
        return eTF_BC2;
    case DXGI_FORMAT_BC2_UNORM:
        return eTF_BC2;
    case DXGI_FORMAT_BC2_UNORM_SRGB:
        return eTF_BC2;
    case DXGI_FORMAT_BC3_TYPELESS:
        return eTF_BC3;
    case DXGI_FORMAT_BC3_UNORM:
        return eTF_BC3;
    case DXGI_FORMAT_BC3_UNORM_SRGB:
        return eTF_BC3;
    case DXGI_FORMAT_BC4_TYPELESS:
        return eTF_BC4U;
    case DXGI_FORMAT_BC4_UNORM:
        return eTF_BC4U;
    case DXGI_FORMAT_BC4_SNORM:
        return eTF_BC4S;
    case DXGI_FORMAT_BC5_TYPELESS:
        return eTF_BC5U;
    case DXGI_FORMAT_BC5_UNORM:
        return eTF_BC5U;
    case DXGI_FORMAT_BC5_SNORM:
        return eTF_BC5S;
    case DXGI_FORMAT_BC6H_UF16:
        return eTF_BC6UH;
    case DXGI_FORMAT_BC6H_SF16:
        return eTF_BC6SH;
    case DXGI_FORMAT_BC7_TYPELESS:
        return eTF_BC7;
    case DXGI_FORMAT_BC7_UNORM:
        return eTF_BC7;
    case DXGI_FORMAT_BC7_UNORM_SRGB:
        return eTF_BC7;
    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
        return eTF_R9G9B9E5;

    // hardware depth buffers
    case DXGI_FORMAT_D16_UNORM:
        return eTF_D16;
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
        return eTF_D24S8;
    case DXGI_FORMAT_D32_FLOAT:
        return eTF_D32F;
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        return eTF_D32FS8;

    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
        return eTF_D24S8;
    case DXGI_FORMAT_R24G8_TYPELESS:
        return eTF_D24S8;
    case DXGI_FORMAT_R32G8X24_TYPELESS:
        return eTF_D32FS8;
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
        return eTF_D32FS8;

    // only available as hardware format under DX11.1 with DXGI 1.2
    case DXGI_FORMAT_B5G6R5_UNORM:
        return eTF_B5G6R5;
    case DXGI_FORMAT_B5G5R5A1_UNORM:
        return eTF_B5G5R5;
        //  case DXGI_FORMAT_B4G4R4A4_UNORM:        return eTF_B4G4R4A4;

#if defined(OPENGL) || defined(CRY_USE_METAL)
    // only available as hardware format under OpenGL
    case DXGI_FORMAT_EAC_R11_UNORM:
    case DXGI_FORMAT_EAC_R11_TYPELESS:
        return eTF_EAC_R11;
    case DXGI_FORMAT_EAC_RG11_UNORM:
    case DXGI_FORMAT_EAC_RG11_TYPELESS:
        return eTF_EAC_RG11;
    case DXGI_FORMAT_ETC2_UNORM:
    case DXGI_FORMAT_ETC2_UNORM_SRGB:
    case DXGI_FORMAT_ETC2_TYPELESS:
        return eTF_ETC2;
    case DXGI_FORMAT_ETC2A_UNORM:
    case DXGI_FORMAT_ETC2A_UNORM_SRGB:
    case DXGI_FORMAT_ETC2A_TYPELESS:
        return eTF_ETC2A;
#endif //defined(OPENGL)

#ifdef CRY_USE_METAL
    case DXGI_FORMAT_PVRTC2_TYPELESS:
    case DXGI_FORMAT_PVRTC2_UNORM:
    case DXGI_FORMAT_PVRTC2_UNORM_SRGB:
        return eTF_PVRTC2;
    case DXGI_FORMAT_PVRTC4_TYPELESS:
    case DXGI_FORMAT_PVRTC4_UNORM:
    case DXGI_FORMAT_PVRTC4_UNORM_SRGB:
        return eTF_PVRTC4;
#endif
#if defined(ANDROID) || defined(CRY_USE_METAL)
    case DXGI_FORMAT_ASTC_4x4_TYPELESS:
    case DXGI_FORMAT_ASTC_4x4_UNORM:
    case DXGI_FORMAT_ASTC_4x4_UNORM_SRGB:
        return eTF_ASTC_4x4;
    case DXGI_FORMAT_ASTC_5x4_TYPELESS:
    case DXGI_FORMAT_ASTC_5x4_UNORM:
    case DXGI_FORMAT_ASTC_5x4_UNORM_SRGB:
        return eTF_ASTC_5x4;
    case DXGI_FORMAT_ASTC_5x5_TYPELESS:
    case DXGI_FORMAT_ASTC_5x5_UNORM:
    case DXGI_FORMAT_ASTC_5x5_UNORM_SRGB:
        return eTF_ASTC_5x5;
    case DXGI_FORMAT_ASTC_6x5_TYPELESS:
    case DXGI_FORMAT_ASTC_6x5_UNORM:
    case DXGI_FORMAT_ASTC_6x5_UNORM_SRGB:
        return eTF_ASTC_6x5;
    case DXGI_FORMAT_ASTC_6x6_TYPELESS:
    case DXGI_FORMAT_ASTC_6x6_UNORM:
    case DXGI_FORMAT_ASTC_6x6_UNORM_SRGB:
        return eTF_ASTC_6x6;
    case DXGI_FORMAT_ASTC_8x5_TYPELESS:
    case DXGI_FORMAT_ASTC_8x5_UNORM:
    case DXGI_FORMAT_ASTC_8x5_UNORM_SRGB:
        return eTF_ASTC_8x5;
    case DXGI_FORMAT_ASTC_8x6_TYPELESS:
    case DXGI_FORMAT_ASTC_8x6_UNORM:
    case DXGI_FORMAT_ASTC_8x6_UNORM_SRGB:
        return eTF_ASTC_8x6;
    case DXGI_FORMAT_ASTC_8x8_TYPELESS:
    case DXGI_FORMAT_ASTC_8x8_UNORM:
    case DXGI_FORMAT_ASTC_8x8_UNORM_SRGB:
        return eTF_ASTC_8x8;
    case DXGI_FORMAT_ASTC_10x5_TYPELESS:
    case DXGI_FORMAT_ASTC_10x5_UNORM:
    case DXGI_FORMAT_ASTC_10x5_UNORM_SRGB:
        return eTF_ASTC_10x5;
    case DXGI_FORMAT_ASTC_10x6_TYPELESS:
    case DXGI_FORMAT_ASTC_10x6_UNORM:
    case DXGI_FORMAT_ASTC_10x6_UNORM_SRGB:
        return eTF_ASTC_10x6;
    case DXGI_FORMAT_ASTC_10x8_TYPELESS:
    case DXGI_FORMAT_ASTC_10x8_UNORM:
    case DXGI_FORMAT_ASTC_10x8_UNORM_SRGB:
        return eTF_ASTC_10x8;
    case DXGI_FORMAT_ASTC_10x10_TYPELESS:
    case DXGI_FORMAT_ASTC_10x10_UNORM:
    case DXGI_FORMAT_ASTC_10x10_UNORM_SRGB:
        return eTF_ASTC_10x10;
    case DXGI_FORMAT_ASTC_12x10_TYPELESS:
    case DXGI_FORMAT_ASTC_12x10_UNORM:
    case DXGI_FORMAT_ASTC_12x10_UNORM_SRGB:
        return eTF_ASTC_12x10;
    case DXGI_FORMAT_ASTC_12x12_TYPELESS:
    case DXGI_FORMAT_ASTC_12x12_UNORM:
    case DXGI_FORMAT_ASTC_12x12_UNORM_SRGB:
        return eTF_ASTC_12x12;
#endif

    // only available as hardware format under DX9
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
        return eTF_B8G8R8A8;
    case DXGI_FORMAT_B8G8R8A8_UNORM:
        return eTF_B8G8R8A8;
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        return eTF_B8G8R8A8;
    case DXGI_FORMAT_B8G8R8X8_TYPELESS:
        return eTF_B8G8R8X8;
    case DXGI_FORMAT_B8G8R8X8_UNORM:
        return eTF_B8G8R8X8;
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        return eTF_B8G8R8X8;

    default:
        assert(0);
    }

    return eTF_Unknown;
}

// this function is valid for FT_USAGE_DEPTHSTENCIL textures only
D3DFormat CTexture::ConvertToDepthStencilFmt(D3DFormat nFormat)
{
    switch (nFormat)
    {
    case DXGI_FORMAT_R16_TYPELESS:
        return DXGI_FORMAT_D16_UNORM;
    case DXGI_FORMAT_R24G8_TYPELESS:
        return DXGI_FORMAT_D24_UNORM_S8_UINT;
    case DXGI_FORMAT_R32_TYPELESS:
        return DXGI_FORMAT_D32_FLOAT;
    case DXGI_FORMAT_R32G8X24_TYPELESS:
        return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

    // Don't assert if they pass in a valid depth format...
    case DXGI_FORMAT_D16_UNORM:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        return nFormat;

    default:
        assert(nFormat == DXGI_FORMAT_D16_UNORM || nFormat == DXGI_FORMAT_D24_UNORM_S8_UINT || nFormat == DXGI_FORMAT_D32_FLOAT || nFormat == DXGI_FORMAT_D32_FLOAT_S8X24_UINT);
    }

    return nFormat;
}

D3DFormat CTexture::ConvertToStencilFmt(D3DFormat nFormat)
{
    switch (nFormat)
    {
    case DXGI_FORMAT_R32G8X24_TYPELESS:
        return DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;
    case DXGI_FORMAT_R24G8_TYPELESS:
        return DXGI_FORMAT_X24_TYPELESS_G8_UINT;

    default:
        assert(nFormat == DXGI_FORMAT_X32_TYPELESS_G8X24_UINT || nFormat == DXGI_FORMAT_X24_TYPELESS_G8_UINT);
    }

    return nFormat;
}

D3DFormat CTexture::ConvertToShaderResourceFmt(D3DFormat nFormat)
{
    //handle special cases
    switch (nFormat)
    {
    case DXGI_FORMAT_R16_TYPELESS:
        return DXGI_FORMAT_R16_UNORM;
    case DXGI_FORMAT_R24G8_TYPELESS:
        return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    case DXGI_FORMAT_R32_TYPELESS:
        return DXGI_FORMAT_R32_FLOAT;
    case DXGI_FORMAT_R32G8X24_TYPELESS:
        return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
    //      case DXGI_FORMAT_R32G32_TYPELESS:       return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

    default:
        break;                                     //pass through
    }

    return nFormat;
}

D3DFormat CTexture::ConvertToTypelessFmt(D3DFormat fmt)
{
    switch (fmt)
    {
    case DXGI_FORMAT_R8G8B8A8_UNORM:
        return DXGI_FORMAT_R8G8B8A8_TYPELESS;
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        return DXGI_FORMAT_R8G8B8A8_TYPELESS;
    case DXGI_FORMAT_R8G8B8A8_SNORM:
        return DXGI_FORMAT_R8G8B8A8_TYPELESS;
    case DXGI_FORMAT_R8G8B8A8_UINT:
        return DXGI_FORMAT_R8G8B8A8_TYPELESS;
    case DXGI_FORMAT_R8G8B8A8_SINT:
        return DXGI_FORMAT_R8G8B8A8_TYPELESS;

    case DXGI_FORMAT_B8G8R8A8_UNORM:
        return DXGI_FORMAT_B8G8R8A8_TYPELESS;
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        return DXGI_FORMAT_B8G8R8A8_TYPELESS;
    case DXGI_FORMAT_B8G8R8X8_UNORM:
        return DXGI_FORMAT_B8G8R8X8_TYPELESS;
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        return DXGI_FORMAT_B8G8R8X8_TYPELESS;

    case DXGI_FORMAT_R8_UNORM:
        return DXGI_FORMAT_R8_TYPELESS;
    case DXGI_FORMAT_R8_SNORM:
        return DXGI_FORMAT_R8_TYPELESS;
    case DXGI_FORMAT_R8_UINT:
        return DXGI_FORMAT_R8_TYPELESS;
    case DXGI_FORMAT_R8_SINT:
        return DXGI_FORMAT_R8_TYPELESS;

    case DXGI_FORMAT_R16_UNORM:
        return DXGI_FORMAT_R16_TYPELESS;
    case DXGI_FORMAT_R16_SNORM:
        return DXGI_FORMAT_R16_TYPELESS;
    case DXGI_FORMAT_R16_FLOAT:
        return DXGI_FORMAT_R16_TYPELESS;
    case DXGI_FORMAT_R16_UINT:
        return DXGI_FORMAT_R16_TYPELESS;
    case DXGI_FORMAT_R16_SINT:
        return DXGI_FORMAT_R16_TYPELESS;

    case DXGI_FORMAT_R32_FLOAT:
        return DXGI_FORMAT_R32_TYPELESS;
    case DXGI_FORMAT_R32_UINT:
        return DXGI_FORMAT_R32_TYPELESS;
    case DXGI_FORMAT_R32_SINT:
        return DXGI_FORMAT_R32_TYPELESS;

    case DXGI_FORMAT_R8G8_UNORM:
        return DXGI_FORMAT_R8G8_TYPELESS;
    case DXGI_FORMAT_R8G8_SNORM:
        return DXGI_FORMAT_R8G8_TYPELESS;
    case DXGI_FORMAT_R8G8_UINT:
        return DXGI_FORMAT_R8G8_TYPELESS;
    case DXGI_FORMAT_R8G8_SINT:
        return DXGI_FORMAT_R8G8_TYPELESS;

    case DXGI_FORMAT_R16G16_FLOAT:
        return DXGI_FORMAT_R16G16_TYPELESS;
    case DXGI_FORMAT_R16G16_UNORM:
        return DXGI_FORMAT_R16G16_TYPELESS;
    case DXGI_FORMAT_R16G16_SNORM:
        return DXGI_FORMAT_R16G16_TYPELESS;
    case DXGI_FORMAT_R16G16_UINT:
        return DXGI_FORMAT_R16G16_TYPELESS;
    case DXGI_FORMAT_R16G16_SINT:
        return DXGI_FORMAT_R16G16_TYPELESS;

    case DXGI_FORMAT_R32G32_FLOAT:
        return DXGI_FORMAT_R32G32_TYPELESS;
    case DXGI_FORMAT_R32G32_UINT:
        return DXGI_FORMAT_R32G32_TYPELESS;
    case DXGI_FORMAT_R32G32_SINT:
        return DXGI_FORMAT_R32G32_TYPELESS;

    case DXGI_FORMAT_R32G32B32_FLOAT:
        return DXGI_FORMAT_R32G32B32_TYPELESS;
    case DXGI_FORMAT_R32G32B32_UINT:
        return DXGI_FORMAT_R32G32B32_TYPELESS;
    case DXGI_FORMAT_R32G32B32_SINT:
        return DXGI_FORMAT_R32G32B32_TYPELESS;

    case DXGI_FORMAT_R10G10B10A2_UNORM:
        return DXGI_FORMAT_R10G10B10A2_TYPELESS;
    case DXGI_FORMAT_R10G10B10A2_UINT:
        return DXGI_FORMAT_R10G10B10A2_TYPELESS;

    case DXGI_FORMAT_R16G16B16A16_FLOAT:
        return DXGI_FORMAT_R16G16B16A16_TYPELESS;
    case DXGI_FORMAT_R16G16B16A16_UNORM:
        return DXGI_FORMAT_R16G16B16A16_TYPELESS;
    case DXGI_FORMAT_R16G16B16A16_SNORM:
        return DXGI_FORMAT_R16G16B16A16_TYPELESS;
    case DXGI_FORMAT_R16G16B16A16_UINT:
        return DXGI_FORMAT_R16G16B16A16_TYPELESS;
    case DXGI_FORMAT_R16G16B16A16_SINT:
        return DXGI_FORMAT_R16G16B16A16_TYPELESS;

    case DXGI_FORMAT_R32G32B32A32_FLOAT:
        return DXGI_FORMAT_R32G32B32A32_TYPELESS;
    case DXGI_FORMAT_R32G32B32A32_UINT:
        return DXGI_FORMAT_R32G32B32A32_TYPELESS;
    case DXGI_FORMAT_R32G32B32A32_SINT:
        return DXGI_FORMAT_R32G32B32A32_TYPELESS;

    case DXGI_FORMAT_BC1_UNORM:
        return DXGI_FORMAT_BC1_TYPELESS;
    case DXGI_FORMAT_BC1_UNORM_SRGB:
        return DXGI_FORMAT_BC1_TYPELESS;

    case DXGI_FORMAT_BC2_UNORM:
        return DXGI_FORMAT_BC2_TYPELESS;
    case DXGI_FORMAT_BC2_UNORM_SRGB:
        return DXGI_FORMAT_BC2_TYPELESS;

    case DXGI_FORMAT_BC3_UNORM:
        return DXGI_FORMAT_BC3_TYPELESS;
    case DXGI_FORMAT_BC3_UNORM_SRGB:
        return DXGI_FORMAT_BC3_TYPELESS;

    case DXGI_FORMAT_BC4_UNORM:
        return DXGI_FORMAT_BC4_TYPELESS;
    case DXGI_FORMAT_BC4_SNORM:
        return DXGI_FORMAT_BC4_TYPELESS;

    case DXGI_FORMAT_BC5_UNORM:
        return DXGI_FORMAT_BC5_TYPELESS;
    case DXGI_FORMAT_BC5_SNORM:
        return DXGI_FORMAT_BC5_TYPELESS;

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DTEXTURE_CPP_SECTION_2
    #include AZ_RESTRICTED_FILE(D3DTexture_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
    case DXGI_FORMAT_BC6H_UF16:
        return DXGI_FORMAT_BC6H_TYPELESS;
    case DXGI_FORMAT_BC6H_SF16:
        return DXGI_FORMAT_BC6H_TYPELESS;
#endif
    case DXGI_FORMAT_BC7_UNORM:
        return DXGI_FORMAT_BC7_TYPELESS;
    case DXGI_FORMAT_BC7_UNORM_SRGB:
        return DXGI_FORMAT_BC7_TYPELESS;

#if defined(OPENGL) && !defined(CRY_USE_METAL)
    case DXGI_FORMAT_EAC_R11_UNORM:
        return DXGI_FORMAT_EAC_R11_TYPELESS;
    case DXGI_FORMAT_EAC_R11_SNORM:
        return DXGI_FORMAT_EAC_R11_TYPELESS;
    case DXGI_FORMAT_EAC_RG11_UNORM:
        return DXGI_FORMAT_EAC_RG11_TYPELESS;
    case DXGI_FORMAT_EAC_RG11_SNORM:
        return DXGI_FORMAT_EAC_RG11_TYPELESS;
    case DXGI_FORMAT_ETC2_UNORM:
        return DXGI_FORMAT_ETC2_TYPELESS;
    case DXGI_FORMAT_ETC2_UNORM_SRGB:
        return DXGI_FORMAT_ETC2_TYPELESS;
    case DXGI_FORMAT_ETC2A_UNORM:
        return DXGI_FORMAT_ETC2A_TYPELESS;
    case DXGI_FORMAT_ETC2A_UNORM_SRGB:
        return DXGI_FORMAT_ETC2A_TYPELESS;
#endif //defined(OPENGL)

    case DXGI_FORMAT_D16_UNORM:
        return DXGI_FORMAT_R16_TYPELESS;
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
        return DXGI_FORMAT_R24G8_TYPELESS;
    case DXGI_FORMAT_D32_FLOAT:
        return DXGI_FORMAT_R32_TYPELESS;
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        return DXGI_FORMAT_R32G8X24_TYPELESS;

        // todo: add missing formats if they found required

#ifdef CRY_USE_METAL
    case DXGI_FORMAT_PVRTC2_UNORM:
    case DXGI_FORMAT_PVRTC2_UNORM_SRGB:
        return DXGI_FORMAT_PVRTC2_TYPELESS;

    case DXGI_FORMAT_PVRTC4_UNORM:
    case DXGI_FORMAT_PVRTC4_UNORM_SRGB:
        return DXGI_FORMAT_PVRTC4_TYPELESS;
#endif
#if defined(ANDROID) || defined(CRY_USE_METAL)
    case DXGI_FORMAT_ASTC_4x4_UNORM:
    case DXGI_FORMAT_ASTC_4x4_UNORM_SRGB:
        return DXGI_FORMAT_ASTC_4x4_TYPELESS;

    case DXGI_FORMAT_ASTC_5x4_UNORM:
    case DXGI_FORMAT_ASTC_5x4_UNORM_SRGB:
        return DXGI_FORMAT_ASTC_5x4_TYPELESS;

    case DXGI_FORMAT_ASTC_5x5_UNORM:
    case DXGI_FORMAT_ASTC_5x5_UNORM_SRGB:
        return DXGI_FORMAT_ASTC_5x5_TYPELESS;

    case DXGI_FORMAT_ASTC_6x5_UNORM:
    case DXGI_FORMAT_ASTC_6x5_UNORM_SRGB:
        return DXGI_FORMAT_ASTC_6x5_TYPELESS;

    case DXGI_FORMAT_ASTC_6x6_UNORM:
    case DXGI_FORMAT_ASTC_6x6_UNORM_SRGB:
        return DXGI_FORMAT_ASTC_6x6_TYPELESS;

    case DXGI_FORMAT_ASTC_8x5_UNORM:
    case DXGI_FORMAT_ASTC_8x5_UNORM_SRGB:
        return DXGI_FORMAT_ASTC_8x5_TYPELESS;

    case DXGI_FORMAT_ASTC_8x6_UNORM:
    case DXGI_FORMAT_ASTC_8x6_UNORM_SRGB:
        return DXGI_FORMAT_ASTC_8x6_TYPELESS;

    case DXGI_FORMAT_ASTC_8x8_UNORM:
    case DXGI_FORMAT_ASTC_8x8_UNORM_SRGB:
        return DXGI_FORMAT_ASTC_8x8_TYPELESS;

    case DXGI_FORMAT_ASTC_10x5_UNORM:
    case DXGI_FORMAT_ASTC_10x5_UNORM_SRGB:
        return DXGI_FORMAT_ASTC_10x5_TYPELESS;

    case DXGI_FORMAT_ASTC_10x6_UNORM:
    case DXGI_FORMAT_ASTC_10x6_UNORM_SRGB:
        return DXGI_FORMAT_ASTC_10x6_TYPELESS;

    case DXGI_FORMAT_ASTC_10x8_UNORM:
    case DXGI_FORMAT_ASTC_10x8_UNORM_SRGB:
        return DXGI_FORMAT_ASTC_10x8_TYPELESS;

    case DXGI_FORMAT_ASTC_10x10_UNORM:
    case DXGI_FORMAT_ASTC_10x10_UNORM_SRGB:
        return DXGI_FORMAT_ASTC_10x10_TYPELESS;

    case DXGI_FORMAT_ASTC_12x10_UNORM:
    case DXGI_FORMAT_ASTC_12x10_UNORM_SRGB:
        return DXGI_FORMAT_ASTC_12x10_TYPELESS;

    case DXGI_FORMAT_ASTC_12x12_UNORM:
    case DXGI_FORMAT_ASTC_12x12_UNORM_SRGB:
        return DXGI_FORMAT_ASTC_12x12_TYPELESS;

#endif

    // No conversion on floating point format.
    case DXGI_FORMAT_R11G11B10_FLOAT:
        return DXGI_FORMAT_R11G11B10_FLOAT; 

    default:
        assert(0);
    }

    return fmt;
}

bool CTexture::IsFormatSupported(ETEX_Format eTFDst)
{
    const SPixFormatSupport* rd = &gcpRendD3D->m_hwTexFormatSupport;

    int D3DFmt = DeviceFormatFromTexFormat(eTFDst);
    if (!D3DFmt)
    {
        return false;
    }
    SPixFormat* pFmt;
    for (pFmt = rd->m_FirstPixelFormat; pFmt; pFmt = pFmt->Next)
    {
        if (pFmt->DeviceFormat == D3DFmt && pFmt->IsValid())
        {
            return true;
        }
    }
    return false;
}

ETEX_Format CTexture::ClosestFormatSupported(ETEX_Format eTFDst)
{
    return ClosestFormatSupported(eTFDst, m_pPixelFormat);
}

ETEX_Format CTexture::ClosestFormatSupported(ETEX_Format eTFDst, const SPixFormat*& pPF)
{
    const SPixFormatSupport* rd = &gcpRendD3D->m_hwTexFormatSupport;

    switch (eTFDst)
    {
    case eTF_R8G8B8A8S:
        if (rd->m_FormatR8G8B8A8S.IsValid())
        {
            pPF = &rd->m_FormatR8G8B8A8S;
            return eTF_R8G8B8A8S;
        }
        return eTF_Unknown;
    case eTF_R8G8B8A8:
        if (rd->m_FormatR8G8B8A8.IsValid())
        {
            pPF = &rd->m_FormatR8G8B8A8;
            return eTF_R8G8B8A8;
        }
        return eTF_Unknown;

    case eTF_B5G5R5:
        if (rd->m_FormatB5G5R5.IsValid())
        {
            pPF = &rd->m_FormatB5G5R5;
            return eTF_B5G5R5;
        }
    case eTF_B5G6R5:
        if (rd->m_FormatB5G6R5.IsValid())
        {
            pPF = &rd->m_FormatB5G6R5;
            return eTF_B5G6R5;
        }

    case eTF_B8G8R8X8:
        if (rd->m_FormatB8G8R8X8.IsValid())
        {
            pPF = &rd->m_FormatB8G8R8X8;
            return eTF_B8G8R8X8;
        }
        return eTF_Unknown;
    case eTF_B4G4R4A4:
        if (rd->m_FormatB4G4R4A4.IsValid())
        {
            pPF = &rd->m_FormatB4G4R4A4;
            return eTF_B4G4R4A4;
        }
    case eTF_B8G8R8A8:
        if (rd->m_FormatB8G8R8A8.IsValid())
        {
            pPF = &rd->m_FormatB8G8R8A8;
            return eTF_B8G8R8A8;
        }
        return eTF_Unknown;

    case eTF_A8:
        if (rd->m_FormatA8.IsValid())
        {
            pPF = &rd->m_FormatA8;
            return eTF_A8;
        }
        return eTF_Unknown;


    case eTF_R8:
        if (rd->m_FormatR8.IsValid())
        {
            pPF = &rd->m_FormatR8;
            return eTF_R8;
        }
        return eTF_Unknown;

    case eTF_R8S:
        if (rd->m_FormatR8S.IsValid())
        {
            pPF = &rd->m_FormatR8S;
            return eTF_R8S;
        }
        return eTF_Unknown;

    case eTF_R16:
        if (rd->m_FormatR16.IsValid())
        {
            pPF = &rd->m_FormatR16;
            return eTF_R16;
        }
        if (rd->m_FormatR16G16.IsValid())
        {
            pPF = &rd->m_FormatR16G16;
            return eTF_R16G16;
        }
        return eTF_Unknown;

    case eTF_R16U:
        if (rd->m_FormatR16U.IsValid())
        {
            pPF = &rd->m_FormatR16U;
            return eTF_R16U;
        }
        return eTF_Unknown;

    case eTF_R16G16U:
        if (rd->m_FormatR16G16U.IsValid())
        {
            pPF = &rd->m_FormatR16G16U;
            return eTF_R16G16U;
        }
        return eTF_Unknown;

    case eTF_R10G10B10A2UI:
        if (rd->m_FormatR10G10B10A2UI.IsValid())
        {
            pPF = &rd->m_FormatR10G10B10A2UI;
            return eTF_R10G10B10A2UI;
        }
        return eTF_Unknown;

    case eTF_R16F:
        if (rd->m_FormatR16F.IsValid())
        {
            pPF = &rd->m_FormatR16F;
            return eTF_R16F;
        }
        if (rd->m_FormatR16G16F.IsValid())
        {
            pPF = &rd->m_FormatR16G16F;
            return eTF_R16G16F;
        }
        return eTF_Unknown;

    case eTF_R32F:
        if (rd->m_FormatR32F.IsValid())
        {
            pPF = &rd->m_FormatR32F;
            return eTF_R32F;
        }
        if (rd->m_FormatR16G16F.IsValid())
        {
            pPF = &rd->m_FormatR16G16F;
            return eTF_R16G16F;
        }
        return eTF_Unknown;

    case eTF_R8G8:
        if (rd->m_FormatR8G8.IsValid())
        {
            pPF = &rd->m_FormatR8G8;
            return eTF_R8G8;
        }
        return eTF_Unknown;

    case eTF_R8G8S:
        if (rd->m_FormatR8G8S.IsValid())
        {
            pPF = &rd->m_FormatR8G8S;
            return eTF_R8G8S;
        }
        return eTF_Unknown;

    case eTF_R16G16:
        if (rd->m_FormatR16G16.IsValid())
        {
            pPF = &rd->m_FormatR16G16;
            return eTF_R16G16;
        }
        return eTF_Unknown;

    case eTF_R16G16S:
        if (rd->m_FormatR16G16S.IsValid())
        {
            pPF = &rd->m_FormatR16G16S;
            return eTF_R16G16S;
        }
        return eTF_Unknown;

    case eTF_R16G16F:
        if (rd->m_FormatR16G16F.IsValid())
        {
            pPF = &rd->m_FormatR16G16F;
            return eTF_R16G16F;
        }
        return eTF_Unknown;

    case eTF_R11G11B10F:
        if (rd->m_FormatR11G11B10F.IsValid())
        {
            pPF = &rd->m_FormatR11G11B10F;
            return eTF_R11G11B10F;
        }
        return eTF_Unknown;

    case eTF_R10G10B10A2:
        if (rd->m_FormatR10G10B10A2.IsValid())
        {
            pPF = &rd->m_FormatR10G10B10A2;
            return eTF_R10G10B10A2;
        }
        return eTF_Unknown;

    case eTF_R16G16B16A16:
        if (rd->m_FormatR16G16B16A16.IsValid())
        {
            pPF = &rd->m_FormatR16G16B16A16;
            return eTF_R16G16B16A16;
        }
        return eTF_Unknown;

    case eTF_R16G16B16A16S:
        if (rd->m_FormatR16G16B16A16S.IsValid())
        {
            pPF = &rd->m_FormatR16G16B16A16S;
            return eTF_R16G16B16A16S;
        }
        return eTF_Unknown;

    case eTF_R16G16B16A16F:
        if (rd->m_FormatR16G16B16A16F.IsValid())
        {
            pPF = &rd->m_FormatR16G16B16A16F;
            return eTF_R16G16B16A16F;
        }
        return eTF_Unknown;

    case eTF_R32G32B32A32F:
        if (rd->m_FormatR32G32B32A32F.IsValid())
        {
            pPF = &rd->m_FormatR32G32B32A32F;
            return eTF_R32G32B32A32F;
        }
        return eTF_Unknown;

    case eTF_BC1:
        if (rd->m_FormatBC1.IsValid())
        {
            pPF = &rd->m_FormatBC1;
            return eTF_BC1;
        }
        if (rd->m_FormatR8G8B8A8.IsValid())
        {
            pPF = &rd->m_FormatR8G8B8A8;
            return eTF_R8G8B8A8;
        }
        return eTF_Unknown;

    case eTF_BC2:
        if (rd->m_FormatBC2.IsValid())
        {
            pPF = &rd->m_FormatBC2;
            return eTF_BC2;
        }
        if (rd->m_FormatR8G8B8A8.IsValid())
        {
            pPF = &rd->m_FormatR8G8B8A8;
            return eTF_R8G8B8A8;
        }
        return eTF_Unknown;

    case eTF_BC3:
        if (rd->m_FormatBC3.IsValid())
        {
            pPF = &rd->m_FormatBC3;
            return eTF_BC3;
        }
        if (rd->m_FormatR8G8B8A8.IsValid())
        {
            pPF = &rd->m_FormatR8G8B8A8;
            return eTF_R8G8B8A8;
        }
        return eTF_Unknown;

    case eTF_BC4U:
        if (rd->m_FormatBC4U.IsValid())
        {
            pPF = &rd->m_FormatBC4U;
            return eTF_BC4U;
        }
        if (rd->m_FormatR8.IsValid())
        {
            pPF = &rd->m_FormatR8;
            return eTF_R8;
        }
        return eTF_Unknown;

    case eTF_BC4S:
        if (rd->m_FormatBC4S.IsValid())
        {
            pPF = &rd->m_FormatBC4S;
            return eTF_BC4S;
        }
        if (rd->m_FormatR8S.IsValid())
        {
            pPF = &rd->m_FormatR8S;
            return eTF_R8S;
        }
        return eTF_Unknown;

    case eTF_BC5U:
        if (rd->m_FormatBC5U.IsValid())
        {
            pPF = &rd->m_FormatBC5U;
            return eTF_BC5U;
        }
        if (rd->m_FormatR8G8.IsValid())
        {
            pPF = &rd->m_FormatR8G8;
            return eTF_R8G8;
        }
        return eTF_Unknown;

    case eTF_BC5S:
        if (rd->m_FormatBC5S.IsValid())
        {
            pPF = &rd->m_FormatBC5S;
            return eTF_BC5S;
        }
        if (rd->m_FormatR8G8S.IsValid())
        {
            pPF = &rd->m_FormatR8G8S;
            return eTF_R8G8S;
        }
        return eTF_Unknown;

    case eTF_BC6UH:
        if (rd->m_FormatBC6UH.IsValid())
        {
            pPF = &rd->m_FormatBC6UH;
            return eTF_BC6UH;
        }
        if (rd->m_FormatR16F.IsValid())
        {
            pPF = &rd->m_FormatR16F;
            return eTF_R16F;
        }
        return eTF_Unknown;

    case eTF_BC6SH:
        if (rd->m_FormatBC6SH.IsValid())
        {
            pPF = &rd->m_FormatBC6SH;
            return eTF_BC6SH;
        }
        if (rd->m_FormatR16F.IsValid())
        {
            pPF = &rd->m_FormatR16F;
            return eTF_R16F;
        }
        return eTF_Unknown;

    case eTF_BC7:
        if (rd->m_FormatBC7.IsValid())
        {
            pPF = &rd->m_FormatBC7;
            return eTF_BC7;
        }
        if (rd->m_FormatR8G8B8A8.IsValid())
        {
            pPF = &rd->m_FormatR8G8B8A8;
            return eTF_R8G8B8A8;
        }
        return eTF_Unknown;

    case eTF_R9G9B9E5:
        if (rd->m_FormatR9G9B9E5.IsValid())
        {
            pPF = &rd->m_FormatR9G9B9E5;
            return eTF_R9G9B9E5;
        }
        if (rd->m_FormatR16G16B16A16F.IsValid())
        {
            pPF = &rd->m_FormatR16G16B16A16F;
            return eTF_R16G16B16A16F;
        }
        return eTF_Unknown;

    case eTF_D16:
        if (rd->m_FormatD16.IsValid())
        {
            pPF = &rd->m_FormatD16;
            return eTF_D16;
        }
    case eTF_D24S8:
        if (rd->m_FormatD24S8.IsValid())
        {
            pPF = &rd->m_FormatD24S8;
            return eTF_D24S8;
        }
    case eTF_D32F:
        if (rd->m_FormatD32F.IsValid())
        {
            pPF = &rd->m_FormatD32F;
            return eTF_D32F;
        }
    case eTF_D32FS8:
        if (rd->m_FormatD32FS8.IsValid())
        {
            pPF = &rd->m_FormatD32FS8;
            return eTF_D32FS8;
        }
        return eTF_Unknown;

    case eTF_EAC_R11:
        if (rd->m_FormatEAC_R11.IsValid())
        {
            pPF = &rd->m_FormatEAC_R11;
            return eTF_EAC_R11;
        }
        return eTF_Unknown;

    case eTF_EAC_RG11:
        if (rd->m_FormatEAC_RG11.IsValid())
        {
            pPF = &rd->m_FormatEAC_RG11;
            return eTF_EAC_RG11;
        }
        return eTF_Unknown;

    case eTF_ETC2:
        if (rd->m_FormatETC2.IsValid())
        {
            pPF = &rd->m_FormatETC2;
            return eTF_ETC2;
        }
        return eTF_Unknown;

    case eTF_ETC2A:
        if (rd->m_FormatETC2A.IsValid())
        {
            pPF = &rd->m_FormatETC2A;
            return eTF_ETC2A;
        }
        return eTF_Unknown;

#ifdef CRY_USE_METAL
    case eTF_PVRTC2:
        if (rd->m_FormatPVRTC2.IsValid())
        {
            pPF = &rd->m_FormatPVRTC2;
            return eTF_PVRTC2;
        }
        return eTF_Unknown;

    case eTF_PVRTC4:
        if (rd->m_FormatPVRTC4.IsValid())
        {
            pPF = &rd->m_FormatPVRTC4;
            return eTF_PVRTC4;
        }
        return eTF_Unknown;
#endif
#if defined(ANDROID) || defined(CRY_USE_METAL)
    case eTF_ASTC_4x4:
        if (rd->m_FormatASTC_4x4.IsValid())
        {
            pPF = &rd->m_FormatASTC_4x4;
            return eTF_ASTC_4x4;
        }
        return eTF_Unknown;

    case eTF_ASTC_5x4:
        if (rd->m_FormatASTC_5x4.IsValid())
        {
            pPF = &rd->m_FormatASTC_5x4;
            return eTF_ASTC_5x4;
        }
        return eTF_Unknown;

    case eTF_ASTC_5x5:
        if (rd->m_FormatASTC_5x5.IsValid())
        {
            pPF = &rd->m_FormatASTC_5x5;
            return eTF_ASTC_5x5;
        }
        return eTF_Unknown;

    case eTF_ASTC_6x5:
        if (rd->m_FormatASTC_6x5.IsValid())
        {
            pPF = &rd->m_FormatASTC_6x5;
            return eTF_ASTC_6x5;
        }
        return eTF_Unknown;

    case eTF_ASTC_6x6:
        if (rd->m_FormatASTC_6x6.IsValid())
        {
            pPF = &rd->m_FormatASTC_6x6;
            return eTF_ASTC_6x6;
        }
        return eTF_Unknown;

    case eTF_ASTC_8x5:
        if (rd->m_FormatASTC_8x5.IsValid())
        {
            pPF = &rd->m_FormatASTC_8x5;
            return eTF_ASTC_8x5;
        }
        return eTF_Unknown;

    case eTF_ASTC_8x6:
        if (rd->m_FormatASTC_8x6.IsValid())
        {
            pPF = &rd->m_FormatASTC_8x6;
            return eTF_ASTC_8x6;
        }
        return eTF_Unknown;

    case eTF_ASTC_8x8:
        if (rd->m_FormatASTC_8x8.IsValid())
        {
            pPF = &rd->m_FormatASTC_8x8;
            return eTF_ASTC_8x8;
        }
        return eTF_Unknown;

    case eTF_ASTC_10x5:
        if (rd->m_FormatASTC_10x5.IsValid())
        {
            pPF = &rd->m_FormatASTC_10x5;
            return eTF_ASTC_10x5;
        }
        return eTF_Unknown;

    case eTF_ASTC_10x6:
        if (rd->m_FormatASTC_10x6.IsValid())
        {
            pPF = &rd->m_FormatASTC_10x6;
            return eTF_ASTC_10x6;
        }
        return eTF_Unknown;

    case eTF_ASTC_10x8:
        if (rd->m_FormatASTC_10x8.IsValid())
        {
            pPF = &rd->m_FormatASTC_10x8;
            return eTF_ASTC_10x8;
        }
        return eTF_Unknown;

    case eTF_ASTC_10x10:
        if (rd->m_FormatASTC_10x10.IsValid())
        {
            pPF = &rd->m_FormatASTC_10x10;
            return eTF_ASTC_10x10;
        }
        return eTF_Unknown;

    case eTF_ASTC_12x10:
        if (rd->m_FormatASTC_12x10.IsValid())
        {
            pPF = &rd->m_FormatASTC_12x10;
            return eTF_ASTC_12x10;
        }
        return eTF_Unknown;

    case eTF_ASTC_12x12:
        if (rd->m_FormatASTC_12x12.IsValid())
        {
            pPF = &rd->m_FormatASTC_12x12;
            return eTF_ASTC_12x12;
        }
        return eTF_Unknown;
#endif

    default:
        assert(0);
    }
    return eTF_Unknown;
}

//===============================================================================

bool CTexture::CreateRenderTarget(ETEX_Format eTF, const ColorF& cClear)
{
    if (eTF == eTF_Unknown)
    {
        eTF = m_eTFDst;
    }
    if (eTF == eTF_Unknown)
    {
        return false;
    }
    const byte* pData[6] = { NULL };

    ETEX_Format eTFDst = ClosestFormatSupported(eTF);
    if (eTF != eTFDst)
    {
        return false;
    }
    m_eTFDst = eTF;
    m_nFlags |= FT_USAGE_RENDERTARGET;
    m_cClearColor = cClear;
    bool bRes = CreateDeviceTexture(pData);
    PostCreate();

    // Assign name to RT for enhanced debugging
#if !defined(_RELEASE)
#if defined(WIN32)
#define D3DTEXTURE_CPP_USE_PRIVATEDATA
#elif defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DTEXTURE_CPP_SECTION_3
    #include AZ_RESTRICTED_FILE(D3DTexture_cpp)
#endif
#endif

#if defined(D3DTEXTURE_CPP_USE_PRIVATEDATA)
    if (bRes)
    {
        m_pDevTexture->GetBaseTexture()->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(m_SrcName.c_str()), m_SrcName.c_str());
    }
#endif

    return bRes;
}

// Resolve anti-aliased RT to the texture
bool CTexture::Resolve([[maybe_unused]] int nTarget, [[maybe_unused]] bool bUseViewportSize)
{
    if (m_bResolved)
    {
        return true;
    }

    m_bResolved = true;
    if (!(m_nFlags & FT_USAGE_MSAA))
    {
        return true;
    }

    assert ((m_nFlags & FT_USAGE_RENDERTARGET) && (m_nFlags & FT_USAGE_MSAA) && m_pDeviceShaderResource && m_pDevTexture && m_pRenderTargetData->m_pDeviceTextureMSAA);
    CDeviceTexture* pDestSurf = GetDevTexture();
    CDeviceTexture* pSrcSurf =  GetDevTextureMSAA();

    assert(pSrcSurf != NULL);
    assert(pDestSurf != NULL);

    gcpRendD3D->GetDeviceContext().ResolveSubresource(pDestSurf->Get2DTexture(), 0, pSrcSurf->Get2DTexture(), 0, (DXGI_FORMAT)m_pPixelFormat->DeviceFormat);
    return true;
}

bool CTexture::CreateDeviceTexture(const byte* pData[6])
{
    if (!m_pPixelFormat)
    {
        ETEX_Format eTF = ClosestFormatSupported(m_eTFDst);

        assert(eTF != eTF_Unknown);
        assert(eTF == m_eTFDst);
    }
    assert(m_pPixelFormat);
    if (!m_pPixelFormat)
    {
        return false;
    }

    if (gRenDev->m_pRT->RC_CreateDeviceTexture(this, pData))
    {
        // Assign name to Texture for enhanced debugging
#if !defined(RELEASE)
#if defined (WIN64)
        m_pDevTexture->GetBaseTexture()->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(m_SrcName.c_str()), m_SrcName.c_str());
#elif defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DTEXTURE_CPP_SECTION_4
    #include AZ_RESTRICTED_FILE(D3DTexture_cpp)
#endif
#endif

        return true;
    }

    return false;
}

void CTexture::Unbind()
{
    if (m_pDeviceShaderResource)
    {
        gcpRendD3D->m_DevMan.UnbindSRV(m_pDeviceShaderResource);
    }

    CDeviceTexture* pDevTex = m_pDevTexture;

    if (pDevTex)
    {
        pDevTex->Unbind();
    }
}

bool CTexture::RT_CreateDeviceTexture(const byte* pData[6])
{
    SCOPED_RENDERER_ALLOCATION_NAME_HINT(GetSourceName());
    AZ_ASSET_ATTACH_TO_SCOPE(this);

    HRESULT hr;

    int32 nESRAMOffset = -1;
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DTEXTURE_CPP_SECTION_5
    #include AZ_RESTRICTED_FILE(D3DTexture_cpp)
#endif

    //if we have any device owned resources allocated, we must sync with render thread
    if (m_pDevTexture)
    {
        ReleaseDeviceTexture(false);
    }
    else
    {
        SAFE_DELETE(m_pRenderTargetData);
    }

    CD3D9Renderer* r = gcpRendD3D;
    int nWdt = m_nWidth;
    int nHgt = m_nHeight;
    int nDepth = m_nDepth;
    int nMips = m_nMips;
    AZ_Assert(nWdt > 0 && nHgt > 0 && nMips > 0, "Attempting to create a device texture '%s' with height:%d, width:%d, and mip levels:%d. All three must be > 0", GetSourceName(), nHgt, nWdt, nMips);

    #ifdef CRY_USE_METAL
        bool isMetalCompressedTextureFormat = GetBlockDim(m_eTFSrc) != Vec2i(1);
    #else
        bool isMetalCompressedTextureFormat = false;
    #endif

        bool allowReinterpretingColorSpace = !isMetalCompressedTextureFormat && RenderCapabilities::SupportsTextureViews();
    
    byte* pTemp = NULL;

    CDeviceManager* pDevMan = &r->m_DevMan;

    bool resetSRGB = true;

    if (m_nFlags & (FT_USAGE_RENDERTARGET | FT_USAGE_UNORDERED_ACCESS))
    {
        m_pRenderTargetData = new RenderTargetData();
        #if defined(AZ_RESTRICTED_PLATFORM)
            #define AZ_RESTRICTED_SECTION D3DTEXTURE_CPP_SECTION_6
            #include AZ_RESTRICTED_FILE(D3DTexture_cpp)
        #endif
    }

    uint32 nArraySize = m_nArraySize;

    if (m_eTT == eTT_2D)
    {
        STextureInfo TI;
        D3DFormat D3DFmt = m_pPixelFormat->DeviceFormat;

        //nMips = 1;
        DXGI_FORMAT nFormatOrig = D3DFmt;
        
        resetSRGB = false;
        
        m_bIsSRGB &= m_pPixelFormat->bCanReadSRGB && (m_nFlags & (FT_USAGE_MSAA | FT_USAGE_RENDERTARGET)) == 0;

        if (m_bIsSRGB)
        {
            D3DFmt = ConvertToSRGBFmt(D3DFmt);
        }

        // must use typeless format to allow runtime casting
        if (m_nFlags & FT_USAGE_ALLOWREADSRGB && allowReinterpretingColorSpace)
        {
            D3DFmt = ConvertToTypelessFmt(D3DFmt);
        }

        uint32 nUsage = 0;
        if (m_nFlags & FT_USAGE_DEPTHSTENCIL)
        {
            nUsage |= CDeviceManager::USAGE_DEPTH_STENCIL;
        }
        if (m_nFlags & FT_USAGE_RENDERTARGET)
        {
            nUsage |= CDeviceManager::USAGE_RENDER_TARGET;
        }
        
        #if defined(AZ_PLATFORM_IOS)
            if (m_nFlags & FT_USAGE_MEMORYLESS)
            {
                nUsage |= CDeviceManager::USAGE_MEMORYLESS;
            }
        #endif
        
        if (m_nFlags & FT_USAGE_DYNAMIC)
        {
            nUsage |= CDeviceManager::USAGE_DYNAMIC;
        }
        if (m_nFlags & FT_STAGE_READBACK)
        {
            nUsage |= CDeviceManager::USAGE_STAGE_ACCESS | CDeviceManager::USAGE_CPU_READ;
        }
        if (m_nFlags & FT_STAGE_UPLOAD)
        {
            nUsage |= CDeviceManager::USAGE_STAGE_ACCESS | CDeviceManager::USAGE_CPU_WRITE;
        }
        if (m_nFlags & FT_USAGE_UNORDERED_ACCESS
#if defined(SUPPORT_DEVICE_INFO)
            && r->DevInfo().FeatureLevel() >= D3D_FEATURE_LEVEL_11_0
#endif //defined(SUPPORT_DEVICE_INFO)
            )
        {
            nUsage |= CDeviceManager::USAGE_UNORDERED_ACCESS;
        }
        if ((m_nFlags & (FT_DONT_RELEASE | FT_DONT_STREAM)) == (FT_DONT_RELEASE | FT_DONT_STREAM))
        {
            nUsage |= CDeviceManager::USAGE_ETERNAL;
        }
        if (m_nFlags & FT_USAGE_UAV_RWTEXTURE)
        {
            nUsage |= CDeviceManager::USAGE_UAV_RWTEXTURE;
        }

        if (m_nFlags & FT_FORCE_MIPS)
        {
            nUsage |= CDeviceManager::USAGE_AUTOGENMIPS;
            if (nMips <= 1)
            {
                m_nMips = nMips = max(1, CTexture::CalcNumMips(nWdt, nHgt) - 2);
            }
        }

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DTEXTURE_CPP_SECTION_14
    #include AZ_RESTRICTED_FILE(D3DTexture_cpp)
#endif

        if (m_nFlags & FT_USAGE_MSAA)
        {
            m_pRenderTargetData->m_nMSAASamples = (uint8)r->m_RP.m_MSAAData.Type;
            m_pRenderTargetData->m_nMSAAQuality = (uint8)r->m_RP.m_MSAAData.Quality;

            TI.m_nMSAASamples = m_pRenderTargetData->m_nMSAASamples;
            TI.m_nMSAAQuality = m_pRenderTargetData->m_nMSAAQuality;
            hr = pDevMan->Create2DTexture(m_SrcName, nWdt, nHgt, nMips, nArraySize, nUsage, m_cClearColor, D3DFmt, (D3DPOOL)0, &m_pRenderTargetData->m_pDeviceTextureMSAA, &TI);

            AZ_Assert(SUCCEEDED(hr), "Call to Create2DTexture failed for '%s'.", GetSourceName());
            m_bResolved = false;

            TI.m_nMSAASamples = 1;
            TI.m_nMSAAQuality = 0;
        }

        if (pData[0])
        {
            {
                STextureInfoData InitData[20];
                int w = nWdt;
                int h = nHgt;
                int nOffset = 0;
                const byte* src = pData[0];
                for (int i = 0; i < nMips; i++)
                {
                    if (!w && !h)
                    {
                        break;
                    }
                    if (!w)
                    {
                        w = 1;
                    }
                    if (!h)
                    {
                        h = 1;
                    }

                    int nSize = TextureDataSize(w, h, 1, 1, 1, m_eTFSrc, m_eSrcTileMode);
                    InitData[i].pSysMem = &src[nOffset];
                    if (m_eSrcTileMode == eTM_None)
                    {
                        const Vec2i BlockDim = GetBlockDim(m_eTFSrc);
                        if (BlockDim == Vec2i(1))
                        {
                            InitData[i].SysMemPitch = TextureDataSize(w, 1, 1, 1, 1, m_eTFSrc, eTM_None);
                        }
                        else
                        {
                            int blockSize = CImageExtensionHelper::BytesPerBlock(m_eTFSrc);
                            InitData[i].SysMemPitch = (w + BlockDim.x - 1) / BlockDim.x * blockSize;
                        }

                        //ignored
                        InitData[i].SysMemSlicePitch = nSize;
                        InitData[i].SysMemTileMode = eTM_None;
                    }
                    else
                    {
                        InitData[i].SysMemPitch = 0;
                        InitData[i].SysMemSlicePitch = 0;
                        InitData[i].SysMemTileMode = m_eSrcTileMode;
                    }

                    w >>= 1;
                    h >>= 1;
                    nOffset += nSize;
                }

                TI.m_pData = InitData;

                SAFE_RELEASE(m_pDevTexture);
                hr = pDevMan->Create2DTexture(m_SrcName, nWdt, nHgt, nMips, nArraySize, nUsage, m_cClearColor, D3DFmt, (D3DPOOL)0, &m_pDevTexture, &TI);
                if (!FAILED(hr) && m_pDevTexture)
                {
                    m_pDevTexture->SetNoDelete(!!(m_nFlags & FT_DONT_RELEASE));
                }
            }
        }
        else // no texture data so just make an empty texture
        {
            #if defined(AZ_RESTRICTED_PLATFORM)
                #define AZ_RESTRICTED_SECTION D3DTEXTURE_CPP_SECTION_7
                #include AZ_RESTRICTED_FILE(D3DTexture_cpp)
            #endif

            SAFE_RELEASE(m_pDevTexture);
            hr = pDevMan->Create2DTexture(m_SrcName, nWdt, nHgt, nMips, nArraySize, nUsage, m_cClearColor, D3DFmt, (D3DPOOL)0, &m_pDevTexture, &TI, false, nESRAMOffset);
            if (!FAILED(hr) && m_pDevTexture)
            {
                m_pDevTexture->SetNoDelete(!!(m_nFlags & FT_DONT_RELEASE));
            }
        }

        if (FAILED(hr))
        {
            AZ_Assert(0, "Call to Create2DTexture failed for '%s'.", GetSourceName());
            return false;
        }

        // Restore format
        if (m_nFlags & FT_USAGE_ALLOWREADSRGB)
        {
            D3DFmt = nFormatOrig;
        }

        //////////////////////////////////////////////////////////////////////////
        m_pDeviceShaderResource = static_cast<D3DShaderResourceView*> (CreateDeviceResourceView(SResourceView::ShaderResourceView(m_eTFDst, 0, -1, 0, nMips, m_bIsSRGB, false)));
        
        m_nMinMipVidActive = 0;

        if (m_nFlags & FT_USAGE_ALLOWREADSRGB && allowReinterpretingColorSpace)
        {
            m_pDeviceShaderResourceSRGB = static_cast<D3DShaderResourceView*> (CreateDeviceResourceView(SResourceView::ShaderResourceView(m_eTFDst, 0, -1, 0, nMips, true, false)));
        }
    }
    else if (m_eTT == eTT_Cube)
    {
        STextureInfo TI;
        D3DFormat D3DFmt = m_pPixelFormat->DeviceFormat;
        uint32 nUsage = 0;
        if (m_nFlags & FT_USAGE_DEPTHSTENCIL)
        {
            nUsage |= CDeviceManager::USAGE_DEPTH_STENCIL;
        }
        if (m_nFlags & FT_USAGE_RENDERTARGET)
        {
            nUsage |= CDeviceManager::USAGE_RENDER_TARGET;
        }
        
#if defined(AZ_PLATFORM_IOS)
        if (m_nFlags & FT_USAGE_MEMORYLESS)
        {
            nUsage |= CDeviceManager::USAGE_MEMORYLESS;
        }
#endif
        
        if (m_nFlags & FT_USAGE_DYNAMIC)
        {
            nUsage |= CDeviceManager::USAGE_DYNAMIC;
        }
        if (m_nFlags & FT_STAGE_READBACK)
        {
            nUsage |= CDeviceManager::USAGE_STAGE_ACCESS | CDeviceManager::USAGE_CPU_READ;
        }
        if (m_nFlags & FT_STAGE_UPLOAD)
        {
            nUsage |= CDeviceManager::USAGE_STAGE_ACCESS | CDeviceManager::USAGE_CPU_WRITE;
        }
        if ((m_nFlags & (FT_DONT_RELEASE | FT_DONT_STREAM)) == (FT_DONT_RELEASE | FT_DONT_STREAM))
        {
            nUsage |= CDeviceManager::USAGE_ETERNAL;
        }

        if (m_nFlags & FT_FORCE_MIPS)
        {
            nUsage |= CDeviceManager::USAGE_AUTOGENMIPS;
            if (nMips <= 1)
            {
                m_nMips = nMips = max(1, CTexture::CalcNumMips(nWdt, nHgt) - 2);
            }
        }

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DTEXTURE_CPP_SECTION_14
    #include AZ_RESTRICTED_FILE(D3DTexture_cpp)
#endif

        if (m_nFlags & FT_USAGE_MSAA)
        {
            m_pRenderTargetData->m_nMSAASamples = (uint8)r->m_RP.m_MSAAData.Type;
            m_pRenderTargetData->m_nMSAAQuality = (uint8)r->m_RP.m_MSAAData.Quality;

            TI.m_nMSAASamples = m_pRenderTargetData->m_nMSAASamples;
            TI.m_nMSAAQuality = m_pRenderTargetData->m_nMSAAQuality;
            hr = pDevMan->CreateCubeTexture(m_SrcName, nWdt, nMips, m_nArraySize, nUsage, m_cClearColor, D3DFmt, (D3DPOOL)0, &m_pRenderTargetData->m_pDeviceTextureMSAA, &TI);

            AZ_Assert(SUCCEEDED(hr), "Call to CreateCubeTexuture failed for '%s'.", GetSourceName());
            m_bResolved = false;

            TI.m_nMSAASamples = 1;
            TI.m_nMSAAQuality = 0;
        }
        DXGI_FORMAT nFormatOrig = D3DFmt;
        DXGI_FORMAT nFormatSRGB = D3DFmt;

        resetSRGB = false;

        {
            m_bIsSRGB &= m_pPixelFormat->bCanReadSRGB && (m_nFlags & (FT_USAGE_MSAA | FT_USAGE_RENDERTARGET)) == 0;

            if ((m_bIsSRGB || m_nFlags & FT_USAGE_ALLOWREADSRGB))
            {
                nFormatSRGB = ConvertToSRGBFmt(D3DFmt);
            }

            if (m_bIsSRGB)
            {
                D3DFmt = nFormatSRGB;
            }

            // must use typeless format to allow runtime casting
            if (m_nFlags & FT_USAGE_ALLOWREADSRGB)
            {
                D3DFmt = ConvertToTypelessFmt(D3DFmt);
            }
        }

        if (pData[0])
        {
            AZ_Assert(m_nArraySize == 1, "There is no implementation for tex array data.");

            STextureInfoData InitData[g_nD3D10MaxSupportedSubres];

            for (int nSide = 0; nSide < 6; nSide++)
            {
                int w = nWdt;
                int h = nHgt;
                int nOffset = 0;
                const byte* src = (!(m_nFlags & FT_REPLICATE_TO_ALL_SIDES)) ? pData[nSide] : pData[0];

                for (int i = 0; i < nMips; i++)
                {
                    if (!w && !h)
                    {
                        break;
                    }
                    if (!w)
                    {
                        w = 1;
                    }
                    if (!h)
                    {
                        h = 1;
                    }

                    int nSubresInd = nSide * nMips + i;
                    int nSize = TextureDataSize(w, h, 1, 1, 1, m_eTFSrc, m_eSrcTileMode);

                    InitData[nSubresInd].pSysMem = &src[nOffset];

                    if (m_eSrcTileMode == eTM_None)
                    {
                        InitData[nSubresInd].SysMemPitch = TextureDataSize(w, 1, 1, 1, 1, m_eTFSrc, eTM_None);

                        //ignored
                        InitData[nSubresInd].SysMemSlicePitch = nSize;
                        InitData[nSubresInd].SysMemTileMode = eTM_None;
                    }
                    else
                    {
                        InitData[nSubresInd].SysMemPitch = 0;
                        InitData[nSubresInd].SysMemSlicePitch = 0;
                        InitData[nSubresInd].SysMemTileMode = m_eSrcTileMode;
                    }

                    w >>= 1;
                    h >>= 1;
                    nOffset += nSize;
                }
            }

            TI.m_pData = InitData;
            SAFE_RELEASE(m_pDevTexture);
            hr = pDevMan->CreateCubeTexture(m_SrcName, nWdt, nMips, m_nArraySize, nUsage, m_cClearColor, D3DFmt, (D3DPOOL)0, &m_pDevTexture, &TI);
            if (!FAILED(hr) && m_pDevTexture)
            {
                m_pDevTexture->SetNoDelete(!!(m_nFlags & FT_DONT_RELEASE));
            }
        }
        else
        {
            //hr = gcpRendD3D->GetDevice().CreateTexture2D(&Desc, 0, 0); // validates parameters
            //assert(hr == S_FALSE);
            SAFE_RELEASE(m_pDevTexture);
            hr = pDevMan->CreateCubeTexture(m_SrcName, nWdt, nMips, m_nArraySize, nUsage, m_cClearColor, D3DFmt, (D3DPOOL)0, &m_pDevTexture, &TI);
            if (!FAILED(hr) && m_pDevTexture)
            {
                m_pDevTexture->SetNoDelete(!!(m_nFlags & FT_DONT_RELEASE));
            }
        }
        if (FAILED(hr))
        {
            AZ_Assert(0, "Call to CreateCubeTexture failed for '%s'.", GetSourceName());
            return false;
        }

        if (m_nFlags & FT_USAGE_ALLOWREADSRGB)
        {
            D3DFmt = nFormatOrig;
        }

        D3DCubeTexture* pID3DTexture = m_pDevTexture->GetCubeTexture();

        D3DShaderResourceView* pRes = NULL;
        D3D11_SHADER_RESOURCE_VIEW_DESC ResDesc;
        ZeroStruct(ResDesc);
        ResDesc.Format = (DXGI_FORMAT)ConvertToShaderResourceFmt(D3DFmt);

        if (m_nArraySize > 1)
        {
            ResDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
            ResDesc.TextureCubeArray.MipLevels = nMips;
            ResDesc.TextureCubeArray.First2DArrayFace = 0;
            ResDesc.TextureCubeArray.NumCubes = m_nArraySize;

            // Create the multi-slice shader resource view
            hr = gcpRendD3D->GetDevice().CreateShaderResourceView(pID3DTexture, &ResDesc, &pRes);
            if (FAILED(hr))
            {
                AZ_Assert(0, "Call to CreateShaderResourceView failed for '%s'.", GetSourceName());
                return false;
            }
            m_pDeviceShaderResource = pRes;
            m_nMinMipVidActive = 0;
        }
        else
        {
            ResDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
            ResDesc.TextureCube.MipLevels = nMips;
            ResDesc.TextureCube.MostDetailedMip = 0;
            hr = gcpRendD3D->GetDevice().CreateShaderResourceView(pID3DTexture, &ResDesc, &pRes);
            if (FAILED(hr))
            {
                AZ_Assert(0, "Call to CreateShaderResourceView failed for '%s'.", GetSourceName());
                return false;
            }
            m_pDeviceShaderResource = pRes;
            m_nMinMipVidActive = 0;

            if (m_nFlags & FT_USAGE_ALLOWREADSRGB)
            {
                ResDesc.Format = (DXGI_FORMAT)ConvertToShaderResourceFmt(nFormatSRGB);

                D3DShaderResourceView* pSRGBRes = NULL;
                hr = gcpRendD3D->GetDevice().CreateShaderResourceView(pID3DTexture, &ResDesc, &pSRGBRes);
                if (FAILED(hr))
                {
                    AZ_Assert(0, "Call to CreateShaderResourceView failed for '%s'.", GetSourceName());
                    return false;
                }
                //assign shader resource views
                m_pDeviceShaderResourceSRGB = pSRGBRes;
            }
        }
    }
    else if (m_eTT == eTT_3D)
    {
        STextureInfo TI;
        D3DFormat D3DFmt = m_pPixelFormat->DeviceFormat;

        uint32 nUsage = 0;
        if (m_nFlags & FT_USAGE_DEPTHSTENCIL)
        {
            nUsage |= CDeviceManager::USAGE_DEPTH_STENCIL;
        }
        if (m_nFlags & FT_USAGE_RENDERTARGET)
        {
            nUsage |= CDeviceManager::USAGE_RENDER_TARGET;
        }
        
#if defined(AZ_PLATFORM_IOS)
        if (m_nFlags & FT_USAGE_MEMORYLESS)
        {
            nUsage |= CDeviceManager::USAGE_MEMORYLESS;
        }
#endif
        
        if (m_nFlags & FT_USAGE_DYNAMIC)
        {
            nUsage |= CDeviceManager::USAGE_DYNAMIC;
        }
        if ((m_nFlags & (FT_DONT_RELEASE | FT_DONT_STREAM)) == (FT_DONT_RELEASE | FT_DONT_STREAM))
        {
            nUsage |= CDeviceManager::USAGE_ETERNAL;
        }
        if (m_nFlags & FT_STAGE_READBACK)
        {
            nUsage |= CDeviceManager::USAGE_STAGE_ACCESS | CDeviceManager::USAGE_CPU_READ;
        }
        if (m_nFlags & FT_STAGE_UPLOAD)
        {
            nUsage |= CDeviceManager::USAGE_STAGE_ACCESS | CDeviceManager::USAGE_CPU_WRITE;
        }
        if (m_nFlags & FT_USAGE_UNORDERED_ACCESS
#if defined(SUPPORT_DEVICE_INFO)
            && r->DevInfo().FeatureLevel() >= D3D_FEATURE_LEVEL_11_0
#endif //defined(SUPPORT_DEVICE_INFO)
            )
        {
            nUsage |= CDeviceManager::USAGE_UNORDERED_ACCESS;
        }
        if (m_nFlags & FT_USAGE_UAV_RWTEXTURE)
        {
            nUsage |= CDeviceManager::USAGE_UAV_RWTEXTURE;
        }

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DTEXTURE_CPP_SECTION_14
    #include AZ_RESTRICTED_FILE(D3DTexture_cpp)
#endif

        if (m_nFlags & FT_USAGE_MSAA)
        {
            m_pRenderTargetData->m_nMSAASamples = (uint8)r->m_RP.m_MSAAData.Type;
            m_pRenderTargetData->m_nMSAAQuality = (uint8)r->m_RP.m_MSAAData.Quality;

            TI.m_nMSAASamples = m_pRenderTargetData->m_nMSAASamples;
            TI.m_nMSAAQuality = m_pRenderTargetData->m_nMSAAQuality;
            hr = pDevMan->CreateVolumeTexture(m_SrcName, nWdt, nHgt, m_nDepth, nMips, nUsage, m_cClearColor, D3DFmt, (D3DPOOL)0, &m_pRenderTargetData->m_pDeviceTextureMSAA, &TI);

            AZ_Assert(SUCCEEDED(hr), "Call to CreateVolumeTexture failed for '%s'.", GetSourceName());
            m_bResolved = false;

            TI.m_nMSAASamples = 1;
            TI.m_nMSAAQuality = 0;
        }
        if (pData[0])
        {
            STextureInfoData InitData[15];

            int w = nWdt;
            int h = nHgt;
            int d = nDepth;
            int nOffset = 0;
            const byte* src = pData[0];

            for (int i = 0; i < nMips; i++)
            {
                if (!w && !h && !d)
                {
                    break;
                }
                if (!w)
                {
                    w = 1;
                }
                if (!h)
                {
                    h = 1;
                }
                if (!d)
                {
                    d = 1;
                }

                int nSliceSize = TextureDataSize(w, h, 1, 1, 1, m_eTFSrc);
                int nMipSize = TextureDataSize(w, h, d, 1, 1, m_eTFSrc);
                InitData[i].pSysMem = &src[nOffset];
                InitData[i].SysMemPitch = TextureDataSize(w, 1, 1, 1, 1, m_eTFSrc);

                //ignored
                InitData[i].SysMemSlicePitch = nSliceSize;
                InitData[i].SysMemTileMode = eTM_None;

                w >>= 1;
                h >>= 1;
                d >>= 1;

                nOffset += nMipSize;
            }

            TI.m_pData = InitData;
            SAFE_RELEASE(m_pDevTexture);
            hr = pDevMan->CreateVolumeTexture(m_SrcName, nWdt, nHgt, nDepth, nMips, nUsage, m_cClearColor, D3DFmt, (D3DPOOL)0, &m_pDevTexture, &TI);
            if (!FAILED(hr) && m_pDevTexture)
            {
                m_pDevTexture->SetNoDelete(!!(m_nFlags & FT_DONT_RELEASE));
            }
        }
        else
        {
            //hr = gcpRendD3D->GetDevice().CreateTexture2D(&Desc, 0, 0); // validates parameters
            //assert(hr == S_FALSE);
            SAFE_RELEASE(m_pDevTexture);
            hr = pDevMan->CreateVolumeTexture(m_SrcName, nWdt, nHgt, nDepth, nMips, nUsage, m_cClearColor, D3DFmt, (D3DPOOL)0, &m_pDevTexture, &TI);
            if (!FAILED(hr) && m_pDevTexture)
            {
                m_pDevTexture->SetNoDelete(!!(m_nFlags & FT_DONT_RELEASE));
            }
        }
        if (SUCCEEDED(hr))
        {
            PREFAST_ASSUME(m_pDevTexture);

            m_pDeviceShaderResource = static_cast<D3DShaderResourceView*> (CreateDeviceResourceView(SResourceView::ShaderResourceView(m_eTFDst)));
            m_nMinMipVidActive = 0;
        }
        else
        {
            AZ_Assert(0, "Call to CreateVolumeTexture failed for '%s'.", GetSourceName());
            return false;
        }
    }
    else
    {
        AZ_Assert(0, "Texture type not supported for this function.");
        return false;
    }

    SetTexStates();

    AZ_Assert(!IsStreamed(), "IsStreamed must be false.");
    if (m_pDevTexture)
    {
        m_nActualSize = m_nPersistentSize = m_pDevTexture->GetDeviceSize();
        if (m_nFlags & (FT_USAGE_DYNAMIC | FT_USAGE_RENDERTARGET))
        {
            CTexture::s_nStatsCurDynamicTexMem += m_nActualSize;
        }
        else
        {
            CTexture::s_nStatsCurManagedNonStreamedTexMem += m_nActualSize;
        }
    }

    // Notify that resource is dirty
    InvalidateDeviceResource(eDeviceResourceDirty | eDeviceResourceViewDirty);

#if defined(D3DTEXTURE_CPP_USE_PRIVATEDATA)
    if (m_pDevTexture)
    {
        m_pDevTexture->GetBaseTexture()->SetPrivateData(WKPDID_D3DDebugObjectName, m_SrcName.length(), m_SrcName.c_str());
    }
#endif

    if (!pData[0])
    {
        return true;
    }

    int nOffset = 0;

    if (resetSRGB)
    {
        m_bIsSRGB = false;
    }

    SetWasUnload(false);

    SAFE_DELETE_ARRAY(pTemp);
    return true;
}

void CTexture::ReleaseDeviceTexture(bool bKeepLastMips, bool bFromUnload)
{
    PROFILE_FRAME(CTexture_ReleaseDeviceTexture);
    AZ_TRACE_METHOD();

    if (!gcpRendD3D->m_pRT->IsRenderThread())
    {
        if (!gcpRendD3D->m_pRT->IsMainThread())
        {
            CryFatalError("Texture is deleted from non-main and non-render thread, which causes command buffer corruption!");
        }

        // Push to render thread to process
        gcpRendD3D->m_pRT->RC_ReleaseDeviceTexture(this);
        return;
    }

    Unbind();

    if (!bFromUnload)
    {
        AbortStreamingTasks(this);
    }

    if (s_pTextureStreamer)
    {
        s_pTextureStreamer->OnTextureDestroy(this);
    }

    SAFE_DELETE(m_pRenderTargetData);

    if (!m_bNoTexture)
    {
        CDeviceTexture* pTex = m_pDevTexture;

        D3DShaderResourceView* pSRVSRGB = m_pDeviceShaderResourceSRGB;
        SAFE_RELEASE(pSRVSRGB);
        m_pDeviceShaderResourceSRGB = NULL;

        D3DShaderResourceView* pSRV = m_pDeviceShaderResource;
        SAFE_RELEASE(pSRV);
        m_pDeviceShaderResource = 0;

        D3DSurface* pRTV = m_pDeviceRTV;
        SAFE_RELEASE(pRTV);
        m_pDeviceRTV = NULL;

        D3DSurface* pRTVMS = m_pDeviceRTVMS;
        SAFE_RELEASE(pRTVMS);
        m_pDeviceRTVMS = NULL;

        if (!m_pFileTexMips || !m_pFileTexMips->m_pPoolItem)
        {
            if (IsStreamed())
            {
                SAFE_DELETE(pTex);          // for manually created textures
            }
            else
            {
                SAFE_RELEASE(pTex);
            }
        }

        m_pDevTexture = NULL;

        // otherwise it's already taken into account in the m_pFileTexMips->m_pPoolItem's dtor
        if (!m_pFileTexMips || !m_pFileTexMips->m_pPoolItem)
        {
            if (IsDynamic())
            {
                assert(CTexture::s_nStatsCurDynamicTexMem >= m_nActualSize);
                CTexture::s_nStatsCurDynamicTexMem -= m_nActualSize;
            }
            else if (!IsStreamed())
            {
                assert(CTexture::s_nStatsCurManagedNonStreamedTexMem >= m_nActualSize);
                CTexture::s_nStatsCurManagedNonStreamedTexMem -= m_nActualSize;
            }
        }
        if (m_pFileTexMips)
        {
            m_bStreamPrepared = false;
            StreamRemoveFromPool();
            if (bKeepLastMips)
            {
                const int nLastMipsStart = m_nMips - m_CacheFileHeader.m_nMipsPersistent;
                int nSides = StreamGetNumSlices();
                for (int nS = 0; nS < nSides; nS++)
                {
                    for (int i = 0; i < nLastMipsStart; i++)
                    {
                        SMipData* mp = &m_pFileTexMips->m_pMipHeader[i].m_Mips[nS];
                        mp->Free();
                    }
                }
            }
            else
            {
                Unlink();
                StreamState_ReleaseInfo(this, m_pFileTexMips);
                m_pFileTexMips = NULL;
                m_bStreamed = false;
                SetStreamingInProgress(InvalidStreamSlot);
                m_bStreamRequested = false;
            }
        }
        m_nActualSize = 0;
        m_nPersistentSize = 0;
    }
    else
    {
        m_pDevTexture = NULL;
        m_pDeviceRTV = NULL;
        m_pDeviceShaderResource = NULL;
        m_pDeviceShaderResourceSRGB = NULL;
    }
    m_bNoTexture = false;
}

void* CTexture::CreateDeviceResourceView(const SResourceView& rv)
{
    const SPixFormat* pPixFormat = NULL;
    if (ClosestFormatSupported((ETEX_Format)rv.m_Desc.nFormat, pPixFormat) == eTF_Unknown)
    {
        return NULL;
    }

    HRESULT hr = E_FAIL;
    void* pResult = NULL;

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DTEXTURE_CPP_SECTION_8
    #include AZ_RESTRICTED_FILE(D3DTexture_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
    // DX expects -1 for selecting all mip maps/slices. max count throws an exception
    const uint nSliceCount = rv.m_Desc.nSliceCount == SResourceView().m_Desc.nSliceCount ? (uint) - 1 : (uint)rv.m_Desc.nSliceCount;
#endif

    if (!m_pDevTexture)
    {
        return nullptr;
    }

    D3DTexture* pTex = m_pDevTexture->Get2DTexture();

    switch (rv.m_Desc.eViewType)
    {
    case SResourceView::eShaderResourceView:
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        SetShaderResourceViewDesc(rv, m_eTT, pPixFormat->DeviceFormat, m_nArraySize, nSliceCount, srvDesc);
        if (rv.m_Desc.bMultisample && m_eTT == eTT_2D)
        {
            pTex = m_pRenderTargetData->m_pDeviceTextureMSAA->Get2DTexture();
        }
        D3DShaderResourceView* pSRV = NULL;
        hr = gcpRendD3D->GetDevice().CreateShaderResourceView(pTex, &srvDesc, &pSRV);
        pResult = pSRV;

#if defined(D3DTEXTURE_CPP_USE_PRIVATEDATA)
        if (pSRV)
        {
            AZStd::string srvName = AZStd::string::format("[SRV] %s", m_SrcName.c_str());
            pSRV->SetPrivateData(WKPDID_D3DDebugObjectName, srvName.length(), srvName.c_str());
        }
#endif

    }
    break;
    case SResourceView::eRenderTargetView:
    {
        D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
        SetRenderTargetViewDesc(rv, m_eTT, pPixFormat->DeviceFormat, m_nArraySize, nSliceCount, rtvDesc);
        if (rv.m_Desc.bMultisample && m_eTT == eTT_2D)
        {
            pTex = m_pRenderTargetData->m_pDeviceTextureMSAA->Get2DTexture();
        }
        D3DSurface* pRTV = NULL;
        hr = gcpRendD3D->GetDevice().CreateRenderTargetView(pTex, &rtvDesc, &pRTV);
        pResult = pRTV;

#if defined(D3DTEXTURE_CPP_USE_PRIVATEDATA)
            if (pRTV)
            {
                AZStd::string rtvName = AZStd::string::format("[RTV] %s", m_SrcName.c_str());
                pRTV->SetPrivateData(WKPDID_D3DDebugObjectName, rtvName.length(), rtvName.c_str());
            }
#endif
    }
    break;
    case SResourceView::eDepthStencilView:
    {
        D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
        SetDepthStencilViewDesc(rv, m_eTT, pPixFormat->DeviceFormat, m_nArraySize, nSliceCount, dsvDesc);

        dsvDesc.Flags = rv.m_Desc.nFlags;
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DTEXTURE_CPP_SECTION_9
    #include AZ_RESTRICTED_FILE(D3DTexture_cpp)
#endif

        D3DDepthSurface* pDSV = NULL;
        hr = gcpRendD3D->GetDevice().CreateDepthStencilView(pTex, &dsvDesc, &pDSV);
        pResult = pDSV;

#if defined(D3DTEXTURE_CPP_USE_PRIVATEDATA)
            if (pDSV)
            {
                AZStd::string dsvName = AZStd::string::format("[DSV] %s", m_SrcName.c_str());
                pDSV->SetPrivateData(WKPDID_D3DDebugObjectName, dsvName.length(), dsvName.c_str());
            }
#endif
    }
    break;
    case SResourceView::eUnorderedAccessView:
    {
        D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;

        SetUnorderedAccessViewDesc(rv, m_eTT, pPixFormat->DeviceFormat, m_nArraySize, nSliceCount, uavDesc);

        if (rv.m_Desc.nFlags & 0x1)     // typed UAV loads are only allowed for single-component 32-bit element types
        {
            uavDesc.Format = DXGI_FORMAT_R32_UINT;
        }

        D3DUnorderedAccessView* pUAV = NULL;
        hr = gcpRendD3D->GetDevice().CreateUnorderedAccessView(pTex, &uavDesc, &pUAV);

        pResult = pUAV;
#if defined(D3DTEXTURE_CPP_USE_PRIVATEDATA)
            if (pUAV)
            {
                AZStd::string uavName = AZStd::string::format("[UAV] %s", m_SrcName.c_str());
                pUAV->SetPrivateData(WKPDID_D3DDebugObjectName, uavName.length(), uavName.c_str());
            }
#endif
    }
    break;
    }

    if (FAILED(hr))
    {
        CRY_ASSERT(0);
        return NULL;
    }

    return pResult;
}

void CTexture::SetTexStates()
{
    STexState s;

    const bool noMipFiltering = m_nMips <= 1 && !(m_nFlags & FT_FORCE_MIPS);
    s.m_nMinFilter = FILTER_LINEAR;
    s.m_nMagFilter = FILTER_LINEAR;
    s.m_nMipFilter = noMipFiltering ? FILTER_NONE : FILTER_LINEAR;

    const int addrMode = (m_nFlags & FT_STATE_CLAMP || m_eTT == eTT_Cube) ? TADDR_CLAMP : TADDR_WRAP;
    s.SetClampMode(addrMode, addrMode, addrMode);

    m_nDefState = (uint16)CTexture::GetTexState(s);
}

static uint32 sAddressMode(int nAddress)
{
    IF (nAddress < 0, 0)
    {
        nAddress = TADDR_WRAP;
    }

    switch (nAddress)
    {
    case TADDR_WRAP:
        return D3D11_TEXTURE_ADDRESS_WRAP;
    case TADDR_CLAMP:
        return D3D11_TEXTURE_ADDRESS_CLAMP;
    case TADDR_BORDER:
        return D3D11_TEXTURE_ADDRESS_BORDER;
    case TADDR_MIRROR:
        return D3D11_TEXTURE_ADDRESS_MIRROR;
    default:
        assert(0);
        return D3D11_TEXTURE_ADDRESS_WRAP;
    }
}

void STexState::SetComparisonFilter(bool bEnable)
{
    if (m_pDeviceState)
    {
        D3DSamplerState* pSamp = (D3DSamplerState*)m_pDeviceState;
        SAFE_RELEASE(pSamp);
        m_pDeviceState = NULL;
    }
    m_bComparison = bEnable;
}

bool STexState::SetClampMode(int nAddressU, int nAddressV, int nAddressW)
{
    if (m_pDeviceState)
    {
        D3DSamplerState* pSamp = (D3DSamplerState*)m_pDeviceState;
        SAFE_RELEASE(pSamp);
        m_pDeviceState = NULL;
    }

    m_nAddressU = sAddressMode(nAddressU);
    m_nAddressV = sAddressMode(nAddressV);
    m_nAddressW = sAddressMode(nAddressW);
    return true;
}

bool STexState::SetFilterMode(int nFilter)
{
    IF (nFilter < 0, 0)
    {
        nFilter = FILTER_TRILINEAR;
    }

    if (m_pDeviceState)
    {
        D3DSamplerState* pSamp = (D3DSamplerState*)m_pDeviceState;
        SAFE_RELEASE(pSamp);
        m_pDeviceState = NULL;
    }

    switch (nFilter)
    {
    case FILTER_POINT:
    case FILTER_NONE:
        m_nMinFilter = FILTER_POINT;
        m_nMagFilter = FILTER_POINT;
        m_nMipFilter = FILTER_NONE;
        m_nAnisotropy = 1;
        break;
    case FILTER_LINEAR:
        m_nMinFilter = FILTER_LINEAR;
        m_nMagFilter = FILTER_LINEAR;
        m_nMipFilter = FILTER_NONE;
        m_nAnisotropy = 1;
        break;
    case FILTER_BILINEAR:
        m_nMinFilter = FILTER_LINEAR;
        m_nMagFilter = FILTER_LINEAR;
        m_nMipFilter = FILTER_POINT;
        m_nAnisotropy = 1;
        break;
    case FILTER_TRILINEAR:
        m_nMinFilter = FILTER_LINEAR;
        m_nMagFilter = FILTER_LINEAR;
        m_nMipFilter = FILTER_LINEAR;
        m_nAnisotropy = 1;
        break;
    case FILTER_ANISO2X:
    case FILTER_ANISO4X:
    case FILTER_ANISO8X:
    case FILTER_ANISO16X:
        m_nMinFilter = nFilter;
        m_nMagFilter = nFilter;
        m_nMipFilter = nFilter;
        if (nFilter == FILTER_ANISO2X)
        {
            m_nAnisotropy = min(gcpRendD3D->m_MaxAnisotropyLevel, 2);
        }
        else
        if (nFilter == FILTER_ANISO4X)
        {
            m_nAnisotropy = min(gcpRendD3D->m_MaxAnisotropyLevel, 4);
        }
        else
        if (nFilter == FILTER_ANISO8X)
        {
            m_nAnisotropy = min(gcpRendD3D->m_MaxAnisotropyLevel, 8);
        }
        else
        if (nFilter == FILTER_ANISO16X)
        {
            m_nAnisotropy = min(gcpRendD3D->m_MaxAnisotropyLevel, 16);
        }
        break;
    default:
        assert(0);
        return false;
    }
    return true;
}

void STexState::SetBorderColor(DWORD dwColor)
{
    if (m_pDeviceState)
    {
        D3DSamplerState* pSamp = (D3DSamplerState*)m_pDeviceState;
        SAFE_RELEASE(pSamp);
        m_pDeviceState = NULL;
    }

    m_dwBorderColor = dwColor;
}

void STexState::PostCreate()
{
    if (m_pDeviceState)
    {
        return;
    }

    D3D11_SAMPLER_DESC Desc;
    ZeroStruct(Desc);
    D3DSamplerState* pSamp = NULL;
    // AddressMode of 0 is INVALIDARG
    Desc.AddressU = m_nAddressU ? (D3D11_TEXTURE_ADDRESS_MODE)m_nAddressU : D3D11_TEXTURE_ADDRESS_WRAP;
    Desc.AddressV = m_nAddressV ? (D3D11_TEXTURE_ADDRESS_MODE)m_nAddressV : D3D11_TEXTURE_ADDRESS_WRAP;
    Desc.AddressW = m_nAddressW ? (D3D11_TEXTURE_ADDRESS_MODE)m_nAddressW : D3D11_TEXTURE_ADDRESS_WRAP;
    ColorF col((uint32)m_dwBorderColor);
    Desc.BorderColor[0] = col.r;
    Desc.BorderColor[1] = col.g;
    Desc.BorderColor[2] = col.b;
    Desc.BorderColor[3] = col.a;
    if (m_bComparison)
    {
        Desc.ComparisonFunc = D3D11_COMPARISON_LESS;
    }
    else
    {
        Desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    }

    Desc.MaxAnisotropy = 1;
    Desc.MinLOD = 0;
    if (m_nMipFilter == FILTER_NONE)
    {
        Desc.MaxLOD = 0.0f;
    }
    else
    {
        Desc.MaxLOD = 100.0f;
    }

    Desc.MipLODBias = m_MipBias;

    if (m_bComparison)
    {
        if (m_nMinFilter == FILTER_LINEAR && m_nMagFilter == FILTER_LINEAR && m_nMipFilter == FILTER_LINEAR || m_nMinFilter == FILTER_TRILINEAR || m_nMagFilter == FILTER_TRILINEAR)
        {
            Desc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
        }
        else
        if (m_nMinFilter == FILTER_LINEAR && m_nMagFilter == FILTER_LINEAR && (m_nMipFilter == FILTER_NONE || m_nMipFilter == FILTER_POINT) || m_nMinFilter == FILTER_BILINEAR || m_nMagFilter == FILTER_BILINEAR)
        {
            Desc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
        }
        else
        if (m_nMinFilter == FILTER_POINT && m_nMagFilter == FILTER_POINT && (m_nMipFilter == FILTER_NONE || m_nMipFilter == FILTER_POINT))
        {
            Desc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
        }
        else
        if (m_nMinFilter >= FILTER_ANISO2X && m_nMagFilter >= FILTER_ANISO2X && m_nMipFilter >= FILTER_ANISO2X)
        {
            Desc.Filter = D3D11_FILTER_COMPARISON_ANISOTROPIC;
            Desc.MaxAnisotropy = m_nAnisotropy;
        }
    }
    else
    {
        if (m_nMinFilter == FILTER_LINEAR && m_nMagFilter == FILTER_LINEAR && m_nMipFilter == FILTER_LINEAR || m_nMinFilter == FILTER_TRILINEAR || m_nMagFilter == FILTER_TRILINEAR)
        {
            Desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        }
        else
        if (m_nMinFilter == FILTER_LINEAR && m_nMagFilter == FILTER_LINEAR && (m_nMipFilter == FILTER_NONE || m_nMipFilter == FILTER_POINT) || m_nMinFilter == FILTER_BILINEAR || m_nMagFilter == FILTER_BILINEAR)
        {
            Desc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        }
        else
        if (m_nMinFilter == FILTER_POINT && m_nMagFilter == FILTER_POINT && (m_nMipFilter == FILTER_NONE || m_nMipFilter == FILTER_POINT))
        {
            Desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        }
        else
        if (m_nMinFilter >= FILTER_ANISO2X && m_nMagFilter >= FILTER_ANISO2X && m_nMipFilter >= FILTER_ANISO2X)
        {
            Desc.Filter = D3D11_FILTER_ANISOTROPIC;
            Desc.MaxAnisotropy = m_nAnisotropy;
        }
        else
        {
            assert(0);
        }
    }

    HRESULT hr = gcpRendD3D->GetDevice().CreateSamplerState(&Desc, &pSamp);
    if (SUCCEEDED(hr))
    {
        m_pDeviceState = pSamp;
    }
    else
    {
        assert(0);
    }
}

void STexState::Destroy()
{
    if (m_pDeviceState)
    {
        D3DSamplerState* pSamp = (D3DSamplerState*)m_pDeviceState;
        SAFE_RELEASE(pSamp);
        m_pDeviceState = NULL;
    }
}

void STexState::Init(const STexState& src)
{
    memcpy(this, &src, sizeof(src));
    if (m_pDeviceState)
    {
        D3DSamplerState* pSamp = (D3DSamplerState*)m_pDeviceState;
        pSamp->AddRef();
    }
}

bool CTexture::SetClampingMode(int nAddressU, int nAddressV, int nAddressW)
{
    return s_sDefState.SetClampMode(nAddressU, nAddressV, nAddressW);
}

bool CTexture::SetFilterMode(int nFilter)
{
    return s_sDefState.SetFilterMode(nFilter);
}

void CTexture::UpdateTexStates()
{
    m_nDefState = (uint16)CTexture::GetTexState(s_sDefState);
}

void CTexture::SetSamplerState(int nTS, int nSUnit, EHWShaderClass eHWSC)
{
    FUNCTION_PROFILER_RENDER_FLAT
    assert(gcpRendD3D->m_pRT->IsRenderThread());
    STexStageInfo* const __restrict TexStages = s_TexStages;

    STexState* pTS = &s_TexStates[nTS];
    D3DSamplerState* pSamp = (D3DSamplerState*)pTS->m_pDeviceState;

    assert(pSamp);

    if (pSamp)
    {
        if (eHWSC == eHWSC_Pixel)
        {
            gcpRendD3D->m_DevMan.BindSampler(eHWSC_Pixel, &pSamp, nSUnit, 1);
        }
        else
        if (eHWSC == eHWSC_Domain)
        {
            gcpRendD3D->m_DevMan.BindSampler(eHWSC_Domain, &pSamp, nSUnit, 1);
        }
        else
        if (eHWSC == eHWSC_Vertex)
        {
            gcpRendD3D->m_DevMan.BindSampler(eHWSC_Vertex, &pSamp, nSUnit, 1);
        }
        else
        if (eHWSC == eHWSC_Compute)
        {
            gcpRendD3D->m_DevMan.BindSampler(eHWSC_Compute, &pSamp, nSUnit, 1);
        }
        else
        if (eHWSC == eHWSC_Geometry)
        {
            gcpRendD3D->m_DevMan.BindSampler(eHWSC_Geometry, &pSamp, nSUnit, 1);
        }
        else
        {
            assert(0);
        }
    }
}

void CTexture::ApplySamplerState(int nSUnit, EHWShaderClass eHWSC, int nState)
{
    FUNCTION_PROFILER_RENDER_FLAT

        uint32 nTSSel = Isel32(nState, (int32)m_nDefState);
    assert(nTSSel >= 0 && nTSSel < s_TexStates.size());

    STexStageInfo *TexStages = s_TexStages;
    CDeviceTexture* pDevTex = m_pDevTexture;

    // avoiding L2 cache misses from usage from up ahead
    PrefetchLine(pDevTex, 0);

    assert(nSUnit >= 0 && nSUnit < 16);
    assert((nSUnit >= 0 || nSUnit == -2) && nSUnit < gcpRendD3D->m_NumSamplerSlots);

    CD3D9Renderer *const __restrict rd = gcpRendD3D;
    const int nFrameID = rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_nFrameUpdateID;

    if (IsVertexTexture())
        eHWSC = eHWSC_Vertex;

    SetSamplerState(nTSSel, nSUnit, eHWSC);
}

//------------------------------------------------------------------------------
// Given a texture and a binding slot, this is the method that actually refreshes 
// the texture resource, validates it and finally binds it to the HW stage.
void CTexture::ApplyTexture(int nTUnit, EHWShaderClass eHWSC, SResourceView::KeyType nResViewKey)
{
    FUNCTION_PROFILER_RENDER_FLAT

    STexStageInfo *TexStages = s_TexStages;
    CDeviceTexture* pDevTex = m_pDevTexture;

    // avoiding L2 cache misses from usage from up ahead
    PrefetchLine(pDevTex, 0);

    assert(nTUnit >= 0 && nTUnit < gcpRendD3D->m_NumResourceSlots);

    CD3D9Renderer *const __restrict rd = gcpRendD3D;
    const int nFrameID = rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_nFrameUpdateID;

    if (IsStreamed() && !m_bPostponed)
    {
        bool bIsUnloaded = IsUnloaded();

        assert(m_pFileTexMips);
        if (bIsUnloaded || !m_bStreamPrepared || IsPartiallyLoaded())
        {
            PROFILE_FRAME(Texture_Precache);
            if (!CRenderer::CV_r_texturesstreaming || !m_bStreamPrepared || bIsUnloaded)
            {
                if (bIsUnloaded)
                    StreamLoadFromCache(0);
                else
                    Load(m_eTFDst);

                pDevTex = m_pDevTexture;
            }
        }
    }

    IF(this != CTexture::s_pTexNULL && (!pDevTex || !pDevTex->GetBaseTexture()), 0)
    {
        // apply black by default
        CTexture*     pBlackTexture = CTextureManager::Instance()->GetBlackTexture();
        if (m_eTT != eTT_Cube && pBlackTexture)
        {
            pBlackTexture->ApplyTexture(nTUnit, eHWSC, nResViewKey);
        }
        else
        {
            pBlackTexture = CTextureManager::Instance()->GetBlackTextureCM();
            if (m_eTT == eTT_Cube && pBlackTexture)
            {
                pBlackTexture->ApplyTexture(nTUnit, eHWSC, nResViewKey);
            }
        }
        return;
    }

    // Resolve multisampled RT to texture
    if (!m_bResolved)
    {
        Resolve();
    }

    bool bStreamed = IsStreamed();
    if (m_nAccessFrameID != nFrameID)
    {
        m_nAccessFrameID = nFrameID;

#if !defined(_RELEASE)
        rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_NumTextures++;
        if (m_nFlags & (FT_USAGE_RENDERTARGET | FT_USAGE_DYNAMIC))
        {
            rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_DynTexturesSize += m_nActualSize;
        }
        else
        {
            if (bStreamed)
            {
                rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_ManagedTexturesStreamVidSize += m_nActualSize;
                rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_ManagedTexturesStreamSysSize += StreamComputeDevDataSize(0);
            }
            else
            {
                rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_ManagedTexturesSysMemSize += m_nActualSize;
                rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_ManagedTexturesVidMemSize += m_nActualSize;
            }
        }
#endif

        // mip map fade in
        if (bStreamed)
        {
            const float fCurrentMipBias = m_fCurrentMipBias;
            if (fabsf(fCurrentMipBias) > 0.26667f)
            {
                // one mip per half a second
                m_fCurrentMipBias -= 0.26667f * fCurrentMipBias;

                gcpRendD3D->GetDeviceContext().SetResourceMinLOD(pDevTex->Get2DTexture(), m_fCurrentMipBias + (float)m_nMinMipVidUploaded);
            }
            else if (fCurrentMipBias != 0.f)
            {
                m_fCurrentMipBias = 0.f;
            }
        }
    }

    if (IsVertexTexture())
        eHWSC = eHWSC_Vertex;

    const bool bUnnorderedAcessView = SResourceView(nResViewKey).m_Desc.eViewType == SResourceView::eUnorderedAccessView;

    D3DShaderResourceView* pResView = GetShaderResourceView(nResViewKey);

    if (pDevTex == TexStages[nTUnit].m_DevTexture && pResView == TexStages[nTUnit].m_pCurResView && eHWSC == TexStages[nTUnit].m_eHWSC)
        return;

    TexStages[nTUnit].m_pCurResView = pResView;
    TexStages[nTUnit].m_eHWSC = eHWSC;

    //  <DEPRECATED> This must get re-factored post C3.  
    //  This check is ultra-buggy, render targets setup is deferred until last moment might not be matching this check at all. Also very wrong for MRTs

    if (rd->m_pCurTarget[0] == this)
    {
        //assert(rd->m_pCurTarget[0]->m_pDeviceRTV);
        rd->m_pNewTarget[0]->m_bWasSetRT = false;
        rd->GetDeviceContext().OMSetRenderTargets(1, &rd->m_pNewTarget[0]->m_pTarget, rd->m_pNewTarget[0]->m_pDepth);
    }

    TexStages[nTUnit].m_DevTexture = pDevTex;

#if !defined(_RELEASE)
    rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_NumTextChanges++;
#endif

#ifdef DO_RENDERLOG
    if (CRenderer::CV_r_log >= 3)
    {
        if (IsNoTexture())
        {
            rd->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "CTexture::Apply(): (%d) \"%s\" (Not found)\n", nTUnit, m_SrcName.c_str());
        }
        else
        {
            rd->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "CTexture::Apply(): (%d) \"%s\"\n", nTUnit, m_SrcName.c_str());
        }
    }
#endif

    {
#ifdef DO_RENDERLOG
        if (CRenderer::CV_r_log >= 3 && int64(nResViewKey) >= 0)
        {
            rd->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "CTexture::Apply(): Shader Resource View: %ul \n", nResViewKey);
        }
#endif

        if (bUnnorderedAcessView)
        {
            // todo: 
            // - add support for pixel shader side via OMSetRenderTargetsAndUnorderedAccessViews
            // - DX11.1 very likely API will be similar to CSSetUnorderedAccessViews, but for all stages
            D3DUnorderedAccessView *pUAV = (D3DUnorderedAccessView*)pResView;
            rd->GetDeviceContext().CSSetUnorderedAccessViews(nTUnit, 1, &pUAV, NULL);
            return;
        }

        {
            if (IsVertexTexture())
                eHWSC = eHWSC_Vertex;

            if (eHWSC == eHWSC_Pixel)
            {
                rd->m_DevMan.BindSRV(eHWSC_Pixel, pResView, nTUnit);
            }
            else if (eHWSC == eHWSC_Vertex)
            {
                rd->m_DevMan.BindSRV(eHWSC_Vertex, pResView, nTUnit);
            }
            else if (eHWSC == eHWSC_Domain)
            {
                rd->m_DevMan.BindSRV(eHWSC_Domain, pResView, nTUnit);
            }
            else if (eHWSC == eHWSC_Compute)
            {
                rd->m_DevMan.BindSRV(eHWSC_Compute, pResView, nTUnit);
            }
            else
            {
                assert(0);
            }
        }
    }
}

void CTexture::Apply(int nTUnit, int nState, int nTexMatSlot, int nSUnit, SResourceView::KeyType nResViewKey, EHWShaderClass eHWSC)
{
    FUNCTION_PROFILER_RENDER_FLAT
    assert(nTUnit >= 0);

    uint32 nTSSel = Isel32(nState, (int32)m_nDefState);
    assert(nTSSel >= 0 && nTSSel < s_TexStates.size());

#if defined(OPENGL)
    // Due to driver issues on MALI gpus only point filtering is allowed for 32 bit float textures.
    // If another filtering is used the sampler returns black.
    if ((gcpRendD3D->m_Features | RFT_HW_ARM_MALI) && 
        (m_eTFDst == eTF_R32F || m_eTFDst == eTF_R32G32B32A32F) &&
        (s_TexStates[nTSSel].m_nMagFilter != FILTER_POINT || s_TexStates[nTSSel].m_nMinFilter != FILTER_POINT))
    {
        STexState newState = s_TexStates[nTSSel];
        newState.SetFilterMode(FILTER_POINT);
        nTSSel = CTexture::GetTexState(newState);
        AZ_WarningOnce("Texture", false, "The current device only supports point filtering for full float textures. Forcing filtering for texture in slot %d", nTUnit);
    }
#endif // defined(OPENGL)

    STexStageInfo* TexStages = s_TexStages;

    CDeviceTexture* pDevTex = m_pDevTexture;

    // avoiding L2 cache misses from usage from up ahead
    PrefetchLine(pDevTex, 0);

    if (nSUnit >= -1)
    {
        nSUnit = Isel32(nSUnit, nTUnit);
    }

    assert(nTUnit >= 0 && nTUnit < gcpRendD3D->m_NumResourceSlots);
    assert((nSUnit >= 0 || nSUnit == -2) && nSUnit < gcpRendD3D->m_NumSamplerSlots);

    CD3D9Renderer* const __restrict rd = gcpRendD3D;
    const int nFrameID = rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_nFrameUpdateID;

    if (IsStreamed() && !m_bPostponed)
    {
        bool bIsUnloaded = IsUnloaded();

        assert(m_pFileTexMips);
        if (bIsUnloaded || !m_bStreamPrepared || IsPartiallyLoaded())
        {
            PROFILE_FRAME(Texture_Precache);
            if (!CRenderer::CV_r_texturesstreaming || !m_bStreamPrepared || bIsUnloaded)
            {
                if (bIsUnloaded)
                {
                    StreamLoadFromCache(0);
                }
                else
                {
                    Load(m_eTFDst);
                }

                pDevTex = m_pDevTexture;
            }
        }

        if (nTexMatSlot != EFTT_UNKNOWN)
        {
            m_nStreamingPriority = max((int8)m_nStreamingPriority, TextureHelpers::LookupTexPriority((EEfResTextures)nTexMatSlot));
        }
    }

    IF (this != CTexture::s_pTexNULL && (!pDevTex || !pDevTex->GetBaseTexture()), 0)
    {
        // apply black by default
        if (m_eTT != eTT_Cube && CTextureManager::Instance()->GetBlackTexture() && this != CTextureManager::Instance()->GetBlackTexture())
        {
            CTextureManager::Instance()->GetBlackTexture()->Apply(nTUnit, nState, nTexMatSlot, nSUnit, nResViewKey);
        }
        else if (m_eTT == eTT_Cube && CTextureManager::Instance()->GetBlackTextureCM() && this != CTextureManager::Instance()->GetBlackTextureCM())
        {
            CTextureManager::Instance()->GetBlackTextureCM()->Apply(nTUnit, nState, nTexMatSlot, nSUnit, nResViewKey);
        }
        return;
    }

    // Resolve multisampled RT to texture
    if (!m_bResolved)
    {
        Resolve();
    }
    bool bStreamed = IsStreamed();
    if (m_nAccessFrameID != nFrameID)
    {
        m_nAccessFrameID = nFrameID;

#if !defined(_RELEASE)
        rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_NumTextures++;
        if (m_nFlags & (FT_USAGE_RENDERTARGET | FT_USAGE_DYNAMIC))
        {
            rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_DynTexturesSize += m_nActualSize;
        }
        else
        {
            if (bStreamed)
            {
                rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_ManagedTexturesStreamVidSize += m_nActualSize;
                rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_ManagedTexturesStreamSysSize += StreamComputeDevDataSize(0);
            }
            else
            {
                rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_ManagedTexturesSysMemSize += m_nActualSize;
                rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_ManagedTexturesVidMemSize += m_nActualSize;
            }
        }
#endif

        // mip map fade in
        if (bStreamed)
        {
            const float fCurrentMipBias = m_fCurrentMipBias;
            if (fabsf(fCurrentMipBias) > 0.26667f)
            {
                // one mip per half a second
                m_fCurrentMipBias -= 0.26667f * fCurrentMipBias;
#if defined(CRY_USE_METAL)
                //For metal, the lodminclamp is set once at initialization for the mtlsamplerstate. The mtlSamplerDescriptor's properties
                //are only used during mtlSamplerState object creation; once created the behaviour of a sampler state object is
                //fixed and cannot be changed. Hence we modify the descriptor with minlod and recreate the sampler state.
                STexState* pTS = &s_TexStates[nTSSel];
                D3DSamplerState* pSamp = (D3DSamplerState*)pTS->m_pDeviceState;
                pSamp->SetLodMinClamp(m_fCurrentMipBias + static_cast<float>(m_nMinMipVidUploaded));
#else
                gcpRendD3D->GetDeviceContext().SetResourceMinLOD(pDevTex->Get2DTexture(), m_fCurrentMipBias + static_cast<float>(m_nMinMipVidUploaded));
#endif
            }
            else if (fCurrentMipBias != 0.f)
            {
                m_fCurrentMipBias = 0.f;
            }
        }
    }

    if (IsVertexTexture())
    {
        eHWSC = eHWSC_Vertex;
    }

    const bool bUnnorderedAcessView = SResourceView(nResViewKey).m_Desc.eViewType == SResourceView::eUnorderedAccessView;
    if (!bUnnorderedAcessView && nSUnit >= 0)
    {
        SetSamplerState(nTSSel, nSUnit, eHWSC);
    }

    D3DShaderResourceView* pResView = GetShaderResourceView(nResViewKey, s_TexStates[nTSSel].m_bSRGBLookup);

    if (pDevTex == TexStages[nTUnit].m_DevTexture && pResView == TexStages[nTUnit].m_pCurResView && eHWSC == TexStages[nTUnit].m_eHWSC)
    {
        return;
    }

    TexStages[nTUnit].m_pCurResView = pResView;
    TexStages[nTUnit].m_eHWSC = eHWSC;
    //  <DEPRECATED> This must get re-factored post C3.
    //  -   This check is ultra-buggy, render targets setup is deferred until last moment might not be matching this check at all. Also very wrong for MRTs

    if (rd->m_pCurTarget[0] == this)
    {
        //assert(rd->m_pCurTarget[0]->m_pDeviceRTV);
        rd->m_pNewTarget[0]->m_bWasSetRT = false;
        rd->GetDeviceContext().OMSetRenderTargets(1, &rd->m_pNewTarget[0]->m_pTarget, rd->m_pNewTarget[0]->m_pDepth);
    }

    TexStages[nTUnit].m_DevTexture = pDevTex;

#if !defined(_RELEASE)
    rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_NumTextChanges++;
#endif

#ifdef DO_RENDERLOG
    if (CRenderer::CV_r_log >= 3)
    {
        if (IsNoTexture())
        {
            rd->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "CTexture::Apply(): (%d) \"%s\" (Not found)\n", nTUnit, m_SrcName.c_str());
        }
        else
        {
            rd->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "CTexture::Apply(): (%d) \"%s\"\n", nTUnit, m_SrcName.c_str());
        }
    }
#endif

    {
#ifdef DO_RENDERLOG
        if (CRenderer::CV_r_log >= 3 && int64(nResViewKey) >= 0)
        {
            rd->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "CTexture::Apply(): Shader Resource View: %ul \n", nResViewKey);
        }
#endif

        if (bUnnorderedAcessView)
        {
            // todo:
            // - add support for pixel shader side via OMSetRenderTargetsAndUnorderedAccessViews
            // - DX11.1 very likely API will be similar to CSSetUnorderedAccessViews, but for all stages
            D3DUnorderedAccessView* pUAV = (D3DUnorderedAccessView*)pResView;
            rd->GetDeviceContext().CSSetUnorderedAccessViews(nTUnit, 1, &pUAV, NULL);
            return;
        }

        {
            if (IsVertexTexture())
            {
                eHWSC = eHWSC_Vertex;
            }

            if (eHWSC == eHWSC_Pixel)
            {
                rd->m_DevMan.BindSRV(eHWSC_Pixel, pResView, nTUnit);
            }
            else
            if (eHWSC == eHWSC_Vertex)
            {
                rd->m_DevMan.BindSRV(eHWSC_Vertex, pResView, nTUnit);
            }
            else
            if (eHWSC == eHWSC_Domain)
            {
                rd->m_DevMan.BindSRV(eHWSC_Domain, pResView, nTUnit);
            }
            else
            if (eHWSC == eHWSC_Compute)
            {
                rd->m_DevMan.BindSRV(eHWSC_Compute, pResView, nTUnit);
            }
            else
            if (eHWSC == eHWSC_Geometry)
            {
                rd->m_DevMan.BindSRV(eHWSC_Geometry, pResView, nTUnit);
            }
            else
            {
                assert(0);
            }
        }
    }
}

void CTexture::UpdateTextureRegion(const uint8_t* data, int nX, int nY, int nZ, int USize, int VSize, int ZSize, ETEX_Format eTFSrc)
{
    gRenDev->m_pRT->RC_UpdateTextureRegion(this, data, nX, nY, nZ, USize, VSize, ZSize, eTFSrc);
}

void CTexture::RT_UpdateTextureRegion(const byte* data, int nX, int nY, int nZ, int USize, int VSize, int ZSize, ETEX_Format eTFSrc)
{
    PROFILE_FRAME(UpdateTextureRegion);

    if (m_eTT != eTT_2D && m_eTT != eTT_3D)
    {
        assert(0);
        return;
    }

    HRESULT hr = S_OK;
    CDeviceTexture* pDevTexture = GetDevTexture();
    assert(pDevTexture);
    if (!pDevTexture)
    {
        return;
    }

    DXGI_FORMAT frmtSrc = (DXGI_FORMAT)CTexture::DeviceFormatFromTexFormat(eTFSrc);
    bool bDone = false;
    D3D11_BOX rc = {aznumeric_caster(nX), aznumeric_caster(nY), 0, aznumeric_caster(nX + USize), aznumeric_caster(nY + VSize), 1};
    if (m_eTT == eTT_2D)
    {
        if (GetBlockDim(m_eTFDst) == Vec2i(1))
        {
            int nBPPSrc = CTexture::BytesPerBlock(eTFSrc);
            int nBPPDst = CTexture::BytesPerBlock(m_eTFDst);
            if (nBPPSrc == nBPPDst)
            {
                int nRowPitch = CTexture::TextureDataSize(USize, 1, 1, 1, 1, eTFSrc);
                const int nSlicePitch = CTexture::TextureDataSize(USize, VSize, 1, 1, 1, eTFSrc);
                gcpRendD3D->GetDeviceContext().UpdateSubresource(pDevTexture->Get2DTexture(), 0, &rc, data, nRowPitch, nSlicePitch);
                bDone = true;
            }
            else
            {
                assert(0);
                bDone = true;
            }
        }
    }
    else if (m_eTT == eTT_3D)
    {
        int nFrame = gRenDev->m_nFrameSwapID;
        rc.front = nZ;
        rc.back = nZ + ZSize;

        int nBPPSrc = CTexture::BytesPerBlock(eTFSrc);
        int nBPPDst = CTexture::BytesPerBlock(m_eTFDst);
        if (nBPPSrc == nBPPDst)
        {
            if (m_nFlags & FT_USAGE_DYNAMIC)
            {
                D3DVolumeTexture* pDT = pDevTexture->GetVolumeTexture();
                int cZ, cY;
                for (cZ = nZ; cZ < ZSize; cZ++)
                {
                    D3D11_MAPPED_SUBRESOURCE lrct;
                    uint32 nLockFlags = D3D11_MAP_WRITE_DISCARD;
                    uint32 nSubRes = D3D11CalcSubresource(0, cZ, 1);

                    hr = gcpRendD3D->GetDeviceContext().Map(pDT, nSubRes, (D3D11_MAP)nLockFlags, 0, &lrct);
                    assert(hr == S_OK);

                    byte* pDst = ((byte*)lrct.pData) + nX * 4 + nY * lrct.RowPitch;
                    for (cY = 0; cY < VSize; cY++)
                    {
                        cryMemcpy(pDst, data, USize * 4);
                        data += USize * 4;
                        pDst += lrct.RowPitch;
                    }
                    gcpRendD3D->GetDeviceContext().Unmap(pDT, nSubRes);
                }
            }
            else
            {
                int U = USize;
                int V = VSize;
                int Z = ZSize;
                for (int i = 0; i < m_nMips; i++)
                {
                    if (!U)
                    {
                        U = 1;
                    }
                    if (!V)
                    {
                        V = 1;
                    }
                    if (!Z)
                    {
                        Z = 1;
                    }

                    int nRowPitch = CTexture::TextureDataSize(U, 1, 1, 1, 1, eTFSrc);
                    int nDepthPitch = m_eTT == eTT_3D ? CTexture::TextureDataSize(U, V, 1, 1, 1, eTFSrc) : 0;

                    gcpRendD3D->GetDeviceContext().UpdateSubresource(pDevTexture->GetBaseTexture(), i, &rc, data, nRowPitch, nDepthPitch);
                    bDone = true;

                    data += nDepthPitch * Z;

                    U >>= 1;
                    V >>= 1;
                    Z >>= 1;

                    rc.front >>= 1;
                    rc.left >>= 1;
                    rc.top >>= 1;

                    rc.back = max(rc.front + 1, rc.back >> 1);
                    rc.right = max(rc.left + 4, rc.right >> 1);
                    rc.bottom = max(rc.top + 4, rc.bottom >> 1);
                }
            }
        }
        else if ((eTFSrc == eTF_B8G8R8A8 || eTFSrc == eTF_B8G8R8X8) && (m_eTFDst == eTF_B5G6R5))
        {
            assert(0);
            bDone = true;
        }
    }

    if (!bDone)
    {
        D3D11_BOX box;
        ZeroStruct(box);
        box.right = USize;
        box.bottom = VSize;
        box.back = 1;
        const int nPitch = CTexture::TextureDataSize(USize, 1, 1, 1, 1, eTFSrc);
        const int nSlicePitch = CTexture::TextureDataSize(USize, VSize, 1, 1, 1, eTFSrc);
        gcpRendD3D->GetDeviceContext().UpdateSubresource(pDevTexture->Get2DTexture(), 0, &box, data, nPitch, nSlicePitch);
    }
}

bool CTexture::Clear()
{
    if (!(m_nFlags & FT_USAGE_RENDERTARGET))
        return false;

    gRenDev->m_pRT->RC_ClearTarget(this, m_cClearColor);
    return true;
}

bool CTexture::Clear(const ColorF& color)
{
    if (!(m_nFlags & FT_USAGE_RENDERTARGET))
    {
        return false;
    }

    gRenDev->m_pRT->RC_ClearTarget(this, color);
    return true;
}

void SEnvTexture::Release()
{
    ReleaseDeviceObjects();
    SAFE_DELETE(m_pTex);
}

void SEnvTexture::RT_SetMatrix()
{
    Matrix44A matView, matProj;
    gRenDev->GetModelViewMatrix(matView.GetData());
    gRenDev->GetProjectionMatrix(matProj.GetData());

    float fWidth = m_pTex ? (float)m_pTex->GetWidth() : 1;
    float fHeight = m_pTex ? (float)m_pTex->GetHeight() : 1;

    Matrix44A matScaleBias(0.5f, 0, 0,   0,
        0, -0.5f,    0,   0,
        0,     0, 0.5f,   0,
        // texel alignment - also push up y axis reflection up a bit
        0.5f + 0.5f / fWidth, 0.5f + 1.0f / fHeight, 0.5f, 1.0f);

    Matrix44A m = matProj * matScaleBias;
    Matrix44A mm = matView * m;
    m_Matrix = mm;
}

void SEnvTexture::ReleaseDeviceObjects()
{
    //if (m_pTex)
    //  m_pTex->ReleaseDynamicRT(true);
}

#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
bool CTexture::RenderToTexture(int handle, const CCamera& camera, AzRTT::RenderContextId contextId)
{
    if (!CRenderer::CV_r_RTT)
    {
        return false;
    }

    CTexture* pTex = CTexture::GetByID(handle);
    if (!pTex || !pTex->GetDevTexture())
    {
        iLog->Log("Failed to render texture.  Invalid texture handle ID.");
        return false;
    }

    // a context may be invalid because it requires hardware resources that are not available
    bool contextIsValid = false;
    AzRTT::RTTRequestBus::BroadcastResult(contextIsValid, &AzRTT::RTTRequestBus::Events::ContextIsValid, contextId);
    if (!contextIsValid)
    {
        return false;
    }

    const int width = pTex->GetWidth();
    const int height = pTex->GetHeight();

    // NOTE: the renderer's camera comes from the thread info double buffer, so it is possible
    // GetCamera() will just return the camera using in the last render to texture pass.  
    // System::GetViewCamera() will have the camera used for the main rendering view
    CCamera prevSysCamera = gEnv->pSystem->GetViewCamera();

    // get the current viewport and renderer settings to restore after rendering to texture
    int vX, vY, vWidth, vHeight;
    gRenDev->GetViewport(&vX, &vY, &vWidth, &vHeight);

    // this flag is used by the engine to denote we are rendering the whole scene to texture
    gcpRendD3D->m_RP.m_TI[gcpRendD3D->m_RP.m_nFillThreadID].m_PersFlags |= RBPF_RENDER_SCENE_TO_TEXTURE;

    // this resets the view and frame view/proj matrices in the thread info
    gcpRendD3D->BeginFrame();

    // this frees up previous frame render cameras and waits for jobs.  It will trigger MainThreadRenderRequestBus::ExecuteQueuedEvents();
    // The main pass also calls this after BeginFrame and before RenderWorld
    gEnv->p3DEngine->Tick();

    // set the active camera 
    gRenDev->SetCamera(camera);

    // set the system view camera 
    CCamera newSystemCamera = camera;
    gEnv->pSystem->SetViewCamera(newSystemCamera);

    // we do not call PreWorldStreamUpdate here because it will compare the distance of the rtt 
    // camera to the main camera and negatively affect stream settings

    gRenDev->m_pRT->EnqueueRenderCommand([=]()
    {
        // disable back buffer swap so the renderer doesn't call present
        gRenDev->EnableSwapBuffers(false);

        CTexture* pTex = CTexture::GetByID(handle);

        // when you set the render target SetViewport is also called with the size of the target
        gRenDev->RT_PushRenderTarget(0, pTex, nullptr, -1);

        // TODO manually set the viewport with our own viewport ID if we enabled TAA.  Currently
        // doesn't work in multi-threaded mode. Causes flickering vegetation/transparent shadows

        // disabling temporal effects turns off auto exposure and reduces flicker.
        gRenDev->m_nDisableTemporalEffects = 1;
    });

    bool contextIsActive = false;
    AzRTT::RTTRequestBus::BroadcastResult(contextIsActive, &AzRTT::RTTRequestBus::Events::SetActiveContext, contextId);
    AZ_Warning("RenderToTexture", contextIsActive, "Failed to activate render to texture context, the render target will not be updated");

    if (contextIsActive)
    {
        // don't draw UI or console to this RTT 
        gEnv->pSystem->SetConsoleDrawEnabled(false);
        gEnv->pSystem->SetUIDrawEnabled(false);

        AzRTT::RenderContextConfig config;
        AzRTT::RTTRequestBus::BroadcastResult(config, &AzRTT::RTTRequestBus::Events::GetContextConfig, contextId);

        uint32_t renderPassFlags = SRenderingPassInfo::DEFAULT_FLAGS | SRenderingPassInfo::RENDER_SCENE_TO_TEXTURE;

        // render to texture does not support merged meshes yet
        renderPassFlags &= ~SRenderingPassInfo::MERGED_MESHES;

        // do not allow SVO (SHDF_ALLOW_AO) for now   
        int renderFlags = SHDF_ZPASS | SHDF_ALLOWHDR | SHDF_ALLOWPOSTPROCESS | SHDF_ALLOW_WATER;

        if (!config.m_shadowsEnabled)
        {
            renderFlags |= SHDF_NO_SHADOWGEN;
            renderPassFlags &= ~SRenderingPassInfo::SHADOWS;
        }

        if (!config.m_oceanEnabled)
        {
            renderPassFlags &= ~SRenderingPassInfo::WATEROCEAN;
        }

        SRenderingPassInfo renderPassInfo = SRenderingPassInfo::CreateGeneralPassRenderingInfo(camera, renderPassFlags);
        gEnv->p3DEngine->RenderWorld(renderFlags, renderPassInfo, __FUNCTION__);

        gEnv->pSystem->SetConsoleDrawEnabled(true);
        gEnv->pSystem->SetUIDrawEnabled(true);

        gEnv->p3DEngine->EndOcclusion();

        gEnv->p3DEngine->WorldStreamUpdate();

        // NOTE: This is how you could copy from the hdr target to the render target if you wanted no post processing (should probably remove SHDF_ALLOWPOSTPROCESS above)
        //gRenDev->m_pRT->EnqueueRenderCommand([=]()
        //{
            //PostProcessUtils().StretchRect(CTexture::s_ptexHDRTarget, pTex, false, false, false, false, SPostEffectsUtils::eDepthDownsample_None, false, &gcpRendD3D->m_FullResRect);
        //});

        // this ends up calling FX_FinalComposite which will use our render target for post effects  
        gcpRendD3D->SwitchToNativeResolutionBackbuffer();
    }

    // Pop our render target
    gcpRendD3D->SetRenderTarget(0);

    gRenDev->m_pRT->EnqueueRenderCommand([=]()
    {
        gcpRendD3D->m_RP.m_TI[gcpRendD3D->m_RP.m_nProcessThreadID].m_PersFlags &= ~RBPF_RENDER_SCENE_TO_TEXTURE;

        gcpRendD3D->SetViewport(vX, vY, vWidth, vHeight);

        // Reset the camera on the render thread or the main thread can get our
        // camera info after syncing with main.
        gcpRendD3D->SetCamera(prevSysCamera);
    });

    AzRTT::RTTRequestBus::Broadcast(&AzRTT::RTTRequestBus::Events::SetActiveContext, AzRTT::RenderContextId::CreateNull());

    // Free all unused render meshes.  Without this you can get lots of fun memory leaks.
    gRenDev->ForceGC();  

    // call endframe on the renderer instead of via d3d to bypass drawing messages
    // set wait to true otherwise EndFrame won't be sent if there is no pending flush condition
    bool wait = true;
    gRenDev->m_pRT->RC_EndFrame(wait);

    // re-enable swap buffers after calling end frame so the main pass will call present()
    gRenDev->m_pRT->EnqueueRenderCommand([=]()
    {
        gRenDev->EnableSwapBuffers(true);
    });

    // normally we would need to remove the cull job producer using RemoveCullJobProducer
    // we don't need to because we use e_statobjbufferrendertask = 0

    // restore previous settings
    gEnv->pSystem->SetViewCamera(prevSysCamera);
    gRenDev->SetCamera(prevSysCamera);

    // this fixes streaming update sync errors when rendering pre frame
    gEnv->p3DEngine->SyncProcessStreamingUpdate();

    return true;
}
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED

bool CTexture::RenderEnvironmentCMHDR([[maybe_unused]] int size, [[maybe_unused]] Vec3& Pos, [[maybe_unused]] TArray<unsigned short>& vecData)
{
#if !defined(CONSOLE)

    iLog->Log("Start generating a cubemap...");

    vecData.SetUse(0);

    int vX, vY, vWidth, vHeight;
    gRenDev->GetViewport(&vX, &vY, &vWidth, &vHeight);

    const int nOldWidth = gRenDev->GetCurrentContextViewportWidth();
    const int nOldHeight = gRenDev->GetCurrentContextViewportHeight();
    bool    bFullScreen = (iConsole->GetCVar("r_Fullscreen")->GetIVal() != 0) && (!gEnv->IsEditor());
    gRenDev->ChangeViewport(0, 0, size, size);

    int nPFlags = gcpRendD3D->m_RP.m_TI[gcpRendD3D->m_RP.m_nProcessThreadID].m_PersFlags;

    CTexture* ptexGenEnvironmentCM = CTexture::Create2DTexture("$GenEnvironmentCM", size, size, 0, FT_DONT_STREAM, 0, eTF_R16G16B16A16F, eTF_R16G16B16A16F);
    if (!ptexGenEnvironmentCM || !ptexGenEnvironmentCM->GetDevTexture())
    {
        iLog->Log("Failed generating a cubemap: out of video memory");
        gRenDev->ChangeViewport(0, 0, nOldWidth, nOldHeight);

        SAFE_RELEASE(ptexGenEnvironmentCM);
        return false;
    }

    // Disable/set cvars that can affect cube map generation. This is thread unsafe (we assume editor will not run in mt mode), no other way around at this time
    //  - coverage buffer unreliable for multiple views
    //  - custom view distance ratios

    ICVar* pCoverageBufferCV = gEnv->pConsole->GetCVar("e_CoverageBuffer");
    const int32 nCoverageBuffer = pCoverageBufferCV ? pCoverageBufferCV->GetIVal() : 0;
    if (pCoverageBufferCV)
    {
        pCoverageBufferCV->Set(0);
    }

    ICVar* pStatObjBufferRenderTasksCV = gEnv->pConsole->GetCVar("e_StatObjBufferRenderTasks");
    const int32 nStatObjBufferRenderTasks = pStatObjBufferRenderTasksCV ? pStatObjBufferRenderTasksCV->GetIVal() : 0;
    if (pStatObjBufferRenderTasksCV)
    {
        pStatObjBufferRenderTasksCV->Set(0);
    }

    ICVar* pViewDistRatioCV = gEnv->pConsole->GetCVar("e_ViewDistRatio");
    const float fOldViewDistRatio = pViewDistRatioCV ? pViewDistRatioCV->GetFVal() : 1.f;
    if (pViewDistRatioCV)
    {
        pViewDistRatioCV->Set(10000.f);
    }

    ICVar* pViewDistRatioVegetationCV = gEnv->pConsole->GetCVar("e_ViewDistRatioVegetation");
    const float fOldViewDistRatioVegetation = pViewDistRatioVegetationCV ? pViewDistRatioVegetationCV->GetFVal() : 100.f;
    if (pViewDistRatioVegetationCV)
    {
        pViewDistRatioVegetationCV->Set(10000.f);
    }

    ICVar* pLodRatioCV = gEnv->pConsole->GetCVar("e_LodRatio");
    const float fOldLodRatio = pLodRatioCV ? pLodRatioCV->GetFVal() : 1.f;
    if (pLodRatioCV)
    {
        pLodRatioCV->Set(1000.f);
    }

    Vec3 oldSunDir, oldSunStr, oldSunRGB;
    float oldSkyKm, oldSkyKr, oldSkyG;
    if (CRenderer::CV_r_HideSunInCubemaps)
    {
        gEnv->p3DEngine->GetSkyLightParameters(oldSunDir, oldSunStr, oldSkyKm, oldSkyKr, oldSkyG, oldSunRGB);
        gEnv->p3DEngine->SetSkyLightParameters(oldSunDir, oldSunStr, oldSkyKm, oldSkyKr, 1.0f, oldSunRGB, true); // Hide sun disc
    }

    const int32 nFlaresCV = CRenderer::CV_r_flares;
    CRenderer::CV_r_flares = 0;

    ICVar* pSSDOHalfResCV = gEnv->pConsole->GetCVar("r_ssdoHalfRes");
    const int nOldSSDOHalfRes = pSSDOHalfResCV ? pSSDOHalfResCV->GetIVal() : 1;
    if (pSSDOHalfResCV)
    {
        pSSDOHalfResCV->Set(0);
    }

    ICVar* pDynamicGI = gEnv->pConsole->GetCVar("e_GI");
    const int oldDynamicGIValue = pDynamicGI ? pDynamicGI->GetIVal() : 1;
    if (pDynamicGI)
    {
        pDynamicGI->Set(0);
    }

    const int nDesktopWidth = gcpRendD3D->m_deskwidth;
    const int nDesktopHeight = gcpRendD3D->m_deskheight;
    gcpRendD3D->m_deskwidth = gcpRendD3D->m_deskheight = size;

    gcpRendD3D->EnableSwapBuffers(false);
    for (int nSide = 0; nSide < 6; nSide++)
    {
        gcpRendD3D->BeginFrame();
        gcpRendD3D->SetViewport(0, 0, size, size);

        gcpRendD3D->SetWidth(size);
        gcpRendD3D->SetHeight(size);

        gcpRendD3D->EF_ClearTargetsLater(FRT_CLEAR, Clr_Transparent);

        DrawSceneToCubeSide(Pos, size, nSide);

        // Transfer to sysmem
        D3D11_BOX srcBox;
        srcBox.left = 0;
        srcBox.right = size;
        srcBox.top = 0;
        srcBox.bottom = size;
        srcBox.front = 0;
        srcBox.back = 1;

        CDeviceTexture* pDevTextureSrc = CTexture::s_ptexHDRTarget->GetDevTexture();
        CDeviceTexture* pDevTextureDst = ptexGenEnvironmentCM->GetDevTexture();

        gcpRendD3D->GetDeviceContext().CopySubresourceRegion(pDevTextureDst->Get2DTexture(), 0, 0, 0, 0, pDevTextureSrc->Get2DTexture(), 0, &srcBox);

        CDeviceTexture* pDstDevTex = ptexGenEnvironmentCM->GetDevTexture();
        pDstDevTex->DownloadToStagingResource(0, [&](void* pData, [[maybe_unused]] uint32 rowPitch, [[maybe_unused]] uint32 slicePitch)
        {
            unsigned short* pTarg = (unsigned short*)pData;
            const uint32 nLineStride = CTexture::TextureDataSize(size, 1, 1, 1, 1, eTF_R16G16B16A16F) / sizeof(unsigned short);

            // Copy vertically flipped image
            for (uint32 nLine = 0; nLine < size; ++nLine)
            {
                vecData.Copy(&pTarg[((size - 1) - nLine) * nLineStride], nLineStride);
            }

            return true;
        });

        gcpRendD3D->EndFrame();
    }

    SAFE_RELEASE(ptexGenEnvironmentCM);

    // Restore previous states

    gcpRendD3D->m_deskwidth = nDesktopWidth;
    gcpRendD3D->m_deskheight = nDesktopHeight;
    gRenDev->ChangeViewport(0, 0, nOldWidth, nOldHeight);

    gcpRendD3D->EnableSwapBuffers(true);
    gcpRendD3D->SetWidth(vWidth);
    gcpRendD3D->SetHeight(vHeight);
    gcpRendD3D->RT_SetViewport(vX, vY, vWidth, vHeight);
    gcpRendD3D->m_RP.m_TI[gcpRendD3D->m_RP.m_nProcessThreadID].m_PersFlags = nPFlags;
    gcpRendD3D->ResetToDefault();

    if (pCoverageBufferCV)
    {
        pCoverageBufferCV->Set(nCoverageBuffer);
    }

    if (pStatObjBufferRenderTasksCV)
    {
        pStatObjBufferRenderTasksCV->Set(nStatObjBufferRenderTasks);
    }

    if (pViewDistRatioCV)
    {
        pViewDistRatioCV->Set(fOldViewDistRatio);
    }

    if (pViewDistRatioVegetationCV)
    {
        pViewDistRatioVegetationCV->Set(fOldViewDistRatioVegetation);
    }

    if (pLodRatioCV)
    {
        pLodRatioCV->Set(fOldLodRatio);
    }

    if (CRenderer::CV_r_HideSunInCubemaps)
    {
        gEnv->p3DEngine->SetSkyLightParameters(oldSunDir, oldSunStr, oldSkyKm, oldSkyKr, oldSkyG, oldSunRGB, true);
    }

    CRenderer::CV_r_flares = nFlaresCV;

    if (pSSDOHalfResCV)
    {
        pSSDOHalfResCV->Set(nOldSSDOHalfRes);
    }

    if (pDynamicGI)
    {
        pDynamicGI->Set(oldDynamicGIValue);
    }

    iLog->Log("Successfully finished generating a cubemap.  The cubemap is being compressed in the background and will update automatically when done.");
#endif

    return true;
}

//////////////////////////////////////////////////////////////////////////

void CTexture::DrawSceneToCubeSide(Vec3& Pos, int tex_size, int side)
{
    const float sCubeVector[6][7] =
    {
        { 1, 0, 0, 0, 0, 1, -90}, //posx
        {-1, 0, 0, 0, 0, 1, 90}, //negx
        { 0, 1, 0, 0, 0, -1,  0}, //posy
        { 0, -1, 0, 0, 0, 1,  0}, //negy
        { 0, 0, 1, 0, 1, 0,  0}, //posz
        { 0, 0, -1, 0, 1, 0,  0}, //negz
    };

    if (!iSystem)
    {
        return;
    }

    CRenderer* r = gRenDev;
    CCamera prevCamera = r->GetCamera();

    I3DEngine* eng = gEnv->p3DEngine;

    Vec3 vForward = Vec3(sCubeVector[side][0], sCubeVector[side][1], sCubeVector[side][2]);
    Vec3 vUp      = Vec3(sCubeVector[side][3], sCubeVector[side][4], sCubeVector[side][5]);

    Matrix33 matRot = Matrix33::CreateOrientation(vForward, vUp, DEG2RAD(sCubeVector[side][6]));
    Matrix34 mFinal = Matrix34(matRot, Pos);

    // Use current viewport camera's near/far to capture what is shown in the editor
    const CCamera& viewCamera = gEnv->pSystem->GetViewCamera();
    const float captureNear = viewCamera.GetNearPlane();
    const float captureFar = viewCamera.GetFarPlane();
    const float captureFOV = DEG2RAD(90.0f);

    CCamera captureCamera;
    captureCamera.SetMatrix(mFinal);
    captureCamera.SetFrustum(tex_size, tex_size, captureFOV, captureNear, captureFar);

#ifdef DO_RENDERLOG
    if (CRenderer::CV_r_log)
    {
        r->Logv(SRendItem::m_RecurseLevel[r->m_RP.m_nProcessThreadID], ".. DrawSceneToCubeSide .. (DrawCubeSide %d)\n", side);
    }
#endif

    eng->RenderWorld(SHDF_CUBEMAPGEN | SHDF_ALLOWPOSTPROCESS | SHDF_ALLOWHDR | SHDF_ZPASS | SHDF_NOASYNC | SHDF_STREAM_SYNC, SRenderingPassInfo::CreateGeneralPassRenderingInfo(captureCamera, (SRenderingPassInfo::DEFAULT_FLAGS | SRenderingPassInfo::CUBEMAP_GEN)), __FUNCTION__);

#ifdef DO_RENDERLOG
    if (CRenderer::CV_r_log)
    {
        r->Logv(SRendItem::m_RecurseLevel[r->m_RP.m_nProcessThreadID], ".. End DrawSceneToCubeSide .. (DrawCubeSide %d)\n", side);
    }
#endif

    r->SetCamera(prevCamera);
}

//////////////////////////////////////////////////////////////////////////

void CTexture::DrawCubeSide(Vec3& Pos, int tex_size, int side, float fMaxDist)
{
    const float sCubeVector[6][7] =
    {
        { 1, 0, 0, 0, 0, 1, -90}, //posx
        {-1, 0, 0, 0, 0, 1, 90}, //negx
        { 0, -1, 0, 0, 0, 1,  0}, //posy
        { 0, 1, 0, 0, 0, -1,  0}, //negy
        { 0, 0, 1, 0, 1, 0,  0}, //posz
        { 0, 0, -1, 0, 1, 0,  0}, //negz
    };

    if (!iSystem)
    {
        return;
    }

    CRenderer* r = gRenDev;
    CCamera prevCamera = r->GetCamera();
    CCamera tmpCamera = prevCamera;

    I3DEngine* eng = gEnv->p3DEngine;
    float fMinDist = 0.25f;

    Vec3 vForward = Vec3(sCubeVector[side][0], sCubeVector[side][1], sCubeVector[side][2]);
    Vec3 vUp      = Vec3(sCubeVector[side][3], sCubeVector[side][4], sCubeVector[side][5]);
    Matrix33 matRot = Matrix33::CreateOrientation(vForward, vUp, DEG2RAD(sCubeVector[side][6]));

    // magic orientation we use in engine
    Matrix33 matScale = Matrix33::CreateScale(Vec3(1, -1, 1));
    matRot = matScale * matRot;

    tmpCamera.SetMatrix(Matrix34(matRot, Pos));
    tmpCamera.SetFrustum(tex_size, tex_size, 90.0f * gf_PI / 180.0f, fMinDist, fMaxDist);
    int nPersFlags = r->m_RP.m_TI[r->m_RP.m_nProcessThreadID].m_PersFlags;
    int nPersFlags2 = r->m_RP.m_PersFlags2;
    r->m_RP.m_TI[r->m_RP.m_nProcessThreadID].m_PersFlags |= /*RBPF_MIRRORCAMERA | */ RBPF_MIRRORCULL | RBPF_DRAWTOTEXTURE | RBPF_ENCODE_HDR;
    int nOldZ = CRenderer::CV_r_usezpass;
    CRenderer::CV_r_usezpass = 0;

    r->RT_SetViewport(0, 0, tex_size, tex_size);

#ifdef DO_RENDERLOG
    if (CRenderer::CV_r_log)
    {
        r->Logv(SRendItem::m_RecurseLevel[r->m_RP.m_nProcessThreadID], ".. DrawLowDetail .. (DrawCubeSide %d)\n", side);
    }
#endif

    eng->RenderWorld(SHDF_ALLOWHDR | SHDF_NOASYNC | SHDF_STREAM_SYNC, SRenderingPassInfo::CreateGeneralPassRenderingInfo(tmpCamera), __FUNCTION__);

#ifdef DO_RENDERLOG
    if (CRenderer::CV_r_log)
    {
        r->Logv(SRendItem::m_RecurseLevel[r->m_RP.m_nProcessThreadID], ".. End DrawLowDetail .. (DrawCubeSide %d)\n", side);
    }
#endif

    r->m_RP.m_TI[r->m_RP.m_nProcessThreadID].m_PersFlags = nPersFlags;
    r->m_RP.m_PersFlags2 = nPersFlags2;
    r->SetCamera(prevCamera);
    CRenderer::CV_r_usezpass = nOldZ;
}

bool CTexture::GenerateMipMaps(bool bSetOrthoProj, bool bUseHW, bool bNormalMap)
{
    if (!(GetFlags() & FT_FORCE_MIPS)
        || bSetOrthoProj || !bUseHW || bNormalMap)  //todo: implement
    {
        return false;
    }

    PROFILE_LABEL_SCOPE("GENERATE_MIPS");
    PROFILE_SHADER_SCOPE;

    CDeviceTexture* pTex = GetDevTexture();
    if (!pTex)
    {
        return false;
    }

    D3D11_TEXTURE2D_DESC pDesc;
    pTex->Get2DTexture()->GetDesc(&pDesc);

    // all d3d11 devices support autogenmipmaps
    if (m_pRenderTargetData)
    {
        gcpRendD3D->GetDeviceContext().GenerateMips(m_pDeviceShaderResource);
    }

    return true;
}

void CTexture::DestroyZMaps()
{
    //SAFE_RELEASE(s_ptexZTarget);
}

void CTexture::GenerateZMaps()
{
    if (gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
    {
        // Custom Z-Target for GMEM render path should already be set
        assert(s_ptexZTarget == CTexture::s_ptexGmemStenLinDepth);
        return;
    }

    int nWidth = gcpRendD3D->m_MainViewport.nWidth; //m_d3dsdBackBuffer.Width;
    int nHeight = gcpRendD3D->m_MainViewport.nHeight; //m_d3dsdBackBuffer.Height;
    ETEX_Format eTFZ = CTexture::s_eTFZ;
    uint32 nFlags = FT_DONT_STREAM | FT_USAGE_RENDERTARGET | FT_DONT_RELEASE;
    if (CRenderer::CV_r_msaa)
    {
        nFlags |= FT_USAGE_MSAA;
    }
    if (!s_ptexZTarget)
    {
        s_ptexZTarget = CreateRenderTarget("$ZTarget", nWidth, nHeight, Clr_White, eTT_2D, nFlags, eTFZ);
        s_ptexFurZTarget = CreateRenderTarget("$FurZTarget", nWidth, nHeight, Clr_White, eTT_2D, nFlags, eTFZ);
    }
    else
    {
        s_ptexZTarget->m_nFlags = nFlags;
        s_ptexZTarget->m_nWidth = nWidth;
        s_ptexZTarget->m_nHeight = nHeight;
        s_ptexZTarget->CreateRenderTarget(eTFZ, Clr_White);

        s_ptexFurZTarget->m_nFlags = nFlags;
        s_ptexFurZTarget->m_nWidth = nWidth;
        s_ptexFurZTarget->m_nHeight = nHeight;
        s_ptexFurZTarget->CreateRenderTarget(eTFZ, Clr_White);
    }
}

void CTexture::DestroySceneMap()
{
    //SAFE_RELEASE(s_ptexSceneTarget);
}

void CTexture::GenerateSceneMap(ETEX_Format eTF)
{
    const int32 nWidth = gcpRendD3D->GetWidth();
    const int32 nHeight = gcpRendD3D->GetHeight();
    uint32 nFlags =  FT_DONT_STREAM | FT_USAGE_RENDERTARGET | FT_USAGE_UNORDERED_ACCESS;
    nFlags |= FT_USAGE_UNORDERED_ACCESS;

    if (!s_ptexSceneTarget)
    {
        s_ptexSceneTarget = CreateRenderTarget("$SceneTarget", nWidth, nHeight, Clr_Empty, eTT_2D, nFlags, eTF, TO_SCENE_TARGET);
    }
    else
    {
        s_ptexSceneTarget->m_nFlags = nFlags;
        s_ptexSceneTarget->m_nWidth = nWidth;
        s_ptexSceneTarget->m_nHeight = nHeight;
        s_ptexSceneTarget->CreateRenderTarget(eTF, Clr_Empty);
    }

    nFlags &= ~(FT_USAGE_MSAA | FT_USAGE_UNORDERED_ACCESS);

    // This RT used for all post processes passes and shadow mask (group 0) as well
    ETEX_Format backbufferFormat;

    static ICVar* DolbyCvar = gEnv->pConsole->GetCVar("r_HDRDolby");

    if (DolbyCvar->GetIVal() == 1)
    {
        backbufferFormat = eTF_R10G10B10A2;
    }
    else
    {
        backbufferFormat = eTF_R8G8B8A8;
    }

    // This RT used for all post processes passes and shadow mask (group 0) as well
    if (!CTexture::IsTextureExist(s_ptexBackBuffer))
    {
        s_ptexBackBuffer = CreateRenderTarget("$BackBuffer", nWidth, nHeight, Clr_Transparent, eTT_2D, nFlags, backbufferFormat, TO_BACKBUFFERMAP);
    }
    else
    {
        s_ptexBackBuffer->m_nFlags = nFlags;
        s_ptexBackBuffer->m_nWidth = nWidth;
        s_ptexBackBuffer->m_nHeight = nHeight;
        s_ptexBackBuffer->CreateRenderTarget(backbufferFormat, Clr_Transparent);
    }

    nFlags &= ~(FT_USAGE_MSAA | FT_USAGE_UNORDERED_ACCESS);

    // This RT can be used by the Render3DModelMgr if the buffer needs to be persistent
    if (CRenderer::CV_r_UsePersistentRTForModelHUD > 0)
    {
        if (!CTexture::IsTextureExist(s_ptexModelHudBuffer))
        {
            s_ptexModelHudBuffer = CreateRenderTarget("$ModelHUD", nWidth, nHeight, Clr_Transparent, eTT_2D, nFlags, eTF_R8G8B8A8, TO_BACKBUFFERMAP);
        }
        else
        {
            s_ptexModelHudBuffer->m_nFlags = nFlags;
            s_ptexModelHudBuffer->m_nWidth = nWidth;
            s_ptexModelHudBuffer->m_nHeight = nHeight;
            s_ptexModelHudBuffer->CreateRenderTarget(eTF_R8G8B8A8, Clr_Transparent);
        }
    }
}

void CTexture::GenerateCachedShadowMaps()
{
    StaticArray<int, MAX_GSM_LODS_NUM> nResolutions = gRenDev->GetCachedShadowsResolution();

    // parse shadow resolutions from cvar
    {
        int nCurPos = 0;
        int nCurRes = 0;

        string strResolutions = gEnv->pConsole->GetCVar("r_ShadowsCacheResolutions")->GetString();
        string strCurRes = strResolutions.Tokenize(" ,;-\t", nCurPos);

        if (!strCurRes.empty())
        {
            nResolutions.fill(0);

            while (!strCurRes.empty())
            {
                int nRes = atoi(strCurRes.c_str());
                nResolutions[nCurRes] = clamp_tpl(nRes, 0, 16384);

                strCurRes = strResolutions.Tokenize(" ,;-\t", nCurPos);
                ++nCurRes;
            }

            gRenDev->SetCachedShadowsResolution(nResolutions);
        }
    }

    const ETEX_Format texFormat = gEnv->pConsole->GetCVar("r_ShadowsCacheFormat")->GetIVal() == 0 ? eTF_D32F : eTF_D16;
    const int cachedShadowsStart = clamp_tpl(CRenderer::CV_r_ShadowsCache, 0, MAX_GSM_LODS_NUM - 1);

    int gsmCascadeCount = gEnv->pSystem->GetConfigSpec() == CONFIG_LOW_SPEC ? 4 : 5;
    if (ICVar* pGsmLodsVar = gEnv->pConsole->GetCVar("e_GsmLodsNum"))
    {
        gsmCascadeCount = pGsmLodsVar->GetIVal();
    }
    const int cachedCascadesCount = cachedShadowsStart > 0 ? clamp_tpl(gsmCascadeCount - cachedShadowsStart + 1, 0, MAX_GSM_LODS_NUM) : 0;

    for (int i = 0; i < MAX_GSM_LODS_NUM; ++i)
    {
        CTexture*& pTx = s_ptexCachedShadowMap[i];

        if (!pTx)
        {
            char szName[32];
            sprintf_s(szName, "CachedShadowMap_%d", i);

            uint32 flags = FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_DEPTHSTENCIL | FT_USE_HTILE;
        #if defined(AZ_RESTRICTED_PLATFORM)
            #define AZ_RESTRICTED_SECTION D3DTEXTURE_CPP_SECTION_13
            #include AZ_RESTRICTED_FILE(D3DTexture_cpp)
        #endif
            pTx = CTexture::CreateTextureObject(szName, nResolutions[i], nResolutions[i], 1, eTT_2D, flags, texFormat);
        }

        pTx->Invalidate(nResolutions[i], nResolutions[i], texFormat);

        // delete existing texture in case it's not needed anymore
        if (CTexture::IsTextureExist(pTx) && nResolutions[i] == 0)
        {
            pTx->ReleaseDeviceTexture(false);
        }

        // allocate texture directly for all cached cascades
        if (!CTexture::IsTextureExist(pTx) && nResolutions[i] > 0 && i < cachedCascadesCount)
        {
            CryLog("Allocating shadow map cache %d x %d: %.2f MB", nResolutions[i], nResolutions[i], sqr(nResolutions[i]) * CTexture::BytesPerBlock(texFormat) / (1024.f * 1024.f));
            pTx->CreateRenderTarget(texFormat, Clr_FarPlane);
        }
    }

    // height map AO
    if (CRenderer::CV_r_HeightMapAO)
    {
        const int nTexRes = (int)clamp_tpl(CRenderer::CV_r_HeightMapAOResolution, 0.f, 16384.f);

        if (!s_ptexHeightMapAODepth[0])
        {
            s_ptexHeightMapAODepth[0] = CTexture::CreateTextureObject("HeightMapAO_Depth_0", nTexRes, nTexRes, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_DEPTHSTENCIL | FT_USE_HTILE, eTF_D16);
            s_ptexHeightMapAODepth[1] = CTexture::CreateTextureObject("HeightMapAO_Depth_1", nTexRes, nTexRes, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET | FT_FORCE_MIPS, eTF_R16);
        }

        s_ptexHeightMapAODepth[0]->Invalidate(nTexRes, nTexRes, eTF_D16);
        s_ptexHeightMapAODepth[1]->Invalidate(nTexRes, nTexRes, eTF_R16);

        if (!CTexture::IsTextureExist(s_ptexHeightMapAODepth[0]) && nTexRes > 0)
        {
            s_ptexHeightMapAODepth[0]->CreateRenderTarget(eTF_D16, Clr_FarPlane);
            s_ptexHeightMapAODepth[1]->CreateRenderTarget(eTF_R16, Clr_FarPlane);
        }
    }

    if (ShadowFrustumMGPUCache* pShadowMGPUCache = gRenDev->GetShadowFrustumMGPUCache())
    {
        pShadowMGPUCache->nUpdateMaskRT = 0;
        pShadowMGPUCache->nUpdateMaskMT = 0;
    }
}

void CTexture::DestroyCachedShadowMaps()
{
    for (int i = 0; i < MAX_GSM_LODS_NUM; ++i)
    {
        SAFE_RELEASE_FORCE(s_ptexCachedShadowMap[i]);
    }

    SAFE_RELEASE_FORCE(s_ptexHeightMapAO[0]);
    SAFE_RELEASE_FORCE(s_ptexHeightMapAO[1]);
}

void CTexture::GenerateNearestShadowMap()
{
    const int texResolution = CRenderer::CV_r_ShadowsNearestMapResolution;
    const ETEX_Format texFormat = eTF_D32F;
    s_ptexNearestShadowMap = CTexture::CreateTextureObject("NearestShadowMap", texResolution, texResolution, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_DEPTHSTENCIL | FT_USE_HTILE, texFormat);
}

void CTexture::DestroyNearestShadowMap()
{
    if (s_ptexNearestShadowMap)
    {
        SAFE_RELEASE_FORCE(s_ptexNearestShadowMap);
    }
}

bool SDynTexture::RT_Update(int nNewWidth, int nNewHeight)
{
    AZ_Assert(gRenDev->m_pRT->IsRenderThread(), ("Error - Cannot call SDynTexture::RT_Update from any thread that is not the primary render thread!"));

    Unlink();

    assert(s_iNumTextureBytesCheckedOut + s_iNumTextureBytesCheckedIn == s_nMemoryOccupied);

    if (nNewWidth != m_nReqWidth || nNewHeight != m_nReqHeight)
    {
        if (m_pTexture)
        {
            ReleaseDynamicRT(false);
        }
        m_pTexture = NULL;

        m_nReqWidth = nNewWidth;
        m_nReqHeight = nNewHeight;

        AdjustRealSize();
    }

    if (!m_pTexture)
    {
        int nNeedSpace = CTexture::TextureDataSize(m_nWidth, m_nHeight, 1, 1, 1, m_eTF);
        if (m_eTT == eTT_Cube)
        {
            nNeedSpace *= 6;
        }
        SDynTexture* pTX = SDynTexture::s_Root.m_Prev;
        const uint32 maxDynamicTextureMemory = SDynTexture::s_CurDynTexMaxSize * 1024u * 1024u;
        if (nNeedSpace + s_nMemoryOccupied > maxDynamicTextureMemory)
        {
            // Commit any render target binds/unbinds in case they are still waiting to be set or unset in a shadow state
            gcpRendD3D->FX_SetActiveRenderTargets();

            m_pTexture = GetDynamicRT();
            if (!m_pTexture)
            {
                bool bFreed = FreeTextures(true, nNeedSpace);
                if (!bFreed)
                {
                    bFreed = FreeTextures(false, nNeedSpace);
                }

                if (!bFreed)
                {
                    pTX = SDynTexture::s_Root.m_Next;
                    int nFrame = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_nFrameUpdateID - 1;
                    while (nNeedSpace + s_nMemoryOccupied > maxDynamicTextureMemory)
                    {
                        if (pTX == &SDynTexture::s_Root)
                        {
                            static int nThrash;
                            if (nThrash != nFrame)
                            {
                                nThrash = nFrame;
                                iLog->Log("Error: Dynamic textures thrashing (try to increase texture pool size - r_DynTexMaxSize)...");
                            }
                            break;
                        }
                        SDynTexture* pNext = pTX->m_Next;
                        // We cannot unload locked texture or texture used in current frame
                        // Better to increase pool size temporarily
                        if (pTX->m_pTexture && !pTX->m_pTexture->IsActiveRenderTarget())
                        {
                            if (pTX->m_pTexture->m_nAccessFrameID < nFrame && pTX->m_pTexture->m_nUpdateFrameID < nFrame && !pTX->m_bLocked)
                            {
                                pTX->ReleaseDynamicRT(true);
                            }
                        }
                        pTX = pNext;
                    }
                }
            }
        }
    }
    if (!m_pTexture)
    {
        m_pTexture = CreateDynamicRT();
    }

    assert(s_iNumTextureBytesCheckedOut + s_iNumTextureBytesCheckedIn == s_nMemoryOccupied);

    if (m_pTexture)
    {
        Link();
        return true;
    }
    return false;
}

bool SDynTexture::RT_SetRT(int nRT, int nWidth, int nHeight, bool bPush, bool bScreenVP)
{
    Update(m_nWidth, m_nHeight);

    SDepthTexture* pDepthSurf = nWidth > 0 ? gcpRendD3D->FX_GetDepthSurface(nWidth, nHeight, false) : &gcpRendD3D->m_DepthBufferOrig;

    assert(m_pTexture);
    if (m_pTexture)
    {
        if (bPush)
        {
            return gcpRendD3D->FX_PushRenderTarget(nRT, m_pTexture, pDepthSurf, -1, bScreenVP);
        }
        else
        {
            return gcpRendD3D->FX_SetRenderTarget(nRT, m_pTexture, pDepthSurf, false, -1, bScreenVP);
        }
    }
    return false;
}

bool SDynTexture::SetRT(int nRT, bool bPush, SDepthTexture* pDepthSurf, bool bScreenVP)
{
    Update(m_nWidth, m_nHeight);

    assert(m_pTexture);
    if (m_pTexture)
    {
        if (bPush)
        {
            return gcpRendD3D->FX_PushRenderTarget(nRT, m_pTexture, pDepthSurf, -1, bScreenVP);
        }
        else
        {
            return gcpRendD3D->FX_SetRenderTarget(nRT, m_pTexture, pDepthSurf, false, -1, bScreenVP);
        }
    }
    return false;
}

bool SDynTexture::RestoreRT(int nRT, bool bPop)
{
    if (bPop)
    {
        return gcpRendD3D->FX_PopRenderTarget(nRT);
    }
    else
    {
        return gcpRendD3D->FX_RestoreRenderTarget(nRT);
    }
}

bool SDynTexture::ClearRT()
{
    gcpRendD3D->FX_ClearTarget(m_pTexture);
    return true;
}

bool SDynTexture2::ClearRT()
{
    gcpRendD3D->FX_ClearTarget(m_pTexture);
    return true;
}

bool SDynTexture2::SetRT(int nRT, bool bPush, SDepthTexture* pDepthSurf, [[maybe_unused]] bool bScreenVP)
{
    Update(m_nWidth, m_nHeight);

    assert(m_pTexture);
    if (m_pTexture)
    {
        bool bRes = false;
        if (bPush)
        {
            bRes = gcpRendD3D->FX_PushRenderTarget(nRT, m_pTexture, pDepthSurf);
        }
        else
        {
            bRes = gcpRendD3D->FX_SetRenderTarget(nRT, m_pTexture, pDepthSurf);
        }
        SetRectStates();
        gcpRendD3D->FX_Commit();
    }
    return false;
}

bool SDynTexture2::SetRectStates()
{
    assert(m_pTexture);
    gcpRendD3D->RT_SetViewport(m_nX, m_nY, m_nWidth, m_nHeight);
    gcpRendD3D->EF_Scissor(true, m_nX, m_nY, m_nWidth, m_nHeight);
    return true;
}

bool SDynTexture2::RestoreRT(int nRT, bool bPop)
{
    bool bRes = false;
    gcpRendD3D->EF_Scissor(false, m_nX, m_nY, m_nWidth, m_nHeight);
    if (bPop)
    {
        bRes = gcpRendD3D->FX_PopRenderTarget(nRT);
    }
    else
    {
        bRes = gcpRendD3D->FX_RestoreRenderTarget(nRT);
    }
    gcpRendD3D->FX_Commit();

    return bRes;
}

void _DrawText(ISystem* pSystem, int x, int y, const float fScale, const char* format, ...);

#if !defined(_RELEASE)
static int __cdecl RTCallback(const VOID* arg1, const VOID* arg2)
{
    CTexture* p1 = *(CTexture**)arg1;
    CTexture* p2 = *(CTexture**)arg2;

    // show big textures first
    int nSize1 = p1->GetDataSize();
    int nSize2 = p2->GetDataSize();
    if (nSize1 > nSize2)
    {
        return -1;
    }
    else if (nSize2 > nSize1)
    {
        return 1;
    }

    return strcmp(p1->GetName(), p2->GetName());
}
#endif

void CD3D9Renderer::DrawAllDynTextures([[maybe_unused]] const char* szFilter, [[maybe_unused]] const bool bLogNames, [[maybe_unused]] const bool bOnlyIfUsedThisFrame)
{
#ifndef _RELEASE
    SDynTexture2::TextureSet2Itor itor;
    char name[256]; //, nm[256];
    cry_strcpy(name, szFilter);
    azstrlwr(name, AZ_ARRAY_SIZE(name));
    TArray<CTexture*> UsedRT;
    int nMaxCount = CV_r_ShowDynTexturesMaxCount;

    float width = 800;
    float height = 600;
    float fArrDim = max(1.f, sqrtf((float)nMaxCount));
    float fPicDimX = width / fArrDim;
    float fPicDimY = height / fArrDim;
    float x = 0;
    float y = 0;

    TransformationMatrices backupSceneMatrices;

    Set2DMode(static_cast<uint32>(width), static_cast<uint32>(height), backupSceneMatrices);

    EF_SetColorOp(eCO_MODULATE, eCO_MODULATE, DEF_TEXARG0, DEF_TEXARG0);
    EF_SetSrgbWrite(false);
    if (name[0] == '*' && !name[1])
    {
        SResourceContainer* pRL = CBaseResource::GetResourcesForClass(CTexture::mfGetClassName());
        ResourcesMapItor it;
        for (it = pRL->m_RMap.begin(); it != pRL->m_RMap.end(); it++)
        {
            CTexture* tp = (CTexture*)it->second;
            if (tp && !tp->IsNoTexture())
            {
                if ((tp->GetFlags() & (FT_USAGE_RENDERTARGET | FT_USAGE_DYNAMIC)) && tp->GetDevTexture())
                {
                    UsedRT.AddElem(tp);
                }
            }
        }
    }
    else
    {
        SResourceContainer* pRL = CBaseResource::GetResourcesForClass(CTexture::mfGetClassName());
        ResourcesMapItor it;
        for (it = pRL->m_RMap.begin(); it != pRL->m_RMap.end(); it++)
        {
            CTexture* tp = (CTexture*)it->second;
            if (!tp || tp->IsNoTexture())
            {
                continue;
            }
            if ((tp->GetFlags() & (FT_USAGE_RENDERTARGET | FT_USAGE_DYNAMIC)) && tp->GetDevTexture())
            {
                char nameBuffer[128];
                cry_strcpy(nameBuffer, tp->GetName());
                azstrlwr(nameBuffer, AZ_ARRAY_SIZE(nameBuffer));
                if (CryStringUtils::MatchWildcard(nameBuffer, name))
                {
                    UsedRT.AddElem(tp);
                }
            }
        }
    }
    if (UsedRT.Num() > 1)
    {
        qsort(&UsedRT[0], UsedRT.Num(), sizeof(CTexture*), RTCallback);
    }
    fPicDimX = width / fArrDim;
    fPicDimY = height / fArrDim;
    x = 0;
    y = 0;
    for (uint32 i = 0; i < UsedRT.Num(); i++)
    {
        SetState(GS_NODEPTHTEST);
        CTexture* tp = UsedRT[i];
        int nSavedAccessFrameID = tp->m_nAccessFrameID;

        if (bOnlyIfUsedThisFrame)
        {
            if (tp->m_nUpdateFrameID < m_RP.m_TI[m_RP.m_nProcessThreadID].m_nFrameUpdateID - 2)
            {
                continue;
            }
        }

        if (tp->GetTextureType() == eTT_2D)
        {
            Draw2dImage(x, y, fPicDimX - 2, fPicDimY - 2, tp->GetID(), 0, 1, 1, 0, 0);
        }

        tp->m_nAccessFrameID = nSavedAccessFrameID;

        const char* pTexName = tp->GetName();
        char nameBuffer[128];
        memset(nameBuffer, 0, sizeof nameBuffer);
        for (int iStr = 0, jStr = 0; pTexName[iStr] && jStr < sizeof nameBuffer - 1; ++iStr)
        {
            if (pTexName[iStr] == '$')
            {
                nameBuffer[jStr] = '$';
                nameBuffer[jStr + 1] = '$';
                jStr += 2;
            }
            else
            {
                nameBuffer[jStr] = pTexName[iStr];
                ++jStr;
            }
        }
        nameBuffer[sizeof nameBuffer - 1] = 0;
        pTexName = nameBuffer;

        int32 nPosX = (int32)ScaleCoordX(x);
        int32 nPosY = (int32)ScaleCoordY(y);
        _DrawText(iSystem, nPosX, nPosY, 1.0f, "%8s", pTexName);
        _DrawText(iSystem, nPosX, nPosY += 10, 1.0f, "%d-%d", tp->m_nUpdateFrameID, tp->m_nAccessFrameID);
        _DrawText(iSystem, nPosX, nPosY += 10, 1.0f, "%dx%d", tp->GetWidth(), tp->GetHeight());

        if (bLogNames)
        {
            iLog->Log("Mem:%d  %dx%d  Type:%s  Format:%s (%s)",
                tp->GetDeviceDataSize(), tp->GetWidth(), tp->GetHeight(), tp->NameForTextureType(tp->GetTextureType()), tp->NameForTextureFormat(tp->GetDstFormat()), tp->GetName());
        }

        x += fPicDimX;
        if (x >= width - 10)
        {
            x = 0;
            y += fPicDimY;
        }
    }

    Unset2DMode(backupSceneMatrices);

    RT_RenderTextMessages();
#endif
}

void CTexture::ReleaseSystemTargets()
{
    CTexture::DestroyHDRMaps();
    CTexture::DestroySceneMap();
    CTexture::DestroyCachedShadowMaps();
    CTexture::DestroyNearestShadowMap();

    if (CDeferredShading::Instance().IsValid())
    {
        CDeferredShading::Instance().DestroyDeferredMaps();
    }

    PostProcessUtils().Release();

    SAFE_RELEASE_FORCE(s_ptexWaterOcean);
    SAFE_RELEASE_FORCE(s_ptexWaterVolumeTemp);
    SAFE_RELEASE_FORCE(s_ptexWaterRipplesDDN);

    SAFE_RELEASE_FORCE(s_ptexSceneNormalsMap);
    SAFE_RELEASE_FORCE(s_ptexSceneNormalsBent);
    SAFE_RELEASE_FORCE(s_ptexAOColorBleed);
    SAFE_RELEASE_FORCE(s_ptexSceneDiffuse);
    SAFE_RELEASE_FORCE(s_ptexSceneSpecular);
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DTEXTURE_CPP_SECTION_10
    #include AZ_RESTRICTED_FILE(D3DTexture_cpp)
#endif
    SAFE_RELEASE_FORCE(s_ptexSceneDiffuseAccMap);
    SAFE_RELEASE_FORCE(s_ptexSceneSpecularAccMap);
    SAFE_RELEASE_FORCE(s_ptexBackBuffer);
    SAFE_RELEASE_FORCE(s_ptexSceneTarget);
    SAFE_RELEASE_FORCE(s_ptexZTargetScaled);
    SAFE_RELEASE_FORCE(s_ptexZTargetScaled2);
    SAFE_RELEASE_FORCE(s_ptexAmbientLookup);
    SAFE_RELEASE_FORCE(s_ptexDepthBufferQuarter);

    gcpRendD3D->m_bSystemTargetsInit = 0;
}

void CTexture::ReleaseMiscTargets()
{
    if (gcpRendD3D->m_pColorGradingControllerD3D)
    {
        gcpRendD3D->m_pColorGradingControllerD3D->ReleaseTextures();
    }
}

void CTexture::CreateSystemTargets()
{
    if (!gcpRendD3D->m_bSystemTargetsInit)
    {
        

        gcpRendD3D->m_bSystemTargetsInit = 1;

        ETEX_Format eTF = (gcpRendD3D->m_RP.m_bUseHDR && gcpRendD3D->m_nHDRType == 1) ? eTF_R16G16B16A16F : eTF_R8G8B8A8;

        // Create HDR targets
        CTexture::GenerateHDRMaps();

        // Create scene targets
        CTexture::GenerateSceneMap(eTF);

        // Create ZTarget
        CTexture::GenerateZMaps();

        // Allocate cached shadow maps if required
        CTexture::GenerateCachedShadowMaps();

        // Allocate the nearest shadow map if required
        CTexture::GenerateNearestShadowMap();

        // Create deferred lighting targets
        if (CDeferredShading::Instance().IsValid())
        {
            CDeferredShading::Instance().CreateDeferredMaps();
        }

        if (CRenderer::CV_r_DeferredShadingTiled > 0)
        {
            gcpRendD3D->GetTiledShading().CreateResources();
        }

        gcpRendD3D->GetVolumetricFog().CreateResources();

        // Create post effects targets
        PostProcessUtils().Create();
    }
}

void CTexture::CopySliceChain(CDeviceTexture* const pDevTexture, int ownerMips, int nDstSlice, int nDstMip, CDeviceTexture* pSrcDevTex, int nSrcSlice, int nSrcMip, int nSrcMips, int nNumMips)
{
    assert(pSrcDevTex && pDevTexture);

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DTEXTURE_CPP_SECTION_11
    #include AZ_RESTRICTED_FILE(D3DTexture_cpp)
#endif

    D3DBaseTexture* pDstResource = pDevTexture->GetBaseTexture();
    D3DBaseTexture* pSrcResource = pSrcDevTex->GetBaseTexture();

#ifndef _RELEASE
    if (!pDstResource)
    {
        __debugbreak();
    }
    if (!pSrcResource)
    {
        __debugbreak();
    }
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DTEXTURE_CPP_SECTION_12
    #include AZ_RESTRICTED_FILE(D3DTexture_cpp)
#endif
    assert(nSrcMip >= 0 && nDstMip >= 0);
    for (int i = 0; i < nNumMips; ++i)
    {
        #if defined(DEVICE_SUPPORTS_D3D11_1)
            gcpRendD3D->GetDeviceContext().CopySubresourceRegion1(
                pDstResource,
                D3D11CalcSubresource(nDstMip + i, nDstSlice, ownerMips),
                0, 0, 0,
                pSrcResource,
                D3D11CalcSubresource(nSrcMip + i, nSrcSlice, nSrcMips),
                NULL,
                D3D11_COPY_NO_OVERWRITE);
        #else
            gcpRendD3D->GetDeviceContext().CopySubresourceRegion(
                pDstResource,
                D3D11CalcSubresource(nDstMip + i, nDstSlice, ownerMips),
                0, 0, 0,
                pSrcResource,
                D3D11CalcSubresource(nSrcMip + i, nSrcSlice, nSrcMips),
                NULL);
        #endif
    }
}
