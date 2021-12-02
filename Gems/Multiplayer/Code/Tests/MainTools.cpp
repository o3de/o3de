/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/UnitTest.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzQtComponents/Utilities/QtPluginPaths.h>
#include <AzTest/AzTest.h>
#include <AzTest/GemTestEnvironment.h>
#include <QApplication>
#include <Multiplayer/Components/NetBindComponent.h>
#include <Source/Pipeline/NetworkSpawnableHolderComponent.h>
#include <UnitTest/ToolsTestApplication.h>

namespace Multiplayer
{
    class MultiplayerToolsTestEnvironment : public AZ::Test::GemTestEnvironment
    {
        AZ::ComponentApplication* CreateApplicationInstance() override
        {
            return aznew UnitTest::ToolsTestApplication("MultiplayerToolsTest");
        }
        
        void AddGemsAndComponents() override
        {
            AZStd::vector<AZ::ComponentDescriptor*> descriptors({
                NetBindComponent::CreateDescriptor(),
                NetworkSpawnableHolderComponent::CreateDescriptor()
            });

            AddComponentDescriptors(descriptors);
        }

        /// Allows derived environments to override to perform additional steps after the system entity is activated.
        void PostSystemEntityActivate() override
        {
            // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);
        }
    };
} // namespace UnitTest

// Required to support running integration tests with Qt
AZTEST_EXPORT int AZ_UNIT_TEST_HOOK_NAME(int argc, char** argv)
{
    ::testing::InitGoogleMock(&argc, argv);
    AzQtComponents::PrepareQtPaths();
    QApplication app(argc, argv);
    AZ::Test::printUnusedParametersWarning(argc, argv);
    AZ::Test::addTestEnvironments({new Multiplayer::MultiplayerToolsTestEnvironment});
    int result = RUN_ALL_TESTS();
    return result;
}
