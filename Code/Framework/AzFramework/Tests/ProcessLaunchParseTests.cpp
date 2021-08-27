/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/CommandLine/CommandLine.h>
#include <AzFramework/Process/ProcessWatcher.h>
#include <AzFramework/Process/ProcessCommunicator.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzFramework/StringFunc/StringFunc.h>

using namespace AzFramework;


namespace UnitTest
{
    class ProcessLaunchParseTests
        : public AllocatorsFixture
    {
    public:
        using ParsedArgMap = AZStd::unordered_map<AZStd::string, AZStd::vector<AZStd::string>>;
        static ParsedArgMap ParseParameters(const AZStd::string& processOutput);
    };

    ProcessLaunchParseTests::ParsedArgMap ProcessLaunchParseTests::ParseParameters(const AZStd::string& processOutput)
    {
        ParsedArgMap parsedArgs;

        AZStd::string currentSwitch;
        bool inSwitches{ false };
        AZStd::vector<AZStd::string> parsedLines;
        AzFramework::StringFunc::Tokenize(processOutput.c_str(), parsedLines, "\r\n");

        for (const AZStd::string& thisLine : parsedLines)
        {
            if (thisLine == "Switch List:")
            {
                inSwitches = true;
                continue;
            }
            else if (thisLine == "End Switch List:")
            {
                inSwitches = false;
                continue;
            }

            if (thisLine.empty())
            {
                continue;
            }

            if(inSwitches)
            {
                if (thisLine[0] != ' ')
                {
                    currentSwitch = thisLine;
                }
                else
                {
                    parsedArgs[currentSwitch].push_back(thisLine.substr(1));
                }
            }
        }
        return parsedArgs;
    }

    TEST_F(ProcessLaunchParseTests, ProcessLauncher_LaunchBasicProcess_Success)
    {
        AzFramework::ProcessOutput processOutput;
        AzFramework::ProcessLauncher::ProcessLaunchInfo processLaunchInfo;
        processLaunchInfo.m_processExecutableString = AZStd::string(AZ_TRAIT_TEST_ROOT_FOLDER "ProcessLaunchTest" AZ_TRAIT_OS_EXECUTABLE_EXTENSION);
        processLaunchInfo.m_workingDirectory = AZ::Test::GetCurrentExecutablePath();
        processLaunchInfo.m_showWindow = false;
        bool launchReturn = AzFramework::ProcessWatcher::LaunchProcessAndRetrieveOutput(processLaunchInfo, AzFramework::ProcessCommunicationType::COMMUNICATOR_TYPE_STDINOUT, processOutput);

        EXPECT_EQ(launchReturn, true);
        EXPECT_EQ(processOutput.outputResult.empty(), false);
    }

    TEST_F(ProcessLaunchParseTests, ProcessLauncher_BasicParameter_Success)
    {
        ProcessLaunchParseTests::ParsedArgMap argMap;
        AzFramework::ProcessOutput processOutput;
        AzFramework::ProcessLauncher::ProcessLaunchInfo processLaunchInfo;

        processLaunchInfo.m_processExecutableString = AZStd::string(AZ_TRAIT_TEST_ROOT_FOLDER "ProcessLaunchTest" AZ_TRAIT_OS_EXECUTABLE_EXTENSION);
        processLaunchInfo.m_commandlineParameters.emplace<AZStd::vector<AZStd::string>>(
            AZStd::vector<AZStd::string>{"-param1", "param1val","-param2", "param2val"});
        processLaunchInfo.m_workingDirectory = AZ::Test::GetCurrentExecutablePath();
        processLaunchInfo.m_showWindow = false;
        bool launchReturn = AzFramework::ProcessWatcher::LaunchProcessAndRetrieveOutput(processLaunchInfo, AzFramework::ProcessCommunicationType::COMMUNICATOR_TYPE_STDINOUT, processOutput);

        EXPECT_EQ(launchReturn, true);

        argMap = ProcessLaunchParseTests::ParseParameters(processOutput.outputResult);

        auto param1itr = argMap.find("param1");
        EXPECT_NE(param1itr, argMap.end());
        AZStd::vector<AZStd::string> param1{ param1itr->second };

        EXPECT_EQ(param1.size(), 1);
        EXPECT_EQ(param1[0], "param1val");

        auto param2itr = argMap.find("param2");
        EXPECT_NE(param2itr, argMap.end());
        AZStd::vector<AZStd::string> param2{ param2itr->second };

        EXPECT_EQ(param2.size(), 1);
        EXPECT_EQ(param2[0], "param2val");
    }

