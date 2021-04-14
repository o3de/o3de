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

// Description : DDS image file format implementation.


#include "RenderDll_precompiled.h"
#include "CImage.h"
#include <CryFile.h>
#include <CryPath.h>
#include "DDSImage.h"
#include "ImageExtensionHelper.h"               // CImageExtensionHelper
#include "TypeInfo_impl.h"
#include "ImageExtensionHelper_info.h"
#include "StringUtils.h"
#include "ILog.h"
#include "../TextureHelpers.h"

CImageDDSFile::CImageDDSFile(const string& filename)
    : CImageFile(filename)
{
}

CImageDDSFile::CImageDDSFile (const string& filename, uint32 nFlags)
    : CImageFile (filename)
{
    LOADING_TIME_PROFILE_SECTION;
    m_pFileMemory = 0;
    if (!Load(filename, nFlags) || NULL == m_pFileMemory)    // load data from file
    {
        if (mfGet_error() == eIFE_OK)
        {
            if (nFlags & FIM_ALPHA)
            {
                mfSet_error(eIFE_BadFormat, "Texture does not have alpha channel"); // Usually requested via FT_HAS_ATTACHED_ALPHA for POM / Offset Bump Mapping
            }
        }
    }
    else
    {
        PostLoad();
    }
}

bool CImageDDSFile::Stream(uint32 nFlags, IImageFileStreamCallback* pStreamCallback)
{
    const string& filename = mfGet_filename();

    DDSSplitted::TPath adjustedFileName;
    AdjustFirstFileName(nFlags, filename.c_str(), adjustedFileName);

    m_pStreamState = new SImageFileStreamState;
    m_pStreamState->m_nPending = 1;
    m_pStreamState->m_nFlags = nFlags;
    m_pStreamState->m_pCallback = pStreamCallback;
    AddRef();

    IStreamEngine* pStreamEngine = gEnv->pSystem->GetStreamEngine();

    StreamReadParams rp;
    rp.nFlags |= IStreamEngine::FLAGS_NO_SYNC_CALLBACK;
    rp.dwUserData = 0;
    m_pStreamState->m_pStreams[0] = pStreamEngine->StartRead(eStreamTaskTypeTexture, adjustedFileName.c_str(), this, &rp);

    return true;
}

DDSSplitted::DDSDesc CImageDDSFile::mfGet_DDSDesc() const
{
    DDSSplitted::DDSDesc d;
    d.eFormat = m_eFormat;
    d.eTileMode = m_eTileMode;
    d.nBaseOffset = mfGet_StartSeek();
    d.nDepth = m_DDSHeader.dwDepth;
    d.nFlags = (m_Flags & (FIM_ALPHA | FIM_SPLITTED | FIM_DX10IO));
    d.nHeight = m_DDSHeader.dwHeight;
    d.nMips = m_DDSHeader.dwMipMapCount;
    d.nMipsPersistent = m_NumPersistentMips;
    d.nSides = m_Sides;
    d.nWidth = m_DDSHeader.dwWidth;
    return d;
}

//////////////////////////////////////////////////////////////////////
bool CImageDDSFile::Load(const string& filename, uint32 nFlags)
{
    LOADING_TIME_PROFILE_SECTION;

    DDSSplitted::TPath adjustedFileName;
    AdjustFirstFileName(nFlags, filename.c_str(), adjustedFileName);

    DDSSplitted::RequestInfo otherMips[64];
    size_t nOtherMips = 0;

    // Read the file into memory regardless of split or un-split Mips
    DDSSplitted::FileWrapper filew(adjustedFileName.c_str(), true);
    if (!LoadFromFile(filew, nFlags, otherMips, nOtherMips, 64))
    {
        return false;
    }

    if (nOtherMips && !DDSSplitted::LoadMipsFromRequests(otherMips, nOtherMips))
    {
        AZ_Error("Render", false, "Failed to load mips for DDS asset %s", adjustedFileName.c_str());
        return false;
    }

    return true;
}

int CImageDDSFile::AdjustHeader()
{
    int nDeltaMips = 0;

    if (!(m_Flags & FIM_SUPPRESS_DOWNSCALING))
    {
        int nFinalMips = min(max(m_NumPersistentMips, max(CRenderer::CV_r_texturesstreamingMinUsableMips, m_NumMips - CRenderer::CV_r_texturesstreamingSkipMips)), m_NumMips);

        nDeltaMips = m_NumMips - nFinalMips;
        if (nDeltaMips > 0)
        {
            m_Width = max(1, m_Width >> nDeltaMips);
            m_Height = max(1, m_Height >> nDeltaMips);
            m_Depth = max(1, m_Depth >> nDeltaMips);
            m_NumMips = nFinalMips;
        }
    }

    return nDeltaMips;
}

