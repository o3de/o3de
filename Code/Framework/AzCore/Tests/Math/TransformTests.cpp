/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Quaternion.h>
#include <AZTestShared/Math/MathTestHelpers.h>
#include "MathTestData.h"

namespace UnitTest
{
    using TransformCreateFixture = ::testing::TestWithParam<AZ::Vector3>;

    TEST_P(TransformCreateFixture, CreateIdentity)
    {
        const AZ::Transform transform = AZ::Transform::CreateIdentity();
        const AZ::Vector3 vector = GetParam();
        EXPECT_THAT(transform.TransformPoint(vector), IsClose(vector));
    }

    TEST_P(TransformCreateFixture, Identity)
    {
        const AZ::Transform transform = AZ::Transform::Identity();
        const AZ::Vector3 vector = GetParam();
        EXPECT_THAT(transform.TransformPoint(vector), IsClose(vector));
    }

    TEST_P(TransformCreateFixture, CreateTranslation)
    {
        const AZ::Vector3 translation(0.8f, 2.3f, -1.9f);
        const AZ::Transform transform = AZ::Transform::CreateTranslation(translation);
        const AZ::Vector3 vector = GetParam();
        EXPECT_THAT(transform.TransformPoint(vector), IsClose(vector + translation));
    }

    INSTANTIATE_TEST_CASE_P(MATH_Transform, TransformCreateFixture, ::testing::ValuesIn(MathTestData::Vector3s));

    using TransformCreateRotationFixture = ::testing::TestWithParam<float>;

    TEST_P(TransformCreateRotationFixture, CreateRotationX)
    {
        const float angle = GetParam();
        AZ::Transform transform = AZ::Transform::CreateRotationX(angle);
        EXPECT_TRUE(transform.IsOrthogonal());
        const AZ::Vector3 vector(1.5f, -0.2f, 2.7f);
        const AZ::Vector3 rotatedVector = transform.TransformPoint(vector);
        // rotating a vector should not affect its length
        EXPECT_NEAR(rotatedVector.GetLengthSq(), vector.GetLengthSq(), AZ::Constants::Tolerance * vector.GetLengthSq());

        // rotating about the X axis should not affect the X component
        EXPECT_NEAR(rotatedVector.GetX(), vector.GetX(), AZ::Constants::Tolerance);

        // when projected into the Y-Z plane, the angle between the rotated vector and the original vector
        // should wrap to the same as the input angle parameter
        const float xSquared = vector.GetX() * vector.GetX();
        const float projectedDotProduct = rotatedVector.Dot(vector) - xSquared;
        const float projectedMagnitudeSq = vector.Dot(vector) - xSquared;
        EXPECT_NEAR(projectedDotProduct, projectedMagnitudeSq * cosf(angle), 1e-2f * projectedMagnitudeSq);
    }

    TEST_P(TransformCreateRotationFixture, CreateRotationY)
    {
        const float angle = GetParam();
        AZ::Transform transform = AZ::Transform::CreateRotationY(angle);
        EXPECT_TRUE(transform.IsOrthogonal());
        const AZ::Vector3 vector(1.5f, -0.2f, 2.7f);
        const AZ::Vector3 rotatedVector = transform.TransformPoint(vector);
        // rotating a vector should not affect its length
        EXPECT_NEAR(rotatedVector.GetLengthSq(), vector.GetLengthSq(), AZ::Constants::Tolerance * vector.GetLengthSq());

        // rotating about the Y axis should not affect the Y component
        EXPECT_NEAR(rotatedVector.GetY(), vector.GetY(), AZ::Constants::Tolerance);

        // when projected into the X-Z plane, the angle between the rotated vector and the original vector
        // should wrap to the same as the input angle parameter
        const float ySquared = vector.GetY() * vector.GetY();
        const float projectedDotProduct = rotatedVector.Dot(vector) - ySquared;
        const float projectedMagnitudeSq = vector.Dot(vector) - ySquared;
        EXPECT_NEAR(projectedDotProduct, projectedMagnitudeSq * cosf(angle), 1e-2f * projectedMagnitudeSq);
    }

