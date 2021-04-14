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

// Description : Common dynamic texture manager implementation.


#include "RenderDll_precompiled.h"

//======================================================================
// Dynamic textures
SDynTexture SDynTexture::s_Root("Root");
uint32 SDynTexture::s_nMemoryOccupied = 0;

uint32 SDynTexture::s_iNumTextureBytesCheckedOut;
uint32 SDynTexture::s_iNumTextureBytesCheckedIn;

SDynTexture::TextureSet    SDynTexture::s_availableTexturePool2D_BC1;
SDynTexture::TextureSubset SDynTexture::s_checkedOutTexturePool2D_BC1;
SDynTexture::TextureSet    SDynTexture::s_availableTexturePool2D_R8G8B8A8;
SDynTexture::TextureSubset SDynTexture::s_checkedOutTexturePool2D_R8G8B8A8;
SDynTexture::TextureSet    SDynTexture::s_availableTexturePool2D_R32F;
SDynTexture::TextureSubset SDynTexture::s_checkedOutTexturePool2D_R32F;
SDynTexture::TextureSet    SDynTexture::s_availableTexturePool2D_R16F;
SDynTexture::TextureSubset SDynTexture::s_checkedOutTexturePool2D_R16F;
SDynTexture::TextureSet    SDynTexture::s_availableTexturePool2D_R16G16F;
SDynTexture::TextureSubset SDynTexture::s_checkedOutTexturePool2D_R16G16F;
SDynTexture::TextureSet    SDynTexture::s_availableTexturePool2D_R8G8B8A8S;
SDynTexture::TextureSubset SDynTexture::s_checkedOutTexturePool2D_R8G8B8A8S;
SDynTexture::TextureSet    SDynTexture::s_availableTexturePool2D_R16G16B16A16F;
SDynTexture::TextureSubset SDynTexture::s_checkedOutTexturePool2D_R16G16B16A16F;
SDynTexture::TextureSet    SDynTexture::s_availableTexturePool2D_R10G10B10A2;
SDynTexture::TextureSubset SDynTexture::s_checkedOutTexturePool2D_R10G10B10A2;
SDynTexture::TextureSet    SDynTexture::s_availableTexturePool2D_R11G11B10F;
SDynTexture::TextureSubset SDynTexture::s_checkedOutTexturePool2D_R11G11B10F;
SDynTexture::TextureSet    SDynTexture::s_availableTexturePoolCube_R11G11B10F;
SDynTexture::TextureSubset SDynTexture::s_checkedOutTexturePoolCube_R11G11B10F;
SDynTexture::TextureSet    SDynTexture::s_availableTexturePool2D_R8G8S;
SDynTexture::TextureSubset SDynTexture::s_checkedOutTexturePool2D_R8G8S;
SDynTexture::TextureSet    SDynTexture::s_availableTexturePoolCube_R8G8S;
SDynTexture::TextureSubset SDynTexture::s_checkedOutTexturePoolCube_R8G8S;

SDynTexture::TextureSet    SDynTexture::s_availableTexturePool2D_Shadows[SBP_MAX];
SDynTexture::TextureSubset SDynTexture::s_checkedOutTexturePool2D_Shadows[SBP_MAX];
SDynTexture::TextureSet    SDynTexture::s_availableTexturePoolCube_Shadows[SBP_MAX];
SDynTexture::TextureSubset SDynTexture::s_checkedOutTexturePoolCube_Shadows[SBP_MAX];

SDynTexture::TextureSet    SDynTexture::s_availableTexturePool2DCustom_R16G16F;
SDynTexture::TextureSubset SDynTexture::s_checkedOutTexturePool2DCustom_R16G16F;

SDynTexture::TextureSet    SDynTexture::s_availableTexturePoolCube_BC1;
SDynTexture::TextureSubset SDynTexture::s_checkedOutTexturePoolCube_BC1;
SDynTexture::TextureSet    SDynTexture::s_availableTexturePoolCube_R8G8B8A8;
SDynTexture::TextureSubset SDynTexture::s_checkedOutTexturePoolCube_R8G8B8A8;
SDynTexture::TextureSet    SDynTexture::s_availableTexturePoolCube_R32F;
SDynTexture::TextureSubset SDynTexture::s_checkedOutTexturePoolCube_R32F;
SDynTexture::TextureSet    SDynTexture::s_availableTexturePoolCube_R16F;
SDynTexture::TextureSubset SDynTexture::s_checkedOutTexturePoolCube_R16F;
SDynTexture::TextureSet    SDynTexture::s_availableTexturePoolCube_R16G16F;
SDynTexture::TextureSubset SDynTexture::s_checkedOutTexturePoolCube_R16G16F;
SDynTexture::TextureSet    SDynTexture::s_availableTexturePoolCube_R8G8B8A8S;
SDynTexture::TextureSubset SDynTexture::s_checkedOutTexturePoolCube_R8G8B8A8S;
SDynTexture::TextureSet    SDynTexture::s_availableTexturePoolCube_R16G16B16A16F;
SDynTexture::TextureSubset SDynTexture::s_checkedOutTexturePoolCube_R16G16B16A16F;
SDynTexture::TextureSet    SDynTexture::s_availableTexturePoolCube_R10G10B10A2;
SDynTexture::TextureSubset SDynTexture::s_checkedOutTexturePoolCube_R10G10B10A2;

SDynTexture::TextureSet    SDynTexture::s_availableTexturePoolCubeCustom_R16G16F;
SDynTexture::TextureSubset SDynTexture::s_checkedOutTexturePoolCubeCustom_R16G16F;

int SDynTexture2::s_nMemoryOccupied[eTP_Max];

uint32 SDynTexture::s_SuggestedDynTexAtlasCloudsMaxsize;
uint32 SDynTexture::s_SuggestedTexAtlasSize;
uint32 SDynTexture::s_SuggestedDynTexMaxSize;
uint32 SDynTexture::s_CurDynTexAtlasCloudsMaxsize;
uint32 SDynTexture::s_CurTexAtlasSize;
uint32 SDynTexture::s_CurDynTexMaxSize;

#if defined(AZ_RESTRICTED_PLATFORM)
    #undef AZ_RESTRICTED_SECTION
    #define SDYNTEXTURE_CPP_SECTION_1 1
#endif

SDynTexture::SDynTexture(const char* szSource)
{
    m_nWidth = 0;
    m_nHeight = 0;
    m_nReqWidth = m_nWidth;
    m_nReqHeight = m_nHeight;
    m_pTexture = NULL;
    m_eTF = eTF_Unknown;
    m_eTT = eTT_2D;
    m_nTexFlags = 0;
    cry_strcpy(m_sSource, szSource);
    m_bLocked = false;
    m_nUpdateMask = 0;
    if (gRenDev)
    {
        m_nFrameReset = gRenDev->m_nFrameReset;
    }

    m_Next = NULL;
    m_Prev = NULL;
    if (!s_Root.m_Next)
    {
        s_Root.m_Next = &s_Root;
        s_Root.m_Prev = &s_Root;
    }
    AdjustRealSize();
}

SDynTexture::SDynTexture(int nWidth, int nHeight, ETEX_Format eTF, ETEX_Type eTT, int nTexFlags, const char* szSource)
{
    m_nWidth =  nWidth;
    m_nHeight = nHeight;
    m_nReqWidth = m_nWidth;
    m_nReqHeight = m_nHeight;
    m_eTF = eTF;
    m_eTT = eTT;
    m_nTexFlags = nTexFlags | FT_USAGE_RENDERTARGET;
    cry_strcpy(m_sSource, szSource);
    m_bLocked = false;
    m_nUpdateMask = 0;
    m_pFrustumOwner = NULL;
    if (gRenDev)
    {
        m_nFrameReset = gRenDev->m_nFrameReset;
    }

    m_pTexture = NULL;
    m_Next = NULL;
    m_Prev = NULL;
    if (!s_Root.m_Next)
    {
        s_Root.m_Next = &s_Root;
        s_Root.m_Prev = &s_Root;
    }
    Link();

    AdjustRealSize();
}

SDynTexture::~SDynTexture()
{
    if (m_pTexture)
    {
        ReleaseDynamicRT(false);
    }
    m_pTexture = NULL;
    Unlink();
}

int SDynTexture::GetTextureID()
{
    return m_pTexture ? m_pTexture->GetTextureID() : 0;
}

