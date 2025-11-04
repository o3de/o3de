/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/FileIO.h>

#include <AzCore/PlatformIncl.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/typetraits/typetraits.h>

#include <stdio.h>

namespace AZ::IO::Platform
{
    // Forward declaration of platform specific implementations

    using FileHandleType = SystemFile::FileHandleType;

    void Seek(FileHandleType handle, const SystemFile* systemFile, SystemFile::SeekSizeType offset, SystemFile::SeekMode mode);
    SystemFile::SizeType Tell(FileHandleType handle, const SystemFile* systemFile);
    bool Eof(FileHandleType handle, const SystemFile* systemFile);
    AZ::u64 ModificationTime(FileHandleType handle, const SystemFile* systemFile);
    SystemFile::SizeType Read(FileHandleType handle, const SystemFile* systemFile, SizeType byteSize, void* buffer);
    SystemFile::SizeType Write(FileHandleType handle, const SystemFile* systemFile, const void* buffer, SizeType byteSize);
    void Flush(FileHandleType handle, const SystemFile* systemFile);
    SystemFile::SizeType Length(FileHandleType handle, const SystemFile* systemFile);

    bool Exists(const char* fileName);
    bool IsDirectory(const char* filePath);
    void FindFiles(const char* filter, SystemFile::FindFileCB cb);
    AZ::u64 ModificationTime(const char* fileName);
    SystemFile::SizeType Length(const char* fileName);
    bool Delete(const char* fileName);
    bool Rename(const char* sourceFileName, const char* targetFileName, bool overwrite);
    bool IsWritable(const char* sourceFileName);
    bool SetWritable(const char* sourceFileName, bool writable);
    bool CreateDir(const char* dirName);
    bool DeleteDir(const char* dirName);
}

namespace AZ::IO
{
    void SystemFile::CreatePath(const char* fileName)
    {
        char folderPath[AZ_MAX_PATH_LEN] = { 0 };
        const char* delimiter1 = strrchr(fileName, '\\');
        const char* delimiter2 = strrchr(fileName, '/');
        const char* delimiter = delimiter2 > delimiter1 ? delimiter2 : delimiter1;
        if (delimiter > fileName)
        {
            azstrncpy(folderPath, AZ_ARRAY_SIZE(folderPath), fileName, delimiter - fileName);
            if (!Exists(folderPath))
            {
                CreateDir(folderPath);
            }
        }
    }

    SystemFile::SystemFile()
        : m_handle{ AZ_TRAIT_SYSTEMFILE_INVALID_HANDLE }
    {
    }

    SystemFile::SystemFile(const char* fileName, int mode, int platformFlags)
        : SystemFile()
    {
        Open(fileName, mode, platformFlags);
    }

    SystemFile::~SystemFile()
    {
        if (IsOpen() && m_closeOnDestruction)
        {
            Close();
        }
    }

    SystemFile::SystemFile(SystemFile&& other)
        : SystemFile{}
    {
        AZStd::swap(m_fileName, other.m_fileName);
        AZStd::swap(m_handle, other.m_handle);
        AZStd::swap(m_closeOnDestruction, other.m_closeOnDestruction);
    }

    SystemFile& SystemFile::operator=(SystemFile&& other)
    {
        // Close the current file and take over the SystemFile handle and filename
        Close();
        m_fileName = AZStd::move(other.m_fileName);
        m_handle = AZStd::move(other.m_handle);
        m_closeOnDestruction = other.m_closeOnDestruction;
        other.m_fileName = {};
        other.m_handle = AZ_TRAIT_SYSTEMFILE_INVALID_HANDLE;
        other.m_closeOnDestruction = true;

        return *this;
    }

    bool SystemFile::Open(const char* fileName, int mode, int platformFlags)
    {
        if (fileName)       // If we reopen the file we are allowed to have NULL file name
        {
            if (strlen(fileName) > m_fileName.max_size())
            {
                return false;
            }

            // store the filename
            m_fileName = fileName;
        }

        AZ_Assert(!IsOpen(), "This file (%s) is already open!", m_fileName.c_str());

        // Sets the close on destruction option if
        // the skip close on destruction mode is set
        m_closeOnDestruction = (mode & OpenMode::SF_SKIP_CLOSE_ON_DESTRUCTION) == 0;

        return PlatformOpen(mode, platformFlags);
    }

    bool SystemFile::ReOpen(int mode, int platformFlags)
    {
        AZ_Assert(!m_fileName.empty(), "Missing filename. You must call open first!");
        return Open(nullptr, mode, platformFlags);
    }

    void SystemFile::Close()
    {
        PlatformClose();
    }

