/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/FileIOEventBus.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Casting/numeric_cast.h>

#include <AzCore/PlatformIncl.h>
#include <AzCore/std/string/wildcard.h>

#include <AzCore/Utils/Utils.h>

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include <dirent.h>
#include <AzCore/Android/APKFileHandler.h>
#include <AzCore/Android/Utils.h>

namespace AZ::IO
{

namespace UnixLikePlatformUtil
{
    bool CanCreateDirRecursive(char* dirPath)
    {
        if (AZ::Android::Utils::IsApkPath(dirPath))
        {
            return false;
        }
        return true;
    }
}

namespace
{
    static const SystemFile::FileHandleType PlatformSpecificInvalidHandle = AZ_TRAIT_SYSTEMFILE_INVALID_HANDLE;
}

bool SystemFile::PlatformOpen(int mode, int platformFlags)
{
    const char* openMode = nullptr;
    if ((mode & SF_OPEN_READ_ONLY) == SF_OPEN_READ_ONLY)
    {
        openMode = "r";
    }
    else if ((mode & SF_OPEN_WRITE_ONLY) == SF_OPEN_WRITE_ONLY)
    {
        if ((mode & SF_OPEN_APPEND) == SF_OPEN_APPEND)
        {
            openMode = "a+";
        }
        else if (mode & (SF_OPEN_TRUNCATE | SF_OPEN_CREATE_NEW | SF_OPEN_CREATE))
        {
            openMode = "w+";
        }
        else
        {
            openMode = "w";
        }
    }
    else if ((mode & SF_OPEN_READ_WRITE) == SF_OPEN_READ_WRITE)
    {
        if ((mode & SF_OPEN_APPEND) == SF_OPEN_APPEND)
        {
            openMode = "a+";
        }
        else if (mode & (SF_OPEN_TRUNCATE | SF_OPEN_CREATE_NEW | SF_OPEN_CREATE))
        {
            openMode = "w+";
        }
        else
        {
            openMode = "r+";
        }
    }
    else
    {
        EBUS_EVENT(FileIOEventBus, OnError, this, nullptr, EINVAL);
        return false;
    }

    bool createPath = false;
    if (mode & (SF_OPEN_CREATE_NEW | SF_OPEN_CREATE))
    {
        createPath = (mode & SF_OPEN_CREATE_PATH) == SF_OPEN_CREATE_PATH;
    }

    bool isApkFile = AZ::Android::Utils::IsApkPath(m_fileName.c_str());

    if (createPath)
    {
        if (isApkFile)
        {
            EBUS_EVENT(FileIOEventBus, OnError, this, nullptr, ENOSPC);
            return false;
        }

        CreatePath(m_fileName.c_str());
    }

    int errorCode = 0;
    if (isApkFile)
    {
        AZ::u64 size = 0;
        m_handle = AZ::Android::APKFileHandler::Open(m_fileName.c_str(), openMode, size);
        errorCode = EACCES; // general error when a file can't be opened from inside the APK
    }
    else
    {
        m_handle = fopen(m_fileName.c_str(), openMode);
        errorCode = errno;
    }

    if (m_handle == PlatformSpecificInvalidHandle)
    {
        EBUS_EVENT(FileIOEventBus, OnError, this, nullptr, errorCode);
        return false;
    }

    return true;
}

void SystemFile::PlatformClose()
{
    if (m_handle != PlatformSpecificInvalidHandle)
    {
        fclose(m_handle);
        m_handle = PlatformSpecificInvalidHandle;
    }
}

namespace Platform::Internal
{
    void FindFilesOnDisk(const char* filter, const SystemFile::FindFileCB& cb)
    {
        // Split the directory of the path filter from the filename portion
        AZ::IO::PathView filterPath(filter);
        AZ::IO::FixedMaxPathString filterDir{ filterPath.ParentPath().Native() };
        AZStd::string_view fileFilter{ filterPath.Filename().Native() };

        DIR* dir = opendir(filterDir.c_str());

        if (dir != nullptr)
        {
            // clear the errno state so we can distinguish between real errors and end of stream
            errno = 0;
            struct dirent* entry = readdir(dir);

            // List all the other files in the directory.
            while (entry != nullptr)
            {
                if (AZStd::wildcard_match(fileFilter, entry->d_name))
                {
                    cb(entry->d_name, (entry->d_type & DT_DIR) == 0);
                }
                entry = readdir(dir);
            }

            int lastError = errno;
            if (lastError != 0)
            {
                EBUS_EVENT(FileIOEventBus, OnError, nullptr, filter, lastError);
            }

            closedir(dir);
        }
        else
        {
            int lastError = errno;
            if (lastError != ENOENT)
            {
                EBUS_EVENT(FileIOEventBus, OnError, nullptr, filter, 0);
            }
        }
    }

    void FindFilesInApk(const char* filter, const SystemFile::FindFileCB& cb)
    {
        // Separate the directory from the filename portion of the filter
        AZ::IO::FixedMaxPath filterPath(AZ::Android::Utils::StripApkPrefix(filter));
        AZ::IO::FixedMaxPathString filterDir{ filterPath.ParentPath().Native() };
        AZStd::string_view fileFilter{ filterPath.Filename().Native() };

        // Construct a wrapper function around the SystemFile::FindFileCB parameter
        // as the APKFileHandler does not pass into the callback a second parameter indicating
        // whether the found file is directory
        auto ParseDirectoryFindFile = [&cb, &fileFilter](const char* filename)
        {
            const bool isFile = !AZ::Android::APKFileHandler::IsDirectory(filename);
            if (AZStd::wildcard_match(fileFilter, filename))
            {
                // Let the user provided callback determine if continuing to iterate over files
                // in the APK is allowed
                return cb(filename, isFile);
            }

            // Return true to keep ParseDirectory iterating over files when the file filter doesn't match
            return true;
        };
        AZ::Android::APKFileHandler::ParseDirectory(filterDir.c_str(), ParseDirectoryFindFile);
    }
}

namespace Platform
{
    using FileHandleType = SystemFile::FileHandleType;

