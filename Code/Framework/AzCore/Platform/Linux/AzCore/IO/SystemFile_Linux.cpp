/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/string/wildcard.h>
#include <../Common/UnixLike/AzCore/IO/Internal/SystemFileUtils_UnixLike.h>

#include <errno.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <dirent.h>

namespace AZ::IO::Platform
{
    void FindFiles(const char* filter, SystemFile::FindFileCB cb)
    {
        // Separate the directory from the filename portion of the filter
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
}

namespace AZ::IO::PosixInternal
{
    int Pipe(int(&pipeFileDescriptors)[2], int, OpenFlags pipeFlags)
    {
        return pipe2(pipeFileDescriptors, static_cast<int>(pipeFlags));
    }
} // namespace AZ::IO::PosixInternal

namespace AZ::IO
{
    // Linux implementation of FileDescriptor::Start
    // uses epoll for waiting for the read end of the pipe to fill with data
    // Same implementation as Android
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