bool SDynTexture::FreeTextures(bool bOldOnly, int nNeedSpace)
{
    bool bFreed = false;
    if (bOldOnly)
    {
        SDynTexture* pTX = SDynTexture::s_Root.m_Prev;
        int nFrame = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_nFrameUpdateID - 400;
        while (nNeedSpace + s_nMemoryOccupied > (int)SDynTexture::s_CurDynTexMaxSize * 1024 * 1024)
        {
            if (pTX == &SDynTexture::s_Root)
            {
                break;
            }
            SDynTexture* pNext = pTX->m_Prev;
            {
                // We cannot unload locked texture or texture used in current frame
                // Better to increase pool size temporarily
                if (pTX->m_pTexture && !pTX->m_pTexture->IsActiveRenderTarget())
                {
                    if (pTX->m_pTexture->m_nAccessFrameID < nFrame && pTX->m_pTexture->m_nUpdateFrameID < nFrame && !pTX->m_bLocked)
                    {
                        pTX->ReleaseDynamicRT(true);
                    }
                }
            }
            pTX = pNext;
        }
        if (nNeedSpace + s_nMemoryOccupied < (int)SDynTexture::s_CurDynTexMaxSize * 1024 * 1024)
        {
            return true;
        }
    }
    bFreed = FreeAvailableDynamicRT(nNeedSpace, m_eTT == eTT_2D ? &s_availableTexturePool2D_R8G8B8A8 : &s_availableTexturePoolCube_R8G8B8A8, bOldOnly);
    if (!bFreed)
    {
        bFreed = FreeAvailableDynamicRT(nNeedSpace, m_eTT == eTT_2D ? &s_availableTexturePool2D_BC1 : &s_availableTexturePoolCube_BC1, bOldOnly);
    }
    if (!bFreed)
    {
        bFreed = FreeAvailableDynamicRT(nNeedSpace, m_eTT == eTT_2D ? &s_availableTexturePool2D_R32F : &s_availableTexturePoolCube_R32F, bOldOnly);
    }
    if (!bFreed)
    {
        bFreed = FreeAvailableDynamicRT(nNeedSpace, m_eTT == eTT_2D ? &s_availableTexturePool2D_R16G16F : &s_availableTexturePoolCube_R16G16F, bOldOnly);
    }
    if (!bFreed)
    {
        bFreed = FreeAvailableDynamicRT(nNeedSpace, m_eTT == eTT_2D ? &s_availableTexturePool2D_R16G16B16A16F : &s_availableTexturePoolCube_R16G16B16A16F, bOldOnly);
    }
    if (!bFreed)
    {
        bFreed = FreeAvailableDynamicRT(nNeedSpace, m_eTT == eTT_2D ? &s_availableTexturePool2D_R11G11B10F : &s_availableTexturePoolCube_R11G11B10F, bOldOnly);
    }
    if (!bFreed)
    {
        bFreed = FreeAvailableDynamicRT(nNeedSpace, m_eTT == eTT_2D ? &s_availableTexturePool2D_R8G8S : &s_availableTexturePoolCube_R8G8S, bOldOnly);
    }

    //if (!bFreed && m_eTT==eTT_2D)
    //bFreed = FreeAvailableDynamicRT(nNeedSpace, &m_availableTexturePool2D_ATIDF24);

    //First pass - Free textures from the pools with the same texture types
    //shadows pools
    for (int nPool = SBP_D16; nPool < SBP_MAX; nPool++)
    {
        if (!bFreed && m_eTT == eTT_2D)
        {
            bFreed = FreeAvailableDynamicRT(nNeedSpace, &s_availableTexturePool2D_Shadows[nPool], bOldOnly);
        }
    }
    for (int nPool = SBP_D16; nPool < SBP_MAX; nPool++)
    {
        if (!bFreed && m_eTT != eTT_2D)
        {
            bFreed = FreeAvailableDynamicRT(nNeedSpace, &s_availableTexturePoolCube_Shadows[nPool], bOldOnly);
        }
    }

    if (!bFreed)
    {
        bFreed = FreeAvailableDynamicRT(nNeedSpace, m_eTT == eTT_2D ? &s_availableTexturePoolCube_R8G8B8A8 : &s_availableTexturePool2D_R8G8B8A8, bOldOnly);
    }
    if (!bFreed)
    {
        bFreed = FreeAvailableDynamicRT(nNeedSpace, m_eTT == eTT_2D ? &s_availableTexturePoolCube_BC1 : &s_availableTexturePool2D_BC1, bOldOnly);
    }
    if (!bFreed)
    {
        bFreed = FreeAvailableDynamicRT(nNeedSpace, m_eTT == eTT_2D ? &s_availableTexturePoolCube_R32F : &s_availableTexturePool2D_R32F, bOldOnly);
    }
    if (!bFreed)
    {
        bFreed = FreeAvailableDynamicRT(nNeedSpace, m_eTT == eTT_2D ? &s_availableTexturePoolCube_R16G16F : &s_availableTexturePool2D_R16G16F, bOldOnly);
    }
    if (!bFreed)
    {
        bFreed = FreeAvailableDynamicRT(nNeedSpace, m_eTT == eTT_2D ? &s_availableTexturePoolCube_R16G16B16A16F : &s_availableTexturePool2D_R16G16B16A16F, bOldOnly);
    }
    //if (!bFreed && m_eTT!=eTT_2D)
    //  bFreed = FreeAvailableDynamicRT(nNeedSpace, &m_availableTexturePool2D_ATIDF24);


    //Second pass - Free textures from the pools with the different texture types
    //shadows pools
    for (int nPool = SBP_D16; nPool < SBP_MAX; nPool++)
    {
        if (!bFreed && m_eTT != eTT_2D)
        {
            bFreed = FreeAvailableDynamicRT(nNeedSpace, &s_availableTexturePool2D_Shadows[nPool], bOldOnly);
        }
    }
    for (int nPool = SBP_D16; nPool < SBP_MAX; nPool++)
    {
        if (!bFreed && m_eTT == eTT_2D)
        {
            bFreed = FreeAvailableDynamicRT(nNeedSpace, &s_availableTexturePoolCube_Shadows[nPool], bOldOnly);
        }
    }
    return bFreed;
}

bool SDynTexture::Update(int nNewWidth, int nNewHeight)
{
    return gRenDev->m_pRT->RC_DynTexUpdate(this, nNewWidth, nNewHeight);
}

void SDynTexture::AdjustRealSize()
{
    m_nWidth = m_nReqWidth;
    m_nHeight = m_nReqHeight;
}

void SDynTexture::GetImageRect(uint32& nX, uint32& nY, uint32& nWidth, uint32& nHeight)
{
    nX = 0;
    nY = 0;
    nWidth = m_nWidth;
    nHeight = m_nHeight;
}

void SDynTexture::Apply(int nTUnit, int nTS)
{
    if (!m_pTexture)
    {
        Update(m_nWidth, m_nHeight);
    }
    if (m_pTexture)
    {
        m_pTexture->Apply(nTUnit, nTS);
    }
    gRenDev->m_cEF.m_RTRect.x = 0;
    gRenDev->m_cEF.m_RTRect.y = 0;
    gRenDev->m_cEF.m_RTRect.z = 1;
    gRenDev->m_cEF.m_RTRect.w = 1;
}

void SDynTexture::ShutDown()
{
    SDynTexture* pTX, * pTXNext;
    for (pTX = SDynTexture::s_Root.m_Next; pTX != &SDynTexture::s_Root; pTX = pTXNext)
    {
        pTXNext = pTX->m_Next;
        SAFE_RELEASE_FORCE(pTX);
    }
    SDynTexture Tex("Release");
    Tex.m_eTT = eTT_2D;
    Tex.FreeTextures(false, 1024 * 1024 * 1024);

    Tex.m_eTT = eTT_Cube;
    Tex.FreeTextures(false, 1024 * 1024 * 1024);
}

bool SDynTexture::FreeAvailableDynamicRT(int nNeedSpace, TextureSet* pSet, bool bOldOnly)
{
    assert(s_iNumTextureBytesCheckedOut + s_iNumTextureBytesCheckedIn == s_nMemoryOccupied);

    int nSpace = s_nMemoryOccupied;
    int nFrame = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_nFrameUpdateID - 400;
    while (nNeedSpace + nSpace > (int)SDynTexture::s_CurDynTexMaxSize * 1024 * 1024)
    {
        TextureSetItor itor = pSet->begin();
        while (itor != pSet->end())
        {
            TextureSubset* pSubset = itor->second;
            TextureSubsetItor itorss = pSubset->begin();
            while (itorss != pSubset->end())
            {
                CTexture* pTex = itorss->second;
                PREFAST_ASSUME(pTex);
                if (!bOldOnly || (pTex->m_nAccessFrameID < nFrame && pTex->m_nUpdateFrameID < nFrame))
                {
                    itorss->second = NULL;
                    pSubset->erase(itorss);
                    itorss = pSubset->begin();
                    nSpace -= pTex->GetDataSize();
                    s_iNumTextureBytesCheckedIn -= pTex->GetDataSize();
                    SAFE_RELEASE(pTex);
                    if (nNeedSpace + nSpace < (int)SDynTexture::s_CurDynTexMaxSize * 1024 * 1024)
                    {
                        break;
                    }
                }
                else
                {
                    itorss++;
                }
            }
            if (pSubset->size() == 0)
            {
                delete pSubset;
                pSet->erase(itor);
                itor = pSet->begin();
            }
            else
            {
                itor++;
            }
            if (nNeedSpace + nSpace < (int)SDynTexture::s_CurDynTexMaxSize * 1024 * 1024)
            {
                break;
            }
        }
        if (itor == pSet->end())
        {
            break;
        }
    }
    s_nMemoryOccupied = nSpace;

    assert(s_iNumTextureBytesCheckedOut + s_iNumTextureBytesCheckedIn == s_nMemoryOccupied);

    if (nNeedSpace + nSpace > (int)SDynTexture::s_CurDynTexMaxSize * 1024 * 1024)
    {
        return false;
    }
    return true;
}

