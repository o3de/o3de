/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EMotionFX_precompiled.h"

#include <AzTest/AzTest.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/MathUtils.h>

#include <MCore/Source/Quaternion.h>
#include <MCore/Source/Vector.h>
#include <MCore/Source/AzCoreConversions.h>

class EmotionFXMathLibTests
    : public ::testing::Test
{
protected:

    void SetUp() override
    {
        m_azNormalizedVector3_a = AZ::Vector3(s_x1, s_y1, s_z1);
        m_azNormalizedVector3_a.Normalize();
        m_emQuaternion_a = MCore::Quaternion(m_azNormalizedVector3_a, s_angle_a);
        m_azQuaternion_a = AZ::Quaternion::CreateFromAxisAngle(m_azNormalizedVector3_a, s_angle_a);
    }

    void TearDown() override
    {
    }


    bool AZQuaternionCompareExact(AZ::Quaternion& quaternion, float x, float y, float z, float w)
    {
        if (quaternion.GetX() != x)
        {
            return false;
        }
        if (quaternion.GetY() != y)
        {
            return false;
        }
        if (quaternion.GetZ() != z)
        {
            return false;
        }
        if (quaternion.GetW() != w)
        {
            return false;
        }
        return true;
    }

    bool EmfxQuaternionCompareExact(MCore::Quaternion& quaternion, float x, float y, float z, float w)
    {
        if (quaternion.x != x)
        {
            return false;
        }
        if (quaternion.y != y)
        {
            return false;
        }
        if (quaternion.z != z)
        {
            return false;
        }
        if (quaternion.w != w)
        {
            return false;
        }
        return true;
    }

    bool AZQuaternionCompareClose(AZ::Quaternion& quaternion, float x, float y, float z, float w, float tolerance)
    {
        if (!AZ::IsClose(quaternion.GetX(), x, tolerance))
        {
            return false;
        }
        if (!AZ::IsClose(quaternion.GetY(), y, tolerance))
        {
            return false;
        }
        if (!AZ::IsClose(quaternion.GetZ(), z, tolerance))
        {
            return false;
        }
        if (!AZ::IsClose(quaternion.GetW(), w, tolerance))
        {
            return false;
        }
        return true;
    }

    bool AZVector3CompareClose(const AZ::Vector3& vector, const AZ::Vector3& vector2, float tolerance)
    {
        if (!AZ::IsClose(vector.GetX(), vector2.GetX(), tolerance))
        {
            return false;
        }
        if (!AZ::IsClose(vector.GetY(), vector2.GetY(), tolerance))
        {
            return false;
        }
        if (!AZ::IsClose(vector.GetZ(), vector2.GetZ(), tolerance))
        {
            return false;
        }
        return true;
    }

    bool AZVector3CompareClose(const AZ::Vector3& vector, float x, float y, float z, float tolerance)
    {
        if (!AZ::IsClose(vector.GetX(), x, tolerance))
        {
            return false;
        }
        if (!AZ::IsClose(vector.GetY(), y, tolerance))
        {
            return false;
        }
        if (!AZ::IsClose(vector.GetZ(), z, tolerance))
        {
            return false;
        }
        return true;
    }

    bool AZEMQuaternionsAreEqual(AZ::Quaternion& azQuaternion, const MCore::Quaternion& emQuaternion)
    {
        if (AZQuaternionCompareExact(azQuaternion, emQuaternion.x, emQuaternion.y,
            emQuaternion.z, emQuaternion.w))
        {
            return true;
        }
        return false;
    }

    bool AZEMQuaternionsAreClose(AZ::Quaternion& azQuaternion, const MCore::Quaternion& emQuaternion, const float tolerance)
    {
        if (AZQuaternionCompareClose(azQuaternion, emQuaternion.x, emQuaternion.y,
            emQuaternion.z, emQuaternion.w, tolerance))
        {
            return true;
        }
        return false;
    }

    static const float  s_toleranceHigh;
    static const float  s_toleranceMedium;
    static const float  s_toleranceLow;
    static const float  s_toleranceReallyLow;
    static const float  s_x1;
    static const float  s_y1;
    static const float  s_z1;
    static const float  s_angle_a;
    AZ::Vector3         m_azNormalizedVector3_a;
    AZ::Quaternion      m_azQuaternion_a;
    MCore::Quaternion   m_emQuaternion_a;
};

const float EmotionFXMathLibTests::s_toleranceHigh = 0.00001f;
const float EmotionFXMathLibTests::s_toleranceMedium = 0.0001f;
const float EmotionFXMathLibTests::s_toleranceLow = 0.001f;
const float EmotionFXMathLibTests::s_toleranceReallyLow = 0.02f;

const float EmotionFXMathLibTests::s_x1 = 0.2f;
const float EmotionFXMathLibTests::s_y1 = 0.3f;
const float EmotionFXMathLibTests::s_z1 = 0.4f;
const float EmotionFXMathLibTests::s_angle_a = 0.5f;


///////////////////////////////////////////////////////////////////////////////


// MCore::Quaternion: Test identity values
TEST_F(EmotionFXMathLibTests, QuaternionIdentity_Identity_Success)
{
    MCore::Quaternion test(0.1f, 0.2f, 0.3f, 0.4f);
    test.Identity();
    ASSERT_TRUE(test == MCore::Quaternion(0.0f, 0.0f, 0.0f, 1.0f));
}

//////////////////////////////////////////////////////////////////
//Getting and setting of Quaternions
//////////////////////////////////////////////////////////////////

TEST_F(EmotionFXMathLibTests, AZQuaternionGet_Elements_Success)
{
    AZ::Quaternion test(0.1f, 0.2f, 0.3f, 0.4f);
    ASSERT_TRUE(AZQuaternionCompareExact(test, 0.1f, 0.2f, 0.3f, 0.4f));
}

// Compare equivalent normalized quaternions between systems
TEST_F(EmotionFXMathLibTests, AZEMQuaternionNormalizeEquivalent_Success)
{
    AZ::Quaternion azTest(0.1f, 0.2f, 0.3f, 0.4f);
    MCore::Quaternion emTest(0.1f, 0.2f, 0.3f, 0.4f);
    azTest.Normalize();
    emTest.Normalize();

    ASSERT_TRUE(AZQuaternionCompareClose(azTest, emTest.x, emTest.y, emTest.z, emTest.w, s_toleranceMedium));
}

///////////////////////////////////////////////////////////////////////////////
// Axis Angle
///////////////////////////////////////////////////////////////////////////////

