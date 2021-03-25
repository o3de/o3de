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

#include "RenderDll_precompiled.h"
#include "D3DVolumetricFog.h"
#include "DriverD3D.h"
#include "D3DPostProcess.h"
#include "CREFogVolume.h"


#define ENABLE_VOLFOG_TEX_FORMAT_RGBA16F

#if defined(AZ_RESTRICTED_PLATFORM)
    #undef AZ_RESTRICTED_SECTION
    #define D3DVOLUMETRICFOG_CPP_SECTION_1 1
#endif

namespace vfInternal
{
    static const uint32 MaxNumTileLights = 255;

    static const uint32 MaxNumFogVolumes = 64;

    static const float ThresholdLengthGlobalProbe = 1.732f * (1000.0f * 0.5f);

    enum EVolumeVolumeTypes
    {
        tlVolumeSphere = 1,
        tlVolumeCone = 2,
        tlVolumeOBB = 3,
        tlVolumeSun = 4,
    };

    enum EVolumeLightTypes
    {
        tlTypeProbe = 1,
        tlTypeAmbientPoint = 2,
        tlTypeAmbientProjector = 3,
        tlTypeAmbientArea = 4,
        tlTypeRegularPoint = 5,
        tlTypeRegularProjector = 6,
        tlTypeRegularPointFace = 7,
        tlTypeRegularArea = 8,
        tlTypeSun = 9,
    };

    struct SVolumeLightCullInfo
    {
        uint32    volumeType;
        uint32    miscFlag;
        Vec2      depthBounds;
        Vec4      posRad;
        Vec4      volumeParams0;
        Vec4      volumeParams1;
        Vec4      volumeParams2;
    };  // 80 bytes

    struct SVolumeLightShadeInfo
    {
        uint32    lightType;
        uint32    resIndex;
        uint32    shadowMaskIndex;
        uint16    stencilID0;
        uint16    stencilID1;
        Vec4      posRad;
        Vec2      attenuationParams;
        Vec2      shadowParams;
        Vec4      color;
        Vec4      shadowChannelIndex;
        Matrix44  projectorMatrix;
        Matrix44  shadowMatrix;
    };  // 208 bytes

    struct SFogVolumeCullInfo
    {
        Vec4      posRad;
        Vec4      volumeParams0;
        Vec4      volumeParams1;
        Vec4      volumeParams2;
    };

    struct SFogVolumeInjectInfo
    {
        uint32      miscFlag;   // 1: volumeType, 8: stencilRef, 1: affectThisAreaOnly, 22: reserved
        Vec3            fogColor;

        float           globalDensity;
        Vec3            fogVolumePos;

        Vec3            heightFalloffBasePoint;
        float           invSoftEdgeLerp;

        Vec3            heightFallOffDirScaled;
        float           densityOffset;

        Vec4            rampParams;

        Vec3            windOffset;
        float           noiseElapsedTime;

        Vec3            noiseSpatialFrequency;
        float           noiseScale;

        Vec3            eyePosInOS;
        float           noiseOffset;

        Matrix34    worldToObjMatrix;
    };

    int GetVolumeTexDepth()
    {
        int d = CRenderer::CV_r_VolumetricFogTexDepth;
        d = (d < 4) ? 4 : d;
    #if defined(AZ_RESTRICTED_PLATFORM)
        #define AZ_RESTRICTED_SECTION D3DVOLUMETRICFOG_CPP_SECTION_1
        #include AZ_RESTRICTED_FILE(D3DVolumetricFog_cpp)
    #endif

    #if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
        #undef AZ_RESTRICTED_SECTION_IMPLEMENTED
    #else  
        d = (d > 255) ? 255 : d;// this limitation due to the limitation of CTexture::CreateTextureArray.
    #endif
        int f = d % 4;
        d = (f > 0) ? d - f : d;// depth should be the multiples of 4.
        return d;
    }

    int GetVolumeTexSize(int size, int scale)
    {
        scale = max(scale, 2);
        return (size / scale) + ((size % scale) > 0 ? 1 : 0);
    }

    AABB RotateAABB(const AABB& aabb, const Matrix33& mat)
    {
        Matrix33 matAbs;
        matAbs.m00 = fabs_tpl(mat.m00);
        matAbs.m01 = fabs_tpl(mat.m01);
        matAbs.m02 = fabs_tpl(mat.m02);
        matAbs.m10 = fabs_tpl(mat.m10);
        matAbs.m11 = fabs_tpl(mat.m11);
        matAbs.m12 = fabs_tpl(mat.m12);
        matAbs.m20 = fabs_tpl(mat.m20);
        matAbs.m21 = fabs_tpl(mat.m21);
        matAbs.m22 = fabs_tpl(mat.m22);

        Vec3 sz  = ((aabb.max - aabb.min) * 0.5f) * matAbs;
        Vec3 pos = ((aabb.max + aabb.min) * 0.5f) * mat;

        return AABB(pos - sz, pos + sz);
    }
}


CVolumetricFog::CVolumetricFog()
{
    STATIC_ASSERT(MaxFrameNum >= MAX_GPU_NUM, "MaxFrameNum must be more than or equal to MAX_GPU_NUM.");

    for (int i = 0; i < MaxFrameNum; ++i)
    {
        m_viewProj[i].SetIdentity();
    }

    m_InscatteringVolume = NULL;
    m_fogInscatteringVolume[0] = NULL;
    m_fogInscatteringVolume[1] = NULL;
    m_MaxDepth = NULL;
    m_MaxDepthTemp = NULL;
    m_ClipVolumeDSVArray = NULL;
    m_fogDensityVolume[0] = NULL;
    m_fogDensityVolume[1] = NULL;
    for (int i = 0; i < 3; ++i)
    {
        m_downscaledShadow[i] = NULL;
    }
    m_downscaledShadowTemp = NULL;
    m_globalEnvProveTex0 = NULL;
    m_globalEnvProveTex1 = NULL;

    m_nTexStateTriLinear = -1;
    m_nTexStateCompare = -1;
    m_nTexStatePoint = -1;
    m_Cleared = MaxFrameNum;
    m_Destroyed = MaxFrameNum;
    m_numTileLights = 0;
    m_numFogVolumes = 0;
    m_frameID = -1;
    m_tick = 0;
    m_ReverseDepthMode = 1;
    m_raymarchDistance = 0.0f;
    m_globalEnvProbeParam0 = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
    m_globalEnvProbeParam1 = Vec4(0.0f, 0.0f, 0.0f, 0.0f);

    for (int i = 0; i < RT_COMMAND_BUF_COUNT; ++i)
    {
        for (int j = 0; j < MAX_REND_RECURSION_LEVELS; ++j)
        {
            for (int k = 0; k < MaxNumFogVolumeType; ++k)
            {
                m_fogVolumeInfoArray[i][j][k].Reserve(vfInternal::MaxNumFogVolumes);
            }
        }
    }
}

void CVolumetricFog::CreateResources()
{
    static const ICVar* pCVarVolumetricFog = NULL;
    if (!pCVarVolumetricFog)
    {
        pCVarVolumetricFog = gEnv->pConsole->GetCVar("e_VolumetricFog");
    }

    if (!pCVarVolumetricFog || pCVarVolumetricFog->GetIVal() == 0)
    {
        return;
    }

    // downscaled shadow maps
    ETEX_Format shadowFormat = eTF_D32F;
    static const ICVar* pCVarShadowMaxTexRes = NULL;
    if (!pCVarShadowMaxTexRes)
    {
        pCVarShadowMaxTexRes = gEnv->pConsole->GetCVar("e_ShadowsMaxTexRes");
    }
    int widthShadowMap = 0;
    int widthShadowMapTemp = 0;
    if (pCVarShadowMaxTexRes)
    {
        widthShadowMapTemp = max(pCVarShadowMaxTexRes->GetIVal(), 32);//this restriction is same as it in CD3D9Renderer::PrepareShadowGenForFrustum.
        switch (CRenderer::CV_r_VolumetricFogDownscaledSunShadowRatio)
        {
        case 0:
            widthShadowMapTemp /= 4;
            widthShadowMap = widthShadowMapTemp;
            break;
        case 1:
            widthShadowMapTemp /= 4;
            widthShadowMap = widthShadowMapTemp / 2;
            break;
        case 2:
            widthShadowMapTemp /= 4;
            widthShadowMap = widthShadowMapTemp / 4;
            break;
        default:
            widthShadowMapTemp /= 4;
            widthShadowMap = widthShadowMapTemp / 4;
            break;
        }
    }

    // view frustum aligned volume texture
    int width = vfInternal::GetVolumeTexSize(gRenDev->GetWidth(), CRenderer::CV_r_VolumetricFogTexScale);
    int height = vfInternal::GetVolumeTexSize(gRenDev->GetHeight(), CRenderer::CV_r_VolumetricFogTexScale);
    int depth = vfInternal::GetVolumeTexDepth();
#ifdef ENABLE_VOLFOG_TEX_FORMAT_RGBA16F
    ETEX_Format fmt = eTF_R16G16B16A16F;
#else
    ETEX_Format fmt = eTF_R11G11B10F;
#endif
    ETEX_Format fmtDensityColor = eTF_R11G11B10F;
    ETEX_Format fmtDensity = eTF_R16F;


    bool validVolumeTexture = m_InscatteringVolume
        && (m_InscatteringVolume->GetWidth() == width)
        && (m_InscatteringVolume->GetHeight() == height)
        && (m_InscatteringVolume->GetDepth() == depth)
        && (m_InscatteringVolume->GetDstFormat() == fmt)
        && CTexture::s_ptexVolumetricFogDensityColor
        && (CTexture::s_ptexVolumetricFogDensityColor->GetDstFormat() == fmtDensityColor);

    bool validDownscaledShadowMaps = (CRenderer::CV_r_VolumetricFogDownscaledSunShadow == 0 && !m_downscaledShadow[0])
        || (CRenderer::CV_r_VolumetricFogDownscaledSunShadow != 0
            && m_downscaledShadow[0] && m_downscaledShadow[0]->GetWidth() == widthShadowMap
            && m_downscaledShadow[0]->GetDstFormat() == shadowFormat
            && ((CRenderer::CV_r_VolumetricFogDownscaledSunShadow == 1 && !m_downscaledShadow[2])
                || (CRenderer::CV_r_VolumetricFogDownscaledSunShadow != 1 && m_downscaledShadow[2])));

#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
    if(gcpRendD3D->IsRenderToTextureActive())
    {
        return;
    }
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED

    if (validVolumeTexture && validDownscaledShadowMaps)
    {
        return;
    }
    else
    {
        DestroyResources(false);
    }

    STexState ts0(FILTER_TRILINEAR, true);
    STexState ts1(FILTER_BILINEAR, true);
    STexState ts2(FILTER_POINT, true);
    ts1.SetComparisonFilter(true);
    m_nTexStateTriLinear = CTexture::GetTexState(ts0);
    m_nTexStateCompare = CTexture::GetTexState(ts1);
    m_nTexStatePoint =  CTexture::GetTexState(ts2);


    uint32 commonFlags = FT_NOMIPS | FT_DONT_RELEASE | FT_DONT_STREAM;

    assert(!m_InscatteringVolume);
    if (!m_InscatteringVolume)
    {
        const int32 w = width;
        const int32 h = height;
        const int32 d = depth;
        //ETEX_Format format = eTF_R8G8B8A8;
        //uint32 flags = FT_NOMIPS | FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_UNORDERED_ACCESS | FT_USAGE_UAV_RWTEXTURE;
        ETEX_Format format = fmt;
        uint32 flags = commonFlags | FT_USAGE_UNORDERED_ACCESS;
        m_InscatteringVolume = CTexture::CreateTextureObject("$InscatteringVolume", w, h, d, eTT_3D, flags, format);
        const byte* pData[6] = {NULL};
        m_InscatteringVolume->CreateDeviceTexture(pData);

        if (m_InscatteringVolume->GetFlags() & FT_FAILED)
        {
            CryFatalError("Couldn't allocate texture.");
        }
    }

    {
        uint32 flags = commonFlags | FT_USAGE_UNORDERED_ACCESS;

        assert(CTexture::s_ptexVolumetricFog);
        if (CTexture::s_ptexVolumetricFog)
        {
            const int32 w = width;
            const int32 h = height;
            const int32 d = depth;
            ETEX_Format format = eTF_R16G16B16A16F;
            if (!CTexture::s_ptexVolumetricFog->Create3DTexture(w, h, d, 1, flags, NULL, format, format))
            {
                CryFatalError("Couldn't allocate texture.");
            }
        }
    }

    {
        const char* tname[2] =
        {
            "$FogInscatteringVolume0",
            "$FogInscatteringVolume1",
        };

        for (int i = 0; i < 2; ++i)
        {
            assert(!m_fogInscatteringVolume[i]);
            if (!m_fogInscatteringVolume[i])
            {
                const int32 w = width;
                const int32 h = height;
                const int32 d = depth;
                ETEX_Format format = fmt;
                uint32 flags = commonFlags | FT_USAGE_UNORDERED_ACCESS;
                m_fogInscatteringVolume[i] = CTexture::CreateTextureObject(tname[i], w, h, d, eTT_3D, flags, format);
                const byte* pData[6] = {NULL};
                m_fogInscatteringVolume[i]->CreateDeviceTexture(pData);

                if (m_fogInscatteringVolume[i]->GetFlags() & FT_FAILED)
                {
                    CryFatalError("Couldn't allocate texture.");
                }
            }
        }
    }

    assert(!m_MaxDepth);
    if (!m_MaxDepth)
    {
        const int32 w = width;
        const int32 h = height;
        ETEX_Format format = eTF_R16F;
        uint32 flags = commonFlags | FT_USAGE_UNORDERED_ACCESS;
        m_MaxDepth = CTexture::CreateTextureObject("$MaxDepth", w, h, 1, eTT_2D, flags, format);
        const byte* pData[6] = {NULL};
        m_MaxDepth->CreateDeviceTexture(pData);

        if (m_MaxDepth->GetFlags() & FT_FAILED)
        {
            CryFatalError("Couldn't allocate texture.");
        }
    }

    assert(!m_MaxDepthTemp);
    if (!m_MaxDepthTemp)
    {
        const int32 w = width << 1;
        const int32 h = CTexture::s_ptexZTargetScaled->GetHeight() >> 1;
        ETEX_Format format = eTF_R16F;
        uint32 flags = commonFlags | FT_USAGE_UNORDERED_ACCESS;
        m_MaxDepthTemp = CTexture::CreateTextureObject("$MaxDepthTemp", w, h, 1, eTT_2D, flags, format);
        const byte* pData[6] = {NULL};
        m_MaxDepthTemp->CreateDeviceTexture(pData);

        if (m_MaxDepthTemp->GetFlags() & FT_FAILED)
        {
            CryFatalError("Couldn't allocate texture.");
        }
    }

    assert(!m_ClipVolumeDSVArray);
    if (!m_ClipVolumeDSVArray)
    {
        typedef D3DDepthSurface* DSVPTR;
        m_ClipVolumeDSVArray = new DSVPTR[depth];
        for (int i = 0; i < depth; ++i)
        {
            m_ClipVolumeDSVArray[i] = NULL;
        }
    }

    assert(CTexture::s_ptexVolumetricClipVolumeStencil);
    if (CTexture::s_ptexVolumetricClipVolumeStencil)
    {
        const int32 w = width;
        const int32 h = height;
        const int32 d = depth;
        ETEX_Format format = eTF_D24S8;
        uint32 flags = commonFlags | FT_USAGE_DEPTHSTENCIL | FT_USAGE_RENDERTARGET;
        CTexture* ptex = CTexture::CreateTextureArray("$ClipVolumeStencilVolume", eTT_2D, w, h, d, 1, flags, format);

        // texture name has to exactly match the name of CTexture::s_ptexVolumetricClipVolumeStencil.
        if (CTexture::s_ptexVolumetricClipVolumeStencil != ptex || CTexture::s_ptexVolumetricClipVolumeStencil->GetFlags() & FT_FAILED)
        {
            CryFatalError("Couldn't allocate volumetric clip volume stencil texture.");
        }

        // This is the code for working around that stencil readable shader resource view can't be created directly.
        D3DShaderResourceView* pSRV = CTexture::s_ptexVolumetricClipVolumeStencil->GetShaderResourceView();
        pSRV->Release();
        D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
        ZeroStruct(SRVDesc);
        SRVDesc.Format = DXGI_FORMAT_X24_TYPELESS_G8_UINT;
        D3DTexture* pID3DTexture = CTexture::s_ptexVolumetricClipVolumeStencil->GetDevTexture()->Get2DTexture();
        SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
        SRVDesc.Texture2DArray.FirstArraySlice = 0;
        SRVDesc.Texture2DArray.ArraySize = d;
        SRVDesc.Texture2DArray.MipLevels = 1;
        SRVDesc.Texture2DArray.MostDetailedMip = 0;
        HRESULT hr = gcpRendD3D->GetDevice().CreateShaderResourceView(pID3DTexture, &SRVDesc, &pSRV);
        assert(SUCCEEDED(hr));
        CTexture::s_ptexVolumetricClipVolumeStencil->SetShaderResourceView(pSRV);

        // Separate DSVs need to be created because of performance issue of drawing to single DSV array.
        ID3D11DepthStencilView* pDSV;
        D3D11_DEPTH_STENCIL_VIEW_DESC DsvDesc;
        ZeroStruct(DsvDesc);
        DsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        DsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
        DsvDesc.Texture2DArray.ArraySize = 1;
        DsvDesc.Texture2DArray.MipSlice = 0;
        for (int i = 0; i < d; ++i)
        {
            DsvDesc.Texture2DArray.FirstArraySlice = i;
            pDSV = NULL;
            hr = gcpRendD3D->GetDevice().CreateDepthStencilView(pID3DTexture, &DsvDesc, &pDSV);
            assert(SUCCEEDED(hr));
            m_ClipVolumeDSVArray[i] = pDSV;
        }
    }

    assert(CTexture::s_ptexVolumetricFogDensityColor);
    if (CTexture::s_ptexVolumetricFogDensityColor)
    {
        const int32 w = width;
        const int32 h = height;
        const int32 d = depth;
        ETEX_Format format = fmtDensityColor;
        uint32 flags = commonFlags | FT_USAGE_UNORDERED_ACCESS | FT_USAGE_RENDERTARGET;

        CTexture::s_ptexVolumetricFogDensityColor->Invalidate(w, h, format);

        if (!CTexture::s_ptexVolumetricFogDensityColor->Create3DTexture(w, h, d, 1, flags, NULL, format, format))
        {
            CryFatalError("Couldn't allocate texture.");
        }
    }

    assert(CTexture::s_ptexVolumetricFogDensity);
    if (CTexture::s_ptexVolumetricFogDensity)
    {
        const int32 w = width;
        const int32 h = height;
        const int32 d = depth;
        ETEX_Format format = fmtDensity;
        uint32 flags = commonFlags | FT_USAGE_UNORDERED_ACCESS | FT_USAGE_RENDERTARGET;

        CTexture::s_ptexVolumetricFogDensity->Invalidate(w, h, format);

        if (!CTexture::s_ptexVolumetricFogDensity->Create3DTexture(w, h, d, 1, flags, NULL, format, format))
        {
            CryFatalError("Couldn't allocate texture.");
        }
    }

#ifndef ENABLE_VOLFOG_TEX_FORMAT_RGBA16F
    {
        const char* tname[2] =
        {
            "$FogDensityVolume0",
            "$FogDensityVolume1",
        };

        for (int i = 0; i < 2; ++i)
        {
            assert(!m_fogDensityVolume[i]);
            if (!m_fogDensityVolume[i])
            {
                const int32 w = width;
                const int32 h = height;
                const int32 d = depth;
                ETEX_Format format = fmtDensity;
                uint32 flags = commonFlags | FT_USAGE_UNORDERED_ACCESS;
                m_fogDensityVolume[i] = CTexture::CreateTextureObject(tname[i], w, h, d, eTT_3D, flags, format);
                m_fogDensityVolume[i]->CreateRenderTarget(format);

                if (m_fogDensityVolume[i]->GetFlags() & FT_FAILED)
                {
                    CryFatalError("Couldn't allocate texture.");
                }
            }
        }
    }
#endif

    if (CRenderer::CV_r_VolumetricFogDownscaledSunShadow != 0)
    {
        const char* tname[3] =
        {
            "$DownscaledShadowMaps0",
            "$DownscaledShadowMaps1",
            "$DownscaledShadowMaps2",
        };

        int count = (CRenderer::CV_r_VolumetricFogDownscaledSunShadow == 1) ? 2 : 3;
        for (int i = 0; i < count; ++i)
        {
            assert(!m_downscaledShadow[i]);
            if (!m_downscaledShadow[i])
            {
                uint32 flags = commonFlags | FT_USAGE_DEPTHSTENCIL | FT_USAGE_RENDERTARGET;

                m_downscaledShadow[i] = CTexture::Create2DTexture(tname[i], widthShadowMap, widthShadowMap, 1, flags, NULL, shadowFormat, shadowFormat);
            }

            if (!m_downscaledShadow[i] || m_downscaledShadow[i]->GetFlags() & FT_FAILED)
            {
                CryFatalError("Couldn't allocate texture.");
            }
        }

        if (CRenderer::CV_r_VolumetricFogDownscaledSunShadowRatio != 0)
        {
            assert(!m_downscaledShadowTemp);
            if (!m_downscaledShadowTemp)
            {
                uint32 flags = commonFlags | FT_USAGE_DEPTHSTENCIL | FT_USAGE_RENDERTARGET;

                m_downscaledShadowTemp = CTexture::Create2DTexture("$DownscaledShadowMapsTemp", widthShadowMapTemp, widthShadowMapTemp, 1, flags, NULL, shadowFormat, shadowFormat);
            }

            if (!m_downscaledShadowTemp || m_downscaledShadowTemp->GetFlags() & FT_FAILED)
            {
                CryFatalError("Couldn't allocate texture.");
            }
        }
    }

    if (!m_lightCullInfoBuf.m_pBuffer)
    {
        DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
        uint32 stride = sizeof(vfInternal::SVolumeLightCullInfo);
        m_lightCullInfoBuf.Create(vfInternal::MaxNumTileLights, stride, format, DX11BUF_DYNAMIC | DX11BUF_STRUCTURED | DX11BUF_BIND_SRV, NULL);
    }

    if (!m_LightShadeInfoBuf.m_pBuffer)
    {
        DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
        uint32 stride = sizeof(vfInternal::SVolumeLightShadeInfo);
        m_LightShadeInfoBuf.Create(vfInternal::MaxNumTileLights, stride, format, DX11BUF_DYNAMIC | DX11BUF_STRUCTURED | DX11BUF_BIND_SRV, NULL);
    }

    if (!m_lightGridBuf.m_pBuffer)
    {
        const uint32 tileSizeX = 4;
        const uint32 tileSizeY = 4;
        const uint32 tileSizeZ = 4;
        const uint32 dispatchSizeX = (width / tileSizeX) + (width % tileSizeX > 0 ? 1 : 0);
        const uint32 dispatchSizeY = (height / tileSizeY) + (height % tileSizeY > 0 ? 1 : 0);
        const uint32 dispatchSizeZ = (depth / tileSizeZ) + (depth % tileSizeZ > 0 ? 1 : 0);
        DXGI_FORMAT format = DXGI_FORMAT_R8_UINT;
        uint32 stride = sizeof(char);
        m_lightGridBuf.Create(dispatchSizeX * dispatchSizeY * dispatchSizeZ * vfInternal::MaxNumTileLights, stride, format, DX11BUF_BIND_SRV | DX11BUF_BIND_UAV, NULL);
    }

    if (!m_lightCountBuf.m_pBuffer)
    {
        const uint32 tileSizeX = 4;
        const uint32 tileSizeY = 4;
        const uint32 tileSizeZ = 4;
        const uint32 dispatchSizeX = (width / tileSizeX) + (width % tileSizeX > 0 ? 1 : 0);
        const uint32 dispatchSizeY = (height / tileSizeY) + (height % tileSizeY > 0 ? 1 : 0);
        const uint32 dispatchSizeZ = (depth / tileSizeZ) + (depth % tileSizeZ > 0 ? 1 : 0);
        DXGI_FORMAT format = DXGI_FORMAT_R8_UINT;
        uint32 stride = sizeof(char);
        m_lightCountBuf.Create(dispatchSizeX * dispatchSizeY * dispatchSizeZ, stride, format, DX11BUF_BIND_SRV | DX11BUF_BIND_UAV, NULL);
    }

    if (!m_fogVolumeCullInfoBuf.m_pBuffer)
    {
        DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
        uint32 stride = sizeof(vfInternal::SFogVolumeCullInfo);
        m_fogVolumeCullInfoBuf.Create(vfInternal::MaxNumFogVolumes, stride, format, DX11BUF_DYNAMIC | DX11BUF_STRUCTURED | DX11BUF_BIND_SRV, NULL);
    }

    if (!m_fogVolumeInjectInfoBuf.m_pBuffer)
    {
        DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
        uint32 stride = sizeof(vfInternal::SFogVolumeInjectInfo);
        m_fogVolumeInjectInfoBuf.Create(vfInternal::MaxNumFogVolumes, stride, format, DX11BUF_DYNAMIC | DX11BUF_STRUCTURED | DX11BUF_BIND_SRV, NULL);
    }
}

