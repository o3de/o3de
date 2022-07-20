/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/Application/Application.h>

namespace TestImpact
{
    class TestProcess
    {
    public:
        TestProcess(int argc, char* argv[]);
        ~TestProcess();

        int MainFunc();

    private:
        void StartupEnvironment();
        void TeardownEnvironment();
        void ParseArgs(int argc, char* argv[]);

        int m_id;
        int m_sleep;
        bool m_dumpLargeText = false;
    };

} // namespace TestImpact
