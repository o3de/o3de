/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/TraceMessagesDriller.h>
#include <AzCore/Math/Crc.h>

namespace AZ::Debug
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
        m_output->BeginTag(AZ_CRC_CE("TraceMessagesDriller"));
        m_output->Write(AZ_CRC_CE("OnAssert"), message);
        m_output->EndTag(AZ_CRC_CE("TraceMessagesDriller"));
    }

    //=========================================================================
    // OnException
    // [2/6/2013]
    //=========================================================================
    void TraceMessagesDriller::OnException(const char* message)
    {
        // Not sure if we can really capture exception since the code will stop executing very soon.
        m_output->BeginTag(AZ_CRC_CE("TraceMessagesDriller"));
        m_output->Write(AZ_CRC_CE("OnException"), message);
        m_output->EndTag(AZ_CRC_CE("TraceMessagesDriller"));
    }

    //=========================================================================
    // OnError
    // [2/6/2013]
    //=========================================================================
    void TraceMessagesDriller::OnError(const char* window, const char* message)
    {
        m_output->BeginTag(AZ_CRC_CE("TraceMessagesDriller"));
        m_output->BeginTag(AZ_CRC_CE("OnError"));
        m_output->Write(AZ_CRC_CE("Window"), window);
        m_output->Write(AZ_CRC_CE("Message"), message);
        m_output->EndTag(AZ_CRC_CE("OnError"));
        m_output->EndTag(AZ_CRC_CE("TraceMessagesDriller"));
    }

    //=========================================================================
    // OnWarning
    // [2/6/2013]
    //=========================================================================
    void TraceMessagesDriller::OnWarning(const char* window, const char* message)
    {
        m_output->BeginTag(AZ_CRC_CE("TraceMessagesDriller"));
        m_output->BeginTag(AZ_CRC_CE("OnWarning"));
        m_output->Write(AZ_CRC_CE("Window"), window);
        m_output->Write(AZ_CRC_CE("Message"), message);
        m_output->EndTag(AZ_CRC_CE("OnWarning"));
        m_output->EndTag(AZ_CRC_CE("TraceMessagesDriller"));
    }

    //=========================================================================
    // OnPrintf
    // [2/6/2013]
    //=========================================================================
    void TraceMessagesDriller::OnPrintf(const char* window, const char* message)
    {
        m_output->BeginTag(AZ_CRC_CE("TraceMessagesDriller"));
        m_output->BeginTag(AZ_CRC_CE("OnPrintf"));
        m_output->Write(AZ_CRC_CE("Window"), window);
        m_output->Write(AZ_CRC_CE("Message"), message);
        m_output->EndTag(AZ_CRC_CE("OnPrintf"));
        m_output->EndTag(AZ_CRC_CE("TraceMessagesDriller"));
    }
} // namespace AZ