void CVolumetricFog::DestroyResources(bool destroyResolutionIndependentResources)
{
    assert((m_ClipVolumeDSVArray && m_InscatteringVolume) || !(m_ClipVolumeDSVArray || m_InscatteringVolume));
    if (m_ClipVolumeDSVArray && m_InscatteringVolume)
    {
        int depth = m_InscatteringVolume->GetDepth();
        for (int i = 0; i < depth; ++i)
        {
            if (m_ClipVolumeDSVArray[i])
            {
                ((ID3D11DepthStencilView*)m_ClipVolumeDSVArray[i])->Release();
            }
        }
        delete [] m_ClipVolumeDSVArray;
        m_ClipVolumeDSVArray = NULL;
    }

    SAFE_RELEASE_FORCE(m_InscatteringVolume);
    SAFE_RELEASE_FORCE(m_fogInscatteringVolume[0]);
    SAFE_RELEASE_FORCE(m_fogInscatteringVolume[1]);
    SAFE_RELEASE_FORCE(m_MaxDepth);
    SAFE_RELEASE_FORCE(m_MaxDepthTemp);
    SAFE_RELEASE_FORCE(m_fogDensityVolume[0]);
    SAFE_RELEASE_FORCE(m_fogDensityVolume[1]);
    for (int i = 0; i < 3; ++i)
    {
        SAFE_RELEASE_FORCE(m_downscaledShadow[i]);
    }
    SAFE_RELEASE_FORCE(m_downscaledShadowTemp);

    m_lightCullInfoBuf.Release();
    m_LightShadeInfoBuf.Release();
    m_lightGridBuf.Release();
    m_lightCountBuf.Release();

    for (int i = 0; i < RT_COMMAND_BUF_COUNT; ++i)
    {
        for (int j = 0; j < MAX_REND_RECURSION_LEVELS; ++j)
        {
            for (int k = 0; k < MaxNumFogVolumeType; ++k)
            {
                m_fogVolumeInfoArray[i][j][k].clear();
            }
        }
    }

    m_fogVolumeCullInfoBuf.Release();
    m_fogVolumeInjectInfoBuf.Release();

    if (destroyResolutionIndependentResources)
    {
        if (CTexture::s_ptexVolumetricFog)
        {
            CTexture::s_ptexVolumetricFog->ReleaseDeviceTexture(false);
        }
        if (CTexture::s_ptexVolumetricClipVolumeStencil)
        {
            CTexture::s_ptexVolumetricClipVolumeStencil->ReleaseDeviceTexture(false);
        }
        if (CTexture::s_ptexVolumetricFogDensityColor)
        {
            CTexture::s_ptexVolumetricFogDensityColor->ReleaseDeviceTexture(false);
        }
        if (CTexture::s_ptexVolumetricFogDensity)
        {
            CTexture::s_ptexVolumetricFogDensity->ReleaseDeviceTexture(false);
        }
    }

    m_Cleared = MaxFrameNum;
    m_Destroyed = MaxFrameNum;
    m_ReverseDepthMode = CRenderer::CV_r_ReverseDepth;
}

void CVolumetricFog::Clear()
{
    ClearFogVolumes();
}

void CVolumetricFog::ClearAll()
{
    Clear();

    m_globalEnvProbeParam0 = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
    m_globalEnvProbeParam1 = Vec4(0.0f, 0.0f, 0.0f, 0.0f);

    // ClearAll() is called during shutdown and post level unload 
    // when we should not be rendering volumetric fog.
    SAFE_RELEASE(m_globalEnvProveTex0);
    SAFE_RELEASE(m_globalEnvProveTex1);

    ClearAllFogVolumes();
    m_Cleared = MaxFrameNum;
}