    TEST_F(ProcessLaunchParseTests, ProcessLauncher_WithCommas_Success)
    {
        ProcessLaunchParseTests::ParsedArgMap argMap;
        AzFramework::ProcessOutput processOutput;
        AzFramework::ProcessLauncher::ProcessLaunchInfo processLaunchInfo;
        processLaunchInfo.m_processExecutableString = AZStd::string(AZ_TRAIT_TEST_ROOT_FOLDER "ProcessLaunchTest" AZ_TRAIT_OS_EXECUTABLE_EXTENSION);
        processLaunchInfo.m_commandlineParameters.emplace<AZStd::vector<AZStd::string>>(
            AZStd::vector<AZStd::string>{"-param1", "param,1val","-param2", "param2v,al"});
        processLaunchInfo.m_workingDirectory = AZ::Test::GetCurrentExecutablePath();
        processLaunchInfo.m_showWindow = false;
        bool launchReturn = AzFramework::ProcessWatcher::LaunchProcessAndRetrieveOutput(processLaunchInfo, AzFramework::ProcessCommunicationType::COMMUNICATOR_TYPE_STDINOUT, processOutput);

        EXPECT_EQ(launchReturn, true);

        argMap = ProcessLaunchParseTests::ParseParameters(processOutput.outputResult);

        auto param1itr = argMap.find("param1");
        EXPECT_NE(param1itr, argMap.end());
        AZStd::vector<AZStd::string> param1{ param1itr->second };

        EXPECT_EQ(param1.size(), 2);
        EXPECT_EQ(param1[0], "param");
        EXPECT_EQ(param1[1], "1val");

        auto param2itr = argMap.find("param2");
        EXPECT_NE(param2itr, argMap.end());
        AZStd::vector<AZStd::string> param2{ param2itr->second };

        EXPECT_EQ(param2.size(), 2);
        EXPECT_EQ(param2[0], "param2v");
        EXPECT_EQ(param2[1], "al");
    }

    TEST_F(ProcessLaunchParseTests, ProcessLauncher_WithSpaces_Success)
    {
        ProcessLaunchParseTests::ParsedArgMap argMap;
        AzFramework::ProcessOutput processOutput;
        AzFramework::ProcessLauncher::ProcessLaunchInfo processLaunchInfo;
        processLaunchInfo.m_processExecutableString = AZStd::string(AZ_TRAIT_TEST_ROOT_FOLDER "ProcessLaunchTest" AZ_TRAIT_OS_EXECUTABLE_EXTENSION);
        processLaunchInfo.m_commandlineParameters.emplace<AZStd::vector<AZStd::string>>(
            AZStd::vector<AZStd::string>{"-param1", R"("param 1val")", "-param2", R"("param2v al")"});
        processLaunchInfo.m_workingDirectory = AZ::Test::GetCurrentExecutablePath();
        processLaunchInfo.m_showWindow = false;
        bool launchReturn = AzFramework::ProcessWatcher::LaunchProcessAndRetrieveOutput(processLaunchInfo, AzFramework::ProcessCommunicationType::COMMUNICATOR_TYPE_STDINOUT, processOutput);

        EXPECT_EQ(launchReturn, true);

        argMap = ProcessLaunchParseTests::ParseParameters(processOutput.outputResult);

        auto param1itr = argMap.find("param1");
        EXPECT_NE(param1itr, argMap.end());
        AZStd::vector<AZStd::string> param1{ param1itr->second };

        EXPECT_EQ(param1.size(), 1);
        EXPECT_EQ(param1[0], "param 1val");

        auto param2itr = argMap.find("param2");
        EXPECT_NE(param2itr, argMap.end());
        AZStd::vector<AZStd::string> param2{ param2itr->second };

        EXPECT_EQ(param2.size(), 1);
        EXPECT_EQ(param2[0], "param2v al");
    }

    TEST_F(ProcessLaunchParseTests, ProcessLauncher_WithSpacesAndComma_Success)
    {
        ProcessLaunchParseTests::ParsedArgMap argMap;
        AzFramework::ProcessOutput processOutput;
        AzFramework::ProcessLauncher::ProcessLaunchInfo processLaunchInfo;
        processLaunchInfo.m_processExecutableString = AZStd::string(AZ_TRAIT_TEST_ROOT_FOLDER "ProcessLaunchTest" AZ_TRAIT_OS_EXECUTABLE_EXTENSION);
        processLaunchInfo.m_commandlineParameters.emplace<AZStd::vector<AZStd::string>>(
            AZStd::vector<AZStd::string>{"-param1", R"("param, 1val")", "-param2", R"("param,2v al")"});
        processLaunchInfo.m_workingDirectory = AZ::Test::GetCurrentExecutablePath();
        processLaunchInfo.m_showWindow = false;
        bool launchReturn = AzFramework::ProcessWatcher::LaunchProcessAndRetrieveOutput(processLaunchInfo, AzFramework::ProcessCommunicationType::COMMUNICATOR_TYPE_STDINOUT, processOutput);

        EXPECT_EQ(launchReturn, true);

        argMap = ProcessLaunchParseTests::ParseParameters(processOutput.outputResult);

        auto param1itr = argMap.find("param1");
        EXPECT_NE(param1itr, argMap.end());
        AZStd::vector<AZStd::string> param1{ param1itr->second };

        EXPECT_EQ(param1.size(), 1);
        EXPECT_EQ(param1[0], "param, 1val");

        auto param2itr = argMap.find("param2");
        EXPECT_NE(param2itr, argMap.end());
        AZStd::vector<AZStd::string> param2{ param2itr->second };

        EXPECT_EQ(param2.size(), 1);
        EXPECT_EQ(param2[0], "param,2v al");
    }