void SDynTexture::ReleaseDynamicRT(bool bForce)
{
    if (!m_pTexture)
    {
        return;
    }
    m_nUpdateMask = 0;

    // first see if the texture is in the checked out pool.
    TextureSubset* pSubset;
    if (m_eTF == eTF_R8G8B8A8)
    {
        pSubset = m_eTT == eTT_2D ? &s_checkedOutTexturePool2D_R8G8B8A8 : &s_checkedOutTexturePoolCube_R8G8B8A8;
    }
    else
    if (m_eTF == eTF_BC1)
    {
        pSubset = m_eTT == eTT_2D ? &s_checkedOutTexturePool2D_BC1 : &s_checkedOutTexturePoolCube_BC1;
    }
    else
    if (m_eTF == eTF_R32F)
    {
        pSubset = m_eTT == eTT_2D ? &s_checkedOutTexturePool2D_R32F : &s_checkedOutTexturePoolCube_R32F;
    }
    else
    if (m_eTF == eTF_R16F)
    {
        pSubset = m_eTT == eTT_2D ? &s_checkedOutTexturePool2D_R16F : &s_checkedOutTexturePoolCube_R16F;
    }
    else
    if (m_eTF == eTF_R16G16F)
    {
        pSubset = m_eTT == eTT_2D ? &s_checkedOutTexturePool2D_R16G16F : &s_checkedOutTexturePoolCube_R16G16F;
    }
    else
    if (m_eTF == eTF_R8G8B8A8S)
    {
        pSubset = m_eTT == eTT_2D ? &s_checkedOutTexturePool2D_R8G8B8A8S : &s_checkedOutTexturePoolCube_R8G8B8A8S;
    }
    else
    if (m_eTF == eTF_R16G16B16A16F)
    {
        pSubset = m_eTT == eTT_2D ? &s_checkedOutTexturePool2D_R16G16B16A16F : &s_checkedOutTexturePoolCube_R16G16B16A16F;
    }
    else
#if defined(WIN32) || defined(APPLE) || defined(LINUX) || defined(SUPPORTS_DEFERRED_SHADING_L_BUFFERS_FORMAT)
    if (m_eTF == eTF_R11G11B10F)
    {
        pSubset = m_eTT == eTT_2D ? &s_checkedOutTexturePool2D_R11G11B10F : &s_checkedOutTexturePoolCube_R11G11B10F;
    }
    else
#endif
    if (m_eTF == eTF_R8G8S)
    {
        pSubset = m_eTT == eTT_2D ? &s_checkedOutTexturePool2D_R8G8S : &s_checkedOutTexturePoolCube_R8G8S;
    }
    else
    if (m_eTF == eTF_R10G10B10A2)
    {
        pSubset = m_eTT == eTT_2D ? &s_checkedOutTexturePool2D_R10G10B10A2 : &s_checkedOutTexturePoolCube_R10G10B10A2;
    }
    else
    if (ConvertTexFormatToShadowsPool(m_eTF) != SBP_UNKNOWN)
    {
        if (m_eTT == eTT_2D)
        {
            pSubset = &s_checkedOutTexturePool2D_Shadows[ConvertTexFormatToShadowsPool(m_eTF)];
        }
        else if (m_eTT == eTT_Cube)
        {
            pSubset = &s_checkedOutTexturePoolCube_Shadows[ConvertTexFormatToShadowsPool(m_eTF)];
        }
        else
        {
            pSubset = NULL;
            assert(0);
        }
    }
    else
    {
        pSubset = NULL;
        assert(0);
    }

    PREFAST_ASSUME(pSubset);

    TextureSubsetItor coTexture = pSubset->find(m_pTexture->GetID());
    if (coTexture != pSubset->end())
    { // if it is there, remove it.
        pSubset->erase(coTexture);
        s_iNumTextureBytesCheckedOut -= m_pTexture->GetDataSize();
    }

    // Don't cache too many unused textures.
    if (bForce)
    {
        s_nMemoryOccupied -= m_pTexture->GetDataSize();
        assert(s_iNumTextureBytesCheckedOut + s_iNumTextureBytesCheckedIn == s_nMemoryOccupied);
#ifndef NDEBUG
        int refCount =
#endif
            m_pTexture->Release();
        assert(refCount <= 0);
        m_pTexture = NULL;
        Unlink();
        return;
    }

    TextureSet* pSet;
    if (m_eTF == eTF_R8G8B8A8)
    {
        pSet = m_eTT == eTT_2D ? &s_availableTexturePool2D_R8G8B8A8 : &s_availableTexturePoolCube_R8G8B8A8;
    }
    else
    if (m_eTF == eTF_BC1)
    {
        pSet = m_eTT == eTT_2D ? &s_availableTexturePool2D_BC1 : &s_availableTexturePoolCube_BC1;
    }
    else
    if (m_eTF == eTF_R32F)
    {
        pSet = m_eTT == eTT_2D ? &s_availableTexturePool2D_R32F : &s_availableTexturePoolCube_R32F;
    }
    else
    if (m_eTF == eTF_R16F)
    {
        pSet = m_eTT == eTT_2D ? &s_availableTexturePool2D_R16F : &s_availableTexturePoolCube_R16F;
    }
    else
    if (m_eTF == eTF_R16G16F)
    {
        pSet = m_eTT == eTT_2D ? &s_availableTexturePool2D_R16G16F : &s_availableTexturePoolCube_R16G16F;
    }
    else
    if (m_eTF == eTF_R8G8B8A8S)
    {
        pSet = m_eTT == eTT_2D ? &s_availableTexturePool2D_R8G8B8A8S : &s_availableTexturePoolCube_R8G8B8A8S;
    }
    else
    if (m_eTF == eTF_R16G16B16A16F)
    {
        pSet = m_eTT == eTT_2D ? &s_availableTexturePool2D_R16G16B16A16F : &s_availableTexturePoolCube_R16G16B16A16F;
    }
    else
    if (m_eTF == eTF_R11G11B10F)
    {
        pSet = m_eTT == eTT_2D ? &s_availableTexturePool2D_R11G11B10F : &s_availableTexturePoolCube_R11G11B10F;
    }
    else
    if (m_eTF == eTF_R8G8S)
    {
        pSet = m_eTT == eTT_2D ? &s_availableTexturePool2D_R8G8S : &s_availableTexturePoolCube_R8G8S;
    }
    else
    if (m_eTF == eTF_R10G10B10A2)
    {
        pSet = m_eTT == eTT_2D ? &s_availableTexturePool2D_R10G10B10A2 : &s_availableTexturePoolCube_R10G10B10A2;
    }
    else
    if (ConvertTexFormatToShadowsPool(m_eTF) != SBP_UNKNOWN)
    {
        if (m_eTT == eTT_2D)
        {
            pSet = &s_availableTexturePool2D_Shadows[ConvertTexFormatToShadowsPool(m_eTF)];
        }
        else if (m_eTT == eTT_Cube)
        {
            pSet = &s_availableTexturePoolCube_Shadows[ConvertTexFormatToShadowsPool(m_eTF)];
        }
        else
        {
            pSet = NULL;
            assert(0);
        }
    }
    else
    {
        pSet = NULL;
        assert(0);
    }

    PREFAST_ASSUME(pSet);

    TextureSetItor subset = pSet->find(m_nWidth);
    if (subset != pSet->end())
    {
        subset->second->insert(TextureSubset::value_type(m_nHeight, m_pTexture));
        s_iNumTextureBytesCheckedIn += m_pTexture->GetDataSize();
    }
    else
    {
        pSubset = new TextureSubset;
        pSet->insert(TextureSet::value_type(m_nWidth, pSubset));
        pSubset->insert(TextureSubset::value_type(m_nHeight, m_pTexture));
        s_iNumTextureBytesCheckedIn += m_pTexture->GetDataSize();
    }
    m_pTexture = NULL;
    Unlink();
}

