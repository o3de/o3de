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
#include <AzCore/UserSettings/UserSettingsComponent.h>

#include <AzToolsFramework/Application/ToolsApplication.h>
#include <TerrainTexture.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace TerrainFuncsUnitTests
{

    class TerrainTexturePythonBindingsFixture
        : public UnitTest::LeakDetectionFixture
    {
    public:
        AzToolsFramework::ToolsApplication m_app;

        void SetUp() override
        {
            AzFramework::Application::Descriptor appDesc;

            m_app.Start(appDesc);
            // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);
            m_app.RegisterComponentDescriptor(AzToolsFramework::TerrainTexturePythonFuncsHandler::CreateDescriptor());
        }

        void TearDown() override
        {
            m_app.Stop();
        }
    };

    TEST_F(TerrainTexturePythonBindingsFixture, TerrainCommands_ApiExists)
    {
        AZ::BehaviorContext* behaviorContext = m_app.GetBehaviorContext();
        ASSERT_TRUE(behaviorContext);

        EXPECT_TRUE(behaviorContext->m_methods.find("open_layers") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("create_layer") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("delete_layer") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("move_layer") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("get_layer_index") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("set_layer_material") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("set_layer_splatmap") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("import_layer_splatmaps") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("export_layer_splatmap") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("set_layer_name") != behaviorContext->m_methods.end());
    }
}
