/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Util/WhiteBoxTextureUtil.h"
#include "WhiteBoxTestFixtures.h"
#include "Rendering/Atom/WhiteBoxMeshAtomData.h"

#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/vector.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <WhiteBox/WhiteBoxToolApi.h>
#include <cmath>
#include <iostream>
#include <random>
#include <vector>

namespace UnitTest
{
    std::random_device rd;
    std::mt19937 gen(rd());

    // rngs for the first and subsequent significant figures of the random numbers
    std::uniform_real_distribution<> rndFirstSigFig(1.0, 10.0);
    std::uniform_real_distribution<> rndOtherSigFigs(0.0, 1.0);

    // generate noise after the specified decimal place with the first significant
    // figure always being one decimal place after afterDecimalPlace
    float GenerateNoiseWithSignificantFigures(AZ::u32 afterDecimalPlace)
    {
        // number of significant figures of randomness to generate
        const double numSigFigs = 8;

        // scaling factor to push the noise back into the desired range
        const double sigFactor = std::pow(10.0, numSigFigs + afterDecimalPlace);

        // random value for first guaranteed significant digit
        const auto firstSigFig = azlossy_cast<AZ::u64>(rndFirstSigFig(gen)) * std::pow(10.0, numSigFigs - 1);

        // random value for the other significant digits
        const auto otherSigFigs = azlossy_cast<AZ::u64>(rndOtherSigFigs(gen) * std::pow(10.0, numSigFigs - 1));

        // unscaled random value with rndSigFigs significant figures
        const auto fixedLengthRandom = firstSigFig + otherSigFigs;

        // scaled random value to push the noise into the desired significant digit range
        const float noise = aznumeric_cast<float>(fixedLengthRandom / sigFactor);

        return noise;
    }

    // generates a vector of Vector3s with noise for in specified range decimal places
    std::vector<Vector3> GenerateNoiseForSignificantFigureRange(AZ::u32 startDecimalPlace, AZ::u32 endDecimalPlace)
    {
        std::vector<Vector3> noise(endDecimalPlace - startDecimalPlace + 1);

        for (AZ::u32 decimal = startDecimalPlace, i = 0; decimal <= endDecimalPlace; decimal++, i++)
        {
            float x = GenerateNoiseWithSignificantFigures(decimal);
            float y = GenerateNoiseWithSignificantFigures(decimal);
            float z = GenerateNoiseWithSignificantFigures(decimal);
            noise[i] = Vector3{x, y, z};
        }

        return noise;
    }

    // vector of noise vectors with between 3 and 6 significat figures
    const std::vector<Vector3> Noise = GenerateNoiseForSignificantFigureRange(3, 6);

    // noise source permutations to be applied to each test
    const std::vector<NoiseSource> Source = {
        NoiseSource::None,        NoiseSource::XComponent,  NoiseSource::YComponent,  NoiseSource::ZComponent,
        NoiseSource::XYComponent, NoiseSource::XZComponent, NoiseSource::YZComponent, NoiseSource::XYZComponent};

    enum CubeVertex : AZ::u32
    {
        FrontTopLeft,
        FrontTopRight,
        BackTopLeft,
        BackTopRight,
        FrontBottomLeft,
        FrontBottomRight,
        BackBottomLeft,
        BackBottomRight
    };

    const std::vector<AZ::Vector3> UnitCube = {
        AZ::Vector3(-0.5f, 0.5f, 0.5f), // FrontTopLeft
        AZ::Vector3(0.5f, 0.5f, 0.5f), // FrontTopRight
        AZ::Vector3(-0.5f, -0.5f, 0.5f), // BackTopLeft
        AZ::Vector3(0.5f, -0.5f, 0.5f), // BackTopRight
        AZ::Vector3(-0.5f, 0.5f, -0.5f), // FrontBottomLeft
        AZ::Vector3(0.5f, 0.5f, -0.5f), // FrontBottomRight
        AZ::Vector3(-0.5f, -0.5f, -0.5f), // BackBottomLeft
        AZ::Vector3(0.5f, -0.5f, -0.5f) // BackBottomRight
    };

    // returns a vector with noise applied to it as determined by the source
    AZ::Vector3 GenerateNoiseyVector(const AZ::Vector3& in, const Vector3& noise, NoiseSource source)
    {
        switch (source)
        {
        case NoiseSource::None:
            return in;
        case NoiseSource::XComponent:
            return AZ::Vector3(in.GetX() + noise.x, in.GetY(), in.GetZ());
        case NoiseSource::YComponent:
            return AZ::Vector3(in.GetX(), in.GetY() + noise.y, in.GetZ());
        case NoiseSource::ZComponent:
            return AZ::Vector3(in.GetX(), in.GetY(), in.GetZ() + noise.z);
        case NoiseSource::XYComponent:
            return AZ::Vector3(in.GetX() + noise.x, in.GetY() + noise.y, in.GetZ());
        case NoiseSource::XZComponent:
            return AZ::Vector3(in.GetX() + noise.x, in.GetY(), in.GetZ() + noise.z);
        case NoiseSource::YZComponent:
            return AZ::Vector3(in.GetX(), in.GetY() + noise.y, in.GetZ() + noise.z);
        case NoiseSource::XYZComponent:
            return AZ::Vector3(in.GetX() + noise.x, in.GetY() + noise.y, in.GetZ() + noise.z);

        default:
            return in;
        }
    }

