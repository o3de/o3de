/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/ViewportSelection/ViewportEditorModeStateTracker.h>

namespace UnitTest
{
    using ViewportEditorMode = AzToolsFramework::ViewportEditorMode;
    using ViewportEditorModeState = AzToolsFramework::ViewportEditorModeState;
    using ViewportEditorModeStateTracker = AzToolsFramework::ViewportEditorModeStateTracker;
    using ViewportEditorModeInfo = AzToolsFramework::ViewportEditorModeInfo;
    using ViewportId = ViewportEditorModeInfo::IdType;
    using ViewportEditorModeStateInterface = AzToolsFramework::ViewportEditorModeStateInterface;

    void SetAllModesActive(ViewportEditorModeState& editorModeState)
    {
        for (auto mode = 0; mode < ViewportEditorModeState::NumEditorModes; mode++)
        {
            editorModeState.SetModeActive(static_cast<ViewportEditorMode>(mode));
        }
    }

    void SetAllModesInactive(ViewportEditorModeState& editorModeState)
    {
        for (auto mode = 0; mode < ViewportEditorModeState::NumEditorModes; mode++)
        {
            editorModeState.SetModeInactive(static_cast<ViewportEditorMode>(mode));
        }
    }

    // Fixture for testing editor mode states
    class ViewportEditorModeStateTestsFixture
        : public ::testing::Test
    {
    public:
        ViewportEditorModeState m_editorModeState;
    };

    // Fixture for testing editor mode states with parameterized test arguments
    class ViewportEditorModeStateTestsFixtureWithParams
        : public ViewportEditorModeStateTestsFixture
        , public ::testing::WithParamInterface<AzToolsFramework::ViewportEditorMode>
    {
    public:
        void SetUp() override
        {
            m_selectedEditorMode = GetParam();
        }

        ViewportEditorMode m_selectedEditorMode;
    };

    // Fixture for testing the viewport editor mode state tracker
    class ViewportEditorModeStateTrackerTestFixture
        : public ToolsApplicationFixture
    {
    public:
        ViewportEditorModeStateTracker m_viewportEditorModeStteTracker;
    };

    // Subscriber of viewport editor mode notifications for a single viewport that expects a single mode to be activated/deactivated
    class ViewportEditorModeNotificationsBusHandler
        : private AzToolsFramework::ViewportEditorModeNotificationsBus::Handler
    {
     public:
        struct ReceivedEvents
        {
            bool m_onEnter = false;
            bool m_onLeave = false;
        };

        using EditModeTracker = AZStd::unordered_map<ViewportEditorMode, ReceivedEvents>;

        ViewportEditorModeNotificationsBusHandler(ViewportId viewportId)
             : m_viewportSubscription(viewportId)
        {
            AzToolsFramework::ViewportEditorModeNotificationsBus::Handler::BusConnect(m_viewportSubscription);
        }

        ~ViewportEditorModeNotificationsBusHandler()
        {
            AzToolsFramework::ViewportEditorModeNotificationsBus::Handler::BusDisconnect();
        }

        ViewportId GetViewportSubscription() const
        {
            return m_viewportSubscription;
        }

        const EditModeTracker& GetEditorModes() const
        {
            return m_editorModes;
        }

        void OnEditorModeEnter([[maybe_unused]]const ViewportEditorModeStateInterface& editorModeState, ViewportEditorMode mode) override
        {
            m_editorModes[mode].m_onEnter = true;
        }

        virtual void OnEditorModeExit([[maybe_unused]] const ViewportEditorModeStateInterface& editorModeState, ViewportEditorMode mode) override
        {
            m_editorModes[mode].m_onLeave = true;
        }

     private:
         ViewportId m_viewportSubscription;
         EditModeTracker m_editorModes;

    };

    // Fixture for testing viewport editor mode notifications publishing
    class ViewportEditorModePublisherTestFixture
        : public ViewportEditorModeStateTrackerTestFixture
    {
    public:

        void SetUpEditorFixtureImpl() override
        {
            for (auto mode = 0; mode < ViewportEditorModeState::NumEditorModes; mode++)
            {
                m_editorModeHandlers[mode] = AZStd::make_unique<ViewportEditorModeNotificationsBusHandler>(mode);
            }
        }

        void TearDownEditorFixtureImpl() override
        {
            for (auto mode = 0; mode < ViewportEditorModeState::NumEditorModes; mode++)
            {
                m_editorModeHandlers[mode].reset();
            }
        }

        AZStd::array<AZStd::unique_ptr<ViewportEditorModeNotificationsBusHandler>, ViewportEditorModeState::NumEditorModes> m_editorModeHandlers;
    };

