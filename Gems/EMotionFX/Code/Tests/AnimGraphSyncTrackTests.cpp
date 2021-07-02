/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SystemComponentFixture.h"
#include "TestAssetCode/MotionEvent.h"
#include <EMotionFX/Source/EventManager.h>
#include <EMotionFX/Source/MotionEventTable.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/TwoStringEventData.h>
#include <EMotionFX/Source/MotionData/NonUniformMotionData.h>

namespace EMotionFX
{
    struct FindEventIndicesParams
    {
        void (*eventFactory)(MotionEventTrack* track);
        float timeValue;
        size_t expectedLeft;
        size_t expectedRight;
    };

    void PrintTo(FindEventIndicesParams const object, ::std::ostream* os)
    {
        if (object.eventFactory == &MakeNoEvents)
        {
            *os << "Events: 0";
        }
        else if (object.eventFactory == &MakeOneEvent)
        {
            *os << "Events: 1";
        }
        else if (object.eventFactory == &MakeTwoEvents)
        {
            *os << "Events: 2";
        }
        else if (object.eventFactory == &MakeThreeEvents)
        {
            *os << "Events: 3";
        }
        else
        {
            *os << "Events: Unknown";
        }
        *os << " Time value: " << object.timeValue
            << " Expected left: " << object.expectedLeft
            << " Expected right: " << object.expectedRight
            ;
    }

    class TestFindEventIndicesFixture : public SystemComponentFixture,
                                        public ::testing::WithParamInterface<FindEventIndicesParams>
    {
        void SetUp() override
        {
            SystemComponentFixture::SetUp();

            m_motion = aznew Motion("TestFindEventIndicesMotion");
            m_motion->SetMotionData(aznew NonUniformMotionData());
            m_motion->GetMotionData()->SetDuration(2.0f);
            m_motion->GetEventTable()->AutoCreateSyncTrack(m_motion);
            m_syncTrack = m_motion->GetEventTable()->GetSyncTrack();

            const FindEventIndicesParams& params = GetParam();
            params.eventFactory(m_syncTrack);
        }

        void TearDown() override
        {
            m_motion->Destroy();
            SystemComponentFixture::TearDown();
        }

    protected:
        Motion* m_motion = nullptr;
        AnimGraphSyncTrack* m_syncTrack = nullptr;
    };

    TEST_P(TestFindEventIndicesFixture, TestFindEventIndices)
    {
        const FindEventIndicesParams& params = GetParam();
        size_t indexLeft, indexRight;
        m_syncTrack->FindEventIndices(params.timeValue, &indexLeft, &indexRight);
        EXPECT_EQ(indexLeft, params.expectedLeft);
        EXPECT_EQ(indexRight, params.expectedRight);
    }

    INSTANTIATE_TEST_CASE_P(TestFindEventIndices, TestFindEventIndicesFixture,
        ::testing::ValuesIn(std::vector<FindEventIndicesParams> {
            {
                MakeNoEvents,
                0.5f,
                MCORE_INVALIDINDEX32,
                MCORE_INVALIDINDEX32
            },
            {
                MakeOneEvent,
                0.0f,
                0,
                0
            },
            {
                MakeOneEvent,
                0.5f,
                0,
                0
            },
            {
                MakeTwoEvents,
                0.0f,
                1,
                0
            },
            {
                MakeTwoEvents,
                0.5f,
                0,
                1
            },
            {
                MakeTwoEvents,
                1.0f,
                1,
                0
            },
            {
                MakeThreeEvents,
                0.0f,
                2,
                0
            },
            {
                MakeThreeEvents,
                0.5f,
                0,
                1
            },
            {
                MakeThreeEvents,
                1.0f,
                1,
                2
            },
            {
                MakeThreeEvents,
                1.5f,
                2,
                0
            },
            {
                [](MotionEventTrack* track)
                {
                    MakeTwoEvents(track);
                    MakeTwoEvents(track);
                },
                0.25,
                1,
                2
            },
        });
    );

    struct FindMatchingEventsParams
    {
        void (*eventFactory)(MotionEventTrack* track);
        size_t startingIndex;
        size_t inEventAIndex;
        size_t inEventBIndex;
        size_t expectedEventA;
        size_t expectedEventB;
        bool mirrorInput;
        bool mirrorOutput;
        bool forward;
    };

    void PrintTo(FindMatchingEventsParams const object, ::std::ostream* os)
    {
        if (object.eventFactory == &MakeNoEvents)
        {
            *os << "Events: 0";
        }
        else if (object.eventFactory == &MakeOneEvent)
        {
            *os << "Events: 1";
        }
        else if (object.eventFactory == &MakeTwoLeftRightEvents)
        {
            *os << "Events: LRLR";
        }
        else
        {
            *os << "Events: Unknown";
        }
        *os << " Start index: " << object.startingIndex
            << " In Event A: " << object.inEventAIndex
            << " In Event B: " << object.inEventBIndex
            << " Expected Event A: " << object.expectedEventA
            << " Expected Event B: " << object.expectedEventB
            << " Mirror Input: " << object.mirrorInput
            << " Mirror Output: " << object.mirrorOutput
            << " Play direction: " << (object.forward ? "Forward" : "Backward")
            ;
    }