// Compare setting a quaternion using axis and angle
TEST_F(EmotionFXMathLibTests, AZEMQuaternionConversion_SetToAxisAngleEquivalent_Success)
{
    MCore::Quaternion emQuaternion(m_azNormalizedVector3_a, s_angle_a);
    AZ::Quaternion azQuaternion = AZ::Quaternion::CreateFromAxisAngle(m_azNormalizedVector3_a, s_angle_a);

    ASSERT_TRUE(AZQuaternionCompareClose(azQuaternion, emQuaternion.x, emQuaternion.y, emQuaternion.z, emQuaternion.w, s_toleranceLow));
}

// Compare equivalent conversions quaternions -> (axis, angle) between systems
TEST_F(EmotionFXMathLibTests, AZEMQuaternionConversion_ToAxisAngleEquivalent_Success)
{
    //populate Quaternions with same data
    MCore::Quaternion emTest = m_emQuaternion_a;
    AZ::Quaternion azTest(emTest.x, emTest.y, emTest.z, emTest.w);

    AZ::Vector3 emAxis;
    float emAngle;
    emTest.ToAxisAngle(&emAxis, &emAngle);

    AZ::Vector3 azAxis;
    float azAngle;
    AZ::ConvertQuaternionToAxisAngle(azTest, azAxis, azAngle);

    bool same = AZ::IsClose(azAngle, emAngle, s_toleranceLow) &&
        AZVector3CompareClose(azAxis, emAxis, s_toleranceLow);

    ASSERT_TRUE(same);
}


///////////////////////////////////////////////////////////////////////////////
//Basic rotations
///////////////////////////////////////////////////////////////////////////////

//Right Hand - counterclockwise looking down axis from positive side

TEST_F(EmotionFXMathLibTests, AZQuaternion_Rotation1ComponentAxisX_Success)
{
    AZ::Vector3 axis = AZ::Vector3(1.0f, 0.0f, 0.0f);
    axis.Normalize();
    AZ::Quaternion azQuaternion1 = AZ::Quaternion::CreateFromAxisAngle(axis, AZ::Constants::HalfPi);
    AZ::Vector3 vertexIn(0.0f, 0.0f, 0.1f);
    AZ::Vector3 vertexOut;
    vertexOut = azQuaternion1.TransformVector(vertexIn);

    bool same = AZVector3CompareClose(vertexOut, AZ::Vector3(0.0f, -0.1f, 0.0f), s_toleranceLow);
    ASSERT_TRUE(same);
}

TEST_F(EmotionFXMathLibTests, AZQuaternion_Rotation1ComponentAxisY_Success)
{
    AZ::Vector3 axis = AZ::Vector3(0.0f, 1.0f, 0.0f);
    axis.Normalize();
    AZ::Quaternion azQuaternion1 = AZ::Quaternion::CreateFromAxisAngle(axis, AZ::Constants::HalfPi);
    AZ::Vector3 vertexIn(0.1f, 0.0f, 0.0f);
    AZ::Vector3 vertexOut;
    vertexOut = azQuaternion1.TransformVector(vertexIn);

    bool same = AZVector3CompareClose(vertexOut, AZ::Vector3(0.0f, 0.0f, -0.1f), s_toleranceLow);
    ASSERT_TRUE(same);
}

TEST_F(EmotionFXMathLibTests, AZQuaternion_Rotation1ComponentAxisZ_Success)
{
    AZ::Vector3 axis = AZ::Vector3(0.0f, 0.0f, 1.0f);
    axis.Normalize();
    AZ::Quaternion azQuaternion1 = AZ::Quaternion::CreateFromAxisAngle(axis, AZ::Constants::HalfPi);
    AZ::Vector3 vertexIn(0.1f, 0.0f, 0.0f);
    AZ::Vector3 vertexOut;
    vertexOut = azQuaternion1.TransformVector(vertexIn);

    bool same = AZVector3CompareClose(vertexOut, AZ::Vector3(0.0f, 0.1f, 0.0f), s_toleranceLow);
    ASSERT_TRUE(same);
}

//AZ Quaternion Normalize Vertex test
TEST_F(EmotionFXMathLibTests, AZazQuaternion_NormalizedQuaternionRotationTest3DAxis_Success)
{
    AZ::Vector3 axis = AZ::Vector3(1.0f, 0.7f, 0.3f);
    axis.Normalize();
    AZ::Quaternion azQuaternion1 = AZ::Quaternion::CreateFromAxisAngle(axis, AZ::Constants::HalfPi);
    AZ::Quaternion azQuaternion1Normalized = azQuaternion1.GetNormalized();
    AZ::Vector3 vertexIn(0.1f, 0.2f, 0.3f);

    AZ::Vector3 vertexOut1, vertexOut1FromNormalizedQuaternion;

    //generate value 1
    vertexOut1 = azQuaternion1.TransformVector(vertexIn);

    vertexOut1FromNormalizedQuaternion = azQuaternion1Normalized.TransformVector(vertexIn);

    bool same = AZVector3CompareClose(vertexOut1, vertexOut1FromNormalizedQuaternion, s_toleranceLow);
    ASSERT_TRUE(same);
}


///////////////////////////////////////////////////////////////////////////////
// Euler  AZ
///////////////////////////////////////////////////////////////////////////////

// AZ Quaternion <-> euler conversion Vertex test 1 component axis
TEST_F(EmotionFXMathLibTests, AZQuaternion_EulerGetSet1ComponentAxis_Success)
{
    AZ::Vector3 axis = AZ::Vector3(0.0f, 0.0f, 1.0f);
    axis.Normalize();
    AZ::Quaternion azQuaternion1 = AZ::Quaternion::CreateFromAxisAngle(axis, AZ::Constants::HalfPi);
    AZ::Vector3 vertexIn(0.1f, 0.2f, 0.3f);
    AZ::Vector3 vertexOut1, vertexOut2;
    AZ::Vector3 euler1;
    AZ::Quaternion test1;

    vertexOut1 = azQuaternion1.TransformVector(vertexIn);

    //euler out and in
    euler1 = AZ::ConvertQuaternionToEulerRadians(azQuaternion1);
    test1 = AZ::ConvertEulerRadiansToQuaternion(euler1);

    //generate vertex value 2
    vertexOut2 = test1.TransformVector(vertexIn);

    bool same = AZVector3CompareClose(vertexOut1, vertexOut2, s_toleranceReallyLow);
    ASSERT_TRUE(same);
}

