/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"
#include <AzTest/AzTest.h>
#include <AzCore/Math/Matrix4x4.h>
#include <CryCommon/Cry_Matrix44.h>
#include <CryCommon/MathConversion.h>

namespace EditorUtilsTest
{
    class LegacryDeprecationHelper : public ::testing::Test
    {
    public:
        LegacryDeprecationHelper()
        {
        }
    };

    typedef float HMatrix[4][4]; /* Right-handed, for column vectors */

   TEST_F(LegacryDeprecationHelper, TestLegacyMaxtrix44_HMatrixConversion)
    {
       AZ_PUSH_DISABLE_WARNING(4996, "-Wdeprecated-declarations");

        // Validation for AffineParts::Decompose and AffineParts::SpectralDecompose
        Matrix34 tm(0.0f, 1.0f, 2.0f, 3.0f, 10.0f, 11.0f, 12.0f, 13.0f, 20.0f, 21.0f, 22.0f, 23.0f);

        // Legacy Base
        Matrix44 cryTm44(tm);
        HMatrix& legacyH = *((HMatrix*)&cryTm44); // Treat HMatrix as a Matrix44.

        // Updated AZ::Matrix4x4
        AZ::Matrix3x4 tm34 = LYTransformToAZMatrix3x4(tm);
        AZ::Matrix4x4 azTm44 = AZ::Matrix4x4::CreateFromMatrix3x4(tm34);
        HMatrix newH;
        azTm44.StoreToRowMajorFloat16((float*)&newH);

        // Validation
        for (int r = 0; r < 4; ++r)
        {
            for (int c = 0; c < 4; ++c)
            {
                ASSERT_FLOAT_EQ(legacyH[r][c], newH[r][c]);
            }
        }
        AZ_POP_DISABLE_WARNING;
    }

} // namespace EditorUtilsTest