    TEST_F(ProcessLaunchParseTests, ProcessLauncher_builder)
    {
        ProcessLaunchParseTests::ParsedArgMap argMap;
        AzFramework::ProcessOutput processOutput;
        AzFramework::ProcessLauncher::ProcessLaunchInfo processLaunchInfo;
        processLaunchInfo.m_processExecutableString = AZStd::string(AZ_TRAIT_TEST_ROOT_FOLDER "ProcessLaunchTest" AZ_TRAIT_OS_EXECUTABLE_EXTENSION);

        AZStd::vector<AZStd::string> params;
        params.emplace_back(AZStd::string::format("-task=%s", "resident"));
        params.emplace_back(AZStd::string::format(R"(-id="%s")", "53E98D3FFE3643FBB2A8B53F987F2731"));
        params.emplace_back(AZStd::string::format(R"(-project-name="%s")", "AutomatedTesting"));
        params.emplace_back(AZStd::string::format(R"(-project-cache-path="%s")", "c:/repo/o3de/AutomatedTesting/Cache"));
        params.emplace_back(AZStd::string::format(R"(-project-path="%s")", "c:\\repo\\o3de\\AutomatedTesting"));
        params.emplace_back(AZStd::string::format(R"(-engine-path="%s")", "C:/repo/o3de"));
        params.emplace_back(AZStd::string::format("-port=%d", 45643));
        params.emplace_back(AZStd::string::format(R"(-module="%s")", "C:\\repo\\build\\windows\\o3de\\msbuild\\debug\\bin\\debug\\Builders"));

        processLaunchInfo.m_commandlineParameters = params;
        processLaunchInfo.m_workingDirectory = AZ::Test::GetCurrentExecutablePath();
        processLaunchInfo.m_showWindow = false;
        bool launchReturn = AzFramework::ProcessWatcher::LaunchProcessAndRetrieveOutput(processLaunchInfo, AzFramework::ProcessCommunicationType::COMMUNICATOR_TYPE_STDINOUT, processOutput);

        EXPECT_EQ(launchReturn, true);

        argMap = ProcessLaunchParseTests::ParseParameters(processOutput.outputResult);

        auto taskItr = argMap.find("task");
        EXPECT_NE(taskItr, argMap.end());
        AZStd::vector<AZStd::string> task{ taskItr->second };

        EXPECT_EQ(task.size(), 1);
        EXPECT_EQ(task[0], "resident");

        auto idItr = argMap.find("id");
        EXPECT_NE(idItr, argMap.end());
        AZStd::vector<AZStd::string> id{ idItr->second };

        EXPECT_EQ(id.size(), 1);
        EXPECT_EQ(id[0], "53E98D3FFE3643FBB2A8B53F987F2731");

        auto project_nameItr = argMap.find("project-name");
        EXPECT_NE(project_nameItr, argMap.end());
        AZStd::vector<AZStd::string> project_name{ project_nameItr->second };

        EXPECT_EQ(project_name.size(), 1);
        EXPECT_EQ(project_name[0], "AutomatedTesting");

        auto project_cache_pathItr = argMap.find("project-cache-path");
        EXPECT_NE(project_cache_pathItr, argMap.end());
        AZStd::vector<AZStd::string> project_cache_path{ project_cache_pathItr->second };

        EXPECT_EQ(project_cache_path.size(), 1);
        EXPECT_EQ(project_cache_path[0], "c:/repo/o3de/AutomatedTesting/Cache");

        auto project_pathItr = argMap.find("project-path");
        EXPECT_NE(project_pathItr, argMap.end());
        AZStd::vector<AZStd::string> project_path{ project_pathItr->second };

        EXPECT_EQ(project_path.size(), 1);
        EXPECT_EQ(project_path[0], "c:\\repo\\o3de\\AutomatedTesting");

        auto engine_pathItr = argMap.find("engine-path");
        EXPECT_NE(engine_pathItr, argMap.end());
        AZStd::vector<AZStd::string> engine_path{ engine_pathItr->second };

        EXPECT_EQ(engine_path.size(), 1);
        EXPECT_EQ(engine_path[0], "C:/repo/o3de");

        auto portItr = argMap.find("port");
        EXPECT_NE(portItr, argMap.end());
        AZStd::vector<AZStd::string> port{ portItr->second };

        EXPECT_EQ(port.size(), 1);
        EXPECT_EQ(port[0], "45643");

        auto moduleItr = argMap.find("module");
        EXPECT_NE(moduleItr, argMap.end());
        AZStd::vector<AZStd::string> module{ moduleItr->second };

        EXPECT_EQ(module.size(), 1);
        EXPECT_EQ(module[0], "C:\\repo\\build\\windows\\o3de\\msbuild\\debug\\bin\\debug\\Builders");
    }

}   // namespace UnitTest
