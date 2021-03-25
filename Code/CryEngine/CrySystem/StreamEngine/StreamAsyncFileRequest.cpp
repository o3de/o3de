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

#include "CrySystem_precompiled.h"

#include "StreamAsyncFileRequest.h"

#include <zlib.h>

#include "StreamEngine.h"
#include "../System.h"
#include <CryPath.h>
#include <AzFramework/Archive/Archive.h>

extern CMTSafeHeap* g_pPakHeap;

#if defined(STREAMENGINE_ENABLE_STATS)
extern SStreamEngineStatistics* g_pStreamingStatistics;
#endif

extern SStreamEngineOpenStats* g_pStreamingOpenStatistics;

volatile int CAsyncIOFileRequest::s_nLiveRequests;
SLockFreeSingleLinkedListHeader CAsyncIOFileRequest::s_freeRequests;


#ifdef STREAMENGINE_ENABLE_LISTENER

class NotifyListenerIO
{
public:
    NotifyListenerIO(IStreamEngineListener* pL, CAsyncIOFileRequest* pReq, AZ::IO::CCachedFileData* pZipEntry, uint32 readSize)
        : m_pL(pL)
        , m_pReq(pReq)
    {
        if (m_pL)
        {
            m_pL->OnStreamBeginIO(
                m_pReq,
                (pZipEntry && pZipEntry->m_pFileEntry->nMethod) ? pZipEntry->GetFileEntry()->desc.lSizeCompressed : m_pReq->m_nFileSize,
                readSize,
                pReq->m_eMediaType);
        }
    }
    ~NotifyListenerIO()
    {
        End();
    }
    void End()
    {
        if (m_pL)
        {
            m_pL->OnStreamEndIO(m_pReq);
            m_pL = NULL;
        }
    }

private:
    NotifyListenerIO(const NotifyListenerIO&);
    NotifyListenerIO& operator = (const NotifyListenerIO&);

private:
    IStreamEngineListener* m_pL;
    CAsyncIOFileRequest* m_pReq;
};

#endif //STREAMENGINE_ENABLE_LISTENER

//////////////////////////////////////////////////////////////////////////

void* CAsyncIOFileRequest::operator new (size_t sz)
{
    return CryModuleMemalign(sz, alignof(CAsyncIOFileRequest));
}

void CAsyncIOFileRequest::operator delete(void* p)
{
    CryModuleMemalignFree(p);
}

//////////////////////////////////////////////////////////////////////////
CAsyncIOFileRequest::CAsyncIOFileRequest()
    : m_nRefCount(0)
    , m_pMemoryBuffer(NULL)
{
    Reset();
}

//////////////////////////////////////////////////////////////////////////
CAsyncIOFileRequest::~CAsyncIOFileRequest()
{
#ifndef _RELEASE
    if (m_pMemoryBuffer)
    {
        __debugbreak();
    }
    if (m_eType)
    {
        __debugbreak();
    }
#endif
}

//////////////////////////////////////////////////////////////////////////
uint32 CAsyncIOFileRequest::ConfigureRead(AZ::IO::CCachedFileData* pFileData)
{
    AZ::IO::ZipDir::FileEntry* pFileEntry = pFileData ? pFileData->m_pFileEntry : NULL;

    if (!pFileData || !pFileEntry->IsCompressed())
    {
        m_bCompressedBuffer = false;
        m_nFileSizeCompressed = m_nFileSize;
        m_nSizeOnMedia = m_nRequestedSize;

        m_nPageReadStart = m_nRequestedOffset;
        m_nPageReadEnd = m_nRequestedOffset + m_nRequestedSize;
    }
    else
    {
        m_bCompressedBuffer = true;
        m_nFileSize = pFileEntry->desc.lSizeUncompressed;
        m_nFileSizeCompressed = pFileEntry->desc.lSizeCompressed;
        m_nSizeOnMedia = m_nFileSizeCompressed;

        m_nPageReadStart = 0;
        m_nPageReadEnd = m_nFileSizeCompressed;
    }

    if (pFileData && !m_bWriteOnlyExternal)
    {
        m_crc32FromHeader = pFileEntry->desc.lCRC32;
    }

    m_nPageReadCurrent = 0;

    m_bStreamInPlace = !m_bCompressedBuffer || ((!m_pExternalMemoryBuffer || !m_bWriteOnlyExternal) && (m_nFileSize > m_nFileSizeCompressed));
    m_bReadBegun = 1;

    return 0;
}

