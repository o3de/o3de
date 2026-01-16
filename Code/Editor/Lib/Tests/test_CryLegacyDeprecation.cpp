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

   TEST_F(LegacryDeprecationHelper, TestLegacyMatrix44_HMatrixConversion)
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

    TEST_F(LegacryDeprecationHelper, TestLegacyMatrix33_CreateFromEulerAngles)
    {
        AZ_PUSH_DISABLE_WARNING(4996, "-Wdeprecated-declarations");

        // The legacy function Matrix33::CreateRotationXYZ is equivalent to the AZ::Matrix3x3 constructur with Quaternion argument created
        // by AZ::Quaternion::CreateFromEulerRadiansZYX
        Matrix33 legacyMatrix = Matrix33::CreateRotationXYZ(Ang3(1.f, 2.f, 3.f));
        AZ::Matrix3x3 newMatrix(AZ::Quaternion::CreateFromEulerRadiansZYX(AZ::Vector3(1.f, 2.f, 3.f)));

        // Validation
        for (int r = 0; r < 3; ++r)
        {
            for (int c = 0; c < 3; ++c)
            {
                ASSERT_NEAR(legacyMatrix(r, c), newMatrix(r, c), 1e-6f);
            }
        }

        AZ_POP_DISABLE_WARNING;
    }

    TEST_F(LegacryDeprecationHelper, TestLegacyMatrix33_GetEulerAngles)
    {
        AZ_PUSH_DISABLE_WARNING(4996, "-Wdeprecated-declarations");

        Matrix33 legacyMatrix = Matrix33::CreateRotationXYZ(Ang3(1.f, 2.f, 3.f));
        AZ::Matrix3x3 newMatrix = AZ::Matrix3x3::CreateFromRowMajorFloat9(legacyMatrix.GetData());

        // The legacy function Ang3::GetAnglesXYZ(Matrix33) is equivalent to
        // AZ::Quaternion::CreateFromMatrix3x3(AZ::Matrix3x3).GetEulerRadiansZYX()
        Ang3 legacyAngles = Ang3::GetAnglesXYZ(legacyMatrix);
        AZ::Vector3 newAngles = AZ::Quaternion::CreateFromMatrix3x3(newMatrix).GetEulerRadiansZYX();

        // Validation
        for (int r = 0; r < 3; ++r)
        {
            ASSERT_NEAR(legacyAngles[r], newAngles[r], 1e-6f);
        }

        AZ_POP_DISABLE_WARNING;
    }
} // namespace EditorUtilsTest
