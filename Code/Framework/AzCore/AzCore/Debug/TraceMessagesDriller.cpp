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

#include <AzCore/Debug/TraceMessagesDriller.h>
#include <AzCore/Math/Crc.h>

namespace AZ
{
    namespace Debug
    {
        //=========================================================================
        // Start
        // [2/6/2013]
        //=========================================================================
        void TraceMessagesDriller::Start(const Param* params, int numParams)
        {
            (void)params;
            (void)numParams;
            BusConnect();
        }

        //=========================================================================
        // Stop
        // [2/6/2013]
        //=========================================================================
        void TraceMessagesDriller::Stop()
        {
            BusDisconnect();
        }

        //=========================================================================
        // OnAssert
        // [2/6/2013]
        //=========================================================================
        void TraceMessagesDriller::OnAssert(const char* message)
        {
            // Not sure if we can really capture assert since the code will stop executing very soon.
            m_output->BeginTag(AZ_CRC("TraceMessagesDriller", 0xa61d1b00));
            m_output->Write(AZ_CRC("OnAssert", 0xb74db4ce), message);
            m_output->EndTag(AZ_CRC("TraceMessagesDriller", 0xa61d1b00));
        }

        //=========================================================================
        // OnException
        // [2/6/2013]
        //=========================================================================
        void TraceMessagesDriller::OnException(const char* message)
        {
            // Not sure if we can really capture exception since the code will stop executing very soon.
            m_output->BeginTag(AZ_CRC("TraceMessagesDriller", 0xa61d1b00));
            m_output->Write(AZ_CRC("OnException", 0xfe457d12), message);
            m_output->EndTag(AZ_CRC("TraceMessagesDriller", 0xa61d1b00));
        }

        //=========================================================================
        // OnError
        // [2/6/2013]
        //=========================================================================
        void TraceMessagesDriller::OnError(const char* window, const char* message)
        {
            m_output->BeginTag(AZ_CRC("TraceMessagesDriller", 0xa61d1b00));
            m_output->BeginTag(AZ_CRC("OnError", 0x4993c634));
            m_output->Write(AZ_CRC("Window", 0x8be4f9dd), window);
            m_output->Write(AZ_CRC("Message", 0xb6bd307f), message);
            m_output->EndTag(AZ_CRC("OnError", 0x4993c634));
            m_output->EndTag(AZ_CRC("TraceMessagesDriller", 0xa61d1b00));
        }

        //=========================================================================
        // OnWarning
        // [2/6/2013]
        //=========================================================================
        void TraceMessagesDriller::OnWarning(const char* window, const char* message)
        {
            m_output->BeginTag(AZ_CRC("TraceMessagesDriller", 0xa61d1b00));
            m_output->BeginTag(AZ_CRC("OnWarning", 0x7d90abea));
            m_output->Write(AZ_CRC("Window", 0x8be4f9dd), window);
            m_output->Write(AZ_CRC("Message", 0xb6bd307f), message);
            m_output->EndTag(AZ_CRC("OnWarning", 0x7d90abea));
            m_output->EndTag(AZ_CRC("TraceMessagesDriller", 0xa61d1b00));
        }

        //=========================================================================
        // OnPrintf
        // [2/6/2013]
        //=========================================================================
        void TraceMessagesDriller::OnPrintf(const char* window, const char* message)
        {
            m_output->BeginTag(AZ_CRC("TraceMessagesDriller", 0xa61d1b00));
            m_output->BeginTag(AZ_CRC("OnPrintf", 0xd4b5c294));
            m_output->Write(AZ_CRC("Window", 0x8be4f9dd), window);
            m_output->Write(AZ_CRC("Message", 0xb6bd307f), message);
            m_output->EndTag(AZ_CRC("OnPrintf", 0xd4b5c294));
            m_output->EndTag(AZ_CRC("TraceMessagesDriller", 0xa61d1b00));
        }
    } // namespace Debug
} // namespace AZ
