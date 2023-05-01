/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/FileIO.h>
#include <ctype.h>
#include <AzCore/Module/Environment.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/string/wildcard.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/PlatformIncl.h>
#include <AzCore/Debug/Profiler.h>

#ifndef SEEK_SET
#  define SEEK_SET        0       /* Seek from beginning of file.  */
#  define SEEK_CUR        1       /* Seek from current position.  */
#  define SEEK_END        2       /* Set file pointer to EOF plus "offset" */
#endif

namespace AZ::IO
{
    static EnvironmentVariable<FileIOBase*> g_fileIOInstance;
    static EnvironmentVariable<FileIOBase*> g_directFileIOInstance;
    static const char* s_EngineFileIOName = "EngineFileIO";
    static const char* s_DirectFileIOName = "DirectFileIO";

    FileIOBase* FileIOBase::GetInstance()
    {
        if (!g_fileIOInstance)
        {
            g_fileIOInstance = Environment::FindVariable<FileIOBase*>(s_EngineFileIOName);
        }

        return g_fileIOInstance ? (*g_fileIOInstance) : nullptr;
    }

    void FileIOBase::SetInstance(FileIOBase* instance)
    {
        if (!g_fileIOInstance)
        {
            g_fileIOInstance = Environment::CreateVariable<FileIOBase*>(s_EngineFileIOName);
            (*g_fileIOInstance) = nullptr;
        }

        // at this point we're guaranteed to have g_fileIOInstance.  Its value might be null.

        if ((instance) && (g_fileIOInstance) && (*g_fileIOInstance))
        {
            AZ_Error("FileIO", false, "FileIOBase::SetInstance was called without first destroying the old instance and setting it to nullptr");
        }

        (*g_fileIOInstance) = instance;
    }

    FileIOBase* FileIOBase::GetDirectInstance()
    {
        if (!g_directFileIOInstance)
        {
            g_directFileIOInstance = Environment::FindVariable<FileIOBase*>(s_DirectFileIOName);
        }

        // for backwards compatibilty, return the regular instance if this is not attached
        if (!g_directFileIOInstance)
        {
            return GetInstance();
        }

        return g_directFileIOInstance ? (*g_directFileIOInstance) : nullptr;
    }

    void FileIOBase::SetDirectInstance(FileIOBase* instance)
    {
        if (!g_directFileIOInstance)
        {
            g_directFileIOInstance = Environment::CreateVariable<FileIOBase*>(s_DirectFileIOName);
            (*g_directFileIOInstance) = nullptr;
        }

        // at this point we're guaranteed to have g_directFileIOInstance.  Its value might be null.

        if ((instance) && (g_directFileIOInstance) && (*g_directFileIOInstance))
        {
            AZ_Error("FileIO", false, "FileIOBase::SetDirectInstance was called without first destroying the old instance and setting it to nullptr");
        }

        (*g_directFileIOInstance) = instance;
    }

    AZStd::optional<AZ::IO::FixedMaxPath> FileIOBase::ConvertToAlias(const AZ::IO::PathView& path) const
    {
        AZ::IO::FixedMaxPath convertedPath;
        if (ConvertToAlias(convertedPath, path))
        {
            return convertedPath;
        }

        return AZStd::nullopt;
    }

    AZStd::optional<AZ::IO::FixedMaxPath> FileIOBase::ResolvePath(const AZ::IO::PathView& path) const
    {
        AZ::IO::FixedMaxPath resolvedPath;
        if (ResolvePath(resolvedPath, path))
        {
            return resolvedPath;
        }

        return AZStd::nullopt;
    }

    SeekType GetSeekTypeFromFSeekMode(int mode)
    {
        switch (mode)
        {
        case SEEK_SET:
            return SeekType::SeekFromStart;
        case SEEK_CUR:
            return SeekType::SeekFromCurrent;
        case SEEK_END:
            return SeekType::SeekFromEnd;
        }

        // Must have some default, hitting here means some random int mode
        return SeekType::SeekFromStart;
    }