bool CImageDDSFile::LoadFromFile(DDSSplitted::FileWrapper& file, uint32 nFlags, DDSSplitted::RequestInfo* pConts, size_t& nConts, [[maybe_unused]] size_t nContsCap)
{
    LOADING_TIME_PROFILE_SECTION;

    if (file.IsValid())
    {
        const size_t fileSize = file.GetLength();

        AZStd::intrusive_ptr<AZ::IO::MemoryBlock> pImageMemory;

        // alloc space for header
        CImageExtensionHelper::DDS_FILE_DESC ddsHeader;
        CImageExtensionHelper::DDS_HEADER_DXT10 ddsExtendedHeader;

        if (nFlags & FIM_ALPHA)
        {
            // Requested alpha image.
            ddsHeader.dwMagic = MAKEFOURCC('D', 'D', 'S', ' ');
            if (!(nFlags & FIM_SPLITTED))
            {
                // Not split. Which means it's somewhere in this file. Go find it.
                if (!DDSSplitted::SeekToAttachedImage(file))
                {
                    mfSet_error(eIFE_ChunkNotFound, "Failed to find attached image");
                    return false;
                }
            }
            else
            {
                file.ReadRaw(&ddsHeader.dwMagic, sizeof(ddsHeader.dwMagic));
            }
            file.ReadRaw(&ddsHeader.header, sizeof(CImageExtensionHelper::DDS_HEADER));
            SwapEndian(ddsHeader.header);
            ddsHeader.dwMagic = MAKEFOURCC('D', 'D', 'S', ' ');
        }
        else
        {
            file.ReadRaw(&ddsHeader, sizeof(CImageExtensionHelper::DDS_FILE_DESC));
            SwapEndian(ddsHeader);
        }

        if (!ddsHeader.IsValid())
        {
            mfSet_error(eIFE_BadFormat, "Bad DDS header");
            return false;
        }

        if (ddsHeader.header.IsDX10Ext())
        {
            file.ReadRaw(&ddsExtendedHeader, sizeof(CImageExtensionHelper::DDS_HEADER_DXT10));
        }

        m_nStartSeek = file.Tell();

        if (!SetHeaderFromMemory((byte*)&ddsHeader, (byte*)&ddsExtendedHeader, nFlags))
        {
            return false;
        }

        // Grab a snapshot of the DDS layout before adjusting the header
        DDSSplitted::DDSDesc desc;
        desc.pName = m_FileName.c_str();
        desc.nWidth = m_Width;
        desc.nHeight = m_Height;
        desc.nDepth = m_Depth;
        desc.nMips = m_NumMips;
        desc.nMipsPersistent = m_NumPersistentMips;
        desc.nSides = m_Sides;
        desc.eFormat = m_eFormat;
        desc.eTileMode = m_eTileMode;
        desc.nBaseOffset = m_nStartSeek;
        desc.nFlags = m_Flags;

        int nDeltaMips = AdjustHeader();

        // If stream prepare, only allocate room for the pers mips

        int nMipsToLoad = (m_Flags & FIM_STREAM_PREPARE)
            ? m_NumPersistentMips
            : m_NumMips;
        int nImageIgnoreMips = m_NumMips - nMipsToLoad;
        int nFirstPersistentMip = m_NumMips - m_NumPersistentMips;

        auto streamer = AZ::Interface<AZ::IO::IStreamer>::Get();

        size_t nImageSideSize = CTexture::TextureDataSize(
                max(1, m_Width >> nImageIgnoreMips),
                max(1, m_Height >> nImageIgnoreMips),
                max(1, m_Depth >> nImageIgnoreMips),
                nMipsToLoad, 1, m_eFormat, m_eTileMode);
        size_t nImageSize = nImageSideSize * m_Sides;
        pImageMemory = gEnv->pCryPak->PoolAllocMemoryBlock(nImageSize, "CImageDDSFile::LoadFromFile",
            streamer->GetRecommendations().m_memoryAlignment);

        mfSet_ImageSize(nImageSideSize);

        DDSSplitted::ChunkInfo chunks[16];
        size_t numChunks = DDSSplitted::GetFilesToRead(chunks, 16, desc, nDeltaMips + nImageIgnoreMips, m_NumMips + nDeltaMips - 1);

        uint32 nDstOffset = 0;
        byte* pDst = (byte*)pImageMemory->m_address.get();

        nConts = 0;

        for (size_t chunkIdx = 0; chunkIdx < numChunks; ++chunkIdx)
        {
            const DDSSplitted::ChunkInfo& chunk = chunks[chunkIdx];

            uint32 nSurfaceSize = CTexture::TextureDataSize(
                    max(1u, (uint32)desc.nWidth >> chunk.nMipLevel),
                    max(1u, (uint32)desc.nHeight >> chunk.nMipLevel),
                    max(1u, (uint32)desc.nDepth >> chunk.nMipLevel),
                    1, 1, desc.eFormat, desc.eTileMode);
            uint32 nSidePitch = nSurfaceSize + chunk.nSideDelta;

            // Only copy persistent mips now. Create continuations for any others.

            int nChunkMip = chunk.nMipLevel - nDeltaMips;
            if (nChunkMip < nFirstPersistentMip)
            {
                string chunkFileName(chunk.fileName);
                for (uint32 sideIdx = 0; sideIdx < m_Sides; ++sideIdx)
                {
                    DDSSplitted::RequestInfo& cont = pConts[nConts++];
                    cont.fileName = chunkFileName;
                    cont.nOffs = chunk.nOffsetInFile + sideIdx * nSidePitch;
                    cont.nRead = nSurfaceSize;
                    cont.pOut = pDst + sideIdx * nImageSideSize + nDstOffset;
                }
            }
            else
            {
                for (uint32 sideIdx = 0; sideIdx < m_Sides; ++sideIdx)
                {
                    file.Seek(chunk.nOffsetInFile + sideIdx * nSidePitch);
                    file.ReadRaw(pDst + sideIdx * nImageSideSize + nDstOffset, nSurfaceSize);
                }
            }

            nDstOffset += nSurfaceSize;
        }

        m_pFileMemory = pImageMemory;

        return true;
    }

    return false;
}

