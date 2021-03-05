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

#ifndef DRILLER_CARRIER_DATAEVENTS_H
#define DRILLER_CARRIER_DATAEVENTS_H

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include "Woodpecker/Driller/DrillerEvent.h"

namespace Driller
{
    namespace Carrier
    {
        enum CarrierEventType
        {
            CET_INFO = 1
        };
    }

    struct CarrierData
    {
        AZ::s32 mDataSend;          //< Data sent (bytes).
        AZ::s32 mDataReceived;      //< Data received (bytes).
        AZ::s32 mDataResent;        //< Data resent (bytes).
        AZ::s32 mDataAcked;         //< Data acknowledged (bytes).
        AZ::s32 mPacketSend;        //< Number of packets sent.
        AZ::s32 mPacketReceived;    //< Number of packets received.
        AZ::s32 mPacketLost;        //< Number of packets lost.
        AZ::s32 mPacketAcked;       //< Number of packets acknowledged.
        float mRTT;                 //< Round-trip time.
        float mPacketLoss;          //< Packet loss percentage.
    };

    class CarrierDataEvent
        : public DrillerEvent
    {
    public:
        AZ_CLASS_ALLOCATOR(CarrierDataEvent, AZ::SystemAllocator, 0);

        CarrierDataEvent()
            : DrillerEvent(Carrier::CET_INFO)
            , mID("<none>")
        {
        };

        virtual void StepForward(Aggregator* data) { (void)data; };
        virtual void StepBackward(Aggregator* data) { (void)data; };

        AZStd::string mID;
        CarrierData mLastSecond;
        CarrierData mEffectiveLastSecond;
    };
}

#endif