void CVolumetricFog::PrepareLightList(TArray<SRenderLight>* envProbes, TArray<SRenderLight>* ambientLights, TArray<SRenderLight>* defLights, uint32 firstShadowLight, uint32 curShadowPoolLight)
{
    const ICVar* pCVarVolumetricFog = gEnv->pConsole->GetCVar("e_VolumetricFog");
    if (!pCVarVolumetricFog || pCVarVolumetricFog->GetIVal() == 0)
    {
        return;
    }

    if (!IsViable())
    {
        return;
    }

    AZ_TRACE_METHOD();
    CD3D9Renderer* const __restrict rd = gcpRendD3D;
    CTiledShading& tdsh = rd->GetTiledShading();

    // Prepare view matrix with flipped z-axis
    Matrix44A matView = rd->m_ViewMatrix;
    matView.m02 *= -1;
    matView.m12 *= -1;
    matView.m22 *= -1;
    matView.m32 *= -1;

    int nThreadID = rd->m_RP.m_nProcessThreadID;
    int nRecurseLevel = SRendItem::m_RecurseLevel[nThreadID];

    uint32 numTileLights = 0;
    uint32 numRenderLights = 0;
    uint32 numValidRenderLights = 0;
    CTexture* texGlobalEnvProbe0 = NULL;
    CTexture* texGlobalEnvProbe1 = NULL;
    Vec3 colorGlobalEnvProbe0(0.0f, 0.0f, 0.0f);
    Vec3 colorGlobalEnvProbe1(0.0f, 0.0f, 0.0f);
    float attenuationGlobalEnvProbe0 = 0.0f;
    float attenuationGlobalEnvProbe1 = 0.0f;
    float maxSizeGlobalEnvProbe0 = 0.0f;
    float maxSizeGlobalEnvProbe1 = 0.0f;

    vfInternal::SVolumeLightCullInfo tileLightsCull[vfInternal::MaxNumTileLights];
    vfInternal::SVolumeLightShadeInfo tileLightsShade[vfInternal::MaxNumTileLights];

    // Reset lights
    ZeroMemory(tileLightsCull, sizeof(vfInternal::SVolumeLightCullInfo) * vfInternal::MaxNumTileLights);
    ZeroMemory(tileLightsShade, sizeof(vfInternal::SVolumeLightShadeInfo) * vfInternal::MaxNumTileLights);

    TArray<SRenderLight>* lightLists[3] = {
        CRenderer::CV_r_DeferredShadingEnvProbes ? envProbes : NULL,
        CRenderer::CV_r_DeferredShadingAmbientLights ? ambientLights : NULL,
        CRenderer::CV_r_DeferredShadingLights ? defLights : NULL,
    };

    const bool areaLights = CRenderer::CV_r_DeferredShadingAreaLights > 0;  //convert from int to bool
    const float minBulbSize = max(0.001f, min(2.0f, CRenderer::CV_r_VolumetricFogMinimumLightBulbSize));// limit the minimum bulb size to reduce the light flicker.

    for (uint32 lightListIdx = 0; lightListIdx < 3; ++lightListIdx)
    {
        if (lightLists[lightListIdx] == NULL)
        {
            continue;
        }

        for (uint32 lightIdx = 0, lightListSize = lightLists[lightListIdx]->size(); lightIdx < lightListSize; ++lightIdx)
        {
            SRenderLight& renderLight = (*lightLists[lightListIdx])[lightIdx];
            vfInternal::SVolumeLightCullInfo& lightCullInfo = tileLightsCull[numTileLights];
            vfInternal::SVolumeLightShadeInfo& lightShadeInfo = tileLightsShade[numTileLights];

            if ((renderLight.m_Flags & DLF_FAKE) || !(renderLight.m_Flags & DLF_VOLUMETRIC_FOG))
            {
                continue;
            }

            // Skip non-ambient area light if support is disabled
            if ((renderLight.m_Flags & DLF_AREA_LIGHT) && !(renderLight.m_Flags & DLF_AMBIENT) && !areaLights)
            {
                continue;
            }

            ++numRenderLights;

            if (numTileLights == vfInternal::MaxNumTileLights)
            {
                continue;  // Skip light
            }
            // Setup standard parameters
            bool areaLightRect = (renderLight.m_Flags & DLF_AREA_LIGHT) && renderLight.m_fAreaWidth && renderLight.m_fAreaHeight && renderLight.m_fLightFrustumAngle;
            float volumeSize = (lightListIdx == 0) ? renderLight.m_ProbeExtents.len() : renderLight.m_fRadius;
            Vec3 pos = renderLight.GetPosition();
            Vec3 worldViewPos = rd->GetViewParameters().vOrigin;
            lightShadeInfo.posRad = Vec4(pos.x - worldViewPos.x, pos.y - worldViewPos.y, pos.z - worldViewPos.z, volumeSize);
            Vec4 posVS = Vec4(pos, 1) * matView;
            lightCullInfo.posRad = Vec4(posVS.x, posVS.y, posVS.z, volumeSize);
            lightShadeInfo.attenuationParams = Vec2(areaLightRect ? (renderLight.m_fAreaWidth + renderLight.m_fAreaHeight) * 0.25f : renderLight.m_fAttenuationBulbSize, renderLight.m_fAreaHeight * 0.5f);
            lightCullInfo.depthBounds = Vec2(posVS.z - volumeSize, posVS.z + volumeSize);
            lightShadeInfo.color = Vec4(renderLight.m_Color.r,
                    renderLight.m_Color.g,
                    renderLight.m_Color.b,
                    renderLight.m_fFogRadialLobe);
            lightShadeInfo.resIndex = 0;
            lightShadeInfo.shadowParams = Vec2(0, 0);
            lightShadeInfo.stencilID0 = renderLight.m_nStencilRef[0] + 1;
            lightShadeInfo.stencilID1 = renderLight.m_nStencilRef[1] + 1;

            // Environment probes
            if (lightListIdx == 0)
            {
                lightCullInfo.volumeType = vfInternal::tlVolumeOBB;
                lightShadeInfo.lightType = vfInternal::tlTypeProbe;
                lightShadeInfo.resIndex = 0xFFFFFFFF;
                lightShadeInfo.attenuationParams.x = renderLight.m_fProbeAttenuation;
                // assigning value isn't needed because AttenuationFalloffMax is hard-coded in VolumeLighting.cfi to mitigate sharp transition between probes.
                //lightShadeInfo.attenuationParams.y = max( renderLight.GetFalloffMax(), 0.001f );
                lightCullInfo.miscFlag = 0;

                AABB aabb = vfInternal::RotateAABB(AABB(-renderLight.m_ProbeExtents, renderLight.m_ProbeExtents), Matrix33(renderLight.m_ObjMatrix));
                aabb = vfInternal::RotateAABB(aabb, Matrix33(matView));
                lightCullInfo.depthBounds = Vec2(posVS.z + aabb.min.z, posVS.z + aabb.max.z);

                Vec4 u0 = Vec4(renderLight.m_ObjMatrix.GetColumn0().GetNormalized(), 0) * matView;
                Vec4 u1 = Vec4(renderLight.m_ObjMatrix.GetColumn1().GetNormalized(), 0) * matView;
                Vec4 u2 = Vec4(renderLight.m_ObjMatrix.GetColumn2().GetNormalized(), 0) * matView;
                lightCullInfo.volumeParams0 = Vec4(u0.x, u0.y, u0.z, renderLight.m_ProbeExtents.x);
                lightCullInfo.volumeParams1 = Vec4(u1.x, u1.y, u1.z, renderLight.m_ProbeExtents.y);
                lightCullInfo.volumeParams2 = Vec4(u2.x, u2.y, u2.z, renderLight.m_ProbeExtents.z);

                lightShadeInfo.projectorMatrix.SetRow4(0, Vec4(renderLight.m_ObjMatrix.GetColumn0().GetNormalized() / renderLight.m_ProbeExtents.x, 0));
                lightShadeInfo.projectorMatrix.SetRow4(1, Vec4(renderLight.m_ObjMatrix.GetColumn1().GetNormalized() / renderLight.m_ProbeExtents.y, 0));
                lightShadeInfo.projectorMatrix.SetRow4(2, Vec4(renderLight.m_ObjMatrix.GetColumn2().GetNormalized() / renderLight.m_ProbeExtents.z, 0));

                Vec3 boxProxyMin(-1000000, -1000000, -1000000);
                Vec3 boxProxyMax(1000000,  1000000,  1000000);

                if (renderLight.m_Flags & DLF_BOX_PROJECTED_CM)
                {
                    boxProxyMin = Vec3(-renderLight.m_fBoxLength * 0.5f, -renderLight.m_fBoxWidth * 0.5f, -renderLight.m_fBoxHeight * 0.5f);
                    boxProxyMax = Vec3(renderLight.m_fBoxLength * 0.5f,  renderLight.m_fBoxWidth * 0.5f,  renderLight.m_fBoxHeight * 0.5f);
                }

                lightShadeInfo.shadowMatrix.SetRow4(0, Vec4(boxProxyMin, 0));
                lightShadeInfo.shadowMatrix.SetRow4(1, Vec4(boxProxyMax, 0));

                int arrayIndex = tdsh.InsertTextureToSpecularProbeAtlas((CTexture*)renderLight.GetSpecularCubemap(), -1);
                if (arrayIndex >= 0)
                {
                    if (tdsh.InsertTextureToDiffuseProbeAtlas((CTexture*)renderLight.GetDiffuseCubemap(), arrayIndex) >= 0)
                    {
                        lightShadeInfo.resIndex = arrayIndex;
                    }
                    else
                    {
                        continue;  // Skip light
                    }
                }
                else
                {
                    continue;  // Skip light
                }
                // Determine the global env probe for lighting analytical volumetric fog if we have it in the scene.
                // Enough big(more than 1000 meters), the biggest and the second biggest including camera position are selected.
                if (volumeSize >= vfInternal::ThresholdLengthGlobalProbe &&
                    (volumeSize > maxSizeGlobalEnvProbe0 || volumeSize > maxSizeGlobalEnvProbe1))
                {
                    Vec3 vLightPos = renderLight.GetPosition();
                    Vec3 vCenterRel = worldViewPos - vLightPos;
                    if (vCenterRel.dot(vCenterRel) <= volumeSize * volumeSize)
                    {
                        Vec3 vCenterOBBSpace;
                        vCenterOBBSpace.x = renderLight.m_ObjMatrix.GetColumn0().GetNormalized().dot(vCenterRel);
                        vCenterOBBSpace.y = renderLight.m_ObjMatrix.GetColumn1().GetNormalized().dot(vCenterRel);
                        vCenterOBBSpace.z = renderLight.m_ObjMatrix.GetColumn2().GetNormalized().dot(vCenterRel);

                        // Check if camera position is within probe OBB
                        Vec3 vProbeExtents = renderLight.m_ProbeExtents;
                        if (fabs(vCenterOBBSpace.x) < vProbeExtents.x && fabs(vCenterOBBSpace.y) < vProbeExtents.y && fabs(vCenterOBBSpace.z) < vProbeExtents.z)
                        {
                            Vec3 probePos;
                            probePos.x = vCenterOBBSpace.x / vProbeExtents.x;
                            probePos.y = vCenterOBBSpace.y / vProbeExtents.y;
                            probePos.z = vCenterOBBSpace.z / vProbeExtents.z;
                            Vec3 pos2 = probePos.CompMul(probePos);
                            Vec3 t;
                            t.x = sqrt(1.0f - 0.5f * pos2.y - 0.5f * pos2.z + 0.333f * pos2.y * pos2.z);
                            t.y = sqrt(1.0f - 0.5f * pos2.z - 0.5f * pos2.x + 0.333f * pos2.z * pos2.x);
                            t.z = sqrt(1.0f - 0.5f * pos2.x - 0.5f * pos2.y + 0.333f * pos2.x * pos2.y);
                            probePos = probePos.CompMul(t);
                            float falloff = max(0.0f, min(1.0f, 1.0f + probePos.Dot(-probePos)));
                            const static float AttenuationFalloffMax = 0.2f;
                            falloff = min(1.0f, falloff / max(renderLight.GetFalloffMax(), AttenuationFalloffMax));

                            // This globalEnvProbe stuff has not been tested since the introduction of m_fProbeAttenuation
                            // because the feature is dead at this time; CVolumetricFog::Clear() gets called before m_globalEnvProbeParam0/1
                            // and m_globalEnvProveTex0/1 are copied to the GPU. I updated the code to use m_fProbeAttenuation as best I could,
                            // but can't be certain that the final result will be correct if/when globalEnvProbe functionality is restored. (9/21/2017)

                            float attenuation = falloff * falloff * (3.0f - 2.0f * falloff) * renderLight.m_fProbeAttenuation;

                            if (renderLight.m_fProbeAttenuation > 0)
                            {
                                if (volumeSize > maxSizeGlobalEnvProbe0)
                                {
                                    maxSizeGlobalEnvProbe1 = maxSizeGlobalEnvProbe0;
                                    texGlobalEnvProbe1 = texGlobalEnvProbe0;
                                    colorGlobalEnvProbe1 = colorGlobalEnvProbe0;
                                    attenuationGlobalEnvProbe1 = attenuationGlobalEnvProbe0;

                                    maxSizeGlobalEnvProbe0 = volumeSize;
                                    texGlobalEnvProbe0 = (CTexture*)renderLight.GetDiffuseCubemap();
                                    colorGlobalEnvProbe0 = Vec3(lightShadeInfo.color);
                                    attenuationGlobalEnvProbe0 = attenuation;
                                }
                                else if (volumeSize > maxSizeGlobalEnvProbe1)
                                {
                                    maxSizeGlobalEnvProbe1 = volumeSize;
                                    texGlobalEnvProbe1 = (CTexture*)renderLight.GetDiffuseCubemap();
                                    colorGlobalEnvProbe1 = Vec3(lightShadeInfo.color);
                                    attenuationGlobalEnvProbe1 = attenuation;
                                }
                            }
                        }
                    }
                }
            }
            // Regular and ambient lights
            else
            {
                const float sqrt_2 = 1.414214f;  // Scale for cone so that it's base encloses a pyramid base

                const bool ambientLight = (lightListIdx == 1);

                lightCullInfo.volumeType = vfInternal::tlVolumeSphere;
                lightShadeInfo.lightType = ambientLight ? vfInternal::tlTypeAmbientPoint : vfInternal::tlTypeRegularPoint;
                lightCullInfo.miscFlag = 0;

                if (!ambientLight)
                {
                    lightShadeInfo.attenuationParams.x = max(lightShadeInfo.attenuationParams.x, minBulbSize);

                    // Adjust light intensity so that the intended brightness is reached 1 meter from the light's surface
                    // Solve I * 1 / (1 + d/lightsize)^2 = 1
                    float intensityMul = 1.0f + 1.0f / lightShadeInfo.attenuationParams.x;
                    intensityMul *= intensityMul;
                    lightShadeInfo.color.x *= intensityMul;
                    lightShadeInfo.color.y *= intensityMul;
                    lightShadeInfo.color.z *= intensityMul;
                }

                // Handle projectors
                if (renderLight.m_Flags & DLF_PROJECT)
                {
                    lightCullInfo.volumeType = vfInternal::tlVolumeCone;
                    lightShadeInfo.lightType = ambientLight ? vfInternal::tlTypeAmbientProjector : vfInternal::tlTypeRegularProjector;
                    lightShadeInfo.resIndex = 0xFFFFFFFF;
                    lightCullInfo.miscFlag = 0;

                    int arrayIndex = tdsh.InsertTextureToSpotTexAtlas((CTexture*)renderLight.m_pLightImage, -1);
                    if (arrayIndex >= 0)
                    {
                        lightShadeInfo.resIndex = (uint32)arrayIndex;
                    }
                    else
                    {
                        continue;  // Skip light
                    }
                    // Prevent culling errors for frustums with large FOVs by slightly enlarging the frustum
                    const float frustumAngleDelta = renderLight.m_fLightFrustumAngle > 50 ? 7.5f : 0.0f;

                    Matrix34 objMat = renderLight.m_ObjMatrix;
                    objMat.m03 = objMat.m13 = objMat.m23 = 0;  // Remove translation
                    Vec3 lightDir = objMat * Vec3(-1, 0, 0);
                    lightCullInfo.volumeParams0 = Vec4(lightDir.x, lightDir.y, lightDir.z, 0) * matView;
                    lightCullInfo.volumeParams0.w = renderLight.m_fRadius * tanf(DEG2RAD(min(renderLight.m_fLightFrustumAngle + frustumAngleDelta, 89.9f))) * sqrt_2;

                    Vec3 coneTip = Vec3(lightCullInfo.posRad.x, lightCullInfo.posRad.y, lightCullInfo.posRad.z);
                    Vec3 coneDir = Vec3(-lightCullInfo.volumeParams0.x, -lightCullInfo.volumeParams0.y, -lightCullInfo.volumeParams0.z);
                    AABB coneBounds = AABB::CreateAABBfromCone(Cone(coneTip, coneDir, renderLight.m_fRadius, lightCullInfo.volumeParams0.w));
                    lightCullInfo.depthBounds = Vec2(coneBounds.min.z, coneBounds.max.z);

                    Matrix44A projMatT;
                    CShadowUtils::GetProjectiveTexGen(&renderLight, 0, &projMatT);

                    // Translate into camera space
                    projMatT.Transpose();
                    const Vec4 vEye(rd->GetViewParameters().vOrigin, 0.0f);
                    Vec4 vecTranslation(vEye.Dot((Vec4&)projMatT.m00), vEye.Dot((Vec4&)projMatT.m10), vEye.Dot((Vec4&)projMatT.m20), vEye.Dot((Vec4&)projMatT.m30));
                    projMatT.m03 += vecTranslation.x;
                    projMatT.m13 += vecTranslation.y;
                    projMatT.m23 += vecTranslation.z;
                    projMatT.m33 += vecTranslation.w;

                    lightShadeInfo.projectorMatrix = projMatT;
                }

                // Handle rectangular area lights
                if (areaLightRect)
                {
                    lightCullInfo.volumeType = vfInternal::tlVolumeOBB;
                    lightShadeInfo.lightType = ambientLight ? vfInternal::tlTypeAmbientArea : vfInternal::tlTypeRegularArea;
                    lightCullInfo.miscFlag = 0;

                    float expensionRadius = renderLight.m_fRadius * 1.08f;
                    Vec3 scale(expensionRadius, expensionRadius, expensionRadius);
                    Matrix34 areaLightMat = CShadowUtils::GetAreaLightMatrix(&renderLight, scale);

                    Vec4 u0 = Vec4(areaLightMat.GetColumn0().GetNormalized(), 0) * matView;
                    Vec4 u1 = Vec4(areaLightMat.GetColumn1().GetNormalized(), 0) * matView;
                    Vec4 u2 = Vec4(areaLightMat.GetColumn2().GetNormalized(), 0) * matView;
                    lightCullInfo.volumeParams0 = Vec4(u0.x, u0.y, u0.z, areaLightMat.GetColumn0().GetLength() * 0.5f);
                    lightCullInfo.volumeParams1 = Vec4(u1.x, u1.y, u1.z, areaLightMat.GetColumn1().GetLength() * 0.5f);
                    lightCullInfo.volumeParams2 = Vec4(u2.x, u2.y, u2.z, areaLightMat.GetColumn2().GetLength() * 0.5f);

                    float volumeExtent = renderLight.m_fRadius + max(renderLight.m_fAreaWidth, renderLight.m_fAreaHeight);
                    lightCullInfo.depthBounds = Vec2(posVS.z - volumeExtent, posVS.z + volumeExtent);

                    float areaFov = renderLight.m_fLightFrustumAngle * 2.0f;
                    if (renderLight.m_Flags & DLF_CASTSHADOW_MAPS)
                    {
                        areaFov = min(areaFov, 135.0f); // Shadow can only cover ~135 degree FOV without looking bad, so we clamp the FOV to hide shadow clipping
                    }
                    const float cosAngle = cosf(areaFov * (gf_PI / 360.0f));

                    Matrix44 areaLightParams;
                    areaLightParams.SetRow4(0, Vec4(renderLight.m_ObjMatrix.GetColumn0().GetNormalized(), 1.0f));
                    areaLightParams.SetRow4(1, Vec4(renderLight.m_ObjMatrix.GetColumn1().GetNormalized(), 1.0f));
                    areaLightParams.SetRow4(2, Vec4(renderLight.m_ObjMatrix.GetColumn2().GetNormalized(), 1.0f));
                    areaLightParams.SetRow4(3, Vec4(renderLight.m_fAreaWidth * 0.5f, renderLight.m_fAreaHeight * 0.5f, 0, cosAngle));

                    lightShadeInfo.projectorMatrix = areaLightParams;
                }

                // Handle shadow casters
                if (!ambientLight && lightIdx >= firstShadowLight && lightIdx < curShadowPoolLight)
                {
                    int numDLights = rd->m_RP.m_DLights[nThreadID][nRecurseLevel].size();
                    int frustumIdx = lightIdx + numDLights;
                    int startIdx = SRendItem::m_StartFrust[nThreadID][frustumIdx];
                    int endIdx = SRendItem::m_EndFrust[nThreadID][frustumIdx];

                    if (endIdx > startIdx && nRecurseLevel < (int)rd->m_RP.m_SMFrustums[nThreadID]->size())
                    {
                        ShadowMapFrustum& firstFrustum = rd->m_RP.m_SMFrustums[nThreadID][nRecurseLevel][startIdx];
                        assert (firstFrustum.bUseShadowsPool);

                        int numSides = firstFrustum.bOmniDirectionalShadow ? 6 : 1;
                        float kernelSize = firstFrustum.bOmniDirectionalShadow ? 2.5f : 1.5f;

                        if (numTileLights + numSides > vfInternal::MaxNumTileLights)
                        {
                            continue;  // Skip light
                        }
                        static ICVar* pShadowAtlasResCVar = iConsole->GetCVar("e_ShadowsPoolSize");
                        const Vec2 shadowParams = Vec2(kernelSize * ((float)firstFrustum.nTexSize / (float)pShadowAtlasResCVar->GetIVal()), firstFrustum.fDepthConstBias);

                        const Vec3 cubeDirs[6] = { Vec3(-1, 0, 0), Vec3(1, 0, 0), Vec3(0, -1, 0), Vec3(0, 1, 0), Vec3(0, 0, -1), Vec3(0, 0, 1) };

                        for (int side = 0; side < numSides; ++side)
                        {
                            rd->ConfigShadowTexgen(0, &firstFrustum, side);
                            Matrix44A shadowMat = rd->m_TempMatrices[0][0];

                            // Translate into camera space
                            const Vec4 vEye(rd->GetViewParameters().vOrigin, 0.0f);
                            Vec4 vecTranslation(vEye.Dot((Vec4&)shadowMat.m00), vEye.Dot((Vec4&)shadowMat.m10), vEye.Dot((Vec4&)shadowMat.m20), vEye.Dot((Vec4&)shadowMat.m30));
                            shadowMat.m03 += vecTranslation.x;
                            shadowMat.m13 += vecTranslation.y;
                            shadowMat.m23 += vecTranslation.z;
                            shadowMat.m33 += vecTranslation.w;

                            // Pre-multiply by inverse frustum far plane distance
                            (Vec4&)shadowMat.m20 *= rd->m_cEF.m_TempVecs[2].x;

                            Vec3 cubeDir = cubeDirs[side];
                            Vec4 spotParamsVS = Vec4(cubeDir.x, cubeDir.y, cubeDir.z, 0) * matView;

                            // slightly enlarge the frustum to prevent culling errors
                            spotParamsVS.w = renderLight.m_fRadius * tanf(DEG2RAD(45.0f + 14.5f)) * sqrt_2;

                            Vec3 coneTip = Vec3(lightCullInfo.posRad.x, lightCullInfo.posRad.y, lightCullInfo.posRad.z);
                            Vec3 coneDir = Vec3(-spotParamsVS.x, -spotParamsVS.y, -spotParamsVS.z);
                            AABB coneBounds = AABB::CreateAABBfromCone(Cone(coneTip, coneDir, renderLight.m_fRadius, spotParamsVS.w));
                            Vec2 depthBoundsVS = Vec2(coneBounds.min.z, coneBounds.max.z);
                            Vec2 sideShadowParams = (firstFrustum.nShadowGenMask & (1 << side)) ? shadowParams : Vec2(ZERO);

                            if (side == 0)
                            {
                                lightShadeInfo.shadowParams = sideShadowParams;
                                lightShadeInfo.shadowMatrix = shadowMat;
                                lightShadeInfo.shadowChannelIndex = Vec4(
                                        renderLight.m_ShadowChanMask % 4 == 0,
                                        renderLight.m_ShadowChanMask % 4 == 1,
                                        renderLight.m_ShadowChanMask % 4 == 2,
                                        renderLight.m_ShadowChanMask % 4 == 3
                                        );
                                lightShadeInfo.shadowMaskIndex = renderLight.m_ShadowMaskIndex;

                                if (numSides > 1)
                                {
                                    lightCullInfo.volumeType = vfInternal::tlVolumeCone;
                                    lightShadeInfo.lightType = vfInternal::tlTypeRegularPointFace;
                                    lightShadeInfo.resIndex = side;
                                    lightCullInfo.volumeParams0 = spotParamsVS;
                                    lightCullInfo.depthBounds = depthBoundsVS;
                                    lightCullInfo.miscFlag = 0;
                                }
                            }
                            else
                            {
                                // Split point light
                                ++numTileLights;
                                tileLightsCull[numTileLights] = lightCullInfo;
                                tileLightsShade[numTileLights] = lightShadeInfo;
                                tileLightsShade[numTileLights].shadowParams = sideShadowParams;
                                tileLightsShade[numTileLights].shadowMatrix = shadowMat;
                                tileLightsShade[numTileLights].resIndex = side;
                                tileLightsCull[numTileLights].volumeParams0 = spotParamsVS;
                                tileLightsCull[numTileLights].depthBounds = depthBoundsVS;
                            }
                        }
                    }
                }
            }

            // Add current light
            ++numTileLights;
            ++numValidRenderLights;
        }
    }

    // Invalidate last light in case it got skipped
    if (numTileLights < vfInternal::MaxNumTileLights)
    {
        ZeroMemory(&tileLightsCull[numTileLights], sizeof(vfInternal::SVolumeLightCullInfo));
        ZeroMemory(&tileLightsShade[numTileLights], sizeof(vfInternal::SVolumeLightShadeInfo));
    }

    // Update light buffer
    m_lightCullInfoBuf.UpdateBufferContent(tileLightsCull, sizeof(vfInternal::SVolumeLightCullInfo) * vfInternal::MaxNumTileLights);
    m_LightShadeInfoBuf.UpdateBufferContent(tileLightsShade, sizeof(vfInternal::SVolumeLightShadeInfo) * vfInternal::MaxNumTileLights);

    m_numTileLights = numTileLights;

    // Update global env probes for analytical volumetric fog
    {
        I3DEngine* pEng = gEnv->p3DEngine;
        Vec3 fogAlbedo(0.0f, 0.0f, 0.0f);
        pEng->GetGlobalParameter(E3DPARAM_VOLFOG2_COLOR, fogAlbedo);
        colorGlobalEnvProbe0 = colorGlobalEnvProbe0.CompMul(fogAlbedo);
        colorGlobalEnvProbe1 = colorGlobalEnvProbe1.CompMul(fogAlbedo);
        m_globalEnvProbeParam0 = Vec4(colorGlobalEnvProbe0, attenuationGlobalEnvProbe0);
        m_globalEnvProbeParam1 = Vec4(colorGlobalEnvProbe1, attenuationGlobalEnvProbe1);

        SAFE_RELEASE(m_globalEnvProveTex0);
        SAFE_RELEASE(m_globalEnvProveTex1);
        if (texGlobalEnvProbe0)
        {
            m_globalEnvProveTex0 = texGlobalEnvProbe0;
            m_globalEnvProveTex0->AddRef();
        }
        if (texGlobalEnvProbe1)
        {
            m_globalEnvProveTex1 = texGlobalEnvProbe1;
            m_globalEnvProveTex1->AddRef();
        }
    }
}

