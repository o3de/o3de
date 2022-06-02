/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/Framework/ScriptCanvasUnitTestFixture.h>
#include <Libraries/Math/CRC.h>

namespace ScriptCanvasUnitTest
{
    using namespace ScriptCanvas;

    using ScriptCanvasUnitTestCRCFunctions = ScriptCanvasUnitTestFixture;

    TEST_F(ScriptCanvasUnitTestCRCFunctions, FromString_Call_GetExpectedResult)
    {
        auto actualResult = CRCFunctions::FromString("dummy");
        EXPECT_EQ(actualResult, AZ::Crc32("dummy"));
    }
} // namespace ScriptCanvasUnitTest