    int GetFSeekModeFromSeekType(SeekType type)
    {
        switch (type)
        {
        case SeekType::SeekFromStart:
            return SEEK_SET;
        case SeekType::SeekFromCurrent:
            return SEEK_CUR;
        case SeekType::SeekFromEnd:
            return SEEK_END;
        }

        return SEEK_SET;
    }

    bool NameMatchesFilter(AZStd::string_view name, AZStd::string_view filter)
    {
        return AZStd::wildcard_match(filter, name);
    }

    FileIOStream::FileIOStream()
        : m_handle(InvalidHandle)
        , m_mode(OpenMode::Invalid)
        , m_ownsHandle(true)
    {

    }

    FileIOStream::FileIOStream(HandleType fileHandle, AZ::IO::OpenMode mode, bool ownsHandle)
        : m_handle(fileHandle)
        , m_mode(mode)
        , m_ownsHandle(ownsHandle)
    {

        FileIOBase* fileIO = FileIOBase::GetInstance();
        AZ_Assert(fileIO, "FileIO is not initialized.");
        AZStd::array<char, MaxPathLength> resolvedPath{ {0} };
        fileIO->GetFilename(m_handle, resolvedPath.data(), resolvedPath.size() - 1);
        m_filename = resolvedPath.data();
    }

    FileIOStream::FileIOStream(const char* path, AZ::IO::OpenMode mode, bool errorOnFailure)
        : m_handle(InvalidHandle)
        , m_mode(mode)
        , m_errorOnFailure(errorOnFailure)
    {
        Open(path, mode);
    }

    FileIOStream::~FileIOStream()
    {
        if (m_ownsHandle)
        {
            Close();
        }
    }

    bool FileIOStream::Open(const char* path, OpenMode mode)
    {
        AZ_Assert(FileIOBase::GetInstance(), "FileIO is not initialized.");
        FileIOBase* fileIO = FileIOBase::GetInstance();

        Close();

        const Result result = fileIO->Open(path, mode, m_handle);
        m_ownsHandle = IsOpen();
        m_mode = mode;

        if (IsOpen())
        {
            // Not using supplied path parameter as it may be unresolved
            AZStd::array<char, MaxPathLength> resolvedPath{ {0} };
            fileIO->GetFilename(m_handle, resolvedPath.data(), resolvedPath.size() - 1);
            m_filename = resolvedPath.data();
        }
        else
        {
            // remember the file name so you can try again with ReOpen
            m_filename = path;
        }

        AZ_PROFILE_INTERVAL_START_COLORED(AzCore, &m_filename, 0xff0000ff, "FileIO: %s", m_filename.c_str());
        return result;
    }

    bool FileIOStream::ReOpen()
    {
        Close();
        return (m_mode != OpenMode::Invalid) ? Open(m_filename.data(), m_mode) : false;
    }

    void FileIOStream::Close()
    {
        if (m_handle != InvalidHandle)
        {
            AZ_Assert(FileIOBase::GetInstance(), "FileIO is not initialized.");

            FileIOBase::GetInstance()->Close(m_handle);
            m_handle = InvalidHandle;
            m_ownsHandle = false;
            AZ_PROFILE_INTERVAL_END(AzCore, &m_filename);
        }
    }

    bool FileIOStream::IsOpen() const
    {
        return (m_handle != InvalidHandle);
    }

    /*!
    \brief Retrieves underlying FileIO Handle from file stream
    \return HandleType
    */
    HandleType FileIOStream::GetHandle() const
    {
        return m_handle;
    }

    /*!
    \brief Retrieves filename
    \return const char*
    */
    const char* FileIOStream::GetFilename() const
    {
        return m_filename.data();
    }

    /*!
    \brief Retrieves OpenMode flags used to open this file
    \return OpenMode
    */
    AZ::IO::OpenMode FileIOStream::GetModeFlags() const
    {
        return m_mode;
    }

    bool FileIOStream::CanSeek() const
    {
        return true;
    }

    bool FileIOStream::CanRead() const
    {
        return (m_mode & (OpenMode::ModeRead | OpenMode::ModeUpdate)) != OpenMode::Invalid;
    }