void CImageDDSFile::StreamAsyncOnComplete(IReadStream* pStream, uint32 nError)
{
    assert (m_pStreamState);

    int nPending = CryInterlockedDecrement(&m_pStreamState->m_nPending);

    bool bIsComplete = false;
    bool bWasSuccess = false;

    if (!nError)
    {
        const StreamReadParams& rp = pStream->GetParams();

        if (rp.dwUserData == 0)
        {
            DDSSplitted::FileWrapper file(pStream->GetBuffer(), pStream->GetBytesRead());

            // Initial read.

            DDSSplitted::RequestInfo otherMips[SImageFileStreamState::MaxStreams - 1];
            const size_t nOtherMipsCap = sizeof(otherMips) / sizeof(otherMips[0]);
            size_t nOtherMips = 0;

            if (LoadFromFile(file, m_pStreamState->m_nFlags, otherMips, nOtherMips, nOtherMipsCap))
            {
                IStreamEngine* pStreamEngine = gEnv->pSystem->GetStreamEngine();

                if (nOtherMips)
                {
                    // Write before starting extra tasks
                    m_pStreamState->m_nPending = nOtherMips;

                    // Issue stream requests for additional mips
                    for (size_t nOtherMip = 0; nOtherMip < nOtherMips; ++nOtherMip)
                    {
                        const DDSSplitted::RequestInfo& req = otherMips[nOtherMip];

                        StreamReadParams params;
                        params.dwUserData = nOtherMip + 1;

#if 0 // TODO Fix me at some point - was disabled due to issue with SPU. Should be enabled again
                        params.nOffset = req.nOffs;
                        params.nSize = req.nRead;
                        params.pBuffer = req.pOut;
#else
                        m_pStreamState->m_requests[nOtherMip + 1].nOffs = req.nOffs;
                        m_pStreamState->m_requests[nOtherMip + 1].nSize = req.nRead;
                        m_pStreamState->m_requests[nOtherMip + 1].pOut = req.pOut;
#endif

                        params.nFlags |= IStreamEngine::FLAGS_NO_SYNC_CALLBACK;
                        AddRef();

                        m_pStreamState->m_pStreams[nOtherMip + 1] = pStreamEngine->StartRead(eStreamTaskTypeTexture, req.fileName.c_str(), this, &params);
                    }
                }
                else
                {
                    bIsComplete = true;
                }

                bWasSuccess = true;
            }
        }
        else
        {
#if 1
            const char* pBase = (const char*)m_pFileMemory->m_address.get();
            const char* pEnd = pBase + m_pFileMemory->m_size;

            char* pDst = (char*)m_pStreamState->m_requests[rp.dwUserData].pOut;
            char* pDstEnd = pDst + m_pStreamState->m_requests[rp.dwUserData].nSize;
            const char* pSrc = reinterpret_cast<const char*>(pStream->GetBuffer()) + m_pStreamState->m_requests[rp.dwUserData].nOffs;
            const char* pSrcEnd = pSrc + m_pStreamState->m_requests[rp.dwUserData].nSize;

            if (pDst < pBase)
            {
                __debugbreak();
            }
            if (pDstEnd > pEnd)
            {
                __debugbreak();
            }
            if (pSrc < pStream->GetBuffer())
            {
                __debugbreak();
            }
            if (pSrcEnd > reinterpret_cast<const char*>(pStream->GetBuffer()) + pStream->GetBytesRead())
            {
                __debugbreak();
            }
            memcpy(pDst, pSrc, m_pStreamState->m_requests[rp.dwUserData].nSize);
#endif

            if (nPending == 0)
            {
                // Done!
                bIsComplete = true;
                bWasSuccess = true;
            }
        }
    }
    else
    {
        bIsComplete = true;
    }

    pStream->FreeTemporaryMemory();

    if (bIsComplete)
    {
        if (bWasSuccess)
        {
            PostLoad();
            m_pStreamState->RaiseComplete(this);
        }
        else
        {
            if (nPending)
            {
                __debugbreak();
            }
            m_pStreamState->RaiseComplete(NULL);
        }
    }

    Release();
}

