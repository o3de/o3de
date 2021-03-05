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

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_FILEUTIL_H
#define CRYINCLUDE_CRYCOMMONTOOLS_FILEUTIL_H
#pragma once

#include <AzFramework/IO/LocalFileIO.h>
#include <AzCore/IO/SystemFile.h>

#include <AzCore/PlatformIncl.h>

#if AZ_TRAIT_OS_PLATFORM_APPLE || defined(AZ_PLATFORM_LINUX)
#include <utime.h>
#endif
#if defined(AZ_PLATFORM_LINUX)
#include "Linux64Specific.h"
#endif // defined(AZ_PLATFORM_LINUX)

#include <platform.h>
#include <vector>

namespace FileUtil
{
    //  Magic number explanation:
    //  Both epochs are Gregorian. 1970 - 1601 = 369. Assuming a leap
    //  year every four years, 369 / 4 = 92. However, 1700, 1800, and 1900
    //  were NOT leap years, so 89 leap years, 280 non-leap years.
    //  89 * 366 + 280 * 365 = 134744 days between epochs. Of course
    //  60 * 60 * 24 = 86400 seconds per day, so 134744 * 86400 =
    //  11644473600 = SECS_BETWEEN_EPOCHS.
    //
    //  This result is also confirmed in the MSDN documentation on how
    //  to convert a time_t value to a win32 FILETIME.
    #define SECS_BETWEEN_EPOCHS 11644473600ll
    /* 10^7 */
    #define SECS_TO_100NS 10000000ll

    // Find all files matching filespec.
    bool ScanDirectory(const string& path, const string& filespec, std::vector<string>& files, bool recursive, const string& dirToIgnore);

    // Ensures that directory specified by szPathIn exists by creating all needed (sub-)directories.
    // Returns false in case of a failure.
    // Example: "c:\temp\test" ("c:\temp\test\" also works) - ensures that "c:\temp\test" exists.
    bool EnsureDirectoryExists(const char* szPathIn);

    // converts the FILETIME to the C Timestamp (compatible with dbghelp.dll)
    inline DWORD FiletimeToUnixTime(const FILETIME& ft)
    {
        return (DWORD)((((int64&)ft) / SECS_TO_100NS) - SECS_BETWEEN_EPOCHS);
    }

    // converts the FILETIME to 64bit C timestamp
    inline AZ::u64 FiletimeTo64BitUnixTime(const FILETIME& fileTime)
    {
        const AZ::u64 time = static_cast<AZ::u64>(fileTime.dwHighDateTime) << 32 | fileTime.dwLowDateTime;
        return ((time / SECS_TO_100NS) - SECS_BETWEEN_EPOCHS);
    }
    
    // converts the C Timestamp (compatible with dbghelp.dll) to FILETIME
    inline FILETIME UnixTimeToFiletime(DWORD nCTime)
    {
        const int64 time = (nCTime + SECS_BETWEEN_EPOCHS) * SECS_TO_100NS;
        return (FILETIME&)time;
    }
    
    //converts the 64 bit C Timestamp to FILETIME
    inline void UnixTime64BitToFiletime(AZ::u64 nCTime, FILETIME& fileTime)
    {
        const AZ::u64 time = (nCTime + SECS_BETWEEN_EPOCHS) * SECS_TO_100NS;
        fileTime.dwLowDateTime = static_cast<DWORD>(time);
        fileTime.dwHighDateTime = static_cast<DWORD>(time >> 32);
    }

    inline FILETIME GetInvalidFileTime()
    {
        FILETIME fileTime;
        fileTime.dwLowDateTime = 0;
        fileTime.dwHighDateTime = 0;
        return fileTime;
    }

    // returns file time stamps
#if defined(AZ_PLATFORM_WINDOWS)
    inline bool GetFileTimes(const char* filename, FILETIME* ftimeCreate = nullptr, FILETIME* ftimeAccess = nullptr, FILETIME* ftimeModify = nullptr)
    {
        WIN32_FIND_DATAA FindFileData;
        const HANDLE hFind = FindFirstFileA(filename, &FindFileData);
        if (hFind == INVALID_HANDLE_VALUE)
        {
            return false;
        }

        if (ftimeModify == nullptr && ftimeCreate == nullptr && ftimeAccess == nullptr)
        {
            return true;
        }

        FindClose(hFind);
        if (ftimeCreate)
        {
            ftimeCreate->dwLowDateTime = FindFileData.ftCreationTime.dwLowDateTime;
            ftimeCreate->dwHighDateTime = FindFileData.ftCreationTime.dwHighDateTime;
        }
        if (ftimeModify)
        {
            ftimeModify->dwLowDateTime = FindFileData.ftLastWriteTime.dwLowDateTime;
            ftimeModify->dwHighDateTime = FindFileData.ftLastWriteTime.dwHighDateTime;
        }
        if (ftimeAccess)
        {
            ftimeAccess->dwLowDateTime = FindFileData.ftLastAccessTime.dwLowDateTime;
            ftimeAccess->dwHighDateTime = FindFileData.ftCreationTime.dwHighDateTime;
        }
        return true;
    }
#else
    inline bool GetFileTimes(const char* filename, AZ::u64* timeCreate = nullptr, AZ::u64* timeAccess = nullptr, AZ::u64* timeModify = nullptr)
    {
        
        struct stat statResult;
        if (stat(filename, &statResult) != 0)
        {
            return false;
        }
        
        if (timeCreate)
        {
            *timeCreate =static_cast<AZ::u64>(statResult.st_ctime);
        }
        if (timeModify)
        {
            *timeModify =static_cast<AZ::u64>(statResult.st_mtime);
        }
        if (timeAccess)
        {
            *timeAccess =static_cast<AZ::u64>(statResult.st_atime);
        }
        return true;
    }
#endif
        
    