// AZ Quaternion <-> euler conversion Vertex test 2 component axis
TEST_F(EmotionFXMathLibTests, AZQuaternion_EulerGetSet2ComponentAxis_Success)
{
    AZ::Vector3 axis = AZ::Vector3(0.0f, 0.7f, 0.3f);
    axis.Normalize();
    AZ::Quaternion azQuaternion1 = AZ::Quaternion::CreateFromAxisAngle(axis, AZ::Constants::HalfPi);
    AZ::Vector3 vertexIn(0.1f, 0.2f, 0.3f);
    AZ::Vector3 vertexOut1, vertexOut2;
    AZ::Vector3 euler1;
    AZ::Quaternion test1;

    //generate vertex value 1
    vertexOut1 = azQuaternion1.TransformVector(vertexIn);

    //euler out and in
    euler1 = AZ::ConvertQuaternionToEulerRadians(azQuaternion1);
    test1 = AZ::ConvertEulerRadiansToQuaternion(euler1);

    //generate vertex value 2
    vertexOut2 = test1.TransformVector(vertexIn);

    bool same = AZVector3CompareClose(vertexOut1, vertexOut2, s_toleranceReallyLow);
    ASSERT_TRUE(same);
}

// AZ Quaternion <-> euler conversion Vertex test 3 component axis
TEST_F(EmotionFXMathLibTests, AZQuaternion_EulerInOutRotationTest3DAxis_Success)
{
    AZ::Vector3 axis = AZ::Vector3(1.0f, 0.7f, 0.3f);
    axis.Normalize();
    AZ::Quaternion azQuaternion1 = AZ::Quaternion::CreateFromAxisAngle(axis, AZ::Constants::HalfPi);
    AZ::Vector3 vertexIn(0.1f, 0.2f, 0.3f);
    AZ::Vector3 vertexOut1, vertexOut2;
    AZ::Vector3 euler1;
    AZ::Quaternion test1;

    //generate vertex value 1
    vertexOut1 = azQuaternion1.TransformVector(vertexIn);

    //euler out and in
    euler1 = AZ::ConvertQuaternionToEulerRadians(azQuaternion1);
    test1 = AZ::ConvertEulerRadiansToQuaternion(euler1);

    //generate vertex value 2
    vertexOut2 = test1.TransformVector(vertexIn);

    bool same = AZVector3CompareClose(vertexOut1, vertexOut2, s_toleranceReallyLow);
    ASSERT_TRUE(same);
}

// Quaternion -> Transform -> Euler conversion is same as Quaternion -> Euler
// AZ Euler get set Transform Compare test 3 dim axis
TEST_F(EmotionFXMathLibTests, AZQuaternion_EulerGetSet3ComponentAxisCompareTransform_Success)
{
    AZ::Vector3 axis = AZ::Vector3(1.0f, 0.7f, 0.3f);
    axis.Normalize();
    AZ::Quaternion azQuaternion1 = AZ::Quaternion::CreateFromAxisAngle(axis, AZ::Constants::HalfPi);
    AZ::Vector3 vertexIn(0.1f, 0.2f, 0.3f);
    AZ::Vector3 vertexOut1, vertexOut2, vertexTransform;
    AZ::Vector3 euler1, eulerVectorFromTransform;
    AZ::Quaternion test1, testTransformQuat;

    //generate vertex value 1
    vertexOut1 = azQuaternion1.TransformVector(vertexIn);

    //use Transform to generate euler
    AZ::Transform TransformFromQuat = AZ::Transform::CreateFromQuaternion(azQuaternion1);
    eulerVectorFromTransform = AZ::ConvertTransformToEulerRadians(TransformFromQuat);
    testTransformQuat = AZ::ConvertEulerRadiansToQuaternion(eulerVectorFromTransform);
    vertexTransform = testTransformQuat.TransformVector(vertexIn);

    //use existing convert function
    euler1 = AZ::ConvertQuaternionToEulerRadians(azQuaternion1);
    test1 = AZ::ConvertEulerRadiansToQuaternion(euler1);

    //generate vertex value 2
    vertexOut2 = test1.TransformVector(vertexIn);

    bool same = AZVector3CompareClose(vertexOut1, vertexTransform, s_toleranceReallyLow)
        && AZVector3CompareClose(vertexOut1, vertexOut2, s_toleranceReallyLow)
        && AZVector3CompareClose(vertexOut2, vertexTransform, s_toleranceHigh);
    ASSERT_TRUE(same);
}


// EM Quaternion to Euler test
TEST_F(EmotionFXMathLibTests, EMQuaternionConversion_ToEulerEquivalent_Success)
{
    AZ::Vector3 eulerIn(0.1f, 0.2f, 0.3f);
    MCore::Quaternion test;
    test.SetEuler(eulerIn.GetX(), eulerIn.GetY(), eulerIn.GetZ());
    AZ::Vector3 eulerOut = test.ToEuler();

    ASSERT_TRUE(AZVector3CompareClose(eulerOut, 0.1f, 0.2f, 0.3f, s_toleranceHigh));
}

// AZ Quaternion to Euler test
//only way to test Quaternions sameness is to apply it to a vector and measure result
TEST_F(EmotionFXMathLibTests, AZQuaternionConversion_ToEulerEquivalent_Success)
{
    AZ::Vector3 eulerIn(0.1f, 0.2f, 0.3f);
    AZ::Vector3 testVertex(0.1f, 0.2f, 0.3f);
    AZ::Vector3 outVertex1, outVertex2;
    AZ::Quaternion test, test2;
    AZ::Vector3 eulerOut1, eulerOut2;

    test = AZ::ConvertEulerRadiansToQuaternion(eulerIn);
    test.Normalize();
    outVertex1 = test.TransformVector(testVertex);

    eulerOut1 = AZ::ConvertQuaternionToEulerRadians(test);

    test2 = AZ::ConvertEulerRadiansToQuaternion(eulerOut1);
    test2.Normalize();
    outVertex2 = test2.TransformVector(testVertex);

    eulerOut2 = AZ::ConvertQuaternionToEulerRadians(test2);
    ASSERT_TRUE(AZVector3CompareClose(eulerOut1, eulerOut2, s_toleranceReallyLow));
}

