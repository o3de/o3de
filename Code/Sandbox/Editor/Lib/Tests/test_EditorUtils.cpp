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
#include "EditorDefs.h"
#include <AzTest/AzTest.h>
#include <Util/EditorUtils.h>
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Debug/TraceMessageBus.h>

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

        ~WarningDetector()
        {
            BusDisconnect();
        }

        virtual bool OnWarning(const char* /*window*/, const char* /*message*/) override 
        {
            m_gotWarning = true;
            return true;
        }

        bool m_gotWarning = false;
    };


    class TestWarningAbsorber
        : public testing::Test
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

        AZ_Warning(nullptr, false, "This warning should occur and not be absorbed by the absorber since the window name is nullptr");
        ASSERT_TRUE(detector.m_gotWarning);
    }
}
