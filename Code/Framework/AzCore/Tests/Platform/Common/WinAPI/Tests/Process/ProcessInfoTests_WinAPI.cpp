/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Process/ProcessInfo.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    class ProcessInfoTestFixture
        : public LeakDetectionFixture
    {
    };

    TEST_F(ProcessInfoTestFixture, QueryMemInfo_Succeeds)
    {
        AZ::ProcessMemInfo memInfo;
        ASSERT_TRUE(AZ::QueryMemInfo(memInfo));
        // WinAPI populates all 5 fields of ProcessMemInfo
        EXPECT_GT(memInfo.m_workingSet, 0);
        EXPECT_GT(memInfo.m_peakWorkingSet, 0);
        EXPECT_GT(memInfo.m_pagefileUsage, 0);
        EXPECT_GT(memInfo.m_peakPagefileUsage, 0);
        EXPECT_GT(memInfo.m_pageFaultCount, 0);
    }
}
