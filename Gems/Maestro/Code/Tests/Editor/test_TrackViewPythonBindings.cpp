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
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>

#include <AzToolsFramework/Application/ToolsApplication.h>
#include <TrackView/TrackViewPythonFuncs.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace TrackViewPythonBindingsUnitTests
{

    class TrackViewPythonBindingsFixture
        : public UnitTest::LeakDetectionFixture
    {
    public:
        AzToolsFramework::ToolsApplication m_app;

        void SetUp() override
        {
            AzFramework::Application::Descriptor appDesc;
            AZ::ComponentApplication::StartupParameters startupParameters;
            startupParameters.m_loadSettingsRegistry = false;
            m_app.Start(appDesc, startupParameters);
            // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash 
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);
            m_app.RegisterComponentDescriptor(AzToolsFramework::TrackViewFuncsHandler::CreateDescriptor());
        }

        void TearDown() override
        {
            m_app.Stop();
        }
    };

    TEST_F(TrackViewPythonBindingsFixture, TrackViewEditorCommands_ApiExists)
    {
        AZ::BehaviorContext* behaviorContext = m_app.GetBehaviorContext();
        ASSERT_TRUE(behaviorContext);

        EXPECT_TRUE(behaviorContext->m_methods.find("set_recording") != behaviorContext->m_methods.end());

        EXPECT_TRUE(behaviorContext->m_methods.find("new_sequence") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("delete_sequence") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("set_current_sequence") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("get_num_sequences") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("get_sequence_name") != behaviorContext->m_methods.end());

        EXPECT_TRUE(behaviorContext->m_methods.find("get_sequence_time_range") != behaviorContext->m_methods.end());

        EXPECT_TRUE(behaviorContext->m_methods.find("set_sequence_time_range") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("play_sequence") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("stop_sequence") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("set_time") != behaviorContext->m_methods.end());

        EXPECT_TRUE(behaviorContext->m_methods.find("add_node") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("add_selected_entities") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("add_layer_node") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("delete_node") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("add_track") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("delete_track") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("get_num_nodes") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("get_node_name") != behaviorContext->m_methods.end());

        EXPECT_TRUE(behaviorContext->m_methods.find("get_num_track_keys") != behaviorContext->m_methods.end());

        EXPECT_TRUE(behaviorContext->m_methods.find("get_key_value") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("get_interpolated_value") != behaviorContext->m_methods.end());
    }

    class TrackViewComponentFixture
        : public UnitTest::LeakDetectionFixture
    {
    public:
        AzToolsFramework::ToolsApplication m_app;

        void SetUp() override
        {
            AzFramework::Application::Descriptor appDesc;
            AZ::ComponentApplication::StartupParameters startupParameters;
            startupParameters.m_loadSettingsRegistry = false;
            m_app.Start(appDesc, startupParameters);
            m_app.RegisterComponentDescriptor(AzToolsFramework::TrackViewComponent::CreateDescriptor());

            // Disable saving global user settings to prevent failure due to detecting file updates
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);
        }

        void TearDown() override
        {
            m_app.Stop();
        }
    };

    TEST_F(TrackViewComponentFixture, TrackViewComponent_ApiExists)
    {
        AZ::BehaviorContext* behaviorContext = m_app.GetBehaviorContext();
        ASSERT_TRUE(behaviorContext);

        auto itTrackViewBus = behaviorContext->m_ebuses.find("TrackViewBus");
        if (itTrackViewBus != behaviorContext->m_ebuses.end())
        {
            AZ::BehaviorEBus* behaviorBus = itTrackViewBus->second;
            EXPECT_TRUE(behaviorBus->m_events.find("AddNode") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("AddTrack") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("AddLayerNode") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("AddSelectedEntities") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("AddNode") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("AddTrack") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("AddLayerNode") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("AddSelectedEntities") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("DeleteNode") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("DeleteTrack") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("DeleteSequence") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("GetInterpolatedValue") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("GetKeyValue") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("GetNodeName") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("GetNumNodes") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("GetNumSequences") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("GetNumTrackKeys") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("GetSequenceName") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("GetSequenceTimeRange") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("NewSequence") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("PlaySequence") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("SetCurrentSequence") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("SetRecording") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("SetSequenceTimeRange") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("SetTime") != behaviorBus->m_events.end());
            EXPECT_TRUE(behaviorBus->m_events.find("StopSequence") != behaviorBus->m_events.end());
        }
    }
}
