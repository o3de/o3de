/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>
#include <AzCore/std/string/string.h>

namespace AZ::IO
{
    using SizeType = AZ::u64;
    using OffsetType = AZ::s64;
    enum class OpenMode : AZ::u32;

    /*
     * Base Class interface for implementing AZ::IO streams
     */
    class GenericStream
    {
    public:
        enum SeekMode
        {
            ST_SEEK_BEGIN,
            ST_SEEK_CUR,
            ST_SEEK_END,
        };

        virtual ~GenericStream() {}

        virtual bool        IsOpen() const = 0;
        virtual bool        CanSeek() const = 0;
        virtual bool        CanRead() const = 0;
        virtual bool        CanWrite() const = 0;
        virtual void        Seek(OffsetType bytes, SeekMode mode) = 0;
        virtual SizeType    Read(SizeType bytes, void* oBuffer) = 0;
        virtual SizeType    Write(SizeType bytes, const void* iBuffer) = 0;
        //! Alternate version of Write that can read data directly from an input stream instead of a pre-populated buffer.
        //! Stream classes should override the default implementation if they have their own internal pre-allocated buffer
        //! that can be passed into the inputStream for direct population without the need for an intermediate buffer.
        virtual SizeType    WriteFromStream(SizeType bytes, GenericStream* inputStream);
        virtual SizeType    GetCurPos() const = 0;
        virtual SizeType    GetLength() const = 0;
        virtual SizeType    ReadAtOffset(SizeType bytes, void* oBuffer, OffsetType offset = -1);
        virtual SizeType    WriteAtOffset(SizeType bytes, const void* iBuffer, OffsetType offset = -1);
        virtual bool        IsCompressed() const { return false; }
        virtual const char* GetFilename() const { return ""; }
        virtual OpenMode    GetModeFlags() const { return OpenMode(); }
        virtual bool        ReOpen() { return true; }
        virtual void        Close() {}

        static inline constexpr size_t StreamToStreamCopyBufferSize = 256;

    protected:
        SizeType ComputeSeekPosition(OffsetType bytes, SeekMode mode);
    };

    /*
     * Wraps around a SystemFile
     */
    class SystemFile;
    class SystemFileStream
        : public GenericStream
    {
    public:
        SystemFileStream(SystemFile* file, bool isOwner, SizeType baseOffset = 0, SizeType fakeLen = static_cast<SizeType>(-1));
        ~SystemFileStream() override;

        bool Open(const char* path, OpenMode mode);

        void Close() override;

        bool        IsOpen() const override;
        bool        CanSeek() const override { return true; }
        bool        CanRead() const override;
        bool        CanWrite() const override;
        void        Seek(OffsetType bytes, SeekMode mode) override;
        SizeType    Read(SizeType bytes, void* oBuffer) override;
        SizeType    Write(SizeType bytes, const void* iBuffer) override;
        SizeType    GetCurPos() const override { return m_curPos; }
        SizeType    GetLength() const override;
        const char* GetFilename() const override;
        OpenMode    GetModeFlags() const override { return m_mode; }
        bool        ReOpen() override;

    private:
        SystemFile* m_file;
        SizeType    m_baseOffset;
        SizeType    m_curPos;
        SizeType    m_fakeLen;
        bool        m_isFileOwner;
        OpenMode    m_mode;
    };

    /*
     * Wraps around a memory buffer
     */
    class MemoryStream
        : public GenericStream
    {
    public:
        enum MemoryStreamMode
        {
            MSM_READONLY,
            MSM_READWRITE,
        };

        MemoryStream(const void* buffer, size_t bufferLen)
            : m_buffer(reinterpret_cast<const char*>(buffer))
            , m_bufferLen(bufferLen)
            , m_curOffset(0)
            , m_curLen(bufferLen)
            , m_mode(MSM_READONLY)
        {
        }

        MemoryStream(void* buffer, size_t bufferLen, size_t curLen)
            : m_buffer(reinterpret_cast<char*>(buffer))
            , m_bufferLen(bufferLen)
            , m_curOffset(0)
            , m_curLen(curLen)
            , m_mode(MSM_READWRITE)
        {
        }

        bool        IsOpen() const override { return m_buffer != NULL; }
        bool        CanSeek() const override { return true; }
        bool        CanRead() const override { return true; }
        bool        CanWrite() const override { return true; }
        void        Seek(OffsetType bytes, SeekMode mode) override;
        SizeType    Read(SizeType bytes, void* oBuffer) override;
        SizeType    Write(SizeType bytes, const void* iBuffer) override;
        SizeType    WriteFromStream(SizeType bytes, GenericStream* inputStream) override;
        virtual const void* GetData() const { return m_buffer; }
        SizeType    GetCurPos() const override { return m_curOffset; }
        SizeType    GetLength() const override { return m_curLen; }

    protected:
        SizeType    PrepareForWrite(SizeType bytes);

        const char* m_buffer;
        size_t              m_bufferLen;
        size_t              m_curOffset;
        size_t              m_curLen;
        MemoryStreamMode    m_mode;
    };

    /*
    * Implementation of the GenericStream interface
    * that can write raw data to stdout file descriptor
    * Read operations are not supported
    */
    class StdoutStream
        : public GenericStream
    {
    public:
        ~StdoutStream() override;
        bool IsOpen() const override;
        bool CanSeek() const override;
        bool CanRead() const override;
        bool CanWrite() const override;
        void Seek(AZ::IO::OffsetType, SeekMode) override;
        AZ::IO::SizeType Read(AZ::IO::SizeType, void*) override;
        AZ::IO::SizeType Write(AZ::IO::SizeType bytes, const void* iBuffer) override;
        AZ::IO::SizeType GetCurPos() const override;
        AZ::IO::SizeType GetLength() const override;
        AZ::IO::OpenMode GetModeFlags() const override;
    };
}   // namespace AZ::IO
