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
#include <AzCore/std/string/conversions.h>

#include <AzCore/PlatformIncl.h>
#include <AzCore/Utils/Utils.h>

#include <stdio.h>

namespace AZ::IO
{
    using FixedMaxPathWString = AZStd::fixed_wstring<MaxPathLength>;
namespace
{
    //=========================================================================
    // GetAttributes
    //  Internal utility to avoid code duplication. Returns result of win32
    //  GetFileAttributes
    //=========================================================================
    DWORD GetAttributes(const char* fileName)
    {
        FixedMaxPathWString fileNameW;
        AZStd::to_wstring(fileNameW, fileName);
        return GetFileAttributesW(fileNameW.c_str());
    }

    //=========================================================================
    // SetAttributes
    //  Internal utility to avoid code duplication. Returns result of win32
    //  SetFileAttributes
    //=========================================================================
    BOOL SetAttributes(const char* fileName, DWORD fileAttributes)
    {
        FixedMaxPathWString fileNameW;
        AZStd::to_wstring(fileNameW, fileName);
        return SetFileAttributesW(fileNameW.c_str(), fileAttributes);
    }

    //=========================================================================
    // CreateDirRecursive
    // [2/3/2013]
    //  Internal utility to create a folder hierarchy recursively without
    //  any additional string copies.
    //  If this function fails (returns false), the error will be available via:
    //   * GetLastError() on Windows-like platforms
    //   * errno on Unix platforms
    //=========================================================================
    bool CreateDirRecursive(AZ::IO::FixedMaxPathWString& dirPath)
    {
        if (CreateDirectoryW(dirPath.c_str(), nullptr))
        {
            return true;    // Created without error
        }
        DWORD error = GetLastError();
        if (error == ERROR_PATH_NOT_FOUND)
        {
            // try to create our parent hierarchy
            if (size_t i = dirPath.find_last_of(LR"(/\)"); i != FixedMaxPathWString::npos)
            {
                wchar_t delimiter = dirPath[i];
                dirPath[i] = 0; // null-terminate at the previous slash
                const bool ret = CreateDirRecursive(dirPath);
                dirPath[i] = delimiter; // restore slash
                if (ret)
                {
                    // now that our parent is created, try to create again
                    return CreateDirectoryW(dirPath.c_str(), nullptr) != 0;
                }
            }
            // if we reach here then there was no parent folder to create, so we failed for other reasons
        }
        else if (error == ERROR_ALREADY_EXISTS)
        {
            DWORD attributes = GetFileAttributesW(dirPath.c_str());
            return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        }
        return false;
    }

    static const SystemFile::FileHandleType PlatformSpecificInvalidHandle = INVALID_HANDLE_VALUE;
}


bool SystemFile::PlatformOpen(int mode, int platformFlags)
{
    DWORD dwDesiredAccess = 0;
    DWORD dwShareMode = FILE_SHARE_READ;
    DWORD dwFlagsAndAttributes = platformFlags;
    DWORD dwCreationDisposition = OPEN_EXISTING;

    bool createPath = false;
    if (mode & SF_OPEN_READ_ONLY)
    {
        dwDesiredAccess |= GENERIC_READ;
        // Always open files for reading with file shared write flag otherwise it may result in a sharing violation if 
        // either some process has already opened the file for writing or if some process will later on try to open the file for writing. 
        dwShareMode |= FILE_SHARE_WRITE;
    }
    if (mode & SF_OPEN_READ_WRITE)
    {
        dwDesiredAccess |= GENERIC_READ;
    }
    if ((mode & SF_OPEN_WRITE_ONLY) || (mode & SF_OPEN_READ_WRITE) || (mode & SF_OPEN_APPEND))
    {
        dwDesiredAccess |= GENERIC_WRITE;
    }

    if ((mode & SF_OPEN_CREATE_NEW))
    {
        dwCreationDisposition = CREATE_NEW;
        createPath = (mode & SF_OPEN_CREATE_PATH) == SF_OPEN_CREATE_PATH;
    }
    else if ((mode & SF_OPEN_CREATE))
    {
        dwCreationDisposition = CREATE_ALWAYS;
        createPath = (mode & SF_OPEN_CREATE_PATH) == SF_OPEN_CREATE_PATH;
    }
    else if ((mode & SF_OPEN_TRUNCATE))
    {
        dwCreationDisposition = TRUNCATE_EXISTING;
    }

    if (createPath)
    {
        CreatePath(m_fileName.c_str());
    }

    AZ::IO::FixedMaxPathWString fileNameW;
    AZStd::to_wstring(fileNameW, m_fileName);
    m_handle = INVALID_HANDLE_VALUE;
    m_handle = CreateFileW(fileNameW.c_str(), dwDesiredAccess, dwShareMode, 0, dwCreationDisposition, dwFlagsAndAttributes, 0);

    if (m_handle == INVALID_HANDLE_VALUE)
    {
        EBUS_EVENT(FileIOEventBus, OnError, this, nullptr, (int)GetLastError());
        return false;
    }
    else
    {
        if (mode & SF_OPEN_APPEND)
        {
            SetFilePointer(m_handle, 0, NULL, FILE_END);
        }
    }

    return true;
}

void SystemFile::PlatformClose()
{
    if (m_handle != PlatformSpecificInvalidHandle)
    {
        if (!CloseHandle(m_handle))
        {
            EBUS_EVENT(FileIOEventBus, OnError, this, nullptr, (int)GetLastError());
        }
        m_handle = INVALID_HANDLE_VALUE;
    }
}

const char* SystemFile::GetNullFilename()
{
    return "NUL";
}

namespace Platform
{
    using FileHandleType = AZ::IO::SystemFile::FileHandleType;

