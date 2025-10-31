/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorDefs.h"
#include <AzTest/AzTest.h>
#include <Util/EditorUtils.h>
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace EditorUtilsTest
{
    class WarningDetector
        : public AZ::Debug::TraceMessageBus::Handler
    {
    public:
        WarningDetector()
        {
            BusConnect();
        }

        ~WarningDetector() override
        {
            BusDisconnect();
        }

        bool OnWarning(const char* /*window*/, const char* /*message*/) override 
        {
            m_gotWarning = true;
            return true;
        }

        bool m_gotWarning = false;
    };


    class TestWarningAbsorber
        : public ::UnitTest::LeakDetectionFixture
    {
    };

    TEST_F(TestWarningAbsorber, Test)
    {
        WarningDetector detector;
        EditorUtils::AzWarningAbsorber absorber("ignore this");

        AZ_Warning("ignore this", false, "This warning should occur but be absorbed by the absorber");
        ASSERT_FALSE(detector.m_gotWarning);

        AZ_Warning("different window", false, "This warning should occur but be left alone by the absorber");
        ASSERT_TRUE(detector.m_gotWarning);
    }

    TEST_F(TestWarningAbsorber, NullWindow)
    {
        WarningDetector detector;
        EditorUtils::AzWarningAbsorber absorber("ignore this");

        AZ_Warning(AZ::Debug::Trace::GetDefaultSystemWindow(), false, "This warning should occur and not be absorbed by the absorber since the window name is nullptr");
        ASSERT_TRUE(detector.m_gotWarning);
    }
}
