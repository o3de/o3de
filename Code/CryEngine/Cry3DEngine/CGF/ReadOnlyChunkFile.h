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

#ifndef CRYINCLUDE_CRY3DENGINE_CGF_READONLYCHUNKFILE_H
#define CRYINCLUDE_CRY3DENGINE_CGF_READONLYCHUNKFILE_H
#pragma once

#include <CrySizer.h>
#include <CryHeaders.h>
#include <smartptr.h>
#include <IChunkFile.h>

////////////////////////////////////////////////////////////////////////
// Chunk file reader.
// Accesses a chunked file structure through file mapping object.
// Opens a chunk file and checks for its validity.
// If it's invalid, closes it as if there was no open operation.
// Error handling is performed through the return value of Read: it must
// be true for successfully open files
////////////////////////////////////////////////////////////////////////

class CReadOnlyChunkFile
    : public IChunkFile
{
public:
    //////////////////////////////////////////////////////////////////////////
    CReadOnlyChunkFile(bool bCopyFileData, bool bNoWarningMode = false);
    virtual ~CReadOnlyChunkFile();

    // interface IChunkFile --------------------------------------------------

    virtual void Release() { delete this; }

    virtual bool IsReadOnly() const { return true; }
    virtual bool IsLoaded() const { return m_bLoaded; }

    virtual bool Read(const char* filename);
    virtual bool ReadFromMemory(const void* pData, int nDataSize);

    virtual bool Write([[maybe_unused]] const char* filename) { return false; }
    virtual bool WriteToMemoryBuffer([[maybe_unused]] void** pData, [[maybe_unused]] int* nSize) { return false; }
    virtual void ReleaseMemoryBuffer() {}

    virtual int AddChunk([[maybe_unused]] ChunkTypes chunkType, [[maybe_unused]] int chunkVersion, [[maybe_unused]] EEndianness eEndianness, [[maybe_unused]] const void* chunkData, [[maybe_unused]] int chunkSize) { return -1; }
    virtual void DeleteChunkById([[maybe_unused]] int nChunkId) {}
    virtual void DeleteChunksByType([[maybe_unused]] ChunkTypes nChunkType) {}

    virtual ChunkDesc* FindChunkByType(ChunkTypes nChunkType);
    virtual ChunkDesc* FindChunkById(int nChunkId);

    virtual int NumChunks() const;

    virtual ChunkDesc* GetChunk(int nIndex);
    virtual const ChunkDesc* GetChunk(int nIndex) const;

    virtual const char* GetLastError() const { return m_LastError; }

    // -----------------------------------------------------------------------

    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
        pSizer->AddObject(m_LastError);
        pSizer->AddObject(m_chunks);
    }
private:
    bool ReadChunkTableFromBuffer();
    void FreeBuffer();

private:
    // this variable contains the last error occurred in this class
    string m_LastError;

    std::vector<ChunkDesc> m_chunks;

    char* m_pFileBuffer;
    int m_nBufferSize;
    bool m_bOwnFileBuffer;
    bool m_bNoWarningMode;
    bool m_bLoaded;
    bool m_bCopyFileData;
};

TYPEDEF_AUTOPTR(CReadOnlyChunkFile);

#endif // CRYINCLUDE_CRY3DENGINE_CGF_READONLYCHUNKFILE_H
