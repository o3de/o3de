/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/Math/Matrix3x4.h>
#include <SceneAPI/SceneCore/Utilities/CoordinateSystemConverter.h>

namespace EMotionFX
{
    const AZ::Vector3 sourceBasisVectors[3] = {
        AZ::Vector3(1.0f, 0.0f, 0.0f),
        AZ::Vector3(0.0f, 1.0f, 0.0f),
        AZ::Vector3(0.0f, 0.0f, 1.0f),
    };
    const AZ::Vector3 targetBasisVectors[3] = {
        AZ::Vector3(-1.0f, 0.0f, 0.0f),
        AZ::Vector3(0.0f, 1.0f, 0.0f),
        AZ::Vector3(0.0f, 0.0f, -1.0f),
    };
    const AZ::u32 targetBasisIndices[3] = { 0, 1, 2 };

    TEST(CoordinateSystemConverterTests, TransformsCorrectlyCreatedFromBasisVectors)
    {
        AZ::SceneAPI::CoordinateSystemConverter coordinateSystemConverter =
            AZ::SceneAPI::CoordinateSystemConverter::CreateFromBasisVectors(sourceBasisVectors, targetBasisVectors, targetBasisIndices);

        EXPECT_TRUE(coordinateSystemConverter.GetSourceTransform().GetBasisX().IsClose(sourceBasisVectors[0]));
        EXPECT_TRUE(coordinateSystemConverter.GetSourceTransform().GetBasisY().IsClose(sourceBasisVectors[1]));
        EXPECT_TRUE(coordinateSystemConverter.GetSourceTransform().GetBasisZ().IsClose(sourceBasisVectors[2]));
        EXPECT_TRUE(coordinateSystemConverter.GetSourceTransform().GetTranslation().IsClose(AZ::Vector3::CreateZero()));
        EXPECT_TRUE(coordinateSystemConverter.GetTargetTransform().GetBasisX().IsClose(targetBasisVectors[0]));
        EXPECT_TRUE(coordinateSystemConverter.GetTargetTransform().GetBasisY().IsClose(targetBasisVectors[1]));
        EXPECT_TRUE(coordinateSystemConverter.GetTargetTransform().GetBasisZ().IsClose(targetBasisVectors[2]));
        EXPECT_TRUE(coordinateSystemConverter.GetTargetTransform().GetTranslation().IsClose(AZ::Vector3::CreateZero()));
    }

    TEST(CoordinateSystemConverterTests, ConverterSimpleRotation)
    {
        AZ::SceneAPI::CoordinateSystemConverter coordinateSystemConverter =
            AZ::SceneAPI::CoordinateSystemConverter::CreateFromBasisVectors(sourceBasisVectors, targetBasisVectors, targetBasisIndices);

        const AZ::Matrix3x4 testMatrix = AZ::Matrix3x4::CreateFromQuaternion(AZ::Quaternion(1.0f, 0.0f, 0.0f, 0.0f));
        const AZ::Transform testTransform = AZ::Transform::CreateFromMatrix3x4(testMatrix);
        const AZ::Transform convertedTransform = coordinateSystemConverter.ConvertTransform(testTransform);

        EXPECT_TRUE(convertedTransform.GetRotation().IsClose(AZ::Quaternion(0.0f, 0.0f, 1.0f, 0.0f)));
    }
} // namespace EMotionFX

