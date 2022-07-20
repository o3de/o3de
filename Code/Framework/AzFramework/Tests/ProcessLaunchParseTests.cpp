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

        processLaunchInfo.m_commandlineParameters.emplace<AZStd::string>(AZStd::string(AZ_TRAIT_TEST_ROOT_FOLDER "ProcessLaunchTest"));

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

        processLaunchInfo.m_commandlineParameters.emplace<AZStd::vector<AZStd::string>>(
            AZStd::vector<AZStd::string>{AZStd::string(AZ_TRAIT_TEST_ROOT_FOLDER "ProcessLaunchTest"), "-param1", "param1val","-param2", "param2val"});

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
            AZStd::vector<AZStd::string>{AZStd::string(AZ_TRAIT_TEST_ROOT_FOLDER "ProcessLaunchTest"), "-param1", "param,1val","-param2", "param2v,al"});

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
            AZStd::string(AZ_TRAIT_TEST_ROOT_FOLDER "ProcessLaunchTest"), "-param1", R"("param 1val")", R"(-param2="param2v al")" });

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
            AZStd::string(AZ_TRAIT_TEST_ROOT_FOLDER "ProcessLaunchTest"), "-param1", R"("param, 1val")", R"(-param2="param,2v al")" });

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