    void FindFiles(const char* filter, SystemFile::FindFileCB cb)
    {
        if (AZ::Android::Utils::IsApkPath(filter))
        {
            Platform::Internal::FindFilesInApk(filter, cb);
        }
        else
        {
            Platform::Internal::FindFilesOnDisk(filter, cb);
        }
    }

    void Seek(FileHandleType handle, const SystemFile* systemFile, SystemFile::SeekSizeType offset, SystemFile::SeekMode mode)
    {
        if (handle != PlatformSpecificInvalidHandle)
        {
            off_t result = fseeko(handle, static_cast<off_t>(offset), mode);
            if (result != 0)
            {
                EBUS_EVENT(FileIOEventBus, OnError, systemFile, nullptr, errno);
            }
        }
    }

    SystemFile::SizeType Tell(FileHandleType handle, const SystemFile* systemFile)
    {
        if (handle != PlatformSpecificInvalidHandle)
        {
            off_t result = ftello(handle);
            if (result == (off_t)-1)
            {
                EBUS_EVENT(FileIOEventBus, OnError, systemFile, nullptr, errno);
                return 0;
            }
            return aznumeric_cast<SizeType>(result);
        }

        return 0;
    }

    bool Eof(FileHandleType handle, const SystemFile*)
    {
        if (handle != PlatformSpecificInvalidHandle)
        {
            return (feof(handle) != 0);
        }

        return false;
    }

    AZ::u64 ModificationTime(FileHandleType handle, const SystemFile* systemFile)
    {
        if (handle != PlatformSpecificInvalidHandle)
        {
            struct stat statResult;
            if (stat(systemFile->Name(), &statResult) != 0)
            {
                return 0;
            }
            return aznumeric_cast<AZ::u64>(statResult.st_mtime);
        }

        return 0;
    }

    SystemFile::SizeType Read(FileHandleType handle, const SystemFile* systemFile, SizeType byteSize, void* buffer)
    {
        if (handle != PlatformSpecificInvalidHandle)
        {
            size_t bytesToRead = static_cast<size_t>(byteSize);
            AZ::Android::APKFileHandler::SetNumBytesToRead(bytesToRead);
            size_t bytesRead = fread(buffer, 1, bytesToRead, handle);

            if (bytesRead != bytesToRead && ferror(handle))
            {
                EBUS_EVENT(FileIOEventBus, OnError, systemFile, nullptr, errno);
                return 0;
            }

            return bytesRead;
        }

        return 0;
    }

    SystemFile::SizeType Write(FileHandleType handle, const SystemFile* systemFile, const void* buffer, SizeType byteSize)
    {
        if (handle != PlatformSpecificInvalidHandle)
        {
            size_t bytesToWrite = static_cast<size_t>(byteSize);
            size_t bytesWritten = fwrite(buffer, 1, bytesToWrite, handle);

            if (bytesWritten != bytesToWrite && ferror(handle))
            {
                EBUS_EVENT(FileIOEventBus, OnError, systemFile, nullptr, errno);
                return 0;
            }

            return bytesWritten;
        }

        return 0;
    }

    void Flush(FileHandleType handle, const SystemFile* systemFile)
    {
        if (handle != PlatformSpecificInvalidHandle)
        {
            if (fflush(handle) != 0)
            {
                EBUS_EVENT(FileIOEventBus, OnError, systemFile, nullptr, errno);
            }
        }
    }

    SystemFile::SizeType Length(FileHandleType handle, const SystemFile* systemFile)
    {
        if (handle != PlatformSpecificInvalidHandle)
        {
            const char* fileName = systemFile->Name();
            if (AZ::Android::Utils::IsApkPath(fileName))
            {
                int fileSize = AZ::Android::APKFileHandler::FileLength(fileName);
                return static_cast<SizeType>(fileSize);
            }
            else
            {
                struct stat fileStat;
                if (stat(fileName, &fileStat) < 0)
                {
                    EBUS_EVENT(FileIOEventBus, OnError, systemFile, nullptr, 0);
                    return 0;
                }
                return static_cast<SizeType>(fileStat.st_size);
            }
        }

        return 0;
    }

    bool Exists(const char* fileName)
    {
        if (AZ::Android::Utils::IsApkPath(fileName))
        {
            return AZ::Android::APKFileHandler::DirectoryOrFileExists(fileName);
        }
        else
        {
            return access(fileName, F_OK) == 0;
        }
    }

    bool IsDirectory(const char* filePath)
    {
        if (AZ::Android::Utils::IsApkPath(filePath))
        {
            return AZ::Android::APKFileHandler::IsDirectory(AZ::Android::Utils::StripApkPrefix(filePath).c_str());
        }

        struct stat result;
        if (stat(filePath, &result) == 0)
        {
            return S_ISDIR(result.st_mode);
        }
        return false;
    }
} // namespace AZ::IO::Platform

} // namespace AZ::IO
