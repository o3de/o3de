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

//=============================================================================


#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define DEVICEMANAGER_D3D11_INL_SECTION_1 1
#define DEVICEMANAGER_D3D11_INL_SECTION_2 2
#define DEVICEMANAGER_D3D11_INL_SECTION_3 3
#define DEVICEMANAGER_D3D11_INL_SECTION_4 4
#define DEVICEMANAGER_D3D11_INL_SECTION_5 5
#define DEVICEMANAGER_D3D11_INL_SECTION_6 6
#define DEVICEMANAGER_D3D11_INL_SECTION_7 7
#define DEVICEMANAGER_D3D11_INL_SECTION_8 8
#endif

#ifdef DEVMAN_USE_STAGING_POOL
D3DResource* CDeviceManager::AllocateStagingResource(D3DResource* pForTex, bool bUpload)
{
    D3D11_TEXTURE2D_DESC Desc;
    memset(&Desc, 0, sizeof(Desc));
    ((D3DTexture*)pForTex)->GetDesc(&Desc);

    // If we're creating download resources or have multiple mips where we'll need to write to one mip at a time,
    // we'll need a resource that we can read back to the CPU.  We can only use write-only for upload resources
    // with a single mip level.
    bool canUseWriteOnly = bUpload && (Desc.MipLevels == 1);
    
    Desc.Usage          = canUseWriteOnly ? D3D11_USAGE_DYNAMIC        : D3D11_USAGE_STAGING;
    Desc.CPUAccessFlags = bUpload         ? D3D11_CPU_ACCESS_WRITE     : D3D11_CPU_ACCESS_READ;
    Desc.BindFlags      = canUseWriteOnly ? D3D11_BIND_SHADER_RESOURCE : 0;


#if defined(AZ_PLATFORM_MAC)
    // For metal when we do a subresource copy we render to the texture so need to set the render target flag
    Desc.BindFlags |= D3D11_BIND_RENDER_TARGET; //todo: Remove this for mac too
#endif

    // BindFlags play a part in matching the descriptions. Only search after we have finished
    // modifying the description.
    StagingPoolVec::iterator it = std::find(m_stagingPool.begin(), m_stagingPool.end(), Desc);

    if (it == m_stagingPool.end())
    {
        D3DTexture* pStagingTexture = NULL;

        gcpRendD3D->GetDevice().CreateTexture2D(&Desc, NULL, &pStagingTexture);

#ifndef _RELEASE
        if (pStagingTexture)
        {
            D3D11_TEXTURE2D_DESC stagingDesc;
            memset(&stagingDesc, 0, sizeof(stagingDesc));

            pStagingTexture->GetDesc(&stagingDesc);
            stagingDesc.Usage          = canUseWriteOnly ? D3D11_USAGE_DYNAMIC        : D3D11_USAGE_STAGING;
            stagingDesc.CPUAccessFlags = bUpload         ? D3D11_CPU_ACCESS_WRITE     : D3D11_CPU_ACCESS_READ;
            stagingDesc.BindFlags      = canUseWriteOnly ? D3D11_BIND_SHADER_RESOURCE : 0;

#if defined(AZ_PLATFORM_MAC)
            // For metal when we do a subresource copy we render to the texture so need to set the render target flag
            stagingDesc.BindFlags |= D3D11_BIND_RENDER_TARGET; //todo: Remove this for mac too
#endif

            if (memcmp(&stagingDesc, &Desc, sizeof(Desc)) != 0)
            {
                __debugbreak();
            }
        }
#endif

        return pStagingTexture;
    }
    else
    {
        D3DTexture* pStagingResource = NULL;

        pStagingResource = it->pStagingTexture;
        m_stagingPool.erase(it);

        return pStagingResource;
    }
}

void CDeviceManager::ReleaseStagingResource(D3DResource* pStagingRes)
{
    D3D11_TEXTURE2D_DESC Desc;
    memset(&Desc, 0, sizeof(Desc));

    ((D3DTexture*)pStagingRes)->GetDesc(&Desc);

    StagingTextureDef def;
    def.desc = Desc;
    def.pStagingTexture = (D3DTexture*)pStagingRes;
    m_stagingPool.push_back(def);
}
#endif

