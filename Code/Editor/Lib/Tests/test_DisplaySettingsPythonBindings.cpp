/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorDefs.h"
#include <AzTest/AzTest.h>
#include <Util/EditorUtils.h>
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <AzToolsFramework/Application/ToolsApplication.h>
#include <DisplaySettings.h>
#include <DisplaySettingsPythonFuncs.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace DisplaySettingsPythonBindingsUnitTests
{

    class DisplaySettingsPythonBindingsFixture
        : public ::UnitTest::LeakDetectionFixture
    {
    public:
        AzToolsFramework::ToolsApplication m_app;

        void SetUp() override
        {
            AzFramework::Application::Descriptor appDesc;
            AZ::ComponentApplication::StartupParameters startupParameters;
            startupParameters.m_loadSettingsRegistry = false;
            m_app.Start(appDesc, startupParameters);
            m_app.RegisterComponentDescriptor(AzToolsFramework::DisplaySettingsPythonFuncsHandler::CreateDescriptor());

            // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);
        }

        void TearDown() override
        {
            m_app.Stop();
        }
    };

    TEST_F(DisplaySettingsPythonBindingsFixture, DisplaySettingsEditorCommands_ApiExists)
    {
        AZ::BehaviorContext* behaviorContext = m_app.GetBehaviorContext();
        ASSERT_TRUE(behaviorContext);

        EXPECT_TRUE(behaviorContext->m_methods.find("get_misc_editor_settings") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("set_misc_editor_settings") != behaviorContext->m_methods.end());
    }

    class DisplaySettingsComponentFixture
        : public ::UnitTest::LeakDetectionFixture
    {
    public:
        AzToolsFramework::ToolsApplication m_app;

        void SetUp() override
        {
            AzFramework::Application::Descriptor appDesc;
            AZ::ComponentApplication::StartupParameters startupParameters;
            startupParameters.m_loadSettingsRegistry = false;
            m_app.Start(appDesc, startupParameters);
            m_app.RegisterComponentDescriptor(AzToolsFramework::DisplaySettingsComponent::CreateDescriptor());

            // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);
        }

        void TearDown() override
        {
            m_app.Stop();
        }
    };

    TEST_F(DisplaySettingsComponentFixture, DisplaySettingsComponent_ApiExists)
    {
        AZ::BehaviorContext* behaviorContext = m_app.GetBehaviorContext();
        ASSERT_TRUE(behaviorContext);

        auto itDisplaySettingsBus = behaviorContext->m_ebuses.find("DisplaySettingsBus");
        if (itDisplaySettingsBus != behaviorContext->m_ebuses.end())
        {
            AZ::BehaviorEBus* behaviorBus = itDisplaySettingsBus->second;
            EXPECT_TRUE(behaviorBus->m_events.find("GetSettingsState") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("SetSettingsState") != behaviorBus->m_events.end());
        }
    }

    TEST_F(DisplaySettingsComponentFixture, DisplaySettingsComponent_ConvertToFlagsAllUnset)
    {
        AzToolsFramework::DisplaySettingsState state;
        state.m_noCollision = false;
        state.m_noLabels = false;
        state.m_simulate = false;
        state.m_hideTracks = false;
        state.m_hideLinks = false;
        state.m_hideHelpers = false;
        state.m_showDimensionFigures = false;

        AzToolsFramework::DisplaySettingsComponent component;
        int result = component.ConvertToFlags(state);
        int expected = 0x0;
        EXPECT_EQ(result, expected);
    }

    TEST_F(DisplaySettingsComponentFixture, DisplaySettingsComponent_ConvertToFlagsAllSet)
    {
        AzToolsFramework::DisplaySettingsState state;
        state.m_noCollision = true;
        state.m_noLabels = true;
        state.m_simulate = true;
        state.m_hideTracks = true;
        state.m_hideLinks = true;
        state.m_hideHelpers = true;
        state.m_showDimensionFigures = true;

        AzToolsFramework::DisplaySettingsComponent component;
        int result = component.ConvertToFlags(state);
        int expected = SETTINGS_NOCOLLISION |
            SETTINGS_NOLABELS |
            SETTINGS_PHYSICS |
            SETTINGS_HIDE_TRACKS |
            SETTINGS_HIDE_LINKS |
            SETTINGS_HIDE_HELPERS |
            SETTINGS_SHOW_DIMENSIONFIGURES;
        EXPECT_EQ(result, expected);
    }

    TEST_F(DisplaySettingsComponentFixture, DisplaySettingsComponent_ConvertToSettingsAllSet)
    {
        int flags = SETTINGS_NOCOLLISION |
            SETTINGS_NOLABELS |
            SETTINGS_PHYSICS |
            SETTINGS_HIDE_TRACKS |
            SETTINGS_HIDE_LINKS |
            SETTINGS_HIDE_HELPERS |
            SETTINGS_SHOW_DIMENSIONFIGURES;

        AzToolsFramework::DisplaySettingsState expected;
        expected.m_noCollision = true;
        expected.m_noLabels = true;
        expected.m_simulate = true;
        expected.m_hideTracks = true;
        expected.m_hideLinks = true;
        expected.m_hideHelpers = true;
        expected.m_showDimensionFigures = true;

        AzToolsFramework::DisplaySettingsComponent component;
        AzToolsFramework::DisplaySettingsState result = component.ConvertToSettings(flags);
        EXPECT_EQ(result, expected);
    }

    TEST_F(DisplaySettingsComponentFixture, DisplaySettingsComponent_ConvertToSettingsAllUnset)
    {
        int flags = 0x0;

        AzToolsFramework::DisplaySettingsState expected;
        expected.m_noCollision = false;
        expected.m_noLabels = false;
        expected.m_simulate = false;
        expected.m_hideTracks = false;
        expected.m_hideLinks = false;
        expected.m_hideHelpers = false;
        expected.m_showDimensionFigures = false;

        AzToolsFramework::DisplaySettingsComponent component;
        AzToolsFramework::DisplaySettingsState result = component.ConvertToSettings(flags);
        EXPECT_EQ(result, expected);
    }

    TEST_F(DisplaySettingsComponentFixture, DisplaySettingsState_ToString)
    {
        AzToolsFramework::DisplaySettingsState state;
        state.m_noCollision = false;
        state.m_noLabels = true;
        state.m_simulate = false;
        state.m_hideTracks = true;
        state.m_hideLinks = false;
        state.m_hideHelpers = true;
        state.m_showDimensionFigures = false;

        AZStd::string result = state.ToString();

        EXPECT_EQ(result, "(no_collision=False, no_labels=True, simulate=False, hide_tracks=True, hide_links=False, hide_helpers=True, show_dimension_figures=False)");
    }
}
