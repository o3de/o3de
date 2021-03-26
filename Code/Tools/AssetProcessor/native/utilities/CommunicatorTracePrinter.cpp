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

#include "CommunicatorTracePrinter.h"

CommunicatorTracePrinter::CommunicatorTracePrinter(AzFramework::ProcessCommunicator* communicator, const char* window) :
    m_communicator(communicator),
    m_window(window)
{
    m_stringBeingConcatenated.reserve(1024);
}

CommunicatorTracePrinter::~CommunicatorTracePrinter()
{
    // flush stdout
    WriteCurrentString(false);
    
    // flush stderr
    WriteCurrentString(true);
}

void CommunicatorTracePrinter::Pump()
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

void CommunicatorTracePrinter::ParseDataBuffer(AZ::u32 readSize, bool isFromStdErr)
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

void CommunicatorTracePrinter::WriteCurrentString(bool isFromStdErr)
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
            AZ_TracePrintf(m_window.c_str(), "%s", bufferToUse.c_str());
        }
        bufferToUse.clear();
    }
}