//=============================================================================

int32 CDeviceTexture::Release()
{
    int32 nRef = Cleanup();

    if (nRef <= 0 && !m_bNoDelete)
    {
        delete this;
    }

    return nRef;
}

void CDeviceTexture::Unbind()
{
    for (uint32 i = 0; i < MAX_TMU; i++)
    {
        if (CTexture::s_TexStages[i].m_DevTexture == this)
        {
            CTexture::s_TexStages[i].m_DevTexture = NULL;

            ID3D11ShaderResourceView* RV = NULL;
            gcpRendD3D->GetDeviceContext().PSSetShaderResources(i, 1, &RV);
        }
    }
}

void CDeviceTexture::DownloadToStagingResource(uint32 nSubRes, StagingHook cbTransfer)
{
    AZ_Assert(nSubRes < m_numSubResources, "Invalid SubResource ID %d, (should be < %d)", nSubRes, m_numSubResources);
    D3DResource* pStagingResource;
    if (!(pStagingResource = GetCurrDownloadStagingResource()))
    {
        pStagingResource = gcpRendD3D->m_DevMan.AllocateStagingResource(m_pD3DTexture, FALSE);
    }

    AZ_Assert(pStagingResource, "Null download staging resource");

#if defined(DEVICE_SUPPORTS_D3D11_1)
    gcpRendD3D->GetDeviceContext().CopySubresourceRegion1(pStagingResource, nSubRes, 0, 0, 0, m_pD3DTexture, nSubRes, NULL, D3D11_COPY_NO_OVERWRITE);
#else
    gcpRendD3D->GetDeviceContext().CopySubresourceRegion (pStagingResource, nSubRes, 0, 0, 0, m_pD3DTexture, nSubRes, NULL);
#endif

    D3D11_MAPPED_SUBRESOURCE lrct;
    HRESULT hr = gcpRendD3D->GetDeviceContext().Map(pStagingResource, nSubRes, D3D11_MAP_READ, 0, &lrct);

    if (S_OK == hr)
    {
        const bool update = cbTransfer(lrct.pData, lrct.RowPitch, lrct.DepthPitch);
        gcpRendD3D->GetDeviceContext().Unmap(pStagingResource, nSubRes);
    }

    if (!(GetCurrDownloadStagingResource()))
    {
        gcpRendD3D->m_DevMan.ReleaseStagingResource(pStagingResource);
    }
}

void CDeviceTexture::DownloadToStagingResource(uint32 nSubRes)
{
    AZ_Assert(nSubRes < m_numSubResources, "Invalid SubResource ID %d, (should be < %d)", nSubRes, m_numSubResources);
    AZ_Assert(GetCurrDownloadStagingResource(), "Null download staging resource");

#if defined(DEVICE_SUPPORTS_D3D11_1)
    gcpRendD3D->GetDeviceContext().CopySubresourceRegion1(GetCurrDownloadStagingResource(), nSubRes, 0, 0, 0, m_pD3DTexture, nSubRes, NULL, D3D11_COPY_NO_OVERWRITE);
#else
    gcpRendD3D->GetDeviceContext().CopySubresourceRegion(GetCurrDownloadStagingResource(), nSubRes, 0, 0, 0, m_pD3DTexture, nSubRes, NULL);
#endif
}

void CDeviceTexture::DownloadToStagingResource()
{
    AZ_Assert(GetCurrDownloadStagingResource(), "Null download staging resource");
    gcpRendD3D->GetDeviceContext().CopyResource(GetCurrDownloadStagingResource(), Get2DTexture());
}

