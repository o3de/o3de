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

#include <AzTest/AzTest.h>
#include <EMotionFX/Pipeline/RCExt/CoordinateSystemConverter.h>

namespace EMotionFX
{
    TEST(CoordinateSystemConverterTests, TransformsCorrectlyCreatedFromBasisVectors)
    {
        const AZ::Vector3 sourceBasisVectors[3] = {
            AZ::Vector3(0.2544f, 0.224f, 0.9408f),
            AZ::Vector3(0.9408f, 0.168f, -0.2944f),
            AZ::Vector3(-0.224f, 0.96f, -0.168f),
        };
        const AZ::Vector3 targetBasisVectors[3] = {
            AZ::Vector3(0.5616f, 0.7488f, 0.352f),
            AZ::Vector3(-0.2112f, -0.2816f, 0.936f),
            AZ::Vector3(0.8f, -0.6f, 0.0f),
        };
        const AZ::u32 targetBasisIndices[3] = { 0, 1, 2 };
        Pipeline::CoordinateSystemConverter coordinateSystemConverter =
            Pipeline::CoordinateSystemConverter::CreateFromBasisVectors(sourceBasisVectors, targetBasisVectors, targetBasisIndices);

        EXPECT_TRUE(coordinateSystemConverter.GetSourceTransform().GetBasisX().IsClose(sourceBasisVectors[0]));
        EXPECT_TRUE(coordinateSystemConverter.GetSourceTransform().GetBasisY().IsClose(sourceBasisVectors[1]));
        EXPECT_TRUE(coordinateSystemConverter.GetSourceTransform().GetBasisZ().IsClose(sourceBasisVectors[2]));
        EXPECT_TRUE(coordinateSystemConverter.GetSourceTransform().GetTranslation().IsClose(AZ::Vector3::CreateZero()));
        EXPECT_TRUE(coordinateSystemConverter.GetTargetTransform().GetBasisX().IsClose(targetBasisVectors[0]));
        EXPECT_TRUE(coordinateSystemConverter.GetTargetTransform().GetBasisY().IsClose(targetBasisVectors[1]));
        EXPECT_TRUE(coordinateSystemConverter.GetTargetTransform().GetBasisZ().IsClose(targetBasisVectors[2]));
        EXPECT_TRUE(coordinateSystemConverter.GetTargetTransform().GetTranslation().IsClose(AZ::Vector3::CreateZero()));
    }
} // namespace EMotionFX

