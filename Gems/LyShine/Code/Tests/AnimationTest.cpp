/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "LyShineTest.h"
#include <Mocks/ISystemMock.h>
#include <AzCore/UnitTest/Mocks/MockITime.h>
#include <AzFramework/Application/Application.h>

#include <UiCanvasComponent.h>
#include <Animation/AnimSequence.h>
#include <Animation/EventNode.h>
#include <Animation/TrackEventTrack.h>

namespace UnitTest
{
    struct AnimationTestStubTimer : public AZ::StubTimeSystem
    {
        AZ_RTTI(UnitTest::AnimationTestStubTimer, "{541EBC6C-E793-4433-9402-4CAD2F6770E3}", AZ::StubTimeSystem);

        AZ::TimeMs GetElapsedTimeMs() const override
        {
            return AZ::TimeUsToMs(m_timeUs);
        }

        AZ::TimeUs GetElapsedTimeUs() const override
        {
            return m_timeUs;
        }

        void AddFrameTime(float sec)
        {
            m_timeUs += AZ::SecondsToTimeUs(sec);
        }

        AZ::TimeUs m_timeUs = AZ::Time::ZeroTimeUs;
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

    class LyShineAnimationTestApplication : public AzFramework::Application
    {
    public:
        LyShineAnimationTestApplication()
            : AzFramework::Application()
        {
            m_timeSystem.reset();
            m_timeSystem = AZStd::make_unique<UnitTest::AnimationTestStubTimer>();
        }

        UnitTest::AnimationTestStubTimer* GetTimer()
        {
            return azdynamic_cast<UnitTest::AnimationTestStubTimer*>(m_timeSystem.get());
        }
    };

    class LyShineAnimationTest
        : public LyShineTest
    {
    protected:
        LyShineAnimationTest()
            : m_canvasComponent(nullptr)
        {
        }

        void SetupApplication() override
        {
            AZ::ComponentApplication::Descriptor appDesc;
            appDesc.m_memoryBlocksByteSize = 10 * 1024 * 1024;
            appDesc.m_recordingMode = AZ::Debug::AllocationRecords::RECORD_FULL;
            appDesc.m_stackRecordLevels = 20;

            m_application = aznew LyShineAnimationTestApplication();
            m_systemEntity = m_application->Create(appDesc);
            m_systemEntity->Init();
            m_systemEntity->Activate();
        }

        void SetupEnvironment() override
        {
            LyShineTest::SetupEnvironment();

            m_canvasComponent = aznew UiCanvasComponent;
        }

        void TearDown() override
        {
            delete m_canvasComponent;

            UiAnimationNotificationBus::ClearQueuedEvents();
            LyShineTest::TearDown();
        }

        UnitTest::AnimationTestStubTimer* GetTimer()
        {
            return static_cast<LyShineAnimationTestApplication*>(m_application)->GetTimer();
        }
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

        UnitTest::AnimationTestStubTimer* timer = GetTimer();
        for (int frame = 0; frame < 2; ++frame)
        {
            static float deltaTime = 1.0f / 60.0f;

            animSys->PreUpdate(deltaTime);
            animSys->PostUpdate(deltaTime);
            timer->AddFrameTime(deltaTime);
        }

        UiAnimationNotificationBus::ExecuteQueuedEvents();

        EXPECT_EQ(eventHandler.m_recievedEvents.size(), 1);
        EXPECT_STREQ(eventHandler.m_recievedEvents[0].m_event.c_str(), key.event.c_str());
        EXPECT_STREQ(eventHandler.m_recievedEvents[0].m_value.c_str(), key.eventValue.c_str());
        EXPECT_STREQ(eventHandler.m_recievedEvents[0].m_sequence.c_str(), sequence->GetName());
    }
} //namespace UnitTest