///////////////////////////////////////////////////////////////////////////////
//Quaternion order test
//determines that ordering is same between systems.
///////////////////////////////////////////////////////////////////////////////
TEST_F(EmotionFXMathLibTests, AZEMQuaternion_OrderTest_Success)
{
    AZ::Vector3 axis = AZ::Vector3(1.0f, 0.7f, 0.3f);
    axis.Normalize();
    AZ::Quaternion azQuaternion1 = AZ::Quaternion::CreateFromAxisAngle(axis, AZ::Constants::HalfPi);

    AZ::Vector3 axis2 = AZ::Vector3(0.2f, 0.5f, 0.9f);
    axis2.Normalize();
    AZ::Quaternion azQuaternion2 = AZ::Quaternion::CreateFromAxisAngle(axis2, AZ::Constants::HalfPi);

    MCore::Quaternion emQuaternion1(azQuaternion1.GetX(), azQuaternion1.GetY(), azQuaternion1.GetZ(), azQuaternion1.GetW());
    MCore::Quaternion emQuaternion2(azQuaternion2.GetX(), azQuaternion2.GetY(), azQuaternion2.GetZ(), azQuaternion2.GetW());

    AZ::Quaternion azQuaterionOut = azQuaternion1 * azQuaternion2;
    AZ::Quaternion azQuaterionOut2 = azQuaternion2 * azQuaternion1;
    MCore::Quaternion emQuaterionOut = emQuaternion1 * emQuaternion2;

    AZ::Vector3 azVertexIn(0.1f, 0.2f, 0.3f);

    AZ::Vector3 azVertexOut, azVertexOut2;
    AZ::Vector3 emVertexOut;

    azVertexOut = azQuaterionOut.TransformVector(azVertexIn);
    azVertexOut2 = azQuaterionOut2.TransformVector(azVertexIn);
    emVertexOut = emQuaterionOut * azVertexIn;

    bool same = AZVector3CompareClose(emVertexOut, azVertexOut.GetX(), azVertexOut.GetY(), azVertexOut.GetZ(), s_toleranceMedium);
    ASSERT_TRUE(same);
}


///////////////////////////////////////////////////////////////////////////////
//  Quaternion Matrix
///////////////////////////////////////////////////////////////////////////////

// Test EM Quaternion made from Matrix X
TEST_F(EmotionFXMathLibTests, AZQuaternionConversion_FromAZTransformXRot_Success)
{
    AZ::Transform azTransform = AZ::Transform::CreateRotationX(AZ::Constants::HalfPi);
    AZ::Quaternion azQuaternion = azTransform.GetRotation();

    AZ::Vector3 emVertexIn(0.0f, 0.1f, 0.0f);
    AZ::Vector3 emVertexOut = azQuaternion.TransformVector(emVertexIn);

    bool same = AZVector3CompareClose(emVertexOut, 0.0f, 0.0f, 0.1f, s_toleranceMedium);
    ASSERT_TRUE(same);
}

// Test EM Quaternion made from Matrix Y
TEST_F(EmotionFXMathLibTests, AZQuaternionConversion_FromAZTransformYRot_Success)
{
    AZ::Transform azTransform = AZ::Transform::CreateRotationY(AZ::Constants::HalfPi);
    AZ::Quaternion azQuaternion = azTransform.GetRotation();

    AZ::Vector3 emVertexIn(0.0f, 0.0f, 0.1f);
    AZ::Vector3 emVertexOut = azQuaternion.TransformVector(emVertexIn);

    bool same = AZVector3CompareClose(emVertexOut, 0.1f, 0.0f, 0.0f, s_toleranceMedium);
    ASSERT_TRUE(same);
}

// Compare Quaternion made from Matrix X
TEST_F(EmotionFXMathLibTests, AZQuaternionConversion_FromMatrixXRot_Success)
{
    AZ::Matrix4x4 azMatrix = AZ::Matrix4x4::CreateRotationX(AZ::Constants::HalfPi);
    AZ::Quaternion azQuaternion = AZ::Quaternion::CreateFromMatrix4x4(azMatrix);

    AZ::Transform azTransform = AZ::Transform::CreateRotationX(AZ::Constants::HalfPi);
    AZ::Quaternion azQuaternionFromTransform = azTransform.GetRotation();

    AZ::Vector3 azVertexIn(0.0f, 0.1f, 0.0f);

    AZ::Vector3 azVertexOut = azQuaternion.TransformVector(azVertexIn);
    AZ::Vector3 emVertexOut = azQuaternionFromTransform.TransformVector(azVertexIn);

    bool same = AZVector3CompareClose(azVertexOut, emVertexOut, s_toleranceMedium);
    ASSERT_TRUE(same);
}

// Compare Quaternion made from Matrix Y
TEST_F(EmotionFXMathLibTests, AZQuaternionConversion_FromMatrixYRot_Success)
{
    AZ::Matrix4x4 azMatrix = AZ::Matrix4x4::CreateRotationY(AZ::Constants::HalfPi);
    AZ::Quaternion azQuaternion = AZ::Quaternion::CreateFromMatrix4x4(azMatrix);

    AZ::Transform azTransform = AZ::Transform::CreateRotationY(AZ::Constants::HalfPi);
    AZ::Quaternion azQuaternionFromTransform = azTransform.GetRotation();

    AZ::Vector3 azVertexIn(0.1f, 0.0f, 0.0f);
    AZ::Vector3 azVertexOut = azQuaternion.TransformVector(azVertexIn);
    AZ::Vector3 emVertexOut = azQuaternionFromTransform.TransformVector(azVertexIn);

    bool same = AZVector3CompareClose(azVertexOut, emVertexOut, s_toleranceMedium);
    ASSERT_TRUE(same);
}

// Compare Quaternion made from Matrix Z
TEST_F(EmotionFXMathLibTests, AZQuaternionConversion_FromMatrixZRot_Success)
{
    AZ::Matrix4x4 azMatrix = AZ::Matrix4x4::CreateRotationZ(AZ::Constants::HalfPi);
    AZ::Quaternion azQuaternion = AZ::Quaternion::CreateFromMatrix4x4(azMatrix);

    AZ::Transform azTransform = AZ::Transform::CreateRotationZ(AZ::Constants::HalfPi);
    AZ::Quaternion azQuaternionFromTransform = azTransform.GetRotation();

    AZ::Vector3 azVertexIn(0.1f, 0.0f, 0.0f);

    AZ::Vector3 azVertexOut = azQuaternion.TransformVector(azVertexIn);
    AZ::Vector3 emVertexOut = azQuaternionFromTransform.TransformVector(azVertexIn);

    bool same = AZVector3CompareClose(azVertexOut, emVertexOut, s_toleranceMedium);
    ASSERT_TRUE(same);
}

