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
#include <AZTestShared/Math/MathTestHelpers.h>

#include <MCore/Source/Vector.h>
#include <MCore/Source/AzCoreConversions.h>

//Right Hand - counterclockwise looking down axis from positive side
TEST(EmotionFXMathLibTests, AZQuaternion_Rotation1ComponentAxisX_Success)
{
    AZ::Vector3 axis = AZ::Vector3(1.0f, 0.0f, 0.0f);
    axis.Normalize();
    AZ::Quaternion azQuaternion1 = AZ::Quaternion::CreateFromAxisAngle(axis, AZ::Constants::HalfPi);
    AZ::Vector3 vertexIn(0.0f, 0.0f, 0.1f);
    AZ::Vector3 vertexOut;
    vertexOut = azQuaternion1.TransformVector(vertexIn);

    EXPECT_THAT(vertexOut, UnitTest::IsClose(AZ::Vector3(0.0f, -0.1f, 0.0f)));
}

TEST(EmotionFXMathLibTests, AZQuaternion_Rotation1ComponentAxisY_Success)
{
    AZ::Vector3 axis = AZ::Vector3(0.0f, 1.0f, 0.0f);
    axis.Normalize();
    AZ::Quaternion azQuaternion1 = AZ::Quaternion::CreateFromAxisAngle(axis, AZ::Constants::HalfPi);
    AZ::Vector3 vertexIn(0.1f, 0.0f, 0.0f);
    AZ::Vector3 vertexOut;
    vertexOut = azQuaternion1.TransformVector(vertexIn);

    EXPECT_THAT(vertexOut, UnitTest::IsClose(AZ::Vector3(0.0f, 0.0f, -0.1f)));
}

TEST(EmotionFXMathLibTests, AZQuaternion_Rotation1ComponentAxisZ_Success)
{
    AZ::Vector3 axis = AZ::Vector3(0.0f, 0.0f, 1.0f);
    axis.Normalize();
    AZ::Quaternion azQuaternion1 = AZ::Quaternion::CreateFromAxisAngle(axis, AZ::Constants::HalfPi);
    AZ::Vector3 vertexIn(0.1f, 0.0f, 0.0f);
    AZ::Vector3 vertexOut;
    vertexOut = azQuaternion1.TransformVector(vertexIn);

    EXPECT_THAT(vertexOut, UnitTest::IsClose(AZ::Vector3(0.0f, 0.1f, 0.0f)));
}

//AZ Quaternion Normalize Vertex test
TEST(EmotionFXMathLibTests, AZazQuaternion_NormalizedQuaternionRotationTest3DAxis_Success)
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

    EXPECT_THAT(vertexOut1, UnitTest::IsClose(vertexOut1FromNormalizedQuaternion));
}


///////////////////////////////////////////////////////////////////////////////
// Euler  AZ
///////////////////////////////////////////////////////////////////////////////

// AZ Quaternion <-> euler conversion Vertex test 1 component axis
TEST(EmotionFXMathLibTests, AZQuaternion_EulerGetSet1ComponentAxis_Success)
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

    EXPECT_THAT(vertexOut1, UnitTest::IsClose(vertexOut2));
}

// AZ Quaternion <-> euler conversion Vertex test 2 component axis
TEST(EmotionFXMathLibTests, AZQuaternion_EulerGetSet2ComponentAxis_Success)
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

    EXPECT_THAT(vertexOut1, UnitTest::IsClose(vertexOut2));
}

// AZ Quaternion <-> euler conversion Vertex test 3 component axis
TEST(EmotionFXMathLibTests, AZQuaternion_EulerInOutRotationTest3DAxis_Success)
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

    EXPECT_THAT(vertexOut1, UnitTest::IsClose(vertexOut2));
}

