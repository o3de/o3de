/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Multiplayer/IMultiplayer.h>
#include <Multiplayer/NetworkTime/RewindableObject.h>
#include <Source/NetworkTime/NetworkTime.h>
#include <AzCore/Console/LoggerSystemComponent.h>
#include <AzCore/Time/TimeSystem.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    class RewindableObjectTests
        : public LeakDetectionFixture
    {
    public:
        Multiplayer::NetworkTime m_networkTime;
        AZ::LoggerSystemComponent m_loggerComponent;
        AZ::TimeSystem m_timeSystem;
    };

    static constexpr uint32_t RewindableBufferFrames = 32;

    TEST_F(RewindableObjectTests, BasicTests)
    {
        Multiplayer::RewindableObject<uint32_t, RewindableBufferFrames> test(0);

        for (uint32_t i = 0; i < 16; ++i)
        {
            test = i;
            EXPECT_EQ(i, test);
            Multiplayer::GetNetworkTime()->IncrementHostFrameId();
        }

        for (uint32_t i = 0; i < 16; ++i)
        {
            Multiplayer::ScopedAlterTime time(static_cast<Multiplayer::HostFrameId>(i), AZ::Time::ZeroTimeMs, 1.f, AzNetworking::InvalidConnectionId);
            EXPECT_EQ(i, test);
        }

        for (uint32_t i = 16; i < 48; ++i)
        {
            test = i;
            EXPECT_EQ(i, test);
            Multiplayer::GetNetworkTime()->IncrementHostFrameId();
        }

        for (uint32_t i = 16; i < 48; ++i)
        {
            Multiplayer::ScopedAlterTime time(static_cast<Multiplayer::HostFrameId>(i), AZ::Time::ZeroTimeMs, 1.f, AzNetworking::InvalidConnectionId);
            EXPECT_EQ(i, test);
        }
    }

    TEST_F(RewindableObjectTests, CurrentPreviousTests)
    {
        Multiplayer::RewindableObject<uint32_t, RewindableBufferFrames> test(0);

        for (uint32_t i = 0; i < RewindableBufferFrames; ++i)
        {
            test = i;
            EXPECT_EQ(i, test);
            Multiplayer::GetNetworkTime()->IncrementHostFrameId();
        }

        {
            // Test that Get/GetPrevious return different value when not on the owning connection
            Multiplayer::ScopedAlterTime time(static_cast<Multiplayer::HostFrameId>(RewindableBufferFrames - 1), AZ::Time::ZeroTimeMs, 1.f, AzNetworking::InvalidConnectionId);
            EXPECT_EQ(RewindableBufferFrames - 1, test.Get());
            EXPECT_EQ(RewindableBufferFrames - 2, test.GetPrevious());
            EXPECT_EQ(0, test.GetLastSerializedValue());
        }

        // Test that Get/GetPrevious return the unaltered frame on the owning conection
        Multiplayer::GetNetworkTime()->AlterTime(static_cast<Multiplayer::HostFrameId>(RewindableBufferFrames - 1), AZ::Time::ZeroTimeMs, 1.f, AzNetworking::ConnectionId(0));
        {
            Multiplayer::ScopedAlterTime time(static_cast<Multiplayer::HostFrameId>(RewindableBufferFrames - 1), AZ::Time::ZeroTimeMs, 1.f, AzNetworking::ConnectionId(0));
            test.SetOwningConnectionId(AzNetworking::ConnectionId(0));
            EXPECT_EQ(RewindableBufferFrames - 1, test.Get());
            EXPECT_EQ(RewindableBufferFrames - 1, test.GetPrevious());
            EXPECT_EQ(0, test.GetLastSerializedValue());
        }
        Multiplayer::GetNetworkTime()->AlterTime(static_cast<Multiplayer::HostFrameId>(RewindableBufferFrames), AZ::TimeMs(0), 1.f, AzNetworking::InvalidConnectionId);
    }

    TEST_F(RewindableObjectTests, OverflowTests)
    {
        Multiplayer::RewindableObject<uint32_t, RewindableBufferFrames> test(0);

        for (uint32_t i = 0; i < RewindableBufferFrames; ++i)
        {
            test = i;
            EXPECT_EQ(i, test);
            Multiplayer::GetNetworkTime()->IncrementHostFrameId();
        }

        {
            // Note that we didn't actually set any value for time rewindableBufferFrames, so we're testing fetching a value past the last time set
            Multiplayer::ScopedAlterTime time(static_cast<Multiplayer::HostFrameId>(RewindableBufferFrames), AZ::Time::ZeroTimeMs, 1.f, AzNetworking::InvalidConnectionId);
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
            AZ::Interface<Multiplayer::INetworkTime>::Get()->IncrementHostFrameId();
        }

        for (uint32_t i = 0; i < RewindableBufferFrames; ++i)
        {
            Multiplayer::ScopedAlterTime time(static_cast<Multiplayer::HostFrameId>(i), AZ::Time::ZeroTimeMs, 1.f, AzNetworking::InvalidConnectionId);
            const Object& value = test;
            EXPECT_EQ(value.value, i);
        }
    }

    TEST_F(RewindableObjectTests, TestBackfillOnLargeTimestep)
    {
        Multiplayer::RewindableObject<uint32_t, RewindableBufferFrames> test(0);
        Multiplayer::ScopedAlterTime time1(static_cast<Multiplayer::HostFrameId>(0), AZ::Time::ZeroTimeMs, 1.f, AzNetworking::InvalidConnectionId);
        test = 1;

        Multiplayer::ScopedAlterTime time2(static_cast<Multiplayer::HostFrameId>(31), AZ::Time::ZeroTimeMs, 1.f, AzNetworking::InvalidConnectionId);
        test = 2;

        for (uint32_t i = 0; i < 31; ++i)
        {
            Multiplayer::ScopedAlterTime time(static_cast<Multiplayer::HostFrameId>(i), AZ::Time::ZeroTimeMs, 1.f, AzNetworking::InvalidConnectionId);
            EXPECT_EQ(1, test);
        }

        Multiplayer::ScopedAlterTime time3(static_cast<Multiplayer::HostFrameId>(31), AZ::Time::ZeroTimeMs, 1.f,  AzNetworking::InvalidConnectionId);
        EXPECT_EQ(2, test);
    }

    TEST_F(RewindableObjectTests, TestMassiveValueOverflow)
    {
        Multiplayer::RewindableObject<uint32_t, RewindableBufferFrames> test(0);

        for (uint32_t i = 0; i < 1000; ++i)
        {
            AZ::Interface<Multiplayer::INetworkTime>::Get()->IncrementHostFrameId();
        }
        test = 1000;

        for (uint32_t i = 0; i < 1000; ++i)
        {
            Multiplayer::ScopedAlterTime time(static_cast<Multiplayer::HostFrameId>(1000 - i), AZ::Time::ZeroTimeMs, 1.f, AzNetworking::InvalidConnectionId);
            EXPECT_EQ(1000, test);
        }
    }
}
