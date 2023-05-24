/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <Builders/LuaBuilder/LuaBuilderWorker.h>
#include <AzTest/Utils.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzCore/Utils/Utils.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>

namespace UnitTest
{
    using namespace LuaBuilder;
    using namespace AssetBuilderSDK;

    class LuaBuilderTests
        : public UnitTest::LeakDetectionFixture
    {
        void SetUp() override
        {
            AZ::SettingsRegistryInterface* registry = AZ::SettingsRegistry::Get();
            auto projectPathKey =
                AZ::SettingsRegistryInterface::FixedValueString(AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey) + "/project_path";
            AZ::IO::FixedMaxPath enginePath;
            registry->Get(enginePath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
            registry->Set(projectPathKey, (enginePath / "AutomatedTesting").Native());
            AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddRuntimeFilePaths(*registry);

            AZ::ComponentApplication::StartupParameters startupParameters;
            startupParameters.m_loadSettingsRegistry = false;
            m_app.Start(m_descriptor, startupParameters);
            // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

            const AZStd::string engineRoot = AZ::Test::GetEngineRootPath();
            AZ::IO::FileIOBase::GetInstance()->SetAlias("@engroot@", engineRoot.c_str());

            AZ::IO::Path assetRoot(AZ::Utils::GetProjectPath());
            assetRoot /= "Cache";
            AZ::IO::FileIOBase::GetInstance()->SetAlias("@products@", assetRoot.c_str());

            // Set the @gemroot:<gem-name> alias for LmbrCentral gem
            AZ::Test::AddActiveGem("LmbrCentral", *registry, AZ::IO::FileIOBase::GetInstance());
        }

        void TearDown() override
        {
            m_app.Stop();
        }

        AzToolsFramework::ToolsApplication m_app;
        AZ::ComponentApplication::Descriptor m_descriptor;
    };

    TEST_F(LuaBuilderTests, ParseLuaScript_DependencyRegexStress_Success)
    {
        LuaBuilderWorker worker;

        AssetBuilderSDK::ProductPathDependencySet pathDependencies;

        AZ::IO::FixedMaxPath resolvedPath;
        AZ::IO::FileIOBase::GetInstance()->ResolvePath(resolvedPath, "@gemroot:LmbrCentral@/Code/Tests/Lua/dependencyRegexStress.lua");

        worker.ParseDependencies(resolvedPath.c_str(), pathDependencies);

        ASSERT_THAT(pathDependencies,
            testing::UnorderedElementsAre(
                ProductPathDependency("test0.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/test1.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/separated/test2.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/separated/test3.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/test4.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("test5.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/test6.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("test7.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/test8.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/test9.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/test10.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/test11.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/test12.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/test13.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/test14.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/test15.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/test16.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("test17.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("local_test0.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/local_test1.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/separated/local_test2.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/separated/local_test3.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/local_test4.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("local_test5.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/local_test6.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("local_test7.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/local_test8.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/local_test9.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/local_test10.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/local_test11.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/local_test12.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/local_test13.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/local_test14.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/local_test15.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/local_test16.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("local_test17.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile)
            )
        );
    }

    TEST_F(LuaBuilderTests, ParseLuaScript_UsingRequire_ShouldFindDependencies)
    {
        LuaBuilderWorker worker;

        AssetBuilderSDK::ProductPathDependencySet pathDependencies;

        AZ::IO::FixedMaxPath resolvedPath;
        AZ::IO::FileIOBase::GetInstance()->ResolvePath(resolvedPath, "@gemroot:LmbrCentral@/Code/Tests/Lua/test1.lua");

        worker.ParseDependencies(resolvedPath.c_str(), pathDependencies);

        ASSERT_THAT(pathDependencies,
            testing::UnorderedElementsAre(
                ProductPathDependency("scripts/test2.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("scripts/test3.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("scripts/test4.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile))
        );
    }

    TEST_F(LuaBuilderTests, ParseLuaScript_UsingReloadScript_ShouldFindDependencies)
    {
        LuaBuilderWorker worker;

        AssetBuilderSDK::ProductPathDependencySet pathDependencies;

        AZ::IO::FixedMaxPath resolvedPath;
        AZ::IO::FileIOBase::GetInstance()->ResolvePath(resolvedPath, "@gemroot:LmbrCentral@/Code/Tests/Lua/test2.lua");

        worker.ParseDependencies(resolvedPath.c_str(), pathDependencies);

        ASSERT_THAT(pathDependencies,
            testing::UnorderedElementsAre(
                ProductPathDependency("some/path/test3.lua", AssetBuilderSDK::ProductPathDependencyType::ProductFile))
        );
    }

    TEST_F(LuaBuilderTests, ParseLuaScript_WithPathInAString_ShouldFindDependencies)
    {
        LuaBuilderWorker worker;

        AssetBuilderSDK::ProductPathDependencySet pathDependencies;

        AZ::IO::FixedMaxPath resolvedPath;
        AZ::IO::FileIOBase::GetInstance()->ResolvePath(resolvedPath, "@gemroot:LmbrCentral@/Code/Tests/Lua/test3_general_dependencies.lua");

        worker.ParseDependencies(resolvedPath.c_str(), pathDependencies);

        ASSERT_THAT(pathDependencies,
            testing::UnorderedElementsAre(
                ProductPathDependency("some/file/in/some/folder.cfg", AssetBuilderSDK::ProductPathDependencyType::ProductFile))
        );
    }

    TEST_F(LuaBuilderTests, ParseLuaScript_WithConsoleCommand_ShouldFindDependencies)
    {
        LuaBuilderWorker worker;

        AssetBuilderSDK::ProductPathDependencySet pathDependencies;

        AZ::IO::FixedMaxPath resolvedPath;
        AZ::IO::FileIOBase::GetInstance()->ResolvePath(resolvedPath, "@gemroot:LmbrCentral@/Code/Tests/Lua/test4_console_command.lua");

        worker.ParseDependencies(resolvedPath.c_str(), pathDependencies);

        ASSERT_THAT(pathDependencies,
            testing::UnorderedElementsAre(
                ProductPathDependency("somefile.cfg", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("somefile/in/a/folder.cfg", AssetBuilderSDK::ProductPathDependencyType::ProductFile))
        );
    }

    void VerifyNoDependenciesGenerated(const AZStd::string& testFileUnresolvedPath)
    {
        LuaBuilderWorker worker;

        AssetBuilderSDK::ProductPathDependencySet pathDependencies;

        AZ::IO::FixedMaxPath resolvedPath;
        AZ::IO::FileIOBase::GetInstance()->ResolvePath(testFileUnresolvedPath.c_str());

        EXPECT_TRUE(AZ::IO::FileIOBase::GetInstance()->Exists(resolvedPath.c_str()));

        worker.ParseDependencies(resolvedPath.c_str(), pathDependencies);

        EXPECT_TRUE(pathDependencies.empty());
    }

    TEST_F(LuaBuilderTests, ParseLuaScript_CommentedOutDependency_EntireLine_ShouldFindNoDependencies)
    {
        VerifyNoDependenciesGenerated("@gemroot:LmbrCentral@/Code/Tests/Lua/test5_whole_line_comment.lua");
    }

    TEST_F(LuaBuilderTests, ParseLuaScript_CommentedOutDependency_PartialLine_ShouldFindNoDependencies)
    {
        VerifyNoDependenciesGenerated("@gemroot:LmbrCentral@/Code/Tests/Lua/test6_partial_line_comment.lua");
    }

    TEST_F(LuaBuilderTests, ParseLuaScript_CommentedOutDependency_BlockComment_ShouldFindNoDependencies)
    {
        VerifyNoDependenciesGenerated("@gemroot:LmbrCentral@/Code/Tests/Lua/test7_block_comment.lua");
    }

    TEST_F(LuaBuilderTests, ParseLuaScript_CommentedOutDependency_NegatedBlockComment_ShouldFindDependencies)
    {
        LuaBuilderWorker worker;

        AssetBuilderSDK::ProductPathDependencySet pathDependencies;

        AZ::IO::FixedMaxPath resolvedPath;
        AZ::IO::FileIOBase::GetInstance()->ResolvePath(resolvedPath, "@gemroot:LmbrCentral@/Code/Tests/Lua/test8_negated_block_comment.lua");

        worker.ParseDependencies(resolvedPath.c_str(), pathDependencies);

        ASSERT_THAT(pathDependencies,
            testing::UnorderedElementsAre(
                ProductPathDependency("somefile.cfg", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("somefile/in/a/folder.cfg", AssetBuilderSDK::ProductPathDependencyType::ProductFile))
        );
    }
}
