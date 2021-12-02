/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/MathUtils.h>

#include <MCore/Source/Vector.h>
#include <MCore/Source/AzCoreConversions.h>

class EmotionFXMathLibTests
    : public ::testing::Test
{
protected:

    void SetUp() override
    {
        m_azNormalizedVector3A = AZ::Vector3(s_x1, s_y1, s_z1);
        m_azNormalizedVector3A.Normalize();
        m_azQuaternionA = AZ::Quaternion::CreateFromAxisAngle(m_azNormalizedVector3A, s_angle_a);
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

    static const float  s_toleranceHigh;
    static const float  s_toleranceMedium;
    static const float  s_toleranceLow;
    static const float  s_toleranceReallyLow;
    static const float  s_x1;
    static const float  s_y1;
    static const float  s_z1;
    static const float  s_angle_a;
    AZ::Vector3         m_azNormalizedVector3A;
    AZ::Quaternion      m_azQuaternionA;
};

const float EmotionFXMathLibTests::s_toleranceHigh = 0.00001f;
const float EmotionFXMathLibTests::s_toleranceMedium = 0.0001f;
const float EmotionFXMathLibTests::s_toleranceLow = 0.001f;
const float EmotionFXMathLibTests::s_toleranceReallyLow = 0.02f;

const float EmotionFXMathLibTests::s_x1 = 0.2f;
const float EmotionFXMathLibTests::s_y1 = 0.3f;
const float EmotionFXMathLibTests::s_z1 = 0.4f;
const float EmotionFXMathLibTests::s_angle_a = 0.5f;

//////////////////////////////////////////////////////////////////
//Getting and setting of Quaternions
//////////////////////////////////////////////////////////////////

TEST_F(EmotionFXMathLibTests, AZQuaternionGet_Elements_Success)
{
    AZ::Quaternion test(0.1f, 0.2f, 0.3f, 0.4f);
    ASSERT_TRUE(AZQuaternionCompareExact(test, 0.1f, 0.2f, 0.3f, 0.4f));
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
