/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Process/ProcessCommunicator.h>

namespace AzFramework
{
    void ProcessOutput::Clear()
    {
        outputResult.clear();
        errorResult.clear();
    }

    bool ProcessOutput::HasOutput() const
    {
        return !outputResult.empty();
    }

    bool ProcessOutput::HasError() const
    {
        return !errorResult.empty();
    }

    AZ::u32 ProcessCommunicator::BlockUntilErrorAvailable(AZStd::string& readBuffer)
    {
        // block until errors can actually be read
        ReadError(readBuffer.data(), 0);
        // at this point errors can be read, return the peek amount
        return PeekError();
    }

    AZ::u32 ProcessCommunicator::BlockUntilOutputAvailable(AZStd::string& readBuffer)
    {
        // block until output can actually be read
        ReadOutput(readBuffer.data(), 0);
        // at this point output can be read, return the peek amount
        return PeekOutput();
    }

    void ProcessCommunicator::ReadIntoProcessOutput(ProcessOutput& processOutput)
    {
        OutputStatus status;

        // read from the process until the handle is no longer valid
        while (true)
        {
            WaitForReadyOutputs(status);
            if (status.shouldReadErrors || status.shouldReadOutput)
            {
                ReadFromOutputs(processOutput, status);
            }

            if (!status.outputDeviceReady && !status.errorsDeviceReady)
            {
                break;
            }
        }
    }

    void ProcessCommunicator::ReadFromOutputs(ProcessOutput& processOutput, OutputStatus& status)
    {
        AZ::u32 bytesRead = 0;
        // Send in the size + 1 to leave room for us to write out the 0 in
        char readBuffer[s_readBufferSize+1];

        if (status.shouldReadOutput)
        {
            bytesRead = ReadOutput(readBuffer, s_readBufferSize);
            readBuffer[bytesRead] = 0;
            processOutput.outputResult.append(readBuffer, bytesRead);
        }

        if (status.shouldReadErrors)
        {
            bytesRead = ReadError(readBuffer, s_readBufferSize);
            readBuffer[bytesRead] = 0;
            processOutput.errorResult.append(readBuffer, bytesRead);
        }

    }

    AZ::u32 ProcessCommunicatorForChildProcess::BlockUntilInputAvailable(AZStd::string& readBuffer)
    {
        ReadInput(readBuffer.data(), 0);
        return PeekInput();
    }

    StdInOutProcessCommunicator::StdInOutProcessCommunicator()
        : m_stdInWrite(new CommunicatorHandleImpl())
        , m_stdOutRead(new CommunicatorHandleImpl())
        , m_stdErrRead(new CommunicatorHandleImpl())
    {
    }

    StdInOutProcessCommunicator::~StdInOutProcessCommunicator()
    {
        CloseAllHandles();
    }

    bool StdInOutProcessCommunicator::IsValid() const
    {
        return m_initialized && (m_stdInWrite->IsValid() || m_stdOutRead->IsValid() || m_stdErrRead->IsValid());
    }

    AZ::u32 StdInOutProcessCommunicator::ReadError(void* readBuffer, AZ::u32 bufferSize)
    {
        AZ_Assert(m_stdErrRead->IsValid(), "Error read handle is invalid, unable to read error stream");
        return ReadDataFromHandle(m_stdErrRead, readBuffer, bufferSize);
    }

    AZ::u32 StdInOutProcessCommunicator::PeekError()
    {
        AZ_Assert(m_stdErrRead->IsValid(), "Error read handle is invalid, unable to read error stream");
        return PeekHandle(m_stdErrRead);
    }

    AZ::u32 StdInOutProcessCommunicator::ReadOutput(void* readBuffer, AZ::u32 bufferSize)
    {
        AZ_Assert(m_stdOutRead->IsValid(), "Output read handle is invalid, unable to read output stream");
        return ReadDataFromHandle(m_stdOutRead, readBuffer, bufferSize);
    }

    AZ::u32 StdInOutProcessCommunicator::PeekOutput()
    {
        AZ_Assert(m_stdOutRead->IsValid(), "Output read handle is invalid, unable to read output stream");
        return PeekHandle(m_stdOutRead);
    }

    AZ::u32 StdInOutProcessCommunicator::WriteInput(const void* writeBuffer, AZ::u32 bytesToWrite)
    {
        AZ_Assert(m_stdInWrite->IsValid(), "Input write handle is invalid, unable to write input stream");
        return WriteDataToHandle(m_stdInWrite, writeBuffer, bytesToWrite);
    }

    void StdInOutProcessCommunicator::CloseAllHandles()
    {
        m_stdInWrite->Close();
        m_stdOutRead->Close();
        m_stdErrRead->Close();
        m_communicatorData.reset();
        m_initialized = false;
    }

    StdInOutProcessCommunicatorForChildProcess::StdInOutProcessCommunicatorForChildProcess()
        : m_stdInRead(new CommunicatorHandleImpl())
        , m_stdOutWrite(new CommunicatorHandleImpl())
        , m_stdErrWrite(new CommunicatorHandleImpl())
    {
    }

    StdInOutProcessCommunicatorForChildProcess::~StdInOutProcessCommunicatorForChildProcess()
    {
        CloseAllHandles();
    }

    bool StdInOutProcessCommunicatorForChildProcess::IsValid() const
    {
        return m_initialized && (m_stdInRead->IsValid() || m_stdOutWrite->IsValid() || m_stdErrWrite->IsValid());
    }

    AZ::u32 StdInOutProcessCommunicatorForChildProcess::WriteError(const void* writeBuffer, AZ::u32 bytesToWrite)
    {
        return WriteDataToHandle(m_stdErrWrite, writeBuffer, bytesToWrite);
    }

    AZ::u32 StdInOutProcessCommunicatorForChildProcess::WriteOutput(const void* writeBuffer, AZ::u32 bytesToWrite)
    {
        return WriteDataToHandle(m_stdOutWrite, writeBuffer, bytesToWrite);
    }

    AZ::u32 StdInOutProcessCommunicatorForChildProcess::PeekInput()
    {
        return PeekHandle(m_stdInRead);
    }

    AZ::u32 StdInOutProcessCommunicatorForChildProcess::ReadInput(void* buffer, AZ::u32 bufferSize)
    {
        return ReadDataFromHandle(m_stdInRead, buffer, bufferSize);
    }

    void StdInOutProcessCommunicatorForChildProcess::CloseAllHandles()
    {
        m_stdInRead->Close();
        m_stdOutWrite->Close();
        m_stdErrWrite->Close();
        m_initialized = false;
    }
} // namespace AzFramework