// Compare Quaternion -> Matrix conversion
// AZ - column major
// Emfx - row major
TEST_F(EmotionFXMathLibTests, AZQuaternionConversion_ToMatrix_Success)
{
    AZ::Vector3 axis = AZ::Vector3(1.0f, 0.7f, 0.3f);
    axis.Normalize();
    AZ::Quaternion azQuaternion = AZ::Quaternion::CreateFromAxisAngle(axis, AZ::Constants::HalfPi);

    AZ::Matrix4x4 azMatrix = AZ::Matrix4x4::CreateFromQuaternion(azQuaternion);
    AZ::Transform azTransform = AZ::Transform::CreateFromQuaternionAndTranslation(azQuaternion, AZ::Vector3::CreateZero());

    bool same = true;

    AZ::Vector3 azTransformBasis[4];
    azTransform.GetBasisAndTranslation(&azTransformBasis[0], &azTransformBasis[1], &azTransformBasis[2], &azTransformBasis[3]);

    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            float emValue = azTransformBasis[j].GetElement(i);
            float azValue = azMatrix.GetElement(i, j);
            if (!AZ::IsClose(emValue, azValue, s_toleranceReallyLow))
            {
                same = false;
                break;
            }
        }

        if (!same)
        {
            break;
        }
    }
    ASSERT_TRUE(same);
    ASSERT_TRUE(AZ::IsClose(azMatrix.GetElement(3, 0), 0.0f, s_toleranceReallyLow));
    ASSERT_TRUE(AZ::IsClose(azMatrix.GetElement(3, 1), 0.0f, s_toleranceReallyLow));
    ASSERT_TRUE(AZ::IsClose(azMatrix.GetElement(3, 2), 0.0f, s_toleranceReallyLow));
    ASSERT_TRUE(AZ::IsClose(azMatrix.GetElement(3, 3), 1.0f, s_toleranceReallyLow));
}

///////////////////////////////////////////////////////////////////////////////
// AZEMQuaternion Compare Output tests
// Determines the AZ and MCore quaternion outputs are same/close after same math operations.
///////////////////////////////////////////////////////////////////////////////
TEST_F(EmotionFXMathLibTests, AZEMQuaternion_CompareOperatorAddEquivalent_Success)
{
    // Quaternion test: operator '+' and operator '+='
    AZ::Quaternion azQuaternion = AZ::Quaternion(0.1f, 0.2f, 0.3f, 1.0f);
    AZ::Quaternion azQuaternion2 = AZ::Quaternion(0.8f, 0.7f, 0.6f, 1.0f);
    azQuaternion.Normalize();
    azQuaternion2.Normalize();
    azQuaternion = azQuaternion + azQuaternion2;
    azQuaternion2 += azQuaternion;
    
    MCore::Quaternion emQuaternion = MCore::Quaternion(0.1f, 0.2f, 0.3f, 1.0f);
    MCore::Quaternion emQuaternion2 = MCore::Quaternion(0.8f, 0.7f, 0.6f, 1.0f);
    emQuaternion.Normalize();
    emQuaternion2.Normalize();
    emQuaternion = emQuaternion + emQuaternion2;
    emQuaternion2 += emQuaternion;
    
    EXPECT_TRUE(AZEMQuaternionsAreClose(azQuaternion, emQuaternion, s_toleranceLow)) << "AZ/MCore Quaternions should have similar output with operator '+'";
    EXPECT_TRUE(AZEMQuaternionsAreClose(azQuaternion2, emQuaternion2, s_toleranceLow)) << "AZ/MCore Quaternions should have similar output with operator '+='";
}

TEST_F(EmotionFXMathLibTests, AZEMQuaternion_CompareOperatorSubtractEquivalent_Success)
{
    // Quaternion test: operator '-' and operator '-='
    AZ::Quaternion azQuaternion = AZ::Quaternion(0.1f, 0.2f, 0.3f, 1.0f);
    AZ::Quaternion azQuaternion2 = AZ::Quaternion(0.8f, 0.7f, 0.6f, 1.0f);
    azQuaternion.Normalize();
    azQuaternion2.Normalize();
    azQuaternion = azQuaternion - azQuaternion2;
    azQuaternion2 -= azQuaternion;

    MCore::Quaternion emQuaternion = MCore::Quaternion(0.1f, 0.2f, 0.3f, 1.0f);
    MCore::Quaternion emQuaternion2 = MCore::Quaternion(0.8f, 0.7f, 0.6f, 1.0f);
    emQuaternion.Normalize();
    emQuaternion2.Normalize();
    emQuaternion = emQuaternion - emQuaternion2;
    emQuaternion2 -= emQuaternion;

    EXPECT_TRUE(AZEMQuaternionsAreClose(azQuaternion, emQuaternion, s_toleranceLow)) << "AZ/MCore Quaternions should have similar output with operator '-'";
    EXPECT_TRUE(AZEMQuaternionsAreClose(azQuaternion2, emQuaternion2, s_toleranceLow)) << "AZ/MCore Quaternions should have similar output with operator '-='";
}

TEST_F(EmotionFXMathLibTests, AZEMQuaternion_CompareOperatorMultiplyHasSimilarOutput_Success)
{
    // Quaternion test: operator '*' and operator '*=' with another quaternion, vector3 and float
    AZ::Quaternion azQuaternion = AZ::Quaternion(0.1f, 0.2f, 0.3f, 1.0f);
    AZ::Quaternion azQuaternion2 = AZ::Quaternion(0.8f, 0.7f, 0.6f, 1.0f);
    AZ::Quaternion azQuaternion3 = AZ::Quaternion(0.1f, 0.2f, 0.3f, 1.0f);
    azQuaternion.Normalize();
    azQuaternion2.Normalize();
    azQuaternion3.Normalize();
    azQuaternion = azQuaternion * azQuaternion2;
    azQuaternion2 *= azQuaternion;
    azQuaternion3 *= 0.5f;
    AZ::Vector3 aztestVec3 = azQuaternion2.TransformVector(m_azNormalizedVector3_a);

    MCore::Quaternion emQuaternion = MCore::Quaternion(0.1f, 0.2f, 0.3f, 1.0f);
    MCore::Quaternion emQuaternion2 = MCore::Quaternion(0.8f, 0.7f, 0.6f, 1.0f);
    MCore::Quaternion emQuaternion3 = MCore::Quaternion(0.1f, 0.2f, 0.3f, 1.0f);
    emQuaternion.Normalize();
    emQuaternion2.Normalize();
    emQuaternion3.Normalize();
    emQuaternion = emQuaternion * emQuaternion2;
    emQuaternion2 *= emQuaternion;
    emQuaternion3 *= 0.5f;
    AZ::Vector3 emtestVec3 = emQuaternion2 * m_azNormalizedVector3_a;

    EXPECT_TRUE(AZEMQuaternionsAreClose(azQuaternion, emQuaternion, s_toleranceLow)) << "AZ/MCore Quaternions should have similar output with operator '*' with another quaternion";
    EXPECT_TRUE(AZEMQuaternionsAreClose(azQuaternion2, emQuaternion2, s_toleranceLow)) << "AZ/MCore Quaternions should have similar output with operator '*=' with another quaternion";
    EXPECT_TRUE(AZEMQuaternionsAreClose(azQuaternion3, emQuaternion3, s_toleranceLow)) << "AZ/MCore Quaternions should have similar output with operator '*=' with a float value";
    EXPECT_TRUE(AZVector3CompareClose(aztestVec3, emtestVec3, s_toleranceLow)) << "AZ/MCore Quaternions should have similar output with operator '*' with a vector3";
}

