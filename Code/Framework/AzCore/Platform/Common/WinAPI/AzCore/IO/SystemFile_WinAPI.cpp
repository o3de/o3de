/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/FileIOEventBus.h>
#include <AzCore/Casting/numeric_cast.h>

#include <AzCore/PlatformIncl.h>
#include <AzCore/Utils/Utils.h>

#include <stdio.h>

namespace AZ::IO
{

namespace
{
    //=========================================================================
    // GetAttributes
    //  Internal utility to avoid code duplication. Returns result of win32
    //  GetFileAttributes
    //=========================================================================
    DWORD GetAttributes(const char* fileName)
    {
#ifdef _UNICODE
        wchar_t fileNameW[AZ_MAX_PATH_LEN];
        size_t numCharsConverted;
        if (mbstowcs_s(&numCharsConverted, fileNameW, fileName, AZ_ARRAY_SIZE(fileNameW) - 1) == 0)
        {
            return GetFileAttributesW(fileNameW);
        }
        else
        {
            return INVALID_FILE_ATTRIBUTES;
        }
#else //!_UNICODE
        return GetFileAttributes(fileName);
#endif // !_UNICODE
    }

    //=========================================================================
    // SetAttributes
    //  Internal utility to avoid code duplication. Returns result of win32
    //  SetFileAttributes
    //=========================================================================
    BOOL SetAttributes(const char* fileName, DWORD fileAttributes)
    {
#ifdef _UNICODE
        wchar_t fileNameW[AZ_MAX_PATH_LEN];
        size_t numCharsConverted;
        if (mbstowcs_s(&numCharsConverted, fileNameW, fileName, AZ_ARRAY_SIZE(fileNameW) - 1) == 0)
        {
            return SetFileAttributesW(fileNameW, fileAttributes);
        }
        else
        {
            return FALSE;
        }
#else //!_UNICODE
        return SetFileAttributes(fileName, fileAttributes);
#endif // !_UNICODE
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
#if defined(_UNICODE)
    bool CreateDirRecursive(wchar_t* dirPath)
    {
        if (CreateDirectoryW(dirPath, nullptr))
        {
            return true;    // Created without error
        }
        DWORD error = GetLastError();
        if (error == ERROR_PATH_NOT_FOUND)
        {
            // try to create our parent hierarchy
            for (size_t i = wcslen(dirPath); i > 0; --i)
            {
                if (dirPath[i] == L'/' || dirPath[i] == L'\\')
                {
                    wchar_t delimiter = dirPath[i];
                    dirPath[i] = 0; // null-terminate at the previous slash
                    bool ret = CreateDirRecursive(dirPath);
                    dirPath[i] = delimiter; // restore slash
                    if (ret)
                    {
                        // now that our parent is created, try to create again
                        return CreateDirectoryW(dirPath, nullptr) != 0;
                    }
                    return false;
                }
            }
            // if we reach here then there was no parent folder to create, so we failed for other reasons
        }
        else if (error == ERROR_ALREADY_EXISTS)
        {
            DWORD attributes = GetFileAttributesW(dirPath);
            return (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        }
        return false;
    }
#else
    bool CreateDirRecursive(char* dirPath)
    {
        if (CreateDirectory(dirPath, nullptr))
        {
            return true;    // Created without error
        }
        DWORD error = GetLastError();
        if (error == ERROR_PATH_NOT_FOUND)
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
                        if (CreateDirectory(dirPath, nullptr))
                        {
                            return true;
                        }
                        DWORD creationError = GetLastError();
                        if (creationError == ERROR_ALREADY_EXISTS)
                        {
                            DWORD attributes = GetFileAttributes(dirPath);
                            return (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
                        }
                        return false;
                    }
                    return false;
                }
            }
            // if we reach here then there was no parent folder to create, so we failed for other reasons
        }
        else if (error == ERROR_ALREADY_EXISTS)
        {
            DWORD attributes = GetFileAttributes(dirPath);
            return (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        }
        return false;
    }
#endif

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

#   ifdef _UNICODE
    wchar_t fileNameW[AZ_MAX_PATH_LEN];
    size_t numCharsConverted;
    m_handle = INVALID_HANDLE_VALUE;
    if (mbstowcs_s(&numCharsConverted, fileNameW, m_fileName.c_str(), AZ_ARRAY_SIZE(fileNameW) - 1) == 0)
    {
        m_handle = CreateFileW(fileNameW, dwDesiredAccess, dwShareMode, 0, dwCreationDisposition, dwFlagsAndAttributes, 0);
    }
#   else //!_UNICODE
    m_handle = CreateFile(m_fileName.c_str(), dwDesiredAccess, dwShareMode, 0, dwCreationDisposition, dwFlagsAndAttributes, 0);
#   endif // !_UNICODE

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


