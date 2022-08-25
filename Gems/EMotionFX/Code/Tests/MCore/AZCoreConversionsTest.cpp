/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AZTestShared/Math/MathTestHelpers.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Vector3.h>
#include <AzTest/AzTest.h>
#include <MCore/Source/AzCoreConversions.h>

namespace MCore
{
    struct EulerTestArgs {
        AZ::Vector3 eular;
        AZ::Quaternion result;
    };
    
    using AngleRadianTestFixtureXYZ = ::testing::TestWithParam<EulerTestArgs>;

    TEST_P(AngleRadianTestFixtureXYZ, AzEulerAnglesToAzQuat) {
        auto& param = GetParam();
        EXPECT_THAT(MCore::AzEulerAnglesToAzQuat(param.eular.GetX(),param.eular.GetY(),param.eular.GetZ()), UnitTest::IsClose(param.result));
    }

    // same test cases in QuaternionTests.cpp AngleRadianTestFixtureZYX
    INSTANTIATE_TEST_CASE_P(
        MATH_AZCoreConversions,
        AngleRadianTestFixtureXYZ,
        ::testing::Values(
            EulerTestArgs{ AZ::Vector3(AZ::Constants::QuarterPi, 0, 0), AZ::Quaternion::CreateRotationX(AZ::Constants::QuarterPi) },
            EulerTestArgs{ AZ::Vector3(0, AZ::Constants::QuarterPi, 0), AZ::Quaternion::CreateRotationY(AZ::Constants::QuarterPi) },
            EulerTestArgs{ AZ::Vector3(0, 0, AZ::Constants::QuarterPi), AZ::Quaternion::CreateRotationZ(AZ::Constants::QuarterPi) },

            EulerTestArgs{ AZ::Vector3(-AZ::Constants::QuarterPi, 0, 0), AZ::Quaternion::CreateRotationX(-AZ::Constants::QuarterPi) },
            EulerTestArgs{ AZ::Vector3(0, -AZ::Constants::QuarterPi, 0), AZ::Quaternion::CreateRotationY(-AZ::Constants::QuarterPi) },
            EulerTestArgs{ AZ::Vector3(0, 0, -AZ::Constants::QuarterPi), AZ::Quaternion::CreateRotationZ(-AZ::Constants::QuarterPi) },

            EulerTestArgs{ AZ::Vector3(AZ::Constants::QuarterPi,AZ::Constants::QuarterPi, 0), AZ::Quaternion::CreateRotationY(AZ::Constants::QuarterPi) *
                AZ::Quaternion::CreateRotationX(AZ::Constants::QuarterPi) },
            EulerTestArgs{ AZ::Vector3(0,AZ::Constants::QuarterPi,AZ::Constants::QuarterPi), AZ::Quaternion::CreateRotationZ(AZ::Constants::QuarterPi) *
                AZ::Quaternion::CreateRotationY(AZ::Constants::QuarterPi) },
            EulerTestArgs{ AZ::Vector3(AZ::Constants::QuarterPi, 0,AZ::Constants::QuarterPi), AZ::Quaternion::CreateRotationZ(AZ::Constants::QuarterPi) *
                AZ::Quaternion::CreateRotationX(AZ::Constants::QuarterPi)},
                
            EulerTestArgs{ AZ::Vector3(AZ::Constants::HalfPi, 0,AZ::Constants::QuarterPi), 
                AZ::Quaternion::CreateRotationZ(AZ::Constants::QuarterPi) *
                AZ::Quaternion::CreateRotationX(AZ::Constants::HalfPi)},
            EulerTestArgs{ AZ::Vector3(-AZ::Constants::QuarterPi, -AZ::Constants::HalfPi,AZ::Constants::QuarterPi), 
                AZ::Quaternion::CreateRotationZ(AZ::Constants::QuarterPi) *
                AZ::Quaternion::CreateRotationY(-AZ::Constants::HalfPi) *
                AZ::Quaternion::CreateRotationX(-AZ::Constants::QuarterPi)},
            EulerTestArgs{ AZ::Vector3(-AZ::Constants::QuarterPi,AZ::Constants::HalfPi,AZ::Constants::TwoOverPi), 
                AZ::Quaternion::CreateRotationZ(AZ::Constants::TwoOverPi) *
                AZ::Quaternion::CreateRotationY(AZ::Constants::HalfPi) *
                AZ::Quaternion::CreateRotationX(-AZ::Constants::QuarterPi)}
        ));

} // namespace MCore