    bool FileIOStream::CanWrite() const
    {
        return (m_mode & (OpenMode::ModeWrite | OpenMode::ModeAppend | OpenMode::ModeUpdate)) != OpenMode::Invalid;
    }

    void FileIOStream::Seek(OffsetType bytes, SeekMode mode)
    {
        AZ_Assert(FileIOBase::GetInstance(), "FileIO is not initialized.");
        AZ_Assert(IsOpen(), "Cannot seek on a FileIOStream that is not open.");

        SeekType seekType = SeekType::SeekFromCurrent;
        switch (mode)
        {
        case GenericStream::ST_SEEK_BEGIN:
            seekType = SeekType::SeekFromStart;
            break;
        case GenericStream::ST_SEEK_CUR:
            seekType = SeekType::SeekFromCurrent;
            break;
        case GenericStream::ST_SEEK_END:
            seekType = SeekType::SeekFromEnd;
            break;
        default:
            seekType = SeekType::SeekFromCurrent;
            break;
        }

        const Result result = FileIOBase::GetInstance()->Seek(m_handle, static_cast<AZ::s64>(bytes), seekType);
        (void)result;
        AZ_Error("FileIOStream", result.GetResultCode() == ResultCode::Success, "Seek failed.");
    }

    SizeType FileIOStream::Read(SizeType bytes, void* oBuffer)
    {
        AZ_Assert(FileIOBase::GetInstance(), "FileIO is not initialized.");
        AZ_Assert(IsOpen(), "Cannot read from a FileIOStream that is not open.");

        AZ::u64 bytesRead = 0;
        const Result result = FileIOBase::GetInstance()->Read(m_handle, oBuffer, bytes, m_errorOnFailure, &bytesRead);
        (void)result;
        AZ_Error("FileIOStream", result.GetResultCode() == ResultCode::Success, "Read failed in file %s.", m_filename.empty() ? "NULL" : m_filename.c_str());
        return static_cast<SizeType>(bytesRead);
    }

    SizeType FileIOStream::Write(SizeType bytes, const void* iBuffer)
    {
        AZ_Assert(FileIOBase::GetInstance(), "FileIO is not initialized.");
        AZ_Assert(IsOpen(), "Cannot write to a FileIOStream that is not open.");

        AZ::u64 bytesWritten = 0;
        const Result result = FileIOBase::GetInstance()->Write(m_handle, iBuffer, bytes, &bytesWritten);
        (void)result;
        AZ_Error("FileIOStream", result.GetResultCode() == ResultCode::Success, "Write failed.");
        return static_cast<SizeType>(bytesWritten);
    }

    SizeType FileIOStream::GetCurPos() const
    {
        AZ_Assert(FileIOBase::GetInstance(), "FileIO is not initialized.");
        AZ_Assert(IsOpen(), "Cannot use a FileIOStream that is not open.");

        AZ::u64 currentPosition = 0;
        const Result result = FileIOBase::GetInstance()->Tell(m_handle, currentPosition);
        (void)result;
        AZ_Error("FileIOStream", result.GetResultCode() == ResultCode::Success, "GetCurPos failed.");
        return static_cast<SizeType>(currentPosition);
    }

    SizeType FileIOStream::GetLength() const
    {
        AZ_Assert(FileIOBase::GetInstance(), "FileIO is not initialized.");
        AZ_Assert(IsOpen(), "Cannot use a FileIOStream that is not open.");

        SizeType fileLengthBytes = 0;
        if (!FileIOBase::GetInstance()->Size(m_handle, fileLengthBytes))
        {
            AZ_Error("FileIOStream", false, "GetLength failed.");
        }

        return fileLengthBytes;
    }

    void FileIOStream::Flush()
    {
        if (m_handle != InvalidHandle)
        {
            AZ_Assert(FileIOBase::GetInstance(), "FileIO is not initialized.");
            FileIOBase::GetInstance()->Flush(m_handle);
        }
    }

} // namespace AZ::IO
