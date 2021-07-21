/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AssetProcessorTest.h"

#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzCore/IO/FileIOEventBus.h>
#include <AzCore/Utils/Utils.h>

#include "BaseAssetProcessorTest.h"
#include <native/utilities/BatchApplicationManager.h>
#include <native/connection/connectionManager.h>

#include <QCoreApplication>

AZ_UNIT_TEST_HOOK(new BaseAssetProcessorTestEnvironment)

namespace AssetProcessor
{
    class UnitTestAppManager : public BatchApplicationManager
    {
    public:
        explicit UnitTestAppManager(int* argc, char*** argv)
            : BatchApplicationManager(argc, argv)
        {}

        bool PrepareForTests()
        {
            if (!ApplicationManager::Activate())
            {
                return false;
            }
            
            // tests which use the builder bus plug in their own mock version, so disconnect ours.
            AssetProcessor::AssetBuilderInfoBus::Handler::BusDisconnect();

            // Disable saving global user settings to prevent failure due to detecting file updates
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

            m_platformConfig.reset(new AssetProcessor::PlatformConfiguration);
            m_connectionManager.reset(new ConnectionManager(m_platformConfig.get()));
            RegisterObjectForQuit(m_connectionManager.get());

            return true;

        }
        AZStd::unique_ptr<AssetProcessor::PlatformConfiguration> m_platformConfig;
        AZStd::unique_ptr<ConnectionManager> m_connectionManager;
    };

    class LegacyTestAdapter : public AssetProcessorTest,
        public ::testing::WithParamInterface<std::string>,
        public AZ::IO::FileIOEventBus::Handler
    {
        void SetUp() override
        {
            AssetProcessorTest::SetUp();
            AZ::IO::FileIOEventBus::Handler::BusConnect();
            
            static int numParams = 1;
            static char processName[] = {"AssetProcessorBatch"};
            static char* namePtr = &processName[0];
            static char** paramStringArray = &namePtr;

            auto registry = AZ::SettingsRegistry::Get();
            auto bootstrapKey = AZ::SettingsRegistryInterface::FixedValueString(AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey);
            auto projectPathKey = bootstrapKey + "/project_path";
            registry->Set(projectPathKey, "AutomatedTesting");
            AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddRuntimeFilePaths(*registry);

            // Forcing the branch token into settings registry before starting the application manager.
            // This avoids writing the asset_processor.setreg file which can cause fileIO errors.
            AZ::IO::FixedMaxPathString enginePath = AZ::Utils::GetEnginePath();
            auto branchTokenKey = bootstrapKey + "/assetProcessor_branch_token";
            AZStd::string token;
            AzFramework::StringFunc::AssetPath::CalculateBranchToken(enginePath.c_str(), token);
            registry->Set(branchTokenKey, token.c_str());

            m_application.reset(new UnitTestAppManager(&numParams, &paramStringArray));
            ASSERT_EQ(m_application->BeforeRun(), ApplicationManager::Status_Success);
            ASSERT_TRUE(m_application->PrepareForTests());
        }

        void TearDown() override
        {
            m_application.reset();
            AZ::IO::FileIOEventBus::Handler::BusDisconnect();
            AssetProcessorTest::TearDown();
        }

        void OnError(
            [[maybe_unused]] const AZ::IO::SystemFile* file,
            [[maybe_unused]] const char* fileName,
            [[maybe_unused]] int errorCode) override
        {
            AZ_Error("LegacyTestAdapter", false, "File error detected with %s with code %d", fileName, errorCode);
        }

        AZStd::unique_ptr<UnitTestAppManager> m_application;

    };

    // use the list of registered legacy unit tests to generate the list of test parameters:
    std::vector<std::string> GenerateTestCases()
    {
        std::vector<std::string> names;
        UnitTestRegistry* currentTest = UnitTestRegistry::first();
        while (currentTest)
        {
            names.push_back(currentTest->getName());
            currentTest = currentTest->next();
        }
        return names;
    }

    // use the above generator function to decide what the name of the test is
    // instead of just showing "0" "1" etc
    std::string GenerateTestName(const ::testing::TestParamInfo<std::string>& info)
    {
        return info.param;
    }

    TEST_P(LegacyTestAdapter, AllTests)
    {
        // this is a generator test function.  This will be called repeatedly based on the above
        // generator function.  Each time, it will set GetParam() to be the generated value.

        // doing just one at a time per setup and teardown makes sure each one works on its own and doesn't
        // interfere with the others.
        UnitTestRegistry* currentTest = UnitTestRegistry::first();
        while (currentTest)
        {
            if (azstricmp(currentTest->getName(), GetParam().c_str()) == 0)
            {
                UnitTestRun* actualTest = currentTest->create();

                volatile bool testIsComplete = false;
                QString failMessage;

                QObject::connect(actualTest, &UnitTestRun::UnitTestPassed, [&testIsComplete]()
                {
                    testIsComplete = true;
                });

                QObject::connect(actualTest, &UnitTestRun::UnitTestFailed, [&testIsComplete, &failMessage](QString message)
                {
                    testIsComplete = true;
                    failMessage = message;
                });

                QElapsedTimer time;
                time.start();

                actualTest->StartTest();
                
                while (!testIsComplete)
                {
                    QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
                    QCoreApplication::processEvents();
                    // operation
                    if (time.elapsed() > 120 * 1000) // (ms) no test, even in debug, takes longer than two minutes
                    {
                        testIsComplete = true;
                        failMessage = QString("Legacy test deadlocked or timed out.");
                    }
                }

                // Explanation of below:  EXPECT_TRUE returns an object that can be used with the stream operator
                // to add additional information when it fails, for display to the user.
                EXPECT_TRUE(failMessage.isEmpty()) << failMessage.toUtf8().constData();

                delete actualTest;
            }
            currentTest = currentTest->next();
        }
    }

    INSTANTIATE_TEST_CASE_P(
        Test,
        LegacyTestAdapter,
        testing::ValuesIn(GenerateTestCases()),
        GenerateTestName);
};

