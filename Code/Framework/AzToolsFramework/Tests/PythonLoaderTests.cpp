/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Component/Entity.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/sort.h>
#include <AzTest/AzTest.h>
#include <AzTest/Utils.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzToolsFramework/API/PythonLoader.h>

namespace UnitTest
{
    class AzToolsFrameworkPythonLoaderFixture
        : public LeakDetectionFixture
    {
    protected:
        AZ::Test::ScopedAutoTempDirectory m_tempDirectory;

        static constexpr AZStd::string_view s_testEnginePath = "O3de_path";
        static constexpr const char* s_testEnginePathHashId = "1af80774";
        static constexpr const char* s_test3rdPartySubPath = ".o3de/3rdParty";
    };

    TEST_F(AzToolsFrameworkPythonLoaderFixture, TestGetPythonVenvPath_Valid)
    {
        auto test3rdPartyPath = m_tempDirectory.GetDirectoryAsFixedMaxPath() / s_test3rdPartySubPath;
        auto testVenvPath = test3rdPartyPath / "venv";

        AZ::IO::FixedMaxPath engineRoot{ s_testEnginePath };

        AZ::IO::SystemFile::CreateDir(test3rdPartyPath.String().c_str());
        //AZ::IO::FileIOBase::GetInstance()->CreatePath(test3rdPartyPath.String().c_str());

        auto result = AzToolsFramework::EmbeddedPython::PythonLoader::GetPythonVenvPath(test3rdPartyPath, engineRoot);

        AZ::IO::FixedMaxPath expectedPath = testVenvPath / s_testEnginePathHashId;

        EXPECT_TRUE(result == expectedPath);
    }

    TEST_F(AzToolsFrameworkPythonLoaderFixture, TestGetPythonVenvExecutablePath_Valid)
    {
        // Prepare the test venv pyvenv.cfg file in the expected location
        AZ::IO::FixedMaxPath tempVenvRelativePath = AZ::IO::FixedMaxPath(s_test3rdPartySubPath) / "venv/" / s_testEnginePathHashId;
        AZ::IO::FixedMaxPath tempVenvFullPath = m_tempDirectory.GetDirectoryAsFixedMaxPath() / tempVenvRelativePath;
        AZ::IO::SystemFile::CreateDir(tempVenvFullPath.String().c_str());
        AZ::IO::FixedMaxPath tempPyConfigFile = tempVenvRelativePath / "pyvenv.cfg";
        AZStd::string testPython3rdPartyPath = "/home/user/python/";
        AZStd::string testPyConfigFileContent = AZStd::string::format("home = %s\n"
                                                                      "include-system-site-packages = false\n"
                                                                      "version = 3.10.13",
                                                                      testPython3rdPartyPath.c_str());
        AZ::Test::CreateTestFile(m_tempDirectory, tempPyConfigFile, testPyConfigFileContent);


        // Test the method
        AZ::IO::FixedMaxPath engineRoot{ s_testEnginePath };
        AZ::IO::FixedMaxPath test3rdPartyRoot = m_tempDirectory.GetDirectoryAsFixedMaxPath() / s_test3rdPartySubPath;

        auto result = AzToolsFramework::EmbeddedPython::PythonLoader::GetPythonExecutablePath(test3rdPartyRoot, engineRoot);
        AZ::IO::FixedMaxPath expectedPath{ testPython3rdPartyPath };

        EXPECT_TRUE(result == expectedPath);
    }

    TEST_F(AzToolsFrameworkPythonLoaderFixture, TestReadPythonEggLinkPaths_Valid)
    {
        // Prepare the test folder and create dummy egg-link files
        AZ::IO::FixedMaxPath testRelativeSiteLibsPath = AZ::IO::FixedMaxPath(s_test3rdPartySubPath) / "venv" / s_testEnginePathHashId / O3DE_PYTHON_SITE_PACKAGE_SUBPATH;
        AZ::IO::FixedMaxPath testFullSiteLIbsPath = m_tempDirectory.GetDirectoryAsFixedMaxPath() / testRelativeSiteLibsPath;
        AZ::IO::SystemFile::CreateDir(testFullSiteLIbsPath.String().c_str());

        AZStd::vector<AZStd::string> expectedResults;
        expectedResults.emplace_back(testFullSiteLIbsPath.LexicallyNormal().Native());

        static constexpr auto testEggLinkPaths = AZStd::to_array<const char*>({ "/lib/path/one", "/lib/path/two", "/lib/path/three" });
        int index = 0;
        for (const char* testEggLinkPath : testEggLinkPaths)
        {
            ++index;
            AZStd::string testEggFileName = AZStd::string::format("test-%d.egg-link", index);
            const char* lineBreak = ((index % 2) == 0) ? "\n" : "\r\n";
            AZStd::string testEggFileContent = AZStd::string::format("%s%s.", testEggLinkPath, lineBreak);
            expectedResults.emplace_back(AZStd::string(testEggLinkPath));

            AZ::IO::FixedMaxPath testEggLinkNamePath = testRelativeSiteLibsPath / testEggFileName;
            AZ::Test::CreateTestFile(m_tempDirectory, testEggLinkNamePath, testEggFileContent);
        }

        // Test the method
        AZ::IO::FixedMaxPath engineRoot{ s_testEnginePath };
        AZ::IO::FixedMaxPath test3rdPartyRoot = m_tempDirectory.GetDirectoryAsFixedMaxPath() / s_test3rdPartySubPath;
        AZStd::vector<AZStd::string> resultEggLinkPaths;
        AzToolsFramework::EmbeddedPython::PythonLoader::ReadPythonEggLinkPaths(
            test3rdPartyRoot,
            engineRoot,
            [&resultEggLinkPaths](AZ::IO::PathView path)
            {
                resultEggLinkPaths.emplace_back(path.Native());
            } );

        // Sort the expected and actual lists
        AZStd::sort(expectedResults.begin(), expectedResults.end());
        AZStd::sort(resultEggLinkPaths.begin(), resultEggLinkPaths.end());

        EXPECT_EQ(expectedResults, resultEggLinkPaths);
    }

} // namespace UnitTest
