/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzToolsFramework/Process/ProcessCommunicator.h>

//! CommunicatorTracePrinter listens to stderr and stdout of a running process and writes its output to the AZ_Trace system
//! Importantly, it does not do any blocking operations.
class CommunicatorTracePrinter
{
public:
    CommunicatorTracePrinter(AzToolsFramework::ProcessCommunicator* communicator, const char* window);
    ~CommunicatorTracePrinter();

    // call this periodically to drain the buffers and write them.
    void Pump();

    // drains the buffer into the string thats being built, then traces the string when it hits a newline.
    void ParseDataBuffer(AZ::u32 readSize, bool isFromStdErr);

    void WriteCurrentString(bool isFromStdError);

private:
    AZStd::string m_window;
    AzToolsFramework::ProcessCommunicator* m_communicator;
    char m_streamBuffer[128];
    AZStd::string m_stringBeingConcatenated;
    AZStd::string m_errorStringBeingConcatenated;
};