void CVolumetricFog::RenderVolumetricsToVolume(void (* RenderFunc)())
{
    if (!IsViable())
    {
        return;
    }

    AZ_TRACE_METHOD();

    static const ICVar* pCVarFogVolumeInject = NULL;
    if (!pCVarFogVolumeInject)
    {
        pCVarFogVolumeInject = gEnv->pConsole->GetCVar("e_FogVolumesTiledInjection");
    }
    bool bFogVolumeInjection = (pCVarFogVolumeInject && pCVarFogVolumeInject->GetIVal() != 0);

    UpdateFrame();

    RenderDownscaledShadowmap();

    BuildLightListGrid();

    RenderDownscaledDepth();

    {
        PROFILE_LABEL_SCOPE("INJECT_PARTICIPATING_MEDIA");

        if (bFogVolumeInjection)
        {
            PrepareFogVolumeList();
            InjectFogDensity();
        }
        else
        {
            InjectExponentialHeightFog();
        }

        CD3D9Renderer* const __restrict rd = gcpRendD3D;
        SRenderPipeline& rp(rd->m_RP);

        // store state
        int oldX, oldY;
        int oldWidth, oldHeight;
        rd->GetViewport(&oldX, &oldY, &oldWidth, &oldHeight);
        const uint64 prevShaderRtFlags = rp.m_FlagsShader_RT;

        if (!bFogVolumeInjection)
        {
            // clear s_ptexVolumetricFogDensityColor.
            rd->FX_ClearTarget(CTexture::s_ptexVolumetricFogDensityColor, Clr_Transparent);
        }

        rd->FX_PushRenderTarget(0, CTexture::s_ptexVolumetricFogDensityColor, NULL);
        rd->FX_PushRenderTarget(1, CTexture::s_ptexVolumetricFogDensity, NULL);
        rd->FX_SetActiveRenderTargets(false);

        // Set the viewport.
        int targetWidth = CTexture::s_ptexVolumetricFogDensity->GetWidth();
        int targetHeight = CTexture::s_ptexVolumetricFogDensity->GetHeight();
        rd->RT_SetViewport(0, 0, targetWidth, targetHeight);

        // overwrite pipeline state.
        const uint32 nOldForceStateAnd = rp.m_ForceStateAnd;
        const uint32 nOldForceStateOr = rp.m_ForceStateOr;
        rp.m_ForceStateAnd = GS_BLSRC_MASK | GS_BLDST_MASK | GS_DEPTHFUNC_MASK | GS_DEPTHWRITE | GS_NODEPTHTEST;
        rp.m_ForceStateOr |= GS_BLSRC_ONE | GS_BLDST_ONE | GS_NODEPTHTEST;

        // inject fog density and fog albedo into volume texture.
        rd->FX_ProcessRenderList(EFSLIST_FOG_VOLUME, 0, RenderFunc, false);
        rd->FX_ProcessRenderList(EFSLIST_FOG_VOLUME, 1, RenderFunc, false);

        rd->FX_PopRenderTarget(0);
        rd->FX_PopRenderTarget(1);
        rd->FX_SetActiveRenderTargets(false);

        // restore pipeline state.
        rp.m_ForceStateAnd = nOldForceStateAnd;
        rp.m_ForceStateOr = nOldForceStateOr;
        rd->RT_SetViewport(oldX, oldY, oldWidth, oldHeight);
        rp.m_FlagsShader_RT = prevShaderRtFlags;
        rd->FX_Commit();
    }
}

void CVolumetricFog::RenderVolumetricFog()
{
    if (!IsViable())
    {
        return;
    }

    PROFILE_LABEL_SCOPE("RENDER_VOLUMETRIC_FOG");

    InjectInscatteringLight();

    const int VolumetricFogBlurInscattering = 1;
    if constexpr (VolumetricFogBlurInscattering > 0)
    {
        int maxBlurCount = VolumetricFogBlurInscattering;
        maxBlurCount = maxBlurCount <= 4 ? maxBlurCount : 4;
        for (int count = 0; count < maxBlurCount; ++count)
        {
#ifdef ENABLE_VOLFOG_TEX_FORMAT_RGBA16F
            BlurInscatterVolume();
#else
            BlurDensityVolume();
            BlurInscatterVolume();
#endif
        }
    }

    ReprojectVolume();

    RaymarchVolumetricFog();
}

float CVolumetricFog::GetDepthIndex(float linearDepth) const
{
    const float raymarchStart = gcpRendD3D->GetViewParameters().fNear;
    Vec3 volFogCtrlParams(0.0f, 0.0f, 0.0f);
    gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLFOG2_CTRL_PARAMS, volFogCtrlParams);
    const float raymarchDistance = (volFogCtrlParams.x > raymarchStart) ? (volFogCtrlParams.x - raymarchStart) : 0.0001f;
    float d = powf(((linearDepth - raymarchStart) / raymarchDistance), (1.0f / 2.0f));
    float maxIndex = static_cast<f32>(CTexture::s_ptexVolumetricFog ? CTexture::s_ptexVolumetricFog->GetDepth() : 1.0f);
    d = (0.5f - d) / maxIndex + d;
    return d;
}

bool CVolumetricFog::IsEnableInFrame() const
{
    bool v = CRenderer::CV_r_DeferredShadingTiled > 0
        && CRenderer::CV_r_DeferredShadingTiled < 4
        && CRenderer::CV_r_usezpass != 0
        && CRenderer::CV_r_Unlit == 0
        && CRenderer::CV_r_DeferredShadingDebug != 2
        && CRenderer::CV_r_measureoverdraw == 0;

    return v;
}

void CVolumetricFog::PushFogVolume(CREFogVolume* pFogVolume, const SRenderingPassInfo& passInfo)
{
    assert(pFogVolume != NULL);
    if (pFogVolume == NULL)
    {
        return;
    }

    const uint32 nThreadID = gcpRendD3D->m_RP.m_nFillThreadID;
    const uint32 nRecurseLevel = passInfo.GetRecursiveLevel();
    const uint32 nVolumeType = pFogVolume->m_volumeType;

    TArray<SFogVolumeInfo>& array = m_fogVolumeInfoArray[nThreadID][nRecurseLevel][nVolumeType];

    if (array.size() >= vfInternal::MaxNumFogVolumes)
    {
        // TODO: show skipped FogVolume number on the screen.
        return;
    }

    SFogVolumeInfo temp;
    temp.m_center = pFogVolume->m_center;
    temp.m_viewerInsideVolume = pFogVolume->m_viewerInsideVolume;
    temp.m_affectsThisAreaOnly = pFogVolume->m_affectsThisAreaOnly;
    temp.m_stencilRef = pFogVolume->m_stencilRef;
    temp.m_volumeType = pFogVolume->m_volumeType;
    temp.m_localAABB = pFogVolume->m_localAABB;
    temp.m_matWSInv = pFogVolume->m_matWSInv;
    temp.m_fogColor = pFogVolume->m_fogColor;
    temp.m_globalDensity = pFogVolume->m_globalDensity;
    temp.m_densityOffset = pFogVolume->m_densityOffset;
    temp.m_softEdgesLerp = pFogVolume->m_softEdgesLerp;
    temp.m_heightFallOffDirScaled = pFogVolume->m_heightFallOffDirScaled;
    temp.m_heightFallOffBasePoint = pFogVolume->m_heightFallOffBasePoint;
    temp.m_eyePosInOS = pFogVolume->m_eyePosInOS;
    temp.m_rampParams = pFogVolume->m_rampParams;
    temp.m_windOffset = pFogVolume->m_windOffset;
    temp.m_noiseScale = pFogVolume->m_noiseScale;
    temp.m_noiseFreq = pFogVolume->m_noiseFreq;
    temp.m_noiseOffset = pFogVolume->m_noiseOffset;
    temp.m_noiseElapsedTime = pFogVolume->m_noiseElapsedTime;
    temp.m_scale = pFogVolume->m_scale;

    array.Add(temp);
}

void CVolumetricFog::ClearFogVolumes()
{
#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
    if(gcpRendD3D->IsRenderToTextureActive())
    {
        return;
    }
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED

    const uint32 nThreadID = gcpRendD3D->m_RP.m_nFillThreadID;
    int32 nRecurseLevel = SRendItem::m_RecurseLevel[nThreadID];

    if (0 <= nRecurseLevel && nRecurseLevel < MAX_REND_RECURSION_LEVELS)
    {
        for (uint32 i = 0; i < MaxNumFogVolumeType; ++i)
        {
            m_fogVolumeInfoArray[nThreadID][nRecurseLevel][i].SetUse(0);
        }
    }
}

void CVolumetricFog::ClearAllFogVolumes()
{
#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
    if(gcpRendD3D->IsRenderToTextureActive())
    {
        return;
    }
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED

    for (int32 i = 0; i < RT_COMMAND_BUF_COUNT; ++i)
    {
        for (int32 j = 0; j < MAX_REND_RECURSION_LEVELS; ++j)
        {
            for (uint32 k = 0; k < MaxNumFogVolumeType; ++k)
            {
                m_fogVolumeInfoArray[i][j][k].Free();
            }
        }
    }
}

void CVolumetricFog::UpdateFrame()
{
    int frameID = gcpRendD3D->GetFrameID(false);
    if (m_frameID != frameID)
    {
        CD3D9Renderer* const __restrict rd = gcpRendD3D;
        Matrix44 mViewProj = rd->m_ViewMatrix * rd->m_ProjMatrix;
        Matrix44& mViewport = SD3DPostEffectsUtils::GetInstance().m_pScaleBias;

        ++m_tick;
        m_frameID = frameID;
        m_viewProj[m_tick % MaxFrameNum] = mViewProj * mViewport;
    }
}

bool CVolumetricFog::IsViable() const
{
    int nThreadID = gcpRendD3D->m_RP.m_nProcessThreadID;
    int nRecurseLevel = SRendItem::m_RecurseLevel[nThreadID];
#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
    if ((gcpRendD3D->m_RP.m_TI[nThreadID].m_PersFlags & RBPF_RENDER_SCENE_TO_TEXTURE) != 0)
    {
        return false;
    }
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED

    bool v = CD3D9Renderer::CV_r_VolumetricFog != 0     // IsEnableInFrame() and e_VolumetricFog are accumulated.
        && gcpRendD3D->m_RP.m_TI[nThreadID].m_FS.m_bEnable
        && nRecurseLevel == 0;

    return v;
}

void CVolumetricFog::InjectInscatteringLight()
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    SRenderPipeline& rp(rd->m_RP);
    const int nThreadID = rp.m_nProcessThreadID;

    // store state
    const int32 prevState = rp.m_CurState;
    const uint32 prevStateAnd = rp.m_StateAnd;
    const uint32 prevStateOr = rp.m_StateOr;
    const uint64 prevShaderRtFlags = rp.m_FlagsShader_RT;
    const int prevPersFlags = rp.m_TI[nThreadID].m_PersFlags;


    const uint32 nScreenWidth = m_InscatteringVolume->GetWidth();
    const uint32 nScreenHeight = m_InscatteringVolume->GetHeight();
    const uint32 volumeDepth = m_InscatteringVolume->GetDepth();

    // calculate fog density and accumulate in-scattering lighting along view ray.
    //PROFILE_LABEL_SCOPE( "INJECT_VOLUMETRIC_FOG_INSCATTERING" );

    D3DShaderResourceView* pNullViews[8] = { NULL };
    rd->m_DevMan.BindSRV(eHWSC_Compute, pNullViews, 0, 8);
    rd->m_DevMan.BindSRV(eHWSC_Compute, pNullViews, 8, 8);
    rd->m_DevMan.BindSRV(eHWSC_Compute, pNullViews, 16, 8);

    D3DUnorderedAccessView* pUAVNull[4] = { NULL };
    rd->m_DevMan.BindUAV(eHWSC_Compute, pUAVNull, NULL, 0, 4);

    // We are bypassing various texture shadow state caches by directly hitting the device manager for clearing texture slots here.
    // CTexture::Apply can cache textures into CTexture::s_TexStages, which are not invalidated by the device manager and can lead CTexture to falsely believe
    // a texture is still bound to the device when it has been cleared.
    rd->RT_UnbindTMUs();

    rd->FX_Commit();
    rd->m_DevMan.CommitDeviceStates();

    rd->FX_SetupShadowsForFog();

    const int nStaticShadowMapSlot = 3;
    SetupStaticShadowMap(nStaticShadowMapSlot);

    rd->GetTiledShading().BindForwardShadingResources(NULL, eHWSC_Compute);

    //// set debug flag
    //if(CRenderer::CV_r_VolumetricFogDebug == 1)
    //{
    //  rp.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_DEBUG0];
    //}
    //else if(CRenderer::CV_r_VolumetricFogDebug == 2)
    //{
    //  rp.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_DEBUG1];
    //}
    //else if(CRenderer::CV_r_VolumetricFogDebug == 3)
    //{
    //  rp.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_DEBUG2];
    //}
    //else if(CRenderer::CV_r_VolumetricFogDebug == 4)
    //{
    //  rp.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_DEBUG3];
    //}

    // set sampling quality
    const uint64 quality  = g_HWSR_MaskBit[HWSR_SAMPLE4];
    const uint64 quality1 = g_HWSR_MaskBit[HWSR_SAMPLE5];
    rp.m_FlagsShader_RT &= ~(quality | quality1);
    switch (CRenderer::CV_r_VolumetricFogSample)
    {
    case 1:
        rp.m_FlagsShader_RT |= quality;
        break;
    case 2:
        rp.m_FlagsShader_RT |= quality1;
        break;
    default:
        break;
    }

    // set shadow quality
    const uint64 shadowQuality  = g_HWSR_MaskBit[HWSR_LIGHTVOLUME0];
    const uint64 shadowQuality1 = g_HWSR_MaskBit[HWSR_LIGHTVOLUME1];
    rp.m_FlagsShader_RT &= ~(shadowQuality | shadowQuality1);
    switch (CRenderer::CV_r_VolumetricFogShadow)
    {
    case 1:
        rp.m_FlagsShader_RT |= shadowQuality;
        break;
    case 2:
        rp.m_FlagsShader_RT |= shadowQuality1;
        break;
    case 3:
        rp.m_FlagsShader_RT |= shadowQuality | shadowQuality1;
        break;
    default:
        break;
    }

    // set downscaled sun shadow maps
    const uint64 shadowMode0  = g_HWSR_MaskBit[HWSR_SAMPLE0];
    const uint64 shadowMode1  = g_HWSR_MaskBit[HWSR_SAMPLE1];
    rp.m_FlagsShader_RT &= ~(shadowMode0 | shadowMode1);
    if (CRenderer::CV_r_VolumetricFogDownscaledSunShadow == 1)
    {
        // replace first and second cascades
        rp.m_FlagsShader_RT |= (CRenderer::CV_r_ShadowsCache > 0) ? shadowMode0 | shadowMode1 : shadowMode0;
    }
    else if (CRenderer::CV_r_VolumetricFogDownscaledSunShadow != 0)
    {
        // replace first, second, and third cascades
        rp.m_FlagsShader_RT |= shadowMode1;
    }

    static CCryNameTSCRC shaderName("InjectVolumetricInscattering");
    SD3DPostEffectsUtils::ShBeginPass(CShaderMan::s_shDeferredShading, shaderName, FEF_DONTSETSTATES);

    D3DUnorderedAccessView* pUAVs[1] = {
        m_InscatteringVolume->GetDeviceUAV(),
    };
    rd->m_DevMan.BindUAV(eHWSC_Compute, pUAVs, NULL, 0, 1);

    if (CRenderer::CV_r_VolumetricFogDownscaledSunShadow != 0)
    {
        D3DShaderResourceView* pDSMs[3] = {
            m_downscaledShadow[0]->GetShaderResourceView(),
            m_downscaledShadow[1]->GetShaderResourceView(),
            m_downscaledShadow[2] ? m_downscaledShadow[2]->GetShaderResourceView() : NULL,
        };
        int count = (CRenderer::CV_r_VolumetricFogDownscaledSunShadow == 1) ? 2 : 3;
        rd->m_DevMan.BindSRV(eHWSC_Compute, pDSMs, 5, count);
    }

#ifdef ENABLE_VOLFOG_TEX_FORMAT_RGBA16F
    D3DShaderResourceView* pSRVs[7] = {
        m_lightGridBuf.GetShaderResourceView(),
        m_lightCountBuf.GetShaderResourceView(),
        m_MaxDepth->GetShaderResourceView(),
        CTexture::s_ptexVolumetricClipVolumeStencil->GetShaderResourceView(),
        m_LightShadeInfoBuf.GetShaderResourceView(),
        CTexture::s_ptexVolumetricFogDensityColor->GetShaderResourceView(),
        CTexture::s_ptexVolumetricFogDensity->GetShaderResourceView(),
    };
    rd->m_DevMan.BindSRV(eHWSC_Compute, pSRVs, 8, 7);
#else
    D3DShaderResourceView* pSRVs[6] = {
        m_lightGridBuf.GetShaderResourceView(),
        m_lightCountBuf.GetShaderResourceView(),
        m_MaxDepth->GetShaderResourceView(),
        CTexture::s_ptexVolumetricClipVolumeStencil->GetShaderResourceView(),
        m_LightShadeInfoBuf.GetShaderResourceView(),
        CTexture::s_ptexVolumetricFogDensityColor->GetShaderResourceView(),
    };
    rd->m_DevMan.BindSRV(eHWSC_Compute, pSRVs, 8, 6);