bool CAsyncIOFileRequest::CanReadInPages()
{
    bool bReadInBlocks = false;

    if (g_cvars.sys_streaming_in_blocks)
    {
        //stream is compressed and uncompressed size is greater than one page
        if (!m_bCompressedBuffer || m_nFileSizeCompressed > STREAMING_BLOCK_SIZE)
        {
            bReadInBlocks = true;
        }
    }

    return bReadInBlocks;
}

uint32 CAsyncIOFileRequest::AllocateOutput([[maybe_unused]] AZ::IO::CCachedFileData* pZipEntry)
{
    if (!m_bOutputAllocated)
    {
        uint32 nAllocSize = 0;
        uint32 nReadAllocSize = 0;
        uint32 nZStreamOffs = 0;
        uint32 nLookaheadOffs = 0;

        if (m_pExternalMemoryBuffer)
        {
            nReadAllocSize = m_bCompressedBuffer
                ? (m_nRequestedSize < m_nFileSize ? m_nFileSize : 0)
                : 0;
        }
        else
        {
            nReadAllocSize = m_bCompressedBuffer
                ? m_nFileSize
                : m_nRequestedSize;
        }

        nAllocSize = Align(nReadAllocSize, BUFFER_ALIGNMENT);

        bool bReadInBlocks = CanReadInPages();
        bool bNeedsLookahead = m_bStreamInPlace;
        bool bBlockDecompress = m_bCompressedBuffer && bReadInBlocks;

        if (bBlockDecompress)
        {
            nZStreamOffs = nAllocSize;
            nAllocSize += Align(sizeof(z_stream), BUFFER_ALIGNMENT);

            if (bNeedsLookahead)
            {
                nLookaheadOffs = nAllocSize;
                nAllocSize += Align(sizeof(*m_pLookahead), BUFFER_ALIGNMENT);
            }
        }

        char* pBuffer = NULL;

        if (nAllocSize)
        {
            const char* usageHint = "AsyncIO TempBuffer";
            pBuffer = (char*)GetStreamEngine()->TempAlloc(nAllocSize, usageHint, true, IgnoreOutofTmpMem(), BUFFER_ALIGNMENT);
            if (!pBuffer)
            {
                return ERROR_OUT_OF_MEMORY;
            }
        }

        if (pBuffer)
        {
            m_pMemoryBuffer = pBuffer;
            m_nMemoryBufferSize = nAllocSize;
        }

        if (nReadAllocSize)
        {
            m_pReadMemoryBuffer = pBuffer;
            m_nReadMemoryBufferSize = nReadAllocSize;
        }
        else
        {
            m_pReadMemoryBuffer = m_pExternalMemoryBuffer;
            m_nReadMemoryBufferSize = m_nRequestedSize;
        }

        m_pOutputMemoryBuffer = m_pExternalMemoryBuffer
            ? m_pExternalMemoryBuffer
            : m_pReadMemoryBuffer;

        if (bBlockDecompress)
        {
            m_pZlibStream = (z_stream*)&pBuffer[nZStreamOffs];
            memset(m_pZlibStream, 0, sizeof(z_stream));

            if (bNeedsLookahead)
            {
                m_pLookahead = new (&pBuffer[nLookaheadOffs]) AZ::IO::ZipDir::UncompressLookahead;
            }

            m_pZlibStream->zalloc = CMTSafeHeap::StaticAlloc;
            m_pZlibStream->zfree = CMTSafeHeap::StaticFree;
            m_pZlibStream->opaque = g_pPakHeap;
        }

        if (m_bCompressedBuffer)
        {
            m_pDecompQueue = new SStreamJobQueue;
        }

        // Doesn't need to be atomic, as there's no concurrency yet.
        int nMemoryBufferUsers = 0;
        if (nReadAllocSize > 0)
        {
            ++nMemoryBufferUsers;
        }
        if (m_bCompressedBuffer)
        {
            ++nMemoryBufferUsers;
        }
        m_nMemoryBufferUsers = nMemoryBufferUsers;

        m_bOutputAllocated = 1;
    }

    return 0;
}

