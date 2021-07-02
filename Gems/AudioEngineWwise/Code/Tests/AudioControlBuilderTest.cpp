/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzTest/Utils.h>
#include <Builder/AudioControlBuilderComponent.h>

#include <AzCore/IO/FileIO.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzToolsFramework/Application/ToolsApplication.h>

#include <AzCore/PlatformId/PlatformId.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>

using namespace AudioControlBuilder;

namespace UnitTest
{
    class AudioControlBuilderTests
        : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            m_app.Start(AZ::ComponentApplication::Descriptor());

            // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash 
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

            if constexpr (AZ::g_currentPlatform == AZ::PlatformID::PLATFORM_WINDOWS_64)
            {
                m_currentPlatform = "pc";
            }
            else
            {
                m_currentPlatform = AZ::GetPlatformName(AZ::g_currentPlatform);
                AZStd::to_lower(m_currentPlatform.begin(), m_currentPlatform.end());
            }

            AZ::IO::FileIOBase::GetInstance()->SetAlias("@engroot@", AZ::Test::GetEngineRootPath().data());
        }

        void TearDown() override
        {
            m_app.Stop();
        }

        AZStd::string GetFullPath(AZStd::string_view fileName)
        {
            constexpr char testFileFolder[] = "@exefolder@/Test.Assets/Gems/AudioEngineWwise/";
            return AZStd::string::format("%s%.*s", testFileFolder, aznumeric_cast<int>(fileName.size()), fileName.data());
        }

        void TestFailureCase(AudioControlBuilderWorker* worker, AZStd::string_view fileName, bool expectedResult = false)
        {
            AssetBuilderSDK::ProductPathDependencySet resolvedPaths;
            AZStd::vector<AssetBuilderSDK::ProductDependency> productDependencies;

            AssetBuilderSDK::ProcessJobRequest request;
            request.m_fullPath = GetFullPath(fileName);
            request.m_sourceFile = fileName;
            request.m_platformInfo.m_identifier = m_currentPlatform;

            bool result = worker->ParseProductDependencies(request, productDependencies, resolvedPaths);
            ASSERT_EQ(result, expectedResult);
            ASSERT_EQ(resolvedPaths.size(), 0);
            ASSERT_EQ(productDependencies.size(), 0);
        }

        void TestSuccessCase(
            AudioControlBuilderWorker* worker,
            AZStd::string_view fileName,
            const AZStd::vector<const char*>& expectedPathDependencies,
            const AZStd::vector<AssetBuilderSDK::ProductDependency>& expectedProductDependencies)
        {
            AssetBuilderSDK::ProductPathDependencySet resolvedPaths;
            AZStd::vector<AssetBuilderSDK::ProductDependency> productDependencies;
            size_t referencedFilePathsCount = expectedPathDependencies.size();
            size_t referencedProductDependenciesCount = expectedProductDependencies.size();

            AssetBuilderSDK::ProductPathDependencySet expectedResolvedPaths;
            for (const char* path : expectedPathDependencies)
            {
                expectedResolvedPaths.emplace(path, AssetBuilderSDK::ProductPathDependencyType::ProductFile);
            }

            AssetBuilderSDK::ProcessJobRequest request;
            request.m_fullPath = GetFullPath(fileName);
            request.m_sourceFile = fileName;
            request.m_platformInfo.m_identifier = m_currentPlatform;

            bool result = worker->ParseProductDependencies(request, productDependencies, resolvedPaths);
            ASSERT_TRUE(result);
            ASSERT_EQ(resolvedPaths.size(), referencedFilePathsCount);
            ASSERT_EQ(productDependencies.size(), referencedProductDependenciesCount);
            if (referencedFilePathsCount > 0)
            {
                for (const AssetBuilderSDK::ProductPathDependency& dependency : expectedResolvedPaths)
                {
                    ASSERT_TRUE(resolvedPaths.find(dependency) != resolvedPaths.end()) << "Expected path dependency is not found in the process result";
                }
            }
            if (referencedProductDependenciesCount > 0)
            {
                for (const AssetBuilderSDK::ProductDependency& dependency : productDependencies)
                {
                    bool expectedDependencyExists = false;
                    for (const AssetBuilderSDK::ProductDependency& expectedProductDependency : expectedProductDependencies)
                    {
                        if (expectedProductDependency.m_dependencyId == dependency.m_dependencyId
                            && expectedProductDependency.m_flags == dependency.m_flags)
                        {
                            expectedDependencyExists = true;
                            break;
                        }
                    }

                    ASSERT_TRUE(expectedDependencyExists) << "Expected product dependency is not found in the process result";
                }
            }
        }

        void TestSuccessCase(AudioControlBuilderWorker* worker, AZStd::string_view fileName, const char* expectedFile)
        {
            AZStd::vector<const char*> expectedFiles;
            expectedFiles.push_back(expectedFile);

            AZStd::vector<AssetBuilderSDK::ProductDependency> expectedProductDependencies;

            TestSuccessCase(worker, fileName, expectedFiles, expectedProductDependencies);
        }

        void TestSuccessCaseNoDependencies(AudioControlBuilderWorker* worker, AZStd::string_view fileName)
        {
            AZStd::vector<const char*> expectedFiles;

            AZStd::vector<AssetBuilderSDK::ProductDependency> expectedProductDependencies;

            TestSuccessCase(worker, fileName, expectedFiles, expectedProductDependencies);
        }

    private:
        AzToolsFramework::ToolsApplication m_app;
        AZStd::string m_currentPlatform;
    };



    TEST_F(AudioControlBuilderTests, TestAudioControl_EmptyFile_NoProductDependencies)
    {
        // Tests passing an empty file in
        // Should output 0 dependency and return false
        AZStd::string fileName = "AudioControls/EmptyControl.xml";
        AudioControlBuilderWorker builderWorker;
        TestFailureCase(&builderWorker, fileName);
    }

    TEST_F(AudioControlBuilderTests, TestAudioControl_NoPreloadsDefined_NoProductDependencies)
    {
        AZStd::string fileName = "AudioControls/MissingPreloads.xml";
        AudioControlBuilderWorker builderWorker;
        TestSuccessCaseNoDependencies(&builderWorker, fileName);
    }

    TEST_F(AudioControlBuilderTests, TestAudioControl_MissingWwiseFileNode_NoProductDependencies)
    {
        AZStd::string fileName = "AudioControls/MissingWwiseFileNode.xml";
        AudioControlBuilderWorker builderWorker;
        TestSuccessCaseNoDependencies(&builderWorker, fileName);
    }

    TEST_F(AudioControlBuilderTests, TestAudioControl_MultiplePreloadsMultipleBanks_MultipleProductDependencies)
    {
        AZStd::vector<const char*> expectedPaths = {
            "sounds/wwise/test_bank1.bnk",
            "sounds/wwise/test_bank2.bnk",
            "sounds/wwise/test_bank3.bnk",
            "sounds/wwise/test_bank4.bnk"
        };
        AZStd::string fileName = "AudioControls/MultiplePreloadsMultipleBanks.xml";
        AudioControlBuilderWorker builderWorker;

        AZStd::vector<AssetBuilderSDK::ProductDependency> expectedProductDependencies;

        TestSuccessCase(&builderWorker, fileName, expectedPaths, expectedProductDependencies);
    }

    TEST_F(AudioControlBuilderTests, TestAudioControl_MultiplePreloadsOneBank_MultipleProductDependencies)
    {
        AZStd::vector<const char*> expectedPaths = {
            "sounds/wwise/test_bank1.bnk",
            "sounds/wwise/test_bank2.bnk"
        };
        AZStd::string fileName = "AudioControls/MultiplePreloadsOneBank.xml";
        AudioControlBuilderWorker builderWorker;

        AZStd::vector<AssetBuilderSDK::ProductDependency> expectedProductDependencies;

        TestSuccessCase(&builderWorker, fileName, expectedPaths, expectedProductDependencies);
    }

    TEST_F(AudioControlBuilderTests, TestAudioControl_OnePreloadMultipleBanks_MultipleProductDependencies)
    {
        AZStd::vector<const char*> expectedPaths = {
            "sounds/wwise/test_bank1.bnk",
            "sounds/wwise/test_bank2.bnk"
        };
        AZStd::string fileName = "AudioControls/OnePreloadMultipleBanks.xml";
        AudioControlBuilderWorker builderWorker;

        AZStd::vector<AssetBuilderSDK::ProductDependency> expectedProductDependencies;

        TestSuccessCase(&builderWorker, fileName, expectedPaths, expectedProductDependencies);
    }

    TEST_F(AudioControlBuilderTests, TestAudioControl_OnePreloadOneBank_OneProductDependency)
    {
        AZStd::vector<const char*> expectedPaths = {
            "sounds/wwise/test_bank1.bnk"
        };
        AZStd::string fileName = "AudioControls/OnePreloadOneBank.xml";
        AudioControlBuilderWorker builderWorker;

        AZStd::vector<AssetBuilderSDK::ProductDependency> expectedProductDependencies;

        TestSuccessCase(&builderWorker, fileName, expectedPaths, expectedProductDependencies);
    }


    // Legacy format tests...
    namespace Legacy
    {
        TEST_F(AudioControlBuilderTests, LegacyTestAudioControl_MissingConfigGroupNameAttribute_NoProductDependencies)
        {
            AZStd::string fileName = "AudioControls/Legacy/MissingConfigGroupNameAttribute.xml";
            AudioControlBuilderWorker builderWorker;
            TestSuccessCaseNoDependencies(&builderWorker, fileName);
        }

        TEST_F(AudioControlBuilderTests, LegacyTestAudioControl_MissingPlatformNameAttribute_NoProductDependencies)
        {
            AZStd::string fileName = "AudioControls/Legacy/MissingPlatformNameAttributeOnePreload.xml";
            AudioControlBuilderWorker builderWorker;
            TestSuccessCaseNoDependencies(&builderWorker, fileName);
        }

        TEST_F(AudioControlBuilderTests, LegacyTestAudioControl_MissingAtlPlatformsNode_NoProductDependencies)
        {
            AZStd::string fileName = "AudioControls/Legacy/MissingAtlPlatformsNode.xml";
            AudioControlBuilderWorker builderWorker;
            TestSuccessCaseNoDependencies(&builderWorker, fileName);
        }

        TEST_F(AudioControlBuilderTests, LegacyTestAudioControl_MissingPlatformNode_NoProductDependencies)
        {
            AZStd::string fileName = "AudioControls/Legacy/MissingPlatformNode.xml";
            AudioControlBuilderWorker builderWorker;
            TestSuccessCaseNoDependencies(&builderWorker, fileName);
        }

        TEST_F(AudioControlBuilderTests, LegacyTestAudioControl_MissingWwiseFileNode_NoProductDependencies)
        {
            AZStd::string fileName = "AudioControls/Legacy/MissingWwiseFileNode.xml";
            AudioControlBuilderWorker builderWorker;
            TestSuccessCaseNoDependencies(&builderWorker, fileName);
        }

        TEST_F(AudioControlBuilderTests, LegacyTestAudioControl_OnePreloadOneBank_OneProductDependency)
        {
            AZStd::string fileName = "AudioControls/Legacy/OnePreloadOneBank.xml";
            AudioControlBuilderWorker builderWorker;
            TestSuccessCase(&builderWorker, fileName, "sounds/wwise/test_bank1.bnk");
        }

        TEST_F(AudioControlBuilderTests, LegacyTestAudioControl_OnePreloadMultipleBanks_MultipleProductDependencies)
        {
            AZStd::vector<const char*> expectedPaths = {
                "sounds/wwise/test_bank1.bnk",
                "sounds/wwise/test_bank2.bnk"
            };
            AZStd::string fileName = "AudioControls/Legacy/OnePreloadMultipleBanks.xml";
            AudioControlBuilderWorker builderWorker;

            AZStd::vector<AssetBuilderSDK::ProductDependency> expectedProductDependencies;

            TestSuccessCase(&builderWorker, fileName, expectedPaths, expectedProductDependencies);
        }

        TEST_F(AudioControlBuilderTests, LegacyTestAudioControl_MultiplePreloadsOneBankEach_MultipleProductDependencies)
        {
            AZStd::vector<const char*> expectedPaths = {
                "sounds/wwise/test_bank1.bnk",
                "sounds/wwise/test_bank2.bnk"
            };
            AZStd::string fileName = "AudioControls/Legacy/MultiplePreloadsOneBank.xml";
            AudioControlBuilderWorker builderWorker;

            AZStd::vector<AssetBuilderSDK::ProductDependency> expectedProductDependencies;

            TestSuccessCase(&builderWorker, fileName, expectedPaths, expectedProductDependencies);
        }

        TEST_F(AudioControlBuilderTests, LegacyTestAudioControl_MultiplePreloadsMultipleBanksEach_MultipleProductDependencies)
        {
            AZStd::vector<const char*> expectedPaths = {
                "sounds/wwise/test_bank1.bnk",
                "sounds/wwise/test_bank2.bnk",
                "sounds/wwise/test_bank3.bnk",
                "sounds/wwise/test_bank4.bnk"
            };
            AZStd::string fileName = "AudioControls/Legacy/MultiplePreloadsMultipleBanks.xml";
            AudioControlBuilderWorker builderWorker;

            AZStd::vector<AssetBuilderSDK::ProductDependency> expectedProductDependencies;

            TestSuccessCase(&builderWorker, fileName, expectedPaths, expectedProductDependencies);
        }

        TEST_F(AudioControlBuilderTests, LegacyTestAudioControl_NoConfigGroups_NoProductDependencies)
        {
            AZStd::string fileName = "AudioControls/Legacy/NoConfigGroups.xml";
            AudioControlBuilderWorker builderWorker;
            TestSuccessCaseNoDependencies(&builderWorker, fileName);
        }

        TEST_F(AudioControlBuilderTests, LegacyTestAudioControl_WrongConfigGroup_NoProductDependencies)
        {
            AZStd::string fileName = "AudioControls/Legacy/WrongConfigGroup.xml";
            AudioControlBuilderWorker builderWorker;
            TestSuccessCaseNoDependencies(&builderWorker, fileName);
        }

    } // namespace Legacy

} // namespace UnitTest