    // returns a quaternion with the specified rotation
    AZ::Quaternion GetQuaternionFromRotation(Rotation rotation)
    {
        static const AZ::Quaternion qXAxis = AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), 45);
        static const AZ::Quaternion qZAxis = AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisZ(), 45);
        static const AZ::Quaternion qXZAxis = qXAxis + qZAxis;

        switch (rotation)
        {
        case Rotation::Identity:
            return AZ::Quaternion::CreateIdentity();
        case Rotation::XAxis:
            return qXAxis;
        case Rotation::ZAxis:
            return qZAxis;
        case Rotation::XZAxis:
            return qXZAxis;
        default:
            return AZ::Quaternion::CreateIdentity();
        }
    }

    TEST_P(WhiteBoxUVTestFixture, FrontFaceCorners)
    {
        using WhiteBox::CreatePlanarUVFromVertex;
        const auto& [noise, source, rotation] = GetParam();

        const AZ::Quaternion qRotation = GetQuaternionFromRotation(rotation);
        const AZ::Vector3 normal =
            GenerateNoiseyVector(qRotation.TransformVector(AZ::Vector3(0.0f, -1.0f, 0.0f)), noise, source);

        const AZ::Vector2 uv00 = CreatePlanarUVFromVertex(normal, UnitCube[FrontTopLeft]);
        const AZ::Vector2 uv10 = CreatePlanarUVFromVertex(normal, UnitCube[FrontTopRight]);
        const AZ::Vector2 uv01 = CreatePlanarUVFromVertex(normal, UnitCube[FrontBottomLeft]);
        const AZ::Vector2 uv11 = CreatePlanarUVFromVertex(normal, UnitCube[FrontBottomRight]);

        EXPECT_THAT(uv00, IsClose(AZ::Vector2(1.0f, 0.0f)));
        EXPECT_THAT(uv10, IsClose(AZ::Vector2(0.0f, 0.0f)));
        EXPECT_THAT(uv01, IsClose(AZ::Vector2(1.0f, 1.0f)));
        EXPECT_THAT(uv11, IsClose(AZ::Vector2(0.0f, 1.0f)));
    }

    TEST_P(WhiteBoxUVTestFixture, BackFaceCorners)
    {
        using WhiteBox::CreatePlanarUVFromVertex;
        const auto& [noise, source, rotation] = GetParam();

        const AZ::Quaternion qRotation = GetQuaternionFromRotation(rotation);
        const AZ::Vector3 normal =
            GenerateNoiseyVector(qRotation.TransformVector(AZ::Vector3(0.0f, 1.0f, 0.0f)), noise, source);

        const AZ::Vector2 uv00 = CreatePlanarUVFromVertex(normal, UnitCube[BackTopLeft]);
        const AZ::Vector2 uv10 = CreatePlanarUVFromVertex(normal, UnitCube[BackTopRight]);
        const AZ::Vector2 uv01 = CreatePlanarUVFromVertex(normal, UnitCube[BackBottomLeft]);
        const AZ::Vector2 uv11 = CreatePlanarUVFromVertex(normal, UnitCube[BackBottomRight]);

        EXPECT_THAT(uv00, IsClose(AZ::Vector2(1.0f, 0.0f)));
        EXPECT_THAT(uv10, IsClose(AZ::Vector2(0.0f, 0.0f)));
        EXPECT_THAT(uv01, IsClose(AZ::Vector2(1.0f, 1.0f)));
        EXPECT_THAT(uv11, IsClose(AZ::Vector2(0.0f, 1.0f)));
    }

    TEST_P(WhiteBoxUVTestFixture, LeftFaceCorners)
    {
        using WhiteBox::CreatePlanarUVFromVertex;
        const auto& [noise, source, rotation] = GetParam();

        const AZ::Quaternion qRotation = GetQuaternionFromRotation(rotation);
        const AZ::Vector3 normal =
            GenerateNoiseyVector(qRotation.TransformVector(AZ::Vector3(-1.0f, 0.0f, 0.0f)), noise, source);

        const AZ::Vector2 uv00 = CreatePlanarUVFromVertex(normal, UnitCube[FrontTopLeft]);
        const AZ::Vector2 uv10 = CreatePlanarUVFromVertex(normal, UnitCube[BackTopLeft]);
        const AZ::Vector2 uv01 = CreatePlanarUVFromVertex(normal, UnitCube[FrontBottomLeft]);
        const AZ::Vector2 uv11 = CreatePlanarUVFromVertex(normal, UnitCube[BackBottomLeft]);

        EXPECT_THAT(uv00, IsClose(AZ::Vector2(0.0f, 0.0f)));
        EXPECT_THAT(uv10, IsClose(AZ::Vector2(0.0f, 1.0f)));
        EXPECT_THAT(uv01, IsClose(AZ::Vector2(1.0f, 0.0f)));
        EXPECT_THAT(uv11, IsClose(AZ::Vector2(1.0f, 1.0f)));
    }

    TEST_P(WhiteBoxUVTestFixture, RightFaceCorners)
    {
        using WhiteBox::CreatePlanarUVFromVertex;
        const auto& [noise, source, rotation] = GetParam();

        const AZ::Quaternion qRotation = GetQuaternionFromRotation(rotation);
        const AZ::Vector3 normal =
            GenerateNoiseyVector(qRotation.TransformVector(AZ::Vector3(1.0f, 0.0f, 0.0f)), noise, source);

        const AZ::Vector2 uv00 = CreatePlanarUVFromVertex(normal, UnitCube[FrontTopRight]);
        const AZ::Vector2 uv10 = CreatePlanarUVFromVertex(normal, UnitCube[BackTopRight]);
        const AZ::Vector2 uv01 = CreatePlanarUVFromVertex(normal, UnitCube[FrontBottomRight]);
        const AZ::Vector2 uv11 = CreatePlanarUVFromVertex(normal, UnitCube[BackBottomRight]);

        EXPECT_THAT(uv00, IsClose(AZ::Vector2(0.0f, 0.0f)));
        EXPECT_THAT(uv10, IsClose(AZ::Vector2(0.0f, 1.0f)));
        EXPECT_THAT(uv01, IsClose(AZ::Vector2(1.0f, 0.0f)));
        EXPECT_THAT(uv11, IsClose(AZ::Vector2(1.0f, 1.0f)));
    }

    TEST_P(WhiteBoxUVTestFixture, TopFaceCorners)
    {
        using WhiteBox::CreatePlanarUVFromVertex;
        const auto& [noise, source, rotation] = GetParam();

        const AZ::Quaternion qRotation = GetQuaternionFromRotation(rotation);
        const AZ::Vector3 normal =
            GenerateNoiseyVector(qRotation.TransformVector(AZ::Vector3(0.0f, 0.0f, 1.0f)), noise, source);

        const AZ::Vector2 uv00 = CreatePlanarUVFromVertex(normal, UnitCube[FrontTopLeft]);
        const AZ::Vector2 uv10 = CreatePlanarUVFromVertex(normal, UnitCube[FrontTopRight]);
        const AZ::Vector2 uv01 = CreatePlanarUVFromVertex(normal, UnitCube[BackTopLeft]);
        const AZ::Vector2 uv11 = CreatePlanarUVFromVertex(normal, UnitCube[BackTopRight]);

        EXPECT_THAT(uv00, IsClose(AZ::Vector2(1.0f, 0.0f)));
        EXPECT_THAT(uv10, IsClose(AZ::Vector2(0.0f, 0.0f)));
        EXPECT_THAT(uv01, IsClose(AZ::Vector2(1.0f, 1.0f)));
        EXPECT_THAT(uv11, IsClose(AZ::Vector2(0.0f, 1.0f)));
    }

    TEST_P(WhiteBoxUVTestFixture, BottomFaceCorners)
    {
        using WhiteBox::CreatePlanarUVFromVertex;
        const auto& [noise, source, rotation] = GetParam();

        const AZ::Quaternion qRotation = GetQuaternionFromRotation(rotation);
        const AZ::Vector3 normal =
            GenerateNoiseyVector(qRotation.TransformVector(AZ::Vector3(0.0f, 0.0f, -1.0f)), noise, source);

        const AZ::Vector2 uv00 = CreatePlanarUVFromVertex(normal, UnitCube[FrontBottomLeft]);
        const AZ::Vector2 uv10 = CreatePlanarUVFromVertex(normal, UnitCube[FrontBottomRight]);
        const AZ::Vector2 uv01 = CreatePlanarUVFromVertex(normal, UnitCube[BackBottomLeft]);
        const AZ::Vector2 uv11 = CreatePlanarUVFromVertex(normal, UnitCube[BackBottomRight]);

        EXPECT_THAT(uv00, IsClose(AZ::Vector2(1.0f, 0.0f)));
        EXPECT_THAT(uv10, IsClose(AZ::Vector2(0.0f, 0.0f)));
        EXPECT_THAT(uv01, IsClose(AZ::Vector2(1.0f, 1.0f)));
        EXPECT_THAT(uv11, IsClose(AZ::Vector2(0.0f, 1.0f)));
    }

    // test with permutations of all noise values and sources with rotations around the x and z axis
    INSTANTIATE_TEST_CASE_P(
        , WhiteBoxUVTestFixture,
        ::testing::Combine(
            ::testing::ValuesIn(Noise), ::testing::ValuesIn(Source),
            ::testing::Values(Rotation::Identity, Rotation::XZAxis)));

    TEST(WhiteBoxRenderTest, WhiteBoxMeshAtomDataAabbIsInitializedToNull)
    {
        WhiteBox::WhiteBoxMeshAtomData atomData(WhiteBox::WhiteBoxFaces{});
        EXPECT_EQ(atomData.GetAabb(), AZ::Aabb::CreateNull());
    }

} // namespace UnitTest