#endif

    //D3DSamplerState *pSamplers[2] = {
    //  (D3DSamplerState *)CTexture::s_TexStates[m_nTexStatePoint].m_pDeviceState,
    //  (D3DSamplerState *)CTexture::s_TexStates[m_nTexStateTriLinear].m_pDeviceState,
    //};
    //rd->m_DevMan.BindSampler( eHWSC_Compute, pSamplers, 0, 2 );

    static CCryNameR paramScreenSize("ScreenSize");
    float fScreenWidth = (float)nScreenWidth;
    float fScreenHeight = (float)nScreenHeight;
    Vec4 vParamScreenSize(fScreenWidth, fScreenHeight, 1.0f / fScreenWidth, 1.0f / fScreenHeight);
    CShaderMan::s_shDeferredShading->FXSetCSFloat(paramScreenSize, &vParamScreenSize, 1);

    SD3DPostEffectsUtils::UpdateFrustumCorners();
    static CCryNameR paramFrustumTL("FrustumTL");
    Vec4 vParamFrustumTL(SD3DPostEffectsUtils::m_vLT.x, SD3DPostEffectsUtils::m_vLT.y, SD3DPostEffectsUtils::m_vLT.z, 0);
    CShaderMan::s_shDeferredShading->FXSetCSFloat(paramFrustumTL, &vParamFrustumTL, 1);
    static CCryNameR paramFrustumTR("FrustumTR");
    Vec4 vParamFrustumTR(SD3DPostEffectsUtils::m_vRT.x, SD3DPostEffectsUtils::m_vRT.y, SD3DPostEffectsUtils::m_vRT.z, 0);
    CShaderMan::s_shDeferredShading->FXSetCSFloat(paramFrustumTR, &vParamFrustumTR, 1);
    static CCryNameR paramFrustumBL("FrustumBL");
    Vec4 vParamFrustumBL(SD3DPostEffectsUtils::m_vLB.x, SD3DPostEffectsUtils::m_vLB.y, SD3DPostEffectsUtils::m_vLB.z, 0);
    CShaderMan::s_shDeferredShading->FXSetCSFloat(paramFrustumBL, &vParamFrustumBL, 1);

    float octave = (CRenderer::CV_r_VolumetricFogReprojectionBlendFactor > 0.0f) ? -1.7f : 0.0f;
    Vec3 sunDir = gEnv->p3DEngine->GetSunDirNormalized();
    static CCryNameR paramSunDir("SunDir");
    Vec4 vParamSunDir(sunDir.x, sunDir.y, sunDir.z, octave);
    CShaderMan::s_shDeferredShading->FXSetCSFloat(paramSunDir, &vParamSunDir, 1);

    Vec3 sunColor;
    gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_SUN_COLOR, sunColor);
    static CCryNameR paramSunColor("SunColor");
    Vec4 vParamSunColor(sunColor.x, sunColor.y, sunColor.z, 0.0f);
    CShaderMan::s_shDeferredShading->FXSetCSFloat(paramSunColor, &vParamSunColor, 1);

    Vec3 fogAlbedoColor;
    gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLFOG2_COLOR, fogAlbedoColor);
    static CCryNameR paramFogColor("ExponentialHeightFogColor");
    Vec4 vParamFogColor(fogAlbedoColor.x, fogAlbedoColor.y, fogAlbedoColor.z, 0.0f);
    CShaderMan::s_shDeferredShading->FXSetCSFloat(paramFogColor, &vParamFogColor, 1);

    Vec3 scatteringParam;
    gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLFOG2_SCATTERING_PARAMS, scatteringParam);
    static CCryNameR paramInjectInscattering("InjectInscatteringParams");
    float k = scatteringParam.z;
    bool bNegative = k < 0.0f ? true : false;
    k = (abs(k) > 0.99999f) ? (bNegative ? -0.99999f : 0.99999f) : k;
    Vec4 vParamInjectInscattering(k, 1.0f - k * k, 0.0f, 0.0f);
    CShaderMan::s_shDeferredShading->FXSetCSFloat(paramInjectInscattering, &vParamInjectInscattering, 1);

    const uint32 tileSizeX = 4;
    const uint32 tileSizeY = 4;
    const uint32 tileSizeZ = 4;
    uint32 dispatchSizeX = (nScreenWidth / tileSizeX) + (nScreenWidth % tileSizeX > 0 ? 1 : 0);
    uint32 dispatchSizeY = (nScreenHeight / tileSizeY) + (nScreenHeight % tileSizeY > 0 ? 1 : 0);
    uint32 dispatchSizeZ = (volumeDepth / tileSizeZ) + (volumeDepth % tileSizeZ > 0 ? 1 : 0);
    static CCryNameR paramDispatchSize("DispatchSize");
    Vec4 vParamDispatchSize((float)dispatchSizeX, (float)dispatchSizeY, (float)dispatchSizeZ, 0.0f);
    CShaderMan::s_shDeferredShading->FXSetCSFloat(paramDispatchSize, &vParamDispatchSize, 1);

    rd->FX_Commit();

    rd->m_DevMan.Dispatch(dispatchSizeX, dispatchSizeY, dispatchSizeZ);

    SD3DPostEffectsUtils::ShEndPass();

    rd->GetTiledShading().UnbindForwardShadingResources(eHWSC_Compute);


    //D3DUAV* pUAVNull[4] = { NULL };
    rd->m_DevMan.BindUAV(eHWSC_Compute, pUAVNull, NULL, 0, 4);

    D3DShaderResourceView* pSRVNull[8] = { NULL };
    rd->m_DevMan.BindSRV(eHWSC_Compute, pSRVNull, 0, 8);
    rd->m_DevMan.BindSRV(eHWSC_Compute, pSRVNull, 8, 8);
    rd->m_DevMan.BindSRV(eHWSC_Compute, pSRVNull, 16, 8);
    
    // We are bypassing various texture shadow state caches by directly hitting the device manager for clearing texture slots here.
    // CTexture::Apply can cache textures into CTexture::s_TexStages, which are not invalidated by the device manager and can lead CTexture to falsely believe
    // a texture is still bound to the device when it has been cleared.
    rd->RT_UnbindTMUs();

    // restore state
    rp.m_StateAnd = prevStateAnd;
    rp.m_StateOr = prevStateOr;
    rd->SetState(prevState);
    rp.m_TI[nThreadID].m_PersFlags = prevPersFlags;
    rp.m_FlagsShader_RT = prevShaderRtFlags;

    rd->FX_Commit();
    rd->m_DevMan.CommitDeviceStates();
}

void CVolumetricFog::BuildLightListGrid()
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    SRenderPipeline& rp(rd->m_RP);
    const int nThreadID = rp.m_nProcessThreadID;

    // store state
    const int32 prevState = rp.m_CurState;
    const uint32 prevStateAnd = rp.m_StateAnd;
    const uint32 prevStateOr = rp.m_StateOr;
    const uint64 prevShaderRtFlags = rp.m_FlagsShader_RT;
    const int prevPersFlags = rp.m_TI[nThreadID].m_PersFlags;


    const uint32 nScreenWidth = m_InscatteringVolume->GetWidth();
    const uint32 nScreenHeight = m_InscatteringVolume->GetHeight();
    const uint32 volumeDepth = m_InscatteringVolume->GetDepth();

    // calculate fog density and accumulate in-scattering lighting along view ray.
    //PROFILE_LABEL_SCOPE( "BUILD_VOLUMETRIC_FOG_CLUSTERED_LIGHT_GRID" );

    D3DShaderResourceView* pNullViews[8] = { NULL };
    rd->m_DevMan.BindSRV(eHWSC_Compute, pNullViews, 0, 8);
    rd->m_DevMan.BindSRV(eHWSC_Compute, pNullViews, 16, 8);

    D3DUnorderedAccessView* pUAVNull[4] = { NULL };
    rd->m_DevMan.BindUAV(eHWSC_Compute, pUAVNull, NULL, 0, 4);

    rd->FX_Commit();


    static CCryNameTSCRC shaderName("BuildLightListGrid");
    SD3DPostEffectsUtils::ShBeginPass(CShaderMan::s_shDeferredShading, shaderName, FEF_DONTSETSTATES);

    D3DUnorderedAccessView* pUAVs[2] = {
        m_lightGridBuf.GetUnorderedAccessView(),
        m_lightCountBuf.GetUnorderedAccessView(),
    };
    rd->m_DevMan.BindUAV(eHWSC_Compute, pUAVs, NULL, 0, 2);

    D3DShaderResourceView* pSRV[1] = {
        m_lightCullInfoBuf.GetShaderResourceView(),
    };
    rd->m_DevMan.BindSRV(eHWSC_Compute, pSRV, 0, 1);

    static CCryNameR paramScreenSize("ScreenSize");
    float fScreenWidth = (float)nScreenWidth;
    float fScreenHeight = (float)nScreenHeight;
    Vec4 vParamScreenSize(fScreenWidth, fScreenHeight, 1.0f / fScreenWidth, 1.0f / fScreenHeight);
    CShaderMan::s_shDeferredShading->FXSetCSFloat(paramScreenSize, &vParamScreenSize, 1);

    SD3DPostEffectsUtils::UpdateFrustumCorners();
    static CCryNameR paramFrustumTL("FrustumTL");
    Vec4 vParamFrustumTL(SD3DPostEffectsUtils::m_vLT.x, SD3DPostEffectsUtils::m_vLT.y, SD3DPostEffectsUtils::m_vLT.z, 0);
    CShaderMan::s_shDeferredShading->FXSetCSFloat(paramFrustumTL, &vParamFrustumTL, 1);
    static CCryNameR paramFrustumTR("FrustumTR");
    Vec4 vParamFrustumTR(SD3DPostEffectsUtils::m_vRT.x, SD3DPostEffectsUtils::m_vRT.y, SD3DPostEffectsUtils::m_vRT.z, 0);
    CShaderMan::s_shDeferredShading->FXSetCSFloat(paramFrustumTR, &vParamFrustumTR, 1);
    static CCryNameR paramFrustumBL("FrustumBL");
    Vec4 vParamFrustumBL(SD3DPostEffectsUtils::m_vLB.x, SD3DPostEffectsUtils::m_vLB.y, SD3DPostEffectsUtils::m_vLB.z, 0);
    CShaderMan::s_shDeferredShading->FXSetCSFloat(paramFrustumBL, &vParamFrustumBL, 1);

    const CameraViewParameters& rc = gcpRendD3D->GetViewParameters();
    float yfov, xfov, aspect, ndist, fdist;
    rc.GetPerspectiveParams(&yfov, &xfov, &aspect, &ndist, &fdist);
    static CCryNameR paramNameScreenInfo("ScreenInfo");
    Vec4 screenInfo(ndist, fdist, 0.0f, (float)volumeDepth);
    CShaderMan::s_shDeferredShading->FXSetCSFloat(paramNameScreenInfo, &screenInfo, 1);

    static CCryNameR paramProj("ProjParams");
    Vec4 vParamProj(rd->m_ProjMatrix.m00, rd->m_ProjMatrix.m11, rd->m_ProjMatrix.m20, rd->m_ProjMatrix.m21);
    CShaderMan::s_shDeferredShading->FXSetCSFloat(paramProj, &vParamProj, 1);

    uint32 numTileLights = m_numTileLights;

    const uint32 tileSizeX = 4;
    const uint32 tileSizeY = 4;
    const uint32 tileSizeZ = 4;
    uint32 dispatchSizeX = (nScreenWidth / tileSizeX) + (nScreenWidth % tileSizeX > 0 ? 1 : 0);
    uint32 dispatchSizeY = (nScreenHeight / tileSizeY) + (nScreenHeight % tileSizeY > 0 ? 1 : 0);
    uint32 dispatchSizeZ = (volumeDepth / tileSizeZ) + (volumeDepth % tileSizeZ > 0 ? 1 : 0);
    static CCryNameR paramDispatchSize("DispatchSize");
    Vec4 vParamDispatchSize((float)dispatchSizeX, (float)dispatchSizeY, (float)dispatchSizeZ, (float)numTileLights);
    CShaderMan::s_shDeferredShading->FXSetCSFloat(paramDispatchSize, &vParamDispatchSize, 1);

    rd->FX_Commit();

    rd->m_DevMan.Dispatch(dispatchSizeX, dispatchSizeY, dispatchSizeZ);

    SD3DPostEffectsUtils::ShEndPass();


    //D3DUAV* pUAVNull[4] = { NULL };
    rd->m_DevMan.BindUAV(eHWSC_Compute, pUAVNull, NULL, 0, 4);

    D3DShaderResourceView* pSRVNull[8] = { NULL };
    rd->m_DevMan.BindSRV(eHWSC_Compute, pSRVNull, 0, 1);

    //D3DSamplerState* pSampNull[2] = { NULL };
    //rd->m_DevMan.BindSampler( eHWSC_Compute, pSampNull, 0, 2 );

    // restore state
    rp.m_StateAnd = prevStateAnd;
    rp.m_StateOr = prevStateOr;
    rd->SetState(prevState);
    rp.m_TI[nThreadID].m_PersFlags = prevPersFlags;
    rp.m_FlagsShader_RT = prevShaderRtFlags;

    rd->FX_Commit();
}

void CVolumetricFog::RaymarchVolumetricFog()
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    // Unbind the VolumetricFog SRV (used in the recursive pass). It gets 
    // implicitly unbound when used as a UAV. Without explicitly calling 
    // Unbind(), it may not get re-bound as an SRV for future shaders.
    CTexture::s_ptexVolumetricFog->Unbind();

    // store state
    SRenderPipeline& rp(rd->m_RP);
    const uint64 prevShaderRtFlags = rp.m_FlagsShader_RT;

    const uint32 nScreenWidth = CTexture::s_ptexVolumetricFogDensity->GetWidth();
    const uint32 nScreenHeight = CTexture::s_ptexVolumetricFogDensity->GetHeight();
    const uint32 volumeDepth = CTexture::s_ptexVolumetricFogDensity->GetDepth();

    //PROFILE_LABEL_SCOPE( "RAYMARCH_VOLUMETRIC_FOG" );

    // accumulate in-scattering lighting along view ray.
    static CCryNameTSCRC shaderName("RaymarchVolumetricFog");
    SD3DPostEffectsUtils::ShBeginPass(CShaderMan::s_shDeferredShading, shaderName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

    D3DUnorderedAccessView* pUAVs[1] = {
        CTexture::s_ptexVolumetricFog->GetDeviceUAV(),
    };
    rd->m_DevMan.BindUAV(eHWSC_Compute, pUAVs, NULL, 0, 1);

#ifdef ENABLE_VOLFOG_TEX_FORMAT_RGBA16F
    D3DShaderResourceView* pSRVs[1] = {
        (GetInscatterTex()->GetShaderResourceView()),
    };
    rd->m_DevMan.BindSRV(eHWSC_Compute, pSRVs, 0, 1);
#else
    D3DShaderResourceView* pSRVs[2] = {
        (GetInscatterTex()->GetShaderResourceView()),
        (GetDensityTex()->GetShaderResourceView()),
    };
    rd->m_DevMan.BindSRV(eHWSC_Compute, pSRVs, 0, 2);
#endif

    //D3DSamplerState *pSamplers[2] = {
    //  (D3DSamplerState *)CTexture::s_TexStates[m_nTexStatePoint].m_pDeviceState,
    //  (D3DSamplerState *)CTexture::s_TexStates[m_nTexStateTriLinear].m_pDeviceState,
    //};
    //rd->m_DevMan.BindSampler( eHWSC_Compute, pSamplers, 0, 2 );

    static CCryNameR paramScreenSize("ScreenSize");
    float fScreenWidth = (float)nScreenWidth;
    float fScreenHeight = (float)nScreenHeight;
    Vec4 vParamScreenSize(fScreenWidth, fScreenHeight, 1.0f / fScreenWidth, 1.0f / fScreenHeight);
    CShaderMan::s_shDeferredShading->FXSetCSFloat(paramScreenSize, &vParamScreenSize, 1);

    SD3DPostEffectsUtils::UpdateFrustumCorners();
    static CCryNameR paramFrustumTL("FrustumTL");
    Vec4 vParamFrustumTL(SD3DPostEffectsUtils::m_vLT.x, SD3DPostEffectsUtils::m_vLT.y, SD3DPostEffectsUtils::m_vLT.z, 0);
    CShaderMan::s_shDeferredShading->FXSetCSFloat(paramFrustumTL, &vParamFrustumTL, 1);
    static CCryNameR paramFrustumTR("FrustumTR");
    Vec4 vParamFrustumTR(SD3DPostEffectsUtils::m_vRT.x, SD3DPostEffectsUtils::m_vRT.y, SD3DPostEffectsUtils::m_vRT.z, 0);
    CShaderMan::s_shDeferredShading->FXSetCSFloat(paramFrustumTR, &vParamFrustumTR, 1);
    static CCryNameR paramFrustumBL("FrustumBL");
    Vec4 vParamFrustumBL(SD3DPostEffectsUtils::m_vLB.x, SD3DPostEffectsUtils::m_vLB.y, SD3DPostEffectsUtils::m_vLB.z, 0);
    CShaderMan::s_shDeferredShading->FXSetCSFloat(paramFrustumBL, &vParamFrustumBL, 1);

    const CameraViewParameters& rc = gcpRendD3D->GetViewParameters();
    float yfov, xfov, aspect, ndist, fdist;
    rc.GetPerspectiveParams(&yfov, &xfov, &aspect, &ndist, &fdist);
    static CCryNameR paramNameScreenInfo("ScreenInfo");
    Vec4 screenInfo(ndist, fdist, 0.0f, (float)volumeDepth);
    CShaderMan::s_shDeferredShading->FXSetCSFloat(paramNameScreenInfo, &screenInfo, 1);

    rd->FX_Commit();

    const uint32 tileSizeX = 8;
    const uint32 tileSizeY = 8;
    uint32 dispatchSizeX = (nScreenWidth / tileSizeX) + (nScreenWidth % tileSizeX > 0 ? 1 : 0);
    uint32 dispatchSizeY = (nScreenHeight / tileSizeY) + (nScreenHeight % tileSizeY > 0 ? 1 : 0);
    rd->m_DevMan.Dispatch(dispatchSizeX, dispatchSizeY, 1);

    SD3DPostEffectsUtils::ShEndPass();


    D3DUnorderedAccessView* pUAVNull[4] = { NULL };
    rd->m_DevMan.BindUAV(eHWSC_Compute, pUAVNull, NULL, 0, 4);

    D3DShaderResourceView* pSRVNull[8] = { NULL };
    rd->m_DevMan.BindSRV(eHWSC_Compute, pSRVNull, 0, 2);

    D3DSamplerState* pSampNull[2] = { NULL };
    rd->m_DevMan.BindSampler(eHWSC_Compute, pSampNull, 0, 2);

    // restore state
    rp.m_FlagsShader_RT = prevShaderRtFlags;

    rd->FX_Commit();
    rd->m_DevMan.CommitDeviceStates();
}