byte* CAsyncIOFileRequest::AllocatePage(size_t sz, bool bOnlyPakMem, SStreamPageHdr*& pHdrOut)
{
    const char* usageHint = "streaming page";
    size_t nSzAligned = Align(sz, BUFFER_ALIGNMENT);
    size_t nToAlloc = nSzAligned + sizeof(SStreamPageHdr);
    byte* pRet = (byte*)GetStreamEngine()->TempAlloc(nToAlloc, usageHint, true, !bOnlyPakMem, BUFFER_ALIGNMENT);
    if (pRet)
    {
        pHdrOut = new (pRet + nSzAligned)SStreamPageHdr(nToAlloc);
    }

    return pRet;
}

//////////////////////////////////////////////////////////////////////////
void CAsyncIOFileRequest::Cancel()
{
    if (!HasFailed())
    {
        CryOptionalAutoLock<CryCriticalSection> readLock(m_externalBufferLockRead, m_pExternalMemoryBuffer != NULL);
        CryOptionalAutoLock<CryCriticalSection> decompLock(m_externalBufferLockDecompress, m_pExternalMemoryBuffer != NULL);

        Failed(ERROR_USER_ABORT);
    }
}

void CAsyncIOFileRequest::SyncWithDecompress()
{
    m_decompJobExecutor.reset(); // destructor waits on job completion
}

