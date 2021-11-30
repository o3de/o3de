/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Process/ProcessCommunicator.h>

//! ProcessCommunicatorTracePrinter listens to stderr and stdout of a running process and writes its output to the AZ_Trace system
//! Importantly, it does not do any blocking operations.
class ProcessCommunicatorTracePrinter
{
public:
    ProcessCommunicatorTracePrinter(AzFramework::ProcessCommunicator* communicator, const char* window);
    ~ProcessCommunicatorTracePrinter();

    //! Call this periodically to drain the buffers and write them.
    void Pump();

    //! Drains the buffer into the string that's being built, then traces the string when it hits a newline.
    void ParseDataBuffer(AZ::u32 readSize, bool isFromStdErr);

    //! Prints the current buffer to AZ_Error or AZ_TracePrintf so that it can be picked up by AZ::Debug::Trace
    void WriteCurrentString(bool isFromStdError);

private:
    AZStd::string m_window;
    AzFramework::ProcessCommunicator* m_communicator;
    char m_streamBuffer[128];
    AZStd::string m_stringBeingConcatenated;
    AZStd::string m_errorStringBeingConcatenated;
};