void CVolumetricFog::BlurInscatterVolume()
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    const uint32 nScreenWidth = m_InscatteringVolume->GetWidth();
    const uint32 nScreenHeight = m_InscatteringVolume->GetHeight();
    const uint32 volumeDepth = m_InscatteringVolume->GetDepth();

    // blur inscattering volume texture for removing jittering noise.
    //PROFILE_LABEL_SCOPE( "VOLUMETRIC_FOG_BLUR" );

    {
        static CCryNameTSCRC shaderNameHorizontal("BlurHorizontalInscatteringVolume");
        SD3DPostEffectsUtils::ShBeginPass(CShaderMan::s_shDeferredShading, shaderNameHorizontal, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

        D3DUnorderedAccessView* pUAVs[1] = {
            static_cast<D3DUnorderedAccessView*>(GetInscatterTex()->GetDeviceUAV()),
        };
        rd->m_DevMan.BindUAV(eHWSC_Compute, pUAVs, NULL, 0, 1);

        D3DShaderResourceView* pSRVs[2] = {
            m_InscatteringVolume->GetShaderResourceView(),
            m_MaxDepth->GetShaderResourceView(),
        };
        rd->m_DevMan.BindSRV(eHWSC_Compute, pSRVs, 0, 2);

        D3DSamplerState* pSamplers[1] = {
            (D3DSamplerState*)CTexture::s_TexStates[m_nTexStateTriLinear].m_pDeviceState,
        };
        rd->m_DevMan.BindSampler(eHWSC_Compute, pSamplers, 0, 1);

        static CCryNameR paramScreenSize("ScreenSize");
        float fScreenWidth = (float)nScreenWidth;
        float fScreenHeight = (float)nScreenHeight;
        Vec4 vParamScreenSize(fScreenWidth, fScreenHeight, 1.0f / fScreenWidth, 1.0f / fScreenHeight);
        CShaderMan::s_shDeferredShading->FXSetCSFloat(paramScreenSize, &vParamScreenSize, 1);

        const CameraViewParameters& rc = gcpRendD3D->GetViewParameters();
        float yfov, xfov, aspect, ndist, fdist;
        rc.GetPerspectiveParams(&yfov, &xfov, &aspect, &ndist, &fdist);
        static CCryNameR paramNameScreenInfo("ScreenInfo");
        Vec4 screenInfo(ndist, fdist, 0.0f, (float)volumeDepth);
        CShaderMan::s_shDeferredShading->FXSetCSFloat(paramNameScreenInfo, &screenInfo, 1);

        rd->FX_Commit();

        const uint32 tileSizeX = 8;
        const uint32 tileSizeY = 8;
        const uint32 tileSizeZ = 1;
        uint32 dispatchSizeX = (nScreenWidth / tileSizeX) + (nScreenWidth % tileSizeX > 0 ? 1 : 0);
        uint32 dispatchSizeY = (nScreenHeight / tileSizeY) + (nScreenHeight % tileSizeY > 0 ? 1 : 0);
        uint32 dispatchSizeZ = (volumeDepth / tileSizeZ) + (volumeDepth % tileSizeZ > 0 ? 1 : 0);
        rd->m_DevMan.Dispatch(dispatchSizeX, dispatchSizeY, dispatchSizeZ);

        SD3DPostEffectsUtils::ShEndPass();
    }

    {
        D3DShaderResourceView* pNullViews[8] = { NULL };
        rd->m_DevMan.BindSRV(eHWSC_Compute, pNullViews, 0, 8);

        D3DUnorderedAccessView* pUAVNull[4] = { NULL };
        rd->m_DevMan.BindUAV(eHWSC_Compute, pUAVNull, NULL, 0, 4);

        rd->FX_Commit();
        rd->m_DevMan.CommitDeviceStates();
    }

    {
        static CCryNameTSCRC shaderNameVertical("BlurVerticalInscatteringVolume");
        SD3DPostEffectsUtils::ShBeginPass(CShaderMan::s_shDeferredShading, shaderNameVertical, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

        D3DUnorderedAccessView* pUAVs[1] = {
            static_cast<D3DUnorderedAccessView*>(m_InscatteringVolume->GetDeviceUAV()),
        };
        rd->m_DevMan.BindUAV(eHWSC_Compute, pUAVs, NULL, 0, 1);

        D3DShaderResourceView* pSRVs[2] = {
            (GetInscatterTex()->GetShaderResourceView()),
            m_MaxDepth->GetShaderResourceView(),
        };
        rd->m_DevMan.BindSRV(eHWSC_Compute, pSRVs, 0, 2);

        D3DSamplerState* pSamplers[1] = {
            (D3DSamplerState*)CTexture::s_TexStates[m_nTexStateTriLinear].m_pDeviceState,
        };
        rd->m_DevMan.BindSampler(eHWSC_Compute, pSamplers, 0, 1);

        static CCryNameR paramScreenSize("ScreenSize");
        float fScreenWidth = (float)nScreenWidth;
        float fScreenHeight = (float)nScreenHeight;
        Vec4 vParamScreenSize(fScreenWidth, fScreenHeight, 1.0f / fScreenWidth, 1.0f / fScreenHeight);
        CShaderMan::s_shDeferredShading->FXSetCSFloat(paramScreenSize, &vParamScreenSize, 1);

        const CameraViewParameters& rc = gcpRendD3D->GetViewParameters();
        float yfov, xfov, aspect, ndist, fdist;
        rc.GetPerspectiveParams(&yfov, &xfov, &aspect, &ndist, &fdist);
        static CCryNameR paramNameScreenInfo("ScreenInfo");
        Vec4 screenInfo(ndist, fdist, 0.0f, (float)volumeDepth);
        CShaderMan::s_shDeferredShading->FXSetCSFloat(paramNameScreenInfo, &screenInfo, 1);

        rd->FX_Commit();

        const uint32 tileSizeX = 8;
        const uint32 tileSizeY = 8;
        const uint32 tileSizeZ = 1;
        uint32 dispatchSizeX = (nScreenWidth / tileSizeX) + (nScreenWidth % tileSizeX > 0 ? 1 : 0);
        uint32 dispatchSizeY = (nScreenHeight / tileSizeY) + (nScreenHeight % tileSizeY > 0 ? 1 : 0);
        uint32 dispatchSizeZ = (volumeDepth / tileSizeZ) + (volumeDepth % tileSizeZ > 0 ? 1 : 0);
        rd->m_DevMan.Dispatch(dispatchSizeX, dispatchSizeY, dispatchSizeZ);

        SD3DPostEffectsUtils::ShEndPass();
    }

    D3DUnorderedAccessView* pUAVNull4[4] = { NULL };
    rd->m_DevMan.BindUAV(eHWSC_Compute, pUAVNull4, NULL, 0, 4);

    D3DShaderResourceView* pNullViews[8] = { NULL };
    rd->m_DevMan.BindSRV(eHWSC_Compute, pNullViews, 0, 8);

    D3DSamplerState* pSampNull[2] = { NULL };
    rd->m_DevMan.BindSampler(eHWSC_Compute, pSampNull, 0, 2);

    rd->FX_Commit();
    rd->m_DevMan.CommitDeviceStates();
}

void CVolumetricFog::BlurDensityVolume()
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    const uint32 nScreenWidth = CTexture::s_ptexVolumetricFogDensity->GetWidth();
    const uint32 nScreenHeight = CTexture::s_ptexVolumetricFogDensity->GetHeight();
    const uint32 volumeDepth = CTexture::s_ptexVolumetricFogDensity->GetDepth();

    // blur density volume texture for removing jitter noise.
    //PROFILE_LABEL_SCOPE( "VOLUMETRIC_FOG_BLUR_DENSITY" );

    {
        D3DShaderResourceView* pNullViews[8] = { NULL };
        rd->m_DevMan.BindSRV(eHWSC_Compute, pNullViews, 0, 8);

        D3DUnorderedAccessView* pUAVNull[4] = { NULL };
        rd->m_DevMan.BindUAV(eHWSC_Compute, pUAVNull, NULL, 0, 4);

        rd->FX_Commit();
        rd->m_DevMan.CommitDeviceStates();
    }

    {
        static CCryNameTSCRC shaderNameHorizontal("BlurHorizontalDensityVolume");
        SD3DPostEffectsUtils::ShBeginPass(CShaderMan::s_shDeferredShading, shaderNameHorizontal, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

        D3DUnorderedAccessView* pUAVs[1] = {
            static_cast<D3DUnorderedAccessView*>(GetDensityTex()->GetDeviceUAV()),
        };
        rd->m_DevMan.BindUAV(eHWSC_Compute, pUAVs, NULL, 0, 1);

        D3DShaderResourceView* pSRVs[2] = {
            CTexture::s_ptexVolumetricFogDensity->GetShaderResourceView(),
            m_MaxDepth->GetShaderResourceView(),
        };
        rd->m_DevMan.BindSRV(eHWSC_Compute, pSRVs, 0, 2);

        D3DSamplerState* pSamplers[1] = {
            (D3DSamplerState*)CTexture::s_TexStates[m_nTexStateTriLinear].m_pDeviceState,
        };
        rd->m_DevMan.BindSampler(eHWSC_Compute, pSamplers, 0, 1);

        static CCryNameR paramScreenSize("ScreenSize");
        float fScreenWidth = (float)nScreenWidth;
        float fScreenHeight = (float)nScreenHeight;
        Vec4 vParamScreenSize(fScreenWidth, fScreenHeight, 1.0f / fScreenWidth, 1.0f / fScreenHeight);
        CShaderMan::s_shDeferredShading->FXSetCSFloat(paramScreenSize, &vParamScreenSize, 1);

        const CameraViewParameters& rc = gcpRendD3D->GetViewParameters();
        float yfov, xfov, aspect, ndist, fdist;
        rc.GetPerspectiveParams(&yfov, &xfov, &aspect, &ndist, &fdist);
        static CCryNameR paramNameScreenInfo("ScreenInfo");
        Vec4 screenInfo(ndist, fdist, 0.0f, (float)volumeDepth);
        CShaderMan::s_shDeferredShading->FXSetCSFloat(paramNameScreenInfo, &screenInfo, 1);

        rd->FX_Commit();

        const uint32 tileSizeX = 8;
        const uint32 tileSizeY = 8;
        const uint32 tileSizeZ = 1;
        uint32 dispatchSizeX = (nScreenWidth / tileSizeX) + (nScreenWidth % tileSizeX > 0 ? 1 : 0);
        uint32 dispatchSizeY = (nScreenHeight / tileSizeY) + (nScreenHeight % tileSizeY > 0 ? 1 : 0);
        uint32 dispatchSizeZ = (volumeDepth / tileSizeZ) + (volumeDepth % tileSizeZ > 0 ? 1 : 0);
        rd->m_DevMan.Dispatch(dispatchSizeX, dispatchSizeY, dispatchSizeZ);

        SD3DPostEffectsUtils::ShEndPass();
    }

    {
        D3DShaderResourceView* pNullViews[8] = { NULL };
        rd->m_DevMan.BindSRV(eHWSC_Compute, pNullViews, 0, 8);

        D3DUnorderedAccessView* pUAVNull[4] = { NULL };
        rd->m_DevMan.BindUAV(eHWSC_Compute, pUAVNull, NULL, 0, 4);

        rd->FX_Commit();
        rd->m_DevMan.CommitDeviceStates();
    }

    {
        static CCryNameTSCRC shaderNameVertical("BlurVerticalDensityVolume");
        SD3DPostEffectsUtils::ShBeginPass(CShaderMan::s_shDeferredShading, shaderNameVertical, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

        D3DUnorderedAccessView* pUAVs[1] = {
            static_cast<D3DUnorderedAccessView*>(CTexture::s_ptexVolumetricFogDensity->GetDeviceUAV()),
        };
        rd->m_DevMan.BindUAV(eHWSC_Compute, pUAVs, NULL, 0, 1);

        D3DShaderResourceView* pSRVs[2] = {
            (GetDensityTex()->GetShaderResourceView()),
            m_MaxDepth->GetShaderResourceView(),
        };
        rd->m_DevMan.BindSRV(eHWSC_Compute, pSRVs, 0, 2);

        D3DSamplerState* pSamplers[1] = {
            (D3DSamplerState*)CTexture::s_TexStates[m_nTexStateTriLinear].m_pDeviceState,
        };
        rd->m_DevMan.BindSampler(eHWSC_Compute, pSamplers, 0, 1);

        static CCryNameR paramScreenSize("ScreenSize");
        float fScreenWidth = (float)nScreenWidth;
        float fScreenHeight = (float)nScreenHeight;
        Vec4 vParamScreenSize(fScreenWidth, fScreenHeight, 1.0f / fScreenWidth, 1.0f / fScreenHeight);
        CShaderMan::s_shDeferredShading->FXSetCSFloat(paramScreenSize, &vParamScreenSize, 1);

        const CameraViewParameters& rc = gcpRendD3D->GetViewParameters();
        float yfov, xfov, aspect, ndist, fdist;
        rc.GetPerspectiveParams(&yfov, &xfov, &aspect, &ndist, &fdist);
        static CCryNameR paramNameScreenInfo("ScreenInfo");
        Vec4 screenInfo(ndist, fdist, 0.0f, (float)volumeDepth);
        CShaderMan::s_shDeferredShading->FXSetCSFloat(paramNameScreenInfo, &screenInfo, 1);

        rd->FX_Commit();

        const uint32 tileSizeX = 8;
        const uint32 tileSizeY = 8;
        const uint32 tileSizeZ = 1;
        uint32 dispatchSizeX = (nScreenWidth / tileSizeX) + (nScreenWidth % tileSizeX > 0 ? 1 : 0);
        uint32 dispatchSizeY = (nScreenHeight / tileSizeY) + (nScreenHeight % tileSizeY > 0 ? 1 : 0);
        uint32 dispatchSizeZ = (volumeDepth / tileSizeZ) + (volumeDepth % tileSizeZ > 0 ? 1 : 0);
        rd->m_DevMan.Dispatch(dispatchSizeX, dispatchSizeY, dispatchSizeZ);

        SD3DPostEffectsUtils::ShEndPass();
    }

    D3DUnorderedAccessView* pUAVNull4[4] = { NULL };
    rd->m_DevMan.BindUAV(eHWSC_Compute, pUAVNull4, NULL, 0, 4);

    D3DShaderResourceView* pNullViews[8] = { NULL };
    rd->m_DevMan.BindSRV(eHWSC_Compute, pNullViews, 0, 8);

    D3DSamplerState* pSampNull[2] = { NULL };
    rd->m_DevMan.BindSampler(eHWSC_Compute, pSampNull, 0, 2);

    rd->FX_Commit();
    rd->m_DevMan.CommitDeviceStates();
}

void CVolumetricFog::ReprojectVolume()
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    // store state
    SRenderPipeline& rp(rd->m_RP);
    const uint64 prevShaderRtFlags = rp.m_FlagsShader_RT;

    const uint32 nScreenWidth = m_InscatteringVolume->GetWidth();
    const uint32 nScreenHeight = m_InscatteringVolume->GetHeight();
    const uint32 volumeDepth = m_InscatteringVolume->GetDepth();

    //PROFILE_LABEL_SCOPE( "REPROJECT_VOLUMETRIC_FOG" );


    // no reprojection in cleared frames
    if (m_Cleared > 0)
    {
#ifdef ENABLE_VOLFOG_TEX_FORMAT_RGBA16F
        rd->GetDeviceContext().CopyResource(GetPrevInscatterTex()->GetDevTexture()->GetVolumeTexture(),
            m_InscatteringVolume->GetDevTexture()->GetVolumeTexture());
#else
        rd->GetDeviceContext().CopyResource(GetPrevInscatterTex()->GetDevTexture()->GetVolumeTexture(),
            m_InscatteringVolume->GetDevTexture()->GetVolumeTexture());
        rd->GetDeviceContext().CopyResource(GetPrevDensityTex()->GetDevTexture()->GetVolumeTexture(),
            CTexture::s_ptexVolumetricFogDensity->GetDevTexture()->GetVolumeTexture());
#endif
    }


    {
        // set reprojection mode
        if (CRenderer::CV_r_VolumetricFogReprojectionMode != 0)
        {
            rp.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE5];
        }

        // accumulate in-scattering lighting along view ray.
        static CCryNameTSCRC shaderName("ReprojectVolumetricFog");
        bool valid = SD3DPostEffectsUtils::ShBeginPass(CShaderMan::s_shDeferredShading, shaderName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

#ifdef ENABLE_VOLFOG_TEX_FORMAT_RGBA16F
        D3DUnorderedAccessView* pUAVs[1] = {
            static_cast<D3DUnorderedAccessView*>(GetInscatterTex()->GetDeviceUAV()),
        };
        rd->m_DevMan.BindUAV(eHWSC_Compute, pUAVs, NULL, 0, 1);

        D3DShaderResourceView* pSRVs[3] = {
            m_InscatteringVolume->GetShaderResourceView(),
            (GetPrevInscatterTex()->GetShaderResourceView()),
            m_MaxDepth->GetShaderResourceView(),
        };
        rd->m_DevMan.BindSRV(eHWSC_Compute, pSRVs, 0, 3);
#else
        D3DUnorderedAccessView* pUAVs[2] = {
            static_cast<D3DUnorderedAccessView*>(GetInscatterTex()->GetDeviceUAV()),
            static_cast<D3DUnorderedAccessView*>(GetDensityTex()->GetDeviceUAV()),
        };
        rd->m_DevMan.BindUAV(eHWSC_Compute, pUAVs, NULL, 0, 2);

        D3DShaderResourceView* pSRVs[5] = {
            m_InscatteringVolume->GetShaderResourceView(),
            (GetPrevInscatterTex()->GetShaderResourceView()),
            CTexture::s_ptexVolumetricFogDensity->GetShaderResourceView(),
            (GetPrevDensityTex()->GetShaderResourceView()),
            m_MaxDepth->GetShaderResourceView(),
        };
        rd->m_DevMan.BindSRV(eHWSC_Compute, pSRVs, 0, 5);
#endif

        D3DSamplerState* pSamplers[2] = {
            (D3DSamplerState*)CTexture::s_TexStates[m_nTexStateTriLinear].m_pDeviceState,
            (D3DSamplerState*)CTexture::s_TexStates[m_nTexStatePoint].m_pDeviceState,
        };
        rd->m_DevMan.BindSampler(eHWSC_Compute, pSamplers, 0, 2);

        static CCryNameR paramScreenSize("ScreenSize");
        float fScreenWidth = (float)nScreenWidth;
        float fScreenHeight = (float)nScreenHeight;
        Vec4 vParamScreenSize(fScreenWidth, fScreenHeight, 1.0f / fScreenWidth, 1.0f / fScreenHeight);
        CShaderMan::s_shDeferredShading->FXSetCSFloat(paramScreenSize, &vParamScreenSize, 1);

        SD3DPostEffectsUtils::UpdateFrustumCorners();
        static CCryNameR paramFrustumTL("FrustumTL");
        Vec4 vParamFrustumTL(SD3DPostEffectsUtils::m_vLT.x, SD3DPostEffectsUtils::m_vLT.y, SD3DPostEffectsUtils::m_vLT.z, 0);
        CShaderMan::s_shDeferredShading->FXSetCSFloat(paramFrustumTL, &vParamFrustumTL, 1);
        static CCryNameR paramFrustumTR("FrustumTR");
        Vec4 vParamFrustumTR(SD3DPostEffectsUtils::m_vRT.x, SD3DPostEffectsUtils::m_vRT.y, SD3DPostEffectsUtils::m_vRT.z, 0);
        CShaderMan::s_shDeferredShading->FXSetCSFloat(paramFrustumTR, &vParamFrustumTR, 1);
        static CCryNameR paramFrustumBL("FrustumBL");
        Vec4 vParamFrustumBL(SD3DPostEffectsUtils::m_vLB.x, SD3DPostEffectsUtils::m_vLB.y, SD3DPostEffectsUtils::m_vLB.z, 0);
        CShaderMan::s_shDeferredShading->FXSetCSFloat(paramFrustumBL, &vParamFrustumBL, 1);

        float reprojectionFactor = max(0.0f, min(1.0f, CRenderer::CV_r_VolumetricFogReprojectionBlendFactor));

        const CameraViewParameters& rc = gcpRendD3D->GetViewParameters();
        float yfov, xfov, aspect, ndist, fdist;
        rc.GetPerspectiveParams(&yfov, &xfov, &aspect, &ndist, &fdist);
        static CCryNameR paramNameScreenInfo("ScreenInfo");
        Vec4 screenInfo(ndist, fdist, reprojectionFactor, (float)volumeDepth);
        CShaderMan::s_shDeferredShading->FXSetCSFloat(paramNameScreenInfo, &screenInfo, 1);

        static CCryNameR param1("PrevViewProjMatrix");
        Vec4* temp = (Vec4*)m_viewProj[max(m_tick - (int32)rd->GetActiveGPUCount(), 0) % MaxFrameNum].GetData();
        CShaderMan::s_shDeferredShading->FXSetCSFloat(param1, temp, 4);

        rd->FX_Commit();

        const uint32 tileSizeX = 4;
        const uint32 tileSizeY = 4;
        const uint32 tileSizeZ = 4;
        uint32 dispatchSizeX = (nScreenWidth / tileSizeX) + (nScreenWidth % tileSizeX > 0 ? 1 : 0);
        uint32 dispatchSizeY = (nScreenHeight / tileSizeY) + (nScreenHeight % tileSizeY > 0 ? 1 : 0);
        uint32 dispatchSizeZ = (volumeDepth / tileSizeZ) + (volumeDepth % tileSizeZ > 0 ? 1 : 0);
        rd->m_DevMan.Dispatch(dispatchSizeX, dispatchSizeY, dispatchSizeZ);

        SD3DPostEffectsUtils::ShEndPass();

        if (valid)
        {
            m_Cleared = (m_Cleared > 0) ? (m_Cleared - 1) : 0;
        }
    }


    D3DUnorderedAccessView* pUAVNull[4] = { NULL };
    rd->m_DevMan.BindUAV(eHWSC_Compute, pUAVNull, NULL, 0, 4);

    D3DShaderResourceView* pSRVNull[8] = { NULL };
    rd->m_DevMan.BindSRV(eHWSC_Compute, pSRVNull, 0, 5);

    D3DSamplerState* pSampNull[2] = { NULL };
    rd->m_DevMan.BindSampler(eHWSC_Compute, pSampNull, 0, 2);

    // restore state
    rp.m_FlagsShader_RT = prevShaderRtFlags;

    rd->FX_Commit();
    rd->m_DevMan.CommitDeviceStates();
}

CTexture* CVolumetricFog::GetInscatterTex() const
{
    return ((m_tick / gcpRendD3D->GetActiveGPUCount()) & 0x1) ? m_fogInscatteringVolume[0] : m_fogInscatteringVolume[1];
}

CTexture* CVolumetricFog::GetPrevInscatterTex() const
{
    return ((m_tick / gcpRendD3D->GetActiveGPUCount()) & 0x1) ? m_fogInscatteringVolume[1] : m_fogInscatteringVolume[0];
}

CTexture* CVolumetricFog::GetDensityTex() const
{
    return ((m_tick / gcpRendD3D->GetActiveGPUCount()) & 0x1) ? m_fogDensityVolume[0] : m_fogDensityVolume[1];
}

CTexture* CVolumetricFog::GetPrevDensityTex() const
{
    return ((m_tick / gcpRendD3D->GetActiveGPUCount()) & 0x1) ? m_fogDensityVolume[1] : m_fogDensityVolume[0];
}

void CVolumetricFog::RenderDownscaledDepth()
{
    //PROFILE_LABEL_SCOPE( "VOLUMETRIC_FOG_DOWNSCALED_DEPTH" );

    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    {
        static CCryNameTSCRC shaderName("StoreDownscaledMaxDepthHorizontal");
        SD3DPostEffectsUtils::ShBeginPass(CShaderMan::s_shDeferredShading, shaderName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

        D3DUnorderedAccessView* pUAVs[1] = {
            static_cast<D3DUnorderedAccessView*>(m_MaxDepthTemp->GetDeviceUAV()),
        };
        rd->m_DevMan.BindUAV(eHWSC_Compute, pUAVs, NULL, 0, 1);

        D3DShaderResourceView* pSRVs[1] = {
            CTexture::s_ptexZTargetScaled->GetShaderResourceView(),
        };
        rd->m_DevMan.BindSRV(eHWSC_Compute, pSRVs, 0, 1);

        D3DSamplerState* pSamplers[1] = {
            (D3DSamplerState*)CTexture::s_TexStates[m_nTexStatePoint].m_pDeviceState,
        };
        rd->m_DevMan.BindSampler(eHWSC_Compute, pSamplers, 0, 1);

        int nScreenWidth = m_MaxDepthTemp->GetWidth();
        int nScreenHeight = m_MaxDepthTemp->GetHeight();
        int nSrcTexWidth = CTexture::s_ptexZTargetScaled->GetWidth();
        int nSrcTexHeight = CTexture::s_ptexZTargetScaled->GetHeight();

        static CCryNameR paramDispatchParams("maxDepthDispatchParams");
        float destW = (float)nScreenWidth;
        float srcW = (float)nSrcTexWidth;
        Vec4 vParamDispatchParams((srcW / destW), (destW / srcW), 0.0f, 0.0f);
        CShaderMan::s_shDeferredShading->FXSetCSFloat(paramDispatchParams, &vParamDispatchParams, 1);

        static CCryNameR paramScreenSize("ScreenSize");
        float fScreenWidth = (float)nScreenWidth;
        float fScreenHeight = (float)nScreenHeight;
        Vec4 vParamScreenSize(fScreenWidth, fScreenHeight, 1.0f / fScreenWidth, 1.0f / fScreenHeight);
        CShaderMan::s_shDeferredShading->FXSetCSFloat(paramScreenSize, &vParamScreenSize, 1);

        rd->FX_Commit();

        const uint32 tileSizeX = 8;
        const uint32 tileSizeY = 8;
        uint32 dispatchSizeX = (nScreenWidth / tileSizeX) + (nScreenWidth % tileSizeX > 0 ? 1 : 0);
        uint32 dispatchSizeY = (nScreenHeight / tileSizeY) + (nScreenHeight % tileSizeY > 0 ? 1 : 0);
        rd->m_DevMan.Dispatch(dispatchSizeX, dispatchSizeY, 1);

        SD3DPostEffectsUtils::ShEndPass();
    }

    {
        D3DShaderResourceView* pNullViews[1] = { NULL };
        rd->m_DevMan.BindSRV(eHWSC_Compute, pNullViews, 0, 1);

        D3DUnorderedAccessView* pUAVNull[1] = { NULL };
        rd->m_DevMan.BindUAV(eHWSC_Compute, pUAVNull, NULL, 0, 1);

        rd->FX_Commit();
        rd->m_DevMan.CommitDeviceStates();
    }

    {
        static CCryNameTSCRC shaderName("StoreDownscaledMaxDepthVertical");
        SD3DPostEffectsUtils::ShBeginPass(CShaderMan::s_shDeferredShading, shaderName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

        D3DUnorderedAccessView* pUAVs[1] = {
            static_cast<D3DUnorderedAccessView*>(m_MaxDepth->GetDeviceUAV()),
        };
        rd->m_DevMan.BindUAV(eHWSC_Compute, pUAVs, NULL, 0, 1);

        D3DShaderResourceView* pSRVs[1] = {
            m_MaxDepthTemp->GetShaderResourceView(),
        };
        rd->m_DevMan.BindSRV(eHWSC_Compute, pSRVs, 0, 1);

        D3DSamplerState* pSamplers[1] = {
            (D3DSamplerState*)CTexture::s_TexStates[m_nTexStatePoint].m_pDeviceState,
        };
        rd->m_DevMan.BindSampler(eHWSC_Compute, pSamplers, 0, 1);

        int nScreenWidth = m_MaxDepth->GetWidth();
        int nScreenHeight = m_MaxDepth->GetHeight();
        int nSrcTexWidth = m_MaxDepthTemp->GetWidth();
        int nSrcTexHeight = m_MaxDepthTemp->GetHeight();

        static CCryNameR paramDispatchParams("maxDepthDispatchParams");
        float destW = (float)nScreenWidth;
        float srcW = (float)nSrcTexWidth;
        Vec4 vParamDispatchParams((srcW / destW), (destW / srcW), 0.0f, 0.0f);
        CShaderMan::s_shDeferredShading->FXSetCSFloat(paramDispatchParams, &vParamDispatchParams, 1);

        static CCryNameR paramScreenSize("ScreenSize");
        float fScreenWidth = (float)nScreenWidth;
        float fScreenHeight = (float)nScreenHeight;
        Vec4 vParamScreenSize(fScreenWidth, fScreenHeight, 1.0f / fScreenWidth, 1.0f / fScreenHeight);
        CShaderMan::s_shDeferredShading->FXSetCSFloat(paramScreenSize, &vParamScreenSize, 1);

        rd->FX_Commit();

        const uint32 tileSizeX = 8;
        const uint32 tileSizeY = 8;
        uint32 dispatchSizeX = (nScreenWidth / tileSizeX) + (nScreenWidth % tileSizeX > 0 ? 1 : 0);
        uint32 dispatchSizeY = (nScreenHeight / tileSizeY) + (nScreenHeight % tileSizeY > 0 ? 1 : 0);
        rd->m_DevMan.Dispatch(dispatchSizeX, dispatchSizeY, 1);

        SD3DPostEffectsUtils::ShEndPass();
    }

    {
        D3DUnorderedAccessView* pUAVNull[1] = { NULL };
        rd->m_DevMan.BindUAV(eHWSC_Compute, pUAVNull, NULL, 0, 1);

        D3DShaderResourceView* pNullViews[1] = { NULL };
        rd->m_DevMan.BindSRV(eHWSC_Compute, pNullViews, 0, 1);

        D3DSamplerState* pSampNull[1] = { NULL };
        rd->m_DevMan.BindSampler(eHWSC_Compute, pSampNull, 0, 1);
    }
}

void CVolumetricFog::ClearVolumeStencil()
{
    if (!IsViable())
    {
        return;
    }

    PROFILE_LABEL_SCOPE("VOLUMETRIC_FOG_CLEAR_VOLUME_STENCIL");
    gcpRendD3D->FX_ClearTarget((D3DDepthSurface*)CTexture::s_ptexVolumetricClipVolumeStencil->GetDeviceDepthStencilSurf(), CLEAR_STENCIL, Clr_Unused.r, 0);
}

void CVolumetricFog::RenderClipVolumeToVolumeStencil(int nClipAreaReservedStencilBit)
{
    if (!IsViable())
    {
        return;
    }

    PROFILE_LABEL_SCOPE("VOLUMETRIC_FOG_CLIPVOLUMES TO STENCIL");

    CD3D9Renderer* const __restrict rd = gcpRendD3D;
    SRenderPipeline& rp(rd->m_RP);
    const bool bReverseDepth = (rp.m_TI[rp.m_nProcessThreadID].m_PersFlags & RBPF_REVERSE_DEPTH) != 0;

    int oldX, oldY;
    int oldWidth, oldHeight;
    rd->GetViewport(&oldX, &oldY, &oldWidth, &oldHeight);

    // FOB_POINT_SPRITE prevents CD3D9Renderer::FX_SetVertexDeclaration function working correctly.
    uint64 objFlags = rp.m_ObjFlags;
    rp.m_ObjFlags &= ~FOB_POINT_SPRITE;

    int targetWidth = CTexture::s_ptexVolumetricClipVolumeStencil->GetWidth();
    int targetHeight = CTexture::s_ptexVolumetricClipVolumeStencil->GetHeight();
    rd->RT_SetViewport(0, 0, targetWidth, targetHeight);

    SDepthTexture D3dDepthSurface;
    D3dDepthSurface.nWidth = targetWidth;
    D3dDepthSurface.nHeight = targetHeight;
    D3dDepthSurface.pSurf = nullptr;
    D3dDepthSurface.pTex = nullptr;
    D3dDepthSurface.pTarget = CTexture::s_ptexVolumetricClipVolumeStencil->GetDevTexture()->Get2DTexture();

    int maxDepthCount = m_InscatteringVolume->GetDepth();
    float a = 0.0f;
    float b = 0.0f;
    bool valid = true;
    float d = (rp.m_TI[rp.m_nProcessThreadID].m_PersFlags & RBPF_REVERSE_DEPTH) ? 1.0f : 0.0f;
    float raymarchDistance = gRenDev->m_cEF.m_PF.m_VolumetricFogDistanceParams.w;

    if (m_Destroyed > 0)
    {
        float fFar = rd->GetViewParameters().fFar;
        float fNear = rd->GetViewParameters().fNear;
        float invMaxIndexMinusOne = 1.0f / (static_cast<f32>(maxDepthCount) - 1.0f);
        a = fFar / (fFar - fNear);
        b = fNear * -a;

        // store values to observe the change.
        m_ReverseDepthMode = CRenderer::CV_r_ReverseDepth;
        m_raymarchDistance = raymarchDistance;

        rd->FX_ClearTarget((D3DDepthSurface*)CTexture::s_ptexVolumetricClipVolumeStencil->GetDeviceDepthStencilSurf(), CLEAR_ZBUFFER, Clr_FarPlane_R.r, 0);
    }

    for (int i = 0; i < maxDepthCount; ++i)
    {
        // set separate DSV.
        D3dDepthSurface.pSurf = (D3DDepthSurface*)m_ClipVolumeDSVArray[i];
        rd->FX_PushRenderTarget(0, (CTexture*)NULL, &D3dDepthSurface);

        // clear depth when it's needed.
        if (m_Destroyed > 0)
        {
            // write jittering depth.
            static CCryNameTSCRC shaderName("StoreJitteringDepthToClipVolumeDepth");
            bool result = GetUtils().ShBeginPass(CShaderMan::s_shDeferredShading, shaderName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

            valid = valid && result;

            rd->FX_SetState(GS_DEPTHWRITE);

            static CCryNameR paramDepth("ParamDepth");
            Vec4 vParamDepth((float)i, b, a, d);
            CShaderMan::s_shDeferredShading->FXSetPSFloat(paramDepth, &vParamDepth, 1);

            GetUtils().DrawFullScreenTri(targetWidth, targetHeight);

            GetUtils().ShEndPass();
        }

        CDeferredShading& pDS = CDeferredShading::Instance();
        pDS.RenderClipVolumesToStencil(nClipAreaReservedStencilBit);

        rd->FX_PopRenderTarget(0);
    }

    if (valid)
    {
        m_Destroyed = (m_Destroyed > 0) ? (m_Destroyed - 1) : 0;

        if (m_ReverseDepthMode != CRenderer::CV_r_ReverseDepth
            || (0.2f < abs(m_raymarchDistance - raymarchDistance)))
        {
            // rewrite the depth if needed.
            m_Destroyed = MaxFrameNum;
        }
    }

    rd->RT_SetViewport(oldX, oldY, oldWidth, oldHeight);
    rp.m_ObjFlags = objFlags;
}

void CVolumetricFog::InjectExponentialHeightFog()
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;
    SRenderPipeline& rp(rd->m_RP);
    const int nThreadID = rp.m_nProcessThreadID;

    // store state
    const int32 prevState = rp.m_CurState;
    const uint32 prevStateAnd = rp.m_StateAnd;
    const uint32 prevStateOr = rp.m_StateOr;
    const uint64 prevShaderRtFlags = rp.m_FlagsShader_RT;
    const int prevPersFlags = rp.m_TI[nThreadID].m_PersFlags;


    // inject atmospheric fog into density volume texture.
    {
        const uint32 nScreenWidth = CTexture::s_ptexVolumetricFogDensity->GetWidth();
        const uint32 nScreenHeight = CTexture::s_ptexVolumetricFogDensity->GetHeight();
        const uint32 volumeDepth = CTexture::s_ptexVolumetricFogDensity->GetDepth();

        //PROFILE_LABEL_SCOPE( "INJECT_ATMOSPHERIC_FOG" );

        D3DShaderResourceView* pNullViews[8] = { NULL };
        rd->m_DevMan.BindSRV(eHWSC_Compute, pNullViews, 0, 8);

        D3DUnorderedAccessView* pUAVNull[4] = { NULL };
        rd->m_DevMan.BindUAV(eHWSC_Compute, pUAVNull, NULL, 0, 4);

        rd->FX_Commit();
        rd->m_DevMan.CommitDeviceStates();


        rd->GetTiledShading().BindForwardShadingResources(NULL, eHWSC_Compute);

        CShader* pShader = CShaderMan::s_shDeferredShading;

        static CCryNameTSCRC shaderName("InjectExponentialHeightFog");
        SD3DPostEffectsUtils::ShBeginPass(CShaderMan::s_shDeferredShading, shaderName, FEF_DONTSETSTATES);

        D3DUnorderedAccessView* pUAVs[1] = {
            static_cast<D3DUnorderedAccessView*>(CTexture::s_ptexVolumetricFogDensity->GetDeviceUAV()),
        };
        rd->m_DevMan.BindUAV(eHWSC_Compute, pUAVs, NULL, 0, 1);

        D3DSamplerState* pSamplers[1] = {
            (D3DSamplerState*)CTexture::s_TexStates[m_nTexStateTriLinear].m_pDeviceState,
        };
        rd->m_DevMan.BindSampler(eHWSC_Compute, pSamplers, 0, 1);

        static CCryNameR paramScreenSize("ScreenSize");
        float fScreenWidth = (float)nScreenWidth;
        float fScreenHeight = (float)nScreenHeight;
        Vec4 vParamScreenSize(fScreenWidth, fScreenHeight, 1.0f / fScreenWidth, 1.0f / fScreenHeight);
        pShader->FXSetCSFloat(paramScreenSize, &vParamScreenSize, 1);

        SD3DPostEffectsUtils::UpdateFrustumCorners();
        static CCryNameR paramFrustumTL("FrustumTL");
        Vec4 vParamFrustumTL(SD3DPostEffectsUtils::m_vLT.x, SD3DPostEffectsUtils::m_vLT.y, SD3DPostEffectsUtils::m_vLT.z, 0);
        pShader->FXSetCSFloat(paramFrustumTL, &vParamFrustumTL, 1);
        static CCryNameR paramFrustumTR("FrustumTR");
        Vec4 vParamFrustumTR(SD3DPostEffectsUtils::m_vRT.x, SD3DPostEffectsUtils::m_vRT.y, SD3DPostEffectsUtils::m_vRT.z, 0);
        pShader->FXSetCSFloat(paramFrustumTR, &vParamFrustumTR, 1);
        static CCryNameR paramFrustumBL("FrustumBL");
        Vec4 vParamFrustumBL(SD3DPostEffectsUtils::m_vLB.x, SD3DPostEffectsUtils::m_vLB.y, SD3DPostEffectsUtils::m_vLB.z, 0);
        pShader->FXSetCSFloat(paramFrustumBL, &vParamFrustumBL, 1);

        static CCryNameR paramInjectExponentialHeightFogParams("InjectExponentialHeightFogParams");
        Vec4 vInjectExponentialHeightFogParams((float)volumeDepth, 0.0f, 0.0f, 0.0f);
        pShader->FXSetCSFloat(paramInjectExponentialHeightFogParams, &vInjectExponentialHeightFogParams, 1);

        rd->FX_Commit();

        const uint32 tileSizeX = 4;
        const uint32 tileSizeY = 4;
        const uint32 tileSizeZ = 4;
        uint32 dispatchSizeX = (nScreenWidth / tileSizeX) + (nScreenWidth % tileSizeX > 0 ? 1 : 0);
        uint32 dispatchSizeY = (nScreenHeight / tileSizeY) + (nScreenHeight % tileSizeY > 0 ? 1 : 0);
        uint32 dispatchSizeZ = (volumeDepth / tileSizeZ) + (volumeDepth % tileSizeZ > 0 ? 1 : 0);
        rd->m_DevMan.Dispatch(dispatchSizeX, dispatchSizeY, dispatchSizeZ);

        SD3DPostEffectsUtils::ShEndPass();

        rd->GetTiledShading().UnbindForwardShadingResources(eHWSC_Compute);
    }


    D3DUnorderedAccessView* pUAVNull4[4] = { NULL };
    rd->m_DevMan.BindUAV(eHWSC_Compute, pUAVNull4, NULL, 0, 4);

    D3DShaderResourceView* pNullViews[8] = { NULL };
    rd->m_DevMan.BindSRV(eHWSC_Compute, pNullViews, 0, 8);

    D3DSamplerState* pSampNull[2] = { NULL };
    rd->m_DevMan.BindSampler(eHWSC_Compute, pSampNull, 0, 2);

    // restore state
    rp.m_StateAnd = prevStateAnd;
    rp.m_StateOr = prevStateOr;
    rd->SetState(prevState);
    rp.m_TI[nThreadID].m_PersFlags = prevPersFlags;
    rp.m_FlagsShader_RT = prevShaderRtFlags;

    rd->FX_Commit();
    rd->m_DevMan.CommitDeviceStates();
}

void CVolumetricFog::PrepareFogVolumeList()
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    int nThreadID = rd->m_RP.m_nProcessThreadID;
    int nRecurseLevel = SRendItem::m_RecurseLevel[nThreadID];

    // Prepare view matrix with flipped z-axis
    Matrix44A matView = rd->m_ViewMatrix;
    matView.m02 *= -1;
    matView.m12 *= -1;
    matView.m22 *= -1;
    matView.m32 *= -1;

    vfInternal::SFogVolumeCullInfo cullInfoArray[vfInternal::MaxNumFogVolumes];
    vfInternal::SFogVolumeInjectInfo injectInfoArray[vfInternal::MaxNumFogVolumes];
    uint32 numFogVol = 0;

    Vec3 volumetricFogRaymarchEnd(0.0f, 0.0f, 0.0f);
    gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLFOG2_CTRL_PARAMS, volumetricFogRaymarchEnd);
    Vec3 cameraFront = rd->GetViewParameters().vZ;
    cameraFront.Normalize();

    Vec3 worldViewPos = rd->GetViewParameters().vOrigin;
    AABB aabbInObj(1.0f);

    for (uint32 type = 0; type < MaxNumFogVolumeType; ++type)
    {
        TArray<SFogVolumeInfo>& srcArray = m_fogVolumeInfoArray[nThreadID][nRecurseLevel][type];
        uint num = srcArray.Num();
        for (uint32 i = 0; i < num; ++i)
        {
            SFogVolumeInfo& fvol = srcArray[i];

            // calculate depth bounds of FogVolume.
            // reusing light depth bounds code from CDeferredShading::GetLightDepthBounds().
            // This is not optimal for a box.
            Matrix34 temp = fvol.m_matWSInv.GetInverted();
            AABB aabbInWS = AABB::CreateTransformedAABB(temp, aabbInObj);
            float fRadius = aabbInWS.GetRadius();
            Vec3 pBounds = cameraFront * fRadius;
            Vec3 pMin = fvol.m_center + pBounds;
            float fMinW = rd->GetViewParameters().WorldToCamZ(pMin);
            fMinW = max(-fMinW, 0.000001f);

            // not needed to be injected when FogVolume is out of volume texture.
            if (fMinW > volumetricFogRaymarchEnd.x)
            {
                continue;
            }

            vfInternal::SFogVolumeCullInfo& cullInfo = cullInfoArray[numFogVol];
            vfInternal::SFogVolumeInjectInfo& injectInfo = injectInfoArray[numFogVol];

            Vec4 posVS = Vec4(fvol.m_center, 1) * matView;
            cullInfo.posRad = Vec4(posVS.x, posVS.y, posVS.z, fRadius);

            Vec4 u0 = Vec4(temp.GetColumn0().GetNormalized(), 0) * matView;
            Vec4 u1 = Vec4(temp.GetColumn1().GetNormalized(), 0) * matView;
            Vec4 u2 = Vec4(temp.GetColumn2().GetNormalized(), 0) * matView;
            Vec3 size = fvol.m_scale.CompMul(fvol.m_localAABB.GetSize() * 0.5f);
            cullInfo.volumeParams0 = Vec4(u0.x, u0.y, u0.z, size.x);
            cullInfo.volumeParams1 = Vec4(u1.x, u1.y, u1.z, size.y);
            cullInfo.volumeParams2 = Vec4(u2.x, u2.y, u2.z, size.z);

            uint32 nData = fvol.m_stencilRef + 1;// first ref value is reserved, see CDeferredShading::PrepareClipVolumeData function.
            injectInfo.miscFlag = fvol.m_volumeType | ((nData & 0xFF) << 1) | (fvol.m_affectsThisAreaOnly << 9);

            injectInfo.fogColor.x = fvol.m_fogColor.r;
            injectInfo.fogColor.y = fvol.m_fogColor.g;
            injectInfo.fogColor.z = fvol.m_fogColor.b;

            float globalDensity = fvol.m_globalDensity * 0.1f;// scale density to volumetric fog.
            injectInfo.globalDensity = globalDensity;

            injectInfo.fogVolumePos = fvol.m_center;

            injectInfo.heightFalloffBasePoint = fvol.m_heightFallOffBasePoint;

            float softEdgeLerp = (fvol.m_softEdgesLerp.x > 0.0f) ? fvol.m_softEdgesLerp.x : 0.0001f;
            injectInfo.invSoftEdgeLerp = 1.0f / softEdgeLerp;

            const Vec3 cHeightFallOffDirScaledVec(fvol.m_heightFallOffDirScaled * 0.015625f);  // scale fall off ramp to volumetric fog.
            injectInfo.heightFallOffDirScaled = cHeightFallOffDirScaledVec;

            injectInfo.densityOffset = fvol.m_densityOffset;

            float rampDist = fvol.m_rampParams.y - fvol.m_rampParams.x;
            rampDist = rampDist < 0.1f ? 0.1f : rampDist;
            float invRampDist = 1.0f / rampDist;
            const Vec4 cRampParams(invRampDist, -fvol.m_rampParams.x * invRampDist, fvol.m_rampParams.z, -fvol.m_rampParams.z + 1.0f);
            injectInfo.rampParams = cRampParams;

            injectInfo.windOffset = fvol.m_windOffset;

            injectInfo.noiseElapsedTime = fvol.m_noiseElapsedTime;

            injectInfo.noiseSpatialFrequency = fvol.m_noiseFreq;

            const float normalizeFactor = (1.0f / (1.0f + 0.5f));
            injectInfo.noiseScale = fvol.m_noiseScale * normalizeFactor;

            injectInfo.eyePosInOS = fvol.m_eyePosInOS;

            injectInfo.noiseOffset = fvol.m_noiseOffset;

            injectInfo.worldToObjMatrix = fvol.m_matWSInv;

            ++numFogVol;
        }
    }

    m_numFogVolumes = numFogVol;

    m_fogVolumeCullInfoBuf.UpdateBufferContent(cullInfoArray, sizeof(vfInternal::SFogVolumeCullInfo) * vfInternal::MaxNumFogVolumes);
    m_fogVolumeInjectInfoBuf.UpdateBufferContent(injectInfoArray, sizeof(vfInternal::SFogVolumeInjectInfo) * vfInternal::MaxNumFogVolumes);
}

void CVolumetricFog::InjectFogDensity()
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;
    SRenderPipeline& rp(rd->m_RP);
    const int nThreadID = rp.m_nProcessThreadID;

    // store state
    const int32 prevState = rp.m_CurState;
    const uint32 prevStateAnd = rp.m_StateAnd;
    const uint32 prevStateOr = rp.m_StateOr;
    const uint64 prevShaderRtFlags = rp.m_FlagsShader_RT;
    const int prevPersFlags = rp.m_TI[nThreadID].m_PersFlags;

    {
        D3DShaderResourceView* pNullViews[8] = { NULL };
        rd->m_DevMan.BindSRV(eHWSC_Compute, pNullViews, 0, 8);
        rd->m_DevMan.BindSRV(eHWSC_Compute, pNullViews, 8, 8);

        D3DUnorderedAccessView* pUAVNull[4] = { NULL };
        rd->m_DevMan.BindUAV(eHWSC_Compute, pUAVNull, NULL, 0, 4);

        rd->FX_Commit();
        rd->m_DevMan.CommitDeviceStates();
    }

    // inject the density of atmospheric fog and FogVolume into volume texture.
    {
        rd->GetTiledShading().BindForwardShadingResources(NULL, eHWSC_Compute);

        const uint32 nScreenWidth = CTexture::s_ptexVolumetricFogDensity->GetWidth();
        const uint32 nScreenHeight = CTexture::s_ptexVolumetricFogDensity->GetHeight();
        const uint32 volumeDepth = CTexture::s_ptexVolumetricFogDensity->GetDepth();

        //PROFILE_LABEL_SCOPE( "INJECT_FOG_DENSITY" );

        static CCryNameTSCRC shaderName("InjectFogDensity");
        SD3DPostEffectsUtils::ShBeginPass(CShaderMan::s_shDeferredShading, shaderName, FEF_DONTSETSTATES);

        D3DUnorderedAccessView* pUAVs[2] = {
            (CTexture::s_ptexVolumetricFogDensity->GetDeviceUAV()),
            (CTexture::s_ptexVolumetricFogDensityColor->GetDeviceUAV()),
        };
        rd->m_DevMan.BindUAV(eHWSC_Compute, pUAVs, NULL, 0, 2);

        D3DShaderResourceView* pSRVs[3] = {
            m_fogVolumeInjectInfoBuf.GetShaderResourceView(),
            m_fogVolumeCullInfoBuf.GetShaderResourceView(),
            m_MaxDepth->GetShaderResourceView(),
        };
        rd->m_DevMan.BindSRV(eHWSC_Compute, pSRVs, 8, 3);

        //D3DSamplerState *pSamplers[1] = {
        //  (D3DSamplerState *)CTexture::s_TexStates[m_nTexStateTriLinear].m_pDeviceState,
        //  //(D3DSamplerState *)CTexture::s_TexStates[m_nTexStatePoint].m_pDeviceState,
        //};
        //rd->m_DevMan.BindSampler( eHWSC_Compute, pSamplers, 0, 1 );

        static CCryNameR paramScreenSize("ScreenSize");
        float fScreenWidth = (float)nScreenWidth;
        float fScreenHeight = (float)nScreenHeight;
        Vec4 vParamScreenSize(fScreenWidth, fScreenHeight, 1.0f / fScreenWidth, 1.0f / fScreenHeight);
        CShaderMan::s_shDeferredShading->FXSetCSFloat(paramScreenSize, &vParamScreenSize, 1);

        SD3DPostEffectsUtils::UpdateFrustumCorners();
        static CCryNameR paramFrustumTL("FrustumTL");
        Vec4 vParamFrustumTL(SD3DPostEffectsUtils::m_vLT.x, SD3DPostEffectsUtils::m_vLT.y, SD3DPostEffectsUtils::m_vLT.z, 0);
        CShaderMan::s_shDeferredShading->FXSetCSFloat(paramFrustumTL, &vParamFrustumTL, 1);
        static CCryNameR paramFrustumTR("FrustumTR");
        Vec4 vParamFrustumTR(SD3DPostEffectsUtils::m_vRT.x, SD3DPostEffectsUtils::m_vRT.y, SD3DPostEffectsUtils::m_vRT.z, 0);
        CShaderMan::s_shDeferredShading->FXSetCSFloat(paramFrustumTR, &vParamFrustumTR, 1);
        static CCryNameR paramFrustumBL("FrustumBL");
        Vec4 vParamFrustumBL(SD3DPostEffectsUtils::m_vLB.x, SD3DPostEffectsUtils::m_vLB.y, SD3DPostEffectsUtils::m_vLB.z, 0);
        CShaderMan::s_shDeferredShading->FXSetCSFloat(paramFrustumBL, &vParamFrustumBL, 1);

        float octave = (CRenderer::CV_r_VolumetricFogReprojectionBlendFactor > 0.0f) ? -1.7f : 0.0f;
        static CCryNameR paramInjectExponentialHeightFogParams("InjectFogDensityParams");
        Vec4 vInjectExponentialHeightFogParams((float)volumeDepth, (float)m_numFogVolumes, octave, 0.0f);
        CShaderMan::s_shDeferredShading->FXSetCSFloat(paramInjectExponentialHeightFogParams, &vInjectExponentialHeightFogParams, 1);

        static CCryNameR paramProj("ProjParams");
        Vec4 vParamProj(rd->m_ProjMatrix.m00, rd->m_ProjMatrix.m11, rd->m_ProjMatrix.m20, rd->m_ProjMatrix.m21);
        CShaderMan::s_shDeferredShading->FXSetCSFloat(paramProj, &vParamProj, 1);

        const uint32 tileSizeX = 4;
        const uint32 tileSizeY = 4;
        const uint32 tileSizeZ = 4;
        uint32 dispatchSizeX = (nScreenWidth / tileSizeX) + (nScreenWidth % tileSizeX > 0 ? 1 : 0);
        uint32 dispatchSizeY = (nScreenHeight / tileSizeY) + (nScreenHeight % tileSizeY > 0 ? 1 : 0);
        uint32 dispatchSizeZ = (volumeDepth / tileSizeZ) + (volumeDepth % tileSizeZ > 0 ? 1 : 0);

        rd->FX_Commit();

        rd->m_DevMan.Dispatch(dispatchSizeX, dispatchSizeY, dispatchSizeZ);

        SD3DPostEffectsUtils::ShEndPass();

        rd->GetTiledShading().UnbindForwardShadingResources(eHWSC_Compute);
    }


    D3DUnorderedAccessView* pUAVNull4[4] = { NULL };
    rd->m_DevMan.BindUAV(eHWSC_Compute, pUAVNull4, NULL, 0, 4);

    D3DShaderResourceView* pNullViews[8] = { NULL };
    rd->m_DevMan.BindSRV(eHWSC_Compute, pNullViews, 0, 8);
    rd->m_DevMan.BindSRV(eHWSC_Compute, pNullViews, 8, 8);

    //D3DSamplerState* pSampNull[2] = { NULL };
    //rd->m_DevMan.BindSampler( eHWSC_Compute, pSampNull, 0, 2 );

    // restore state
    rp.m_StateAnd = prevStateAnd;
    rp.m_StateOr = prevStateOr;
    rd->SetState(prevState);
    rp.m_TI[nThreadID].m_PersFlags = prevPersFlags;
    rp.m_FlagsShader_RT = prevShaderRtFlags;

    rd->FX_Commit();
    rd->m_DevMan.CommitDeviceStates();
}

