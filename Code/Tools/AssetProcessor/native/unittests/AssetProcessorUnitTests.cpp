/*
* Copyright (c) Contributors to the Open 3D Engine Project.
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
*
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/

#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <native/unittests/AssetProcessorUnitTests.h>
#include <native/unittests/UnitTestUtils.h> // for the assert absorber.
#include <native/tests/MockAssetDatabaseRequestsHandler.h>

namespace UnitTest
{
    AssetProcessorUnitTestAppManager::AssetProcessorUnitTestAppManager(int* argc, char*** argv)
        : BatchApplicationManager(argc, argv)
    {
    }

    bool AssetProcessorUnitTestAppManager::PrepareForTests()
    {
        if (!ApplicationManager::Activate())
        {
            return false;
        }

        // Not needed for this test, so disconnect
        AssetProcessor::AssetBuilderInfoBus::Handler::BusDisconnect();

        // Disable saving global user settings to prevent failure due to detecting file updates
        AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

        m_platformConfig = AZStd::make_unique<AssetProcessor::PlatformConfiguration>();
        m_connectionManager = AZStd::make_unique<ConnectionManager>(m_platformConfig.get());
        RegisterObjectForQuit(m_connectionManager.get());

        return true;
    }

    AssetProcessorUnitTestBase::AssetProcessorUnitTestBase()
        :UnitTest::LeakDetectionFixture()
    {
    }

    AssetProcessorUnitTestBase::~AssetProcessorUnitTestBase()
    {
    }

    void AssetProcessorUnitTestBase::SetUp()
    {
        UnitTest::LeakDetectionFixture::SetUp();

        m_errorAbsorber = AZStd::make_unique<UnitTestUtils::AssertAbsorber>();
        m_assetDatabaseRequestsHandler = AZStd::make_unique<AssetProcessor::MockAssetDatabaseRequestsHandler>();

        static int numParams = 1;
        static char processName[] = { "AssetProcessorBatch" };
        static char* namePtr = &processName[0];
        static char** paramStringArray = &namePtr;
        m_appManager = AZStd::make_unique<AssetProcessorUnitTestAppManager>(&numParams, &paramStringArray);

        auto registry = AZ::SettingsRegistry::Get();
        auto bootstrapKey = AZ::SettingsRegistryInterface::FixedValueString(AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey);
        auto projectPathKey = bootstrapKey + "/project_path";
        AZ::IO::FixedMaxPath enginePath;
        registry->Get(enginePath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
        registry->Set(projectPathKey, (enginePath / "AutomatedTesting").Native());
        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddRuntimeFilePaths(*registry);

        // Forcing the branch token into settings registry before starting the application manager.
        // This avoids writing the asset_processor.setreg file which can cause fileIO errors.
        auto branchTokenKey = bootstrapKey + "/assetProcessor_branch_token";
        AZStd::string token;
        AzFramework::StringFunc::AssetPath::CalculateBranchToken(enginePath.c_str(), token);
        registry->Set(branchTokenKey, token.c_str());

        ASSERT_EQ(m_appManager->BeforeRun(), ApplicationManager::Status_Success);
        ASSERT_TRUE(m_appManager->PrepareForTests());
    }

    void AssetProcessorUnitTestBase::TearDown()
    {
        m_appManager.reset();

        // The temporary folder for storing the database should be removed at the end of the test.
        // If this fails it means someone left a handle to the database open.
        AZStd::string databaseLocation;
        ASSERT_TRUE(m_assetDatabaseRequestsHandler->GetAssetDatabaseLocation(databaseLocation));
        ASSERT_FALSE(databaseLocation.empty());
        m_assetDatabaseRequestsHandler.reset();
        ASSERT_FALSE(QFileInfo(databaseLocation.c_str()).dir().exists());

        m_errorAbsorber.reset();

        UnitTest::LeakDetectionFixture::TearDown();
    }
}