void CDeviceTexture::UploadFromStagingResource(const uint32 nSubRes, StagingHook cbTransfer)
{
    AZ_Assert(nSubRes < m_numSubResources, "Invalid SubResource ID %d, (should be < %d)", nSubRes, m_numSubResources);
    D3DResource* pStagingResource;
    if (!(pStagingResource = GetCurrUploadStagingResource()))
    {
        pStagingResource = gcpRendD3D->m_DevMan.AllocateStagingResource(m_pD3DTexture, TRUE);
    }

    AZ_Assert(pStagingResource, "Null upload staging resource");

    // If we have more than one mip, we'll need to do partial writes, so we can't just discard the previous GPU data.
    D3D11_MAPPED_SUBRESOURCE lrct;
    HRESULT hr = gcpRendD3D->GetDeviceContext().Map(pStagingResource, nSubRes, (m_numSubResources > 1) ? D3D11_MAP_WRITE : D3D11_MAP_WRITE_DISCARD, 0, &lrct);

    if (S_OK == hr)
    {
        const bool update = cbTransfer(lrct.pData, lrct.RowPitch, lrct.DepthPitch);
        gcpRendD3D->GetDeviceContext().Unmap(pStagingResource, nSubRes);
        if (update)
        {
#if defined(DEVICE_SUPPORTS_D3D11_1)
            gcpRendD3D->GetDeviceContext().CopySubresourceRegion1(m_pD3DTexture, nSubRes, 0, 0, 0, pStagingResource, nSubRes, NULL, D3D11_COPY_NO_OVERWRITE);
#else
            gcpRendD3D->GetDeviceContext().CopySubresourceRegion (m_pD3DTexture, nSubRes, 0, 0, 0, pStagingResource, nSubRes, NULL);
#endif
        }
    }

    if (!GetCurrUploadStagingResource())
    {
        gcpRendD3D->m_DevMan.ReleaseStagingResource(pStagingResource);
    }
}

void CDeviceTexture::UploadFromStagingResource(const uint32 nSubRes)
{
    AZ_Assert(nSubRes < m_numSubResources, "Invalid SubResource ID %d, (should be < %d)", nSubRes, m_numSubResources);
    AZ_Assert(GetCurrUploadStagingResource(), "Null upload staging resource");

#if defined(DEVICE_SUPPORTS_D3D11_1)
    gcpRendD3D->GetDeviceContext().CopySubresourceRegion1(m_pD3DTexture, nSubRes, 0, 0, 0, GetCurrUploadStagingResource(), nSubRes, NULL, D3D11_COPY_NO_OVERWRITE);
#else
    gcpRendD3D->GetDeviceContext().CopySubresourceRegion(m_pD3DTexture, nSubRes, 0, 0, 0, GetCurrUploadStagingResource(), nSubRes, NULL);
#endif
}

void CDeviceTexture::UploadFromStagingResource()
{
    AZ_Assert(GetCurrUploadStagingResource(), "Null upload staging resource");
    gcpRendD3D->GetDeviceContext().CopyResource(Get2DTexture(), GetCurrUploadStagingResource());
}

void CDeviceTexture::AccessCurrStagingResource(uint32 nSubRes, bool forUpload, StagingHook cbTransfer)
{
    AZ_Assert(nSubRes < m_numSubResources, "Invalid SubResource ID %d, (should be < %d)", nSubRes, m_numSubResources);

    D3D11_MAPPED_SUBRESOURCE lrct;
    HRESULT hr = gcpRendD3D->GetDeviceContext().Map(GetCurrStagingResource(forUpload), nSubRes, forUpload ? D3D11_MAP_WRITE : D3D11_MAP_READ, D3D11_MAP_FLAG_DO_NOT_WAIT, &lrct);

    if (S_OK == hr)
    {
        const bool update = cbTransfer(lrct.pData, lrct.RowPitch, lrct.DepthPitch);
        gcpRendD3D->GetDeviceContext().Unmap(GetCurrStagingResource(forUpload), nSubRes);
    }
}


//=============================================================================