    TEST_F(ViewportEditorModeStateTestsFixture, NumberOfEditorModesIsEqualTo4)
    {
        EXPECT_EQ(ViewportEditorModeState::NumEditorModes, 4);
    }

    TEST_F(ViewportEditorModeStateTestsFixture, InitialEditorModeStateHasAllInactiveModes)
    {
        for (auto mode = 0; mode < ViewportEditorModeState::NumEditorModes; mode++)
        {
            EXPECT_FALSE(m_editorModeState.IsModeActive(static_cast<ViewportEditorMode>(mode)));
        }
    }

    TEST_P(ViewportEditorModeStateTestsFixtureWithParams, SettingModeActiveActivatesOnlyThatMode)
    {
        m_editorModeState.SetModeActive(m_selectedEditorMode);

        for (auto mode = 0; mode < ViewportEditorModeState::NumEditorModes; mode++)
        {
            const auto editorMode = static_cast<ViewportEditorMode>(mode);
            if (editorMode == m_selectedEditorMode)
            {
                EXPECT_TRUE(m_editorModeState.IsModeActive(static_cast<ViewportEditorMode>(editorMode)));
            }
            else
            {
                EXPECT_FALSE(m_editorModeState.IsModeActive(static_cast<ViewportEditorMode>(editorMode)));
            }
        }
    }

    TEST_P(ViewportEditorModeStateTestsFixtureWithParams, SettingModeInactiveInactivatesOnlyThatMode)
    {
        SetAllModesActive(m_editorModeState);
        m_editorModeState.SetModeInactive(m_selectedEditorMode);

        for (auto mode = 0; mode < ViewportEditorModeState::NumEditorModes; mode++)
        {
            const auto editorMode = static_cast<ViewportEditorMode>(mode);
            if (editorMode == m_selectedEditorMode)
            {
                EXPECT_FALSE(m_editorModeState.IsModeActive(editorMode));
            }
            else
            {
                EXPECT_TRUE(m_editorModeState.IsModeActive(editorMode));
            }
        }
    }

    TEST_P(ViewportEditorModeStateTestsFixtureWithParams, SettingMultipleModesActiveActivatesAllThoseModesNonMutuallyExclusively)
    {
        for (auto mode = 0; mode < ViewportEditorModeState::NumEditorModes - 1; mode++)
        {
            // Given only the selected mode active
            SetAllModesInactive(m_editorModeState);
            m_editorModeState.SetModeActive(m_selectedEditorMode);

            const auto editorMode = static_cast<ViewportEditorMode>(mode);
            if (editorMode == m_selectedEditorMode)
            {
                continue;
            }

            // When other modes are activated
            m_editorModeState.SetModeActive(editorMode);

            for (auto expectedMode = 0; expectedMode < ViewportEditorModeState::NumEditorModes; expectedMode++)
            {
                const auto expectedEditorMode = static_cast<ViewportEditorMode>(expectedMode);
                if (expectedEditorMode == editorMode || expectedEditorMode == m_selectedEditorMode)
                {
                    // Expect the activated modes to be active
                    EXPECT_TRUE(m_editorModeState.IsModeActive(expectedEditorMode));
                }
                else
                {
                    // Expect the modes not active to be inactive
                    EXPECT_FALSE(m_editorModeState.IsModeActive(expectedEditorMode));
                }
            }
        }
    }

    TEST_P(ViewportEditorModeStateTestsFixtureWithParams, SettingMultipleModesInactiveInactivatesAllThoseModesNonMutuallyExclusively)
    {
        for (auto mode = 0; mode < ViewportEditorModeState::NumEditorModes - 1; mode++)
        {
            // Given only the selected mode inactive
            SetAllModesActive(m_editorModeState);
            m_editorModeState.SetModeInactive(m_selectedEditorMode);

            const auto editorMode = static_cast<ViewportEditorMode>(mode);
            if (editorMode == m_selectedEditorMode)
            {
                continue;
            }

            // When other modes are deactivated
            m_editorModeState.SetModeInactive(editorMode);

            for (auto expectedMode = 0; expectedMode < ViewportEditorModeState::NumEditorModes; expectedMode++)
            {
                const auto expectedEditorMode = static_cast<ViewportEditorMode>(expectedMode);
                if (expectedEditorMode == editorMode || expectedEditorMode == m_selectedEditorMode)
                {
                    // Expect the deactivated modes to be inactive
                    EXPECT_FALSE(m_editorModeState.IsModeActive(expectedEditorMode));
                }
                else
                {
                    // Expects the modes not deactivated to still be active
                    EXPECT_TRUE(m_editorModeState.IsModeActive(expectedEditorMode));
                }
            }
        }
    }

