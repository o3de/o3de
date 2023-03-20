/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <gtest/gtest.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <cmath>

#include <Tests/Printers.h>
#include <Tests/Matchers.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Quaternion.h>
#include <MCore/Source/AzCoreConversions.h>
#include <EMotionFX/Source/PlayBackInfo.h>

#include <EMotionFX/Source/Transform.h>

#if defined(EMFX_SCALE_DISABLED)
#define EMFX_SCALE false
#else
#define EMFX_SCALE true
#endif

namespace EMotionFX
{
    static const float sqrt2 = std::sqrt(2.0f);
    static const float sqrt2over2 = sqrt2 / 2.0f;

    AZ::Matrix3x3 TensorProduct(const AZ::Vector3& u, const AZ::Vector3& v)
    {
        AZ::Matrix3x3 mat{};
        mat.SetElement(0, 0, u.GetX() * v.GetX());
        mat.SetElement(0, 1, u.GetX() * v.GetY());
        mat.SetElement(0, 2, u.GetX() * v.GetZ());
        mat.SetElement(1, 0, u.GetY() * v.GetX());
        mat.SetElement(1, 1, u.GetY() * v.GetY());
        mat.SetElement(1, 2, u.GetY() * v.GetZ());
        mat.SetElement(2, 0, u.GetZ() * v.GetX());
        mat.SetElement(2, 1, u.GetZ() * v.GetY());
        mat.SetElement(2, 2, u.GetZ() * v.GetZ());
        return mat;
    }

    TEST(TransformFixture, CreateIdentity)
    {
        const Transform transform = Transform::CreateIdentity();
        EXPECT_TRUE(transform.m_position.IsZero());
        EXPECT_EQ(transform.m_rotation, AZ::Quaternion::CreateIdentity());
        EMFX_SCALECODE
        (
            EXPECT_EQ(transform.m_scale, AZ::Vector3::CreateOne());
        )
    }

    TEST(TransformFixture, CreateIdentityWithZeroScale)
    {
        const Transform transform = Transform::CreateIdentityWithZeroScale();
        EXPECT_TRUE(transform.m_position.IsZero());
        EXPECT_EQ(transform.m_rotation, AZ::Quaternion::CreateIdentity());
        EMFX_SCALECODE
        (
            EXPECT_EQ(transform.m_scale, AZ::Vector3::CreateZero());
        )
    }

    TEST(TransformFixture, CreateZero)
    {
        const Transform transform = Transform::CreateZero();
        EXPECT_TRUE(transform.m_position.IsZero());
        EXPECT_EQ(transform.m_rotation, AZ::Quaternion::CreateZero());
        EMFX_SCALECODE
        (
            EXPECT_EQ(transform.m_scale, AZ::Vector3::CreateZero());
        )
    }


    TEST(TransformFixture, ConstructFromVec3Quat)
    {
        const Transform transform(AZ::Vector3(6.0f, 7.0f, 8.0f), AZ::Quaternion::CreateRotationX(AZ::Constants::HalfPi));
        EXPECT_EQ(transform.m_position, AZ::Vector3(6.0f, 7.0f, 8.0f));
        EXPECT_THAT(transform.m_rotation, IsClose(AZ::Quaternion(sqrt2over2, 0.0f, 0.0f, sqrt2over2)));
        EMFX_SCALECODE
        (
            EXPECT_EQ(transform.m_scale, AZ::Vector3::CreateOne());
        )
    }

    using TransformConstructFromVec3QuatVec3Params = ::testing::tuple<AZ::Vector3, ::testing::tuple<float, float, float>, AZ::Vector3>;
    class TransformConstructFromVec3QuatVec3Fixture
        : public ::testing::TestWithParam<TransformConstructFromVec3QuatVec3Params>
    {
    public:
        const AZ::Vector3& ExpectedPosition() const
        {
            return ::testing::get<0>(GetParam());
        }
        AZ::Quaternion ExpectedRotation() const
        {
            return AZ::Quaternion::CreateFromEulerRadiansZYX(AZ::Vector3(
                ::testing::get<0>(::testing::get<1>(GetParam())),
                ::testing::get<1>(::testing::get<1>(GetParam())),
                ::testing::get<2>(::testing::get<1>(GetParam()))
            ));
        }
        const AZ::Vector3& ExpectedScale() const
        {
            return ::testing::get<2>(GetParam());
        }

        bool HasNonUniformScale() const
        {
            const AZ::Vector3 scale = ExpectedScale();
            return
                !AZ::IsClose(scale.GetX(), scale.GetY(), AZ::Constants::Tolerance) ||
                !AZ::IsClose(scale.GetX(), scale.GetZ(), AZ::Constants::Tolerance) ||
                !AZ::IsClose(scale.GetY(), scale.GetZ(), AZ::Constants::Tolerance);
        }

        // Returns a transformation matrix where the position is mirrored, the
        // rotation axis is mirrored, and the rotation angle is negated
        AZ::Matrix4x4 GetMirroredTransform(const AZ::Vector3& axis) const
        {
            const AZ::Matrix3x3 mirrorMatrix = AZ::Matrix3x3::CreateIdentity() - (2.0f * TensorProduct(axis, axis));
            const AZ::Vector3 mirrorPosition = mirrorMatrix * ExpectedPosition();

            AZ::Vector3 extractedAxis;
            float extractedAngle;
            ExpectedRotation().ConvertToAxisAngle(extractedAxis, extractedAngle);
            const AZ::Quaternion mirrorRotation = AZ::Quaternion::CreateFromAxisAngle(
                mirrorMatrix * AZ::Vector3(extractedAxis.GetX(), extractedAxis.GetY(), extractedAxis.GetZ()),
                -extractedAngle
            );

            return AZ::Matrix4x4::CreateFromQuaternionAndTranslation(mirrorRotation, mirrorPosition)
                * AZ::Matrix4x4::CreateScale(ExpectedScale());
        }
    };