HRESULT CDeviceManager::Create2DTexture(const string& textureName, uint32 nWidth, uint32 nHeight, uint32 nMips, uint32 nArraySize, uint32 nUsage, [[maybe_unused]] const ColorF& cClearValue, D3DFormat Format, [[maybe_unused]] D3DPOOL Pool,
    LPDEVICETEXTURE* ppDevTexture, STextureInfo* pTI, [[maybe_unused]] bool bShouldBeCreated, [[maybe_unused]] int32 nESRAMOffset)
{
    HRESULT hr = S_OK;

    CDeviceTexture* pDeviceTexture = 0;//*/ new CDeviceTexture();
    D3DTexture* pD3DTex = NULL;

    uint32 nBindFlags = D3D11_BIND_SHADER_RESOURCE;
    if (nUsage & USAGE_DEPTH_STENCIL)
    {
        nBindFlags |= D3D11_BIND_DEPTH_STENCIL;
    }
    else if (nUsage & USAGE_RENDER_TARGET)
    {
        nBindFlags |= D3D11_BIND_RENDER_TARGET;
    }
    
#if defined(AZ_PLATFORM_IOS)
    if (nUsage & USAGE_MEMORYLESS)
    {
        nBindFlags |= D3D11_BIND_MEMORYLESS;
    }
#endif
    
    if (nUsage & USAGE_UNORDERED_ACCESS)
    {
        nBindFlags |= D3D11_BIND_UNORDERED_ACCESS;
    }
    
    uint32 nMiscFlags = 0;
    if (nUsage & USAGE_AUTOGENMIPS)
    {
        nMiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
    }

    D3D11_TEXTURE2D_DESC Desc;
    ZeroStruct(Desc);
    Desc.Width = nWidth;
    Desc.Height = nHeight;
    Desc.MipLevels = nMips;
    Desc.Format = Format;
    Desc.ArraySize = nArraySize;
    Desc.BindFlags = nBindFlags;
    Desc.CPUAccessFlags = (nUsage & USAGE_DYNAMIC) ? D3D11_CPU_ACCESS_WRITE : 0;

    Desc.Usage = (nUsage & USAGE_DYNAMIC) ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
    Desc.Usage = (nUsage & USAGE_STAGING) ? D3D11_USAGE_STAGING : Desc.Usage;

    Desc.SampleDesc.Count = pTI ? pTI->m_nMSAASamples : 1;
    Desc.SampleDesc.Quality = pTI ? pTI->m_nMSAAQuality : 0;
    // AntonK: supported only by DX11 feature set
    // needs to be enabled for pure DX11 context
    if (nUsage & USAGE_STREAMING)
    {
        nMiscFlags |= D3D11_RESOURCE_MISC_RESOURCE_CLAMP;
    }
    //if (nUsage & USAGE_STREAMING)
    //    Desc.CPUAccessFlags |= D3D11_CPU_ACCESS_READ;
    Desc.MiscFlags = nMiscFlags;

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DEVICEMANAGER_D3D11_INL_SECTION_1
    #include AZ_RESTRICTED_FILE(DeviceManager_D3D11_inl)
#endif

    D3D11_SUBRESOURCE_DATA* pSRD = NULL;
    D3D11_SUBRESOURCE_DATA SRD[20];
    if (pTI && pTI->m_pData)
    {
        pSRD = &SRD[0];
        for (int i = 0; i < nMips; i++)
        {
            SRD[i].pSysMem = pTI->m_pData[i].pSysMem;
            SRD[i].SysMemPitch = pTI->m_pData[i].SysMemPitch;
            SRD[i].SysMemSlicePitch = pTI->m_pData[i].SysMemSlicePitch;

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DEVICEMANAGER_D3D11_INL_SECTION_2
    #include AZ_RESTRICTED_FILE(DeviceManager_D3D11_inl)
#endif
        }
    }

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DEVICEMANAGER_D3D11_INL_SECTION_3
    #include AZ_RESTRICTED_FILE(DeviceManager_D3D11_inl)
#endif

    hr = gcpRendD3D->GetDevice().CreateTexture2D(&Desc, pSRD, &pD3DTex);

    if (SUCCEEDED(hr) && pDeviceTexture == 0)
    {
        pDeviceTexture = new CDeviceTexture();
    }

    if (SUCCEEDED(hr) && pD3DTex)
    {
        pDeviceTexture->m_pD3DTexture = pD3DTex;
        pDeviceTexture->m_numSubResources = nMips * nArraySize;
        if (!pDeviceTexture->m_nBaseAllocatedSize)
        {
            pDeviceTexture->m_nBaseAllocatedSize = CDeviceTexture::TextureDataSize(nWidth, nHeight, 1, nMips, 1, CTexture::TexFormatFromDeviceFormat(Format));

            // Register the VRAM allocation with the driller
            pDeviceTexture->TrackTextureMemory(nUsage, textureName.c_str());
        }

        if (nUsage & USAGE_STAGE_ACCESS)
        {
            if (nUsage & USAGE_CPU_READ)
            {
                pDeviceTexture->m_pStagingResourceDownload = gcpRendD3D->m_DevMan.AllocateStagingResource(pDeviceTexture->m_pD3DTexture, FALSE);
            }
            if (nUsage & USAGE_CPU_WRITE)
            {
                for (int i = 0; i < CDeviceTexture::NUM_UPLOAD_STAGING_RES; i++)
                {
                    pDeviceTexture->m_pStagingResourceUpload[i] = gcpRendD3D->m_DevMan.AllocateStagingResource(pDeviceTexture->m_pD3DTexture, TRUE);
                }
            }
        }
    }
    else
    {
        SAFE_DELETE(pDeviceTexture);
    }

    *ppDevTexture = pDeviceTexture;

    return hr;
}

