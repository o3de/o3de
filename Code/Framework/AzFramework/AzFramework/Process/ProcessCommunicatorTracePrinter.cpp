/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ProcessCommunicatorTracePrinter.h"

ProcessCommunicatorTracePrinter::ProcessCommunicatorTracePrinter(
    AzFramework::ProcessCommunicator* communicator, const char* window, TraceProcessing processingType)
    :
    m_communicator(communicator),
    m_window(window),
    m_processingType(processingType)
{
    m_stringBeingConcatenated.reserve(1024);

    if (m_processingType == TraceProcessing::Threaded)
    {
        StartPumpThread();
    }
}

ProcessCommunicatorTracePrinter::~ProcessCommunicatorTracePrinter()
{
    FlushInternal();
}

void ProcessCommunicatorTracePrinter::Pump()
{
    if (m_communicator->IsValid())
    {
        // Don't call readOutput unless there is output or else it will block...
        while (m_communicator->PeekOutput())
        {
            AZ::u32 readSize = m_communicator->ReadOutput(m_streamBuffer, AZ_ARRAY_SIZE(m_streamBuffer));
            ParseDataBuffer(readSize, false);
        }
        while (m_communicator->PeekError())
        {
            AZ::u32 readSize = m_communicator->ReadError(m_streamBuffer, AZ_ARRAY_SIZE(m_streamBuffer));
            ParseDataBuffer(readSize, true);
        }
    }
}

void ProcessCommunicatorTracePrinter::Flush()
{
    FlushInternal();

    // Start the pump thread back up again after flushing.
    if (m_processingType == TraceProcessing::Threaded)
    {
        StartPumpThread();
    }
}

void ProcessCommunicatorTracePrinter::FlushInternal()
{
    if (m_processingType == TraceProcessing::Threaded)
    {
        EndPumpThread();
    }

    // flush stdout
    Pump(); // get any remaining data if available and split it by newlines

    // if there's any further data left over in the buffer then make sure it gets written, too
    WriteCurrentString(false);

    // flush stderr
    WriteCurrentString(true);
}

void ProcessCommunicatorTracePrinter::StartPumpThread()
{
    m_runThread = true;

    AZStd::thread_desc desc;
    desc.m_name = "Process Trace Printer Pump";
    m_pumpThread = AZStd::thread(
        desc,
        [this]()
        {
            // read from the process until the handle is no longer valid
            while (m_runThread)
            {
                // Wait for more output to be ready. Note that this might not block on all platforms.
                AzFramework::ProcessCommunicator::OutputStatus status;
                m_communicator->WaitForReadyOutputs(status);

                // If we've found some data, process as much as we're given
                if (status.shouldReadErrors || status.shouldReadOutput)
                {
                    Pump();
                }
                else
                {
                    // Sleep on every empty iteration through the loop.
                    // WaitForReadyOutputs() isn't guaranteed to actually be a blocking wait on all platforms,
                    // so if we aren't supposed to read anything, sleep a bit before trying to read again.
                    constexpr int PumpingDelayMs = 10;
                    AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(PumpingDelayMs));
                }

                // If the streams are no longer valid, write any final strings that we have and exit the thread.
                // No more outputs will show up, so there's no point in looping any further.
                if (!status.outputDeviceReady && !status.errorsDeviceReady)
                {
                    WriteCurrentString(true);
                    WriteCurrentString(false);
                    break;
                }
            }
        });
}

void ProcessCommunicatorTracePrinter::EndPumpThread()
{
    // Signal our pump thread to exit as soon as it can, wait for it to exit and clear it out.
    m_runThread = false;
    if (m_pumpThread.joinable())
    {
        m_pumpThread.join();
    }
    m_pumpThread = {};

    // Make sure any remaining strings for stderr and stdout are written out.
    WriteCurrentString(true);
    WriteCurrentString(false);
}

void ProcessCommunicatorTracePrinter::ParseDataBuffer(AZ::u32 readSize, bool isFromStdErr)
{
    if (readSize > AZ_ARRAY_SIZE(m_streamBuffer))
    {
        AZ_ErrorOnce("ERROR", false, "Programmer bug:  Read size is overflowing in traceprintf communicator.");
        return;
    }

    // we cannot write the string to the same buffer, as stdError and stdOut are different streams and could
    // have different cutting points as buffers empty.
    AZStd::string& bufferToUse = isFromStdErr ? m_errorStringBeingConcatenated : m_stringBeingConcatenated;

    for (size_t pos = 0; pos < readSize; ++pos)
    {
        if ((m_streamBuffer[pos] == '\n') || (m_streamBuffer[pos] == '\r'))
        {
            WriteCurrentString(isFromStdErr);
        }
        else
        {
            bufferToUse.push_back(m_streamBuffer[pos]);
        }
    }
}

void ProcessCommunicatorTracePrinter::WriteCurrentString(bool isFromStdErr)
{
    AZStd::string& bufferToUse = isFromStdErr ? m_errorStringBeingConcatenated : m_stringBeingConcatenated;

    if (!bufferToUse.empty())
    {
        if (isFromStdErr)
        {
            AZ_Error(m_window.c_str(), false, "%s", bufferToUse.c_str());
        }
        else
        {
            // Note that the input string will never have a newline at the end of it
            // because it comes from ParseDataBuffer above, which splits by newline
            // but does not include the newlines.
            AZ_TracePrintf(m_window.c_str(), "%s\n", bufferToUse.c_str());
        }
        bufferToUse.clear();
    }
}