bool CImageDDSFile::SetHeaderFromMemory(byte* pFileStart, byte* pFileAfterHeader, uint32 nFlags)
{
    LOADING_TIME_PROFILE_SECTION


    CImageExtensionHelper::DDS_FILE_DESC& dds = *(CImageExtensionHelper::DDS_FILE_DESC*)pFileStart;
    CImageExtensionHelper::DDS_HEADER_DXT10& ddx = *(CImageExtensionHelper::DDS_HEADER_DXT10*)pFileAfterHeader;

    SwapEndian(dds);
    if (dds.header.IsDX10Ext())
    {
        SwapEndian(ddx);
    }

    if (!dds.IsValid())
    {
        mfSet_error (eIFE_BadFormat, "Bad DDS header");
        return false;
    }

    m_DDSHeader = dds.header;
    m_DDSHeaderExtension = ddx;

    // check for nativeness of texture
    const uint32 imageFlags = CImageExtensionHelper::GetImageFlags(&m_DDSHeader);
    if (!CImageExtensionHelper::IsImageNative(imageFlags))
    {
        mfSet_error(eIFE_BadFormat, "Not converted for this platform");
        return false;
    }

    // setup texture properties
    m_Width = m_DDSHeader.dwWidth;
    m_Height = m_DDSHeader.dwHeight;

    m_Flags |= m_DDSHeader.IsDX10Ext() ? FIM_DX10IO : 0;

    m_eFormat = DDSFormats::GetFormatByDesc(m_DDSHeader.ddspf, m_DDSHeaderExtension.dxgiFormat);
    if (eTF_Unknown == m_eFormat)
    {
        mfSet_error (eIFE_BadFormat, "Unknown DDS pixel format!");
        return false;
    }

    m_eTileMode = eTM_None;
    if (imageFlags & CImageExtensionHelper::EIF_Tiled)
    {
        switch (m_DDSHeader.bTileMode)
        {
        case CImageExtensionHelper::eTM_LinearPadded:
            m_eTileMode = eTM_LinearPadded;
            break;

        case CImageExtensionHelper::eTM_Optimal:
            m_eTileMode = eTM_Optimal;
            break;
        }
    }

    mfSet_numMips(m_DDSHeader.GetMipCount());
    m_Depth = max((uint32)1ul, (uint32)m_DDSHeader.dwDepth);

    m_Sides = 1;
    if ((m_DDSHeader.dwSurfaceFlags & DDS_SURFACE_FLAGS_CUBEMAP) && (m_DDSHeader.dwCubemapFlags & DDS_CUBEMAP_ALLFACES))
    {
        m_Sides = 6;
    }

    if (m_DDSHeader.dwTextureStage == 'CRYF')
    {
        m_NumPersistentMips = m_DDSHeader.bNumPersistentMips;
    }
    else
    {
        m_NumPersistentMips = 0;
    }

    m_NumPersistentMips = min(m_NumMips, max((int)DDSSplitted::etexNumLastMips, m_NumPersistentMips));

    m_fAvgBrightness = m_DDSHeader.fAvgBrightness;
    m_cMinColor = m_DDSHeader.cMinColor;
    m_cMaxColor = m_DDSHeader.cMaxColor;
#ifdef NEED_ENDIAN_SWAP
    SwapEndianBase(&m_fAvgBrightness);
    SwapEndianBase(&m_cMinColor);
    SwapEndianBase(&m_cMaxColor);
#endif

    if (DDSFormats::IsNormalMap(m_eFormat))
    {
        const int nLastMipWidth = m_Width >> (m_NumMips - 1);
        const int nLastMipHeight = m_Height >> (m_NumMips - 1);
        if (nLastMipWidth < 4 || nLastMipHeight < 4)
        {
            mfSet_error (eIFE_BadFormat, "Texture has wrong number of mips");
        }
    }

    bool bStreamable = (nFlags & FIM_STREAM_PREPARE) != 0;

    // Can't stream volume textures and textures without mips
    if (m_eFormat == eTF_Unknown || m_Depth > 1 || m_NumMips < 2)
    {
        bStreamable = false;
    }

    if (
        (m_Width <= DDSSplitted::etexLowerMipMaxSize || m_Height <= DDSSplitted::etexLowerMipMaxSize) ||
        m_NumMips <= m_NumPersistentMips ||
        m_NumPersistentMips == 0)
    {
        bStreamable = false;
    }

    if (bStreamable)
    {
        m_Flags |= FIM_STREAM_PREPARE;
    }
    m_Flags |= nFlags & (FIM_SPLITTED | FIM_ALPHA);
    if (imageFlags & CImageExtensionHelper::EIF_Splitted)
    {
        m_Flags |= FIM_SPLITTED;
    }

    // set up flags
    if (!(nFlags & FIM_ALPHA))
    {
        if ((imageFlags & DDS_RESF1_NORMALMAP) || TextureHelpers::VerifyTexSuffix(EFTT_NORMALS, m_FileName) || DDSFormats::IsNormalMap(m_eFormat))
        {
            m_Flags |= FIM_NORMALMAP;
        }
    }

    if (imageFlags & CImageExtensionHelper::EIF_Decal)
    {
        m_Flags |= FIM_DECAL;
    }
    if (imageFlags & CImageExtensionHelper::EIF_SRGBRead)
    {
        m_Flags |= FIM_SRGB_READ;
    }
    if (imageFlags & CImageExtensionHelper::EIF_Greyscale)
    {
        m_Flags |= FIM_GREYSCALE;
    }
#if defined(AZ_PLATFORM_PROVO) || defined(TOOLS_SUPPORT_PROVO)
    #define AZ_RESTRICTED_SECTION IMAGEEXTENSIONHELPER_H_SECTION_ISNATIVE
    #include AZ_RESTRICTED_FILE_EXPLICIT(Textures/Image/DDSImage_cpp, provo)
#endif
#if defined(AZ_PLATFORM_JASPER) || defined(TOOLS_SUPPORT_JASPER)
    #define AZ_RESTRICTED_SECTION IMAGEEXTENSIONHELPER_H_SECTION_ISNATIVE
    #include AZ_RESTRICTED_FILE_EXPLICIT(Textures/Image/DDSImage_cpp, jasper)
#endif
#if defined(AZ_PLATFORM_SALEM) || defined(TOOLS_SUPPORT_SALEM)
    #define AZ_RESTRICTED_SECTION IMAGEEXTENSIONHELPER_H_SECTION_ISNATIVE
    #include AZ_RESTRICTED_FILE_EXPLICIT(Textures/Image/DDSImage_cpp, salem)
#endif    
    if (imageFlags & CImageExtensionHelper::EIF_AttachedAlpha)
    {
        m_Flags |= FIM_HAS_ATTACHED_ALPHA;
    }
    if (imageFlags & CImageExtensionHelper::EIF_SupressEngineReduce)
    {
        m_Flags |= FIM_SUPPRESS_DOWNSCALING;
    }
    if (imageFlags & CImageExtensionHelper::EIF_RenormalizedTexture)
    {
        m_Flags |= FIM_RENORMALIZED_TEXTURE;
    }

    if (m_Flags & FIM_NORMALMAP)
    {
        if (DDSFormats::IsSigned(m_eFormat))
        {
            m_cMinColor = ColorF(0.0f,  0.0f,  0.0f,  0.0f);
            m_cMaxColor = ColorF(1.0f,  1.0f,  1.0f,  1.0f);
        }
        else
        {
            m_cMinColor = ColorF(-1.0f, -1.0f, -1.0f, -1.0f);
            m_cMaxColor = ColorF(1.0f,  1.0f,  1.0f,  1.0f);

            //          mfSet_error(eIFE_BadFormat, "Texture has to have a signed format");
        }
    }

    return true;
}