HRESULT CDeviceManager::CreateCubeTexture(const string& textureName, uint32 nSize, uint32 nMips, uint32 nArraySize, uint32 nUsage, [[maybe_unused]] const ColorF& cClearValue, D3DFormat Format, [[maybe_unused]] D3DPOOL Pool, LPDEVICETEXTURE* ppDevTexture, STextureInfo* pTI, [[maybe_unused]] bool bShouldBeCreated)
{
    HRESULT hr = S_OK;

    CDeviceTexture* pDeviceTexture = 0;//new CDeviceTexture();
    D3DCubeTexture* pD3DTex = NULL;

    uint32 nBindFlags = D3D11_BIND_SHADER_RESOURCE;
    uint32 nMiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
    if (nUsage & USAGE_DEPTH_STENCIL)
    {
        nBindFlags |= D3D11_BIND_DEPTH_STENCIL;
    }
    else if (nUsage & USAGE_RENDER_TARGET)
    {
        nBindFlags |= D3D11_BIND_RENDER_TARGET;
    }
    
#if defined(AZ_PLATFORM_IOS)
    if (nUsage & USAGE_MEMORYLESS)
    {
        nBindFlags |= D3D11_BIND_MEMORYLESS;
    }
#endif
    
    if (nUsage & USAGE_AUTOGENMIPS)
    {
        nMiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
    }
    D3D11_TEXTURE2D_DESC Desc;
    ZeroStruct(Desc);
    Desc.Width = nSize;
    Desc.Height = nSize;
    Desc.MipLevels = nMips;
    Desc.Format = Format;
    Desc.ArraySize = nArraySize * 6;
    Desc.BindFlags |= nBindFlags;
    Desc.CPUAccessFlags = (nUsage & USAGE_DYNAMIC) ? D3D11_CPU_ACCESS_WRITE : 0;
    Desc.Usage = (nUsage & USAGE_DYNAMIC) ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
    Desc.SampleDesc.Count = 1;
    Desc.SampleDesc.Quality = 0;
    Desc.MiscFlags = nMiscFlags;
    // AntonK: supported only by DX11 feature set
    // needs to be enabled for pure DX11 context
    //if(nUsage & USAGE_STREAMING)
    //Desc.MiscFlags |= D3D11_RESOURCE_MISC_RESOURCE_CLAMP;

    D3D11_SUBRESOURCE_DATA* pSRD = NULL;
    D3D11_SUBRESOURCE_DATA SRD[g_nD3D10MaxSupportedSubres];
    if (pTI && pTI->m_pData)
    {
        pSRD = &SRD[0];
        for (int j = 0; j < 6; j++)
        {
            for (int i = 0; i < nMips; i++)
            {
                int nSubresInd = j * nMips + i;
                SRD[nSubresInd].pSysMem = pTI->m_pData[nSubresInd].pSysMem;
                SRD[nSubresInd].SysMemPitch = pTI->m_pData[nSubresInd].SysMemPitch;
                SRD[nSubresInd].SysMemSlicePitch = pTI->m_pData[nSubresInd].SysMemSlicePitch;

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DEVICEMANAGER_D3D11_INL_SECTION_4
    #include AZ_RESTRICTED_FILE(DeviceManager_D3D11_inl)
#endif
            }
        }
    }

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DEVICEMANAGER_D3D11_INL_SECTION_5
    #include AZ_RESTRICTED_FILE(DeviceManager_D3D11_inl)
#endif

    hr = gcpRendD3D->GetDevice().CreateTexture2D(&Desc, pSRD, &pD3DTex);

    if (SUCCEEDED(hr) && pDeviceTexture == 0)
    {
        pDeviceTexture = new CDeviceTexture();
    }

    if (SUCCEEDED(hr) && pD3DTex)
    {
        pDeviceTexture->m_pD3DTexture = pD3DTex;
        pDeviceTexture->m_bCube = true;
        pDeviceTexture->m_numSubResources = nMips * nArraySize;
        if (!pDeviceTexture->m_nBaseAllocatedSize)
        {
            pDeviceTexture->m_nBaseAllocatedSize = CDeviceTexture::TextureDataSize(nSize, nSize, 1, nMips, 1, CTexture::TexFormatFromDeviceFormat(Format)) * 6;

            // Register the VRAM allocation with the driller
            pDeviceTexture->TrackTextureMemory(nUsage, textureName.c_str());
        }

        if (nUsage & USAGE_STAGE_ACCESS)
        {
            if (nUsage & USAGE_CPU_READ)
            {
                pDeviceTexture->m_pStagingResourceDownload = gcpRendD3D->m_DevMan.AllocateStagingResource(pDeviceTexture->m_pD3DTexture, FALSE);
            }
            if (nUsage & USAGE_CPU_WRITE)
            {
                for (int i = 0; i < CDeviceTexture::NUM_UPLOAD_STAGING_RES; i++)
                {
                    pDeviceTexture->m_pStagingResourceUpload[i] = gcpRendD3D->m_DevMan.AllocateStagingResource(pDeviceTexture->m_pD3DTexture, TRUE);
                }
            }
        }
    }
    else
    {
        SAFE_DELETE(pDeviceTexture);
    }

    *ppDevTexture = pDeviceTexture;
    return hr;
}

