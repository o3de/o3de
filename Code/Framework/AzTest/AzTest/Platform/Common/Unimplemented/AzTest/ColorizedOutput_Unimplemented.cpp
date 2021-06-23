/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/base.h>
#include <gtest/gtest.h>

namespace UnitTest
{
    namespace Platform
    {
        void EnableVirtualConsoleProcessingForStdout()
        {
        }

        bool TerminalSupportsColor()
        {
            return false;
        }
    }
}