    void SystemFile::Seek(SeekSizeType offset, SeekMode mode)
    {
        Platform::Seek(m_handle, this, offset, mode);
    }

    SystemFile::SizeType SystemFile::Tell() const
    {
        return Platform::Tell(m_handle, this);
    }

    bool SystemFile::Eof() const
    {
        return Platform::Eof(m_handle, this);
    }

    AZ::u64 SystemFile::ModificationTime()
    {
        return Platform::ModificationTime(m_handle, this);
    }

    SystemFile::SizeType SystemFile::Read(SizeType byteSize, void* buffer)
    {
        return Platform::Read(m_handle, this, byteSize, buffer);
    }

    SystemFile::SizeType SystemFile::Write(const void* buffer, SizeType byteSize)
    {
        return Platform::Write(m_handle, this, buffer, byteSize);
    }

    void SystemFile::Flush()
    {
        Platform::Flush(m_handle, this);
    }

    SystemFile::SizeType SystemFile::Length() const
    {
        return Platform::Length(m_handle, this);
    }

    bool SystemFile::IsOpen() const
    {
        return m_handle != AZ_TRAIT_SYSTEMFILE_INVALID_HANDLE;
    }

    SystemFile::SizeType SystemFile::DiskOffset() const
    {
#if AZ_TRAIT_DOES_NOT_SUPPORT_FILE_DISK_OFFSET
#pragma message("--- File Disk Offset is not available ---")
#endif
        return 0;
    }

    bool SystemFile::Exists(const char* fileName)
    {
        return Platform::Exists(fileName);
    }

    bool SystemFile::IsDirectory(const char* filePath)
    {
        return Platform::IsDirectory(filePath);
    }

    void SystemFile::FindFiles(const char* filter, FindFileCB cb)
    {
        Platform::FindFiles(filter, cb);
    }

    AZ::u64 SystemFile::ModificationTime(const char* fileName)
    {
        return Platform::ModificationTime(fileName);
    }

    SystemFile::SizeType SystemFile::Length(const char* fileName)
    {
        return Platform::Length(fileName);
    }

    SystemFile::SizeType SystemFile::Read(const char* fileName, void* buffer, SizeType byteSize, SizeType byteOffset)
    {
        SizeType numBytesRead = 0;
        SystemFile f;
        if (f.Open(fileName, SF_OPEN_READ_ONLY))
        {
            if (byteSize == 0)
            {
                byteSize = f.Length(); // read the entire file
            }
            if (byteOffset != 0)
            {
                f.Seek(byteOffset, SF_SEEK_BEGIN);
            }

            numBytesRead = f.Read(byteSize, buffer);
            f.Close();
        }

        return numBytesRead;
    }

    bool SystemFile::Delete(const char* fileName)
    {
        if (!Exists(fileName))
        {
            return false;
        }

        return Platform::Delete(fileName);
    }

    bool SystemFile::Rename(const char* sourceFileName, const char* targetFileName, bool overwrite)
    {
        if (!Exists(sourceFileName))
        {
            return false;
        }

        return Platform::Rename(sourceFileName, targetFileName, overwrite);
    }

    bool SystemFile::IsWritable(const char* sourceFileName)
    {
        return Platform::IsWritable(sourceFileName);
    }

    bool SystemFile::SetWritable(const char* sourceFileName, bool writable)
    {
        return Platform::SetWritable(sourceFileName, writable);
    }

    bool SystemFile::CreateDir(const char* dirName)
    {
        return Platform::CreateDir(dirName);
    }

    bool SystemFile::DeleteDir(const char* dirName)
    {
        return Platform::DeleteDir(dirName);
    }