bool CImageDDSFile::PostLoad()
{
    const byte* ptrBuffer = (const byte*)m_pFileMemory->m_address.get();

    int nSrcSideSize = mfGet_ImageSize();

    for (int nS = 0; nS < m_Sides; nS++)
    {
        mfFree_image(nS);
        mfGet_image(nS);

        // stop of an allocation failed
        if (m_pByteImage[nS] == NULL)
        {
            // free already allocated data
            for (int i = 0; i < nS; ++i)
            {
                mfFree_image(i);
            }
            mfSet_ImageSize(0);
            mfSet_error(eIFE_OutOfMemory, "Failed to allocate Memory");
            return false;
        }

        memcpy(m_pByteImage[nS], ptrBuffer + nSrcSideSize * nS, nSrcSideSize);
    }

    // We don't need file memory anymore, free it.
    m_pFileMemory = 0;

    return true;
}

void CImageDDSFile::AdjustFirstFileName(uint32& nFlags, const char* pFileName, DDSSplitted::TPath& adjustedFileName)
{
    using namespace AZ::IO;

    LOADING_TIME_PROFILE_SECTION;

    const bool bIsAttachedAlpha = (nFlags & FIM_ALPHA) != 0;
    adjustedFileName = pFileName;

    if (!bIsAttachedAlpha)
    {
        // First file for non attached mip chain is always just .dds
        return;
    }

    DDSSplitted::TPath firstAttachedAlphaChunkName;
    DDSSplitted::MakeName(firstAttachedAlphaChunkName, pFileName, 0, nFlags | FIM_SPLITTED);

#if defined (RELEASE)
    // In release we assume alpha is split if a .dds.a exists. This breaks loading from a .dds outside of PAKs that contains all data (non split).
    if (gEnv->pCryPak->IsFileExist(firstAttachedAlphaChunkName.c_str()))
    {
        nFlags |= FIM_SPLITTED;
        adjustedFileName = firstAttachedAlphaChunkName;
    }
#else
    // Otherwise we check the .dds header which always works, but is slower (two reads from .dds and .dds.a on load)
    CImageExtensionHelper::DDS_FILE_DESC ddsFileDesc;

    auto streamer = AZ::Interface<IStreamer>::Get();
    AZStd::binary_semaphore wait;
    AZStd::atomic_bool success = true;

    FileRequestPtr request = streamer->Read(pFileName, &ddsFileDesc, sizeof(ddsFileDesc), sizeof(ddsFileDesc), IStreamerTypes::s_deadlineNow);
    streamer->SetRequestCompleteCallback(request, [&wait, &success](FileRequestHandle request)
    {
        auto streamer = AZ::Interface<IStreamer>::Get();
        success = streamer->GetRequestStatus(request) == IStreamerTypes::RequestStatus::Completed;
        wait.release();
    });
    streamer->QueueRequest(AZStd::move(request));
    wait.acquire();

    if (success)
    {
        const uint32 imageFlags = CImageExtensionHelper::GetImageFlags(&ddsFileDesc.header);
        if ((imageFlags& CImageExtensionHelper::EIF_Splitted) != 0)
        {
            nFlags |= FIM_SPLITTED;
            adjustedFileName = firstAttachedAlphaChunkName;
        }
    }
#endif
}

//////////////////////////////////////////////////////////////////////

