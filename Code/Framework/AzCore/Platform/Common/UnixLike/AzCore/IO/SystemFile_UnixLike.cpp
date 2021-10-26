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
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Debug/Profiler.h>

#include <AzCore/PlatformIncl.h>
#include <AzCore/Utils/Utils.h>

#include <stdio.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>

namespace AZ::IO
{

const char* SystemFile::GetNullFilename()
{
    return "/dev/null";
}

namespace UnixLikePlatformUtil
{
    // Platform specific helpers
    bool CanCreateDirRecursive(char* dirPath);
}

namespace
{
    //=========================================================================
    //  Internal utility to create a folder hierarchy recursively without
    //  any additional string copies.
    //  If this function fails (returns false), the error will be available
    //   via errno on Unix platforms
    //=========================================================================
    bool CreateDirRecursive(char* dirPath)
    {
        if (!UnixLikePlatformUtil::CanCreateDirRecursive(dirPath))
        {
            // Our current platform has told us we have failed
            return false;
        }

        int result = mkdir(dirPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if (result == 0)
        {
            return true;    // Created without error
        }
        else if (result == -1)
        {
            // If result == -1, the error is stored in errno
            // http://pubs.opengroup.org/onlinepubs/007908799/xsh/mkdir.html
            result = errno;
        }

        if (result == ENOTDIR || result == ENOENT)
        {
            // try to create our parent hierarchy
            for (size_t i = strlen(dirPath); i > 0; --i)
            {
                if (dirPath[i] == '/' || dirPath[i] == '\\')
                {
                    char delimiter = dirPath[i];
                    dirPath[i] = 0; // null-terminate at the previous slash
                    bool ret = CreateDirRecursive(dirPath);
                    dirPath[i] = delimiter; // restore slash
                    if (ret)
                    {
                        // now that our parent is created, try to create again
                        return mkdir(dirPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0;
                    }
                    return false;
                }
            }
            // if we reach here then there was no parent folder to create, so we failed for other reasons
        }
        else if (result == EEXIST)
        {
            struct stat s;
            if (stat(dirPath, &s) == 0)
            {
                return s.st_mode & S_IFDIR;
            }
        }
        return false;
    }
}

namespace Platform
{
    AZ::u64 ModificationTime(const char* fileName)
    {
        struct stat statResult;
        if (stat(fileName, &statResult) != 0)
        {
            return 0;
        }
        return aznumeric_cast<AZ::u64>(statResult.st_mtime);
    }

    SystemFile::SizeType Length(const char* fileName)
    {
        SizeType len = 0;

        SystemFile f;
        if (f.Open(fileName, SystemFile::SF_OPEN_READ_ONLY))
        {
            len = f.Length();
            f.Close();
        }

        return len;
    }

    bool Delete(const char* fileName)
    {
        int result = remove(fileName);
        if (result != 0)
        {
            EBUS_EVENT(FileIOEventBus, OnError, nullptr, fileName, result);
            return false;
        }

        return true;
    }

    bool Rename(const char* sourceFileName, const char* targetFileName, bool overwrite)
    {
        int result = rename(sourceFileName, targetFileName);
        if (result)
        {
            EBUS_EVENT(FileIOEventBus, OnError, nullptr, sourceFileName, result);
            return false;
        }

        return true;
    }

#if !(AZ_TRAIT_SYSTEMFILE_UNIX_LIKE_PLATFORM_IS_WRITEABLE_DEFINED_ELSEWHERE)
    bool IsWritable(const char* sourceFileName)
    {
        return (access(sourceFileName, W_OK) == 0);
    }
#endif // !(AZ_TRAIT_SYSTEMFILE_UNIX_LIKE_PLATFORM_IS_WRITEABLE_DEFINED_ELSEWHERE)

    bool SetWritable(const char* sourceFileName, bool writable)
    {
        struct stat s;
        if (stat(sourceFileName, &s) == 0)
        {
            int permissions = (s.st_mode & S_IRWXU) | (s.st_mode & S_IRWXO) | (s.st_mode & S_IRWXG);
            if (writable)
            {
                if (s.st_mode & S_IWUSR)
                {
                    // file is already writeable
                    return true;
                }
                return chmod(sourceFileName, permissions | S_IWUSR) == 0;
            }
            else
            {
                if (s.st_mode & S_IWUSR)
                {
                    return chmod(sourceFileName, permissions & ~(S_IWUSR)) == 0;
                }
                else
                {
                    // file is already read-only
                    return true;
                }
            }
        }
        return false;
    }

    bool CreateDir(const char* dirName)
    {
        if (dirName)
        {
            char dirPath[AZ_MAX_PATH_LEN];
            if (strlen(dirName) > AZ_MAX_PATH_LEN)
            {
                return false;
            }
            azstrcpy(dirPath, AZ_MAX_PATH_LEN, dirName);
            bool success = CreateDirRecursive(dirPath);
            if (!success)
            {
                EBUS_EVENT(FileIOEventBus, OnError, nullptr, dirName, errno);
            }
            return success;
        }
        return false;
    }

    bool DeleteDir(const char* dirName)
    {
        AZ_PROFILE_SCOPE(AzCore, "SystemFile::DeleteDir(util) - %s", dirName);

        if (dirName)
        {
            return rmdir(dirName) == 0;
        }
        return false;
    }
} // namespace AZ::IO::Platform

namespace PosixInternal
{
    int Dup(int fileDescriptor)
    {
        return dup(fileDescriptor);
    }

    int Dup2(int fileDescriptorSource, int fileDescriptorDestination)
    {
        return dup2(fileDescriptorSource, fileDescriptorDestination);
    }
} // namespace AZ::IO::PosixInternal

} // namespace AZ::IO