TEST_F(EmotionFXMathLibTests, AZEMQuaternion_EquivalentOperatorsHasSameOutput_Success)
{
    // Testing Quaternion == Quaternion and operator!=
    bool azCheck = AZ::Quaternion(0.1f, 0.2f, 0.3f, 1.0f).GetNormalized() == AZ::Quaternion(0.1f, 0.2f, 0.3f, 1.0f).GetNormalized();
    bool azCheck2 = AZ::Quaternion(0.1f, 0.2f, 0.3f, 1.0f).GetNormalized() == AZ::Quaternion(0.1000001f, 0.2000001f, 0.3000001f, 1.0f).GetNormalized();
    bool azCheck3 = AZ::Quaternion(0.1f, 0.2f, 0.3f, 1.0f).GetNormalized() != AZ::Quaternion(0.1f, 0.2f, 0.3f, 1.0f).GetNormalized();
    bool azCheck4 = AZ::Quaternion(0.1f, 0.2f, 0.3f, 1.0f).GetNormalized() != AZ::Quaternion(0.1000001f, 0.2000001f, 0.3000001f, 1.0f).GetNormalized();

    bool emCheck = MCore::Quaternion(0.1f, 0.2f, 0.3f, 1.0f).Normalized() == MCore::Quaternion(0.1f, 0.2f, 0.3f, 1.0f).Normalized();
    bool emCheck2 = MCore::Quaternion(0.1f, 0.2f, 0.3f, 1.0f).Normalized() == MCore::Quaternion(0.1000001f, 0.2000001f, 0.3000001f, 1.0f).Normalized();
    bool emCheck3 = MCore::Quaternion(0.1f, 0.2f, 0.3f, 1.0f).Normalized() != MCore::Quaternion(0.1f, 0.2f, 0.3f, 1.0f).Normalized();
    bool emCheck4 = MCore::Quaternion(0.1f, 0.2f, 0.3f, 1.0f).Normalized() != MCore::Quaternion(0.1000001f, 0.2000001f, 0.3000001f, 1.0f).Normalized();

    EXPECT_TRUE(azCheck == emCheck) << "AZ/MCore Quaternions should have same output of 'true' with operator '=='";
    EXPECT_TRUE(azCheck2 == emCheck2) << "AZ/MCore Quaternions should have same output of 'false' with operator '=='";
    EXPECT_TRUE(azCheck3 == emCheck3) << "AZ/MCore Quaternions should have same output of 'false' with operator '!='";
    EXPECT_TRUE(azCheck4 == emCheck4) << "AZ/MCore Quaternions should have same output of 'true' with operator '!='";
}

TEST_F(EmotionFXMathLibTests, AZEMQuaternion_InverseHasSimilarOutput_Success)
{
    // Test quaternions inverse method
    AZ::Quaternion azQuaternion = AZ::Quaternion(0.1f, 0.2f, 0.3f, 1.0f).GetNormalized().GetInverseFull();
    AZ::Quaternion azQuaternion2 = AZ::Quaternion(0.0f, 0.0f, 0.0f, 1.0f).GetNormalized().GetInverseFull();

    MCore::Quaternion emQuaternion = MCore::Quaternion(0.1f, 0.2f, 0.3f, 1.0f).Normalized().Inverse();
    MCore::Quaternion emQuaternion2 = MCore::Quaternion(0.0f, 0.0f, 0.0f, 1.0f).Normalized().Inverse();

    EXPECT_TRUE(AZEMQuaternionsAreClose(azQuaternion, emQuaternion, s_toleranceLow)) << "AZ/MCore Quaternions should have similar Inverse output";
    EXPECT_TRUE(AZEMQuaternionsAreClose(azQuaternion2, emQuaternion2, s_toleranceLow)) << "AZ/MCore Quaternion(0.0f, 0.0f, 0.0f, 1.0f) should have similar Inverse output";
}

TEST_F(EmotionFXMathLibTests, AZEMQuaternion_ConjugateHasSimilarOutput_Success)
{
    // Test quaternion conjugate method
    AZ::Quaternion azQuaternion = AZ::Quaternion(0.1f, 0.2f, 0.3f, 1.0f).GetNormalized().GetConjugate();
    AZ::Quaternion azQuaternion2 = AZ::Quaternion(0.0f, 0.0f, 0.0f, 1.0f).GetNormalized().GetConjugate();

    MCore::Quaternion emQuaternion = MCore::Quaternion(0.1f, 0.2f, 0.3f, 1.0f).Normalized().Conjugate();
    MCore::Quaternion emQuaternion2 = MCore::Quaternion(0.0f, 0.0f, 0.0f, 1.0f).Normalized().Conjugate();

    EXPECT_TRUE(AZEMQuaternionsAreClose(azQuaternion, emQuaternion, s_toleranceLow)) << "AZ/MCore Quaternions should have similar Conjugate output";
    EXPECT_TRUE(AZEMQuaternionsAreClose(azQuaternion2, emQuaternion2, s_toleranceLow)) << "AZ/MCore Quaternion(0.0f, 0.0f, 0.0f, 1.0f) should have similar Conjugate output";
}

TEST_F(EmotionFXMathLibTests, AZEMQuaternion_HasSameSquareLengthOutput_Success)
{
    // Test AZ and MCore quaternions to have similar square length
    float azTest = AZ::Quaternion(0.1f, 0.2f, 0.3f, 1.0f).GetNormalized().GetLengthSq();
    float azTest2 = AZ::Quaternion(0.0f, 0.0f, 0.0f, 1.0f).GetNormalized().GetLengthSq();

    float emTest = MCore::Quaternion(0.1f, 0.2f, 0.3f, 1.0f).Normalized().SquareLength();
    float emTest2 = MCore::Quaternion(0.0f, 0.0f, 0.0f, 1.0f).Normalized().SquareLength();
    
    EXPECT_TRUE(AZ::GetAbs(azTest - emTest) < s_toleranceLow) << "AZ/MCore Quaternions should have similar square length output";
    EXPECT_TRUE(AZ::GetAbs(azTest2 - emTest2) < s_toleranceLow) << "AZ/MCore Quaternion(0.0f, 0.0f, 0.0f, 1.0f) should have similar square length output";
}

