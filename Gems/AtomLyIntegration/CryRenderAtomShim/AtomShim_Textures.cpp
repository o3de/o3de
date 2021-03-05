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

// Description : NULL device specific texture manager implementation.

#include "CryRenderOther_precompiled.h"
#include "AtomShim_Renderer.h"
#include <Atom/RHI/Factory.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>

//=================================================================================

///////////////////////////////////////////////////////////////////////////////////


void CAtomShimRenderer::MakeSprite(IDynTexture*& rTexturePtr, [[maybe_unused]] float _fSpriteDistance, [[maybe_unused]] int nTexSize, [[maybe_unused]] float angle, [[maybe_unused]] float angle2, [[maybe_unused]] IStatObj* pStatObj, [[maybe_unused]] const float fBrightnessMultiplier, [[maybe_unused]] SRendParams& rParms)
{
    rTexturePtr = NULL;
}

int CAtomShimRenderer::GenerateAlphaGlowTexture([[maybe_unused]] float k)
{
    return 0;
}

bool CAtomShimRenderer::EF_SetLightHole([[maybe_unused]] Vec3 vPos, [[maybe_unused]] Vec3 vNormal, [[maybe_unused]] int idTex, [[maybe_unused]] float fScale, [[maybe_unused]] bool bAdditive)
{
    return false;
}

bool CAtomShimRenderer::EF_PrecacheResource([[maybe_unused]] ITexture* pTP, [[maybe_unused]] float fDist, [[maybe_unused]] float fTimeToReady, [[maybe_unused]] int Flags, [[maybe_unused]] int nUpdateId, [[maybe_unused]] int nCounter)
{
    return false;
}

bool CTexture::RenderEnvironmentCMHDR([[maybe_unused]] int size, [[maybe_unused]] Vec3& Pos, [[maybe_unused]] TArray<unsigned short>& vecData)
{
    return true;
}

void CTexture::Apply([[maybe_unused]] int nTUnit, [[maybe_unused]] int nState, [[maybe_unused]] int nTMatSlot, [[maybe_unused]] int nSUnit, [[maybe_unused]] SResourceView::KeyType nResViewKey, [[maybe_unused]] EHWShaderClass eSHClass)
{
}

#if defined(TEXTURE_GET_SYSTEM_COPY_SUPPORT)
byte* CTexture::Convert([[maybe_unused]] const byte* pSrc, [[maybe_unused]] int nWidth, [[maybe_unused]] int nHeight, [[maybe_unused]] int nMips, [[maybe_unused]] ETEX_Format eTFSrc, [[maybe_unused]] ETEX_Format eTFDst, [[maybe_unused]] int& nOutSize, [[maybe_unused]] bool bLinear)
{
    return NULL;
}
#endif

void CTexture::ReleaseDeviceTexture([[maybe_unused]] bool bKeepLastMips, [[maybe_unused]] bool bFromUnload)
{
}

