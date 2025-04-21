/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <iostream>
#include <AzCore/std/utility/charconv.h>
#include <AzFramework/CommandLine/CommandLine.h>

// this is an intentional relative local include so that it can be shared between two unrelated projects
// namely this one shot test executable, and the tests which run it in a different project with no relation.
#include "ProcessLaunchTestTokens.h"

void OutputArgs(const AzFramework::CommandLine& commandLine)
{
    std::cout << "Switch List:" << std::endl;
    for (auto& switchPair : commandLine)
    {
        // We strip white space from all of our switch names, so "flush" names will start arguments
        if (!switchPair.m_option.empty())
        {
            std::cout << switchPair.m_option.c_str() << std::endl;
        }
        std::cout << " " << switchPair.m_value.c_str() << std::endl;
        
    }
    std::cout << "End Switch List:" << std::endl;
}

int main(int argc, char** argv)
{
    using namespace ProcessLaunchTestInternal;

    const AZ::Debug::Trace tracer;
    int exitCode = 0;
    {
        AzFramework::CommandLine commandLine;

        commandLine.Parse(argc, argv);

        if (commandLine.GetNumSwitchValues("exitCode") > 0)
        {
            exitCode = atoi(commandLine.GetSwitchValue("exitCode").c_str());
            const AZStd::string& exitCodeStr = commandLine.GetSwitchValue("exitCode");
            AZStd::from_chars(exitCodeStr.begin(), exitCodeStr.end(), exitCode);
        }

        if (commandLine.HasSwitch("plentyOfOutput"))
        {
            // special case - plenty of output mode requires that it output an easily recognizable begin
            // then plentiful output (enough to saturate any buffers) then output a recognizable end.
            // this makes sure that if there are any buffers that get filled up or overwritten we can detect
            // deadlock or buffer starvation or buffer overflows.
            size_t midLength = strlen(s_midToken);
            size_t beginLength = strlen(s_beginToken);
            size_t endLength = strlen(s_endToken);
            auto spamABunchOfLines = [&](FILE *fileDescriptor)
            {
                fwrite(s_beginToken, beginLength, 1, fileDescriptor);
                size_t currentBytesOutput = beginLength;
                while (currentBytesOutput < s_numOutputBytesInPlentyMode - endLength)
                {
                    fwrite(s_midToken, midLength, 1, fileDescriptor);
                    currentBytesOutput += midLength;
                }
                fwrite(s_endToken, endLength, 1, fileDescriptor);
            };

            spamABunchOfLines(stdout);

            if (exitCode != 0)
            {
                // note that stderr is unbuffered so this will take longer!
                spamABunchOfLines(stderr);
            }
        }
        else
        {
            OutputArgs(commandLine);
        }
        
    }
    return exitCode;
}
