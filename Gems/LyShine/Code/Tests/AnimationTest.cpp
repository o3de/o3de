/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "LyShine_precompiled.h"

#include "LyShineTest.h"
#include <Mocks/ISystemMock.h>
#include <Mocks/ITimerMock.h>

#include <UiCanvasComponent.h>
#include <Animation/AnimSequence.h>
#include <Animation/EventNode.h>
#include <Animation/TrackEventTrack.h>

namespace UnitTest
{
    class FrameTimerMock
        : public TimerMock
    {
    public:
        const CTimeValue& GetFrameStartTime([[maybe_unused]] ITimer::ETimer which = ITimer::ETIMER_GAME) const override
        {
            return m_frameStartTime;
        }
        void AddFrameStartTime(float seconds)
        {
            m_frameStartTime += CTimeValue(seconds);
        }

    private:
        CTimeValue m_frameStartTime = CTimeValue();
    };

    class TrackEventHandler
        : public UiAnimationNotificationBus::Handler
    {
    public:
        void Connect(AZ::EntityId id)
        {
            m_busId = id;
            UiAnimationNotificationBus::Handler::BusConnect(id);
        }
        ~TrackEventHandler()
        {
            UiAnimationNotificationBus::Handler::BusDisconnect(m_busId);
        }

        void OnUiAnimationEvent([[maybe_unused]] IUiAnimationListener::EUiAnimationEvent uiAnimationEvent, AZStd::string animSequenceName) {};
        void OnUiTrackEvent(AZStd::string eventName, AZStd::string valueName, AZStd::string animSequenceName)
        {
            m_recievedEvents.push_back(EventInfo{ eventName, valueName, animSequenceName });
        }

        struct EventInfo
        {
            AZStd::string m_event;
            AZStd::string m_value;
            AZStd::string m_sequence;
        };

        AZ::EntityId m_busId;
        AZStd::vector<EventInfo> m_recievedEvents;
    };

    class LyShineAnimationTest
        : public LyShineTest
    {
    protected:
        LyShineAnimationTest()
            : m_canvasComponent(nullptr)
        {
        }

        void SetupEnvironment() override
        {
            LyShineTest::SetupEnvironment();

            m_data = AZStd::make_unique<Data>();
            m_env->m_stubEnv.pTimer = &m_data->m_timer;

            m_canvasComponent = aznew UiCanvasComponent;
        }

        void TearDown() override
        {
            delete m_canvasComponent;
            m_data.reset();

            UiAnimationNotificationBus::ClearQueuedEvents();
            LyShineTest::TearDown();
        }

        struct Data
        {
            testing::NiceMock<FrameTimerMock> m_timer;
        };

        AZStd::unique_ptr<Data> m_data;
        UiCanvasComponent* m_canvasComponent;
    };

    TEST_F(LyShineAnimationTest, Animation_TrackEventTriggered_FT)
    {
        IUiAnimationSystem* animSys = m_canvasComponent->GetAnimationSystem();
        IUiAnimSequence* sequence = animSys->CreateSequence("TestSequence", true);
        CUiAnimEventNode* eventNode = aznew CUiAnimEventNode;

        sequence->AddNode(eventNode);
        sequence->AddTrackEvent("TestTrackEvent");
        eventNode->CreateDefaultTracks();

        IUiAnimTrack* eventTrack = eventNode->GetTrackByIndex(0);
        int keyIndex = eventTrack->CreateKey(0.01f);

        IEventKey key;
        eventTrack->SetKey(keyIndex, &key);
        key.event = "TestTrackEvent";
        key.eventValue = "TestValue";
        eventTrack->SetKey(keyIndex, &key);

        animSys->AddUiAnimationListener(sequence, m_canvasComponent);
        TrackEventHandler eventHandler;
        eventHandler.Connect(m_canvasComponent->GetEntityId());
        animSys->PlaySequence(sequence, nullptr, true, true);

        for (int frame = 0; frame < 2; ++frame)
        {
            static float deltaTime = 1.0f / 60.0f;

            animSys->PreUpdate(deltaTime);
            animSys->PostUpdate(deltaTime);
            m_data->m_timer.AddFrameStartTime(deltaTime);
        }

        UiAnimationNotificationBus::ExecuteQueuedEvents();

        EXPECT_EQ(eventHandler.m_recievedEvents.size(), 1);
        EXPECT_STREQ(eventHandler.m_recievedEvents[0].m_event.c_str(), key.event.c_str());
        EXPECT_STREQ(eventHandler.m_recievedEvents[0].m_value.c_str(), key.eventValue.c_str());
        EXPECT_STREQ(eventHandler.m_recievedEvents[0].m_sequence.c_str(), sequence->GetName());
    }
} //namespace UnitTest