HRESULT CDeviceManager::CreateVolumeTexture(const string& textureName, uint32 nWidth, uint32 nHeight, uint32 nDepth, uint32 nMips, uint32 nUsage, [[maybe_unused]] const ColorF& cClearValue, D3DFormat Format, [[maybe_unused]] D3DPOOL Pool, LPDEVICETEXTURE* ppDevTexture, STextureInfo* pTI)
{
    HRESULT hr = S_OK;
    CDeviceTexture* pDeviceTexture = new CDeviceTexture();
    D3DVolumeTexture* pD3DTex = NULL;
    uint32 nBindFlags = D3D11_BIND_SHADER_RESOURCE;
    if (nUsage & USAGE_DEPTH_STENCIL)
    {
        nBindFlags |= D3D11_BIND_DEPTH_STENCIL;
    }
    else if (nUsage & USAGE_RENDER_TARGET)
    {
        nBindFlags |= D3D11_BIND_RENDER_TARGET;
    }
    
#if defined(AZ_PLATFORM_IOS)
    if (nUsage & USAGE_MEMORYLESS)
    {
        nBindFlags |= D3D11_BIND_MEMORYLESS;
    }
#endif
    
    if (nUsage & USAGE_UNORDERED_ACCESS)
    {
        nBindFlags |= D3D11_BIND_UNORDERED_ACCESS;
    }
    D3D11_TEXTURE3D_DESC Desc;
    ZeroStruct(Desc);
    Desc.Width = nWidth;
    Desc.Height = nHeight;
    Desc.Depth = nDepth;
    Desc.MipLevels = nMips;
    Desc.Format = (nUsage & USAGE_UAV_RWTEXTURE) ? CTexture::ConvertToTypelessFmt(Format) : Format;
    Desc.BindFlags = nBindFlags;
    Desc.CPUAccessFlags = 0;

    Desc.Usage = (nUsage & USAGE_DYNAMIC) ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
    Desc.Usage = (nUsage & USAGE_STAGING) ? D3D11_USAGE_STAGING : Desc.Usage;

    Desc.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA* pSRD = NULL;
    D3D11_SUBRESOURCE_DATA SRD[20] = { D3D11_SUBRESOURCE_DATA() };
    if (pTI && pTI->m_pData)
    {
        pSRD = &SRD[0];
        for (int i = 0; i < nMips; i++)
        {
            SRD[i].pSysMem = pTI->m_pData[i].pSysMem;
            SRD[i].SysMemPitch = pTI->m_pData[i].SysMemPitch;
            SRD[i].SysMemSlicePitch = pTI->m_pData[i].SysMemSlicePitch;

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DEVICEMANAGER_D3D11_INL_SECTION_6
    #include AZ_RESTRICTED_FILE(DeviceManager_D3D11_inl)
#endif
        }
    }
    hr = gcpRendD3D->GetDevice().CreateTexture3D(&Desc, pSRD, &pD3DTex);

    if (SUCCEEDED(hr) && pD3DTex)
    {
        pDeviceTexture->m_pD3DTexture = pD3DTex;
        pDeviceTexture->m_numSubResources = nMips;

        if (!pDeviceTexture->m_nBaseAllocatedSize)
        {
            pDeviceTexture->m_nBaseAllocatedSize = CDeviceTexture::TextureDataSize(nWidth, nHeight, nDepth, nMips, 1, CTexture::TexFormatFromDeviceFormat(Format));

            // Register the VRAM allocation with the driller
            pDeviceTexture->TrackTextureMemory(nUsage, textureName.c_str());
        }

        if (nUsage & USAGE_STAGE_ACCESS)
        {
            if (nUsage & USAGE_CPU_READ)
            {
                pDeviceTexture->m_pStagingResourceDownload = gcpRendD3D->m_DevMan.AllocateStagingResource(pDeviceTexture->m_pD3DTexture, FALSE);
            }
            if (nUsage & USAGE_CPU_WRITE)
            {
                for (int i = 0; i < CDeviceTexture::NUM_UPLOAD_STAGING_RES; i++)
                {
                    pDeviceTexture->m_pStagingResourceUpload[i] = gcpRendD3D->m_DevMan.AllocateStagingResource(pDeviceTexture->m_pD3DTexture, TRUE);
                }
            }
        }
    }
    else
    {
        SAFE_DELETE(pDeviceTexture);
    }

    *ppDevTexture = pDeviceTexture;
    return hr;
}