    TEST_P(TransformCreateRotationFixture, CreateRotationZ)
    {
        const float angle = GetParam();
        AZ::Transform transform = AZ::Transform::CreateRotationZ(angle);
        EXPECT_TRUE(transform.IsOrthogonal());
        const AZ::Vector3 vector(1.5f, -0.2f, 2.7f);
        const AZ::Vector3 rotatedVector = transform.TransformPoint(vector);
        // rotating a vector should not affect its length
        EXPECT_NEAR(rotatedVector.GetLengthSq(), vector.GetLengthSq(), AZ::Constants::Tolerance * vector.GetLengthSq());

        // rotating about the Z axis should not affect the Z component
        EXPECT_NEAR(rotatedVector.GetZ(), vector.GetZ(), AZ::Constants::Tolerance);

        // when projected into the X-Y plane, the angle between the rotated vector and the original vector
        // should wrap to the same as the input angle parameter
        const float zSquared = vector.GetZ() * vector.GetZ();
        const float projectedDotProduct = rotatedVector.Dot(vector) - zSquared;
        const float projectedMagnitudeSq = vector.Dot(vector) - zSquared;
        EXPECT_NEAR(projectedDotProduct, projectedMagnitudeSq * cosf(angle), 1e-2f * projectedMagnitudeSq);
    }

    INSTANTIATE_TEST_CASE_P(MATH_Transform, TransformCreateRotationFixture, ::testing::ValuesIn(MathTestData::Angles));

    TEST(MATH_Transform, GetSetTranslation)
    {
        const AZ::Vector3 inputTranslation(-1.2f, -0.2f, 1.9f);
        const float x = -2.7f;
        const float y = -0.7f;
        const float z = -1.2f;
        AZ::Transform transform;
        transform.SetTranslation(inputTranslation);
        EXPECT_THAT(transform.GetTranslation(), IsClose(inputTranslation));
        transform.SetTranslation(x, y, z);
        EXPECT_THAT(transform.GetTranslation(), IsClose(AZ::Vector3(x, y, z)));
    }

    using TransformCreateFromQuaternionFixture = ::testing::TestWithParam<AZ::Quaternion>;

    TEST_P(TransformCreateFromQuaternionFixture, CreateFromQuaternion)
    {
        const AZ::Quaternion quaternion = GetParam();
        const AZ::Transform transform = AZ::Transform::CreateFromQuaternion(quaternion);
        EXPECT_THAT(transform.GetTranslation(), IsClose(AZ::Vector3::CreateZero()));
        const AZ::Vector3 vector(2.3f, -0.6, 1.8f);
        EXPECT_THAT(transform.TransformPoint(vector), IsClose(quaternion.TransformVector(vector)));
    }

    TEST_P(TransformCreateFromQuaternionFixture, CreateFromQuaternionAndTranslation)
    {
        const AZ::Quaternion quaternion = GetParam();
        const AZ::Vector3 translation(-2.6f, 1.7f, 0.8f);
        const AZ::Transform transform = AZ::Transform::CreateFromQuaternionAndTranslation(quaternion, translation);
        EXPECT_THAT(transform.GetTranslation(), IsClose(translation));
        const AZ::Vector3 vector(2.3f, -0.6, 1.8f);
        EXPECT_THAT(transform.TransformPoint(vector), IsClose(quaternion.TransformVector(vector) + translation));
    }

    TEST_P(TransformCreateFromQuaternionFixture, SetRotation)
    {
        const AZ::Quaternion quaternion = GetParam();
        AZ::Transform transform = AZ::Transform::CreateIdentity();
        transform.SetRotation(quaternion);
        const AZ::Vector3 vector(2.3f, -0.6, 1.8f);
        EXPECT_THAT(transform.TransformPoint(vector), IsClose(quaternion.TransformVector(vector)));
    }

    INSTANTIATE_TEST_CASE_P(MATH_Transform, TransformCreateFromQuaternionFixture, ::testing::ValuesIn(MathTestData::UnitQuaternions));

    TEST(MATH_Transform, CreateUniformScale)
    {
        const float scale = 1.7f;
        const AZ::Transform transform = AZ::Transform::CreateUniformScale(scale);
        const AZ::Vector3 vector(0.2f, -1.6f, 0.4f);
        EXPECT_THAT(transform.GetTranslation(), IsClose(AZ::Vector3::CreateZero()));
        const AZ::Vector3 transformedVector = transform.TransformPoint(vector);
        const AZ::Vector3 expected(0.34f, -2.72f, 0.68f);
        EXPECT_THAT(transformedVector, IsClose(expected));
    }

    using TransformCreateLookAtFixture = ::testing::TestWithParam<MathTestData::AxisPair>;

