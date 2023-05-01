/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <errno.h>
#include <sys/ioctl.h>
#include <AzFramework/Process/ProcessCommunicator.h>

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

    void CommunicatorHandleImpl::Break()
    {
        m_broken = true;
    }

    void CommunicatorHandleImpl::Close()
    {
        close(m_handle);
        m_handle = -1;
    }

    void CommunicatorHandleImpl::SetHandle(int handle)
    {
        m_handle = handle;
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

        return static_cast<AZ::u32>(bytesAvailable);
    }

    AZ::u32 StdInOutCommunication::ReadDataFromHandle(StdProcessCommunicatorHandle& handle, void* readBuffer, AZ::u32 bufferSize)
    {
        if (handle->IsBroken() || (!handle->IsValid() && (errno == EBADF)))
        {
            return 0;
        }

        //Block if buffersize == 0
        if (bufferSize == 0)
        {
            fd_set set;
            FD_ZERO(&set);
            FD_SET(handle->GetHandle(), &set);

            [[maybe_unused]] int numReady = select(handle->GetHandle() + 1, &set, NULL, NULL, NULL);

            // if numReady == -1 and errno == EINTR then the child process died unexpectedly and
            // the handle was closed. Not something to assert about in regards to trying to read
            // data from the child as there is not anything useful we can say or do in that case.
            // Normal code/data flow will work and we as the parent will know that the child is
            // dead and return any error codes the child may have written to the error stream.
            AZ_Assert(numReady != -1 || errno == EINTR, "Could not determine if any data is available for reading due to an error. Errno: %d", errno);

            [[maybe_unused]] const bool wasSet = FD_ISSET(handle->GetHandle(), &set);
            AZ_Assert(wasSet, "handle was not set when we selected it for read");

            return 0;
        }

        ssize_t bytesRead = 0;
        bytesRead = read(handle->GetHandle(), readBuffer, bufferSize);
        if (bytesRead < 0)
        {
            AZ_Assert(errno != EIO, "ReadFile performed unexpected async io");
            if (errno == EBADF || errno == EINVAL)
            {
                // Child process exited, we may have read something, so return amount
                handle->Break();
                return static_cast<AZ::u32>(bytesRead);
            }
            AZ_Assert(false, "Unexpected error from ReadFile %d", errno);
            return static_cast<AZ::u32>(bytesRead);
        }

        //EOF
        if (bytesRead == 0)
        {
            handle->Break();
        }
        return static_cast<AZ::u32>(bytesRead);
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
            AZ_Assert(errno != EIO, "Parent performed unexpected async io when trying to write to child process.");
            if (errno == EPIPE)
            {
                // Child process exited, may have written something, so return amount
                handle->Break();
                return static_cast<AZ::u32>(bytesWritten);
            }
            AZ_Assert(false, "Unexpected error trying to write to child process. errno = %d", errno);
            return 0;
        }

        return static_cast<AZ::u32>(bytesWritten);
    }

    bool StdInOutProcessCommunicator::CreatePipesForProcess(ProcessData* processData)
    {
        int pipeFileDescriptors[2] = { 0 };

        // Create a pipe to monitor process std in (output from us)
        int result = pipe(pipeFileDescriptors);
        AZ_Assert(result != -1, "Failed to create pipe for std in pipe: errno = %d", errno);
        if (result == -1)
        {
            return false;
        }

        processData->m_startupInfo.m_inputHandleForChild = pipeFileDescriptors[0];
        m_stdInWrite->SetHandle(pipeFileDescriptors[1]);

        // Create a pipe to monitor process std out (input to us)
        result = pipe(pipeFileDescriptors);
        AZ_Assert(result != -1, "Failed to create pipe for std out pipe: errno = %d", errno);
        if (result == -1)
        {
            processData->m_startupInfo.CloseAllHandles();
            CloseAllHandles();

            return false;
        }

        m_stdOutRead->SetHandle(pipeFileDescriptors[0]);
        processData->m_startupInfo.m_outputHandleForChild = pipeFileDescriptors[1];

        // Create a pipe to monitor process std error (input to us)
        result = pipe(pipeFileDescriptors);
        AZ_Assert(result != -1, "Failed to create pipe for std err pipe: errno = %d", errno);
        if (result == -1)
        {
            processData->m_startupInfo.CloseAllHandles();
            CloseAllHandles();

            return false;
        }

        m_stdErrRead->SetHandle(pipeFileDescriptors[0]);
        processData->m_startupInfo.m_errorHandleForChild = pipeFileDescriptors[1];

        m_initialized = true;
        return true;
    }

    void StdInOutProcessCommunicator::WaitForReadyOutputs(OutputStatus& status)
    {
        status.outputDeviceReady = m_stdOutRead->IsValid() && !m_stdOutRead->IsBroken();
        status.errorsDeviceReady = m_stdErrRead->IsValid() && !m_stdErrRead->IsBroken();
        status.shouldReadOutput = status.shouldReadErrors = false;

        if (status.outputDeviceReady || status.errorsDeviceReady)
        {
            fd_set readSet;
            int maxHandle = 0;

            FD_ZERO(&readSet);

            if (status.outputDeviceReady)
            {
                int currentHandle = m_stdOutRead->GetHandle();
                FD_SET(currentHandle, &readSet);
                maxHandle = currentHandle > maxHandle ? currentHandle : maxHandle;
            }

            if (status.errorsDeviceReady)
            {
                int currentHandle = m_stdErrRead->GetHandle();
                FD_SET(currentHandle, &readSet);
                maxHandle = currentHandle > maxHandle ? currentHandle : maxHandle;
            }

            if (select(maxHandle + 1, &readSet, nullptr, nullptr, nullptr) != -1)
            {
                status.shouldReadOutput = (status.outputDeviceReady && FD_ISSET(m_stdOutRead->GetHandle(), &readSet));
                status.shouldReadErrors = (status.errorsDeviceReady && FD_ISSET(m_stdErrRead->GetHandle(), &readSet));
            }
        }
    }

    bool StdInOutProcessCommunicatorForChildProcess::AttachToExistingPipes()
    {
        m_stdInRead->SetHandle(STDIN_FILENO);
        AZ_Assert(m_stdInRead->IsValid(), "In read handle is invalid");

        m_stdOutWrite->SetHandle(STDOUT_FILENO);
        AZ_Assert(m_stdOutWrite->IsValid(), "Output write handle is invalid");

        m_stdErrWrite->SetHandle(STDERR_FILENO);
        AZ_Assert(m_stdErrWrite->IsValid(), "Error write handle is invalid");

        m_initialized = m_stdInRead->IsValid() && m_stdOutWrite->IsValid() && m_stdErrWrite->IsValid();
        return m_initialized;
    }
} // namespace AzFramework