TEST_F(EmotionFXMathLibTests, AZEMQuaternion_HasSameLengthOutput_Success)
{
    // Test AZ and MCore quaternions to have similar length
    // AZ GetLength, GetLengthApprox, GetLength all returns sqrtf(Dot(*this))
    float azTest = AZ::Quaternion(0.1f, 0.2f, 0.3f, 1.0f).GetNormalized().GetLength();
    float azTest2 = AZ::Quaternion(0.0f, 0.0f, 0.0f, 1.0f).GetNormalized().GetLength();

    float emTest = MCore::Quaternion(0.1f, 0.2f, 0.3f, 1.0f).Normalized().Length();
    float emTest2 = MCore::Quaternion(0.0f, 0.0f, 0.0f, 1.0f).Normalized().Length();

    EXPECT_TRUE(AZ::GetAbs(azTest - emTest) < s_toleranceLow) << "AZ/MCore Quaternions should have similar length output";
    EXPECT_TRUE(AZ::GetAbs(azTest2 - emTest2) < s_toleranceLow) << "AZ/MCore Quaternion(0.0f, 0.0f, 0.0f, 1.0f) should have similar length output";
}

TEST_F(EmotionFXMathLibTests, AZEMQuaternion_HasSameDotProductOutput_Success)
{
    // Test AZ and MCore quaternions to have similar dot product
    float azDotTest = AZ::Quaternion(0.1f, 0.2f, 0.3f, 1.0f).GetNormalized().Dot(AZ::Quaternion(0.8f, 0.7f, 0.6f, 1.0f));
    float azDotTest2 = AZ::Quaternion(0.1f, 0.2f, 0.3f, 1.0f).GetNormalized().Dot(AZ::Quaternion(0.0f, 0.0f, 0.0f, 1.0f));
    float azDotTest3 = AZ::Quaternion(0.0f, 0.0f, 0.0f, 1.0f).GetNormalized().Dot(AZ::Quaternion(0.0f, 0.0f, 0.0f, 1.0f));

    float emDotTest = MCore::Quaternion(0.1f, 0.2f, 0.3f, 1.0f).Normalized().Dot(MCore::Quaternion(0.8f, 0.7f, 0.6f, 1.0f));
    float emDotTest2 = MCore::Quaternion(0.1f, 0.2f, 0.3f, 1.0f).Normalized().Dot(MCore::Quaternion(0.0f, 0.0f, 0.0f, 1.0f));
    float emDotTest3 = MCore::Quaternion(0.0f, 0.0f, 0.0f, 1.0f).Normalized().Dot(MCore::Quaternion(0.0f, 0.0f, 0.0f, 1.0f));

    EXPECT_TRUE(AZ::GetAbs(azDotTest - emDotTest) < s_toleranceLow) << "AZ/MCore Quaternions should have similar dot product output";
    EXPECT_TRUE(AZ::GetAbs(azDotTest2 - emDotTest2) < s_toleranceLow) << "AZ/MCore Quaternions should have similar dot product output";
    EXPECT_TRUE(AZ::GetAbs(azDotTest3 - emDotTest3) < s_toleranceLow) << "AZ/MCore Quaternion(0.0f, 0.0f, 0.0f, 1.0f) should have similar dot product output";
}

TEST_F(EmotionFXMathLibTests, AZEMQuaternion_HasSimilarLerpOutput_Success)
{
    // Test AZ and MCore quaternions to have similar Linear Interpolated quaternions
    float testCases[6] = { 0.0f, 0.1f, 0.25f, 0.5f, 0.8f, 1.0f };
    for (float testVal : testCases)
    {
        AZ::Quaternion azQuaternionA = AZ::Quaternion(0.1f, 0.2f, 0.3f, 1.0f).GetNormalized();
        AZ::Quaternion azQuaternionB = AZ::Quaternion(0.8f, 0.7f, 0.6f, 1.0f).GetNormalized();
        AZ::Quaternion azQuaternionC = azQuaternionA.Lerp(azQuaternionB, testVal);

        MCore::Quaternion emQuaternionA = MCore::Quaternion(0.1f, 0.2f, 0.3f, 1.0f).Normalized();
        MCore::Quaternion emQuaternionB = MCore::Quaternion(0.8f, 0.7f, 0.6f, 1.0f).Normalized();
        MCore::Quaternion emQuaternionC = emQuaternionA.Lerp(emQuaternionB, testVal);

        EXPECT_TRUE(AZEMQuaternionsAreClose(azQuaternionA, emQuaternionA, s_toleranceLow)) << "AZ/MCore Quaternions should have similar Lerp output with given float: " << testVal;
    }
}

TEST_F(EmotionFXMathLibTests, AZEMQuaternion_HasSimilarNLerpOutput_Success)
{
    // Test AZ and MCore quaternions to have similar Linear Interpolated and then normalized quaternions
    float testCases[6] = {0.0f, 0.1f, 0.25f, 0.5f, 0.8f, 1.0f};
    for (float testVal : testCases)
    {
        AZ::Quaternion azQuaternionA = AZ::Quaternion(0.1f, 0.2f, 0.3f, 1.0f).GetNormalized();
        AZ::Quaternion azQuaternionB = AZ::Quaternion(0.8f, 0.7f, 0.6f, 1.0f).GetNormalized();
        AZ::Quaternion azQuaternionC = azQuaternionA.NLerp(azQuaternionB, testVal);

        MCore::Quaternion emQuaternionA = MCore::Quaternion(0.1f, 0.2f, 0.3f, 1.0f).Normalized();
        MCore::Quaternion emQuaternionB = MCore::Quaternion(0.8f, 0.7f, 0.6f, 1.0f).Normalized();
        MCore::Quaternion emQuaternionC = emQuaternionA.NLerp(emQuaternionB, testVal);

        EXPECT_TRUE(AZEMQuaternionsAreClose(azQuaternionA, emQuaternionA, s_toleranceLow)) << "AZ/MCore Quaternions should have similar NLerp output with given float: " << testVal;
    }
}