    TEST_P(TransformConstructFromVec3QuatVec3Fixture, ConstructFromVec3QuatVec3)
    {
        const Transform transform(ExpectedPosition(), ExpectedRotation(), ExpectedScale());
        EXPECT_THAT(transform.m_position, IsClose(ExpectedPosition()));
        EXPECT_THAT(transform.m_rotation, IsClose(ExpectedRotation()));
        EMFX_SCALECODE
        (
            EXPECT_THAT(transform.m_scale, IsClose(ExpectedScale()));
        )
    }

    TEST_P(TransformConstructFromVec3QuatVec3Fixture, SetFromVec3QuatVec3)
    {
        Transform transform(AZ::Vector3(5.0f, 6.0f, 7.0f), AZ::Quaternion(0.1f, 0.2f, 0.3f, 0.4f), AZ::Vector3(8.0f, 9.0f, 10.0f));
        transform.Set(ExpectedPosition(), ExpectedRotation(), ExpectedScale());
        EXPECT_THAT(transform.m_position, IsClose(ExpectedPosition()));
        EXPECT_THAT(transform.m_rotation, IsClose(ExpectedRotation()));
        EMFX_SCALECODE
        (
            EXPECT_THAT(transform.m_scale, IsClose(ExpectedScale()));
        )
    }

    INSTANTIATE_TEST_CASE_P(Test, TransformConstructFromVec3QuatVec3Fixture,
        ::testing::Combine(
            ::testing::Values(
                AZ::Vector3::CreateZero(),
                AZ::Vector3(6.0f, 7.0f, 8.0f)
            ),
            ::testing::Combine(
                ::testing::Values(0.0f, AZ::Constants::QuarterPi, AZ::Constants::HalfPi),
                ::testing::Values(0.0f, AZ::Constants::QuarterPi, AZ::Constants::HalfPi),
                ::testing::Values(0.0f, AZ::Constants::QuarterPi, AZ::Constants::HalfPi)
            ),
            ::testing::Values(
                AZ::Vector3::CreateOne(),
                AZ::Vector3(2.0f, 2.0f, 2.0f),
                AZ::Vector3(2.0f, 3.0f, 4.0f)
            )
        )
    );

    TEST(TransformFixture, SetFromVec3Quat)
    {
        Transform transform(AZ::Vector3(5.0f, 6.0f, 7.0f), AZ::Quaternion(0.1f, 0.2f, 0.3f, 0.4f), AZ::Vector3(8.0f, 9.0f, 10.0f));
        transform.Set(AZ::Vector3(1.0f, 2.0f, 3.0f), AZ::Quaternion::CreateRotationX(AZ::Constants::QuarterPi));
        EXPECT_EQ(transform.m_position, AZ::Vector3(1.0f, 2.0f, 3.0f));
        EXPECT_THAT(transform.m_rotation, IsClose(AZ::Quaternion::CreateRotationX(AZ::Constants::QuarterPi)));
        EMFX_SCALECODE
        (
            EXPECT_EQ(transform.m_scale, AZ::Vector3::CreateOne());
        )
    }

    TEST(TransformFixture, Identity)
    {
        Transform transform(AZ::Vector3(1.0f, 2.0f, 3.0f), AZ::Quaternion(0.1f, 0.2f, 0.3f, 0.4f), AZ::Vector3(4.0f, 5.0f, 6.0f));
        transform.Identity();
        EXPECT_EQ(transform.m_position, AZ::Vector3::CreateZero());
        EXPECT_EQ(transform.m_rotation, AZ::Quaternion::CreateIdentity());
        EMFX_SCALECODE
        (
            EXPECT_EQ(transform.m_scale, AZ::Vector3::CreateOne());
        )
    }

    TEST(TransformFixture, Zero)
    {
        Transform transform(AZ::Vector3(1.0f, 2.0f, 3.0f), AZ::Quaternion(0.1f, 0.2f, 0.3f, 0.4f), AZ::Vector3(4.0f, 5.0f, 6.0f));
        transform.Zero();
        EXPECT_EQ(transform.m_position, AZ::Vector3::CreateZero());
        EXPECT_EQ(transform.m_rotation, AZ::Quaternion::CreateZero());
        EMFX_SCALECODE
        (
            EXPECT_EQ(transform.m_scale, AZ::Vector3::CreateZero());
        )
    }

    TEST(TransformFixture, IdentityWithZeroScale)
    {
        Transform transform(AZ::Vector3(1.0f, 2.0f, 3.0f), AZ::Quaternion(0.1f, 0.2f, 0.3f, 0.4f), AZ::Vector3(4.0f, 5.0f, 6.0f));
        transform.IdentityWithZeroScale();
        EXPECT_EQ(transform.m_position, AZ::Vector3::CreateZero());
        EXPECT_EQ(transform.m_rotation, AZ::Quaternion::CreateIdentity());
        EMFX_SCALECODE
        (
            EXPECT_EQ(transform.m_scale, AZ::Vector3::CreateZero());
        )
    }

    using TransformMultiplyParams = ::testing::tuple<Transform, Transform, Transform, Transform>;
    using TransformMultiplyFixture = ::testing::TestWithParam<TransformMultiplyParams>;

    TEST_P(TransformMultiplyFixture, Multiply)
    {
        const Transform& inputA = ::testing::get<0>(GetParam());
        const Transform& inputB = ::testing::get<1>(GetParam());
        const Transform& expected = ::testing::get<2>(GetParam());

        Transform multiply(inputA);
        multiply.Multiply(inputB);

        EXPECT_THAT(multiply, IsClose(expected));
    }