bool CTexture::Clear([[maybe_unused]] const ColorF& color)
{
    return true;
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

bool CTexture::CreateDeviceTexture([[maybe_unused]] const byte* pData[6])
{
    return true;
}

void* CTexture::CreateDeviceResourceView([[maybe_unused]] const SResourceView& rv)
{
    return NULL;
}

ETEX_Format CTexture::ClosestFormatSupported(ETEX_Format eTFDst)
{
    return eTFDst;
}

bool CTexture::SetFilterMode(int nFilter)
{
    return s_sDefState.SetFilterMode(nFilter);
}

bool CTexture::CreateRenderTarget([[maybe_unused]] ETEX_Format eTF, [[maybe_unused]] const ColorF& cClear)
{
    return true;
}

bool CTexture::SetClampingMode(int nAddressU, int nAddressV, int nAddressW)
{
    return s_sDefState.SetClampMode(nAddressU, nAddressV, nAddressW);
}

void CTexture::UpdateTexStates()
{
}

void CTexture::GenerateCachedShadowMaps()
{
}

void CTexture::Readback([[maybe_unused]] AZ::u32 subresourceIndex, StagingHook callback)
{
}

//======================================================================================

void SEnvTexture::Release()
{
}

void SEnvTexture::RT_SetMatrix(void)
{
}

bool SDynTexture::RestoreRT([[maybe_unused]] int nRT, [[maybe_unused]] bool bPop)
{
    return true;
}

bool SDynTexture::ClearRT()
{
    return true;
}

bool SDynTexture2::ClearRT()
{
    return true;
}

bool SDynTexture::SetRT([[maybe_unused]] int nRT, [[maybe_unused]] bool bPush, [[maybe_unused]] SDepthTexture* pDepthSurf, [[maybe_unused]] bool bScreenVP)
{
    return true;
}

bool SDynTexture2::SetRT([[maybe_unused]] int nRT, [[maybe_unused]] bool bPush, [[maybe_unused]] SDepthTexture* pDepthSurf, [[maybe_unused]] bool bScreenVP)
{
    return true;
}

bool SDynTexture2::RestoreRT([[maybe_unused]] int nRT, [[maybe_unused]] bool bPop)
{
    return true;
}

bool SDynTexture2::SetRectStates()
{
    return true;
}

//===============================================================================

void STexState::PostCreate()
{
}

void STexState::Destroy()
{
}

void STexState::Init(const STexState& src)
{
    memcpy(this, &src, sizeof(src));
}

void STexState::SetComparisonFilter([[maybe_unused]] bool bEnable)
{
}

bool STexState::SetClampMode(int nAddressU, int nAddressV, int nAddressW)
{
    m_nAddressU = nAddressU;
    m_nAddressV = nAddressV;
    m_nAddressW = nAddressW;
    return true;
}

bool STexState::SetFilterMode([[maybe_unused]] int nFilter)
{
    m_nMinFilter = 0;
    m_nMagFilter = 0;
    m_nMipFilter = 0;
    return true;
}

void STexState::SetBorderColor(DWORD dwColor)
{
    m_dwBorderColor = dwColor;
}


SDepthTexture::~SDepthTexture()
{
}

void SDepthTexture::Release([[maybe_unused]] bool bReleaseTex)
{
}

ETEX_Format CTexture::TexFormatFromDeviceFormat([[maybe_unused]] D3DFormat nFormat)
{
    return eTF_Unknown;
}

bool CTexture::RT_CreateDeviceTexture([[maybe_unused]] const byte* pData[6])
{
    return true;
}

void CTexture::UpdateTextureRegion([[maybe_unused]] const uint8_t* data, [[maybe_unused]] int X, [[maybe_unused]] int Y, [[maybe_unused]] int Z, [[maybe_unused]] int USize, [[maybe_unused]] int VSize, [[maybe_unused]] int ZSize, [[maybe_unused]] ETEX_Format eTFSrc)
{
}
void CTexture::RT_UpdateTextureRegion([[maybe_unused]] const uint8_t* data, [[maybe_unused]] int X, [[maybe_unused]] int Y, [[maybe_unused]] int Z, [[maybe_unused]] int USize, [[maybe_unused]] int VSize, [[maybe_unused]] int ZSize, [[maybe_unused]] ETEX_Format eTFSrc)
{
}

void CTexture::Unbind()
{
}

bool SDynTexture::RT_SetRT([[maybe_unused]] int nRT, [[maybe_unused]] int nWidth, [[maybe_unused]] int nHeight, [[maybe_unused]] bool bPush, [[maybe_unused]] bool bScreenVP)
{
    return true;
}

bool SDynTexture::RT_Update([[maybe_unused]] int nNewWidth, [[maybe_unused]] int nNewHeight)
{
    return true;
}

void CTexture::ReleaseSystemTargets(void) {}
void CTexture::ReleaseMiscTargets(void) {}
void CTexture::CreateSystemTargets(void) {}

//===============================================================================

namespace  TextureHelpers
{
    bool VerifyTexSuffix([[maybe_unused]] EEfResTextures texSlot, [[maybe_unused]] const char* texPath)
    {
        return false;
    }

    bool VerifyTexSuffix([[maybe_unused]] EEfResTextures texSlot, [[maybe_unused]] const string& texPath)
    {
        return false;
    }

    const char* LookupTexSuffix([[maybe_unused]] EEfResTextures texSlot)
    {
        return nullptr;
    }

    int8 LookupTexPriority([[maybe_unused]] EEfResTextures texSlot)
    {
        return 0;
    }

    CTexture* LookupTexDefault([[maybe_unused]] EEfResTextures texSlot)
    {
        return nullptr;
    }

    CTexture* LookupTexBlank([[maybe_unused]] EEfResTextures texSlot)
    {
        return nullptr;
    }
}

bool CTexture::Clear() { return true; }

uint32 CDeviceTexture::TextureDataSize([[maybe_unused]] uint32 nWidth, [[maybe_unused]] uint32 nHeight, [[maybe_unused]] uint32 nDepth, [[maybe_unused]] uint32 nMips, [[maybe_unused]] uint32 nSlices, [[maybe_unused]] const ETEX_Format eTF)
{
    return 0;
}

AtomShimTexture::~AtomShimTexture()
{
    if(AZ::Data::AssetBus::Handler::BusIsConnected()) 
    {
        AZ::Data::AssetBus::Handler::BusDisconnect();
    }
}

// Hot-reloading support for the AtomShimTexture.
// This only supports OnAssetReady, not OnAssetReloaded, because it is only intended to handle the case where a texture has not been processed or does not exist.
// The RPI::StreamingImage will handle re-loading if the file changes after it has been loaded initially
void AtomShimTexture::QueueForHotReload(const AZ::Data::AssetId& assetId)
{
    AZ::Data::AssetBus::Handler::BusConnect(assetId);

    // Lyshine may try to load a texture before the ImageSystem wasn't ready
    if (AZ::RPI::ImageSystemInterface::Get())
    {
        CreateFromImage(AZ::RPI::ImageSystemInterface::Get()->GetSystemImage(AZ::RPI::SystemImage::Magenta));
    }
}

void AtomShimTexture::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
{
    AZ::Data::AssetBus::Handler::BusDisconnect(asset.GetId());

    AZ::Data::Asset<AZ::RPI::StreamingImageAsset> imageAsset = asset;
    AZ_Assert(imageAsset, "This should be a streaming image asset");

    CreateFromStreamingImageAsset(imageAsset);
}

void AtomShimTexture::CreateFromStreamingImageAsset(const AZ::Data::Asset<AZ::RPI::StreamingImageAsset>& imageAsset)
{
    AZ::Data::Instance<AZ::RPI::Image> image = AZ::RPI::StreamingImage::FindOrCreate(imageAsset);
    if (!image)
    {
        AZ_Error("CAtomShimRenderer", false, "Failed to find or create an image instance from image asset '%s'", imageAsset.GetHint().c_str());
        return;
    }

    CreateFromImage(image);
}

void AtomShimTexture::CreateFromImage(const AZ::Data::Instance<AZ::RPI::Image>& image)
{
    AZ::RHI::Format rhiViewFormat = AZ::RHI::Format::Unknown;
    AZ::RHI::ImageViewDescriptor viewDesc = AZ::RHI::ImageViewDescriptor(rhiViewFormat);
    AZ::RHI::Factory& factory = AZ::RHI::Factory::Get();
    AZ::RHI::Image* rhiImage = image->GetRHIImage();
    
    AZ::RHI::Ptr<AZ::RHI::ImageView> imageView = rhiImage->GetImageView(viewDesc);
    if(!imageView.get())
    {
        AZ_Assert(false, "Failed to acquire an image view");
        return;        
    }
    
    m_instance = image;
    m_image = rhiImage;
    m_imageView = imageView;

    SetWidth(rhiImage->GetDescriptor().m_size.m_width);
    SetHeight(rhiImage->GetDescriptor().m_size.m_height);
}