    class TestFindMatchingEventsFixture : public SystemComponentFixture,
                                          public ::testing::WithParamInterface<FindMatchingEventsParams>
    {
        void SetUp() override
        {
            SystemComponentFixture::SetUp();

            m_motion = aznew Motion("TestFindMatchingEventsMotion");
            m_motion->SetMotionData(aznew NonUniformMotionData());
            m_motion->GetMotionData()->SetDuration(4.0f);
            m_motion->GetEventTable()->AutoCreateSyncTrack(m_motion);
            m_syncTrack = m_motion->GetEventTable()->GetSyncTrack();

            const FindMatchingEventsParams& params = GetParam();
            params.eventFactory(m_syncTrack);
        }

        void TearDown() override
        {
            m_motion->Destroy();
            SystemComponentFixture::TearDown();
        }

    protected:
        Motion* m_motion = nullptr;
        AnimGraphSyncTrack* m_syncTrack = nullptr;
    };

    TEST_P(TestFindMatchingEventsFixture, TestFindMatchingEvents)
    {
        const FindMatchingEventsParams& params = GetParam();

        // Make sure we have an event to get the id of
        const size_t eventCount = m_syncTrack->GetNumEvents();
        const size_t eventAID = eventCount ? m_syncTrack->GetEvent(params.inEventAIndex).HashForSyncing(params.mirrorInput) : 0;
        const size_t eventBID = eventCount ? m_syncTrack->GetEvent(params.inEventBIndex).HashForSyncing(params.mirrorInput) : 0;

        size_t outLeft, outRight;
        m_syncTrack->FindMatchingEvents(
            params.startingIndex,
            eventAID,
            eventBID,
            &outLeft,
            &outRight,
            params.forward,
            params.mirrorOutput
        );
        EXPECT_EQ(outLeft, params.expectedEventA);
        EXPECT_EQ(outRight, params.expectedEventB);
    }