// Quaternion -> Transform -> Euler conversion is same as Quaternion -> Euler
// AZ Euler get set Transform Compare test 3 dim axis
TEST(EmotionFXMathLibTests, AZQuaternion_EulerGetSet3ComponentAxisCompareTransform_Success)
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

    EXPECT_THAT(vertexOut1, UnitTest::IsClose(vertexTransform));
    EXPECT_THAT(vertexOut1, UnitTest::IsClose(vertexOut2));
    EXPECT_THAT(vertexOut2, UnitTest::IsClose(vertexTransform));
}

// AZ Quaternion to Euler test
//only way to test Quaternions sameness is to apply it to a vector and measure result
TEST(EmotionFXMathLibTests, AZQuaternionConversion_ToEulerEquivalent_Success)
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
    EXPECT_THAT(eulerOut1, UnitTest::IsClose(eulerOut2));
}

///////////////////////////////////////////////////////////////////////////////
//  Quaternion Matrix
///////////////////////////////////////////////////////////////////////////////

// Test EM Quaternion made from Matrix X
TEST(EmotionFXMathLibTests, AZQuaternionConversion_FromAZTransformXRot_Success)
{
    AZ::Transform azTransform = AZ::Transform::CreateRotationX(AZ::Constants::HalfPi);
    AZ::Quaternion azQuaternion = azTransform.GetRotation();

    AZ::Vector3 emVertexIn(0.0f, 0.1f, 0.0f);
    AZ::Vector3 emVertexOut = azQuaternion.TransformVector(emVertexIn);

    EXPECT_THAT(emVertexOut, UnitTest::IsClose(AZ::Vector3(0.0f, 0.0f, 0.1f)));
}

// Test EM Quaternion made from Matrix Y
TEST(EmotionFXMathLibTests, AZQuaternionConversion_FromAZTransformYRot_Success)
{
    AZ::Transform azTransform = AZ::Transform::CreateRotationY(AZ::Constants::HalfPi);
    AZ::Quaternion azQuaternion = azTransform.GetRotation();

    AZ::Vector3 emVertexIn(0.0f, 0.0f, 0.1f);
    AZ::Vector3 emVertexOut = azQuaternion.TransformVector(emVertexIn);

    EXPECT_THAT(emVertexOut, UnitTest::IsClose(AZ::Vector3(0.1f, 0.0f, 0.0f)));
}

// Compare Quaternion made from Matrix X
TEST(EmotionFXMathLibTests, AZQuaternionConversion_FromMatrixXRot_Success)
{
    AZ::Matrix4x4 azMatrix = AZ::Matrix4x4::CreateRotationX(AZ::Constants::HalfPi);
    AZ::Quaternion azQuaternion = AZ::Quaternion::CreateFromMatrix4x4(azMatrix);

    AZ::Transform azTransform = AZ::Transform::CreateRotationX(AZ::Constants::HalfPi);
    AZ::Quaternion azQuaternionFromTransform = azTransform.GetRotation();

    AZ::Vector3 azVertexIn(0.0f, 0.1f, 0.0f);

    AZ::Vector3 azVertexOut = azQuaternion.TransformVector(azVertexIn);
    AZ::Vector3 emVertexOut = azQuaternionFromTransform.TransformVector(azVertexIn);

    EXPECT_THAT(azVertexOut, UnitTest::IsClose(emVertexOut));
}

// Compare Quaternion made from Matrix Y
TEST(EmotionFXMathLibTests, AZQuaternionConversion_FromMatrixYRot_Success)
{
    AZ::Matrix4x4 azMatrix = AZ::Matrix4x4::CreateRotationY(AZ::Constants::HalfPi);
    AZ::Quaternion azQuaternion = AZ::Quaternion::CreateFromMatrix4x4(azMatrix);

    AZ::Transform azTransform = AZ::Transform::CreateRotationY(AZ::Constants::HalfPi);
    AZ::Quaternion azQuaternionFromTransform = azTransform.GetRotation();

    AZ::Vector3 azVertexIn(0.1f, 0.0f, 0.0f);
    AZ::Vector3 azVertexOut = azQuaternion.TransformVector(azVertexIn);
    AZ::Vector3 emVertexOut = azQuaternionFromTransform.TransformVector(azVertexIn);

    EXPECT_THAT(azVertexOut, UnitTest::IsClose(emVertexOut));
}

