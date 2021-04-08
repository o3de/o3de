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

#include <Source/NetworkTime/RewindableObject.h>
#include <Source/NetworkTime/NetworkTime.h>
#include <AzCore/Console/LoggerSystemComponent.h>
#include <AzCore/Time/TimeSystemComponent.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    class RewindableObjectTests
        : public AllocatorsFixture
    {
    public:
        Multiplayer::NetworkTime m_networkTime;
        AZ::LoggerSystemComponent m_loggerComponent;
        AZ::TimeSystemComponent m_timeComponent;
    };

    static constexpr uint32_t RewindableBufferFrames = 32;

    TEST_F(RewindableObjectTests, BasicTests)
    {
        Multiplayer::RewindableObject<uint32_t, RewindableBufferFrames> test(0);

        for (uint32_t i = 0; i < 16; ++i)
        {
            test = i;
            EXPECT_EQ(i, test);
            AZ::Interface<Multiplayer::INetworkTime>::Get()->IncrementApplicationFrameId();
        }

        for (uint32_t i = 0; i < 16; ++i)
        {
            Multiplayer::ScopedAlterTime time(static_cast<Multiplayer::ApplicationFrameId>(i), AzNetworking::InvalidConnectionId);
            EXPECT_EQ(i, test);
        }

        for (uint32_t i = 16; i < 48; ++i)
        {
            test = i;
            EXPECT_EQ(i, test);
            AZ::Interface<Multiplayer::INetworkTime>::Get()->IncrementApplicationFrameId();
        }

        for (uint32_t i = 16; i < 48; ++i)
        {
            Multiplayer::ScopedAlterTime time(static_cast<Multiplayer::ApplicationFrameId>(i), AzNetworking::InvalidConnectionId);
            EXPECT_EQ(i, test);
        }
    }

    TEST_F(RewindableObjectTests, OverflowTests)
    {
        Multiplayer::RewindableObject<uint32_t, RewindableBufferFrames> test(0);

        for (uint32_t i = 0; i < RewindableBufferFrames; ++i)
        {
            test = i;
            EXPECT_EQ(i, test);
            AZ::Interface<Multiplayer::INetworkTime>::Get()->IncrementApplicationFrameId();
        }

        {
            // Note that we didn't actually set any value for time rewindableBufferFrames, so we're testing fetching a value past the last time set
            Multiplayer::ScopedAlterTime time(static_cast<Multiplayer::ApplicationFrameId>(RewindableBufferFrames), AzNetworking::InvalidConnectionId);
            EXPECT_EQ(RewindableBufferFrames - 1, test);
        }
    }

    struct Object
    {
        uint32_t value;
    };

    TEST_F(RewindableObjectTests, ComplexObject)
    {
        Multiplayer::RewindableObject<Object, RewindableBufferFrames> test({ 0 });

        for (uint32_t i = 0; i < RewindableBufferFrames; ++i)
        {
            Object& value = test.Modify();
            value.value = i;
            AZ::Interface<Multiplayer::INetworkTime>::Get()->IncrementApplicationFrameId();
        }

        for (uint32_t i = 0; i < RewindableBufferFrames; ++i)
        {
            Multiplayer::ScopedAlterTime time(static_cast<Multiplayer::ApplicationFrameId>(i), AzNetworking::InvalidConnectionId);
            const Object& value = test;
            EXPECT_EQ(value.value, i);
        }
    }

    TEST_F(RewindableObjectTests, TestBackfillOnLargeTimestep)
    {
        Multiplayer::RewindableObject<uint32_t, RewindableBufferFrames> test(0);
        Multiplayer::ScopedAlterTime time1(static_cast<Multiplayer::ApplicationFrameId>(0), AzNetworking::InvalidConnectionId);
        test = 1;

        Multiplayer::ScopedAlterTime time2(static_cast<Multiplayer::ApplicationFrameId>(31), AzNetworking::InvalidConnectionId);
        test = 2;

        for (uint32_t i = 0; i < 31; ++i)
        {
            Multiplayer::ScopedAlterTime time(static_cast<Multiplayer::ApplicationFrameId>(i), AzNetworking::InvalidConnectionId);
            EXPECT_EQ(1, test);
        }

        Multiplayer::ScopedAlterTime time3(static_cast<Multiplayer::ApplicationFrameId>(31), AzNetworking::InvalidConnectionId);
        EXPECT_EQ(2, test);
    }

    TEST_F(RewindableObjectTests, TestMassiveValueOverflow)
    {
        Multiplayer::RewindableObject<uint32_t, RewindableBufferFrames> test(0);

        for (uint32_t i = 0; i < 1000; ++i)
        {
            AZ::Interface<Multiplayer::INetworkTime>::Get()->IncrementApplicationFrameId();
        }
        test = 1000;

        for (uint32_t i = 0; i < 1000; ++i)
        {
            Multiplayer::ScopedAlterTime time(static_cast<Multiplayer::ApplicationFrameId>(1000 - i), AzNetworking::InvalidConnectionId);
            EXPECT_EQ(1000, test);
        }
    }
}