CTexture* SDynTexture::GetDynamicRT()
{
    TextureSet* pSet;
    TextureSubset* pSubset;

    assert(s_iNumTextureBytesCheckedOut + s_iNumTextureBytesCheckedIn == s_nMemoryOccupied);

    if (m_eTF == eTF_R8G8B8A8)
    {
        pSet = m_eTT == eTT_2D ? &s_availableTexturePool2D_R8G8B8A8 : &s_availableTexturePoolCube_R8G8B8A8;
        pSubset = m_eTT == eTT_2D ? &s_checkedOutTexturePool2D_R8G8B8A8 : &s_checkedOutTexturePoolCube_R8G8B8A8;
    }
    else
    if (m_eTF == eTF_BC1)
    {
        pSet = m_eTT == eTT_2D ? &s_availableTexturePool2D_BC1 : &s_availableTexturePoolCube_BC1;
        pSubset = m_eTT == eTT_2D ? &s_checkedOutTexturePool2D_BC1 : &s_checkedOutTexturePoolCube_BC1;
    }
    else
    if (m_eTF == eTF_R32F)
    {
        pSet = m_eTT == eTT_2D ? &s_availableTexturePool2D_R32F : &s_availableTexturePoolCube_R32F;
        pSubset = m_eTT == eTT_2D ? &s_checkedOutTexturePool2D_R32F : &s_checkedOutTexturePoolCube_R32F;
    }
    else
    if (m_eTF == eTF_R16F)
    {
        pSet = m_eTT == eTT_2D ? &s_availableTexturePool2D_R16F : &s_availableTexturePoolCube_R16F;
        pSubset = m_eTT == eTT_2D ? &s_checkedOutTexturePool2D_R16F : &s_checkedOutTexturePoolCube_R16F;
    }
    else
    if (m_eTF == eTF_R16G16F)
    {
        pSet = m_eTT == eTT_2D ? &s_availableTexturePool2D_R16G16F : &s_availableTexturePoolCube_R16G16F;
        pSubset = m_eTT == eTT_2D ? &s_checkedOutTexturePool2D_R16G16F : &s_checkedOutTexturePoolCube_R16G16F;
    }
    else
    if (m_eTF == eTF_R8G8B8A8S)
    {
        pSet = m_eTT == eTT_2D ? &s_availableTexturePool2D_R8G8B8A8S : &s_availableTexturePoolCube_R8G8B8A8S;
        pSubset = m_eTT == eTT_2D ? &s_checkedOutTexturePool2D_R8G8B8A8S : &s_checkedOutTexturePoolCube_R8G8B8A8S;
    }
    else
    if (m_eTF == eTF_R16G16B16A16F)
    {
        pSet = m_eTT == eTT_2D ? &s_availableTexturePool2D_R16G16B16A16F : &s_availableTexturePoolCube_R16G16B16A16F;
        pSubset = m_eTT == eTT_2D ? &s_checkedOutTexturePool2D_R16G16B16A16F : &s_checkedOutTexturePoolCube_R16G16B16A16F;
    }
    else
    if (m_eTF == eTF_R11G11B10F)
    {
        pSet = m_eTT == eTT_2D ? &s_availableTexturePool2D_R11G11B10F : &s_availableTexturePoolCube_R11G11B10F;
        pSubset = m_eTT == eTT_2D ? &s_checkedOutTexturePool2D_R11G11B10F : &s_checkedOutTexturePoolCube_R11G11B10F;
    }
    else
    if (m_eTF == eTF_R8G8S)
    {
        pSet = m_eTT == eTT_2D ? &s_availableTexturePool2D_R8G8S : &s_availableTexturePoolCube_R8G8S;
        pSubset = m_eTT == eTT_2D ? &s_checkedOutTexturePool2D_R8G8S : &s_checkedOutTexturePoolCube_R8G8S;
    }
    else
    if (m_eTF == eTF_R10G10B10A2)
    {
        pSet = m_eTT == eTT_2D ? &s_availableTexturePool2D_R10G10B10A2 : &s_availableTexturePoolCube_R10G10B10A2;
        pSubset = m_eTT == eTT_2D ? &s_checkedOutTexturePool2D_R10G10B10A2 : &s_checkedOutTexturePoolCube_R10G10B10A2;
    }
    else
    if (ConvertTexFormatToShadowsPool(m_eTF) != SBP_UNKNOWN)
    {
        if (m_eTT == eTT_2D)
        {
            pSet = &s_availableTexturePool2D_Shadows[ConvertTexFormatToShadowsPool(m_eTF)];
            pSubset = &s_checkedOutTexturePool2D_Shadows[ConvertTexFormatToShadowsPool(m_eTF)];
        }
        else if (m_eTT == eTT_Cube)
        {
            pSet = &s_availableTexturePoolCube_Shadows[ConvertTexFormatToShadowsPool(m_eTF)];
            pSubset = &s_checkedOutTexturePoolCube_Shadows[ConvertTexFormatToShadowsPool(m_eTF)];
        }
        else
        {
            pSet = NULL;
            pSubset = NULL;
            assert(0);
        }
    }
    else
    {
        pSet = NULL;
        pSubset = NULL;
        assert(0);
    }

    PREFAST_ASSUME(pSet);

    TextureSetItor subset = pSet->find(m_nWidth);
    if (subset != pSet->end())
    {
        TextureSubsetItor texture = subset->second->find(m_nHeight);
        if (texture != subset->second->end())
        { //  found one!
          // extract the texture
            CTexture* pTexture = texture->second;
            texture->second = NULL;
            // first remove it from this set.
            subset->second->erase(texture);
            // now add it to the checked out texture set.
            pSubset->insert(TextureSubset::value_type(pTexture->GetID(), pTexture));
            s_iNumTextureBytesCheckedOut += pTexture->GetDataSize();
            s_iNumTextureBytesCheckedIn -= pTexture->GetDataSize();

            assert(s_iNumTextureBytesCheckedOut + s_iNumTextureBytesCheckedIn == s_nMemoryOccupied);

            return pTexture;
        }
    }
    return NULL;
}