HRESULT CDeviceManager::CreateBuffer(
    uint32 nSize
    , uint32 elemSize
    , int32 nUsage
    , int32 nBindFlags
    , D3DBuffer** ppBuff)
{
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_RENDERER);
    HRESULT hr = S_OK;

# ifndef _RELEASE
    // ToDo verify that the usage and bindflags are correctly set (e.g certain
    // bit groups are mutually exclusive).
# endif

    D3D11_BUFFER_DESC BufDesc;
    ZeroStruct(BufDesc);

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DEVICEMANAGER_D3D11_INL_SECTION_7
    #include AZ_RESTRICTED_FILE(DeviceManager_D3D11_inl)
#endif

    BufDesc.ByteWidth = nSize * elemSize;
    int nD3DUsage = D3D11_USAGE_DEFAULT;
    nD3DUsage = (nUsage & USAGE_DYNAMIC) ? D3D11_USAGE_DYNAMIC : nD3DUsage;
    nD3DUsage = (nUsage & USAGE_IMMUTABLE) ? D3D11_USAGE_IMMUTABLE : nD3DUsage;
    nD3DUsage = (nUsage & USAGE_STAGING) ? D3D11_USAGE_STAGING : nD3DUsage;
    
#if defined(CRY_USE_METAL)
    nD3DUsage = (nUsage & USAGE_TRANSIENT) ? D3D11_USAGE_TRANSIENT : nD3DUsage;
#if BUFFER_USE_STAGED_UPDATES == 0
    //  direct access usage is allowed only if staged updates logic is off.
    CRY_ASSERT(!(nUsage & USAGE_DIRECT_ACCESS) || !(nUsage & USAGE_STAGING));
    nD3DUsage = (nUsage & USAGE_DIRECT_ACCESS) ? D3D11_USAGE_DIRECT_ACCESS : nD3DUsage;
    
    if ((nUsage & USAGE_DIRECT_ACCESS) && (nUsage & USAGE_STAGING))
    {
        CryFatalError("staging buffers not supported if BUFFER_USE_STAGED_UPDATES not defined");
    }