//////////////////////////////////////////////////////////////////////////
bool CAsyncIOFileRequest::TryCancel()
{
    if (!HasFailed())
    {
        bool bExt = false;
        if (m_pExternalMemoryBuffer != NULL)
        {
            if (!m_externalBufferLockRead.TryLock())
            {
                return false;
            }
            if (!m_externalBufferLockDecompress.TryLock())
            {
                m_externalBufferLockRead.Unlock();
                return false;
            }
            bExt = true;
        }

        Failed(ERROR_USER_ABORT);

        if (bExt)
        {
            m_externalBufferLockDecompress.Unlock();
            m_externalBufferLockRead.Unlock();
        }
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CAsyncIOFileRequest::FreeBuffer()
{
    if (m_pZlibStream)
    {
        //if the stream was cancelled in flight, inform zlib to free internal allocs
        if (m_pZlibStream->state)
        {
            inflateEnd(m_pZlibStream);
        }

        m_pZlibStream = NULL;
    }

    m_pLookahead = NULL;

    SStreamEngineTempMemStats& tms = GetStreamEngine()->GetTempMemStats();

    if (m_pDecompQueue)
    {
        m_pDecompQueue->Flush(tms);
        delete m_pDecompQueue;
        m_pDecompQueue = NULL;
    }

    if (m_pMemoryBuffer)
    {
        CStreamEngine* pStreamEngine = GetStreamEngine();

        pStreamEngine->TempFree(m_pMemoryBuffer, m_nMemoryBufferSize);
        m_pMemoryBuffer = 0;
    }

#ifdef STREAMENGINE_ENABLE_STATS
    // Update Streaming statistics.
    if (g_pStreamingStatistics && m_nSizeOnMedia != 0 && m_bStatsUpdated)
    {
        m_bStatsUpdated = false;
        int nSize = (int)m_nSizeOnMedia;
        SStreamEngineStatistics& stats = *g_pStreamingStatistics;
        CryInterlockedAdd(&stats.nPendingReadBytes, -nSize);
        CryInterlockedAdd(&stats.typeInfo[m_eType].nPendingReadBytes, -nSize);
    }
#endif
}

CAsyncIOFileRequest* CAsyncIOFileRequest::Allocate(EStreamTaskType eType)
{
    CAsyncIOFileRequest* pReq = static_cast<CAsyncIOFileRequest*>(CryInterlockedPopEntrySList(s_freeRequests));
    IF_UNLIKELY (!pReq)
    {
        pReq = new CAsyncIOFileRequest;
    }

    pReq->Init(eType);

    return pReq;
}

void CAsyncIOFileRequest::Flush()
{
    for (CAsyncIOFileRequest* pReq = static_cast<CAsyncIOFileRequest*>(CryInterlockedPopEntrySList(s_freeRequests));
         pReq;
         pReq = static_cast<CAsyncIOFileRequest*>(CryInterlockedPopEntrySList(s_freeRequests)))
    {
        delete pReq;
    }
}

void CAsyncIOFileRequest::Reset()
{
    m_decompJobExecutor.reset(); // destructor waits on job completion

#ifndef _RELEASE
    if (m_pMemoryBuffer)
    {
        __debugbreak();
    }
#endif

    m_pReadStream = NULL;
    m_strFileName.resize(0);
    m_pakFile.resize(0);

    // Reset POD members of the structure
    memset(&m_nSortKey, 0, ((char*)(this + 1) - (char*)&m_nSortKey));
}

void CAsyncIOFileRequest::Init(EStreamTaskType eType)
{
#ifndef _RELEASE
    if (!eType)
    {
        __debugbreak();
    }
#endif

    m_eType = eType;

#ifdef STREAMENGINE_ENABLE_STATS
    m_startTime = gEnv->pTimer->GetAsyncTime();
#endif

    if (g_pStreamingOpenStatistics)
    {
        SStreamEngineOpenStats& stats = *g_pStreamingOpenStatistics;
        CryInterlockedIncrement(&stats.nOpenRequestCount);
        CryInterlockedIncrement(&stats.nOpenRequestCountByType[eType]);
    }

    CryInterlockedIncrement(&s_nLiveRequests);
}

void CAsyncIOFileRequest::Finalize()
{
#ifndef _RELEASE
    if (!m_eType)
    {
        __debugbreak();
    }
#endif

#ifdef STREAMENGINE_ENABLE_LISTENER
    IStreamEngineListener* pListener = gEnv->pSystem->GetStreamEngine()->GetListener();
    if (pListener)
    {
        pListener->OnStreamDone(this);
    }
#endif

    if (g_pStreamingOpenStatistics)
    {
        SStreamEngineOpenStats& stats = *g_pStreamingOpenStatistics;
        CryInterlockedDecrement(&stats.nOpenRequestCount);
        CryInterlockedDecrement(&stats.nOpenRequestCountByType[m_eType]);
    }

    CryInterlockedDecrement(&s_nLiveRequests);

    FreeBuffer();
    Reset();
}

uint32 CAsyncIOFileRequest::OpenFile(CCryFile& file)
{
    auto* pIPak = gEnv ? gEnv->pCryPak : NULL;
    PREFAST_ASSUME(pIPak);

    if (m_pReadStream && m_pReadStream->GetParams().nFlags & IStreamEngine::FLAGS_FILE_ON_DISK)
    {
        pIPak = 0;
    }

    file = CCryFile(pIPak);
    if (!file.Open(m_strFileName.c_str(), "rb", AZ::IO::IArchive::FOPEN_FORSTREAMING))
    {
        return ERROR_CANT_OPEN_FILE;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
uint32 CAsyncIOFileRequest::ReadFile(CStreamingIOThread* pIOThread)
{
    uint32 nError = m_nError;

    if (nError)
    {
        return nError;
    }

    CCryFile file;
    if ((nError = OpenFile(file)))
    {
        return nError;
    }

    m_nFileSize = file.GetLength();

    if (m_nRequestedOffset >= m_nFileSize)
    {
        return ERROR_OFFSET_OUT_OF_RANGE;
    }

    if (m_nRequestedOffset + m_nRequestedSize > m_nFileSize)
    {
        return ERROR_SIZE_OUT_OF_RANGE;
    }

    if (m_nRequestedSize == 0)
    {
        // by default, we read the whole file
        m_nRequestedSize = m_nFileSize - m_nRequestedOffset;
    }

    if (!m_pExternalMemoryBuffer && m_pReadStream)
    {
        bool bAbortOnFailToAlloc = false;
        CReadStream* pReadStream = static_cast<CReadStream*>(&*m_pReadStream);
        m_pExternalMemoryBuffer = pReadStream->OnNeedStorage(m_nFileSize, bAbortOnFailToAlloc);

        if (!m_pExternalMemoryBuffer && bAbortOnFailToAlloc)
        {
            Cancel();
            m_pReadStream->Abort();
            return ERROR_USER_ABORT;
        }
    }

    if (HasFailed())
    {
        return m_nError;
    }

    auto pZipEntry = ((AZ::IO::Archive*)(gEnv->pCryPak))->GetOpenedFileDataInZip(file.GetHandle());
    if ((nError = ConfigureRead(pZipEntry.get())))
    {
        return nError;
    }

    if ((nError = AllocateOutput(pZipEntry.get())))
    {
        return nError;
    }

    return ReadFileInPages(pIOThread, file);
}

uint32 CAsyncIOFileRequest::ReadFileResume(CStreamingIOThread* pIOThread)
{
    uint32 nError = m_nError;

    if (nError)
    {
        return nError;
    }

    CCryFile file;
    if ((nError = OpenFile(file)))
    {
        return nError;
    }

    auto pZipEntry = ((AZ::IO::Archive*)(gEnv->pCryPak))->GetOpenedFileDataInZip(file.GetHandle());

    if ((nError = AllocateOutput(pZipEntry.get())))
    {
        return nError;
    }

#ifdef STREAMENGINE_ENABLE_LISTENER
    IStreamEngineListener* pListener = gEnv->pSystem->GetStreamEngine()->GetListener();
    if (pListener)
    {
        pListener->OnStreamResumed(this);
    }
#endif

    return ReadFileInPages(pIOThread, file);
}

uint32 CAsyncIOFileRequest::ReadFileInPages(CStreamingIOThread* pIOThread, CCryFile& file)
{
    auto pCryPak = static_cast<AZ::IO::Archive*>(gEnv->pCryPak);
    auto pZipEntry = pCryPak->GetOpenedFileDataInZip(file.GetHandle());

    AZ::IO::ZipDir::Cache* pZip = NULL;
    unsigned int nZipFlags = 0;
    if (pZipEntry)
    {
        pZip = pZipEntry->GetZip();
        nZipFlags = pZipEntry->m_nArchiveFlags;
    }

    if (pIOThread->IsMisscheduled(EStreamSourceMediaType::eStreamSourceTypeHDD))
    {
        // We're on the wrong IO thread!
        return ERROR_MISSCHEDULED;
    }

    bool bReadInPages = CanReadInPages();

    uint32 nPageReadLen = (m_nPageReadEnd - m_nPageReadStart);

    bool const bCompressed = m_bCompressedBuffer;
    bool const bInPlace = m_bStreamInPlace;
    bool const bIgnoreOutOfTmp = IgnoreOutofTmpMem();

    size_t const nReadStartOffset = bCompressed
        ? (m_nFileSize - m_nFileSizeCompressed)
        : 0;

    byte* const pReadBase = (byte*)m_pReadMemoryBuffer + nReadStartOffset;
    byte* const pReadEnd = (byte*)m_pReadMemoryBuffer + m_nReadMemoryBufferSize;

    CStreamEngine* pStreamEngine = static_cast<CStreamEngine*>(gEnv->pSystem->GetStreamEngine());

    uint32 nPageSize = bReadInPages
        ? min((uint32)STREAMING_PAGE_SIZE, nPageReadLen - m_nPageReadCurrent)
        : nPageReadLen - m_nPageReadCurrent;

    while (nPageSize > 0)
    {
        CryOptionalAutoLock<CryCriticalSection> readLock(m_externalBufferLockRead, m_pExternalMemoryBuffer != NULL);

        uint32 nError = m_nError;

        if (nError)
        {
            return nError;
        }

        //check if job needs to be pre-empted
        if ((nError = ReadFileCheckPreempt(pIOThread)))
        {
            return nError;
        }

        byte* pReadTarget = pReadBase + m_nPageReadCurrent;
        byte* pReadTargetEnd = pReadTarget + nPageSize;
        bool bTemporaryReadTarget = false;
        SStreamPageHdr* pTemporaryPageHdr = NULL;

        if (bInPlace)
        {
            if (pReadTargetEnd > pReadEnd)
            {
                __debugbreak();
            }
        }
        else
        {
            pReadTarget = AllocatePage(nPageSize, !bIgnoreOutOfTmp, pTemporaryPageHdr);

            if (!pReadTarget)
            {
                return ERROR_OUT_OF_MEMORY;
            }

            bTemporaryReadTarget = true;
            pTemporaryPageHdr->nRefs = 1;
        }

#ifndef _RELEASE
        if (m_nPageReadCurrent + nPageSize > nPageReadLen)
        {
            __debugbreak();
        }
#endif

        {
#ifdef STREAMENGINE_ENABLE_LISTENER
            NotifyListenerIO IOListener(gEnv->pSystem->GetStreamEngine()->GetListener(), this, pZipEntry.get(), nPageSize);
#endif

#ifdef STREAMENGINE_ENABLE_STATS
            CTimeValue t0 = gEnv->pTimer->GetAsyncTime();
#endif

            //printf("[StreamRead] %p %i %p %i %i\n", this, m_bCompressedBuffer, pReadTarget, m_nPageReadStart + m_nPageReadCurrent, nPageSize);

            bool bReadOk = false;

            if (pZipEntry)
            {
                bReadOk = pZipEntry->m_pZip->ReadFile(pZipEntry->m_pFileEntry, pReadTarget, nullptr) == AZ::IO::ZipDir::ZD_ERROR_SUCCESS;
            }
            else
            {
                file.Seek(m_nPageReadStart + m_nPageReadCurrent, SEEK_SET);
                bReadOk = file.ReadRaw(pReadTarget, nPageSize) == nPageSize;
            }

            if (bReadOk)
            {
                //send each block to listener
#ifdef STREAMENGINE_ENABLE_LISTENER
                IOListener.End();
#endif

#ifdef STREAMENGINE_ENABLE_STATS
                m_readTime += gEnv->pTimer->GetAsyncTime() - t0;
#endif

                //release external mem lock, allows jobs to be cancelled mid stream
                readLock.Release();
            }
            else
            {
                if (bTemporaryReadTarget)
                {
                    GetStreamEngine()->TempFree(pReadTarget, pTemporaryPageHdr->nSize);
                }

                return ERROR_REFSTREAM_ERROR;
            }

            bool bLastBlock = (m_nPageReadCurrent + nPageSize) == nPageReadLen;

            if (bCompressed)
            {
                PushDecompressPage(pStreamEngine->GetJobEngineState(), pReadTarget, pTemporaryPageHdr, nPageSize, bLastBlock);
            }
            else if (bTemporaryReadTarget)
            {
                __debugbreak();
            }

            if (pTemporaryPageHdr && CryInterlockedDecrement(&pTemporaryPageHdr->nRefs) == 0)
            {
                GetStreamEngine()->TempFree(pReadTarget, pTemporaryPageHdr->nSize);
            }

            m_nPageReadCurrent += nPageSize;
            nPageSize = min((uint32)STREAMING_PAGE_SIZE, nPageReadLen - m_nPageReadCurrent);
        }
    }

    return 0;
}

uint32 CAsyncIOFileRequest::ReadFileCheckPreempt(CStreamingIOThread* pIOThread)
{
    if (m_ePriority != estpUrgent)
    {
        if (pIOThread->HasUrgentRequests())
        {
            //printf("Read Job %s pre-empted mid stream. Progress %d / %d bytes\n", m_strFileName.c_str(), m_nBytesRead, m_nFileSizeCompressed);
#ifdef STREAMENGINE_ENABLE_LISTENER
            IStreamEngineListener* pListener = gEnv->pSystem->GetStreamEngine()->GetListener();
            if (pListener)
            {
                pListener->OnStreamPreempted(this);
            }
#endif
            return ERROR_PREEMPTED;
        }
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
CStreamEngine* CAsyncIOFileRequest::GetStreamEngine()
{
    return (CStreamEngine*)GetISystem()->GetStreamEngine();
}

//////////////////////////////////////////////////////////////////////////
EStreamSourceMediaType CAsyncIOFileRequest::GetMediaType()
{
    EStreamSourceMediaType mediaType = m_eMediaType;

    if (mediaType == eStreamSourceTypeUnknown)
    {
        if (m_bSortKeyComputed)
        {
            return mediaType;
        }

        if (m_strFileName.empty())
        {
            mediaType = eStreamSourceTypeMemory;
            return mediaType;
        }

        mediaType = gEnv->pCryPak->GetFileMediaType(m_strFileName.c_str());
    }

    return mediaType;
}

//////////////////////////////////////////////////////////////////////////
void CAsyncIOFileRequest::ComputeSortKey(uint64 nCurrentKeyInProgress)
{
    if (m_bSortKeyComputed)
    {
        return;
    }

    m_bSortKeyComputed = true;

    if (m_strFileName.empty())
    {
        m_eMediaType = eStreamSourceTypeMemory;
        m_nSortKey = m_ePriority;
        return;
    }
    if (HasFailed())
    {
        m_nSortKey = 0;
        return;
    }

    const int MaxPath = 0x800;
    char szFullPathBuf[MaxPath];
    const char* szFullPath = gEnv->pCryPak->AdjustFileName(m_strFileName.c_str(), szFullPathBuf, AZ_ARRAY_SIZE(szFullPathBuf), AZ::IO::IArchive::FOPEN_HINT_QUIET);

    auto pCryPak = static_cast<AZ::IO::Archive*>(gEnv->pCryPak);

    AZ::IO::ZipDir::CachePtr pZip = 0;
    unsigned int archFlags = 0;
    AZ::IO::ZipDir::FileEntry* pFileEntry = NULL;

    // tests if the given file path refers to an existing file inside registered (opened) packs
    // the path must be absolute normalized lower-case with forward-slashes
    AZ::IO::ArchiveLocationPriority varPakPriority = pCryPak->GetPakPriority();
    bool willOpenFromPak = (varPakPriority != AZ::IO::ArchiveLocationPriority::ePakPriorityFileFirst)
        || !AZ::IO::FileIOBase::GetDirectInstance()->Exists(szFullPath);
    
    if (willOpenFromPak)
    {
        pFileEntry = pCryPak->FindPakFileEntry(szFullPath, archFlags, &pZip, false);
    }

    EStreamSourceMediaType ssmt = pCryPak->GetFileMediaType(szFullPath);

    if (pFileEntry)
    {
        pZip->Refresh(pFileEntry);
        m_nDiskOffset = (uint64)pFileEntry->nFileDataOffset;

        m_nSizeOnMedia = pFileEntry->desc.lSizeCompressed;
        m_nFileSize = pFileEntry->desc.lSizeUncompressed;

        m_pakFile = pZip->GetFilePath();
    }

    m_eMediaType = ssmt;

    if (ssmt != eStreamSourceTypeMemory)
    {
        int32 nCurrentSweep = (nCurrentKeyInProgress >> 30) & ((1 << 10) - 1);
        int32 nCurrentTG = (nCurrentKeyInProgress >> 40) & ((1 << 20) - 1);

        if (pFileEntry && !pFileEntry->IsCompressed())
        {
            m_nDiskOffset += m_nRequestedOffset;
        }

        // group items by priority, then by snapped request time, then sort by disk offset
        m_nTimeGroup = (uint64)(gEnv->pTimer->GetAsyncTime().GetSeconds() / max(1, g_cvars.sys_streaming_requests_grouping_time_period));
        m_nSweep = (m_nTimeGroup == nCurrentTG)
            ? nCurrentSweep
            : 0;
        uint64 nPrioriry = m_ePriority;

        int64 nDiskOffsetKB = m_nDiskOffset >> 10; // KB
        m_nSortKey = (nDiskOffsetKB) | (((uint64)m_nTimeGroup) << 40) | ((uint64)m_nSweep << 30) | (nPrioriry << 60);

        // make sure we do not break incremental head movement within time group on every new request
        if (m_nSortKey <= nCurrentKeyInProgress)
        {
            ++m_nSweep;
            m_nSortKey = (nDiskOffsetKB) | (((uint64)m_nTimeGroup) << 40) | ((uint64)m_nSweep << 30) | (nPrioriry << 60);
        }
    }
    else
    {
        m_nSortKey = m_ePriority;
    }

#if defined(STREAMENGINE_ENABLE_STATS)
    // Update Streaming statistics.
    if (!m_bStatsUpdated && g_pStreamingStatistics && m_nSizeOnMedia != 0)
    {
        m_bStatsUpdated = true;
        SStreamEngineStatistics& stats = *g_pStreamingStatistics;

        // if the file is not compressed then it will only read the requested size
        uint32 nReadSize = m_nSizeOnMedia;
        if (m_nSizeOnMedia == m_nFileSize)
        {
            nReadSize = m_nRequestedSize;
        }

        CryInterlockedAdd(&stats.nPendingReadBytes, nReadSize);
        CryInterlockedAdd(&stats.typeInfo[m_eType].nPendingReadBytes, nReadSize);
    }
#endif
}

void CAsyncIOFileRequest::SetPriority(EStreamTaskPriority estp)
{
    m_ePriority = estp;

    if (m_eMediaType != eStreamSourceTypeMemory)
    {
        m_nSortKey &= ~(15ULL << 60);
        m_nSortKey |= static_cast<uint64>(estp) << 60;
    }
    else
    {
        m_nSortKey = m_ePriority;
    }
}

void CAsyncIOFileRequest::BumpSweep()
{
    ++m_nSweep;
    if (m_eMediaType != eStreamSourceTypeMemory)
    {
        m_nSortKey += 1 << 30;
    }
}

//////////////////////////////////////////////////////////////////////////
bool CAsyncIOFileRequest::IgnoreOutofTmpMem() const
{
    if (m_pReadStream &&
        (m_pReadStream->GetParams().ePriority == estpUrgent ||
         m_pReadStream->GetParams().nFlags & IStreamEngine::FLAGS_IGNORE_TMP_OUT_OF_MEM))
    {
        return true;
    }

    return false;
}

SStreamRequestQueue::SStreamRequestQueue()
{
    m_requests.reserve(4096);
}

SStreamRequestQueue::~SStreamRequestQueue()
{
    Reset();
}

void SStreamRequestQueue::Reset()
{
    CryAutoLock<CryCriticalSection> l(m_lock);
    for (size_t i = 0, c = m_requests.size(); i != c; ++i)
    {
        m_requests[i]->Release();
    }
    m_requests.clear();
}

bool SStreamRequestQueue::IsEmpty() const
{
    return m_requests.empty();
}

bool SStreamRequestQueue::TryPopRequest(CAsyncIOFileRequest_AutoPtr& pOut)
{
    CryAutoLock<CryCriticalSection> l(m_lock);
    if (!m_requests.empty())
    {
        pOut = m_requests.front();
        pOut->Release();
        m_requests.erase(m_requests.begin());
        return true;
    }
    return false;
}

void* SStreamEngineTempMemStats::TempAlloc(CMTSafeHeap* pHeap, size_t nSize, const char* szDbgSource, bool bFallBackToMalloc, bool bUrgent, uint32 align)
{
    // Only allow falling back to malloc if the size fits within the stream budget, the request is urgent, or the temp memory is 0 - for those files that are over budget on their own.
    long nInUse = m_nTempAllocatedMemory;
    bFallBackToMalloc =
        bUrgent
        || (bFallBackToMalloc && ((nInUse == 0) || (nInUse + static_cast<long>(nSize) <= m_nTempMemoryBudget)))
    ;

    void* p = pHeap->TempAlloc(nSize, szDbgSource, bFallBackToMalloc, align);
#if MTSAFE_USE_GENERAL_HEAP
    bool bInGenHeap = pHeap->IsInGeneralHeap(p);
#else
    bool bInGenHeap = false;
#endif
    if (p && !bInGenHeap)
    {
        ReportTempMemAlloc(nSize, 0, false);
    }

    return p;
}
