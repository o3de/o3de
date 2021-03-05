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

#ifndef CRYINCLUDE_CRY3DENGINE_CGF_CHUNKFILEWRITERS_H
#define CRYINCLUDE_CRY3DENGINE_CGF_CHUNKFILEWRITERS_H
#pragma once

#include "ChunkFileComponents.h"

// RESOURCE_COMPIELR is defined by max plugin, too, and the max plugin MAY NOT include any of azcore
// or anything else which uses modern C++
#if !defined(RESOURCE_COMPILER)
#include <AzCore/IO/FileIO.h>
#endif

namespace AZ::IO
{
    struct IArchive;
}
namespace ChunkFile
{
    struct IWriter
    {
        virtual ~IWriter()
        {
        }

        virtual void Erase() = 0;
        virtual void Close() = 0; // if Close() is not called then the file should be deleted in destructor

        virtual int32 GetPos() const = 0;

        virtual bool Write(const void* buffer, size_t size) = 0;

        // non-virtual helper function
        bool WriteZeros(size_t size);
    };


    class OsFileWriter
        : public IWriter
    {
    public:
        OsFileWriter();
        virtual ~OsFileWriter();

        bool Create(const char* filename);

        //-------------------------------------------------------
        // IWriter interface
        virtual void Erase();
        virtual void Close();

        virtual int32 GetPos() const;

        virtual bool Write(const void* buffer, size_t size);
        //-------------------------------------------------------

    private:
        string m_filename;
        FILE* m_f;
        int32 m_offset;
    };


#if !defined(RESOURCE_COMPILER)
    class CryPakFileWriter
        : public IWriter
    {
    public:
        CryPakFileWriter();
        virtual ~CryPakFileWriter();

        bool Create(AZ::IO::IArchive* pPak, const char* filename);

        //-------------------------------------------------------
        // IWriter interface
        virtual void Erase();
        virtual void Close();

        virtual int32 GetPos() const;

        virtual bool Write(const void* buffer, size_t size);
        //-------------------------------------------------------

    private:
        string m_filename;
        AZ::IO::IArchive* m_pPak;
        AZ::IO::HandleType m_fileHandle;
        int32 m_offset;
    };
#endif


    // Doesn't write any data, just computes the size
    class SizeWriter
        : public IWriter
    {
    public:
        SizeWriter()
        {
        }
        virtual ~SizeWriter()
        {
        }

        void Start()
        {
            m_offset = 0;
        }

        //-------------------------------------------------------
        // IWriter interface
        virtual void Erase()
        {
        }

        virtual void Close()
        {
        }

        virtual int32 GetPos() const
        {
            return m_offset;
        }

        virtual bool Write([[maybe_unused]] const void* buffer, size_t size)
        {
            m_offset += size;
            return true;
        }
        //-------------------------------------------------------

    private:
        int32 m_offset;
    };


    class MemoryWriter
        : public IWriter
    {
    public:
        MemoryWriter();
        virtual ~MemoryWriter();

        bool Start(void* ptr, int32 size);

        //-------------------------------------------------------
        // IWriter interface
        virtual void Erase();
        virtual void Close();

        virtual int32 GetPos() const;

        virtual bool Write(const void* buffer, size_t size);
        //-------------------------------------------------------

    private:
        char* m_ptr;
        int32 m_size;
        int32 m_offset;
    };


