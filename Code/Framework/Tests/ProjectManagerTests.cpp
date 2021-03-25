/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzTest/AzTest.h>
#include <AzFramework/ProjectManager/ProjectManager.h>

#include <AzCore/IO/SystemFile.h>
#include <Utils/Utils.h>

namespace ProjectManagerTests
{


    ////////////////////////////////////////////////////////////////////////////////////////////////
    class ProjectManagerTest : public ::testing::Test
    {
    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        void SetUp() override
        {

        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        void TearDown() override
        {

        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    class ProjectManagerBootstrapTest : public ::testing::Test
    {
    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        void SetUp() override
        {
            AZ::IO::SystemFile engineFile;
            AZ::IO::FixedMaxPath engineFilePath(m_tempDir.GetDirectory());

            m_projectPath = engineFilePath / "TestProject";
            AZ::IO::SystemFile::CreateDir(m_projectPath.c_str());

            engineFilePath = engineFilePath / "engine.json";
            engineFile.Open(engineFilePath.c_str(), AZ::IO::SystemFile::SF_OPEN_CREATE);
            engineFile.Close();
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        void TearDown() override
        {

        }
        AZ::IO::FixedMaxPath m_projectPath;
        UnitTest::ScopedTemporaryDirectory m_tempDir;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    TEST_F(ProjectManagerTest, HasProjectNameTest_ProjectNameGivenWithDash_Success)
    {
        constexpr int numArguments = 2;
        static const char* argumentList1[numArguments] = { "C:/somepath/to/exe",  "-projectpath" };
        EXPECT_TRUE(AzFramework::ProjectManager::HasCommandLineProjectName(numArguments, const_cast<char**>(argumentList1)));

        static const char* argumentList2[numArguments] = { "C:/somepath/to/exe",  "--projectpath" };
        EXPECT_TRUE(AzFramework::ProjectManager::HasCommandLineProjectName(numArguments, const_cast<char**>(argumentList2)));

        static const char* argumentList3[numArguments] = { "C:/somepath/to/exe",  "/projectpath" };
        EXPECT_TRUE(AzFramework::ProjectManager::HasCommandLineProjectName(numArguments, const_cast<char**>(argumentList3)));
    }


    TEST_F(ProjectManagerTest, HasProjectNameTest_ProjectNameGivenWithRegSet_Success)
    {
        constexpr int numArguments = 2;
        static const char* argumentList1[numArguments] = { "C:/somepath/to/exe",  "-regset=\"/Amazon/AzCore/Bootstrap/sys_game_folder=SomeFolder\"" };
        EXPECT_TRUE(AzFramework::ProjectManager::HasCommandLineProjectName(numArguments, const_cast<char**>(argumentList1)));

        static const char* argumentList2[numArguments] = { "C:/somepath/to/exe",  "--regset=\"/Amazon/AzCore/Bootstrap/sys_game_folder=SomeFolder\"" };
        EXPECT_TRUE(AzFramework::ProjectManager::HasCommandLineProjectName(numArguments, const_cast<char**>(argumentList2)));

        static const char* argumentList3[numArguments] = { "C:/somepath/to/exe",  "/regset=\"/Amazon/AzCore/Bootstrap/sys_game_folder=SomeFolder\"" };
        EXPECT_TRUE(AzFramework::ProjectManager::HasCommandLineProjectName(numArguments, const_cast<char**>(argumentList3)));
    }

    TEST_F(ProjectManagerTest, HasProjectNameTest_ProjectNameGivenWithImroperPrefixes_Fails)
    {
        constexpr int numArguments = 2;
        static const char* argumentList1[numArguments] = { "C:/somepath/to/exe",  "projectpath" };
        EXPECT_FALSE(AzFramework::ProjectManager::HasCommandLineProjectName(numArguments, const_cast<char**>(argumentList1)));

        static const char* argumentList2[numArguments] = { "C:/somepath/to/exe",  "---projectpath" };
        EXPECT_FALSE(AzFramework::ProjectManager::HasCommandLineProjectName(numArguments, const_cast<char**>(argumentList2)));

        static const char* argumentList3[numArguments] = { "C:/somepath/to/exe",  "//projectpath" };
        EXPECT_FALSE(AzFramework::ProjectManager::HasCommandLineProjectName(numArguments, const_cast<char**>(argumentList3)));

        static const char* argumentList4[numArguments] = { "C:/somepath/to/exe",  "pprojectpath" };
        EXPECT_FALSE(AzFramework::ProjectManager::HasCommandLineProjectName(numArguments, const_cast<char**>(argumentList4)));
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////
    TEST_F(ProjectManagerTest, HasProjectNameTest_ProjectNameNotGiven_Fails)
    {
        constexpr int numArguments = 2;
        static const char* argumentList[numArguments] = { "C:/somepath/to/exe", "someotherparam" };

        EXPECT_FALSE(AzFramework::ProjectManager::HasCommandLineProjectName(numArguments, const_cast<char**>(argumentList)));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    TEST_F(ProjectManagerTest, HasProjectNameTest_NoAdditionalParams_Fails)
    {
        constexpr int numArguments = 1;
        static const char* argumentList[numArguments] = { "C:/somepath/to/exe" };

        EXPECT_FALSE(AzFramework::ProjectManager::HasCommandLineProjectName(numArguments, const_cast<char**>(argumentList)));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    TEST_F(ProjectManagerTest, ContentTest_HasValidProjectName_Passes)
    {
        AZStd::fixed_string<AzFramework::ProjectManager::MaxBootstrapFileSize> testContent =
R"(-- When you see an option that does not have a platform preceeding it, that is the default
--value for anything not specificly set per platform.So if remote_filesystem = 0 and you have
-- ios_remote_file_system = 1 then remote filesystem will be off for all platforms except ios
-- Any of the settings in this file can be prefixed with a platform name :
--android, ios, mac, linux, windows, etc...
-- or left unprefixed, to set all platforms not specified.The rules apply in the order they're declared

sys_game_folder=SamplesProject

-- remote_filesystem - enable Virtual File System(VFS)
--This feature allows a remote instance of the game to run off assets
-- on the asset processor computers cache instead of deploying them the remote device
-- By default it is offand can be overridden for any platform)";

        EXPECT_TRUE(AzFramework::ProjectManager::ContentHasProjectName(testContent));
    }

    TEST_F(ProjectManagerTest, ContentTest_HasValidProjectNameSpaced_Passes)
    {
        AZStd::fixed_string<AzFramework::ProjectManager::MaxBootstrapFileSize> testContent =
R"(-- When you see an option that does not have a platform preceeding it, that is the default
--value for anything not specificly set per platform.So if remote_filesystem = 0 and you have
-- ios_remote_file_system = 1 then remote filesystem will be off for all platforms except ios
-- Any of the settings in this file can be prefixed with a platform name :
--android, ios, mac, linux, windows, etc...
-- or left unprefixed, to set all platforms not specified.The rules apply in the order they're declared

sys_game_folder = SamplesProject

-- remote_filesystem - enable Virtual File System(VFS)
--This feature allows a remote instance of the game to run off assets
-- on the asset processor computers cache instead of deploying them the remote device
-- By default it is offand can be overridden for any platform)";

        EXPECT_TRUE(AzFramework::ProjectManager::ContentHasProjectName(testContent));
    }

    TEST_F(ProjectManagerTest, ContentTest_HasNoProjectName_Fails)
    {
        AZStd::fixed_string<AzFramework::ProjectManager::MaxBootstrapFileSize> testContent =
R"(-- When you see an option that does not have a platform preceeding it, that is the default
--value for anything not specificly set per platform.So if remote_filesystem = 0 and you have
-- ios_remote_file_system = 1 then remote filesystem will be off for all platforms except ios
-- Any of the settings in this file can be prefixed with a platform name :
--android, ios, mac, linux, windows, etc...
-- or left unprefixed, to set all platforms not specified.The rules apply in the order they're declared

sys_game_folder=

-- remote_filesystem - enable Virtual File System(VFS)
--This feature allows a remote instance of the game to run off assets
-- on the asset processor computers cache instead of deploying them the remote device
-- By default it is offand can be overridden for any platform)";

        EXPECT_FALSE(AzFramework::ProjectManager::ContentHasProjectName(testContent));
    }

    TEST_F(ProjectManagerTest, ContentTest_HasCommentedProjectName_Fails)
    {
        AZStd::fixed_string<AzFramework::ProjectManager::MaxBootstrapFileSize> testContent =
R"(-- When you see an option that does not have a platform preceeding it, that is the default
--value for anything not specificly set per platform.So if remote_filesystem = 0 and you have
-- ios_remote_file_system = 1 then remote filesystem will be off for all platforms except ios
-- Any of the settings in this file can be prefixed with a platform name :
--android, ios, mac, linux, windows, etc...
-- or left unprefixed, to set all platforms not specified.The rules apply in the order they're declared

-sys_game_folder=SamplesProject

-- remote_filesystem - enable Virtual File System(VFS)
--This feature allows a remote instance of the game to run off assets
-- on the asset processor computers cache instead of deploying them the remote device
-- By default it is offand can be overridden for any platform)";

        EXPECT_FALSE(AzFramework::ProjectManager::ContentHasProjectName(testContent));
    }

    TEST_F(ProjectManagerTest, ContentTest_HasCommentedThenValidProjectName_Passes)
    {
        AZStd::fixed_string<AzFramework::ProjectManager::MaxBootstrapFileSize> testContent =
R"(-- When you see an option that does not have a platform preceeding it, that is the default
--value for anything not specificly set per platform.So if remote_filesystem = 0 and you have
-- ios_remote_file_system = 1 then remote filesystem will be off for all platforms except ios
-- Any of the settings in this file can be prefixed with a platform name :
--android, ios, mac, linux, windows, etc...
-- or left unprefixed, to set all platforms not specified.The rules apply in the order they're declared

-sys_game_folder=SamplesProject
sys_game_folder=SamplesProject

-- remote_filesystem - enable Virtual File System(VFS)
--This feature allows a remote instance of the game to run off assets
-- on the asset processor computers cache instead of deploying them the remote device
-- By default it is offand can be overridden for any platform)";

        EXPECT_TRUE(AzFramework::ProjectManager::ContentHasProjectName(testContent));
    }

    TEST_F(ProjectManagerTest, ContentTest_OnlyHasValidKey_Passes)
    {
        AZStd::fixed_string<AzFramework::ProjectManager::MaxBootstrapFileSize> testContent =
            R"(sys_game_folder=SamplesProject)";

        EXPECT_TRUE(AzFramework::ProjectManager::ContentHasProjectName(testContent));
    }

    TEST_F(ProjectManagerTest, ContentTest_OnlyHasValidKeyNewline_Passes)
    {
        AZStd::fixed_string<AzFramework::ProjectManager::MaxBootstrapFileSize> testContent =
            R"(sys_game_folder=SamplesProject
)";

        EXPECT_TRUE(AzFramework::ProjectManager::ContentHasProjectName(testContent));
    }

    TEST_F(ProjectManagerTest, ContentTest_EmptyContent_Failse)
    {
        AZStd::fixed_string<AzFramework::ProjectManager::MaxBootstrapFileSize> testContent;

        EXPECT_FALSE(AzFramework::ProjectManager::ContentHasProjectName(testContent));
    }
#if AZ_TRAIT_DISABLE_FAILED_FRAMEWORK_TESTS
    TEST_F(ProjectManagerBootstrapTest, DISABLED_BootstrapContentTest_HasValidProjectName_Passes)
#else
    TEST_F(ProjectManagerBootstrapTest, BootstrapContentTest_HasValidProjectName_Passes)
#endif // AZ_TRAIT_DISABLE_FAILED_FRAMEWORK_TESTS
    {
        AZStd::fixed_string<AzFramework::ProjectManager::MaxBootstrapFileSize> testContent =
            R"(-- When you see an option that does not have a platform preceeding it, that is the default
--value for anything not specificly set per platform.So if remote_filesystem = 0 and you have
-- ios_remote_file_system = 1 then remote filesystem will be off for all platforms except ios
-- Any of the settings in this file can be prefixed with a platform name :
--android, ios, mac, linux, windows, etc...
-- or left unprefixed, to set all platforms not specified.The rules apply in the order they're declared

sys_game_folder=SamplesProject

-- remote_filesystem - enable Virtual File System(VFS)
--This feature allows a remote instance of the game to run off assets
-- on the asset processor computers cache instead of deploying them the remote device
-- By default it is offand can be overridden for any platform)";

        AZ::IO::SystemFile bootstrapFile;
        AZ::IO::FixedMaxPath bootstrapPath(m_tempDir.GetDirectory());
        bootstrapPath /= "bootstrap.cfg";
        bootstrapFile.Open(bootstrapPath.c_str(), AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY | AZ::IO::SystemFile::SF_OPEN_CREATE);

        bootstrapFile.Write(testContent.data(), testContent.length());
        bootstrapFile.Close();
        EXPECT_TRUE(AzFramework::ProjectManager::HasBootstrapProjectName(m_projectPath.c_str()));
    }

#if AZ_TRAIT_DISABLE_FAILED_FRAMEWORK_TESTS
    TEST_F(ProjectManagerBootstrapTest, DISABLED_BootstrapContentTest_HasNoValidProjectName_Fails)
#else
    TEST_F(ProjectManagerBootstrapTest, BootstrapContentTest_HasNoValidProjectName_Fails)
#endif // AZ_TRAIT_DISABLE_FAILED_FRAMEWORK_TESTS
    {
        AZStd::fixed_string<AzFramework::ProjectManager::MaxBootstrapFileSize> testContent =
            R"(-- When you see an option that does not have a platform preceeding it, that is the default
--value for anything not specificly set per platform.So if remote_filesystem = 0 and you have
-- ios_remote_file_system = 1 then remote filesystem will be off for all platforms except ios
-- Any of the settings in this file can be prefixed with a platform name :
--android, ios, mac, linux, windows, etc...
-- or left unprefixed, to set all platforms not specified.The rules apply in the order they're declared

sys_game_folder=

-- remote_filesystem - enable Virtual File System(VFS)
--This feature allows a remote instance of the game to run off assets
-- on the asset processor computers cache instead of deploying them the remote device
-- By default it is offand can be overridden for any platform)";

        AZ::IO::SystemFile bootstrapFile;
        AZ::IO::FixedMaxPath bootstrapPath(m_tempDir.GetDirectory());
        bootstrapPath /= "bootstrap.cfg";
        bootstrapFile.Open(bootstrapPath.c_str(), AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY | AZ::IO::SystemFile::SF_OPEN_CREATE);

        bootstrapFile.Write(testContent.data(), testContent.length());
        bootstrapFile.Close();
        EXPECT_FALSE(AzFramework::ProjectManager::HasBootstrapProjectName(m_projectPath.c_str()));
    }
} // ProjectManagerTests