    TEST_P(TransformCreateLookAtFixture, CreateLookAt)
    {
        const AZ::Transform::Axis axis = GetParam().first;
        const AZ::Vector3 axisDirection = GetParam().second;
        const AZ::Vector3 from(2.5f, 0.2f, 3.6f);
        const AZ::Vector3 to(1.3f, 0.5f, 3.2f);
        const AZ::Vector3 expectedForward = to - from;
        const AZ::Transform transform = AZ::Transform::CreateLookAt(from, to, axis);
        EXPECT_TRUE(transform.IsOrthogonal());
        EXPECT_THAT(transform.GetBasisX().Cross(transform.GetBasisY()), IsClose(transform.GetBasisZ()));
        EXPECT_THAT(transform.GetTranslation(), IsClose(from));
        // the column of the transform corresponding to the axis direction should be parallel to the expected forward direction
        const AZ::Vector3 forward = transform.TransformVector(axisDirection);
        EXPECT_THAT(forward, IsClose(expectedForward.GetNormalized()));
    }

    INSTANTIATE_TEST_CASE_P(MATH_Transform, TransformCreateLookAtFixture, ::testing::ValuesIn(MathTestData::Axes));

    using TransformFromMatrixFixture = ::testing::TestWithParam<AZ::Matrix3x4>;

    TEST_P(TransformFromMatrixFixture, ReconstructMatrixFromRetrievedTransformScale)
    {
        AZ::Matrix3x4 matrix = GetParam();
        AZ::Transform retrievedTransform = AZ::Transform::CreateFromMatrix3x4(matrix);
        retrievedTransform.ExtractUniformScale();
        AZ::Vector3 retrievedNonUniformScale = matrix.RetrieveScale();

        AZ::Matrix3x4 reconstructedMatrix = AZ::Matrix3x4::CreateFromTransform(retrievedTransform);
        reconstructedMatrix.MultiplyByScale(retrievedNonUniformScale);

        EXPECT_THAT(matrix, IsClose(reconstructedMatrix));
    }

    INSTANTIATE_TEST_CASE_P(MATH_Transform, TransformFromMatrixFixture, ::testing::ValuesIn(MathTestData::NonOrthogonalMatrix3x4s));

    TEST(MATH_Transform, CreateLookAtDegenerateCases)
    {
        const AZ::Vector3 from(2.5f, 0.2f, 3.6f);
        // to and from are the same, should generate an error
        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_THAT(AZ::Transform::CreateLookAt(from, from), IsClose(AZ::Transform::Identity()));
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);

        // to - from is parallel to usual up direction
        const AZ::Vector3 to(2.5f, 0.2f, 5.2f);
        const AZ::Transform transform = AZ::Transform::CreateLookAt(from, to);
        EXPECT_TRUE(transform.IsOrthogonal());
        EXPECT_THAT(transform.GetTranslation(), IsClose(from));

        // the default is for the Y basis of the look at transform to be the forward direction
        const AZ::Vector3 axisDirection = AZ::Vector3::CreateAxisY();
        const AZ::Vector3 forwardDirection = (to - from).GetNormalized();