    // Memoryless chunk file writer
    //
    // Usage example:
    //
    //  OsFileWriter writer;
    //  if (!writer.Create(filename))
    //  {
    //      showAnErrorMessage();
    //  }
    //  else
    //  {
    //      MemorylessChunkFileWriter wr(eChunFileFormat, writer);
    //      while (wr.StartPass())
    //      {
    //          // default alignment of chunk data in file is 4, but you may change it by calling wr.SetAlignment(xxx)
    //
    //          wr.StartChunk(eEndianness_Native, chunkA_type, chunkA_version, chunkA_id);
    //          wr.AddChunkData(data_ptr0, data_size0);  // make sure that data have endianness specified by bLittleEdndian
    //          wr.AddChunkData(data_ptr1, data_size1);
    //          ...
    //          wr.StartChunk(eEndianness_Native, chunkB_type, chunkB_version, chunkB_id);
    //          wr.AddChunkData(data_ptrN, data_sizeN);
    //          ...
    //      }
    //      if (!wr.HasWrittenSuccessfully())
    //      {
    //          showAnErrorMessage();
    //      }
    //  }
    //
    // Usage example (interface way): see
    //
    //  OsFileWriter writer;
    //  if (!writer.Create(filename))
    //  {
    //      showAnErrorMessage();
    //  }
    //  else
    //  {
    //      MemorylessChunkFileWriter wr(eChunFileFormat, writer);
    //      while (wr.StartPass())
    //      {
    //          // default alignment of chunk data in file is 4, but you may change it by calling wr.SetAlignment(xxx)
    //
    //          wr.StartChunk(eEndianness_Native, chunkA_type, chunkA_version, chunkA_id);
    //          wr.AddChunkData(data_ptr0, data_size0);  // make sure that data have endianness specified by bLittleEdndian
    //          wr.AddChunkData(data_ptr1, data_size1);
    //          ...
    //          wr.StartChunk(eEndianness_Native, chunkB_type, chunkB_version, chunkB_id);
    //          wr.AddChunkData(data_ptrN, data_sizeN);
    //          ...
    //      }
    //      if (!wr.HasWrittenSuccessfully())
    //      {
    //          showAnErrorMessage();
    //      }
    //  }
    //


    struct IChunkFileWriter
    {
        virtual ~IChunkFileWriter()
        {
        }


        // Sets alignment for *beginning* of chunk data.
        // Allowed to be called at any time, influences all future
        // StartChunk() calls (until a new SetAlignment() call).
        virtual void SetAlignment(size_t alignment) = 0;

        // Returns false when there is no more passes left.
        virtual bool StartPass() = 0;

        // eEndianness specifies endianness of the data user is
        // going to provide via AddChunkData*(). The data will be
        // sent to the low-level writer as is, without any re-coding.
        virtual void StartChunk(EEndianness eEndianness, uint32 type, uint32 version, uint32 id) = 0;
        virtual void AddChunkData(void* ptr, size_t size) = 0;
        virtual void AddChunkDataZeros(size_t size) = 0;
        virtual void AddChunkDataAlignment(size_t alignment) = 0;

        virtual bool HasWrittenSuccessfully() const = 0;

        virtual IWriter* GetWriter() const = 0;
    };



    class MemorylessChunkFileWriter
        : public IChunkFileWriter
    {
    public:
        enum EChunkFileFormat
        {
            eChunkFileFormat_0x745,
            eChunkFileFormat_0x746,
        };

        MemorylessChunkFileWriter(EChunkFileFormat eFormat, IWriter* pWriter);

        //-------------------------------------------------------
        // IChunkFileWriter interface
        virtual ~MemorylessChunkFileWriter();

        virtual void SetAlignment(size_t alignment);

        virtual bool StartPass();

        virtual void StartChunk(EEndianness eEndianness, uint32 type, uint32 version, uint32 id);
        virtual void AddChunkData(void* ptr, size_t size);
        virtual void AddChunkDataZeros(size_t size);
        virtual void AddChunkDataAlignment(size_t alignment);

        virtual bool HasWrittenSuccessfully() const;

        virtual IWriter* GetWriter() const;
        //-------------------------------------------------------

    private:
        void Fail();

        static size_t ComputeSizeOfAlignmentArea(size_t pos, size_t alignment);

        size_t GetSizeOfHeader() const;
        void WriteFileHeader(int32 chunkCount, uint32 chunkTableOffsetInFile);

        size_t GetSizeOfChunkTable(int32 chunkCount) const;
        void WriteChunkTableHeader(int32 chunkCount);
        void WriteChunkEntry();

    private:
        IWriter* const m_pWriter;

        EChunkFileFormat m_eChunkFileFormat;

        size_t m_alignment;

        int32 m_chunkCount;

        int32 m_chunkIndex;
        uint16 m_chunkType;
        uint16 m_chunkVersion;
        uint32 m_chunkId;
        uint32 m_chunkSize;
        uint32 m_chunkOffsetInFile;
        EEndianness m_chunkEndianness;

        uint32 m_dataOffsetInFile;

        enum EState
        {
            eState_Init,

            eState_CountingChunks,
            eState_WritingChunkTable,
            eState_WritingData,

            eState_Success,
            eState_Fail,
        };
        EState m_eState;
    };
}  // namespace ChunkFile

#endif // CRYINCLUDE_CRY3DENGINE_CGF_CHUNKFILEWRITERS_H
