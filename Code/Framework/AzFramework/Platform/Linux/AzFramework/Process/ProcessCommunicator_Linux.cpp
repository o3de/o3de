/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/epoll.h>

#include <AzFramework/Process/ProcessCommunicator.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace ProcessCommunicatorLinuxInternal
{

    class CommunicatorDataImpl : public AzFramework::StdInOutProcessCommunicatorData
    {
    public:
        AZ_CLASS_ALLOCATOR(CommunicatorDataImpl, AZ::SystemAllocator);
        CommunicatorDataImpl() = default;
        virtual ~CommunicatorDataImpl()
        {
            if (m_stdInAndOutPollHandle != -1)
            {
                close(m_stdInAndOutPollHandle);
            }
        }
        int m_stdInAndOutPollHandle = -1;
        
        AZ_DISABLE_COPY_MOVE(CommunicatorDataImpl);
    };

}

namespace AzFramework
{
    bool CommunicatorHandleImpl::IsValid() const
    {
        return fcntl(m_handle, F_GETFD) != -1;
    }

    bool CommunicatorHandleImpl::IsBroken() const
    {
        return m_broken;
    }

    int CommunicatorHandleImpl::GetHandle() const
    {
        return m_handle;
    }

    int CommunicatorHandleImpl::GetEPollHandle() const
    {
        return m_epollHandle;
    }

    void CommunicatorHandleImpl::Break()
    {
        m_broken = true;
    }

    void CommunicatorHandleImpl::Close()
    {
        if (m_handle != -1)
        {
            close(m_handle);
            m_handle = -1;
        }

        if (m_epollHandle != -1)
        {
            close(m_epollHandle);
            m_epollHandle = -1;
        }
    }

    bool CommunicatorHandleImpl::SetHandle(int handle, bool createEPollHandle)
    {
        m_handle = handle;
        // also create an epoll handle
        if ( (handle != -1) && (createEPollHandle) )
        {
            int m_epollHandle = epoll_create1(0);

            AZ_Assert(m_epollHandle -1, "Failed to create epoll handle. errno: %d", errno);
            if (m_epollHandle == -1) 
            {
                return false;
            }
            struct epoll_event event;

            event.events = EPOLLIN;
            event.data.fd = GetHandle();
            if(epoll_ctl(m_epollHandle, EPOLL_CTL_ADD, event.data.fd, &event))
            {
                AZ_Assert(false, "Unable to add a file descriptor to epoll. errno: %d", errno);
                close(m_epollHandle);
                m_epollHandle = -1;
                return false;
            }
        }
        return true;
    }

    AZ::u32 StdInOutCommunication::PeekHandle(StdProcessCommunicatorHandle& handle)
    {
        if (handle->IsBroken() || (!handle->IsValid() && (errno == EBADF)))
        {
            return 0;
        }

        size_t bytesAvailable = 0;
        const int result = ioctl(handle->GetHandle(), FIONREAD, &bytesAvailable);
        if ((result == -1) && (errno == EBADF))
        {
            // Child process released pipe
            handle->Break();
        }

        return bytesAvailable;
    }

    AZ::u32 StdInOutCommunication::ReadDataFromHandle(StdProcessCommunicatorHandle& handle, void* readBuffer, AZ::u32 bufferSize)
    {
        if (handle->IsBroken() || (!handle->IsValid() && (errno == EBADF)))
        {
            return 0;
        }

        // Block until data is ready if buffersize == 0
        if (bufferSize == 0)
        {
            if (handle->GetEPollHandle() == -1)
            {
                return 0;
            }
            struct epoll_event event;
            epoll_wait(handle->GetEPollHandle(), &event, 1, -1);
            return 0;
        }

        ssize_t bytesRead = 0;
        bytesRead = read(handle->GetHandle(), readBuffer, bufferSize);
        if (bytesRead < 0)
        {
            AZ_Assert(errno != EIO, "ReadFile performed unexpected async io. errno: %d", errno);
            if (errno == EBADF || errno == EINVAL)
            {
                // Child process exited, we may have read something, so return amount
                handle->Break();
                return bytesRead;
            }
            AZ_Assert(false, "Unexpected error from ReadFile %d", errno);
            return bytesRead;
        }

        //EOF
        if (bytesRead == 0)
        {
            handle->Break();
        }
        return bytesRead;
    }

    AZ::u32 StdInOutCommunication::WriteDataToHandle(StdProcessCommunicatorHandle& handle, const void* writeBuffer, AZ::u32 bytesToWrite)
    {
        AZ_Assert(writeBuffer, "Write buffer is null");

        if (!writeBuffer || handle->IsBroken() || (!handle->IsValid() && (errno == EBADF)))
        {
            return 0;
        }

        const ssize_t bytesWritten = write(handle->GetHandle(), writeBuffer, bytesToWrite);
        if (bytesWritten < 0)
        {
            AZ_Assert(errno != EIO, "Parent performed unexpected async io when trying to write to child process (errno=%d EIO).", errno);
            if (errno == EPIPE)
            {
                // Child process exited, may have written something, so return amount
                handle->Break();
                return bytesWritten;
            }
            AZ_Assert(false, "Unexpected error trying to write to child process. errno = %d", errno);
            return 0;
        }

        return bytesWritten;
    }