        EXPECT_THAT(transform.TransformVector(axisDirection), IsClose(forwardDirection));
    }

    TEST(MATH_Transform, MultiplyByTransform)
    {
        const AZ::Transform transform1 = AZ::Transform::CreateRotationY(0.3f);
        const AZ::Transform transform2 = AZ::Transform::CreateUniformScale(1.3f);
        const AZ::Transform transform3 = AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion(0.42f, 0.46f, -0.66f, 0.42f), AZ::Vector3(2.8f, -3.7f, 1.6f));
        const AZ::Transform transform4 = AZ::Transform::CreateRotationX(-0.7f) * AZ::Transform::CreateUniformScale(0.6f);
        AZ::Transform transform5 = transform1;
        transform5 *= transform4;
        const AZ::Vector3 vector(1.9f, 2.3f, 0.2f);
        EXPECT_THAT((transform1 * (transform2 * transform3)), IsClose((transform1 * transform2) * transform3));
        EXPECT_THAT((transform3 * transform4).TransformPoint(vector), IsClose(transform3.TransformPoint((transform4.TransformPoint(vector)))));
        EXPECT_THAT((transform2 * AZ::Transform::Identity()), IsClose(transform2));
        EXPECT_THAT((transform3 * AZ::Transform::Identity()), IsClose(AZ::Transform::Identity() * transform3));
        EXPECT_THAT(transform5, IsClose(transform1 * transform4));
    }

    TEST(MATH_Transform, TranslationCorrectInTransformHierarchy)
    {
        AZ::Transform parent = AZ::Transform::CreateRotationZ(AZ::DegToRad(45.0f));
        parent.SetUniformScale(3.0f);
        parent.SetTranslation(AZ::Vector3(0.2f, 0.3f, 0.4f));
        AZ::Transform child = AZ::Transform::CreateRotationZ(AZ::DegToRad(90.0f));
        child.SetTranslation(AZ::Vector3(0.5f, 0.6f, 0.7f));
        const AZ::Transform overallTransform = parent * child;
        const AZ::Vector3 overallTranslation = overallTransform.GetTranslation();
        const AZ::Vector3 expectedTranslation(-0.012132f, 2.633452f, 2.5f);
        EXPECT_THAT(overallTranslation, IsClose(expectedTranslation));
    }

    TEST(MATH_Transform, TransformPointVector3)
    {
        AZ::Transform transform = AZ::Transform::CreateIdentity();
        transform.SetFromEulerDegrees(AZ::Vector3(10.0f, 20.0f, 30.0f));
        const AZ::Vector3 translation1 = AZ::Vector3(1.0f, 2.0f, 3.0f);
        transform.SetTranslation(translation1);
        const AZ::Vector3 vector(0.2f, 0.1f, -0.3f);
        const AZ::Vector3 expected1(1.01317f, 2.24004f, 2.71328f);
        EXPECT_THAT(transform.TransformPoint(vector), IsClose(expected1));
        // MultiplyByVector3 should be affected by the translation part of the Transform
        const AZ::Vector3 translation2 = AZ::Vector3(-4.0f, -5.0f, 2.0f);
        transform.SetTranslation(translation2);
        const AZ::Vector3 expected2 = expected1 + translation2 - translation1;
        EXPECT_THAT(transform.TransformPoint(vector), IsClose(expected2));
    }

    TEST(MATH_Transform, TransformVector)
    {
        AZ::Transform transform = AZ::Transform::CreateIdentity();
        transform.SetFromEulerDegrees(AZ::Vector3(10.0f, 20.0f, 30.0f));
        const AZ::Vector3 translation1 = AZ::Vector3(1.0f, 2.0f, 3.0f);
        transform.SetTranslation(translation1);
        const AZ::Vector3 vector(0.2f, 0.1f, -0.3f);
        const AZ::Vector3 expected(0.01317f, 0.24004f, -0.28672f);
        EXPECT_THAT(transform.TransformVector(vector), IsClose(expected));
        // the result of TransformVector should not be affected by translation
        const AZ::Vector3 translation2 = AZ::Vector3(-4.0f, -5.0f, 2.0f);
        transform.SetTranslation(translation2);
        EXPECT_THAT(transform.TransformVector(vector), IsClose(expected));
    }

    TEST(MATH_Transform, TransformPointVector4)
    {
        AZ::Transform transform = AZ::Transform::CreateIdentity();
        transform.SetFromEulerDegrees(AZ::Vector3(10.0f, 20.0f, 30.0f));
        const AZ::Vector3 translation = AZ::Vector3(1.0f, 2.0f, 3.0f);
        transform.SetTranslation(translation);
        const AZ::Vector4 vector(0.4f, -1.0f, -0.2f, 0.3f);
        const AZ::Vector4 product = transform.TransformPoint(vector);
        const AZ::Vector4 expected(1.02696f, 0.02700f, 0.31417f, 0.3f);
        EXPECT_THAT(product, IsClose(expected));
    }

    using TransformInvertFixture = ::testing::TestWithParam<AZ::Transform>;

    TEST_P(TransformInvertFixture, GetInverse)
    {
        const AZ::Transform transform = GetParam();
        const AZ::Transform inverse = transform.GetInverse();
        const AZ::Vector3 vector(0.9f, 3.2f, -1.4f);
        EXPECT_THAT((inverse * transform).TransformPoint(vector), IsClose(vector));
        EXPECT_THAT((transform * inverse).TransformPoint(vector), IsClose(vector));
        EXPECT_THAT((inverse * transform), IsClose(AZ::Transform::Identity()));
    }

    TEST_P(TransformInvertFixture, Invert)
    {
        const AZ::Transform transform = GetParam();
        AZ::Transform inverse = transform;
        inverse.Invert();
        const AZ::Vector3 vector(2.8f, -1.3f, 2.6f);
        EXPECT_THAT((inverse * transform).TransformPoint(vector), IsClose(vector));
        EXPECT_THAT((transform * inverse).TransformPoint(vector), IsClose(vector));
        EXPECT_THAT((inverse * transform), IsClose(AZ::Transform::Identity()));
    }

    INSTANTIATE_TEST_CASE_P(MATH_Transform, TransformInvertFixture, ::testing::ValuesIn(MathTestData::OrthogonalTransforms));

    using TransformScaleFixture = ::testing::TestWithParam<AZ::Transform>;

    TEST_P(TransformScaleFixture, Scale)
    {
        const AZ::Transform orthogonalTransform = GetParam();
        EXPECT_NEAR(orthogonalTransform.GetUniformScale(), 1.0f, AZ::Constants::Tolerance);
        AZ::Transform unscaledTransform = orthogonalTransform;
        unscaledTransform.ExtractUniformScale();
        EXPECT_NEAR(unscaledTransform.GetUniformScale(), 1.0f, AZ::Constants::Tolerance);
        const float scale = 2.8f;
        AZ::Transform scaledTransform = orthogonalTransform;
        scaledTransform.MultiplyByUniformScale(scale);
        EXPECT_NEAR(scaledTransform.GetUniformScale(), scale, AZ::Constants::Tolerance);
    }

    INSTANTIATE_TEST_CASE_P(MATH_Transform, TransformScaleFixture, ::testing::ValuesIn(MathTestData::OrthogonalTransforms));

    TEST(MATH_Transform, IsOrthogonal)
    {
        EXPECT_TRUE(AZ::Transform::CreateIdentity().IsOrthogonal());
        EXPECT_TRUE(AZ::Transform::CreateRotationZ(0.3f).IsOrthogonal());
        EXPECT_FALSE(AZ::Transform::CreateUniformScale(0.8f).IsOrthogonal());
        EXPECT_TRUE(AZ::Transform::CreateFromQuaternion(AZ::Quaternion(-0.52f, -0.08f, 0.56f, 0.64f)).IsOrthogonal());
        AZ::Transform transform;
        transform.SetFromEulerRadians(AZ::Vector3(0.2f, 0.4f, 0.1f));
        EXPECT_TRUE(transform.IsOrthogonal());
    }

    using TransformSetFromEulerDegreesFixture = ::testing::TestWithParam<AZ::Vector3>;

    TEST_P(TransformSetFromEulerDegreesFixture, SetFromEulerDegrees)
    {
        const AZ::Vector3 eulerDegrees = GetParam();
        AZ::Transform transform;
        transform.SetFromEulerDegrees(eulerDegrees);
        const AZ::Vector3 eulerRadians = AZ::Vector3DegToRad(eulerDegrees);
        const AZ::Transform rotX = AZ::Transform::CreateRotationX(eulerRadians.GetX());
        const AZ::Transform rotY = AZ::Transform::CreateRotationY(eulerRadians.GetY());
        const AZ::Transform rotZ = AZ::Transform::CreateRotationZ(eulerRadians.GetZ());
        EXPECT_THAT(transform, IsClose(rotX * rotY * rotZ));
        transform.SetFromEulerDegrees(eulerDegrees);
        EXPECT_THAT(transform, IsClose(rotX * rotY * rotZ));
    }

    INSTANTIATE_TEST_CASE_P(MATH_Transform, TransformSetFromEulerDegreesFixture, ::testing::ValuesIn(MathTestData::EulerAnglesDegrees));

    using TransformSetFromEulerRadiansFixture = ::testing::TestWithParam<AZ::Vector3>;

    TEST_P(TransformSetFromEulerRadiansFixture, SetFromEulerRadians)
    {
        const AZ::Vector3 eulerRadians = GetParam();
        AZ::Transform transform;
        transform.SetFromEulerRadians(eulerRadians);
        const AZ::Transform rotX = AZ::Transform::CreateRotationX(eulerRadians.GetX());
        const AZ::Transform rotY = AZ::Transform::CreateRotationY(eulerRadians.GetY());
        const AZ::Transform rotZ = AZ::Transform::CreateRotationZ(eulerRadians.GetZ());
        EXPECT_THAT(transform, IsClose(rotX * rotY * rotZ));
    }

    INSTANTIATE_TEST_CASE_P(MATH_Transform, TransformSetFromEulerRadiansFixture, ::testing::ValuesIn(MathTestData::EulerAnglesRadians));

    using TransformGetEulerFixture = ::testing::TestWithParam<AZ::Transform>;

    TEST_P(TransformGetEulerFixture, GetEuler)
    {
        // there isn't a one to one mapping between matrices and Euler angles, so testing for a particular set of Euler
        // angles to be returned would be fragile, but getting the Euler angles and creating a new transform from them
        // should return the original transform
        AZ::Transform transform = GetParam();
        transform.SetTranslation(AZ::Vector3::CreateZero());
        const AZ::Vector3 eulerDegrees = transform.GetEulerDegrees();
        AZ::Transform eulerTransform;
        eulerTransform.SetFromEulerDegrees(eulerDegrees);
        EXPECT_THAT(eulerTransform, IsClose(transform));
        const AZ::Vector3 eulerRadians = transform.GetEulerRadians();
        eulerTransform = AZ::Transform::Identity();
        eulerTransform.SetFromEulerRadians(eulerRadians);
        EXPECT_THAT(eulerTransform, IsClose(transform));
    }

    INSTANTIATE_TEST_CASE_P(MATH_Transform, TransformGetEulerFixture, ::testing::ValuesIn(MathTestData::OrthogonalTransforms));

    class MATH_TransformApplicationFixture
        : public LeakDetectionFixture
    {
    public:
        MATH_TransformApplicationFixture()
            : LeakDetectionFixture()
        {
        }

    protected:
        void SetUp() override
        {
            LeakDetectionFixture::SetUp();
            AZ::ComponentApplication::Descriptor desc;
            desc.m_useExistingAllocator = true;
            m_app.reset(aznew AZ::ComponentApplication);
            AZ::ComponentApplication::StartupParameters startupParameters;
            startupParameters.m_loadSettingsRegistry = false;
            m_app->Create(desc, startupParameters);
        }

        void TearDown() override
        {
            m_app->Destroy();
            m_app.reset();
            LeakDetectionFixture::TearDown();
        }

        AZStd::unique_ptr<AZ::ComponentApplication> m_app;
    };

    TEST_F(MATH_TransformApplicationFixture, DeserializingOldFormat)
    {
        const char* objectStreamBuffer =
            R"DELIMITER(<ObjectStream version="3">
            <Class name="Transform" field="m_data" value="0.79429845 0.8545947 -0.94273965 -0.1610121 1.1699124 0.92486745 1.2622065 -0.3885522 0.71123985 513.7845459 492.5420837 32.0000000" type="{5D9958E9-9F1E-4985-B532-FFFDE75FEDFD}"/>
            </ObjectStream>)DELIMITER";

        AZ::Transform* deserializedTransform = AZ::Utils::LoadObjectFromBuffer<AZ::Transform>(objectStreamBuffer, strlen(objectStreamBuffer) + 1);

        const AZ::Vector3 expectedTranslation(513.7845459f, 492.5420837f, 32.0000000f);
        const float expectedScale = 1.5f;
        const AZ::Quaternion expectedRotation(0.2624075f, 0.4405251f, 0.2029076f, 0.8342113f);
        const AZ::Transform expectedTransform =
            AZ::Transform::CreateFromQuaternionAndTranslation(expectedRotation, expectedTranslation) *
            AZ::Transform::CreateUniformScale(expectedScale);

        EXPECT_TRUE(deserializedTransform->IsClose(expectedTransform));
        azfree(deserializedTransform);
    }

    TEST(MATH_Transform, IsCloseRespectsTranslationTolerance)
    {
        AZ::Transform transformA = AZ::Transform::CreateTranslation(AZ::Vector3(0.001f, 0.0f, 0.0f));
        AZ::Transform transformB = AZ::Transform::CreateTranslation(AZ::Vector3(0.003f, 0.0f, 0.0f));

        // default tolerance fails (using Constants::Tolerance)
        EXPECT_FALSE(transformA.IsClose(transformB));
        // precise custom tolerance fails
        EXPECT_FALSE(transformA.IsClose(transformB, 0.001f));
        // relaxed custom tolerance passes
        EXPECT_TRUE(transformA.IsClose(transformB, 0.01f));
    }
} // namespace UnitTest