#if defined(WIN32) || defined(WIN64)
byte* WriteDDS(const byte* dat, int wdt, int hgt, int dpth, const char* name, ETEX_Format eTF, int nMips, ETEX_Type eTT, bool bToMemory, int* pSize)
{
    CImageExtensionHelper::DDS_FILE_DESC fileDesc;
    memset(&fileDesc, 0, sizeof(fileDesc));
    byte* pData = NULL;
    CCryFile file;
    int nOffs = 0;
    int nSize = CTexture::TextureDataSize(wdt, hgt, dpth, nMips, 1, eTF);

    fileDesc.dwMagic = MAKEFOURCC('D', 'D', 'S', ' ');

    if (!bToMemory)
    {
        if (!file.Open(name, "wb"))
        {
            return NULL;
        }

        file.Write(&fileDesc.dwMagic, sizeof(fileDesc.dwMagic));
    }
    else
    {
        pData = new byte[sizeof(fileDesc) + nSize];
        *(DWORD*)pData = fileDesc.dwMagic;
        nOffs += sizeof(fileDesc.dwMagic);
    }

    fileDesc.header.dwSize = sizeof(fileDesc.header);
    fileDesc.header.dwWidth = wdt;
    fileDesc.header.dwHeight = hgt;
    fileDesc.header.dwMipMapCount = max(1, nMips);
    fileDesc.header.dwHeaderFlags = DDS_HEADER_FLAGS_TEXTURE | DDS_HEADER_FLAGS_MIPMAP;
    fileDesc.header.dwSurfaceFlags = DDS_SURFACE_FLAGS_TEXTURE | DDS_SURFACE_FLAGS_MIPMAP;
    fileDesc.header.dwTextureStage = 'CRYF';
    fileDesc.header.dwReserved1 = 0;
    fileDesc.header.fAvgBrightness = 0.0f;
    fileDesc.header.cMinColor = 0.0f;
    fileDesc.header.cMaxColor = 1.0f;
    int nSides = 1;
    if (eTT == eTT_Cube)
    {
        fileDesc.header.dwSurfaceFlags |= DDS_SURFACE_FLAGS_CUBEMAP;
        fileDesc.header.dwCubemapFlags |= DDS_CUBEMAP_ALLFACES;
        nSides = 6;
    }
    else if (eTT == eTT_3D)
    {
        fileDesc.header.dwHeaderFlags |= DDS_HEADER_FLAGS_VOLUME;
    }
    if (eTT != eTT_3D)
    {
        dpth = 1;
    }
    fileDesc.header.dwDepth = dpth;
    if (name)
    {
        size_t len = strlen(name);
        if (len > 4)
        {
            if (!_stricmp(&name[len - 4], ".ddn"))
            {
                fileDesc.header.dwReserved1 = DDS_RESF1_NORMALMAP;
            }
        }
    }
    fileDesc.header.ddspf = DDSFormats::GetDescByFormat(eTF);
    fileDesc.header.dwPitchOrLinearSize = CTexture::TextureDataSize(wdt, 1, 1, 1, 1, eTF);
    if (!bToMemory)
    {
        file.Write(&fileDesc.header, sizeof(fileDesc.header));

        nOffs = 0;
        int nSide;
        for (nSide = 0; nSide < nSides; nSide++)
        {
            file.Write(&dat[nOffs], nSize);
            nOffs += nSize;
        }
    }
    else
    {
        memcpy(&pData[nOffs], &fileDesc.header, sizeof(fileDesc.header));
        nOffs += sizeof(fileDesc.header);

        int nSide;
        int nSrcOffs = 0;
        for (nSide = 0; nSide < nSides; nSide++)
        {
            memcpy(&pData[nOffs], &dat[nSrcOffs], nSize);
            nSrcOffs += nSize;
            nOffs += nSize;
        }

        if (pSize)
        {
            *pSize = nOffs;
        }
        return pData;
    }

    return NULL;
}
#endif // #if defined(WIN32) || defined(WIN64)

//////////////////////////////////////////////////////////////////////
namespace DDSSplitted
{
    TPath& MakeName(TPath& sOut, const char* sOriginalName, const uint32 nChunk, const uint32 nFlags)
    {
        sOut = sOriginalName;

        assert (nChunk < 100);

        char buffer[10];
        if ((nFlags & FIM_SPLITTED) && (nChunk > 0))
        {
            buffer[0] = '.';
            if (nChunk < 10)
            {
                buffer[1] = '0' + nChunk;
                buffer[2] = 0;
            }
            else
            {
                buffer[1] = '0' + (nChunk / 10);
                buffer[2] = '0' + (nChunk % 10);
                buffer[3] = 0;
            }
        }
        else
        {
            buffer[0] = 0;
        }

        sOut.append(buffer);
        if ((nFlags & (FIM_SPLITTED | FIM_ALPHA)) == (FIM_SPLITTED | FIM_ALPHA))
        {
            // additional suffix for attached alpha channel
            if (buffer[0])
            {
                sOut.append("a");
            }
            else
            {
                sOut.append(".a");
            }
        }

        return sOut;
    }