CTexture* SDynTexture::CreateDynamicRT()
{
    //assert(m_eTF == eTF_A8R8G8B8 && m_eTT == eTT_2D);

    assert(s_iNumTextureBytesCheckedOut + s_iNumTextureBytesCheckedIn == s_nMemoryOccupied);

    char name[256];
    CTexture* pTexture = GetDynamicRT();
    if (pTexture)
    {
        return pTexture;
    }

    TextureSet* pSet;
    TextureSubset* pSubset;
    if (m_eTF == eTF_R8G8B8A8)
    {
        pSet = m_eTT == eTT_2D ? &s_availableTexturePool2D_R8G8B8A8 : &s_availableTexturePoolCube_R8G8B8A8;
        pSubset = m_eTT == eTT_2D ? &s_checkedOutTexturePool2D_R8G8B8A8 : &s_checkedOutTexturePoolCube_R8G8B8A8;
    }
    else
    if (m_eTF == eTF_BC1)
    {
        pSet = m_eTT == eTT_2D ? &s_availableTexturePool2D_BC1 : &s_availableTexturePoolCube_BC1;
        pSubset = m_eTT == eTT_2D ? &s_checkedOutTexturePool2D_BC1 : &s_checkedOutTexturePoolCube_BC1;
    }
    else
    if (m_eTF == eTF_R32F)
    {
        pSet = m_eTT == eTT_2D ? &s_availableTexturePool2D_R32F : &s_availableTexturePoolCube_R32F;
        pSubset = m_eTT == eTT_2D ? &s_checkedOutTexturePool2D_R32F : &s_checkedOutTexturePoolCube_R32F;
    }
    else
    if (m_eTF == eTF_R16F)
    {
        pSet = m_eTT == eTT_2D ? &s_availableTexturePool2D_R16F : &s_availableTexturePoolCube_R16F;
        pSubset = m_eTT == eTT_2D ? &s_checkedOutTexturePool2D_R16F : &s_checkedOutTexturePoolCube_R16F;
    }
    else
    if (m_eTF == eTF_R16G16F)
    {
        pSet = m_eTT == eTT_2D ? &s_availableTexturePool2D_R16G16F : &s_availableTexturePoolCube_R16G16F;
        pSubset = m_eTT == eTT_2D ? &s_checkedOutTexturePool2D_R16G16F : &s_checkedOutTexturePoolCube_R16G16F;
    }
    else
    if (m_eTF == eTF_R8G8B8A8S)
    {
        pSet = m_eTT == eTT_2D ? &s_availableTexturePool2D_R8G8B8A8S : &s_availableTexturePoolCube_R8G8B8A8S;
        pSubset = m_eTT == eTT_2D ? &s_checkedOutTexturePool2D_R8G8B8A8S : &s_checkedOutTexturePoolCube_R8G8B8A8S;
    }
    else
    if (m_eTF == eTF_R16G16B16A16F)
    {
        pSet = m_eTT == eTT_2D ? &s_availableTexturePool2D_R16G16B16A16F : &s_availableTexturePoolCube_R16G16B16A16F;
        pSubset = m_eTT == eTT_2D ? &s_checkedOutTexturePool2D_R16G16B16A16F : &s_checkedOutTexturePoolCube_R16G16B16A16F;
    }
    else
    if (m_eTF == eTF_R11G11B10F)
    {
        pSet = m_eTT == eTT_2D ? &s_availableTexturePool2D_R11G11B10F : &s_availableTexturePoolCube_R11G11B10F;
        pSubset = m_eTT == eTT_2D ? &s_checkedOutTexturePool2D_R11G11B10F : &s_checkedOutTexturePoolCube_R11G11B10F;
    }
    else
    if (m_eTF == eTF_R8G8S)
    {
        pSet = m_eTT == eTT_2D ? &s_availableTexturePool2D_R8G8S : &s_availableTexturePoolCube_R8G8S;
        pSubset = m_eTT == eTT_2D ? &s_checkedOutTexturePool2D_R8G8S : &s_checkedOutTexturePoolCube_R8G8S;
    }
    else
    if (m_eTF == eTF_R10G10B10A2)
    {
        pSet = m_eTT == eTT_2D ? &s_availableTexturePool2D_R10G10B10A2 : &s_availableTexturePoolCube_R10G10B10A2;
        pSubset = m_eTT == eTT_2D ? &s_checkedOutTexturePool2D_R10G10B10A2 : &s_checkedOutTexturePoolCube_R10G10B10A2;
    }
    else
    if (ConvertTexFormatToShadowsPool(m_eTF) != SBP_UNKNOWN)
    {
        if (m_eTT == eTT_2D)
        {
            pSet = &s_availableTexturePool2D_Shadows[ConvertTexFormatToShadowsPool(m_eTF)];
            pSubset = &s_checkedOutTexturePool2D_Shadows[ConvertTexFormatToShadowsPool(m_eTF)];
        }
        else if (m_eTT == eTT_Cube)
        {
            pSet = &s_availableTexturePoolCube_Shadows[ConvertTexFormatToShadowsPool(m_eTF)];
            pSubset = &s_checkedOutTexturePoolCube_Shadows[ConvertTexFormatToShadowsPool(m_eTF)];
        }
        else
        {
            pSet = NULL;
            pSubset = NULL;
            assert(0);
        }
    }
    else
    {
        pSet = NULL;
        pSubset = NULL;
        assert(0);
    }

    if (m_eTT == eTT_2D)
    {
        sprintf_s(name, "$Dyn_%s_2D_%s_%d", m_sSource, CTexture::NameForTextureFormat(m_eTF), gRenDev->m_TexGenID++);
    }
    else
    {
        sprintf_s(name, "$Dyn_%s_Cube_%s_%d", m_sSource, CTexture::NameForTextureFormat(m_eTF), gRenDev->m_TexGenID++);
    }

    PREFAST_ASSUME(pSet);

    TextureSetItor subset = pSet->find(m_nWidth);
    if (subset != pSet->end())
    {
        CTexture* pNewTexture = CTexture::CreateRenderTarget(name, m_nWidth, m_nHeight, Clr_Unknown, m_eTT, m_nTexFlags, m_eTF);
        pSubset->insert(TextureSubset::value_type(pNewTexture->GetID(), pNewTexture));
        s_nMemoryOccupied += pNewTexture->GetDataSize();
        s_iNumTextureBytesCheckedOut += pNewTexture->GetDataSize();

        assert(s_iNumTextureBytesCheckedOut + s_iNumTextureBytesCheckedIn == s_nMemoryOccupied);

        return pNewTexture;
    }
    else
    {
        TextureSubset* pSSet = new TextureSubset;
        pSet->insert(TextureSet::value_type(m_nWidth, pSSet));
        CTexture* pNewTexture = CTexture::CreateRenderTarget(name, m_nWidth, m_nHeight, Clr_Unknown, m_eTT, m_nTexFlags, m_eTF);
#ifndef CRY_USE_METAL
        pNewTexture->Clear(ColorF(0.0f, 0.0f, 0.0f, 1.0f));
#endif
        pSubset->insert(TextureSubset::value_type(pNewTexture->GetID(), pNewTexture));
        s_nMemoryOccupied += pNewTexture->GetDataSize();
        s_iNumTextureBytesCheckedOut += pNewTexture->GetDataSize();

        assert(s_iNumTextureBytesCheckedOut + s_iNumTextureBytesCheckedIn == s_nMemoryOccupied);

        return pNewTexture;
    }
}

void SDynTexture::ResetUpdateMask()
{
    m_nUpdateMask = 0;
    if (gRenDev)
    {
        m_nFrameReset = gRenDev->m_nFrameReset;
    }
}

void SDynTexture::SetUpdateMask()
{
    int nFrame = gRenDev->RT_GetCurrGpuID();
    m_nUpdateMask |= 1 << nFrame;
}

void SDynTexture::ReleaseForce()
{
    ReleaseDynamicRT(true);
    delete this;
}

bool SDynTexture::IsValid()
{
    if (!m_pTexture)
    {
        return false;
    }
    if (m_nFrameReset != gRenDev->m_nFrameReset)
    {
        m_nFrameReset = gRenDev->m_nFrameReset;
        m_nUpdateMask = 0;
        return false;
    }
    if (gRenDev->GetActiveGPUCount() > 1)
    {
        if ((gRenDev->GetFeatures() & RFT_HW_MASK) == RFT_HW_ATI)
        {
            uint32 nX, nY, nW, nH;
            GetImageRect(nX, nY, nW, nH);
            if (nW < 1024 && nH < 1024)
            {
                return true;
            }
        }

        int nFrame = gRenDev->RT_GetCurrGpuID();
        if (!((1 << nFrame) & m_nUpdateMask))
        {
            return false;
        }
    }
    return true;
}

void SDynTexture::Tick()
{
    if (s_SuggestedDynTexMaxSize != CRenderer::CV_r_dyntexmaxsize ||
        s_SuggestedDynTexAtlasCloudsMaxsize != CRenderer::CV_r_dyntexatlascloudsmaxsize ||
        s_SuggestedTexAtlasSize != CRenderer::CV_r_texatlassize)
    {
        Init();
    }
}

void SDynTexture::Init()
{
    s_SuggestedDynTexAtlasCloudsMaxsize = CRenderer::CV_r_dyntexatlascloudsmaxsize;
    s_SuggestedTexAtlasSize = CRenderer::CV_r_texatlassize;
    s_SuggestedDynTexMaxSize = CRenderer::CV_r_dyntexmaxsize;

    s_CurDynTexAtlasCloudsMaxsize = s_SuggestedDynTexAtlasCloudsMaxsize;
    s_CurTexAtlasSize = s_SuggestedTexAtlasSize;
    s_CurDynTexMaxSize = s_SuggestedDynTexMaxSize;
}

EShadowBuffers_Pool SDynTexture::ConvertTexFormatToShadowsPool(ETEX_Format e)
{
    switch (e)
    {
    case (eTF_D16):
        return SBP_D16;
    case (eTF_D24S8):
        return SBP_D24S8;
    case (eTF_D32F):
    case (eTF_D32FS8):
        return SBP_D32FS8;
    case (eTF_R16G16):
        return SBP_R16G16;

    default:
        break;
    }
    //assert( false );
    return SBP_UNKNOWN;
}


int SDynTexture2::GetTextureID()
{
    return m_pTexture ? m_pTexture->GetTextureID() : 0;
}

void SDynTexture2::GetImageRect(uint32& nX, uint32& nY, uint32& nWidth, uint32& nHeight)
{
    nX = 0;
    nY = 0;
    if (m_pTexture)
    {
        if (m_pTexture->GetWidth() != SDynTexture::s_CurTexAtlasSize || m_pTexture->GetHeight() != SDynTexture::s_CurTexAtlasSize)
        {
            UpdateAtlasSize(SDynTexture::s_CurTexAtlasSize, SDynTexture::s_CurTexAtlasSize);
        }
        assert (m_pTexture->GetWidth() == SDynTexture::s_CurTexAtlasSize || m_pTexture->GetHeight() == SDynTexture::s_CurTexAtlasSize);
    }
    nWidth  = SDynTexture::s_CurTexAtlasSize;
    nHeight = SDynTexture::s_CurTexAtlasSize;
}

//====================================================================================

SDynTexture_Shadow SDynTexture_Shadow::s_RootShadow("RootShadow");