    TEST_P(TransformMultiplyFixture, Multiplied)
    {
        const Transform& inputA = ::testing::get<0>(GetParam());
        const Transform& inputB = ::testing::get<1>(GetParam());
        const Transform& expected = ::testing::get<2>(GetParam());
        EXPECT_THAT(
            inputA.Multiplied(inputB),
            IsClose(expected)
        );
        EXPECT_THAT(
            inputA.Multiplied(Transform::CreateIdentity()),
            IsClose(inputA)
        );
    }

    TEST_P(TransformMultiplyFixture, PreMultiply)
    {
        Transform inputA = ::testing::get<0>(GetParam());
        const Transform& inputB = ::testing::get<1>(GetParam());
        const Transform& expected = ::testing::get<3>(GetParam());
        EXPECT_THAT(
            inputA.PreMultiply(inputB),
            IsClose(expected)
        );
        EXPECT_THAT(
            inputA.PreMultiply(Transform::CreateIdentity()),
            IsClose(inputA)
        );
    }

    TEST_P(TransformMultiplyFixture, MultiplyWithOutputParam)
    {
        const Transform& inputA = ::testing::get<0>(GetParam());
        const Transform& inputB = ::testing::get<1>(GetParam());
        const Transform& expected = ::testing::get<2>(GetParam());

        Transform output = Transform::CreateIdentity();
        inputA.Multiply(inputB, &output);

        EXPECT_THAT(output, IsClose(expected));
    }

    TEST_P(TransformMultiplyFixture, PreMultiplied)
    {
        const Transform& inputA = ::testing::get<0>(GetParam());
        const Transform& inputB = ::testing::get<1>(GetParam());
        const Transform& expected = ::testing::get<3>(GetParam());
        EXPECT_THAT(
            inputA.PreMultiplied(inputB),
            IsClose(expected)
        );
        EXPECT_THAT(
            inputA.PreMultiplied(Transform::CreateIdentity()),
            IsClose(inputA)
        );
    }

    TEST_P(TransformMultiplyFixture, PreMultiplyWithOutputParam)
    {
        const Transform& inputA = ::testing::get<0>(GetParam());
        const Transform& inputB = ::testing::get<1>(GetParam());
        const Transform& expected = ::testing::get<3>(GetParam());

        Transform output;
        inputA.PreMultiply(inputB, &output);

        EXPECT_THAT(
            output,
            IsClose(expected)
        );
    }

    TEST_P(TransformMultiplyFixture, operatorMult)
    {
        const Transform& inputA = ::testing::get<0>(GetParam());
        const Transform& inputB = ::testing::get<1>(GetParam());
        const Transform& expected = ::testing::get<2>(GetParam());
        const Transform& expectedPreMult = ::testing::get<3>(GetParam());

        EXPECT_THAT(
            inputA * inputB,
            IsClose(expected)
        );
        EXPECT_THAT(
            inputB * inputA,
            IsClose(expectedPreMult)
        );

        EXPECT_THAT(
            inputA * Transform::CreateIdentity(),
            IsClose(inputA)
        );
        EXPECT_THAT(
            inputB * Transform::CreateIdentity(),
            IsClose(inputB)
        );
    }

