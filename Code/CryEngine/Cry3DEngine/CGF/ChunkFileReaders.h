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

#ifndef CRYINCLUDE_CRY3DENGINE_CGF_CHUNKFILEREADERS_H
#define CRYINCLUDE_CRY3DENGINE_CGF_CHUNKFILEREADERS_H
#pragma once

#include <CryFile.h>        // for CCryFile
#include "ChunkFileComponents.h"
#include <CryFile.h>
#include "IChunkFile.h"

namespace ChunkFile
{
    struct IReader
    {
        virtual ~IReader()
        {
        }

        virtual void Close() = 0;
        virtual int32 GetSize() = 0;
        virtual bool SetPos(int32 pos) = 0;
        virtual bool Read(void* buffer, size_t size) = 0;
    };


    class CryFileReader
        : public IReader
    {
    public:
        CryFileReader();
        virtual ~CryFileReader();

        bool Open(const char* filename);

        //-------------------------------------------------------
        // IReader interface
        virtual void Close();
        virtual int32 GetSize();
        virtual bool SetPos(int32 pos);
        virtual bool Read(void* buffer, size_t size);
        //-------------------------------------------------------

    private:
        CCryFile m_f;
        int32 m_offset;
    };


    class MemoryReader
        : public IReader
    {
    public:
        MemoryReader();
        virtual ~MemoryReader();

        bool Start(void* ptr, int32 size);

        //-------------------------------------------------------
        // IReader interface
        virtual void Close();
        virtual int32 GetSize();
        virtual bool SetPos(int32 pos);
        virtual bool Read(void* buffer, size_t size);
        //-------------------------------------------------------

    private:
        char* m_ptr;
        int32 m_size;
        int32 m_offset;
    };


    const char* GetChunkTableEntries_0x744_0x745(IReader* pReader, std::vector<IChunkFile::ChunkDesc>& chunks);
    const char* GetChunkTableEntries_0x744_0x745(IReader* pReader, std::vector<IChunkFile::ChunkDesc*>& chunks);

    const char* GetChunkTableEntries_0x746(IReader* pReader, std::vector<IChunkFile::ChunkDesc>& chunks);
    const char* GetChunkTableEntries_0x746(IReader* pReader, std::vector<IChunkFile::ChunkDesc*>& chunks);

    const char* StripChunkHeaders_0x744_0x745(IReader* pReader, std::vector<IChunkFile::ChunkDesc>& chunks);
    const char* StripChunkHeaders_0x744_0x745(IReader* pReader, std::vector<IChunkFile::ChunkDesc*>& chunks);
}  // namespace ChunkFile

#endif // CRYINCLUDE_CRY3DENGINE_CGF_CHUNKFILEREADERS_H
