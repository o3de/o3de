/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Process/ProcessCommunicator.h>

//! CommunicatorTracePrinter listens to stderr and stdout of a running process and writes its output to the AZ_Trace system
//! Importantly, it does not do any blocking operations.
class CommunicatorTracePrinter
{
public:
    CommunicatorTracePrinter(AzFramework::ProcessCommunicator* communicator, const char* window);
    ~CommunicatorTracePrinter();

    // call this periodically to drain the buffers and write them.
    void Pump();

    // drains the buffer into the string thats being built, then traces the string when it hits a newline.
    void ParseDataBuffer(AZ::u32 readSize, bool isFromStdErr);

    void WriteCurrentString(bool isFromStdError);

private:
    AZStd::string m_window;
    AzFramework::ProcessCommunicator* m_communicator;
    char m_streamBuffer[128];
    AZStd::string m_stringBeingConcatenated;
    AZStd::string m_errorStringBeingConcatenated;
};
