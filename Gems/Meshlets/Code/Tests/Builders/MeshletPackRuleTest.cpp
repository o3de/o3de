/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <AzTest/AzTest.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Builders/MeshletPackRule.h>

namespace UnitTest
{
    using AZ::Meshlets::Builders::MeshletPackRule;

    TEST(MeshletPackRule, DefaultsMatchSpec)
    {
        // 128/256, not the SP1-era 64/64: the defaults were widened when the
        // mesh-shader path landed (bigger clusters => fewer per-cluster draw commands
        // and better vertex-cache reuse). This test had never run -- the module had no
        // AZ_UNIT_TEST_HOOK -- so it kept asserting the original values unnoticed.
        MeshletPackRule r;
        EXPECT_EQ(128u, r.GetMaxVerticesPerCluster());
        EXPECT_EQ(256u, r.GetMaxTrianglesPerCluster());
        EXPECT_FLOAT_EQ(0.5f, r.GetConeWeight());
        EXPECT_TRUE(r.GetMeshFilter().empty());
    }

    TEST(MeshletPackRule, ReflectRegistersWithoutCrashing)
    {
        AZ::SerializeContext ctx;
        MeshletPackRule::Reflect(&ctx);
        // If we reach here without an assert, registration is OK.
        SUCCEED();
    }
}
