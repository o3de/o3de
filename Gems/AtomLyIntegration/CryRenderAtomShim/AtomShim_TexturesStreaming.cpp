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

#include "CryRenderOther_precompiled.h"
#include "AtomShim_Renderer.h"
#include "../Common/Textures/TextureStreamPool.h"

//===============================================================================

bool STexPoolItem::IsStillUsedByGPU([[maybe_unused]] uint32 nCurTick)
{
    return false;
}

STexPoolItem::~STexPoolItem()
{
}

void CTexture::InitStreamingDev()
{
}

void CTexture::StreamExpandMip([[maybe_unused]] const void* pRawData, [[maybe_unused]] int nMip, [[maybe_unused]] int nBaseMipOffset, [[maybe_unused]] int nSideDelta)
{
}

void CTexture::StreamCopyMipsTexToTex([[maybe_unused]] STexPoolItem* pSrcItem, [[maybe_unused]] int nMipSrc, [[maybe_unused]] STexPoolItem* pDestItem, [[maybe_unused]] int nMipDest, [[maybe_unused]] int nNumMips)
{
}

bool CTexture::StreamPrepare_Platform()
{
    return true;
}

int CTexture::StreamTrim([[maybe_unused]] int nToMip)
{
    return 0;
}

// Just remove item from the texture object and keep Item in Pool list for future use
// This function doesn't release API texture
void CTexture::StreamRemoveFromPool()
{
}

void CTexture::StreamCopyMipsTexToMem([[maybe_unused]] int nStartMip, [[maybe_unused]] int nEndMip, [[maybe_unused]] bool bToDevice, [[maybe_unused]] STexPoolItem* pNewPoolItem)
{
}

STexPoolItem* CTexture::StreamGetPoolItem([[maybe_unused]] int nStartMip, [[maybe_unused]] int nMips, [[maybe_unused]] bool bShouldBeCreated, [[maybe_unused]] bool bCreateFromMipData, [[maybe_unused]] bool bCanCreate, [[maybe_unused]] bool bForStreamOut)
{
    return NULL;
}

void CTexture::StreamAssignPoolItem([[maybe_unused]] STexPoolItem* pItem, [[maybe_unused]] int nMinMip)
{
}


CTextureStreamPoolMgr::CTextureStreamPoolMgr()
{
}

CTextureStreamPoolMgr::~CTextureStreamPoolMgr()
{
}

void CTextureStreamPoolMgr::Flush()
{
}

STexPoolItem* CTextureStreamPoolMgr::GetPoolItem([[maybe_unused]] int nWidth, [[maybe_unused]] int nHeight, [[maybe_unused]] int nArraySize, [[maybe_unused]] int nMips, [[maybe_unused]] ETEX_Format eTF, [[maybe_unused]] bool bIsSRGB, [[maybe_unused]] ETEX_Type eTT, [[maybe_unused]] bool bShouldBeCreated, [[maybe_unused]] const char* sName, [[maybe_unused]] STextureInfo* pTI, [[maybe_unused]] bool bCanCreate, [[maybe_unused]] bool bWaitForIdle)
{
    return NULL;
}

void CTextureStreamPoolMgr::ReleaseItem([[maybe_unused]] STexPoolItem* pItem)
{
}

void CTextureStreamPoolMgr::GarbageCollect([[maybe_unused]] size_t* nCurTexPoolSize, [[maybe_unused]] size_t nLowerPoolLimit, [[maybe_unused]] int nMaxItemsToFree)
{
}

void CTextureStreamPoolMgr::GetMemoryUsage([[maybe_unused]] ICrySizer* pSizer)
{
}
