/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Geometry3DUtils.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    class Geometry3DUtilsFixture :
        public UnitTest::AllocatorsFixture,
        public ::testing::WithParamInterface<uint8_t>
    {
    };

    TEST_P(Geometry3DUtilsFixture, IcoSphere_UnitRadius)
    {
        const AZStd::vector<AZ::Vector3> icoSphere = AZ::Geometry3dUtils::GenerateIcoSphere(GetParam());
        float minRadius = AZ::Constants::FloatMax;
        float maxRadius = 0;
        for (const auto& vertex : icoSphere)
        {
            const float radius = vertex.GetLength();
            minRadius = AZ::GetMin(minRadius, radius);
            maxRadius = AZ::GetMax(maxRadius, radius);
        }

        EXPECT_NEAR(minRadius, 1.0f, 1e-3f);
        EXPECT_NEAR(maxRadius, 1.0f, 1e-3f);
    }

    INSTANTIATE_TEST_CASE_P(MATH_Geometry3DUtilsTests, Geometry3DUtilsFixture, ::testing::Values(0u, 1u, 2u, 3u));
} // namespace UnitTest