    size_t GetFilesToRead_Split(ChunkInfo* pFiles, [[maybe_unused]] size_t nFilesCapacity, const DDSDesc& desc, uint32 nStartMip, uint32 nEndMip)
    {
        FUNCTION_PROFILER_RENDERER;

        assert(nStartMip <= nEndMip);
        assert(desc.nFlags & FIM_SPLITTED);

        if (nEndMip > desc.nMips)
        {
            nEndMip = desc.nMips - 1;
        }

        size_t nNumFiles = 0;

        for (uint32 mip = nStartMip; mip <= nEndMip; ++mip)
        {
            const int chunkNumber = (mip >= (int)(desc.nMips - desc.nMipsPersistent))
                ? 0
                : desc.nMips - mip - desc.nMipsPersistent;

            ChunkInfo& newChunk = pFiles[nNumFiles];
            MakeName(newChunk.fileName, desc.pName, chunkNumber, desc.nFlags);

            newChunk.nMipLevel = mip;

            if (chunkNumber != 0)
            {
                newChunk.nOffsetInFile = 0;
                newChunk.nSizeInFile = 0;
                newChunk.nSideDelta = 0;
            }
            else
            {
                uint32 nFirstPersistentMip = desc.nMips - desc.nMipsPersistent;
                uint32 nSurfaceSize = CTexture::TextureDataSize(
                        max(1u, desc.nWidth >> mip), max(1u, desc.nHeight >> mip), max(1u, desc.nDepth >> mip),
                        1, 1, desc.eFormat, desc.eTileMode);
                uint32 nSidePitch = CTexture::TextureDataSize(
                        max(1u, desc.nWidth >> nFirstPersistentMip), max(1u, desc.nHeight >> nFirstPersistentMip), max(1u, desc.nDepth >> nFirstPersistentMip),
                        desc.nMipsPersistent, 1, desc.eFormat, desc.eTileMode);
                uint32 nStartOffset = CTexture::TextureDataSize(
                        max(1u, desc.nWidth >> nFirstPersistentMip), max(1u, desc.nHeight >> nFirstPersistentMip), max(1u, desc.nDepth >> nFirstPersistentMip),
                        mip - nFirstPersistentMip, 1, desc.eFormat, desc.eTileMode);

                newChunk.nOffsetInFile = desc.nBaseOffset + nStartOffset;
                newChunk.nSizeInFile = nSidePitch * (desc.nSides - 1) + nSurfaceSize;
                newChunk.nSideDelta = nSidePitch - nSurfaceSize;
            }

            ++nNumFiles;
        }

        return nNumFiles;
    }

    size_t GetFilesToRead_UnSplit(ChunkInfo* pFiles, size_t nFilesCapacity, const DDSDesc& desc, uint32 nStartMip, uint32 nEndMip)
    {
        FUNCTION_PROFILER_RENDERER;

        assert(nStartMip <= nEndMip);
        assert(nEndMip < desc.nMips);
        assert(!(desc.nFlags & FIM_SPLITTED));

        size_t nNumFiles = 0;

        uint32 nSideStart = CTexture::TextureDataSize(desc.nWidth, desc.nHeight, desc.nDepth, nStartMip, 1, desc.eFormat, desc.eTileMode);
        uint32 nSidePitch = CTexture::TextureDataSize(desc.nWidth, desc.nHeight, desc.nDepth, desc.nMips, 1, desc.eFormat, desc.eTileMode);

        for (uint32 nMip = nStartMip; nMip <= nEndMip; ++nMip)
        {
            uint32 nOffset = desc.nBaseOffset + nSideStart;
            uint32 nSurfaceSize = CTexture::TextureDataSize(
                    max(1u, desc.nWidth >> nMip),
                    max(1u, desc.nHeight >> nMip),
                    max(1u, desc.nDepth >> nMip),
                    1, 1, desc.eFormat, desc.eTileMode);

            if (nNumFiles < nFilesCapacity)
            {
                pFiles[nNumFiles].fileName = desc.pName;
                pFiles[nNumFiles].nMipLevel = nMip;
                pFiles[nNumFiles].nOffsetInFile = nOffset;
                pFiles[nNumFiles].nSizeInFile = nSidePitch * (desc.nSides - 1) + nSurfaceSize;
                pFiles[nNumFiles].nSideDelta = nSidePitch - nSurfaceSize;
            }

            ++nNumFiles;
            nSideStart += nSurfaceSize;
        }

        return nNumFiles;
    }

    size_t GetFilesToRead(ChunkInfo* pFiles, size_t nFilesCapacity, const DDSDesc& desc, uint32 nStartMip, uint32 nEndMip)
    {
        return (desc.nFlags & FIM_SPLITTED)
               ? GetFilesToRead_Split(pFiles, nFilesCapacity, desc, nStartMip, nEndMip)
               : GetFilesToRead_UnSplit(pFiles, nFilesCapacity, desc, nStartMip, nEndMip);
    }

    bool SeekToAttachedImage(FileWrapper& file)
    {
        CImageExtensionHelper::DDS_FILE_DESC ddsFileDesc;
        CImageExtensionHelper::DDS_HEADER_DXT10 ddsExtendedHeader;

        if (!file.ReadRaw(&ddsFileDesc, sizeof(ddsFileDesc)))
        {
            return false;
        }

        SwapEndian(ddsFileDesc);
        if (!ddsFileDesc.IsValid())
        {
            return false;
        }

        if (ddsFileDesc.header.IsDX10Ext())
        {
            file.ReadRaw(&ddsExtendedHeader, sizeof(CImageExtensionHelper::DDS_HEADER_DXT10));
        }
        else
        {
            memset(&ddsExtendedHeader, 0, sizeof(CImageExtensionHelper::DDS_HEADER_DXT10));
        }

        const uint32 imageFlags = CImageExtensionHelper::GetImageFlags(&ddsFileDesc.header);

        ETEX_Format eTF = DDSFormats::GetFormatByDesc(ddsFileDesc.header.ddspf, ddsExtendedHeader.dxgiFormat);
        if (eTF_Unknown == eTF)
        {
            return false;
        }

        ETEX_TileMode eTM = eTM_None;
        if (imageFlags & CImageExtensionHelper::EIF_Tiled)
        {
            switch (ddsFileDesc.header.bTileMode)
            {
            case CImageExtensionHelper::eTM_LinearPadded:
                eTM = eTM_LinearPadded;
                break;

            case CImageExtensionHelper::eTM_Optimal:
                eTM = eTM_Optimal;
                break;
            }
        }

        uint32 numSlices = (imageFlags& CImageExtensionHelper::EIF_Cubemap) ? 6 : 1;
        uint32 ddsSize = CTexture::TextureDataSize(ddsFileDesc.header.dwWidth, ddsFileDesc.header.dwHeight, ddsFileDesc.header.dwDepth, max(static_cast<DWORD>(1), ddsFileDesc.header.dwMipMapCount), numSlices, eTF, eTM);

        size_t headerEnd = file.Tell();

        file.Seek(headerEnd + ddsSize);

        uint8 tmp[1024];
        file.ReadRaw(tmp, 1024);

        const CImageExtensionHelper::DDS_HEADER* pHdr = CImageExtensionHelper::GetAttachedImage(tmp);
        if (pHdr)
        {
            file.Seek(headerEnd + ddsSize + ((const uint8*)pHdr - tmp));
            return true;
        }

        return false;
    }

