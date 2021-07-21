/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
