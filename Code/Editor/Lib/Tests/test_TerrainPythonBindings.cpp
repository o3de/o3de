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
#include <Terrain/PythonTerrainFuncs.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace TerrainFuncsUnitTests
{

    class TerrainPythonBindingsFixture
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
            m_app.RegisterComponentDescriptor(AzToolsFramework::TerrainPythonFuncsHandler::CreateDescriptor());
        }

        void TearDown() override
        {
            m_app.Stop();
        }
    };

    TEST_F(TerrainPythonBindingsFixture, TerrainCommands_ApiExists)
    {
        AZ::BehaviorContext* behaviorContext = m_app.GetBehaviorContext();
        ASSERT_TRUE(behaviorContext);

        EXPECT_TRUE(behaviorContext->m_methods.find("get_max_height") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("set_max_height") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("import_heightmap") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("export_heightmap") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("get_elevation") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("get_heightmap_elevation") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("set_elevation") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("flatten") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("reduce_range") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("smooth") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("slope_smooth") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("erase") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("resize") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("make_isle") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("normalize") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("invert") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("fetch") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("hold") != behaviorContext->m_methods.end());
    }
}