#endif  //  BUFFER_USE_STAGED_UPDATES == 0
#endif  //  CRY_USE_METAL

    
    BufDesc.Usage = (D3D11_USAGE)nD3DUsage;

# if !defined(OPENGL)
    if (BufDesc.Usage != D3D11_USAGE_STAGING)
# endif
    {
        switch (nBindFlags)
        {
        case BIND_VERTEX_BUFFER:
            BufDesc.BindFlags |= D3D11_BIND_VERTEX_BUFFER;
            break;
        case BIND_INDEX_BUFFER:
            BufDesc.BindFlags |= D3D11_BIND_INDEX_BUFFER;
            break;
        case BIND_CONSTANT_BUFFER:
            BufDesc.BindFlags |= D3D11_BIND_CONSTANT_BUFFER;
            break;

        case BIND_SHADER_RESOURCE:
        case BIND_UNORDERED_ACCESS:
            if (nBindFlags & BIND_SHADER_RESOURCE)
            {
                BufDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
            }
            if (nBindFlags & BIND_UNORDERED_ACCESS)
            {
                BufDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
            }
            break;

        case BIND_STREAM_OUTPUT:
        case BIND_RENDER_TARGET:
        case BIND_DEPTH_STENCIL:
            CryFatalError("trying to create (currently) unsupported buffer type");
            break;

        default:
            CryFatalError("trying to create unknown buffer type");
            break;
        }
    }
# if !defined(OPENGL)
    else
    {
        BufDesc.BindFlags = 0;
    }
# endif

    BufDesc.MiscFlags = 0;
    BufDesc.CPUAccessFlags = 0;
    if (BufDesc.Usage != D3D11_USAGE_DEFAULT && BufDesc.Usage != D3D11_USAGE_IMMUTABLE)
    {
        BufDesc.CPUAccessFlags = (nUsage & USAGE_CPU_WRITE) ? D3D11_CPU_ACCESS_WRITE : 0;
        BufDesc.CPUAccessFlags |= (nUsage & USAGE_CPU_READ) ? D3D11_CPU_ACCESS_READ : 0;
    }

    if (nBindFlags & BIND_UNORDERED_ACCESS)
    {
        BufDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    }

    hr = gcpRendD3D->GetDevice().CreateBuffer(&BufDesc, NULL, ppBuff);
    CHECK_HRESULT(hr);

    Render::Debug::VRAMAllocationSubcategory subcategory = Render::Debug::VRAM_SUBCATEGORY_BUFFER_OTHER_BUFFER;
    if (BufDesc.Usage & D3D11_BIND_VERTEX_BUFFER)
    {
        subcategory = Render::Debug::VRAM_SUBCATEGORY_BUFFER_VERTEX_BUFFER;
    }
    else if (BufDesc.Usage & D3D11_BIND_INDEX_BUFFER)
    {
        subcategory = Render::Debug::VRAM_SUBCATEGORY_BUFFER_INDEX_BUFFER;
    }
    else if (BufDesc.Usage & D3D11_BIND_CONSTANT_BUFFER)
    {
        subcategory = Render::Debug::VRAM_SUBCATEGORY_BUFFER_CONSTANT_BUFFER;
    }
    
    void* address = static_cast<void*>(*ppBuff);
    size_t byteSize = BufDesc.ByteWidth;
    EBUS_EVENT(Render::Debug::VRAMDrillerBus, RegisterAllocation, address, byteSize, "CDeviceManager::CreateBuffer", Render::Debug::VRAM_CATEGORY_BUFFER, subcategory);

    return hr;
}

void CDeviceManager::ExtractBasePointer([[maybe_unused]] D3DBuffer* buffer, uint8*& base_ptr)
{
#if defined(CRY_USE_METAL) && (BUFFER_USE_STAGED_UPDATES == 0)
    base_ptr = (uint8*)DXMETALGetBufferStorage(buffer);
#else
#   if BUFFER_ENABLE_DIRECT_ACCESS
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DEVICEMANAGER_D3D11_INL_SECTION_8
    #include AZ_RESTRICTED_FILE(DeviceManager_D3D11_inl)
#endif
#   else
    base_ptr = NULL;
#   endif
#endif
}
