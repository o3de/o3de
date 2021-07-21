/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzFramework/Process/ProcessCommunicator.h>

namespace AzFramework
{
    bool CommunicatorHandleImpl::IsPipe() const
    {
        return m_pipe;
    }

    bool CommunicatorHandleImpl::IsValid() const
    {
        return m_handle != INVALID_HANDLE_VALUE;
    }

    bool CommunicatorHandleImpl::IsBroken() const
    {
        return m_broken;
    }

    const HANDLE& CommunicatorHandleImpl::GetHandle() const
    {
        return m_handle;
    }

    void CommunicatorHandleImpl::Break()
    {
        m_broken = true;
    }

    void CommunicatorHandleImpl::Close()
    {
        CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
    }

    void CommunicatorHandleImpl::SetHandle(const HANDLE& handle, bool isPipe)
    {
        m_handle = handle;
        m_pipe = isPipe;
    }

    AZ::u32 StdInOutCommunication::PeekHandle(StdProcessCommunicatorHandle& handle)
    {
        if (handle->IsBroken() || !handle->IsValid())
        {
            return 0;
        }

        DWORD bytesAvailable = 0;
        BOOL result;
        if (handle->IsPipe())
        {
            result = PeekNamedPipe(handle->GetHandle(), NULL, 0, NULL, &bytesAvailable, NULL);
        }
        else
        {
            result = GetNumberOfConsoleInputEvents(handle->GetHandle(), &bytesAvailable);
        }

        DWORD error = 0;
        if (!result)
        {
            error = GetLastError();
            if (error == ERROR_BROKEN_PIPE)
            {
                // Child process released pipe
                handle->Break();
            }
        }

        AZ_Assert(result || error == ERROR_BROKEN_PIPE, "Peek failed with unexpected error %d", error);
        return bytesAvailable;
    }

    AZ::u32 StdInOutCommunication::ReadDataFromHandle(StdProcessCommunicatorHandle& handle, void* readBuffer, AZ::u32 bufferSize)
    {
        if (handle->IsBroken() || !handle->IsValid())
        {
            return 0;
        }

        DWORD bytesRead = 0;
        BOOL result = ReadFile(handle->GetHandle(), readBuffer, bufferSize, &bytesRead, NULL);
        DWORD error = 0;
        if (!result)
        {
            error = GetLastError();
            AZ_Assert(error != ERROR_IO_PENDING, "ReadFile performed unexpected async io");
            if (error == ERROR_BROKEN_PIPE)
            {
                // Child process exited, we may have read something, so return amount
                handle->Break();
                return bytesRead;
            }
            AZ_Assert(false, "Unexpected error from ReadFile %d", error);
            return 0;
        }

        return bytesRead;
    }

    AZ::u32 StdInOutCommunication::WriteDataToHandle(StdProcessCommunicatorHandle& handle, const void* writeBuffer, AZ::u32 bytesToWrite)
    {
        AZ_Assert(writeBuffer, "Write buffer is null");
        if (!writeBuffer)
        {
            return 0;
        }

        if (handle->IsBroken() || !handle->IsValid())
        {
            return 0;
        }

        DWORD bytesWritten = 0;
        BOOL result = WriteFile(handle->GetHandle(), writeBuffer, bytesToWrite, &bytesWritten, nullptr);
        DWORD error = 0;
        if (!result)
        {
            error = GetLastError();
            AZ_Assert(error != ERROR_IO_PENDING, "WriteFile performed unexpected async io");
            if (error == ERROR_BROKEN_PIPE)
            {
                // Child process exited, may have written something, so return amount
                handle->Break();
                return bytesWritten;
            }
            AZ_Assert(false, "Unexpected error from WriteFile %d", error);
            return 0;
        }

        return bytesWritten;
    }

