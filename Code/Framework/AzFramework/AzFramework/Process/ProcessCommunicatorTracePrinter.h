/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzFramework/Process/ProcessCommunicator.h>

//! ProcessCommunicatorTracePrinter listens to stderr and stdout of a running process and writes its output to the AZ_Trace system
//! Importantly, it does not do any blocking operations.
class ProcessCommunicatorTracePrinter
{
public:
    enum class TraceProcessing
    {
        Poll,
        Threaded
    };

    //! Wraps a ProcessCommunicatorTracePrinter around an existing ProcessCommunicator, which it will then
    //! invoke to read from stdout/stderr.
    //! Because it is going to invoke functions on the given ProcessCommuncator which you pass in, it is important
    //! that the 'communicator' you pass in is only destroyed after you destroyProcessCommunicatorTracePrinter.
    ProcessCommunicatorTracePrinter(
        AzFramework::ProcessCommunicator* communicator, const char* window, TraceProcessing processingType = TraceProcessing::Poll);
    ~ProcessCommunicatorTracePrinter();

    //! Call this periodically to drain the buffers and write them.
    //! If using threaded trace printing, this should not get called
    void Pump();

    //! Output any remaining data that exists.
    //! If using threaded trace printing, the pump thread will continue to run after calling this.
    void Flush();

private:
    //! Drains the buffer into the string that's being built, then traces the string when it hits a newline.
    void ParseDataBuffer(AZ::u32 readSize, bool isFromStdErr);

    //! Prints the current buffer to AZ_Error or AZ_TracePrintf so that it can be picked up by AZ::Debug::Trace
    void WriteCurrentString(bool isFromStdError);

    //! Start the thread for monitoring the output / error pipes.
    void StartPumpThread();

    //! End the thread for monitoring the output / error pipes.
    void EndPumpThread();

    //! End the pump thread and flush the current data.
    void FlushInternal();

    AZStd::string m_window;
    AzFramework::ProcessCommunicator* m_communicator;
    char m_streamBuffer[128];
    AZStd::string m_stringBeingConcatenated;
    AZStd::string m_errorStringBeingConcatenated;
    AZStd::thread m_pumpThread;
    AZStd::atomic_bool m_runThread = false;
    const TraceProcessing m_processingType;
};
