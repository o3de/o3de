/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Process/ProcessWatcher.h>
#include <AzFramework/Process/ProcessCommunicator.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/IO/Path/Path.h>

// this is an intentional relative local include so that it can be shared between two unrelated projects
// namely this one test file as well as the one shot executable that these tests invoke.
#include "ProcessLaunchTestTokens.h"

namespace UnitTest
{
    class ProcessLaunchParseTests
        : public LeakDetectionFixture
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
        AZ::StringFunc::Tokenize(processOutput.c_str(), parsedLines, "\r\n");

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

        processLaunchInfo.m_commandlineParameters.emplace<AZStd::string>((AZ::IO::Path(AZ::Test::GetCurrentExecutablePath()) / "ProcessLaunchTest").Native());

        processLaunchInfo.m_workingDirectory = AZ::Test::GetCurrentExecutablePath();
        processLaunchInfo.m_showWindow = false;
        bool launchReturn = AzFramework::ProcessWatcher::LaunchProcessAndRetrieveOutput(processLaunchInfo, AzFramework::ProcessCommunicationType::COMMUNICATOR_TYPE_STDINOUT, processOutput);

        EXPECT_EQ(launchReturn, true);
        EXPECT_EQ(processOutput.outputResult.empty(), false);
    }

    TEST_F(ProcessLaunchParseTests, LaunchProcessAndRetrieveOutput_LargeDataNoError_ContainsEntireOutput)
    {
        using namespace ProcessLaunchTestInternal;

        AzFramework::ProcessOutput processOutput;
        AzFramework::ProcessLauncher::ProcessLaunchInfo processLaunchInfo;

        processLaunchInfo.m_commandlineParameters.emplace<AZStd::vector<AZStd::string>>(
            AZStd::vector<AZStd::string>{(AZ::IO::Path(AZ::Test::GetCurrentExecutablePath()) / "ProcessLaunchTest").Native(), "-plentyOfOutput"});

        processLaunchInfo.m_workingDirectory = AZ::Test::GetCurrentExecutablePath();
        processLaunchInfo.m_showWindow = false;
        bool launchReturn = AzFramework::ProcessWatcher::LaunchProcessAndRetrieveOutput(processLaunchInfo, AzFramework::ProcessCommunicationType::COMMUNICATOR_TYPE_STDINOUT, processOutput);

        EXPECT_TRUE(launchReturn);
        EXPECT_FALSE(processOutput.outputResult.empty());
        EXPECT_FALSE(processOutput.HasError());

        const auto& output = processOutput.outputResult;
        EXPECT_EQ(output.length(), s_numOutputBytesInPlentyMode);
        EXPECT_TRUE(output.starts_with(s_beginToken));
        EXPECT_TRUE(output.ends_with(s_endToken));
    }
    
    // this also tests that it can separately read error and stdout, and that the stream way of doing it works.
    TEST_F(ProcessLaunchParseTests, LaunchProcessAndRetrieveOutput_LargeDataWithError_ContainsEntireOutput)
    {
        using namespace ProcessLaunchTestInternal;

        AzFramework::ProcessOutput processOutput;
        AzFramework::ProcessLauncher::ProcessLaunchInfo processLaunchInfo;

        processLaunchInfo.m_commandlineParameters.emplace<AZStd::vector<AZStd::string>>(
            AZStd::vector<AZStd::string>{(AZ::IO::Path(AZ::Test::GetCurrentExecutablePath()) / "ProcessLaunchTest").Native(), "-plentyOfOutput", "-exitCode", "1"});

        processLaunchInfo.m_workingDirectory = AZ::Test::GetCurrentExecutablePath();
        processLaunchInfo.m_showWindow = false;
        bool launchReturn = AzFramework::ProcessWatcher::LaunchProcessAndRetrieveOutput(processLaunchInfo, AzFramework::ProcessCommunicationType::COMMUNICATOR_TYPE_STDINOUT, processOutput);

        EXPECT_TRUE(launchReturn);
        EXPECT_TRUE(processOutput.HasOutput());
        EXPECT_TRUE(processOutput.HasError());

        EXPECT_EQ(processOutput.outputResult.length(), s_numOutputBytesInPlentyMode);
        EXPECT_TRUE(processOutput.outputResult.starts_with(s_beginToken));
        EXPECT_TRUE(processOutput.outputResult.ends_with(s_endToken));

        EXPECT_EQ(processOutput.errorResult.length(), s_numOutputBytesInPlentyMode);
        EXPECT_TRUE(processOutput.errorResult.starts_with(s_beginToken));
        EXPECT_TRUE(processOutput.errorResult.ends_with(s_endToken));
    }
   
    TEST_F(ProcessLaunchParseTests, ProcessLauncher_BasicParameter_Success)
    {
        ProcessLaunchParseTests::ParsedArgMap argMap;
        AzFramework::ProcessOutput processOutput;
        AzFramework::ProcessLauncher::ProcessLaunchInfo processLaunchInfo;

        processLaunchInfo.m_commandlineParameters.emplace<AZStd::vector<AZStd::string>>(
            AZStd::vector<AZStd::string>{(AZ::IO::Path(AZ::Test::GetCurrentExecutablePath()) / "ProcessLaunchTest").Native(), "-param1", "param1val", "-param2", "param2val"});

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

        processLaunchInfo.m_commandlineParameters.emplace<AZStd::vector<AZStd::string>>(
            AZStd::vector<AZStd::string>{(AZ::IO::Path(AZ::Test::GetCurrentExecutablePath()) / "ProcessLaunchTest").Native(), "-param1", "param,1val", "-param2", "param2v,al"});

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

        processLaunchInfo.m_commandlineParameters.emplace<AZStd::vector<AZStd::string>>(AZStd::vector<AZStd::string>{
            (AZ::IO::Path(AZ::Test::GetCurrentExecutablePath()) / "ProcessLaunchTest").Native(), "-param1", R"("param 1val")", R"(-param2="param2v al")" });

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

        processLaunchInfo.m_commandlineParameters.emplace<AZStd::vector<AZStd::string>>(AZStd::vector<AZStd::string>{
            (AZ::IO::Path(AZ::Test::GetCurrentExecutablePath()) / "ProcessLaunchTest").Native(), "-param1", R"("param, 1val")", R"(-param2="param,2v al")" });

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

}   // namespace UnitTest
