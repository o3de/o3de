/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TestImpactTestProcess.h"
#include "TestImpactTestProcessLargeText.h"

#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/Settings/CommandLine.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/string/string.h>

#include <iostream>

namespace TestImpact
{
    TestProcess::TestProcess(int argc, char* argv[])
    {
        StartupEnvironment();
        ParseArgs(argc, argv);
    }

    TestProcess::~TestProcess()
    {
        TeardownEnvironment();
    }

    void TestProcess::StartupEnvironment()
    {
        AZ::AllocatorInstance<AZ::SystemAllocator>::Create();
    }

    void TestProcess::TeardownEnvironment()
    {
        AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
    }

    void TestProcess::ParseArgs(int argc, char* argv[])
    {
        const AZStd::string idArg = "id";
        const AZStd::string sleepArg = "sleep";
        const AZStd::string largeArg = "large";

        AZ::CommandLine commandLine;
        commandLine.Parse(argc, argv);

        m_id = AZStd::stoi(commandLine.GetSwitchValue(idArg, 0));
        m_sleep = AZStd::stoi(commandLine.GetSwitchValue(sleepArg, 0));

        m_dumpLargeText = false;
        for (auto i = 0; i < commandLine.GetNumMiscValues(); i++)
        {
            const auto& miscValue = commandLine.GetMiscValue(i);
            if (miscValue == largeArg)
            {
                m_dumpLargeText = true;
                break;
            }
        }
    }

    int TestProcess::MainFunc()
    {
        if (m_dumpLargeText)
        {
            // Dump the large text blob to stdout and stderr
            std::cout << LongText;
            std::cerr << LongText;
        }
        else
        {
            // Dump the short known output string with id appended to stdout and stderr
            std::cout << "TestProcessMainStdOut" << m_id;
            std::cerr << "TestProcessMainStdErr" << m_id;
        }

        AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(m_sleep));

        return m_id;
    }
} // namespace TestImpact
