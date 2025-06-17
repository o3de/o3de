/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/FileIO.h>
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
        // You can use this to issue a warning and print out the BYTES (not characters) of an invalid UTF-8 string
        // in the format [00][01][02]...[FF].
        void WarnInvalidUtf8String([[maybe_unused]] const char* message, [[maybe_unused]] const char* str)
        {
#if defined(AZ_ENABLE_TRACING)
            FixedMaxPathString stringBytes;
            size_t pos = 0;
            while (stringBytes.size() < stringBytes.max_size() - 5 && str[pos] != 0)
            {
                unsigned char currentByte = str[pos];
                stringBytes.append(AZStd::string::format("[%02X]", currentByte).c_str());
                ++pos;
            }
            
            AZ_Warning("SystemFile", false, "%s: Invalid UTF-8 string: %s", message, stringBytes.c_str());
#endif
        }


        //=========================================================================
        // GetAttributes
        //  Internal utility to avoid code duplication. Returns result of win32
        //  GetFileAttributes
        //=========================================================================
        DWORD GetAttributes(const char* fileName)
        {
            FixedMaxPathWString fileNameW;
            if (!AZStd::to_wstring(fileNameW, fileName))
            {
                WarnInvalidUtf8String("Invalid filename given to SystemFile::GetAttributes", fileName);
                return INVALID_FILE_ATTRIBUTES;
            }
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
            if (!AZStd::to_wstring(fileNameW, fileName))
            {
                WarnInvalidUtf8String("Invalid filename given to SystemFile::GetAttributes", fileName);
                return 0;
            }
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
} // namespace AZ::IO

namespace AZ::IO
{
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
        if (!AZStd::to_wstring(fileNameW, m_fileName))
        {
            WarnInvalidUtf8String("Invalid UTF-8 encoded string passed to SystemFile::Open", m_fileName.c_str());
            return false;
        }
        m_handle = INVALID_HANDLE_VALUE;
        m_handle = CreateFileW(fileNameW.c_str(), dwDesiredAccess, dwShareMode, 0, dwCreationDisposition, dwFlagsAndAttributes, 0);

        if (m_handle == INVALID_HANDLE_VALUE)
        {
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
            CloseHandle(m_handle);
            m_handle = INVALID_HANDLE_VALUE;
        }
    }

    const char* SystemFile::GetNullFilename()
    {
        return "NUL";
    }

    SystemFile SystemFile::GetStdin()
    {
        SystemFile systemFile;
        systemFile.m_handle = GetStdHandle(STD_INPUT_HANDLE);
        systemFile.m_fileName = "/dev/stdin";
        // The destructor of the SystemFile will not close the stdin handle
        systemFile.m_closeOnDestruction = false;
        return systemFile;
    }

    SystemFile SystemFile::GetStdout()
    {
        SystemFile systemFile;
        systemFile.m_handle = GetStdHandle(STD_OUTPUT_HANDLE);
        systemFile.m_fileName = "/dev/stdout";
        // The destructor of the SystemFile will not close the stdout handle
        systemFile.m_closeOnDestruction = false;
        return systemFile;
    }
    SystemFile SystemFile::GetStderr()
    {
        SystemFile systemFile;
        systemFile.m_handle = GetStdHandle(STD_ERROR_HANDLE);
        systemFile.m_fileName = "/dev/stderr";
        // The destructor of the SystemFile will not close the stderr handle
        systemFile.m_closeOnDestruction = false;
        return systemFile;
    }
} // namespace AZ::IO

namespace AZ::IO::Platform
{
    using FileHandleType = AZ::IO::SystemFile::FileHandleType;

    void Seek(FileHandleType handle, [[maybe_unused]] const SystemFile* systemFile, SystemFile::SeekSizeType offset, SystemFile::SeekMode mode)
    {
        if (handle != PlatformSpecificInvalidHandle)
        {
            DWORD dwMoveMethod = mode;
            LARGE_INTEGER distToMove;
            distToMove.QuadPart = offset;

            SetFilePointerEx(handle, distToMove, 0, dwMoveMethod);
        }
    }

    SystemFile::SizeType Tell(FileHandleType handle, [[maybe_unused]] const SystemFile* systemFile)
    {
        if (handle != PlatformSpecificInvalidHandle)
        {
            LARGE_INTEGER distToMove;
            distToMove.QuadPart = 0;

            LARGE_INTEGER newFilePtr;
            if (!SetFilePointerEx(handle, distToMove, &newFilePtr, FILE_CURRENT))
            {
                return 0;
            }

            return aznumeric_cast<SizeType>(newFilePtr.QuadPart);
        }

        return 0;
    }

