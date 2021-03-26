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
    AZ::AllocatorInstance<AZ::OSAllocator>::Create();
    AZ::AllocatorInstance<AZ::SystemAllocator>::Create();

    {
        AzFramework::CommandLine commandLine;

        commandLine.Parse(argc, argv);
        OutputArgs(commandLine);
    }

    AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
    AZ::AllocatorInstance<AZ::OSAllocator>::Destroy();
    return 0;
}