    size_t LoadMipRequests(RequestInfo* pReqs, size_t nReqsCap, const DDSDesc& desc, byte* pBuffer, uint32 nStartMip, uint32 nEndMip)
    {
        size_t nReqs = 0;

        ChunkInfo names[16];
        size_t nNumNames = GetFilesToRead(names, 16, desc, nStartMip, nEndMip);
        if (nNumNames)
        {
            if (nNumNames * desc.nSides > nReqsCap)
            {
                __debugbreak();
            }

            for (ChunkInfo* it = names, * itEnd = names + nNumNames; it != itEnd; ++it)
            {
                const ChunkInfo& chunk = *it;

                uint32 nSideSize = CTexture::TextureDataSize(desc.nWidth, desc.nHeight, desc.nDepth, desc.nMips, 1, desc.eFormat, desc.eTileMode);
                uint32 nSideSizeToRead = CTexture::TextureDataSize(
                        max(1u, desc.nWidth >> chunk.nMipLevel),
                        max(1u, desc.nHeight >> chunk.nMipLevel),
                        max(1u, desc.nDepth >> chunk.nMipLevel),
                        1, 1, desc.eFormat, desc.eTileMode);

                string sFileName = string(it->fileName);

                uint32 nSrcOffset = chunk.nOffsetInFile;
                uint32 nDstOffset = 0;

                for (uint32 iSide = 0; iSide < desc.nSides; ++iSide)
                {
                    pReqs[nReqs].fileName = sFileName;
                    pReqs[nReqs].nOffs = nSrcOffset;
                    pReqs[nReqs].nRead = nSideSizeToRead;
                    pReqs[nReqs].pOut = pBuffer + (nSideSize * iSide) + nDstOffset;

                    ++nReqs;
                    nSrcOffset += nSideSizeToRead + chunk.nSideDelta;
                }

                nDstOffset += nSideSizeToRead;
            }
        }

        return nReqs;
    }

    bool LoadMipsFromRequests(const RequestInfo* pReqs, size_t nReqs)
    {
        using namespace AZ::IO;

        LOADING_TIME_PROFILE_SECTION;
        auto streamer = AZ::Interface<IStreamer>::Get();

        struct SharedRequestData
        {
            IStreamer* m_streamer;
            AZStd::atomic_int m_numProcessingRequests;
            AZStd::atomic_bool m_succeeded = true;
            AZStd::binary_semaphore m_wait;
        };
        SharedRequestData sharedData;
        sharedData.m_numProcessingRequests = nReqs;
        sharedData.m_streamer = streamer;

        AZStd::vector<FileRequestPtr> requests;
        requests.reserve(nReqs);
        streamer->CreateRequestBatch(requests, nReqs);

        auto callback = [&sharedData](FileRequestHandle request)
        {
            if (sharedData.m_streamer->GetRequestStatus(request) != IStreamerTypes::RequestStatus::Completed)
            {
                sharedData.m_succeeded = false;
            }
            if (--sharedData.m_numProcessingRequests == 0)
            {
                sharedData.m_wait.release();
            }
        };

        // load files
        for (size_t i = 0; i < nReqs; ++i)
        {
            const RequestInfo& req = pReqs[i];
            FileRequestPtr& request = requests[i];
            streamer->Read(request, req.fileName.c_str(), req.pOut, req.nRead, req.nRead,
                IStreamerTypes::s_deadlineNow, IStreamerTypes::s_priorityMedium, req.nOffs);
            streamer->SetRequestCompleteCallback(request, callback);
        }
        streamer->QueueRequestBatch(AZStd::move(requests));

        // Wait for all mips to finish loading
        sharedData.m_wait.acquire();

        if (!sharedData.m_succeeded)
        {
            AZ_Error("Render", false ,"Couldn't read one or more mip requests during async mip load");
            return false;
        }

        return true;
    }

    bool LoadMips(byte* pBuffer, const DDSDesc& desc, uint32 nStartMip, uint32 nEndMip)
    {
        bool success = 0;

        RequestInfo reqs[64];
        size_t nReqs = LoadMipRequests(reqs, 64, desc, pBuffer, nStartMip, nEndMip);

        if (nReqs)
        {
            success = LoadMipsFromRequests(reqs, nReqs);
        }

        return success;
    }

    int GetNumLastMips([[maybe_unused]] const int nWidth, [[maybe_unused]] const int nHeight, [[maybe_unused]] const int nNumMips, [[maybe_unused]] const int nSides, [[maybe_unused]] ETEX_Format eTF, [[maybe_unused]] const uint32 nFlags)
    {
        return (int)DDSSplitted::etexNumLastMips;
    }
}
