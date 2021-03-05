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

#include <platform.h>
#include "ReadOnlyChunkFile.h"
#include "ChunkFileComponents.h"
#include "ChunkFileReaders.h"
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/IO/FileIO.h>

#define MAX_CHUNKS_NUM 10000000

#if !defined(FUNCTION_PROFILER_3DENGINE)
#   define FUNCTION_PROFILER_3DENGINE
#endif

#if !defined(LOADING_TIME_PROFILE_SECTION)
#   define LOADING_TIME_PROFILE_SECTION
#endif

//////////////////////////////////////////////////////////////////////////
CReadOnlyChunkFile::CReadOnlyChunkFile(bool bCopyFileData, bool bNoWarningMode)
{
    m_pFileBuffer = 0;
    m_nBufferSize = 0;

    m_bNoWarningMode = bNoWarningMode;

    m_bOwnFileBuffer = false;
    m_bLoaded = false;
    m_bCopyFileData = bCopyFileData;
}

CReadOnlyChunkFile::~CReadOnlyChunkFile()
{
    FreeBuffer();
}

//////////////////////////////////////////////////////////////////////////
void CReadOnlyChunkFile::FreeBuffer()
{
    if (m_pFileBuffer && m_bOwnFileBuffer)
    {
        delete [] m_pFileBuffer;
    }
    m_pFileBuffer = 0;
    m_nBufferSize = 0;
    m_bOwnFileBuffer = false;
    m_bLoaded = false;
}

//////////////////////////////////////////////////////////////////////////
CReadOnlyChunkFile::ChunkDesc* CReadOnlyChunkFile::GetChunk(int nIndex)
{
    assert(size_t(nIndex) < m_chunks.size());
    return &m_chunks[nIndex];
}

//////////////////////////////////////////////////////////////////////////
const CReadOnlyChunkFile::ChunkDesc* CReadOnlyChunkFile::GetChunk(int nIndex) const
{
    assert(size_t(nIndex) < m_chunks.size());
    return &m_chunks[nIndex];
}

// number of chunks
int CReadOnlyChunkFile::NumChunks() const
{
    return (int)m_chunks.size();
}

//////////////////////////////////////////////////////////////////////////
CReadOnlyChunkFile::ChunkDesc* CReadOnlyChunkFile::FindChunkByType(ChunkTypes nChunkType)
{
    for (size_t i = 0, count = m_chunks.size(); i < count; ++i)
    {
        if (m_chunks[i].chunkType == nChunkType)
        {
            return &m_chunks[i];
        }
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
CReadOnlyChunkFile::ChunkDesc* CReadOnlyChunkFile::FindChunkById(int id)
{
    ChunkDesc chunkToFind;
    chunkToFind.chunkId = id;

    std::vector<ChunkDesc>::iterator it = std::lower_bound(m_chunks.begin(), m_chunks.end(), chunkToFind, IChunkFile::ChunkDesc::LessId);
    if (it != m_chunks.end() && id == (*it).chunkId)
    {
        return &(*it);
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
bool CReadOnlyChunkFile::ReadChunkTableFromBuffer()
{
    LOADING_TIME_PROFILE_SECTION;

    if (m_pFileBuffer == 0)
    {
        m_LastError.Format("Unexpected empty buffer");
        return false;
    }

    {
        ChunkFile::MemoryReader f;

        if (!f.Start(m_pFileBuffer, m_nBufferSize))
        {
            m_LastError.Format("Empty memory chunk file");
            return false;
        }

        bool bStripHeaders = false;
        const char* err = 0;

        err = ChunkFile::GetChunkTableEntries_0x746(&f, m_chunks);
        if (err)
        {
            err = ChunkFile::GetChunkTableEntries_0x744_0x745(&f, m_chunks);
            bStripHeaders  = true;
        }

        if (!err)
        {
            for (size_t i = 0; i < m_chunks.size(); ++i)
            {
                ChunkDesc& cd = m_chunks[i];
                cd.data = m_pFileBuffer + cd.fileOffset;
            }
            if (bStripHeaders)
            {
                err = ChunkFile::StripChunkHeaders_0x744_0x745(&f, m_chunks);
            }
        }

        if (err)
        {
            m_LastError = err;
            return false;
        }
    }

    // Sort chunks by Id, for faster queries later (see FindChunkById()).
    std::sort(m_chunks.begin(), m_chunks.end(), IChunkFile::ChunkDesc::LessId);

    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CReadOnlyChunkFile::Read(const char* filename)
{
    LOADING_TIME_PROFILE_SECTION;

    FreeBuffer();

    if (!AZ::IO::FileIOBase::GetInstance())
    {
        m_LastError = "File system not ready yet.";
        return false;
    }

    if (!AZ::IO::FileIOBase::GetInstance()->Exists(filename))
    {
        m_LastError.Format("File '%s' not found", filename);
        return false;
    }

    AZ::u64 fileSize;
    if (!AZ::IO::FileIOBase::GetInstance()->Size(filename, fileSize))
    {
        m_LastError.Format("Failed to retrieve file size for '%s'", filename);
        return false;
    }

    m_pFileBuffer = new char[fileSize];
    m_bOwnFileBuffer = true;

    AZ::IO::FileIOStream stream(filename, AZ::IO::OpenMode::ModeRead);
    if (stream.Read(fileSize, m_pFileBuffer) != fileSize)
    {
        m_LastError.Format("Failed to read %u bytes from file '%s'", fileSize, filename);
        return false;
    }

    m_nBufferSize = aznumeric_caster(fileSize);

    if (!ReadChunkTableFromBuffer())
    {
        return false;
    }

    m_bLoaded = true;

    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CReadOnlyChunkFile::ReadFromMemory(const void* pData, int nDataSize)
{
    LOADING_TIME_PROFILE_SECTION;

    FreeBuffer();

    m_pFileBuffer = (char*)pData;
    m_bOwnFileBuffer = false;
    m_nBufferSize = nDataSize;

    if (!ReadChunkTableFromBuffer())
    {
        return false;
    }
    m_bLoaded = true;
    return true;
}
