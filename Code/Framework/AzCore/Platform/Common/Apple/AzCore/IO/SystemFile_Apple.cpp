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
#include <sys/event.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>

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
    int Pipe(int(&pipeFileDescriptors)[2], int, OpenFlags readStatusFlags)
    {
        // Apple does not support pipe2
        // Therefore use a combination of pipe + fcntl
        // to set the read end of the pipe status flags
        const int pipeResult = pipe(pipeFileDescriptors);
        if (pipeResult == 0)
        {
            // Query the file status flags from the read end and bit-wise or the input status flags to
            const int fcntlFlags = fcntl(pipeFileDescriptors[0], F_GETFL);
            if (const int fcntlStatusResult = fcntl(pipeFileDescriptors[0], F_SETFL, fcntlFlags | static_cast<int>(readStatusFlags));
                fcntlStatusResult == -1)
            {
                // If the status flags could not be set, close the pipe and return the fcntl status result
                close(pipeFileDescriptors[1]);
                close(pipeFileDescriptors[0]);
                return fcntlStatusResult;
            }
        }
        return pipeResult;
    }
} // namespace AZ::IO::PosixInternal

namespace AZ::IO
{
    // Apple OSes implementation of FileDescriptor::Start
    // uses kqueue for waiting for the read end of the pipe to fill with data
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
        if (m_pipeData = kqueue();
            m_pipeData == -1)
        {
            Reset();
            return;
        }

        // monitored event
        struct kevent event;

        EV_SET(&event, m_pipe[ReadEnd], EVFILT_READ, EV_ADD | EV_CLEAR, 0,
            0, nullptr);

        if (int kEventResult = kevent(static_cast<int>(m_pipeData), &event, 1, nullptr, 0, nullptr);
            kEventResult == -1)
        {
            Reset();
            return;
        }

        // Setup flush thread which pumps the read end of the pipe when filled with data
        m_redirectCallback = AZStd::move(redirectCallback);

        auto PumpReadQueue = [this, waitTimeout]
        {
            do
            {
                struct timespec timeout;
                auto timeoutInSeconds = AZStd::chrono::duration_cast<AZStd::chrono::seconds>(waitTimeout);
                timeout.tv_sec = static_cast<time_t>(timeoutInSeconds.count());
                timeout.tv_nsec = static_cast<long>((
                    AZStd::chrono::duration_cast<AZStd::chrono::nanoseconds>(waitTimeout) - timeoutInSeconds).count());

                // triggered event
                struct kevent triggeredEvent;
                if (int triggeredResult = kevent(static_cast<int>(m_pipeData), nullptr, 0, &triggeredEvent, 1, &timeout);
                    triggeredResult > 0 && (triggeredEvent.fflags & EV_EOF) == 0)
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