SDynTexture_Shadow::SDynTexture_Shadow(const char* szSource)
    : SDynTexture(szSource)
{
    m_nUniqueID = 0;
    m_NextShadow = NULL;
    m_PrevShadow = NULL;
    if (!s_RootShadow.m_NextShadow)
    {
        s_RootShadow.m_NextShadow = &s_RootShadow;
        s_RootShadow.m_PrevShadow = &s_RootShadow;
    }
    if (this != &s_RootShadow)
    {
        LinkShadow(&s_RootShadow);
    }
}

SDynTexture_Shadow::SDynTexture_Shadow(int nWidth, int nHeight, ETEX_Format eTF, ETEX_Type eTT, int nTexFlags, const char* szSource)
    : SDynTexture(nWidth, nHeight, eTF, eTT, nTexFlags, szSource)
{
    m_nWidth = nWidth;
    m_nHeight = nHeight;

    if (gRenDev)
    {
        m_nUniqueID = gRenDev->m_TexGenID++;
    }
    else
    {
        m_nUniqueID = 0;
    }

    m_NextShadow = NULL;
    m_PrevShadow = NULL;
    if (!s_RootShadow.m_NextShadow)
    {
        s_RootShadow.m_NextShadow = &s_RootShadow;
        s_RootShadow.m_PrevShadow = &s_RootShadow;
    }
    if (this != &s_RootShadow)
    {
        LinkShadow(&s_RootShadow);
    }
}

void SDynTexture_Shadow::AdjustRealSize()
{
    if (m_eTT == eTT_2D)
    {
        if (m_nWidth < 256)
        {
            m_nWidth = 256;
        }
        else
        if (m_nWidth > 2048)
        {
            m_nWidth = 2048;
        }
        m_nHeight = m_nWidth;
    }
    if (m_eTT == eTT_Cube)
    {
        if (m_nWidth < 256)
        {
            m_nWidth = 256;
        }
        else
        if (m_nWidth > 512)
        {
            m_nWidth = 512;
        }
        m_nHeight = m_nWidth;
    }
}

SDynTexture_Shadow::~SDynTexture_Shadow()
{
    UnlinkShadow();
}

void SDynTexture_Shadow::RT_EntityDelete(IRenderNode* pRenderNode)
{
    // remove references to the entity
    SDynTexture_Shadow* pTX, * pNext;
    for (pTX = SDynTexture_Shadow::s_RootShadow.m_NextShadow; pTX != &SDynTexture_Shadow::s_RootShadow; pTX = pNext)
    {
        pNext = pTX->m_NextShadow;
        if (pTX->pLightOwner == pRenderNode)
        {
            delete pTX;
        }
    }
}

void SDynTexture_Shadow::ShutDown()
{
    SDynTexture_Shadow* pTX, * pNext;
    for (pTX = SDynTexture_Shadow::s_RootShadow.m_NextShadow; pTX != &SDynTexture_Shadow::s_RootShadow; pTX = pNext)
    {
        pNext = pTX->m_NextShadow;
        delete pTX;
    }
}

SDynTexture_Shadow* SDynTexture_Shadow::GetForFrustum(ShadowMapFrustum* pFrustum)
{
    SDynTexture_Shadow* pDynTX = NULL;

    for (SDynTexture_Shadow* pTX = SDynTexture_Shadow::s_RootShadow.m_NextShadow; pTX != &SDynTexture_Shadow::s_RootShadow; pTX = pTX->m_NextShadow)
    {
        if (pTX->m_pFrustumOwner == pFrustum->pFrustumOwner)
        {
            pDynTX = pTX;
            break;
        }
    }

    if (pDynTX)
    {
        if (pDynTX->m_eTF != pFrustum->m_eReqTF || pDynTX->m_eTT != pFrustum->m_eReqTT || pDynTX->pLightOwner != pFrustum->pLightOwner ||
            pDynTX->m_nReqWidth != pFrustum->nTextureWidth ||  pDynTX->m_nReqHeight != pFrustum->nTextureHeight)
        {
            SAFE_DELETE(pDynTX);

            //force all cubemap faces update
            pFrustum->RequestUpdate();
        }
    }

    //check after freeing texture
    if (!pDynTX)
    {
        uint32_t flags = FT_USAGE_DEPTHSTENCIL | FT_STATE_CLAMP | FT_DONT_STREAM | FT_USE_HTILE;
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION SDYNTEXTURE_CPP_SECTION_1
    #include AZ_RESTRICTED_FILE(DynTexture_cpp)
#endif
        pDynTX = new SDynTexture_Shadow(pFrustum->nTextureWidth, pFrustum->nTextureHeight, pFrustum->m_eReqTF, pFrustum->m_eReqTT, flags, "ShadowRT");
        CRY_ASSERT(pFrustum->nTextureWidth == pDynTX->m_nWidth && pFrustum->nTextureHeight == pDynTX->m_nHeight);
    }

    pDynTX->RT_Update(pFrustum->nTextureWidth, pFrustum->nTextureHeight);
    pDynTX->pLightOwner = pFrustum->pLightOwner;
    pDynTX->m_pFrustumOwner = pFrustum->pFrustumOwner;

    return pDynTX;
}
//====================================================================================

AZStd::unique_ptr<SDynTexture2::TextureSet2> SDynTexture2::s_TexturePool[eTP_Max];

SDynTexture2::SDynTexture2(const char* szSource, ETexPool eTexPool)
{
    m_nWidth = 0;
    m_nHeight = 0;

    m_pOwner = NULL;

#ifndef _DEBUG
    m_sSource = (char*)szSource;
#else
    azstrcpy(m_sSource, strlen(szSource) + 1, szSource);
#endif
    m_bLocked = false;

    m_eTexPool = eTexPool;

    m_nBlockID = ~0;
    m_pAllocator = NULL;
    m_Next = NULL;
    m_PrevLink = NULL;
    m_nUpdateMask = 0;
    if (gRenDev)
    {
        m_nFrameReset = gRenDev->m_nFrameReset;
    }
    SetUpdateMask();

    m_nFlags = 0;
}

void SDynTexture2::SetUpdateMask()
{
    if (gRenDev)
    {
        int nFrame = gRenDev->RT_GetCurrGpuID();
        m_nUpdateMask |= 1 << nFrame;
    }
}

void SDynTexture2::ResetUpdateMask()
{
    m_nUpdateMask = 0;
    if (gRenDev)
    {
        m_nFrameReset = gRenDev->m_nFrameReset;
    }
}

SDynTexture2::SDynTexture2(uint32 nWidth, uint32 nHeight, [[maybe_unused]] uint32 nTexFlags, const char* szSource, ETexPool eTexPool)
{
    m_nWidth = nWidth;
    m_nHeight = nHeight;

    m_eTexPool = eTexPool;

    ETEX_Format eTF = GetPoolTexFormat(eTexPool);

    TextureSet2Itor tset = s_TexturePool[eTexPool]->find(eTF);
    if (tset == s_TexturePool[eTexPool]->end())
    {
        m_pOwner = new STextureSetFormat(eTF, eTexPool, FT_NOMIPS | FT_USAGE_ATLAS);
        s_TexturePool[eTexPool]->insert(TextureSet2::value_type(eTF, m_pOwner));
    }
    else
    {
        m_pOwner = tset->second;
    }

    m_Next = NULL;
    m_PrevLink = NULL;
    m_bLocked = false;

#ifndef _DEBUG
    m_sSource = (char*)szSource;
#else
    azstrcpy(m_sSource, strlen(szSource) + 1, szSource);
#endif

    m_pTexture = NULL;
    m_nBlockID = ~0;
    m_pAllocator = NULL;
    m_nUpdateMask = 0;
    if (gRenDev)
    {
        m_nFrameReset = gRenDev->m_nFrameReset;
    }
    SetUpdateMask();
    m_nFlags = 0;
}

SDynTexture2::~SDynTexture2()
{
    Remove();
    m_bLocked = false;
}

int SDynTexture2::GetPoolMaxSize(ETexPool eTexPool)
{
    if (eTexPool == eTP_Clouds)
    {
        return SDynTexture::s_SuggestedDynTexAtlasCloudsMaxsize;
    }
    assert(0);
    return 0;
}
void SDynTexture2::SetPoolMaxSize(ETexPool eTexPool, int nSize, bool bWarn)
{
    if (eTexPool == eTP_Clouds)
    {
        if (bWarn)
        {
            Warning("Increasing maximum Clouds atlas pool to %d Mb", nSize);
        }
        SDynTexture::s_SuggestedDynTexAtlasCloudsMaxsize = nSize;
    }
    else
    {
        assert(0);
    }
}