// Compare Quaternion made from Matrix Z
TEST(EmotionFXMathLibTests, AZQuaternionConversion_FromMatrixZRot_Success)
{
    AZ::Matrix4x4 azMatrix = AZ::Matrix4x4::CreateRotationZ(AZ::Constants::HalfPi);
    AZ::Quaternion azQuaternion = AZ::Quaternion::CreateFromMatrix4x4(azMatrix);

    AZ::Transform azTransform = AZ::Transform::CreateRotationZ(AZ::Constants::HalfPi);
    AZ::Quaternion azQuaternionFromTransform = azTransform.GetRotation();

    AZ::Vector3 azVertexIn(0.1f, 0.0f, 0.0f);

    AZ::Vector3 azVertexOut = azQuaternion.TransformVector(azVertexIn);
    AZ::Vector3 emVertexOut = azQuaternionFromTransform.TransformVector(azVertexIn);

    EXPECT_THAT(azVertexOut, UnitTest::IsClose(emVertexOut));
}

// Compare Quaternion -> Matrix conversion
// AZ - column major
// Emfx - row major
TEST(EmotionFXMathLibTests, AZQuaternionConversion_ToMatrix_Success)
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
            if (!AZ::IsClose(emValue, azValue, 0.001f))
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
    EXPECT_NEAR(azMatrix.GetElement(3, 0), 0.0f, 0.001f);
    EXPECT_NEAR(azMatrix.GetElement(3, 1), 0.0f, 0.001f);
    EXPECT_NEAR(azMatrix.GetElement(3, 2), 0.0f, 0.001f);
    EXPECT_NEAR(azMatrix.GetElement(3, 3), 1.0f, 0.001f);
}

//////////////////////////////////////////////////////////////////
// Skinning
//////////////////////////////////////////////////////////////////

TEST(EmotionFXMathLibTests, AZTransform_Skin_Success)
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

    EXPECT_THAT(outPos, UnitTest::IsCloseTolerance(AZ::Vector3(0.055596f, 0.032098f, 0.111349f), 0.00001f));
    EXPECT_THAT(outNormal, UnitTest::IsCloseTolerance(AZ::Vector3(0.105288f, -0.039203f, 0.050066f), 0.00001f));
}

TEST(EmotionFXMathLibTests, AZTransform_SkinWithTangent_Success)
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

    EXPECT_THAT(outPos, UnitTest::IsCloseTolerance(AZ::Vector3(0.260395f, -0.024972f, 0.134559f), 0.00001f));
    EXPECT_THAT(outNormal, UnitTest::IsCloseTolerance(AZ::Vector3(0.216733f, -0.080089f, -0.036997f), 0.00001f));
    EXPECT_THAT(outTangent.GetAsVector3(), UnitTest::IsCloseTolerance(AZ::Vector3(-0.039720f, -0.000963f, -0.230602f), 0.00001f));
    EXPECT_NEAR(outTangent.GetW(), inTangent.GetW(), 0.00001f);
}

TEST(EmotionFXMathLibTests, AZTransform_SkinWithTangentAndBitangent_Success)
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

    EXPECT_THAT(outPos, UnitTest::IsCloseTolerance(AZ::Vector3(0.038364f, 0.110234f, -0.243101f), 0.00001f));
    EXPECT_THAT(outNormal, UnitTest::IsCloseTolerance(AZ::Vector3(0.153412f, 0.216512f, -0.220482f), 0.00001f));
    EXPECT_THAT(outTangent.GetAsVector3(), UnitTest::IsCloseTolerance(AZ::Vector3(-0.291665f, 0.020134f, -0.183170f), 0.00001f));
    EXPECT_NEAR(outTangent.GetW(), inTangent.GetW(), 0.00001f);
    EXPECT_THAT(outBitangent, UnitTest::IsCloseTolerance(AZ::Vector3(-0.102085f, 0.267847f, 0.191994f), 0.00001f));
}