    void FindFiles(const char* filter, SystemFile::FindFileCB cb)
    {
        WIN32_FIND_DATA fd;
        HANDLE hFile;
        int lastError;

#ifdef _UNICODE
        wchar_t filterW[AZ_MAX_PATH_LEN];
        size_t numCharsConverted;
        hFile = INVALID_HANDLE_VALUE;
        if (mbstowcs_s(&numCharsConverted, filterW, filter, AZ_ARRAY_SIZE(filterW) - 1) == 0)
        {
            hFile = FindFirstFile(filterW, &fd);
        }
#else // !_UNICODE
        hFile = FindFirstFile(filter, &fd);
#endif // !_UNICODE

        if (hFile != INVALID_HANDLE_VALUE)
        {
            const char* fileName;

#ifdef _UNICODE
            char fileNameA[AZ_MAX_PATH_LEN];
            fileName = NULL;
            if (wcstombs_s(&numCharsConverted, fileNameA, fd.cFileName, AZ_ARRAY_SIZE(fileNameA) - 1) == 0)
            {
                fileName = fileNameA;
            }
#else // !_UNICODE
            fileName = fd.cFileName;
#endif // !_UNICODE

            cb(fileName, (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0);

            // List all the other files in the directory.
            while (FindNextFile(hFile, &fd) != 0)
            {
#ifdef _UNICODE
                fileName = NULL;
                if (wcstombs_s(&numCharsConverted, fileNameA, fd.cFileName, AZ_ARRAY_SIZE(fileNameA) - 1) == 0)
                {
                    fileName = fileNameA;
                }
#else // !_UNICODE
                fileName = fd.cFileName;
#endif // !_UNICODE

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

#ifdef _UNICODE
        wchar_t fileNameW[AZ_MAX_PATH_LEN];
        size_t numCharsConverted;
        if (mbstowcs_s(&numCharsConverted, fileNameW, fileName, AZ_ARRAY_SIZE(fileNameW) - 1) == 0)
        {
            handle = CreateFileW(fileNameW, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        }
#else // !_UNICODE
        handle = CreateFileA(fileName, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
#endif // !_UNICODE

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

#ifdef _UNICODE
        wchar_t fileNameW[AZ_MAX_PATH_LEN];
        size_t numCharsConverted;
        if (mbstowcs_s(&numCharsConverted, fileNameW, fileName, AZ_ARRAY_SIZE(fileNameW) - 1) == 0)
        {
            result = GetFileAttributesExW(fileNameW, GetFileExInfoStandard, &data);
        }
#else // !_UNICODE
        result = GetFileAttributesExA(fileName, GetFileExInfoStandard, &data);
#endif // !_UNICODE

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
#ifdef _UNICODE
        wchar_t fileNameW[AZ_MAX_PATH_LEN];
        size_t numCharsConverted;
        if (mbstowcs_s(&numCharsConverted, fileNameW, fileName, AZ_ARRAY_SIZE(fileNameW) - 1) == 0)
        {
            if (DeleteFileW(fileNameW) == 0)
            {
                EBUS_EVENT(FileIOEventBus, OnError, nullptr, fileName, (int)GetLastError());
                return false;
            }
        }
        else
        {
            return false;
        }
#else // !_UNICODE
        if (DeleteFile(fileName) == 0)
        {
            EBUS_EVENT(FileIOEventBus, OnError, nullptr, fileName, (int)GetLastError());
            return false;
        }
#endif // !_UNICODE

        return true;
    }

    bool Rename(const char* sourceFileName, const char* targetFileName, bool overwrite)
    {
#ifdef _UNICODE
        wchar_t sourceFileNameW[AZ_MAX_PATH_LEN];
        wchar_t targetFileNameW[AZ_MAX_PATH_LEN];
        size_t numCharsConverted;
        if (mbstowcs_s(&numCharsConverted, sourceFileNameW, sourceFileName, AZ_ARRAY_SIZE(sourceFileNameW) - 1) == 0 &&
            mbstowcs_s(&numCharsConverted, targetFileNameW, targetFileName, AZ_ARRAY_SIZE(targetFileNameW) - 1) == 0)
        {
            if (MoveFileExW(sourceFileNameW, targetFileNameW, overwrite ? MOVEFILE_REPLACE_EXISTING : 0) == 0)
            {
                EBUS_EVENT(FileIOEventBus, OnError, nullptr, sourceFileName, (int)GetLastError());
                return false;
            }
        }
        else
        {
            return false;
        }
#else // !_UNICODE
        if (MoveFileEx(sourceFileName, targetFileName, overwrite ? MOVEFILE_REPLACE_EXISTING : 0) == 0)
        {
            EBUS_EVENT(FileIOEventBus, OnError, nullptr, sourceFileName, (int)GetLastError());
            return false;
        }
#endif // !_UNICODE

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
#if defined(_UNICODE)
            wchar_t dirPath[AZ_MAX_PATH_LEN];
            size_t numCharsConverted;
            if (mbstowcs_s(&numCharsConverted, dirPath, dirName, AZ_ARRAY_SIZE(dirPath) - 1) == 0)
            {
                bool success = CreateDirRecursive(dirPath);
                if (!success)
                {
                    EBUS_EVENT(FileIOEventBus, OnError, nullptr, dirName, (int)GetLastError());
                }
                return success;
            }
#else
            char dirPath[AZ_MAX_PATH_LEN];
            if (azstrcpy(dirPath, AZ_ARRAY_SIZE(dirPath), dirName) == 0)
            {
                bool success = CreateDirRecursive(dirPath);
                if (!success)
                {
                    EBUS_EVENT(FileIOEventBus, OnError, nullptr, dirName, (int)GetLastError());
                }
                return success;
            }
#endif
        }
        return false;
    }

    bool DeleteDir(const char* dirName)
    {
        if (dirName)
        {
#if defined(_UNICODE)
            wchar_t dirNameW[AZ_MAX_PATH_LEN];
            size_t numCharsConverted;
            if (mbstowcs_s(&numCharsConverted, dirNameW, dirName, AZ_ARRAY_SIZE(dirNameW) - 1) == 0)
            {
                return RemoveDirectory(dirNameW) != 0;
            }
#else
            return RemoveDirectory(dirName) != 0;
#endif
        }

        return false;
    }

}

} // namespace AZ::IO