    INSTANTIATE_TEST_CASE_P(
        AllEditorModes,
        ViewportEditorModeStateTestsFixtureWithParams,
        ::testing::Values(
            AzToolsFramework::ViewportEditorMode::Default,
            AzToolsFramework::ViewportEditorMode::Component,
            AzToolsFramework::ViewportEditorMode::Focus,
            AzToolsFramework::ViewportEditorMode::Pick));

    TEST_F(ViewportEditorModeStateTestsFixture, SettingOutOfBoundsModeActiveIssuesErrorMsg)
    {
        UnitTest::TestRunner::Instance().StartAssertTests();
        m_editorModeState.SetModeActive(static_cast<ViewportEditorMode>(ViewportEditorModeState::NumEditorModes));
        EXPECT_EQ(1, UnitTest::TestRunner::Instance().StopAssertTests());
    }

    TEST_F(ViewportEditorModeStateTestsFixture, SettingOutOfBoundsModeInactiveIssuesErrorMsg)
    {
        UnitTest::TestRunner::Instance().StartAssertTests();
        m_editorModeState.SetModeInactive(static_cast<ViewportEditorMode>(ViewportEditorModeState::NumEditorModes));
        EXPECT_EQ(1, UnitTest::TestRunner::Instance().StopAssertTests());
    }

    TEST_F(ViewportEditorModeStateTrackerTestFixture, InitialCentralStateTrackerHasNoViewportEditorModeStates)
    {
        EXPECT_EQ(m_viewportEditorModeStteTracker.GetNumTrackedViewports(), 0);
    }

    TEST_F(ViewportEditorModeStateTrackerTestFixture, EnteringViewportEditorModeForNonExistentIdCreatesViewportEditorModeStateForThatId)
    {
        // Given a viewport not currently being tracked
        const ViewportId viewportid = 0;
        EXPECT_FALSE(m_viewportEditorModeStteTracker.IsViewportStateBeingTracked({ viewportid }));
        EXPECT_EQ(m_viewportEditorModeStteTracker.GetEditorModeState({ viewportid }), nullptr);

        // When a mode is activated for that viewport
        const auto editorMode = ViewportEditorMode::Default;
        m_viewportEditorModeStteTracker.EnterMode({ viewportid }, editorMode);
        const auto* viewportEditorModeState = m_viewportEditorModeStteTracker.GetEditorModeState({ viewportid });

        // Expect that viewport to now be tracked
        EXPECT_TRUE(m_viewportEditorModeStteTracker.IsViewportStateBeingTracked({ viewportid }));
        EXPECT_NE(viewportEditorModeState, nullptr);

        // Expect the mode for that viewport to be active
        EXPECT_TRUE(viewportEditorModeState->IsModeActive(editorMode));
    }

    TEST_F(ViewportEditorModeStateTrackerTestFixture, ExitingViewportEditorModeForNonExistentIdCreatesViewportEditorModeStateForThatIdButIssuesErrorMsg)
    {
        // Given a viewport not currently being tracked
        const ViewportId viewportid = 0;
        EXPECT_FALSE(m_viewportEditorModeStteTracker.IsViewportStateBeingTracked({ viewportid }));
        EXPECT_EQ(m_viewportEditorModeStteTracker.GetEditorModeState({ viewportid }), nullptr);

        // When a mode is deactivated for that viewport
        const auto editorMode = ViewportEditorMode::Default;
        UnitTest::ErrorHandler errorHandler(AZStd::string::format(
            "Call to ExitMode for mode '%u' on id '%i' without precursor call to EnterMode", static_cast<AZ::u32>(editorMode), viewportid).c_str());
        m_viewportEditorModeStteTracker.ExitMode({ viewportid }, editorMode);

        // Expect a warning to be issued due to no precursor activation of that mode
        EXPECT_EQ(errorHandler.GetExpectedWarningCount(), 1);

        // Expect that viewport to now be tracked
        const auto* viewportEditorModeState = m_viewportEditorModeStteTracker.GetEditorModeState({ viewportid });
        EXPECT_TRUE(m_viewportEditorModeStteTracker.IsViewportStateBeingTracked({ viewportid }));

        // Expect the mode for that viewport to be inactive
        EXPECT_NE(viewportEditorModeState, nullptr);
        EXPECT_FALSE(viewportEditorModeState->IsModeActive(editorMode));
    }