TEST_F(EmotionFXMathLibTests, AZEMQuaternion_HasSimilarSLerpOutput_Success)
{
    // Test AZ and MCore quaternions to have similar spherical Linear Interpolated quaternions
    float testCases[6] = { 0.0f, 0.1f, 0.25f, 0.5f, 0.8f, 1.0f };
    for (float testVal : testCases)
    {
        AZ::Quaternion azQuaternionA = AZ::Quaternion(0.1f, 0.2f, 0.3f, 1.0f).GetNormalized();
        AZ::Quaternion azQuaternionB = AZ::Quaternion(0.8f, 0.7f, 0.6f, 1.0f).GetNormalized();
        AZ::Quaternion azQuaternionC = azQuaternionA.Slerp(azQuaternionB, testVal);

        MCore::Quaternion emQuaternionA = MCore::Quaternion(0.1f, 0.2f, 0.3f, 1.0f).Normalized();
        MCore::Quaternion emQuaternionB = MCore::Quaternion(0.8f, 0.7f, 0.6f, 1.0f).Normalized();
        MCore::Quaternion emQuaternionC = emQuaternionA.Slerp(emQuaternionB, testVal);

        EXPECT_TRUE(AZEMQuaternionsAreClose(azQuaternionA, emQuaternionA, s_toleranceLow)) << "AZ/MCore Quaternions should have similar Slerp output with given float: " << testVal;
    }
}

//////////////////////////////////////////////////////////////////
// Skinning
//////////////////////////////////////////////////////////////////

TEST_F(EmotionFXMathLibTests, AZTransform_Skin_Success)
{
    const AZ::Quaternion rotation(0.40f, 0.08f, 0.44f, 0.80f);
    const AZ::Vector3 translation(0.2f, 0.1f, -0.1f);
    const AZ::Matrix3x4 inMat = AZ::Matrix3x4::CreateFromQuaternionAndTranslation(rotation, translation);
    const AZ::Vector3 inPos(0.5f, 0.6f, 0.7f);
    const AZ::Vector3 inNormal(0.36f, -0.352f, 0.864f);
    AZ::Vector3 outPos = AZ::Vector3::CreateZero();
    AZ::Vector3 outNormal = AZ::Vector3::CreateZero();
    float weight = 0.123f;

    MCore::Skin(inMat, &inPos, &inNormal, &outPos, &outNormal, weight);

    EXPECT_TRUE(AZVector3CompareClose(outPos, 0.055596f, 0.032098f, 0.111349f, s_toleranceHigh));
    EXPECT_TRUE(AZVector3CompareClose(outNormal, 0.105288f, -0.039203f, 0.050066f, s_toleranceHigh));
}

TEST_F(EmotionFXMathLibTests, AZTransform_SkinWithTangent_Success)
{
    const AZ::Quaternion rotation(0.72f, 0.48f, 0.24f, 0.44f);
    const AZ::Vector3 translation(0.3f, -0.2f, 0.2f);
    const AZ::Matrix3x4 inMat = AZ::Matrix3x4::CreateFromQuaternionAndTranslation(rotation, translation);
    const AZ::Vector3 inPos(0.4f, 0.7f, 0.4f);
    const AZ::Vector3 inNormal(0.096f, 0.36f, 0.928f);
    const AZ::Vector4 inTangent = AZ::Vector4::CreateFromVector3AndFloat(AZ::Vector3::CreateAxisX().Cross(inNormal).GetNormalized(), 0.8f);
    AZ::Vector3 outPos = AZ::Vector3::CreateZero();
    AZ::Vector3 outNormal = AZ::Vector3::CreateZero();
    AZ::Vector4 outTangent = AZ::Vector4::CreateZero();
    float weight = 0.234f;

    MCore::Skin(inMat, &inPos, &inNormal, &inTangent, &outPos, &outNormal, &outTangent, weight);

    EXPECT_TRUE(AZVector3CompareClose(outPos, 0.260395f, -0.024972f, 0.134559f, s_toleranceHigh));
    EXPECT_TRUE(AZVector3CompareClose(outNormal, 0.216733f, -0.080089f, -0.036997f, s_toleranceHigh));
    EXPECT_TRUE(AZVector3CompareClose(outTangent.GetAsVector3(), -0.039720f, -0.000963f, -0.230602f, s_toleranceHigh));
    EXPECT_NEAR(outTangent.GetW(), inTangent.GetW(), s_toleranceHigh);
}

TEST_F(EmotionFXMathLibTests, AZTransform_SkinWithTangentAndBitangent_Success)
{
    const AZ::Quaternion rotation(0.72f, 0.64f, 0.12f, 0.24f);
    const AZ::Vector3 translation(0.1f, 0.2f, -0.1f);
    const AZ::Matrix3x4 inMat = AZ::Matrix3x4::CreateFromQuaternionAndTranslation(rotation, translation);
    const AZ::Vector3 inPos(0.2f, -0.3f, 0.5f);
    const AZ::Vector3 inNormal(0.768f, 0.024f, 0.64f);
    const AZ::Vector4 inTangent = AZ::Vector4::CreateFromVector3AndFloat(AZ::Vector3::CreateAxisX().Cross(inNormal).GetNormalized(), 0.6f);
    const AZ::Vector3 inBitangent = inNormal.Cross(inTangent.GetAsVector3());
    AZ::Vector3 outPos = AZ::Vector3::CreateZero();
    AZ::Vector3 outNormal = AZ::Vector3::CreateZero();
    AZ::Vector4 outTangent = AZ::Vector4::CreateZero();
    AZ::Vector3 outBitangent = AZ::Vector3::CreateZero();
    float weight = 0.345f;

    MCore::Skin(inMat, &inPos, &inNormal, &inTangent, &inBitangent, &outPos, &outNormal, &outTangent, &outBitangent, weight);

    EXPECT_TRUE(AZVector3CompareClose(outPos, 0.038364f, 0.110234f, -0.243101f, s_toleranceHigh));
    EXPECT_TRUE(AZVector3CompareClose(outNormal, 0.153412f, 0.216512f, -0.220482f, s_toleranceHigh));
    EXPECT_TRUE(AZVector3CompareClose(outTangent.GetAsVector3(), -0.291665f, 0.020134f, -0.183170f, s_toleranceHigh));
    EXPECT_NEAR(outTangent.GetW(), inTangent.GetW(), s_toleranceHigh);
    EXPECT_TRUE(AZVector3CompareClose(outBitangent, -0.102085f, 0.267847f, 0.191994f, s_toleranceHigh));
}

// Last test
TEST_F(EmotionFXMathLibTests, LastTest)
{
    ASSERT_TRUE(true);
}