    INSTANTIATE_TEST_CASE_P(TestFindMatchingEvents, TestFindMatchingEventsFixture,
        ::testing::ValuesIn(std::vector<FindMatchingEventsParams> {
            // With no events, it shouldn't matter what we put in, we'll get
            // back invalid indices
            {
                MakeNoEvents,
                0,     // startingIndex
                0,     // inEventAIndex
                1,     // inEventBIndex
                MCORE_INVALIDINDEX32, // expectedEventA
                MCORE_INVALIDINDEX32, // expectedEventB
                false, // mirrorInput
                false, // mirrorOutput
                true   // forward
            },

            // With just one event, we'll always get back indices (0,0)
            {
                MakeOneEvent,
                0,     // startingIndex
                0,     // inEventAIndex
                0,     // inEventBIndex
                0,     // expectedEventA
                0,     // expectedEventB
                false, // mirrorInput
                false, // mirrorOutput
                true   // forward
            },

            // When forward is true
            // Look for L->R events. The L->R event pairs are (0,1) and (2,3)
            // (expectedEventA will be 0 or 2 and expectedEventB will be 1 or 3)
            {
                // Starting at event 0[L], looking for events L->R, should find events 0 and 1
                MakeTwoLeftRightEvents,
                0,     // startingIndex
                0,     // inEventAIndex
                1,     // inEventBIndex
                0,     // expectedEventA
                1,     // expectedEventB
                false, // mirrorInput
                false, // mirrorOutput
                true   // forward
            },
            {
                // Starting at event 1[R], looking for events L->R, should find events 2 and 3
                MakeTwoLeftRightEvents,
                1,     // startingIndex
                0,     // inEventAIndex
                1,     // inEventBIndex
                2,     // expectedEventA
                3,     // expectedEventB
                false, // mirrorInput
                false, // mirrorOutput
                true   // forward
            },
            {
                // Starting at event 2[L], looking for events L->R, should find events 2 and 3
                MakeTwoLeftRightEvents,
                2,     // startingIndex
                0,     // inEventAIndex
                1,     // inEventBIndex
                2,     // expectedEventA
                3,     // expectedEventB
                false, // mirrorInput
                false, // mirrorOutput
                true   // forward
            },
            {
                // Starting at event 3[R], looking for events L->R, should find events 0 and 1
                MakeTwoLeftRightEvents,
                3,     // startingIndex
                0,     // inEventAIndex
                1,     // inEventBIndex
                0,     // expectedEventA
                1,     // expectedEventB
                false, // mirrorInput
                false, // mirrorOutput
                true   // forward
            },

            // Look for R->L events. The R->R event pairs are (1,2) and (3,0)
            // (expectedEventA will be 1 or 3 and expectedEventB will be 2 or 0)
            {
                // Starting at event 0[L], looking for events R->L, should find events 1 and 2
                MakeTwoLeftRightEvents,
                0,     // startingIndex
                1,     // inEventAIndex
                2,     // inEventBIndex
                1,     // expectedEventA
                2,     // expectedEventB
                false, // mirrorInput
                false, // mirrorOutput
                true   // forward
            },
            {
                // Starting at event 1[R], looking for events R->L, should find events 1 and 2
                MakeTwoLeftRightEvents,
                1,     // startingIndex
                1,     // inEventAIndex
                2,     // inEventBIndex
                1,     // expectedEventA
                2,     // expectedEventB
                false, // mirrorInput
                false, // mirrorOutput
                true   // forward
            },
            {
                // Starting at event 2[L], looking for events R->L, should find events 3 and 0
                MakeTwoLeftRightEvents,
                2,     // startingIndex
                1,     // inEventAIndex
                2,     // inEventBIndex
                3,     // expectedEventA
                0,     // expectedEventB
                false, // mirrorInput
                false, // mirrorOutput
                true   // forward
            },
            {
                // Starting at event 3[R], looking for events R->L, should find events 3 and 0
                MakeTwoLeftRightEvents,
                3,     // startingIndex
                1,     // inEventAIndex
                2,     // inEventBIndex
                3,     // expectedEventA
                0,     // expectedEventB
                false, // mirrorInput
                false, // mirrorOutput
                true   // forward
            },

            // When forward is false
            // Look for L->R events. The L->R event pairs are (0,1) and (2,3)
            // (expectedEventA will be 0 or 2 and expectedEventB will be 1 or 3)
            {
                // Starting at event 0[L], looking for events L->R, going backward, should find events 2 and 3
                MakeTwoLeftRightEvents,
                0,     // startingIndex
                0,     // inEventAIndex
                1,     // inEventBIndex
                2,     // expectedEventA
                3,     // expectedEventB
                false, // mirrorInput
                false, // mirrorOutput
                false  // forward
            },
            {
                // Starting at event 1[R], looking for events L->R, going backward, should find events 0 and 1
                MakeTwoLeftRightEvents,
                1,     // startingIndex
                0,     // inEventAIndex
                1,     // inEventBIndex
                0,     // expectedEventA
                1,     // expectedEventB
                false, // mirrorInput
                false, // mirrorOutput
                false  // forward
            },
            {
                // Starting at event 2[L], looking for events L->R, going backward, should find events 0 and 1
                MakeTwoLeftRightEvents,
                2,     // startingIndex
                0,     // inEventAIndex
                1,     // inEventBIndex
                0,     // expectedEventA
                1,     // expectedEventB
                false, // mirrorInput
                false, // mirrorOutput
                false  // forward
            },
            {
                // Starting at event 3[R], looking for events L->R, going backward, should find events 2 and 3
                MakeTwoLeftRightEvents,
                3,     // startingIndex
                0,     // inEventAIndex
                1,     // inEventBIndex
                2,     // expectedEventA
                3,     // expectedEventB
                false, // mirrorInput
                false, // mirrorOutput
                false  // forward
            },

            // Look for R->L events. The R->R event pairs are (1,2) and (3,0)
            // (expectedEventA will be 1 or 3 and expectedEventB will be 2 or 0)
            {
                // Starting at event 0[L], looking for events R->L, going backward, should find events 3 and 0
                MakeTwoLeftRightEvents,
                0,     // startingIndex
                1,     // inEventAIndex
                2,     // inEventBIndex
                3,     // expectedEventA
                0,     // expectedEventB
                false, // mirrorInput
                false, // mirrorOutput
                false  // forward
            },
            {
                // Starting at event 1[R], looking for events R->L, going backward, should find events 3 and 0
                MakeTwoLeftRightEvents,
                1,     // startingIndex
                1,     // inEventAIndex
                2,     // inEventBIndex
                3,     // expectedEventA
                0,     // expectedEventB
                false, // mirrorInput
                false, // mirrorOutput
                false  // forward
            },
            {
                // Starting at event 2[L], looking for events R->L, going backward, should find events 1 and 2
                MakeTwoLeftRightEvents,
                2,     // startingIndex
                1,     // inEventAIndex
                2,     // inEventBIndex
                1,     // expectedEventA
                2,     // expectedEventB
                false, // mirrorInput
                false, // mirrorOutput
                false  // forward
            },
            {
                // Starting at event 3[R], looking for events R->L, going backward, should find events 1 and 2
                MakeTwoLeftRightEvents,
                3,     // startingIndex
                1,     // inEventAIndex
                2,     // inEventBIndex
                1,     // expectedEventA
                2,     // expectedEventB
                false, // mirrorInput
                false, // mirrorOutput
                false  // forward
            }
        })
    );

} // end namespace EMotionFX
