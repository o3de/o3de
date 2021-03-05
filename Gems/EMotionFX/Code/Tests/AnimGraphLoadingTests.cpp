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

#include "AzCore/UnitTest/UnitTest.h"
#include <gtest/gtest.h>

#include <EMotionFX/Source/AnimGraph.h>
#include <Tests/SystemComponentFixture.h>

namespace EMotionFX
{
    TEST_F(SystemComponentFixture, CanHandleInvalidAnimGraphFile)
    {
        UnitTest::TraceBusRedirector redirector;
        const AZStd::string_view fileContents{R"(<ObjectStream version="3">
    <Class name="AnimGraph" version="1" type="{BD543125-CFEE-426C-B0AC-129F2A4C6BC8}">

    </Class>
</ObjectStream>

)"};
        AZ_TEST_START_TRACE_SUPPRESSION;
        AZStd::unique_ptr<EMotionFX::AnimGraph> animGraph {EMotionFX::AnimGraph::LoadFromBuffer(fileContents.data(), fileContents.size(), GetSerializeContext())};
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_FALSE(animGraph);
    }
} // namespace EMotionFX