const char* SDynTexture2::GetPoolName(ETexPool eTexPool)
{
    if (eTexPool == eTP_Clouds)
    {
        return "Clouds";
    }

    assert(0);
    return 0;
}

ETEX_Format SDynTexture2::GetPoolTexFormat(ETexPool eTexPool)
{
    if (eTexPool == eTP_Clouds)
    {
        return eTF_R8G8B8A8;
    }

    assert(0);
    return eTF_R8G8B8A8;
}

void SDynTexture2::Init(ETexPool eTexPool)
{
    if (!s_TexturePool[eTexPool])
    {
        s_TexturePool[eTexPool] = AZStd::make_unique<TextureSet2>();
    }
    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_texpreallocateatlases)
    {
        int nSize = CRenderer::CV_r_texatlassize;
        TArray<SDynTexture2*> Texs;
        int nMaxSize = GetPoolMaxSize(eTexPool);
        const char* szName = GetPoolName(eTexPool);
        nMaxSize *= 1024 * 1024;
        while (true)
        {
            ETEX_Format eTF = GetPoolTexFormat(eTexPool);
            int nNeedSpace = CTexture::TextureDataSize(nSize, nSize, 1, 1, 1, eTF);
            if (nNeedSpace + s_nMemoryOccupied[eTexPool] > nMaxSize)
            {
                break;
            }
            SDynTexture2* pTex = new SDynTexture2(nSize, nSize,  FT_STATE_CLAMP | FT_NOMIPS, szName, eTexPool);
            pTex->Update(nSize, nSize);
            Texs.AddElem(pTex);
        }
        for (uint32 i = 0; i < Texs.Num(); i++)
        {
            SDynTexture2* pTex = Texs[i];
            SAFE_DELETE(pTex);
        }
    }
}

bool SDynTexture2::UpdateAtlasSize(int nNewWidth, int nNewHeight)
{
    if (!m_pOwner)
    {
        return false;
    }
    if (!m_pTexture)
    {
        return false;
    }
    if (!m_pAllocator)
    {
        return false;
    }
    if (m_pTexture->GetWidth() != nNewWidth || m_pTexture->GetHeight() != nNewHeight)
    {
        SDynTexture2* pDT, * pNext;
        for (pDT = m_pOwner->m_pRoot; pDT; pDT = pNext)
        {
            pNext = pDT->m_Next;
            if (pDT == this)
            {
                continue;
            }
            assert(!pDT->m_bLocked);
            pDT->Remove();
            pDT->SetUpdateMask();
        }
        int nBlockW = (m_nWidth + TEX_POOL_BLOCKSIZE - 1) / TEX_POOL_BLOCKSIZE;
        int nBlockH = (m_nHeight + TEX_POOL_BLOCKSIZE - 1) / TEX_POOL_BLOCKSIZE;
        m_pAllocator->RemoveBlock(m_nBlockID);
        assert (m_pAllocator && m_pAllocator->GetNumUsedBlocks() == 0);

        int nW = (nNewWidth + TEX_POOL_BLOCKSIZE - 1) / TEX_POOL_BLOCKSIZE;
        int nH = (nNewHeight + TEX_POOL_BLOCKSIZE - 1) / TEX_POOL_BLOCKSIZE;
        m_pAllocator->UpdateSize(nW, nH);
        m_nBlockID = m_pAllocator->AddBlock(nBlockW, nBlockH);
        s_nMemoryOccupied[m_eTexPool] -= CTexture::TextureDataSize(m_pTexture->GetWidth(), m_pTexture->GetHeight(), 1, 1, 1, m_pOwner->m_eTF);
        SAFE_RELEASE(m_pTexture);

        char name[256];
        sprintf_s(name, "$Dyn_2D_%s_%s_%d", CTexture::NameForTextureFormat(m_pOwner->m_eTF), GetPoolName(m_eTexPool), gRenDev->m_TexGenID++);
        m_pAllocator->m_pTexture = CTexture::CreateRenderTarget(name, nNewWidth, nNewHeight, Clr_Transparent, m_pOwner->m_eTT, m_pOwner->m_nTexFlags, m_pOwner->m_eTF);
        s_nMemoryOccupied[m_eTexPool] += CTexture::TextureDataSize(nNewWidth, nNewHeight, 1, 1, 1, m_pOwner->m_eTF);
        m_pTexture = m_pAllocator->m_pTexture;
    }
    return true;
}