    TEST_F(ViewportEditorModeStateTrackerTestFixture, GettingNonExistentViewportEditorModeStateForIdReturnsNull)
    {
        const ViewportId viewportid = 0;
        EXPECT_FALSE(m_viewportEditorModeStteTracker.IsViewportStateBeingTracked({ viewportid }));
        EXPECT_EQ(m_viewportEditorModeStteTracker.GetEditorModeState({ viewportid }), nullptr);
    }

    TEST_F(ViewportEditorModeStateTrackerTestFixture, EnteringViewportEditorModeStateForExistingIdInThatStateIssuesWarningMsg)
    {
        // Given a viewport not currently tracked
        const ViewportId viewportid = 0;
        EXPECT_FALSE(m_viewportEditorModeStteTracker.IsViewportStateBeingTracked({ viewportid }));
        EXPECT_EQ(m_viewportEditorModeStteTracker.GetEditorModeState({ viewportid }), nullptr);

        const auto editorMode = ViewportEditorMode::Default;
        const auto expectedWarning = AZStd::string::format(
            "Duplicate call to EnterMode for mode '%u' on id '%i'", static_cast<AZ::u32>(editorMode), viewportid);

        {
            UnitTest::ErrorHandler errorHandler(expectedWarning.c_str());

            // When the mode is activated for the viewport
            m_viewportEditorModeStteTracker.EnterMode({ viewportid }, editorMode);

            // Expect no warning to be issued as there is no duplicate activation
            EXPECT_EQ(errorHandler.GetExpectedWarningCount(), 0);

            // Expect the mode to be active for the viewport
            const auto* viewportEditorModeState = m_viewportEditorModeStteTracker.GetEditorModeState({ viewportid });
            EXPECT_TRUE(m_viewportEditorModeStteTracker.IsViewportStateBeingTracked({ viewportid }));
            EXPECT_NE(viewportEditorModeState, nullptr);
            EXPECT_TRUE(viewportEditorModeState->IsModeActive(editorMode));
        }
        {
            // When the mode is activated again for the viewport
            UnitTest::ErrorHandler errorHandler(expectedWarning.c_str());
            m_viewportEditorModeStteTracker.EnterMode({ viewportid }, editorMode);

            // Expect a warning to be issued for the duplicate activation
            EXPECT_EQ(errorHandler.GetExpectedWarningCount(), 1);

            // Expect the mode to still be active for the viewport
            const auto* viewportEditorModeState = m_viewportEditorModeStteTracker.GetEditorModeState({ viewportid });
            EXPECT_TRUE(m_viewportEditorModeStteTracker.IsViewportStateBeingTracked({ viewportid }));
            EXPECT_NE(viewportEditorModeState, nullptr);
            EXPECT_TRUE(viewportEditorModeState->IsModeActive(editorMode));
        }
    }

    TEST_F(ViewportEditorModeStateTrackerTestFixture, ExitingViewportEditorModeStateForExistingIdNotInThatStateIssuesWarningMsg)
    {
        // Given a viewport not currently tracked
        const ViewportId viewportid = 0;
        EXPECT_FALSE(m_viewportEditorModeStteTracker.IsViewportStateBeingTracked({ viewportid }));
        EXPECT_EQ(m_viewportEditorModeStteTracker.GetEditorModeState({ viewportid }), nullptr);

        const auto editorMode = ViewportEditorMode::Default;
        const auto expectedWarning =
            AZStd::string::format("Duplicate call to ExitMode for mode '%u' on id '%i'", static_cast<AZ::u32>(editorMode), viewportid);

        {
            UnitTest::ErrorHandler errorHandler(expectedWarning.c_str());

            // When the mode is activated and then deactivated for the viewport
            m_viewportEditorModeStteTracker.EnterMode({ viewportid }, editorMode);
            m_viewportEditorModeStteTracker.ExitMode({ viewportid }, editorMode);

            // Expect no warning to be issued as there is no duplicate deactivation
            EXPECT_EQ(errorHandler.GetExpectedWarningCount(), 0);

            // Expect the mode to be inctive for the viewport
            const auto* viewportEditorModeState = m_viewportEditorModeStteTracker.GetEditorModeState({ viewportid });
            EXPECT_TRUE(m_viewportEditorModeStteTracker.IsViewportStateBeingTracked({ viewportid }));
            EXPECT_NE(viewportEditorModeState, nullptr);
            EXPECT_FALSE(viewportEditorModeState->IsModeActive(editorMode));
        }
        {
            UnitTest::ErrorHandler errorHandler(expectedWarning.c_str());

            // When the mode is deactivated again for the viewport
            m_viewportEditorModeStteTracker.ExitMode({ viewportid }, editorMode);

            // Expect a warning to be issued for the duplicate deactivation
            EXPECT_EQ(errorHandler.GetExpectedWarningCount(), 1);

            // Expect the mode to still be inactive for the viewport
            const auto* viewportEditorModeState = m_viewportEditorModeStteTracker.GetEditorModeState({ viewportid });
            EXPECT_TRUE(m_viewportEditorModeStteTracker.IsViewportStateBeingTracked({ viewportid }));
            EXPECT_NE(viewportEditorModeState, nullptr);
            EXPECT_FALSE(viewportEditorModeState->IsModeActive(editorMode));
        }
    }

