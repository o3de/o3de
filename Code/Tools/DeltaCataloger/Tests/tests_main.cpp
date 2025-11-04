/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzFramework/Asset/AssetBundleManifest.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/AssetBundle/AssetBundleAPI.h>
#include <AzToolsFramework/AssetBundle/AssetBundleComponent.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>

class AssetBundleComponentTests
    : public UnitTest::LeakDetectionFixture,
    public AZ::Debug::TraceMessageBus::Handler
{
public:
    const char* sourcePakPath = "dir1/dir2/some_test_pak.pak";
    AZStd::vector<AZStd::string> fileEntriesHasCatalog;
    AZStd::vector<AZStd::string> fileEntriesNoCatalog;
    AZStd::string catalogPath;

    AzToolsFramework::ToolsApplication app;

    using AssetBundleCommandsBus = AzToolsFramework::AssetBundleCommandsBus;

    AZStd::string CreateCatalogPrefix() const
    {
        return AzToolsFramework::AssetBundleComponent::DeltaCatalogName;
    }
protected:
    void SetUp() override
    {
        AZ::SettingsRegistryInterface* registry = AZ::SettingsRegistry::Get();
        auto projectPathKey =
            AZ::SettingsRegistryInterface::FixedValueString(AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey) + "/project_path";
        AZ::IO::FixedMaxPath enginePath;
        registry->Get(enginePath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
        registry->Set(projectPathKey, (enginePath / "AutomatedTesting").Native());
        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddRuntimeFilePaths(*registry);

        AZ::ComponentApplication::Descriptor desc;
        desc.m_useExistingAllocator = true;
        app.Start(desc);

        // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
        // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash 
        // in the unit tests.
        AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

        catalogPath = AZStd::string::format("%s.111111.xml", CreateCatalogPrefix().c_str());
        
        // normalize paths before inserting them in the containers
        AZStd::string sourcePak(sourcePakPath);
        AzFramework::StringFunc::Path::Normalize(sourcePak);
        fileEntriesHasCatalog.push_back(sourcePak);
        
        AzFramework::StringFunc::Path::Normalize(catalogPath);
        fileEntriesHasCatalog.push_back(catalogPath);
        
        fileEntriesHasCatalog.push_back(AzFramework::AssetBundleManifest::s_manifestFileName);
        
        AZStd::string firstDummyPath("basePath/somePath1");
        AzFramework::StringFunc::Path::Normalize(firstDummyPath);
        fileEntriesHasCatalog.emplace_back(firstDummyPath);
        
        AZStd::string secondDummyPath("somePath2");
        AzFramework::StringFunc::Path::Normalize(secondDummyPath);
        fileEntriesHasCatalog.emplace_back(secondDummyPath);

        fileEntriesNoCatalog.push_back(sourcePak);
        fileEntriesNoCatalog.emplace_back(firstDummyPath);
        fileEntriesNoCatalog.emplace_back(secondDummyPath);
    }


    bool OnPreError([[maybe_unused]] const char* window, [[maybe_unused]] const char* fileName, [[maybe_unused]] int line, [[maybe_unused]] const char* func, [[maybe_unused]] const char* message) override
    {
        return true;
    }

    void TearDown() override
    {
        app.Stop();
    }
};

TEST_F(AssetBundleComponentTests, HasManifest_ManifestInBundle_ExpectTrue)
{
    AZStd::vector<AZStd::string> fileEntries;
    fileEntries.push_back(AzFramework::AssetBundleManifest::s_manifestFileName);
    EXPECT_TRUE(AzToolsFramework::AssetBundleComponent::HasManifest(fileEntries));
}
TEST_F(AssetBundleComponentTests, HasManifest_ManifestNotInBundle_ExpectFalse)
{
    AZStd::vector<AZStd::string> fileEntries;
    fileEntries.push_back("randomString");
    EXPECT_FALSE(AzToolsFramework::AssetBundleComponent::HasManifest(fileEntries));
}

TEST_F(AssetBundleComponentTests, RemoveNonAssetEntries_HasManifest_NotFound)
{
    AZStd::string normalizedSourcePakPath = sourcePakPath;
    AzFramework::StringFunc::Path::Normalize(normalizedSourcePakPath);
    
    AzFramework::AssetBundleManifest manifest;
    manifest.SetCatalogName(AZStd::string::format("%s.111111.xml", CreateCatalogPrefix().c_str()));
    
    bool result = AzToolsFramework::AssetBundleComponent::RemoveNonAssetFileEntries(fileEntriesHasCatalog, normalizedSourcePakPath, &manifest);
    EXPECT_TRUE(result);

    // check to make sure that sourcePakPath doesn't exist in fileEntriesHasCatalog
    auto itr = AZStd::find(fileEntriesHasCatalog.begin(), fileEntriesHasCatalog.end(), normalizedSourcePakPath);
    EXPECT_EQ(itr, fileEntriesHasCatalog.end());

    // check to make sure that manifest doesn't exist in fileEntriesHasCatalog
    itr = AZStd::find(fileEntriesHasCatalog.begin(), fileEntriesHasCatalog.end(), AZStd::string(AzFramework::AssetBundleManifest::s_manifestFileName));
    EXPECT_EQ(itr, fileEntriesHasCatalog.end());

    // check to make sure that the catalog doesn't exist in fileEntriesHasCatalog
    itr = AZStd::find(fileEntriesHasCatalog.begin(), fileEntriesHasCatalog.end(), manifest.GetCatalogName());
    EXPECT_EQ(itr, fileEntriesHasCatalog.end());
}

TEST_F(AssetBundleComponentTests, RemoveNonAssetEntries_HasManifestCatalog_FailedToFindCatalog)
{
    AZStd::string normalizedSourcePakPath = sourcePakPath;
    AzFramework::StringFunc::Path::Normalize(normalizedSourcePakPath);

    AzFramework::AssetBundleManifest manifest;
    manifest.SetCatalogName(AZStd::string::format("%s.22222.xml", CreateCatalogPrefix().c_str()));

    AZ::Debug::TraceMessageBus::Handler::BusConnect();
    bool result = AzToolsFramework::AssetBundleComponent::RemoveNonAssetFileEntries(fileEntriesHasCatalog, normalizedSourcePakPath, &manifest);
    EXPECT_FALSE(result);
    AZ::Debug::TraceMessageBus::Handler::BusDisconnect();

    // check to make sure that sourcePakPath doesn't exist in fileEntriesHasCatalog
    auto itr = AZStd::find(fileEntriesHasCatalog.begin(), fileEntriesHasCatalog.end(), normalizedSourcePakPath);
    EXPECT_EQ(itr, fileEntriesHasCatalog.end());

    // check to make sure that manifest doesn't exist in fileEntriesHasCatalog
    itr = AZStd::find(fileEntriesHasCatalog.begin(), fileEntriesHasCatalog.end(), AZStd::string(AzFramework::AssetBundleManifest::s_manifestFileName));
    EXPECT_EQ(itr, fileEntriesHasCatalog.end());

    // check to make sure that the catalog doesn't exist in 
    itr = AZStd::find(fileEntriesHasCatalog.begin(), fileEntriesHasCatalog.end(), manifest.GetCatalogName());
    EXPECT_EQ(itr, fileEntriesHasCatalog.end());
}

TEST_F(AssetBundleComponentTests, RemoveNonAssetEntries_PakAssetEntryWasRemoved_Success)
{
    AZStd::string normalizedSourcePakPath = sourcePakPath;
    AzFramework::StringFunc::Path::Normalize(normalizedSourcePakPath);

    bool result = AzToolsFramework::AssetBundleComponent::RemoveNonAssetFileEntries(fileEntriesHasCatalog, normalizedSourcePakPath, nullptr);
    EXPECT_TRUE(result);

    // check to make sure that sourcePakPath doesn't exist in fileEntriesHasCatalog
    auto itr = AZStd::find(fileEntriesHasCatalog.begin(), fileEntriesHasCatalog.end(), normalizedSourcePakPath);
    EXPECT_EQ(itr, fileEntriesHasCatalog.end());
}

AZ_TOOLS_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