    bool StdInOutProcessCommunicator::CreatePipesForProcess(ProcessData* processData)
    {
        using namespace ProcessCommunicatorLinuxInternal;
        int pipeFileDescriptors[2] = { 0 };

        // Create a pipe to monitor process std in (output from us)
        int result = pipe(pipeFileDescriptors);
        AZ_Assert(result != -1, "Failed to create pipe for std in pipe: errno = %d", errno);
        if (result == -1)
        {
            return false;
        }

        processData->m_startupInfo.m_inputHandleForChild = pipeFileDescriptors[0];
        // we don't need to create an epoll handle for the child's stdin, since we don't use
        // epoll for it.
        if (!m_stdInWrite->SetHandle(pipeFileDescriptors[1], /*create epoll handle*/false))
        {
            processData->m_startupInfo.CloseAllHandles();
            CloseAllHandles();
            return false;
        }

        // Create a pipe to monitor process std out (input to us)
        result = pipe(pipeFileDescriptors);
        AZ_Assert(result != -1, "Failed to create pipe for std out pipe: errno = %d", errno);
        if (result == -1)
        {
            processData->m_startupInfo.CloseAllHandles();
            CloseAllHandles();
            return false;
        }

        processData->m_startupInfo.m_outputHandleForChild = pipeFileDescriptors[1];
        if (!m_stdOutRead->SetHandle(pipeFileDescriptors[0], /*create epoll handle*/true))
        {
            processData->m_startupInfo.CloseAllHandles();
            CloseAllHandles();
            return false;
        }

        // Create a pipe to monitor process std error (input to us)
        result = pipe(pipeFileDescriptors);
        AZ_Assert(result != -1, "Failed to create pipe for std err pipe: errno = %d", errno);
        if (result == -1)
        {
            processData->m_startupInfo.CloseAllHandles();
            CloseAllHandles();
            return false;
        }

        processData->m_startupInfo.m_errorHandleForChild = pipeFileDescriptors[1];
        if (!m_stdErrRead->SetHandle(pipeFileDescriptors[0], /*create epoll handle*/true))
        {
            processData->m_startupInfo.CloseAllHandles();
            CloseAllHandles();
            return false;
        }

        // create an epoll file descriptor for both inputs (stderr and stdout)
        m_communicatorData.reset(aznew CommunicatorDataImpl());
        CommunicatorDataImpl* platformImpl = static_cast<CommunicatorDataImpl*>(m_communicatorData.get());
        platformImpl->m_stdInAndOutPollHandle = epoll_create1(0);

        AZ_Assert(platformImpl->m_stdInAndOutPollHandle != -1, "Failed to create epoll handle to monitor output process, errno = %d", errno);
        if (platformImpl->m_stdInAndOutPollHandle == -1) 
        {
            processData->m_startupInfo.CloseAllHandles();
            CloseAllHandles();
            return false;
        }
        
        struct epoll_event event;
        event.events = EPOLLIN;
        event.data.fd = m_stdOutRead->GetHandle();
        if(epoll_ctl(platformImpl->m_stdInAndOutPollHandle, EPOLL_CTL_ADD, event.data.fd, &event))
        {
            AZ_Assert(false, "Unable to add a file descriptor for stdOut to epoll, errno = %d", errno);
            processData->m_startupInfo.CloseAllHandles();
            CloseAllHandles();
            return false;
        }

        event.events = EPOLLIN;
        event.data.fd = m_stdErrRead->GetHandle();
        if(epoll_ctl(platformImpl->m_stdInAndOutPollHandle, EPOLL_CTL_ADD, event.data.fd, &event))
        {
            AZ_Assert(false, "Unable to add a file descriptor for stdErr to epoll, errno = %d", errno);
            processData->m_startupInfo.CloseAllHandles();
            CloseAllHandles();
            return false;
        }

        m_initialized = true;
        return true;
    }

    void StdInOutProcessCommunicator::WaitForReadyOutputs(OutputStatus& status)
    {
        using namespace ProcessCommunicatorLinuxInternal;

        status.outputDeviceReady = m_stdOutRead->IsValid() && !m_stdOutRead->IsBroken();
        status.errorsDeviceReady = m_stdErrRead->IsValid() && !m_stdErrRead->IsBroken();
        status.shouldReadOutput = status.shouldReadErrors = false;

        CommunicatorDataImpl* platformImpl = static_cast<CommunicatorDataImpl*>(m_communicatorData.get());
        if (status.outputDeviceReady || status.errorsDeviceReady)
        {
            struct epoll_event events[2];
            if (int numberSignalled = epoll_wait(platformImpl->m_stdInAndOutPollHandle, events, 1, -1) > -1)
            {
                for (int readEventIndex = 0; readEventIndex < numberSignalled; ++ readEventIndex)
                {
                    if (events[readEventIndex].data.fd == m_stdOutRead->GetHandle())
                    {
                        status.shouldReadOutput = true;
                    }
                    else if (events[readEventIndex].data.fd == m_stdErrRead->GetHandle())
                    {
                        status.shouldReadErrors = true;
                    }
                }
            }
        }
    }

    bool StdInOutProcessCommunicatorForChildProcess::AttachToExistingPipes()
    {
        m_initialized = false;

        if (!m_stdInRead->SetHandle(STDIN_FILENO,/*create epoll handle*/false))
        {
            return false;
        }
        AZ_Assert(m_stdInRead->IsValid(), "In read handle is invalid");

        if (!m_stdOutWrite->SetHandle(STDOUT_FILENO, /*create epoll handle*/ true))
        {
            return false;
        }
        AZ_Assert(m_stdOutWrite->IsValid(), "Output write handle is invalid");

        if (!m_stdErrWrite->SetHandle(STDERR_FILENO, /*create epoll handle*/ true))
        {
            return false;
        }
        AZ_Assert(m_stdErrWrite->IsValid(), "Error write handle is invalid");

        m_initialized = m_stdInRead->IsValid() && m_stdOutWrite->IsValid() && m_stdErrWrite->IsValid();
        return m_initialized;
    }
} // namespace AzFramework