    TEST_F(
        ViewportEditorModePublisherTestFixture,
        EnteringViewportEditorModeStateForExistingIdPublishesOnViewportEditorModeEnterEventForAllSubscribers)
    {
        // Given a set of subscribers tracking the editor modes for their exclusive viewport
        for (auto mode = 0; mode < ViewportEditorModeState::NumEditorModes; mode++)
        {
            // Expect each subscriber to have received no editor mode state changes
            EXPECT_EQ(m_editorModeHandlers[mode]->GetEditorModes().size(), 0);
        }

        // When each editor mode is activated by the state tracker for a specific viewport
        for (auto mode = 0; mode < ViewportEditorModeState::NumEditorModes; mode++)
        {
            const ViewportId viewportId = mode;
            const ViewportEditorMode editorMode = static_cast<ViewportEditorMode>(mode);
            m_viewportEditorModeStteTracker.EnterMode({ mode }, editorMode);
        }

        for (auto mode = 0; mode < ViewportEditorModeState::NumEditorModes; mode++)
        {
            // Expect only the subscribers of each viewport to have received the editor mode activated event
            const ViewportEditorMode editorMode = static_cast<ViewportEditorMode>(mode);
            const auto& editorModes = m_editorModeHandlers[mode]->GetEditorModes();
            EXPECT_EQ(editorModes.size(), 1);
            EXPECT_EQ(editorModes.count(editorMode), 1);
            const auto& expectedEditorModeSet = editorModes.find(editorMode);
            EXPECT_NE(expectedEditorModeSet, editorModes.end());
            EXPECT_TRUE(expectedEditorModeSet->second.m_onEnter);
            EXPECT_FALSE(expectedEditorModeSet->second.m_onLeave);
        }
    }

    TEST_F(
        ViewportEditorModePublisherTestFixture,
        ExitingViewportEditorModeStateForExistingIdPublishesOnViewportEditorModeExitEventForAllSubscribers)
    {
        // Given a set of subscribers tracking the editor modes for their exclusive viewport
        for (auto mode = 0; mode < ViewportEditorModeState::NumEditorModes; mode++)
        {
            EXPECT_EQ(m_editorModeHandlers[mode]->GetEditorModes().size(), 0);
        }

        // When each editor mode is activated deactivated by the state tracker for a specific viewport
        for (auto mode = 0; mode < ViewportEditorModeState::NumEditorModes; mode++)
        {
            const ViewportId viewportId = mode;
            const ViewportEditorMode editorMode = static_cast<ViewportEditorMode>(mode);
            m_viewportEditorModeStteTracker.EnterMode({ mode }, editorMode);
            m_viewportEditorModeStteTracker.ExitMode({ mode }, editorMode);
        }

        for (auto mode = 0; mode < ViewportEditorModeState::NumEditorModes; mode++)
        {
            // Expect only the subscribers of each viewport to have received the editor mode activated and deactivated event
            const ViewportEditorMode editorMode = static_cast<ViewportEditorMode>(mode);
            const auto& editorModes = m_editorModeHandlers[mode]->GetEditorModes();
            EXPECT_EQ(editorModes.size(), 1);
            EXPECT_EQ(editorModes.count(editorMode), 1);
            const auto& expectedEditorModeSet = editorModes.find(editorMode);
            EXPECT_NE(expectedEditorModeSet, editorModes.end());
            EXPECT_TRUE(expectedEditorModeSet->second.m_onEnter);
            EXPECT_TRUE(expectedEditorModeSet->second.m_onLeave);
        }
    }
}