    void Seek(FileHandleType handle, const SystemFile* systemFile, SystemFile::SeekSizeType offset, SystemFile::SeekMode mode)
    {
        if (handle != PlatformSpecificInvalidHandle)
        {
            DWORD dwMoveMethod = mode;
            LARGE_INTEGER distToMove;
            distToMove.QuadPart = offset;

            if (!SetFilePointerEx(handle, distToMove, 0, dwMoveMethod))
            {
                EBUS_EVENT(FileIOEventBus, OnError, systemFile, nullptr, (int)GetLastError());
            }
        }
    }

    SystemFile::SizeType Tell(FileHandleType handle, const SystemFile* systemFile)
    {
        if (handle != PlatformSpecificInvalidHandle)
        {
            LARGE_INTEGER distToMove;
            distToMove.QuadPart = 0;

            LARGE_INTEGER newFilePtr;
            if (!SetFilePointerEx(handle, distToMove, &newFilePtr, FILE_CURRENT))
            {
                EBUS_EVENT(FileIOEventBus, OnError, systemFile, nullptr, (int)GetLastError());
                return 0;
            }

            return aznumeric_cast<SizeType>(newFilePtr.QuadPart);
        }

        return 0;
    }

    bool Eof(FileHandleType handle, const SystemFile* systemFile)
    {
        if (handle != PlatformSpecificInvalidHandle)
        {
            LARGE_INTEGER zero;
            zero.QuadPart = 0;

            LARGE_INTEGER currentFilePtr;
            if (!SetFilePointerEx(handle, zero, &currentFilePtr, FILE_CURRENT))
            {
                EBUS_EVENT(FileIOEventBus, OnError, systemFile, nullptr, (int)GetLastError());
                return false;
            }

            FILE_STANDARD_INFO fileInfo;
            if (!GetFileInformationByHandleEx(handle, FileStandardInfo, &fileInfo, sizeof(fileInfo)))
            {
                EBUS_EVENT(FileIOEventBus, OnError, systemFile, nullptr, (int)GetLastError());
                return false;
            }

            return currentFilePtr.QuadPart == fileInfo.EndOfFile.QuadPart;
        }

        return false;
    }

    AZ::u64 ModificationTime(FileHandleType handle, const SystemFile* systemFile)
    {
        if (handle != PlatformSpecificInvalidHandle)
        {
            FILE_BASIC_INFO fileInfo;
            if (!GetFileInformationByHandleEx(handle, FileBasicInfo, &fileInfo, sizeof(fileInfo)))
            {
                EBUS_EVENT(FileIOEventBus, OnError, systemFile, nullptr, (int)GetLastError());
                return 0;
            }

            AZ::u64 modTime = aznumeric_cast<AZ::u64>(fileInfo.ChangeTime.QuadPart);
            // On exFat filesystems the ChangeTime field will be 0
            // Use the LastWriteTime as a failsafe in that case
            if (modTime == 0)
            {
                modTime = aznumeric_cast<AZ::u64>(fileInfo.LastWriteTime.QuadPart);
            }
            return modTime;
        }

        return 0;
    }