    inline FILETIME GetLastWriteFileTime(const char* filename)
    {          
        FILETIME timeModify = GetInvalidFileTime();
#if defined(AZ_PLATFORM_WINDOWS)
        GetFileTimes(filename, nullptr, nullptr, &timeModify);
#else
        AZ::u64 modTime = 0;
        GetFileTimes(filename, nullptr, nullptr, &modTime);
        if(modTime != 0)
        {
            UnixTime64BitToFiletime(modTime, timeModify);
        }
#endif
        return timeModify;        
    }

    inline bool FileTimesAreEqual(const FILETIME& fileTime0, const FILETIME& fileTime1)
    {
        return
            (fileTime0.dwLowDateTime == fileTime1.dwLowDateTime) &&
            (fileTime0.dwHighDateTime == fileTime1.dwHighDateTime);
    }

    inline bool FileTimesAreEqual(const char* const srcfilename, const char* const targetfilename)
    {
        FILETIME ftSource = FileUtil::GetLastWriteFileTime(srcfilename);
        FILETIME ftTarget = FileUtil::GetLastWriteFileTime(targetfilename);
        return FileTimesAreEqual(ftSource, ftTarget);
    }

    inline bool FileTimeIsValid(const FILETIME& fileTime)
    {
        return !FileTimesAreEqual(GetInvalidFileTime(), fileTime);
    }

    inline bool SetFileTimes(const char* const filename, const FILETIME& creationFileTime, const FILETIME& accessFileTime, const FILETIME& modifcationFileTime)
    {
#if defined(AZ_PLATFORM_WINDOWS)
        const HANDLE hf = CreateFileA(filename, FILE_WRITE_ATTRIBUTES, FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
        if (hf != INVALID_HANDLE_VALUE)
        {
            if (SetFileTime(hf, &creationFileTime, &accessFileTime, &modifcationFileTime))
            {
                if (CloseHandle(hf))
                {
                    return true;
                }
            }

            CloseHandle(hf);
        }
#else
        AZ::u64 creationTime = FiletimeTo64BitUnixTime(creationFileTime);
        AZ::u64 modificationTime = FiletimeTo64BitUnixTime(modifcationFileTime);
        
        struct utimbuf puttime;
        puttime.modtime = modificationTime;
        puttime.actime = creationTime;
        
        if (utime(filename, &puttime) == 0)
        {
            return true;
        }
        
#endif
        return false;
    }

    inline bool SetFileTimes(const char* const filename, const FILETIME& fileTime)
    {
#if defined(AZ_PLATFORM_WINDOWS)
        const HANDLE hf = CreateFileA(filename, FILE_WRITE_ATTRIBUTES, FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
        if (hf != INVALID_HANDLE_VALUE)
        {
            if (SetFileTime(hf, &fileTime, &fileTime, &fileTime))
            {
                if (CloseHandle(hf))
                {
                    return true;
                }
            }

            CloseHandle(hf);
        }
#else

        AZ::u64 newTime = FiletimeTo64BitUnixTime(fileTime);
        
        struct utimbuf puttime;
        puttime.modtime = newTime;
        puttime.actime = newTime;
        
        if (utime(filename, &puttime) == 0)
        {
            return true;
        }
#endif
        return false;
    }

    inline bool SetFileTimes(const char* const srcfilename, const char* const targetfilename)
    {
#if defined(AZ_PLATFORM_WINDOWS)
        FILETIME creationFileTime, accessFileTime, modifcationFileTime;
        if (GetFileTimes(srcfilename, &creationFileTime, &accessFileTime, &modifcationFileTime))
        {
            const HANDLE hf = CreateFileA(targetfilename, FILE_WRITE_ATTRIBUTES, FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
            if (hf != INVALID_HANDLE_VALUE)
            {
                if (SetFileTime(hf, &creationFileTime, &accessFileTime, &modifcationFileTime))
                {
                    if (CloseHandle(hf))
                    {
                        return true;
                    }
                }

                CloseHandle(hf);
            }
        }
#else
        AZ::u64 creationFileTime, accessFileTime, modifcationFileTime;
        if (GetFileTimes(srcfilename, &creationFileTime, &accessFileTime, &modifcationFileTime))
        {
            struct utimbuf puttime;
            puttime.modtime = modifcationFileTime;
            puttime.actime = accessFileTime;

            if (utime(targetfilename, &puttime) == 0)
            {
                return true;
            }
        }
#endif
        return false;
    }

    inline uint64 GetFileSize(const char* const filename)
    {
        AZ::u64 fileSize = AZ::IO::SystemFile::Length(filename);
        return fileSize >= 0? fileSize : -1;

    }

    inline bool FileExists(const char* szPath)
    {     
        return  AZ::IO::LocalFileIO().Exists(szPath);
    }

    inline bool DirectoryExists(const char* szPath)
    {        
        return  AZ::IO::LocalFileIO().IsDirectory(szPath);
    }
};

#endif // CRYINCLUDE_CRYCOMMONTOOLS_FILEUTIL_H