void CVolumetricFog::RenderDownscaledShadowmap()
{
    if (!gcpRendD3D->m_bShadowsEnabled)
    {
        return;
    }

    if (CRenderer::CV_r_VolumetricFogDownscaledSunShadow == 0)
    {
        return;
    }

    //PROFILE_LABEL_SCOPE("DOWNSCALE_SHADOWMAP");

    CD3D9Renderer* const __restrict rd = gcpRendD3D;
    SRenderPipeline& rp(rd->m_RP);

    // store state
    int oldX, oldY;
    int oldWidth, oldHeight;
    rd->GetViewport(&oldX, &oldY, &oldWidth, &oldHeight);
    const uint64 prevShaderRtFlags = rp.m_FlagsShader_RT;

    rd->FX_SetupShadowsForFog();

    SDepthTexture D3dDepthSurface;
    static const int texStatePoint = CTexture::GetTexState(STexState(FILTER_POINT, true));
    int count = (CRenderer::CV_r_VolumetricFogDownscaledSunShadow == 1) ? 2 : 3;

    for (int i = 0; i < count; ++i)
    {
        {
            const uint64 lv0  = g_HWSR_MaskBit[HWSR_LIGHTVOLUME0];
            const uint64 lv1 = g_HWSR_MaskBit[HWSR_LIGHTVOLUME1];
            rp.m_FlagsShader_RT &= ~(lv0 | lv1);
            switch (i)
            {
            case 0:
                rp.m_FlagsShader_RT |= lv0;
                break;
            case 1:
                rp.m_FlagsShader_RT |= lv1;
                break;
            case 2:
                rp.m_FlagsShader_RT |= lv0 | lv1;
                break;
            default:
                break;
            }

            CTexture* target = NULL;
            if (CRenderer::CV_r_VolumetricFogDownscaledSunShadowRatio == 0)
            {
                target = m_downscaledShadow[i];
            }
            else
            {
                target = m_downscaledShadowTemp;
            }

            int targetWidth = target->GetWidth();
            int targetHeight = target->GetHeight();

            D3dDepthSurface.nWidth = targetWidth;
            D3dDepthSurface.nHeight = targetHeight;
            D3dDepthSurface.pTex = target;
            D3dDepthSurface.pSurf = (D3DDepthSurface*)target->GetDeviceDepthStencilSurf();
            D3dDepthSurface.pTarget = target->GetDevTexture()->Get2DTexture();
            rd->FX_PushRenderTarget(0, (CTexture*)NULL, &D3dDepthSurface);
            rd->FX_SetActiveRenderTargets();

            rd->FX_SetState(GS_COLMASK_NONE | GS_DEPTHWRITE | GS_DEPTHFUNC_NOTEQUAL);

            static CCryNameTSCRC shaderName("RenderDownscaledShadowMap");
            GetUtils().ShBeginPass(CShaderMan::s_shDeferredShading, shaderName, FEF_DONTSETSTATES);

            D3DSamplerState* pSamplers[1] = {
                (D3DSamplerState*)CTexture::s_TexStates[m_nTexStatePoint].m_pDeviceState,
            };
            rd->m_DevMan.BindSampler(eHWSC_Pixel, pSamplers, 0, 1);

            GetUtils().DrawFullScreenTri(targetWidth, targetHeight);

            GetUtils().ShEndPass();

            rd->FX_PopRenderTarget(0);

            rp.m_FlagsShader_RT = prevShaderRtFlags;
        }

        // additional downscale pass
        if (CRenderer::CV_r_VolumetricFogDownscaledSunShadowRatio != 0)
        {
            CTexture* source = m_downscaledShadowTemp;
            CTexture* target = m_downscaledShadow[i];

            int targetWidth = target->GetWidth();
            int targetHeight = target->GetHeight();

            D3dDepthSurface.nWidth = targetWidth;
            D3dDepthSurface.nHeight = targetHeight;
            D3dDepthSurface.pTex = target;
            D3dDepthSurface.pSurf = target->GetDeviceDepthStencilSurf();
            D3dDepthSurface.pTarget = target->GetDevTexture()->Get2DTexture();

            rd->FX_PushRenderTarget(0, (CTexture*)NULL, &D3dDepthSurface);
            rd->FX_SetActiveRenderTargets();

            rd->FX_SetState(GS_COLMASK_NONE | GS_DEPTHWRITE | GS_DEPTHFUNC_NOTEQUAL);

            static CCryNameTSCRC shaderName0("DownscaleShadowMap2");
            static CCryNameTSCRC shaderName1("DownscaleShadowMap4");
            if (CRenderer::CV_r_VolumetricFogDownscaledSunShadowRatio == 1)
            {
                GetUtils().ShBeginPass(CShaderMan::s_shDeferredShading, shaderName0, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
            }
            else
            {
                GetUtils().ShBeginPass(CShaderMan::s_shDeferredShading, shaderName1, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
            }

            source->Apply(0, texStatePoint, EFTT_UNKNOWN, -1, SResourceView::DefaultView);

            GetUtils().DrawFullScreenTri(targetWidth, targetHeight);

            GetUtils().ShEndPass();

            rd->FX_PopRenderTarget(0);
        }
    }

    // restore pipeline state.
    rd->RT_SetViewport(oldX, oldY, oldWidth, oldHeight);
    rp.m_FlagsShader_RT = prevShaderRtFlags;
    rd->FX_Commit();
}

void CVolumetricFog::SetupStaticShadowMap(int nSlot) const
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;
    SRenderPipeline& rp(rd->m_RP);
    const int nThreadID = rp.m_nProcessThreadID;

    const int nSunFrustumID = 0;
    const int nStartIdx = SRendItem::m_StartFrust[nThreadID][nSunFrustumID];
    const int nEndIdx = SRendItem::m_EndFrust[nThreadID][nSunFrustumID];

    for (int i = nStartIdx; i < nEndIdx; ++i)
    {
        ShadowMapFrustum& fr = rp.m_SMFrustums[nThreadID][nSunFrustumID][i];
        if (fr.m_eFrustumType == ShadowMapFrustum::e_GsmCached)
        {
            rd->ConfigShadowTexgen(nSlot, &fr, -1, true);
            break;
        }
    }
}

