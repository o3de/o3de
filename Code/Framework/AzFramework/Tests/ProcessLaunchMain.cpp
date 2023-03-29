/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/AllocatorManager.h>
#include <AzFramework/CommandLine/CommandLine.h>

#include <iostream>

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
    const AZ::Debug::Trace tracer;
    {
        AzFramework::CommandLine commandLine;

        commandLine.Parse(argc, argv);
        OutputArgs(commandLine);
    }
    return 0;
}
