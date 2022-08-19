/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Libraries/ScriptCanvasNodeRegistry.h>
#include <Tests/Framework/ScriptCanvasUnitTestFixture.h>

namespace ScriptCanvasUnitTest
{
    using ScriptCanvasNodeRegistryTest = ScriptCanvasUnitTestFixture;

    TEST_F(ScriptCanvasNodeRegistryTest, GetInstance_Call_ExpectToBeValid)
    {
        auto nodeRegistry = ScriptCanvas::NodeRegistry::GetInstance();
        EXPECT_TRUE(nodeRegistry);
    }

    TEST_F(ScriptCanvasNodeRegistryTest, GetInstance_Call_ExpectToBeConsistent)
    {
        auto nodeRegistry1 = ScriptCanvas::NodeRegistry::GetInstance();
        auto nodeRegistry2 = ScriptCanvas::NodeRegistry::GetInstance();
        EXPECT_EQ(nodeRegistry1, nodeRegistry2);
    }
} // namespace ScriptCanvasUnitTest