    INSTANTIATE_TEST_CASE_P(Test, TransformMultiplyFixture,
        ::testing::Values(
            TransformMultiplyParams {
                /* input a */{Transform::CreateIdentity()},
                /* input b */{Transform::CreateIdentity()},
                /* a * b = */{Transform::CreateIdentity()},
                /* b * a = */{Transform::CreateIdentity()}
            },
            // symmetric cases (where a*b == b*a) -----------------------------
            TransformMultiplyParams {
                // just translation
                /* input a */{AZ::Vector3::CreateOne(), AZ::Quaternion::CreateIdentity(), AZ::Vector3::CreateOne()},
                /* input b */{AZ::Vector3::CreateOne(), AZ::Quaternion::CreateIdentity(), AZ::Vector3::CreateOne()},
                /* a * b = */{AZ::Vector3(2.0f, 2.0f, 2.0f), AZ::Quaternion::CreateIdentity(), AZ::Vector3::CreateOne()},
                /* b * a = */{AZ::Vector3(2.0f, 2.0f, 2.0f), AZ::Quaternion::CreateIdentity(), AZ::Vector3::CreateOne()}
            },
            TransformMultiplyParams {
                // just rotation
                /* input a */{AZ::Vector3::CreateZero(), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi), AZ::Vector3::CreateOne()},
                /* input b */{AZ::Vector3::CreateZero(), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi), AZ::Vector3::CreateOne()},
                /* a * b = */{AZ::Vector3::CreateZero(), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::HalfPi), AZ::Vector3::CreateOne()},
                /* b * a = */{AZ::Vector3::CreateZero(), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::HalfPi), AZ::Vector3::CreateOne()}
            },
            TransformMultiplyParams {
                // just scale
                /* input a */{AZ::Vector3::CreateZero(), AZ::Quaternion::CreateIdentity(), AZ::Vector3(2.0f, 2.0f, 2.0f)},
                /* input b */{AZ::Vector3::CreateZero(), AZ::Quaternion::CreateIdentity(), AZ::Vector3(2.0f, 2.0f, 2.0f)},
                /* a * b = */{AZ::Vector3::CreateZero(), AZ::Quaternion::CreateIdentity(), AZ::Vector3(4.0f, 4.0f, 4.0f)},
                /* b * a = */{AZ::Vector3::CreateZero(), AZ::Quaternion::CreateIdentity(), AZ::Vector3(4.0f, 4.0f, 4.0f)}
            },
            TransformMultiplyParams {
                // translation and rotation
                /* input a */{AZ::Vector3::CreateAxisY(), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi), AZ::Vector3::CreateOne()},
                /* input b */{AZ::Vector3::CreateAxisY(), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi), AZ::Vector3::CreateOne()},
                /* a * b = */{AZ::Vector3(0.0f, 1.0f + sqrt2over2, sqrt2over2), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::HalfPi), AZ::Vector3::CreateOne()},
                /* b * a = */{AZ::Vector3(0.0f, 1.0f + sqrt2over2, sqrt2over2), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::HalfPi), AZ::Vector3::CreateOne()}
            },
            TransformMultiplyParams {
                // rotation and scale
                /* input a */{AZ::Vector3::CreateZero(), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi), AZ::Vector3(2.0f, 2.0f, 2.0f)},
                /* input b */{AZ::Vector3::CreateZero(), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi), AZ::Vector3(2.0f, 2.0f, 2.0f)},
                /* a * b = */{AZ::Vector3::CreateZero(), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::HalfPi), AZ::Vector3(4.0f, 4.0f, 4.0f)},
                /* b * a = */{AZ::Vector3::CreateZero(), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::HalfPi), AZ::Vector3(4.0f, 4.0f, 4.0f)}
            },
            TransformMultiplyParams {
                // translation and scale
                /* input a */{AZ::Vector3::CreateOne(), AZ::Quaternion::CreateIdentity(), AZ::Vector3(2.0f, 2.0f, 2.0f)},
                /* input b */{AZ::Vector3::CreateOne(), AZ::Quaternion::CreateIdentity(), AZ::Vector3(2.0f, 2.0f, 2.0f)},
                /* a * b = */{AZ::Vector3(3.0f, 3.0f, 3.0f), AZ::Quaternion::CreateIdentity(), AZ::Vector3(4.0f, 4.0f, 4.0f)},
                /* b * a = */{AZ::Vector3(3.0f, 3.0f, 3.0f), AZ::Quaternion::CreateIdentity(), AZ::Vector3(4.0f, 4.0f, 4.0f)}
            },
            TransformMultiplyParams {
                // translation, rotation, and scale
                /* input a */{AZ::Vector3::CreateOne(), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi), AZ::Vector3(2.0f, 2.0f, 2.0f)},
                /* input b */{AZ::Vector3::CreateOne(), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi), AZ::Vector3(2.0f, 2.0f, 2.0f)},
                /* a * b = */{AZ::Vector3(3.0f, 1.0f, 1.0f + 2*sqrt2), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::HalfPi), AZ::Vector3(4.0f, 4.0f, 4.0f)},
                /* b * a = */{AZ::Vector3(3.0f, 1.0f, 1.0f + 2*sqrt2), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::HalfPi), AZ::Vector3(4.0f, 4.0f, 4.0f)}
            },
            // asymmetric cases (where a*b != b*a) -----------------------------
            TransformMultiplyParams {
                // translation and rotation
                /* input a */{AZ::Vector3::CreateOne(), AZ::Quaternion::CreateIdentity(), AZ::Vector3::CreateOne()},
                /* input b */{AZ::Vector3::CreateZero(), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi), AZ::Vector3::CreateOne()},
                // translate then rotate
                /* a * b = */{AZ::Vector3(1.0f, 0.0f, sqrt2), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi), AZ::Vector3::CreateOne()},
                // rotate then translate
                /* b * a = */{AZ::Vector3::CreateOne(), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi), AZ::Vector3::CreateOne()}
            },
            TransformMultiplyParams {
                // translation and scale
                /* input a */{AZ::Vector3::CreateOne(), AZ::Quaternion::CreateIdentity(), AZ::Vector3::CreateOne()},
                /* input b */{AZ::Vector3::CreateZero(), AZ::Quaternion::CreateIdentity(), AZ::Vector3(2.0f, 2.0f, 2.0f)},
                // translate then scale
                /* a * b = */{AZ::Vector3(2.0f, 2.0f, 2.0f), AZ::Quaternion::CreateIdentity(), AZ::Vector3(2.0f, 2.0f, 2.0f)},
                // scale then translate
                /* b * a = */{AZ::Vector3::CreateOne(), AZ::Quaternion::CreateIdentity(), AZ::Vector3(2.0f, 2.0f, 2.0f)}
            },
            TransformMultiplyParams {
                // rotation and scale
                // rotation * scale are only asymmetric when there is a translation involved as well
                /* input a */{AZ::Vector3::CreateOne(), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi), AZ::Vector3::CreateOne()},
                /* input b */{AZ::Vector3::CreateOne(), AZ::Quaternion::CreateIdentity(), AZ::Vector3(2.0f, 2.0f, 2.0f)},
                // rotate then scale
                /* a * b = */{AZ::Vector3(3.0f, 3.0f, 3.0f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi), AZ::Vector3(2.0f, 2.0f, 2.0f)},
                // scale then rotate
                /* b * a = */{AZ::Vector3(2.0f, 1.0f, 1.0f + sqrt2), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi), AZ::Vector3(2.0f, 2.0f, 2.0f)},
            }
        )
    );

    TEST(TransformFixture, TransformPoint)
    {
        EXPECT_THAT(
            Transform(AZ::Vector3(5.0f, 0.0f, 0.0f), AZ::Quaternion::CreateIdentity())
                .TransformPoint(AZ::Vector3::CreateZero()),
            IsClose(AZ::Vector3(5.0f, 0.0f, 0.0f))
        );

        EXPECT_THAT(
            Transform(AZ::Vector3(5.0f, 0.0f, 0.0f), AZ::Quaternion::CreateIdentity(), AZ::Vector3(2.5f, 1.0f, 1.0f))
                .TransformPoint(AZ::Vector3::CreateAxisX()),
            IsClose(EMFX_SCALE ? AZ::Vector3(7.5f, 0.0f, 0.0f) : AZ::Vector3(6.0f, 0.0f, 0.0f))
        );

        EXPECT_THAT(
            Transform(AZ::Vector3::CreateZero(), AZ::Quaternion::CreateRotationX(AZ::Constants::QuarterPi), AZ::Vector3::CreateOne())
                .TransformPoint(AZ::Vector3(0.0f, 1.0f, 0.0f)),
            IsClose(AZ::Vector3(0.0f, sqrt2over2, sqrt2over2))
        );

        EXPECT_THAT(
            Transform(AZ::Vector3::CreateZero(), AZ::Quaternion::CreateRotationX(AZ::Constants::QuarterPi), AZ::Vector3(1.0f, 2.0f, 3.0f))
                .TransformPoint(AZ::Vector3::CreateOne()),
            IsClose(AZ::Vector3(1.0f, -sqrt2over2, sqrt2over2 * 5.0f))
        );

        EXPECT_THAT(
            Transform(AZ::Vector3(5.0f, 6.0f, 7.0f), AZ::Quaternion::CreateRotationX(AZ::Constants::QuarterPi), AZ::Vector3(1.0f, 2.0f, 3.0f))
                .TransformPoint(AZ::Vector3::CreateOne()),
            IsClose(AZ::Vector3(6.0f, 6.0f - sqrt2over2, 7.0f + sqrt2over2 * 5.0f))
        );
    }

    TEST(TransformFixture, TransformVector)
    {
        EXPECT_THAT(
            Transform(AZ::Vector3(5.0f, 0.0f, 0.0f), AZ::Quaternion::CreateIdentity())
                .TransformVector(AZ::Vector3::CreateZero()),
            IsClose(AZ::Vector3::CreateZero())
        );

        EXPECT_THAT(
            Transform(AZ::Vector3(5.0f, 0.0f, 0.0f), AZ::Quaternion::CreateIdentity(), AZ::Vector3(2.5f, 1.0f, 1.0f))
                .TransformVector(AZ::Vector3::CreateAxisX()),
            IsClose(EMFX_SCALE ? AZ::Vector3(2.5f, 0.0f, 0.0f) : AZ::Vector3::CreateAxisX())
        );

        EXPECT_THAT(
            Transform(AZ::Vector3::CreateZero(), AZ::Quaternion::CreateRotationX(AZ::Constants::QuarterPi), AZ::Vector3::CreateOne())
                .TransformVector(AZ::Vector3::CreateAxisY()),
            IsClose(AZ::Vector3(0.0f, sqrt2over2, sqrt2over2))
        );

        EXPECT_THAT(
            Transform(AZ::Vector3::CreateZero(), AZ::Quaternion::CreateRotationX(AZ::Constants::QuarterPi), AZ::Vector3(1.0f, 2.0f, 3.0f))
                .TransformVector(AZ::Vector3::CreateOne()),
            IsClose(AZ::Vector3(1.0f, -sqrt2over2, sqrt2over2 * 5.0f))
        );
    }

    TEST(TransformFixture, RotateVector)
    {
        EXPECT_THAT(
            Transform(AZ::Vector3(5.0f, 0.0f, 0.0f), AZ::Quaternion::CreateIdentity())
                .RotateVector(AZ::Vector3::CreateZero()),
            IsClose(AZ::Vector3::CreateZero())
        );

        EXPECT_THAT(
            Transform(AZ::Vector3(5.0f, 0.0f, 0.0f), AZ::Quaternion::CreateIdentity(), AZ::Vector3(2.5f, 1.0f, 1.0f))
                .RotateVector(AZ::Vector3::CreateAxisX()),
            IsClose(AZ::Vector3::CreateAxisX())
        );

        EXPECT_THAT(
            Transform(AZ::Vector3::CreateZero(), AZ::Quaternion::CreateRotationX(AZ::Constants::QuarterPi), AZ::Vector3::CreateOne())
                .RotateVector(AZ::Vector3::CreateAxisY()),
            IsClose(AZ::Vector3(0.0f, sqrt2over2, sqrt2over2))
        );
    }

    TEST_P(TransformConstructFromVec3QuatVec3Fixture, Inverse)
    {
        // Inverse does not work properly when there is non-uniform scale
        if (HasNonUniformScale())
        {
            return;
        }

        const Transform transform(ExpectedPosition(), ExpectedRotation(), ExpectedScale());
        const Transform inverse = Transform(ExpectedPosition(), ExpectedRotation(), ExpectedScale()).Inverse();

        const AZ::Vector3 point(1.0f, 2.0f, 3.0f);

        EXPECT_THAT(
            inverse.TransformPoint(transform.TransformPoint(point)),
            IsClose(point)
        );
    }

    TEST_P(TransformConstructFromVec3QuatVec3Fixture, Inversed)
    {
        if (HasNonUniformScale())
        {
            return;
        }

        const Transform transform(ExpectedPosition(), ExpectedRotation(), ExpectedScale());
        const Transform inverse = transform.Inversed();

        const AZ::Vector3 point(1.0f, 2.0f, 3.0f);

        EXPECT_THAT(
            inverse.TransformPoint(transform.TransformPoint(point)),
            IsClose(point)
        );
    }

    TEST_P(TransformConstructFromVec3QuatVec3Fixture, CalcRelativeToWithOutputParam)
    {
        const Transform transform(ExpectedPosition(), ExpectedRotation(), ExpectedScale());

        const Transform someTransform(
            AZ::Vector3(20.0f, 30.0f, 40.0f),
            AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(0.2f, 0.4f, 0.7f).GetNormalized(), 0.25f),
            AZ::Vector3(2.0f, 3.0f, 4.0f)
        );

        Transform relative;
        transform.CalcRelativeTo(someTransform, &relative);

        EXPECT_THAT(
            relative * someTransform,
            IsClose(transform)
        );
    }

    TEST_P(TransformConstructFromVec3QuatVec3Fixture, CalcRelativeTo)
    {
        const Transform transform(ExpectedPosition(), ExpectedRotation(), ExpectedScale());

        const Transform someTransform(
            AZ::Vector3(20.0f, 30.0f, 40.0f),
            AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(0.2f, 0.4f, 0.7f).GetNormalized(), 0.25f),
            AZ::Vector3(2.0f, 3.0f, 4.0f)
        );

        const Transform relative = transform.CalcRelativeTo(someTransform);

        EXPECT_THAT(
            relative * someTransform,
            IsClose(transform)
        );
    }

    TEST_P(TransformConstructFromVec3QuatVec3Fixture, InverseWithOutputParam)
    {
        if (HasNonUniformScale())
        {
            return;
        }

        const Transform transform(ExpectedPosition(), ExpectedRotation(), ExpectedScale());
        Transform inverse;
        transform.Inverse(&inverse);

        const AZ::Vector3 point(1.0f, 2.0f, 3.0f);

        EXPECT_THAT(
            inverse.TransformPoint(transform.TransformPoint(point)),
            IsClose(point)
        );
    }

    TEST_P(TransformConstructFromVec3QuatVec3Fixture, Mirror)
    {
        const AZ::Vector3 axis = AZ::Vector3::CreateAxisX();

        const Transform mirrorTransform = Transform(ExpectedPosition(), ExpectedRotation(), ExpectedScale()).Mirror(axis);

        const AZ::Matrix4x4 mirrorMatrix = GetMirroredTransform(axis);

        const AZ::Vector3 point(3.0f, 4.0f, 5.0f);

        EXPECT_THAT(
            mirrorTransform.TransformPoint(point),
            IsClose(mirrorMatrix * point)
        );
    }

    TEST(TransformFixture, MirrorWithFlags)
    {
    }

    TEST_P(TransformConstructFromVec3QuatVec3Fixture, Mirrored)
    {
        const AZ::Vector3 axis = AZ::Vector3::CreateAxisX();
        const Transform mirrorTransform = Transform(ExpectedPosition(), ExpectedRotation(), ExpectedScale()).Mirrored(axis);
        const AZ::Matrix4x4 mirrorMatrix = GetMirroredTransform(axis);
        const AZ::Vector3 point(3.0f, 4.0f, 5.0f);

        EXPECT_THAT(
            mirrorTransform.TransformPoint(point),
            IsClose(mirrorMatrix * point)
        );
    }

    TEST_P(TransformConstructFromVec3QuatVec3Fixture, MirrorWithOutputParam)
    {
        const AZ::Vector3 axis = AZ::Vector3::CreateAxisX();
        Transform mirrorTransform;
        Transform(ExpectedPosition(), ExpectedRotation(), ExpectedScale()).Mirror(axis, &mirrorTransform);

        const AZ::Matrix4x4 mirrorMatrix = GetMirroredTransform(axis);
        const AZ::Vector3 point(3.0f, 4.0f, 5.0f);

        EXPECT_THAT(
            mirrorTransform.TransformPoint(point),
            IsClose(mirrorMatrix * point)
        );
    }

    struct ApplyDeltaParams
    {
        const Transform m_initial;
        const Transform m_a;
        const Transform m_b;
        const Transform m_expected;
        const float m_weight;
    };

    using TransformApplyDeltaFixture = ::testing::TestWithParam<ApplyDeltaParams>;

    TEST_P(TransformApplyDeltaFixture, ApplyDelta)
    {
        if (GetParam().m_weight != 1.0f)
        {
            return;
        }

        Transform transform = GetParam().m_initial;
        transform.ApplyDelta(GetParam().m_a, GetParam().m_b);
        EXPECT_THAT(
            transform,
            IsClose(GetParam().m_expected)
        );
    }

    TEST_P(TransformApplyDeltaFixture, ApplyDeltaMirrored)
    {
        if (GetParam().m_weight != 1.0f)
        {
            return;
        }

        const AZ::Vector3 mirrorAxis = AZ::Vector3::CreateAxisX();

        Transform transform = GetParam().m_initial;
        transform.ApplyDeltaMirrored(GetParam().m_a, GetParam().m_b, mirrorAxis);
        EXPECT_THAT(
            transform,
            IsClose(GetParam().m_expected.Mirrored(mirrorAxis))
        );
    }

    TEST_P(TransformApplyDeltaFixture, ApplyDeltaWithWeight)
    {
        Transform transform = GetParam().m_initial;
        transform.ApplyDeltaWithWeight(GetParam().m_a, GetParam().m_b, GetParam().m_weight);
        EXPECT_THAT(
            transform,
            IsClose(GetParam().m_expected)
        );
    }

    INSTANTIATE_TEST_CASE_P(Test, TransformApplyDeltaFixture,
        ::testing::ValuesIn(std::vector<ApplyDeltaParams>{
            {
                {Transform::CreateIdentity()},
                {AZ::Vector3(1.0f, 2.0f, 3.0f), AZ::Quaternion::CreateIdentity(), AZ::Vector3::CreateOne()},
                {AZ::Vector3(2.0f, 3.0f, 4.0f), AZ::Quaternion::CreateIdentity(), AZ::Vector3::CreateOne()},
                {AZ::Vector3(0.5f, 0.5f, 0.5f), AZ::Quaternion::CreateIdentity(), AZ::Vector3::CreateOne()},
                0.5f,
            },
            {
                {Transform::CreateIdentity()},
                {AZ::Vector3(1.0f, 2.0f, 3.0f), AZ::Quaternion::CreateIdentity(), AZ::Vector3::CreateOne()},
                {AZ::Vector3(2.0f, 3.0f, 4.0f), AZ::Quaternion::CreateIdentity(), AZ::Vector3::CreateOne()},
                {AZ::Vector3(1.0f, 1.0f, 1.0f), AZ::Quaternion::CreateIdentity(), AZ::Vector3::CreateOne()},
                1.0f,
            },
            {
                {Transform::CreateIdentity()},
                {AZ::Vector3::CreateZero(), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi / 2.0f), AZ::Vector3::CreateOne()},
                {AZ::Vector3::CreateZero(), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi), AZ::Vector3::CreateOne()},
                {AZ::Vector3::CreateZero(), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi / 4.0f), AZ::Vector3::CreateOne()},
                0.5f,
            },
            {
                {Transform::CreateIdentity()},
                {AZ::Vector3::CreateZero(), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi / 2.0f), AZ::Vector3::CreateOne()},
                {AZ::Vector3::CreateZero(), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi), AZ::Vector3::CreateOne()},
                {AZ::Vector3::CreateZero(), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi / 2.0f), AZ::Vector3::CreateOne()},
                1.0f,
            },
            {
                {Transform::CreateIdentity()},
                {AZ::Vector3::CreateZero(), AZ::Quaternion::CreateIdentity(), AZ::Vector3::CreateOne()},
                {AZ::Vector3::CreateZero(), AZ::Quaternion::CreateIdentity(), AZ::Vector3(2.0f, 2.0f, 2.0f)},
                {AZ::Vector3::CreateZero(), AZ::Quaternion::CreateIdentity(), AZ::Vector3(1.5f, 1.5f, 1.5f)},
                0.5f,
            },
            {
                {Transform::CreateIdentity()},
                {AZ::Vector3::CreateZero(), AZ::Quaternion::CreateIdentity(), AZ::Vector3::CreateOne()},
                {AZ::Vector3::CreateZero(), AZ::Quaternion::CreateIdentity(), AZ::Vector3(2.0f, 2.0f, 2.0f)},
                {AZ::Vector3::CreateZero(), AZ::Quaternion::CreateIdentity(), AZ::Vector3(2.0f, 2.0f, 2.0f)},
                1.0f,
            },
        })
    );

    TEST_P(TransformConstructFromVec3QuatVec3Fixture, CheckIfHasScale)
    {
        const Transform transform(ExpectedPosition(), ExpectedRotation(), ExpectedScale());

        EXPECT_EQ(transform.CheckIfHasScale(), !ExpectedScale().IsClose(AZ::Vector3::CreateOne()));
    }

    TEST(TransformFixture, Normalize)
    {
        const Transform transform = Transform(
            AZ::Vector3::CreateOne(),
            AZ::Quaternion(2.0f, 0.0f, 0.0f, 2.0f),
            AZ::Vector3::CreateOne()
        ).Normalize();
        EXPECT_FLOAT_EQ(transform.m_rotation.GetLength(), 1.0f);
    }

    TEST(TransformFixture, Normalized)
    {
        const Transform transform = Transform(
            AZ::Vector3::CreateOne(),
            AZ::Quaternion(2.0f, 0.0f, 0.0f, 2.0f),
            AZ::Vector3::CreateOne()
        ).Normalized();
        EXPECT_FLOAT_EQ(transform.m_rotation.GetLength(), 1.0f);
    }

    TEST(TransformFixture, BlendAdditive)
    {
        {
            const Transform result =
                Transform(AZ::Vector3(5.0f, 6.0f, 7.0f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi), AZ::Vector3::CreateOne()).BlendAdditive(
                    /*dest=*/Transform(AZ::Vector3(11.0f, 12.0f, 13.0f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::HalfPi), AZ::Vector3(2.0f, 2.0f, 2.0f)),
                    /*orgTransform=*/Transform(AZ::Vector3(8.0f, 10.0f, 12.0f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi), AZ::Vector3(2.0f, 3.0f, 2.0f)),
                    0.5f
                );

            EXPECT_THAT(
                result,
                IsClose(Transform(AZ::Vector3(6.5f, 7.0f, 7.5f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::Pi * 3.0f / 8.0f), AZ::Vector3(1.0f, 0.5f, 1.0f)))
            );
        }
    }

    class TwoTransformsFixture
        : public ::testing::Test
    {
    protected:
        const AZ::Vector3 m_translationA{5.0f, 6.0f, 7.0f};
        const AZ::Quaternion m_rotationA = AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi);
        const AZ::Vector3 m_scaleA = AZ::Vector3::CreateOne();

        const AZ::Vector3 m_translationB{11.0f, 12.0f, 13.0f};
        const AZ::Quaternion m_rotationB = AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::HalfPi);
        const AZ::Vector3 m_scaleB{3.0f, 4.0f, 5.0f};

    };

    TEST_F(TwoTransformsFixture, Blend)
    {
        const Transform transformA(m_translationA, m_rotationA, m_scaleA);
        const Transform transformB(m_translationB, m_rotationB, m_scaleB);

        EXPECT_THAT(
            Transform(m_translationA, m_rotationA, m_scaleA).Blend(transformB, 0.0f),
            IsClose(transformA)
        );
        EXPECT_THAT(
            Transform(m_translationA, m_rotationA, m_scaleA).Blend(transformB, 0.25f),
            IsClose(Transform(AZ::Vector3(6.5f, 7.5f, 8.5f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::Pi * 5.0f / 16.0f), AZ::Vector3(1.5f, 1.75f, 2.0f)))
        );
        EXPECT_THAT(
            Transform(m_translationA, m_rotationA, m_scaleA).Blend(transformB, 0.5f),
            IsClose(Transform(AZ::Vector3(8.0f, 9.0f, 10.0f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::Pi * 3.0f / 8.0f), AZ::Vector3(2.0f, 2.5f, 3.0f)))
        );
        EXPECT_THAT(
            Transform(m_translationA, m_rotationA, m_scaleA).Blend(transformB, 0.75f),
            IsClose(Transform(AZ::Vector3(9.5f, 10.5f, 11.5f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::Pi * 7.0f / 16.0f), AZ::Vector3(2.5f, 3.25f, 4.0f)))
        );
        EXPECT_THAT(
            Transform(m_translationA, m_rotationA, m_scaleA).Blend(transformB, 1.0f),
            IsClose(transformB)
        );
    }

    TEST_F(TwoTransformsFixture, ApplyAdditiveTransform)
    {
        EXPECT_THAT(
            Transform(m_translationA, m_rotationA, m_scaleA).ApplyAdditive(Transform(m_translationB, m_rotationB, m_scaleB)),
            IsClose(Transform(m_translationA + m_translationB, m_rotationA * m_rotationB, m_scaleA * m_scaleB))
        );
    }

    TEST_F(TwoTransformsFixture, ApplyAdditiveTransformFloat)
    {
        const float factor = 0.5f;
        EXPECT_THAT(
            Transform(m_translationA, m_rotationA, m_scaleA).ApplyAdditive(Transform(m_translationB, m_rotationB, m_scaleB), factor),
            IsClose(Transform(m_translationA + m_translationB * factor, m_rotationA.NLerp(m_rotationA * m_rotationB, factor), m_scaleA * AZ::Vector3::CreateOne().Lerp(m_scaleB, factor)))
        );
    }

    TEST_F(TwoTransformsFixture, AddTransform)
    {
        EXPECT_THAT(
            Transform(m_translationA, m_rotationA, m_scaleA).Add(Transform(m_translationB, m_rotationB, m_scaleB)),
            IsClose(Transform(m_translationA + m_translationB, m_rotationA + m_rotationB, m_scaleA + m_scaleB))
        );
    }

    TEST_F(TwoTransformsFixture, AddTransformFloat)
    {
        const float factor = 0.5f;
        EXPECT_THAT(
            Transform(m_translationA, m_rotationA, m_scaleA).Add(Transform(m_translationB, m_rotationB, m_scaleB), factor),
            IsClose(Transform(m_translationA + m_translationB * factor, m_rotationA + m_rotationB * factor, m_scaleA + m_scaleB * factor))
        );
    }

    TEST_F(TwoTransformsFixture, Subtract)
    {
        EXPECT_THAT(
            Transform(m_translationA, m_rotationA, m_scaleA).Subtract(Transform(m_translationB, m_rotationB, m_scaleB)),
            IsClose(Transform(m_translationA - m_translationB, m_rotationA - m_rotationB, m_scaleA - m_scaleB))
        );
    }

    class TransformProjectedToGroundPlaneFixture
        : public TransformConstructFromVec3QuatVec3Fixture
    {
    public:
        bool ShouldSkip() const
        {
            // These tests do not meet the expectation when there is both a
            // pitch and a roll value
            // This is because the combination of pitch + roll, even when yaw
            // is 0, introduces a rotation around z
            return ::testing::get<0>(::testing::get<1>(GetParam())) != 0
                && ::testing::get<1>(::testing::get<1>(GetParam())) != 0;
        }

        void Expect(const Transform& transform, float zValue) const
        {
            EXPECT_THAT(
                transform,
                IsClose(Transform(
                    AZ::Vector3(ExpectedPosition().GetX(), ExpectedPosition().GetY(), zValue),
                    AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisZ(), ::testing::get<2>(::testing::get<1>(GetParam()))),
                    ExpectedScale()
                ))
            );
        }
    };

    TEST_P(TransformProjectedToGroundPlaneFixture, ApplyMotionExtractionFlags)
    {
        if (ShouldSkip())
        {
            return;
        }
        Transform transform(ExpectedPosition(), ExpectedRotation(), ExpectedScale());
        transform.ApplyMotionExtractionFlags(EMotionExtractionFlags(0));

        Expect(transform, 0.0f);
    }

    TEST_P(TransformProjectedToGroundPlaneFixture, ApplyMotionExtractionFlagsCaptureZ)
    {
        if (ShouldSkip())
        {
            return;
        }
        Transform transform(ExpectedPosition(), ExpectedRotation(), ExpectedScale());
        transform.ApplyMotionExtractionFlags(MOTIONEXTRACT_CAPTURE_Z);

        Expect(transform, ExpectedPosition().GetZ());
    }

    TEST_P(TransformProjectedToGroundPlaneFixture, ProjectedToGroundPlane)
    {
        if (ShouldSkip())
        {
            return;
        }
        Expect(Transform(ExpectedPosition(), ExpectedRotation(), ExpectedScale()).ProjectedToGroundPlane(), 0.0f);
    }

    const auto possiblePitchAndYawValues = ::testing::Values(
        -AZ::Constants::HalfPi,
        -AZ::Constants::QuarterPi,
        0.0f,
        AZ::Constants::QuarterPi,
        AZ::Constants::HalfPi
    );
    INSTANTIATE_TEST_CASE_P(Test, TransformProjectedToGroundPlaneFixture,
        ::testing::Combine(
            ::testing::Values(
                AZ::Vector3::CreateZero(),
                AZ::Vector3(6.0f, 7.0f, 8.0f)
            ),
            ::testing::Combine(
                /* possible pitch values */ possiblePitchAndYawValues,
                /* possible roll values */ ::testing::Values(0.0f, AZ::Constants::QuarterPi),
                /* possible yaw values */ possiblePitchAndYawValues
            ),
            ::testing::Values(
                AZ::Vector3::CreateOne()
            )
        )
    );

} // namespace EMotionFX