bool SDynTexture2::Update(int nNewWidth, int nNewHeight)
{
    int i;
    if (!m_pOwner)
    {
        return false;
    }
    bool bRecreate = false;
    if (!m_pAllocator)
    {
        bRecreate = true;
    }
    int nStage = -1;
    m_nAccessFrame = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_nFrameUpdateID;
    SDynTexture2* pDT = NULL;

    if (m_nWidth != nNewWidth || m_nHeight != nNewHeight)
    {
        bRecreate = true;
        m_nWidth = nNewWidth;
        m_nHeight = nNewHeight;
    }
    uint32 nFrame = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_nFrameUpdateID;
    if (bRecreate)
    {
        int nSize = CRenderer::CV_r_texatlassize;
        if (nSize <= 512)
        {
            nSize = 512;
        }
        else
        if (nSize <= 1024)
        {
            nSize = 1024;
        }
        else
        if (nSize > 2048)
        {
            nSize = 2048;
        }
        CRenderer::CV_r_texatlassize = nSize;
        int nMaxSize = GetPoolMaxSize(m_eTexPool);
        int nNeedSpace = CTexture::TextureDataSize(nSize, nSize, 1, 1, 1, m_pOwner->m_eTF);
        if (nNeedSpace > nMaxSize * 1024 * 1024)
        {
            SetPoolMaxSize(m_eTexPool, nNeedSpace / (1024 * 1024), true);
        }

        int nBlockW = (nNewWidth + TEX_POOL_BLOCKSIZE - 1) / TEX_POOL_BLOCKSIZE;
        int nBlockH = (nNewHeight + TEX_POOL_BLOCKSIZE - 1) / TEX_POOL_BLOCKSIZE;
        Remove();
        SetUpdateMask();

        uint32 nID = ~0;
        CPowerOf2BlockPacker* pPack = NULL;
        for (i = 0; i < (int)m_pOwner->m_TexPools.size(); i++)
        {
            pPack = m_pOwner->m_TexPools[i];
            nID = pPack->AddBlock(LogBaseTwo(nBlockW), LogBaseTwo(nBlockH));
            if (nID != -1)
            {
                break;
            }
        }
        if (i == m_pOwner->m_TexPools.size())
        {
            nStage = 1;
            nMaxSize = GetPoolMaxSize(m_eTexPool);
            if (nNeedSpace + s_nMemoryOccupied[m_eTexPool] > nMaxSize * 1024 * 1024)
            {
                SDynTexture2* pDTBest = NULL;
                SDynTexture2* pDTBestLarge = NULL;
                int nError = 1000000;
                uint32 nFr = INT_MAX;
                uint32 nFrLarge = INT_MAX;
                int n = 0;
                for (pDT = m_pOwner->m_pRoot; pDT; pDT = pDT->m_Next)
                {
                    if (pDT == this || pDT->m_bLocked)
                    {
                        continue;
                    }
                    n++;
                    assert (pDT->m_pAllocator && pDT->m_pTexture && pDT->m_nBlockID != -1);
                    if (pDT->m_nWidth == m_nWidth && pDT->m_nHeight == m_nHeight)
                    {
                        if (nFr > pDT->m_nAccessFrame)
                        {
                            nFr = pDT->m_nAccessFrame;
                            pDTBest = pDT;
                        }
                    }
                    else
                    if (pDT->m_nWidth >= m_nWidth && pDT->m_nHeight >= m_nHeight)
                    {
                        int nEr = pDT->m_nWidth - m_nWidth + pDT->m_nHeight - m_nHeight;
                        int fEr = nEr + (nFrame - pDT->m_nAccessFrame);

                        if (fEr < nError)
                        {
                            nFrLarge = pDT->m_nAccessFrame;
                            nError = fEr;
                            pDTBestLarge = pDT;
                        }
                    }
                }
                pDT = NULL;
                if (pDTBest && nFr + 1 < nFrame && pDTBest->m_nBlockID != -1)
                {
                    nStage = 2;
                    pDT = pDTBest;
                    nID = pDT->m_nBlockID;
                    pPack = pDT->m_pAllocator;
                    pDT->m_pAllocator = NULL;
                    pDT->m_pTexture = NULL;
                    pDT->m_nBlockID = ~0;
                    pDT->m_nUpdateMask = 0;
                    pDT->SetUpdateMask();
                    pDT->Unlink();
                }
                else
                if (pDTBestLarge && nFrLarge + 1 < nFrame)
                {
                    nStage = 3;
                    pDT = pDTBestLarge;
                    CPowerOf2BlockPacker* pAllocator = pDT->m_pAllocator;
                    pDT->Remove();
                    pDT->SetUpdateMask();
                    nID = pAllocator->AddBlock(LogBaseTwo(nBlockW), LogBaseTwo(nBlockH));
                    assert (nID != -1);
                    if (nID != -1)
                    {
                        pPack = pAllocator;
                    }
                    else
                    {
                        pDT = NULL;
                    }
                }
                if (!pDT)
                {
                    nStage = 4;
                    // Try to find oldest texture pool
                    float fTime = FLT_MAX;
                    CPowerOf2BlockPacker* pPackBest = NULL;
                    for (i = 0; i < (int)m_pOwner->m_TexPools.size(); i++)
                    {
                        CPowerOf2BlockPacker* pNextPack = m_pOwner->m_TexPools[i];
                        if (fTime > pNextPack->m_fLastUsed)
                        {
                            fTime = pNextPack->m_fLastUsed;
                            pPackBest = pNextPack;
                        }
                    }
                    if (!pPackBest || fTime + 0.5f > gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_RealTime)
                    {
                        nStage = 5;
                        // Try to find most fragmented texture pool with less number of blocks
                        uint32 nUsedBlocks = TEX_POOL_BLOCKSIZE * TEX_POOL_BLOCKSIZE + 1;
                        pPackBest = NULL;
                        for (i = 0; i < (int)m_pOwner->m_TexPools.size(); i++)
                        {
                            CPowerOf2BlockPacker* pNextPack = m_pOwner->m_TexPools[i];
                            int nBlocks = pNextPack->GetNumUsedBlocks();
                            if (nBlocks < (int)nUsedBlocks)
                            {
                                nUsedBlocks = nBlocks;
                                pPackBest = pNextPack;
                            }
                        }
                    }
                    if (pPackBest)
                    {
                        SDynTexture2* pNext = NULL;
                        for (pDT = m_pOwner->m_pRoot; pDT; pDT = pNext)
                        {
                            pNext = pDT->m_Next;
                            if (pDT == this || pDT->m_bLocked)
                            {
                                continue;
                            }
                            if (pDT->m_pAllocator == pPackBest)
                            {
                                pDT->Remove();
                            }
                        }
                        assert (pPackBest->GetNumUsedBlocks() == 0);
                        pPack = pPackBest;
                        nID = pPack->AddBlock(LogBaseTwo(nBlockW), LogBaseTwo(nBlockH));
                        pPack->m_fLastUsed = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_RealTime;
                        if (nID != -1)
                        {
                            pDT = this;
                            pDT->m_nUpdateMask = 0;
                        }
                        else
                        {
                            assert(0);
                            pDT = NULL;
                        }
                    }
                    else
                    {
                        nStage = 6;
                        assert(0); // there was no free spot in the texture pool - either the pools are too small or there are too many requests or reuse is not possible because some of them are not released when they could be
                    }
                }
            }

            if (!pDT)
            {
                nStage |= 0x100;
                int n = (nSize + TEX_POOL_BLOCKSIZE - 1) / TEX_POOL_BLOCKSIZE;
                pPack = new CPowerOf2BlockPacker(LogBaseTwo(n), LogBaseTwo(n));
                m_pOwner->m_TexPools.push_back(pPack);
                nID = pPack->AddBlock(LogBaseTwo(nBlockW), LogBaseTwo(nBlockH));
                char name[256];
                sprintf_s(name, "$Dyn_2D_%s_%s_%d", CTexture::NameForTextureFormat(m_pOwner->m_eTF), GetPoolName(m_eTexPool), gRenDev->m_TexGenID++);
                pPack->m_pTexture = CTexture::CreateRenderTarget(name, nSize, nSize, Clr_Transparent, m_pOwner->m_eTT, m_pOwner->m_nTexFlags, m_pOwner->m_eTF);
                s_nMemoryOccupied[m_eTexPool] += nNeedSpace;
                if (nID == -1)
                {
                    assert(0);
                    nID = (uint32) - 2;
                }
            }
        }
        assert(nID != -1 && nID != -2);
        m_nBlockID = nID;
        m_pAllocator = pPack;
        if (pPack)
        {
            m_pTexture = pPack->m_pTexture;
            uint32 nX1, nX2, nY1, nY2;
            pPack->GetBlockInfo(nID, nX1, nY1, nX2, nY2);
            m_nX = nX1 << TEX_POOL_BLOCKLOGSIZE;
            m_nY = nY1 << TEX_POOL_BLOCKLOGSIZE;
            m_nWidth  = (nX2 - nX1) << TEX_POOL_BLOCKLOGSIZE;
            m_nHeight = (nY2 - nY1) << TEX_POOL_BLOCKLOGSIZE;
            m_nUpdateMask = 0;
            SetUpdateMask();
        }
    }
    PREFAST_ASSUME(m_pAllocator);
    m_pAllocator->m_fLastUsed = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_RealTime;
    Unlink();
    if (m_pTexture)
    {
        Link();
        return true;
    }
    return false;
}

bool SDynTexture2::Remove()
{
    if (!m_pAllocator)
    {
        return false;
    }
    if (m_nBlockID != -1)
    {
        m_pAllocator->RemoveBlock(m_nBlockID);
    }
    m_nBlockID = ~0;
    m_pTexture = NULL;
    m_nUpdateMask = 0;
    m_pAllocator = NULL;
    Unlink();

    return true;
}

void SDynTexture2::Apply(int nTUnit, int nTS)
{
    if (!m_pAllocator)
    {
        return;
    }
    m_pAllocator->m_fLastUsed = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_RealTime;

    if (!m_pTexture)
    {
        Update(m_nWidth, m_nHeight);
    }
    if (m_pTexture)
    {
        m_pTexture->ApplyTexture(nTUnit, nTS);
    }
    gRenDev->m_cEF.m_RTRect.x = (float)m_nX / (float)m_pTexture->GetWidth();
    gRenDev->m_cEF.m_RTRect.y = (float)m_nY / (float)m_pTexture->GetHeight();
    gRenDev->m_cEF.m_RTRect.z = (float)m_nWidth / (float)m_pTexture->GetWidth();
    gRenDev->m_cEF.m_RTRect.w = (float)m_nHeight / (float)m_pTexture->GetHeight();
}

bool SDynTexture2::IsValid()
{
    if (!m_pTexture)
    {
        return false;
    }
    CRenderer* rd = gRenDev;
    m_nAccessFrame = rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_nFrameUpdateID;
    if (m_nFrameReset != rd->m_nFrameReset)
    {
        m_nFrameReset = rd->m_nFrameReset;
        m_nUpdateMask = 0;
        return false;
    }
    if (rd->GetActiveGPUCount() > 1)
    {
        if ((rd->GetFeatures() & RFT_HW_MASK) == RFT_HW_ATI)
        {
            uint32 nX, nY, nW, nH;
            GetImageRect(nX, nY, nW, nH);
            if (nW < 1024 && nH < 1024)
            {
                return true;
            }
        }

        int nFrame = rd->RT_GetCurrGpuID();
        if (!((1 << nFrame) & m_nUpdateMask))
        {
            return false;
        }
    }
    return true;
}

void SDynTexture2::ReleaseForce()
{
    delete this;
}

void SDynTexture2::ShutDown()
{
    uint32 i;
    for (i = 0; i < eTP_Max; i++)
    {
        if (s_TexturePool[i])
        {
            for (TextureSet2Itor it = s_TexturePool[i]->begin(); it != s_TexturePool[i]->end(); it++)
            {
                STextureSetFormat* pF = it->second;
                PREFAST_ASSUME(pF);
                SDynTexture2* pDT, *pNext;
                for (pDT = pF->m_pRoot; pDT; pDT = pNext)
                {
                    pNext = pDT->m_Next;
                    SAFE_RELEASE_FORCE(pDT);
                }
                SAFE_DELETE(pF);
            }
            s_TexturePool[i]->clear();
        }
    }
}

STextureSetFormat::~STextureSetFormat()
{
    uint32 i;

    for (i = 0; i < m_TexPools.size(); i++)
    {
        CPowerOf2BlockPacker* pP = m_TexPools[i];
        PREFAST_ASSUME(pP);
        if (pP->m_pTexture)
        {
            int nSize = CTexture::TextureDataSize(pP->m_pTexture->GetWidth(), pP->m_pTexture->GetHeight(), 1, 1, 1, pP->m_pTexture->GetTextureDstFormat());
            SDynTexture2::s_nMemoryOccupied[m_eTexPool] = max(0, SDynTexture2::s_nMemoryOccupied[m_eTexPool] - nSize);
        }

        SAFE_DELETE(pP);
    }
    m_TexPools.clear();
}