    bool StdInOutProcessCommunicator::CreatePipesForProcess(ProcessData* processData)
    {
        SECURITY_ATTRIBUTES securityAttributes;

        // Set the bInheritHandle flag so pipe handles are inherited.
        securityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
        securityAttributes.bInheritHandle = TRUE;
        securityAttributes.lpSecurityDescriptor = NULL;

        BOOL Result;

        // Create a pipe to monitor process std out (input to us)
        HANDLE handle = nullptr;
        Result = CreatePipe(&handle, &processData->startupInfo.hStdOutput, &securityAttributes, 0);
        AZ_Assert(Result, "CreatePipe failed for std out pipe");
        if (!Result)
        {
            return false;
        }

        // Ensure the read handle to the pipe for std out is not inherited
        Result = SetHandleInformation(handle, HANDLE_FLAG_INHERIT, 0);
        AZ_Assert(Result, "Unable to disable inheritance on std out read handle");
        if (!Result)
        {
            return false;
        }
        m_stdOutRead->SetHandle(handle, true);

        // Create a pipe to monitor process std in (output from us)
        handle = nullptr;
        Result = CreatePipe(&processData->startupInfo.hStdInput, &handle, &securityAttributes, 0);
        AZ_Assert(Result, "CreatePipe failed for std in pipe");
        if (!Result)
        {
            return false;
        }

        // Ensure the write handle to the pipe for std in is not inherited
        Result = SetHandleInformation(handle, HANDLE_FLAG_INHERIT, 0);
        AZ_Assert(Result, "Unable to disable inheritance on std in write handle");
        if (!Result)
        {
            return false;
        }
        m_stdInWrite->SetHandle(handle, false);

        // Create a pipe to monitor process std error (input to us)
        handle = nullptr;
        Result = CreatePipe(&handle, &processData->startupInfo.hStdError, &securityAttributes, 0);
        AZ_Assert(Result, "CreatePipe failed for std err pipe");
        if (!Result)
        {
            return false;
        }

        // Ensure the read handle to the pipe for std err is not inherited
        Result = SetHandleInformation(handle, HANDLE_FLAG_INHERIT, 0);
        AZ_Assert(Result, "Unable to disable inheritance on std err read handle");
        if (!Result)
        {
            return false;
        }
        m_stdErrRead->SetHandle(handle, true);

        m_initialized = true;
        return true;
    }

    void StdInOutProcessCommunicator::WaitForReadyOutputs(OutputStatus& status) const
    {
        status.outputDeviceReady = m_stdOutRead->IsValid() && !m_stdOutRead->IsBroken();
        status.errorsDeviceReady = m_stdErrRead->IsValid() && !m_stdErrRead->IsBroken();
        status.shouldReadOutput = status.shouldReadErrors = false;

        if (status.outputDeviceReady || status.errorsDeviceReady)
        {
            DWORD waitResult = 0;
            HANDLE waitHandles[2];
            AZ::u32 handleCount = 0;

            if (status.outputDeviceReady)
            {
                waitHandles[handleCount++] = m_stdOutRead->GetHandle();
            }

            if (status.errorsDeviceReady)
            {
                waitHandles[handleCount++] = m_stdErrRead->GetHandle();
            }

            waitResult = WaitForMultipleObjects(handleCount, waitHandles, false, INFINITE);
            switch (waitResult)
            {
            case WAIT_OBJECT_0:
                // If output handle was present, that's the one that signaled, otherwise it was stdError
                status.shouldReadOutput = status.outputDeviceReady;
                status.shouldReadErrors = !status.shouldReadOutput;
                break;
            case WAIT_OBJECT_0 + 1:
                // this can only ever be stdError
                status.shouldReadErrors = true;
                break;
            default:
                break;
            };
        }
    }

    bool StdInOutProcessCommunicatorForChildProcess::AttachToExistingPipes()
    {
        m_stdOutWrite->SetHandle(GetStdHandle(STD_OUTPUT_HANDLE), false);
        AZ_Assert(m_stdOutWrite->IsValid(), "Unable to get valid handle for STD_OUTPUT_HANDLE");

        m_stdErrWrite->SetHandle(GetStdHandle(STD_ERROR_HANDLE), false);
        AZ_Assert(m_stdErrWrite->IsValid(), "Unable to get valid handle for STD_ERROR_HANDLE");

        HANDLE stdInRead = GetStdHandle(STD_INPUT_HANDLE);
        bool isPipe = false;
        if (stdInRead != INVALID_HANDLE_VALUE)
        {
            DWORD dummy;
            isPipe = !GetConsoleMode(stdInRead, &dummy);
        }

        m_stdInRead->SetHandle(stdInRead, isPipe);
        AZ_Assert(m_stdInRead->IsValid(), "Unable to get valid handle for STD_INPUT_HANDLE");

        m_initialized = m_stdOutWrite->IsValid() && m_stdErrWrite->IsValid() && m_stdInRead->IsValid();
        return m_initialized;
    }
} // namespace AzToolsFramework

