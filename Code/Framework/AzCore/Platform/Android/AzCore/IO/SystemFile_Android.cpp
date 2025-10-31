/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Casting/numeric_cast.h>

#include <AzCore/PlatformIncl.h>
#include <AzCore/std/string/wildcard.h>

#include <AzCore/Utils/Utils.h>

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include <dirent.h>
#include <AzCore/Android/APKFileHandler.h>
#include <AzCore/Android/Utils.h>

namespace AZ::IO::UnixLikePlatformUtil
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

namespace AZ::IO
{
    namespace
    {
        static const SystemFile::FileHandleType PlatformSpecificInvalidHandle = AZ_TRAIT_SYSTEMFILE_INVALID_HANDLE;
    }
}

namespace AZ::IO
{
     SystemFile SystemFile::GetStdin()
    {
        SystemFile systemFile;
        systemFile.m_handle = stdin;
        systemFile.m_fileName = "/dev/stdin";
        // The destructor of the SystemFile will not close the stdin handle
        systemFile.m_closeOnDestruction = false;
        return systemFile;
    }

    SystemFile SystemFile::GetStdout()
    {
        SystemFile systemFile;
        systemFile.m_handle = stdout;
        systemFile.m_fileName = "/dev/stdout";
        // The destructor of the SystemFile will not close the stdout handle
        systemFile.m_closeOnDestruction = false;
        return systemFile;
    }
    SystemFile SystemFile::GetStderr()
    {
        SystemFile systemFile;
        systemFile.m_handle = stderr;
        systemFile.m_fileName = "/dev/stderr";
        // The destructor of the SystemFile will not close the stderr handle
        systemFile.m_closeOnDestruction = false;
        return systemFile;
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
                return false;
            }

            CreatePath(m_fileName.c_str());
        }

        if (isApkFile)
        {
            AZ::u64 size = 0;
            m_handle = AZ::Android::APKFileHandler::Open(m_fileName.c_str(), openMode, size);
        }
        else
        {
            m_handle = fopen(m_fileName.c_str(), openMode);
        }

        if (m_handle == PlatformSpecificInvalidHandle)
        {
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
} // namespace AZ::IO

namespace AZ::IO::Platform::Internal
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

            closedir(dir);
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
} // namespace AZ::IO::Platform::Internal

namespace AZ::IO::Platform
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
            fseeko(handle, static_cast<off_t>(offset), mode);
        }
    }

    SystemFile::SizeType Tell(FileHandleType handle, const SystemFile* systemFile)
    {
        if (handle != PlatformSpecificInvalidHandle)
        {
            off_t result = ftello(handle);
            if (result == (off_t)-1)
            {
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
            fflush(handle);
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

namespace AZ::IO::PosixInternal
{
    int Pipe(int(&pipeFileDescriptors)[2], int, OpenFlags pipeFlags)
    {
        return pipe2(pipeFileDescriptors, static_cast<int>(pipeFlags));
    }
} // namespace AZ::IO::PosixInternal

namespace AZ::IO
{
    // Android implementation of FileDescriptor::Start
    // uses epoll for waiting for the read end of the pipe to fill with data.
    // Same as Linux
    void FileDescriptorCapturer::Start(OutputRedirectVisitor redirectCallback,
        AZStd::chrono::milliseconds waitTimeout,
        int pipeSize)
    {
        if (auto expectedState = RedirectState::Idle;
            !m_redirectState.compare_exchange_strong(expectedState, RedirectState::Active))
        {
            // Return as a capture is already in progress
            return;
        }

        if (m_sourceDescriptor == -1)
        {
            // Source file descriptor isn't set.
            return;
        }

        // The pipe is created in an int[2] array
        int redirectPipe[2];
        int pipeCreated = PosixInternal::Pipe(redirectPipe, pipeSize, PosixInternal::OpenFlags::NonBlock);
        // copy created pipe descriptors to intptr_t[2] array
        m_pipe[0] = redirectPipe[0];
        m_pipe[1] = redirectPipe[1];

        if (pipeCreated == -1)
        {
            return;
        }

        // Duplicate the original source descriptor to restore in Stop
        m_dupSourceDescriptor = PosixInternal::Dup(m_sourceDescriptor);

        // Duplicate the write end of the pipe onto the original source descriptor
        // This causes the writes to the source descriptor to redirect to the pipe
        if (PosixInternal::Dup2(static_cast<int>(m_pipe[WriteEnd]), m_sourceDescriptor) == -1)
        {
            // Failed to redirect the source descriptor to the pipe
            PosixInternal::Close(m_dupSourceDescriptor);
            PosixInternal::Close(static_cast<int>(m_pipe[WriteEnd]));
            PosixInternal::Close(static_cast<int>(m_pipe[ReadEnd]));
            // Reset pipe descriptor to -1
            m_pipe[WriteEnd] = -1;
            m_pipe[ReadEnd] = -1;
            m_dupSourceDescriptor = -1;
            return;
        }

        // Create an epoll handle
        if (m_pipeData = epoll_create1(0);
            m_pipeData == -1)
        {
            // Failed to create epoll handle so reset descriptors
            Reset();
            return;
        }

        struct epoll_event event;

        event.events = EPOLLIN;
        event.data.fd = static_cast<int>(m_pipe[ReadEnd]);
        if (int epollCtlResult = epoll_ctl(static_cast<int>(m_pipeData), EPOLL_CTL_ADD, event.data.fd, &event);
            epollCtlResult == -1)
        {
            // Failed to create epoll handle so reset descriptors
            Reset();
            return;
        }

        // Setup flush thread which pumps the read end of the pipe when filled with data
        m_redirectCallback = AZStd::move(redirectCallback);

        auto PumpReadQueue = [this, waitTimeout]
        {
            do
            {
                struct epoll_event event;
                auto timeout = static_cast<int>(AZStd::min<decltype(waitTimeout)::rep>(waitTimeout.count(),
                    AZStd::numeric_limits<int>::max()));

                if (bool waitResult = epoll_wait(static_cast<int>(m_pipeData), &event, 1, timeout);
                    waitResult > 0)
                {
                    Flush();
                }
                // Since this is redirecting file descriptors it isn't safe
                // to log an error using a file descriptor such as stdout or stderr
                // since it may be the descriptor that is being redirected
                // Normally if the return value is -1 errno has been set to an error
            } while (m_redirectState < RedirectState::DisconnectedPipe);
        };
        m_flushThread = AZStd::thread(PumpReadQueue);

    }
} // namespace AZ::IO