    SystemFile::SizeType Read(FileHandleType handle, const SystemFile* systemFile, SizeType byteSize, void* buffer)
    {
        if (handle != PlatformSpecificInvalidHandle)
        {
            DWORD dwNumBytesRead = 0;
            DWORD nNumberOfBytesToRead = (DWORD)byteSize;
            if (!ReadFile(handle, buffer, nNumberOfBytesToRead, &dwNumBytesRead, 0))
            {
                EBUS_EVENT(FileIOEventBus, OnError, systemFile, nullptr, (int)GetLastError());
                return 0;
            }
            return static_cast<SizeType>(dwNumBytesRead);
        }

        return 0;
    }

    SystemFile::SizeType Write(FileHandleType handle, const SystemFile* systemFile, const void* buffer, SizeType byteSize)
    {
        if (handle != PlatformSpecificInvalidHandle)
        {
            DWORD dwNumBytesWritten = 0;
            DWORD nNumberOfBytesToWrite = (DWORD)byteSize;
            if (!WriteFile(handle, buffer, nNumberOfBytesToWrite, &dwNumBytesWritten, 0))
            {
                EBUS_EVENT(FileIOEventBus, OnError, systemFile, nullptr, (int)GetLastError());
                return 0;
            }
            return static_cast<SizeType>(dwNumBytesWritten);
        }

        return 0;
    }

    void Flush(FileHandleType handle, const SystemFile* systemFile)
    {
        if (handle != PlatformSpecificInvalidHandle)
        {
            if (!FlushFileBuffers(handle))
            {
                EBUS_EVENT(FileIOEventBus, OnError, systemFile, nullptr, (int)GetLastError());
            }
        }
    }

    SystemFile::SizeType Length(FileHandleType handle, const SystemFile* systemFile)
    {
        if (handle != PlatformSpecificInvalidHandle)
        {
            LARGE_INTEGER size;
            if (!GetFileSizeEx(handle, &size))
            {
                EBUS_EVENT(FileIOEventBus, OnError, systemFile, nullptr, (int)GetLastError());
                return 0;
            }

            return static_cast<SizeType>(size.QuadPart);
        }

        return 0;
    }

    bool Exists(const char* fileName)
    {
        return GetAttributes(fileName) != INVALID_FILE_ATTRIBUTES;
    }

    bool IsDirectory(const char* filePath)
    {
        DWORD attributes = GetAttributes(filePath);
        return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }


    void FindFiles(const char* filter, SystemFile::FindFileCB cb)
    {
        WIN32_FIND_DATA fd;
        HANDLE hFile;
        int lastError;

        AZ::IO::FixedMaxPathWString filterW;
        AZStd::to_wstring(filterW, filter);
        hFile = INVALID_HANDLE_VALUE;
        hFile = FindFirstFileW(filterW.c_str(), &fd);

        if (hFile != INVALID_HANDLE_VALUE)
        {
            const char* fileName;

            AZ::IO::FixedMaxPathString fileNameUtf8;
            AZStd::to_string(fileNameUtf8, fd.cFileName);
            fileName = fileNameUtf8.c_str();

            cb(fileName, (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0);

            // List all the other files in the directory.
            while (FindNextFileW(hFile, &fd) != 0)
            {
                AZStd::to_string(fileNameUtf8, fd.cFileName);
                fileName = fileNameUtf8.c_str();

                cb(fileName, (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0);
            }

            lastError = (int)GetLastError();
            FindClose(hFile);
            if (lastError != ERROR_NO_MORE_FILES)
            {
                EBUS_EVENT(FileIOEventBus, OnError, nullptr, fileName, lastError);
            }
        }
        else
        {
            lastError = (int)GetLastError();
            if (lastError != ERROR_FILE_NOT_FOUND)
            {
                EBUS_EVENT(FileIOEventBus, OnError, nullptr, filter, lastError);
            }
        }
    }

    AZ::u64 ModificationTime(const char* fileName)
    {
        HANDLE handle = nullptr;

        AZ::IO::FixedMaxPathWString fileNameW;
        AZStd::to_wstring(fileNameW, fileName);
        handle = CreateFileW(fileNameW.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);

        if (handle == INVALID_HANDLE_VALUE)
        {
            EBUS_EVENT(FileIOEventBus, OnError, nullptr, fileName, (int)GetLastError());
            return 0;
        }

        FILE_BASIC_INFO fileInfo{};
        if (!GetFileInformationByHandleEx(handle, FileBasicInfo, &fileInfo, sizeof(fileInfo)))
        {
            EBUS_EVENT(FileIOEventBus, OnError, nullptr, fileName, (int)GetLastError());
        }

        CloseHandle(handle);

        // On exFat filesystems the ChangeTime field will be 0
        // Use the LastWriteTime as a failsafe in that case
        if (fileInfo.ChangeTime.QuadPart == 0)
        {
            return aznumeric_cast<AZ::u64>(fileInfo.LastWriteTime.QuadPart);
        }

        return aznumeric_cast<AZ::u64>(fileInfo.ChangeTime.QuadPart);
    }

    SystemFile::SizeType Length(const char* fileName)
    {
        SizeType len = 0;
        WIN32_FILE_ATTRIBUTE_DATA data = { 0 };
        BOOL result = FALSE;

        AZ::IO::FixedMaxPathWString fileNameW;
        AZStd::to_wstring(fileNameW, fileName);
        result = GetFileAttributesExW(fileNameW.c_str(), GetFileExInfoStandard, &data);

        if (result)
        {
            // Convert hi/lo parts to a SizeType
            LARGE_INTEGER fileSize;
            fileSize.LowPart = data.nFileSizeLow;
            fileSize.HighPart = data.nFileSizeHigh;
            len = aznumeric_cast<SizeType>(fileSize.QuadPart);
        }
        else
        {
            EBUS_EVENT(FileIOEventBus, OnError, nullptr, fileName, (int)GetLastError());
        }

        return len;
    }

    bool Delete(const char* fileName)
    {
        AZ::IO::FixedMaxPathWString fileNameW;
        AZStd::to_wstring(fileNameW, fileName);
        if (DeleteFileW(fileNameW.c_str()) == 0)
        {
            EBUS_EVENT(FileIOEventBus, OnError, nullptr, fileName, (int)GetLastError());
            return false;
        }

        return true;
    }

    bool Rename(const char* sourceFileName, const char* targetFileName, bool overwrite)
    {
        AZ::IO::FixedMaxPathWString sourceFileNameW;
        AZStd::to_wstring(sourceFileNameW, sourceFileName);
        AZ::IO::FixedMaxPathWString targetFileNameW;
        AZStd::to_wstring(targetFileNameW, targetFileName);
        if (MoveFileExW(sourceFileNameW.c_str(), targetFileNameW.c_str(), overwrite ? MOVEFILE_REPLACE_EXISTING : 0) == 0)
        {
            EBUS_EVENT(FileIOEventBus, OnError, nullptr, sourceFileName, (int)GetLastError());
            return false;
        }

        return true;
    }

    bool IsWritable(const char* sourceFileName)
    {
        auto fileAttr = GetAttributes(sourceFileName);
        return !((fileAttr == INVALID_FILE_ATTRIBUTES) || (fileAttr & FILE_ATTRIBUTE_DIRECTORY) || (fileAttr & FILE_ATTRIBUTE_READONLY));
    }

    bool SetWritable(const char* sourceFileName, bool writable)
    {
        auto fileAttr = GetAttributes(sourceFileName);
        if ((fileAttr == INVALID_FILE_ATTRIBUTES) || (fileAttr & FILE_ATTRIBUTE_DIRECTORY))
        {
            return false;
        }

        if (writable)
        {
            fileAttr = fileAttr & (~FILE_ATTRIBUTE_READONLY);
        }
        else
        {
            fileAttr = fileAttr | (FILE_ATTRIBUTE_READONLY);
        }
        auto success = SetAttributes(sourceFileName, fileAttr);
        return success != FALSE;
    }

    bool CreateDir(const char* dirName)
    {
        if (dirName)
        {
            AZ::IO::FixedMaxPathWString dirNameW;
            AZStd::to_wstring(dirNameW, dirName);
            bool success = CreateDirRecursive(dirNameW);
            if (!success)
            {
                EBUS_EVENT(FileIOEventBus, OnError, nullptr, dirName, (int)GetLastError());
            }
            return success;
        }
        return false;
    }

    bool DeleteDir(const char* dirName)
    {
        if (dirName)
        {
            AZ::IO::FixedMaxPathWString dirNameW;
            AZStd::to_wstring(dirNameW, dirName);
            return RemoveDirectory(dirNameW.c_str()) != 0;
        }

        return false;
    }

}

} // namespace AZ::IO