    namespace
    {
#define HasPosixEnumOption(EnumValue) \
    static_assert(std::is_enum_v<decltype(EnumValue)>, "PosixInternal enum option " #EnumValue " not defined for platform " AZ_TRAIT_OS_PLATFORM_NAME);

        using namespace AZ::IO::PosixInternal;
        // Verify all posix internal flags have been defined
        // If you happen to add any OpenFlag or PermissionModeFlag, please update this list

        HasPosixEnumOption(OpenFlags::Append);
        HasPosixEnumOption(OpenFlags::Create);
        HasPosixEnumOption(OpenFlags::Temporary);
        HasPosixEnumOption(OpenFlags::Exclusive);
        HasPosixEnumOption(OpenFlags::Truncate);
        HasPosixEnumOption(OpenFlags::ReadOnly);
        HasPosixEnumOption(OpenFlags::WriteOnly);
        HasPosixEnumOption(OpenFlags::ReadWrite);
        HasPosixEnumOption(OpenFlags::NoInherit);
        HasPosixEnumOption(OpenFlags::Random);
        HasPosixEnumOption(OpenFlags::Sequential);
        HasPosixEnumOption(OpenFlags::Binary);
        HasPosixEnumOption(OpenFlags::Text);
        HasPosixEnumOption(OpenFlags::U16Text);
        HasPosixEnumOption(OpenFlags::U8Text);
        HasPosixEnumOption(OpenFlags::WText);
        HasPosixEnumOption(OpenFlags::Async);
        HasPosixEnumOption(OpenFlags::Direct);
        HasPosixEnumOption(OpenFlags::Directory);
        HasPosixEnumOption(OpenFlags::NoFollow);
        HasPosixEnumOption(OpenFlags::NoAccessTime);
        HasPosixEnumOption(OpenFlags::Path);

        HasPosixEnumOption(PermissionModeFlags::None);
        HasPosixEnumOption(PermissionModeFlags::Read);
        HasPosixEnumOption(PermissionModeFlags::Write);

#undef HasPosixEnumOption
    }


    FileDescriptorRedirector::FileDescriptorRedirector(int sourceFileDescriptor)
        : m_sourceFileDescriptor(sourceFileDescriptor)
    {
    }

    FileDescriptorRedirector::~FileDescriptorRedirector()
    {
        Reset();
    }

    void FileDescriptorRedirector::RedirectTo(AZStd::string_view toFileName, Mode mode)
    {
        if (m_sourceFileDescriptor == -1)
        {
            return;
        }

        using OpenFlags = PosixInternal::OpenFlags;
        using PermissionModeFlags = PosixInternal::PermissionModeFlags;

        AZ::IO::FixedMaxPathString redirectPath = AZ::IO::FixedMaxPathString(toFileName.data(), toFileName.size());
        const OpenFlags createOrAppend = mode == Mode::Create ? (OpenFlags::Create | OpenFlags::Truncate) : OpenFlags::Append;

        int toFileDescriptor = PosixInternal::Open(redirectPath.c_str(),
            createOrAppend | OpenFlags::WriteOnly,
            PermissionModeFlags::Read | PermissionModeFlags::Write);

        if (toFileDescriptor == -1)
        {
            return;
        }

        m_redirectionFileDescriptor = toFileDescriptor;
        // Duplicate the source file descriptor and redirect the opened file descriptor
        // to the source file descriptor
        m_dupSourceFileDescriptor = PosixInternal::Dup(m_sourceFileDescriptor);
        if (PosixInternal::Dup2(m_redirectionFileDescriptor, m_sourceFileDescriptor) == -1)
        {
            // Failed to redirect the newly opened file descriptor to the source file descriptor
            PosixInternal::Close(m_redirectionFileDescriptor);
            PosixInternal::Close(m_dupSourceFileDescriptor);
            m_redirectionFileDescriptor = -1;
            m_dupSourceFileDescriptor = -1;
            return;
        }
    }


    void FileDescriptorRedirector::Reset()
    {
        if (m_redirectionFileDescriptor != -1)
        {
            // Close the redirect file
            PosixInternal::Close(m_redirectionFileDescriptor);
            // Restore the redirected file descriptor back to the original
            PosixInternal::Dup2(m_dupSourceFileDescriptor, m_sourceFileDescriptor);
            PosixInternal::Close(m_dupSourceFileDescriptor);
            m_redirectionFileDescriptor = -1;
            m_dupSourceFileDescriptor = -1;
        }
    }

    void FileDescriptorRedirector::WriteBypassingRedirect(const void* data, unsigned int size)
    {
        int targetFileDescriptor = m_sourceFileDescriptor;
        if (m_redirectionFileDescriptor != -1)
        {
            targetFileDescriptor = m_dupSourceFileDescriptor;
        }
        AZ::IO::PosixInternal::Write(targetFileDescriptor, data, size);
    }
} // namespace AZ::IO

namespace AZ::IO
{
    // Captures File Descriptor output through a pipe
    FileDescriptorCapturer::~FileDescriptorCapturer()
    {
        Reset();
    }

    void FileDescriptorCapturer::Stop()
    {
        // Closes the pipe and resets the descriptor
        Reset();
    }

    int FileDescriptorCapturer::WriteBypassingCapture(const void* data, unsigned int size)
    {
        const int writeDescriptor = m_dupSourceDescriptor == -1 ? m_sourceDescriptor : m_dupSourceDescriptor;
        return AZ::IO::PosixInternal::Write(writeDescriptor, data, size);
    }
} // namespace AZ::IO
