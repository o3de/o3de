/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Time/TimeSystem.h>
#include <gmock/gmock.h>

namespace AZ
{
    class MockTimeSystem;
    using NiceTimeSystemMock =::testing::NiceMock<MockTimeSystem>;

    //used if you wish to mock any of the Get time functions.
    class MockTimeSystem
        : public ITimeRequestBus::Handler
    {
    public:
        MockTimeSystem()
        {
            AZ::Interface<ITime>::Register(this);
            ITimeRequestBus::Handler::BusConnect();
        }
        virtual ~MockTimeSystem()
        {
            AZ::Interface<ITime>::Unregister(this);
            ITimeRequestBus::Handler::BusDisconnect();
        }

        MOCK_CONST_METHOD0(GetElapsedTimeMs, TimeMs());
        MOCK_CONST_METHOD0(GetElapsedTimeUs, TimeUs());
        MOCK_CONST_METHOD0(GetRealElapsedTimeMs, TimeMs());
        MOCK_CONST_METHOD0(GetRealElapsedTimeUs, TimeUs());
        MOCK_CONST_METHOD0(GetSimulationTickDeltaTimeUs, TimeUs());
        MOCK_CONST_METHOD0(GetRealTickDeltaTimeUs, TimeUs());
        MOCK_CONST_METHOD0(GetLastSimulationTickTime, TimeUs());
        MOCK_METHOD1(SetElapsedTimeMsDebug, void(TimeMs));
        MOCK_METHOD1(SetElapsedTimeUsDebug, void(TimeUs));
        MOCK_METHOD1(SetSimulationTickDeltaOverride, void(TimeMs));
        MOCK_METHOD1(SetSimulationTickDeltaOverride, void(TimeUs));
        MOCK_CONST_METHOD0(GetSimulationTickDeltaOverride, TimeUs());
        MOCK_METHOD1(SetSimulationTickScale, void(float));
        MOCK_CONST_METHOD0(GetSimulationTickScale, float());
        MOCK_METHOD1(SetSimulationTickRate, void(int));
        MOCK_CONST_METHOD0(GetSimulationTickRate, int32_t());
    };

    //used if you wish to override any of the Get time functions with specific functionality.
    class StubTimeSystem
        : public AZ::TimeSystem
    {
    public:
        AZ_RTTI(AZ::StubTimeSystem, "{DD5D5A6A-345F-49FD-A61E-A40E63C49CFA}", AZ::TimeSystem);

        virtual AZ::TimeMs GetElapsedTimeMs() const override
        {
            return AZ::Time::ZeroTimeMs;
        }

        virtual AZ::TimeUs GetElapsedTimeUs() const override
        {
            return AZ::Time::ZeroTimeUs;
        }

        virtual AZ::TimeMs GetRealElapsedTimeMs() const override
        {
            return AZ::Time::ZeroTimeMs;
        }

        virtual AZ::TimeUs GetRealElapsedTimeUs() const override
        {
            return AZ::Time::ZeroTimeUs;
        }

        virtual AZ::TimeUs GetSimulationTickDeltaTimeUs() const override
        {
            return AZ::Time::ZeroTimeUs;
        }

        virtual AZ::TimeUs GetRealTickDeltaTimeUs() const override
        {
            return AZ::Time::ZeroTimeUs;
        }

        virtual AZ::TimeUs GetLastSimulationTickTime() const override
        {
            return AZ::Time::ZeroTimeUs;
        }

        virtual void SetElapsedTimeMsDebug([[maybe_unused]] TimeMs timeMs) override
        {
        }

        virtual void SetElapsedTimeUsDebug([[maybe_unused]] TimeUs timeUs) override
        {
        }

        virtual void SetSimulationTickDeltaOverride([[maybe_unused]]TimeMs timeMs) override
        {
        }

        virtual TimeUs GetSimulationTickDeltaOverride() const override
        {
            return AZ::Time::ZeroTimeUs; 
        }

        virtual void SetSimulationTickScale([[maybe_unused]] float scale) override
        {
        }

        virtual float GetSimulationTickScale() const override
        {
            return 1.0f;
        }

        virtual void SetSimulationTickRate([[maybe_unused]] int rate) override
        {
        }

        virtual int32_t GetSimulationTickRate() const override
        {
            return 0;
        }
    };

} // namespace AZ
