/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AZTestShared/Math/MathTestHelpers.h>

#include <UnitTestHelper.h>

#include <System/NvTypes.h>

namespace UnitTest
{
    TEST(NvClothSystem, NvTypes_AzVectorToNvRange_BeginAndEndPointersMatch)
    {
        const AZStd::vector<int> azVectorEmpty;
        const AZStd::vector<int> azVectorInts = {{ 5, 6, 4, 9, 514 }};
        const AZStd::vector<float> azVectorFloats = {{ 12.0f, 546.05f, -561461.68f, 654.0f }};

        auto nvRangeEmpty = NvCloth::ToNvRange(azVectorEmpty);
        auto nvRangeInts = NvCloth::ToNvRange(azVectorInts);
        auto nvRangeFloats = NvCloth::ToNvRange(azVectorFloats);

        EXPECT_EQ(nvRangeEmpty.begin(), azVectorEmpty.begin());
        EXPECT_EQ(nvRangeEmpty.end(), azVectorEmpty.end());
        EXPECT_EQ(nvRangeInts.begin(), azVectorInts.begin());
        EXPECT_EQ(nvRangeInts.end(), azVectorInts.end());
        EXPECT_EQ(nvRangeFloats.begin(), azVectorFloats.begin());
        EXPECT_EQ(nvRangeFloats.end(), azVectorFloats.end());
        ExpectEq(azVectorEmpty, nvRangeEmpty);
        ExpectEq(azVectorInts, nvRangeInts);
        ExpectEq(azVectorFloats, nvRangeFloats);
    }

    TEST(NvClothSystem, NvTypes_aAzVector4AzVectorToPxVec4NvRange_BeginAndEndPointersMatch)
    {
        const AZStd::vector<AZ::Vector4> azVectorEmpty;
        const AZStd::vector<AZ::Vector4> azVectorValues = {{
            AZ::Vector4(15.0f, -692.0f, 65.0f, -15.0f),
            AZ::Vector4(1851.594f, 1.0f, -125.0f, 168.0f),
            AZ::Vector4(2384.05f, -692.0f, 41865.153f, 1567.0f),
            AZ::Vector4(35.02f, 2572.453f, 2465.0f, 987.0f),
            AZ::Vector4(-14.161f, 47.0f, 65.0f, -6358.52f)
        }};

        nv::cloth::Range<const physx::PxVec4> nvRangeEmpty = NvCloth::ToPxVec4NvRange(azVectorEmpty);
        nv::cloth::Range<const physx::PxVec4> nvRangeValues = NvCloth::ToPxVec4NvRange(azVectorValues);

        EXPECT_EQ(reinterpret_cast<const AZ::Vector4*>(nvRangeEmpty.begin()), azVectorEmpty.begin());
        EXPECT_EQ(reinterpret_cast<const AZ::Vector4*>(nvRangeEmpty.end()), azVectorEmpty.end());
        EXPECT_EQ(reinterpret_cast<const AZ::Vector4*>(nvRangeValues.begin()), azVectorValues.begin());
        EXPECT_EQ(reinterpret_cast<const AZ::Vector4*>(nvRangeValues.end()), azVectorValues.end());
        ExpectEq(azVectorEmpty, nvRangeEmpty);
        ExpectEq(azVectorValues, nvRangeValues);
    }
} // namespace UnitTest