    bool Eof(FileHandleType handle, [[maybe_unused]] const SystemFile* systemFile)
    {
        if (handle != PlatformSpecificInvalidHandle)
        {
            LARGE_INTEGER zero;
            zero.QuadPart = 0;

            LARGE_INTEGER currentFilePtr;
            if (!SetFilePointerEx(handle, zero, &currentFilePtr, FILE_CURRENT))
            {
                return false;
            }

            FILE_STANDARD_INFO fileInfo;
            if (!GetFileInformationByHandleEx(handle, FileStandardInfo, &fileInfo, sizeof(fileInfo)))
            {
                return false;
            }

            return currentFilePtr.QuadPart == fileInfo.EndOfFile.QuadPart;
        }

        return false;
    }

    AZ::u64 ModificationTime(FileHandleType handle, [[maybe_unused]] const SystemFile* systemFile)
    {
        if (handle != PlatformSpecificInvalidHandle)
        {
            FILE_BASIC_INFO fileInfo;
            if (!GetFileInformationByHandleEx(handle, FileBasicInfo, &fileInfo, sizeof(fileInfo)))
            {
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

    SystemFile::SizeType Read(FileHandleType handle, [[maybe_unused]] const SystemFile* systemFile, SizeType byteSize, void* buffer)
    {
        if (handle != PlatformSpecificInvalidHandle)
        {
            DWORD dwNumBytesRead = 0;
            DWORD nNumberOfBytesToRead = (DWORD)byteSize;
            if (!ReadFile(handle, buffer, nNumberOfBytesToRead, &dwNumBytesRead, 0))
            {
                return 0;
            }
            return static_cast<SizeType>(dwNumBytesRead);
        }

        return 0;
    }

    SystemFile::SizeType Write(FileHandleType handle, [[maybe_unused]] const SystemFile* systemFile, const void* buffer, SizeType byteSize)
    {
        if (handle != PlatformSpecificInvalidHandle)
        {
            DWORD dwNumBytesWritten = 0;
            DWORD nNumberOfBytesToWrite = (DWORD)byteSize;
            if (!WriteFile(handle, buffer, nNumberOfBytesToWrite, &dwNumBytesWritten, 0))
            {
                return 0;
            }
            return static_cast<SizeType>(dwNumBytesWritten);
        }

        return 0;
    }

    void Flush(FileHandleType handle, [[maybe_unused]] const SystemFile* systemFile)
    {
        if (handle != PlatformSpecificInvalidHandle)
        {
            FlushFileBuffers(handle);
        }
    }

    SystemFile::SizeType Length(FileHandleType handle, [[maybe_unused]] const SystemFile* systemFile)
    {
        if (handle != PlatformSpecificInvalidHandle)
        {
            LARGE_INTEGER size;
            if (!GetFileSizeEx(handle, &size))
            {
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

        AZ::IO::FixedMaxPathWString filterW;
        if (!AZStd::to_wstring(filterW, filter))
        {
            // we'd print out the string - but its an invalid string and not safe to do so... since its invalid encoding!
            WarnInvalidUtf8String("Invalid UTF-8 encoded string passed to SystemFile::FindFiles", filter);
            return;
        }
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

            FindClose(hFile);
        }
    }

    AZ::u64 ModificationTime(const char* fileName)
    {
        HANDLE handle = nullptr;

        AZ::IO::FixedMaxPathWString fileNameW;
        if (!AZStd::to_wstring(fileNameW, fileName))
        {
            WarnInvalidUtf8String("Invalid UTF-8 encoded string passed to SystemFile::ModificationTime, returning 0", fileName);
            return 0;
        }
        handle = CreateFileW(fileNameW.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);

        if (handle == INVALID_HANDLE_VALUE)
        {
            return 0;
        }

        FILE_BASIC_INFO fileInfo{};
        GetFileInformationByHandleEx(handle, FileBasicInfo, &fileInfo, sizeof(fileInfo));

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
        if (!AZStd::to_wstring(fileNameW, fileName))
        {
            WarnInvalidUtf8String("Invalid UTF-8 encoded string passed to SystemFile::Length, returning 0", fileName);
            return 0;
        }
        result = GetFileAttributesExW(fileNameW.c_str(), GetFileExInfoStandard, &data);

        if (result)
        {
            // Convert hi/lo parts to a SizeType
            LARGE_INTEGER fileSize;
            fileSize.LowPart = data.nFileSizeLow;
            fileSize.HighPart = data.nFileSizeHigh;
            len = aznumeric_cast<SizeType>(fileSize.QuadPart);
        }

        return len;
    }

    bool Delete(const char* fileName)
    {
        AZ::IO::FixedMaxPathWString fileNameW;
        if (!AZStd::to_wstring(fileNameW, fileName))
        {
            WarnInvalidUtf8String("Invalid UTF-8 encoded string passed to SystemFile::Delete", fileName);
            return false;
        }

        if (DeleteFileW(fileNameW.c_str()) == 0)
        {
            return false;
        }

        return true;
    }

    bool Rename(const char* sourceFileName, const char* targetFileName, bool overwrite)
    {
        AZ::IO::FixedMaxPathWString sourceFileNameW;
        if (!AZStd::to_wstring(sourceFileNameW, sourceFileName))
        {
            WarnInvalidUtf8String("Invalid UTF-8 encoded string sourceFileName in SystemFile::Rename", sourceFileName);
            return false;
        }

        AZ::IO::FixedMaxPathWString targetFileNameW;
        if (!AZStd::to_wstring(targetFileNameW, targetFileName))
        {
            WarnInvalidUtf8String("Invalid UTF-8 encoded string targetFileName in SystemFile::Rename", targetFileName);
            return false;
        }

        if (MoveFileExW(sourceFileNameW.c_str(), targetFileNameW.c_str(), overwrite ? MOVEFILE_REPLACE_EXISTING : 0) == 0)
        {
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
            if (!AZStd::to_wstring(dirNameW, dirName))
            {
                WarnInvalidUtf8String("Invalid UTF-8 encoded string passed to SystemFile::CreateDir", dirName);
                return false;
            }
            bool success = CreateDirRecursive(dirNameW);
            return success;
        }
        return false;
    }

    bool DeleteDir(const char* dirName)
    {
        if (dirName)
        {
            AZ::IO::FixedMaxPathWString dirNameW;
            if (!AZStd::to_wstring(dirNameW, dirName))
            {
                WarnInvalidUtf8String("Invalid UTF-8 encoded string passed to SystemFile::DeleteDir", dirName);
                return false;
            }
            return RemoveDirectory(dirNameW.c_str()) != 0;
        }

        return false;
    }
} // namespace AZ::IO

namespace AZ::IO::PosixInternal
{
    int Pipe(int(&pipeFileDescriptors)[2], int pipeSize, OpenFlags pipeFlags)
    {
        return _pipe(pipeFileDescriptors, pipeSize, static_cast<int>(pipeFlags));
    }
}

namespace AZ::IO::Internal
{
    // The only way for a completion routine to pass in any
    // context is through the LPOVERLAPPED structure that gets passed to it
    // So the AsyncPipeData struct contains an OVERLAPPED instance at offset 0
    // and then inside of the completion routine the LPOVERLAPPED structure
    // is cast to an AsyncPipeData*
    // See example of using completion routines for Named Pipes at
    // https://learn.microsoft.com/en-us/windows/win32/ipc/named-pipe-server-using-completion-routines
    struct AsyncPipeData
    {
        AsyncPipeData(FileDescriptorCapturer* descriptorCapturer)
            : m_descriptorCapturer(descriptorCapturer)
        {}

        OVERLAPPED m_asyncIO{};
        FileDescriptorCapturer* m_descriptorCapturer;
        //! Storage buffer for read results of Async ReadFileEx opeastion
        AZStd::array<AZStd::byte, AZ::IO::FileDescriptorCapturer::DefaultPipeSize> m_capturedBytes;
    };

    static_assert(offsetof(AsyncPipeData, m_asyncIO) == 0, "OVERLAPPED IO structure must be at offset 0."
        " Virtual functions cannot be added to the above struct as that affects the layout");
}

namespace AZ::IO
{
    // FileDescriptorCapturer WinAPI Impl

    FileDescriptorCapturer::FileDescriptorCapturer(int sourceDescriptor)
        : m_sourceDescriptor(sourceDescriptor)
        , m_pipeData(0)
    {
    }

    void FileDescriptorCapturer::Start(OutputRedirectVisitor redirectCallback,
        AZStd::chrono::milliseconds waitTimeout,
        int pipeSize)
    {
        // atomically update the redirect state to active if idle
        if (auto expectedState = RedirectState::Idle;
            !m_redirectState.compare_exchange_strong(expectedState, RedirectState::Active))
        {
            // A capturer is already in progress
            return;
        }
        if (m_sourceDescriptor == -1)
        {
            // Source file descriptor isn't set.
            return;
        }

        const auto pipeName = FixedMaxPathWString::format(LR"(\\.\pipe\capturer.%hs)", AZ::Uuid::CreateRandom().ToFixedString().c_str());

        auto& pipeServerEnd{ reinterpret_cast<HANDLE&>(m_pipe[WriteEnd]) };
        if (pipeServerEnd = CreateNamedPipeW(pipeName.c_str(), PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED,
            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE, 1,
            pipeSize, pipeSize, 0, nullptr);
            pipeServerEnd == INVALID_HANDLE_VALUE)
        {
            return;
        }

        // Duplicate the original source descriptor to restore in Stop
        m_dupSourceDescriptor = PosixInternal::Dup(m_sourceDescriptor);

        // Duplicate the write end of the pipe onto the original source descriptor
        // This causes the writes to the source descriptor to redirect to the pipe
        m_pipe[WriteEnd] = _open_osfhandle(m_pipe[WriteEnd], 0);
        if (PosixInternal::Dup2(static_cast<int>(m_pipe[WriteEnd]), m_sourceDescriptor) == -1)
        {
            // Failed to redirect the source descriptor to the pipe
            PosixInternal::Close(m_dupSourceDescriptor);
            PosixInternal::Close(static_cast<int>(m_pipe[WriteEnd]));
            // Reset pipe descriptor to -1
            m_pipe[WriteEnd] = -1;
            m_pipe[ReadEnd] = -1;
            m_dupSourceDescriptor = -1;
            return;
        }

        // Open read end of the pipe
        auto& pipeClientEnd{ reinterpret_cast<HANDLE&>(m_pipe[ReadEnd]) };
        if (pipeClientEnd = CreateFileW(pipeName.c_str(), GENERIC_READ, 0, nullptr,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, nullptr);
            pipeClientEnd == INVALID_HANDLE_VALUE)
        {
            Reset();
            return;
        }

        // Store the OVERLAPPED IO in the m_pipeData
        // It will be used in the Flush(), to provide the async callback
        auto asyncPipeData = new Internal::AsyncPipeData{ this };

        constexpr bool eventNeedsToBeManuallyReset = true;
        constexpr bool initialStateIsSignalled = false;
        asyncPipeData->m_asyncIO.hEvent = CreateEventW(nullptr, eventNeedsToBeManuallyReset,
            initialStateIsSignalled, nullptr);

        // Store the AsyncPipeData instance in the intptr_t opaque data
        m_pipeData = reinterpret_cast<intptr_t>(asyncPipeData);

        // Setup flush thread which pumps the read end of the pipe when filled with data
        m_redirectCallback = AZStd::move(redirectCallback);

        auto ReadFromPipeAsync = [this, waitTimeout]
        {
            do
            {
                if (Flush())
                {
                    if (auto asyncPipeData = reinterpret_cast<Internal::AsyncPipeData*>(m_pipeData);
                        asyncPipeData != nullptr)
                    {
                        DWORD bytesTransferred{};

                        constexpr bool hasAsyncProcedureCall = true;
                        const auto timeoutMs = static_cast<DWORD>(AZStd::min<decltype(waitTimeout)::rep>(waitTimeout.count(), INFINITE));
                        if (bool waitResult = GetOverlappedResultEx(reinterpret_cast<HANDLE>(m_pipe[ReadEnd]),
                            &asyncPipeData->m_asyncIO, &bytesTransferred,
                            timeoutMs, hasAsyncProcedureCall);
                            waitResult)
                        {
                            ResetEvent(asyncPipeData->m_asyncIO.hEvent);
                        }
                    }
                }
            } while (m_redirectState < RedirectState::DisconnectedPipe);
        };
        m_flushThread = AZStd::thread(ReadFromPipeAsync);
    }

    bool FileDescriptorCapturer::Flush()
    {
        LPOVERLAPPED_COMPLETION_ROUTINE ReadCompleted = [](DWORD errorCode, DWORD bytesRead,
            LPOVERLAPPED asyncIO)
        {
            auto asyncPipeData = reinterpret_cast<Internal::AsyncPipeData*>(asyncIO);
            auto& self = *asyncPipeData->m_descriptorCapturer;
            if (self.m_redirectCallback && errorCode == ERROR_SUCCESS && bytesRead > 0)
            {
                self.m_redirectCallback(AZStd::span(asyncPipeData->m_capturedBytes.data(), bytesRead));
            }
        };

        if (auto asyncPipeData = reinterpret_cast<Internal::AsyncPipeData*>(m_pipeData);
            asyncPipeData != nullptr)
        {
            // Use the address of the m_asyncIO OVERLAPPED io member to pass through context data
            // to the async io completion routine
            const bool readResult = ReadFileEx(reinterpret_cast<HANDLE>(m_pipe[ReadEnd]),
                asyncPipeData->m_capturedBytes.data(), static_cast<DWORD>(asyncPipeData->m_capturedBytes.size()),
                &asyncPipeData->m_asyncIO, ReadCompleted);

            return readResult;
        }

        return false;
    }

    // Reset which uses WinAPI CloseHandle to close pipe descriptors
    void FileDescriptorCapturer::Reset()
    {
        if (auto expectedState = RedirectState::Active;
            !m_redirectState.compare_exchange_strong(expectedState, RedirectState::ClosingPipeWriteSide))
        {
            // Since the descriptor capturer is not active return
            return;
        }

        // Close the write end of the pipe first
        if (m_pipe[WriteEnd] != -1)
        {
            // Retrieve OS Handle from file descriptor
            HANDLE writeEndHandle = reinterpret_cast<HANDLE>(_get_osfhandle(static_cast<int>(m_pipe[WriteEnd])));

            // Flush the write end of the pipe to make sure the read end receives any partial data at this point
            FlushFileBuffers(writeEndHandle);

            // Disconnect the pipe. This also signals the read end of the pipe if it is in an alertible wait
            DisconnectNamedPipe(writeEndHandle);

            // Using _close on Windows since the write end of the pipe is represented by a file descriptor
            PosixInternal::Close(static_cast<int>(m_pipe[WriteEnd]));
        }

        // Set the pipe state to disconnected
        m_redirectState = RedirectState::DisconnectedPipe;
        // At this point the the flush thread can now join without a deadlock occuring
        // As the pipe is disconnected at this point

        if (m_flushThread.joinable())
        {
            // Join the flush thread, now that that it is in disconnected pipe state
            m_flushThread.join();
        }

        // Default contruct the thread to clear it out.
        m_flushThread = {};

        // Close the read end of the pipe after the write end is closed
        if (m_pipe[ReadEnd] != -1)
        {
            // Using CloseHandle on Windows since the read end of the pipe is represented by a HANDLE
            CloseHandle(reinterpret_cast<HANDLE>(m_pipe[ReadEnd]));
        }

        // Reset the redirect callback
        m_redirectCallback = {};

        if (auto asyncPipeData = reinterpret_cast<Internal::AsyncPipeData*>(m_pipeData);
            asyncPipeData != nullptr)
        {
            // Deallocate the OVERLAPPED IO structure if it was allocated in the m_waitHandle
            delete asyncPipeData;
            m_pipeData = 0;
        }

        // Reset the pipe descriptor files after the thread has been joined
        // to avoid a race condition where the `m_pipe[ReadEnd]` is an -1 while
        // the flush thread is still running
        m_pipe[WriteEnd] = -1;
        m_pipe[ReadEnd] = -1;

        // Take the duplicate of the original source descriptor and restore it
        // Afterwards close the duplicate descriptor
        if (m_dupSourceDescriptor != -1)
        {
            PosixInternal::Dup2(m_dupSourceDescriptor, m_sourceDescriptor);
            PosixInternal::Close(m_dupSourceDescriptor);
            m_dupSourceDescriptor = -1;
        }

        // Reset the redirect state to Idle
        m_redirectState = RedirectState::Idle;
        // At this point it is now safe to call Start() again on this Capturer
    }
} // namespace AZ::IO

namespace AZ::IO::Posix
{
    int Fileno(FILE* stream)
    {
        return _fileno(stream);
    }
}
