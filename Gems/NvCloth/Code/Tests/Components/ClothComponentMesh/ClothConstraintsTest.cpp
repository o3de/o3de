/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <Components/ClothComponentMesh/ClothConstraints.h>

#include <UnitTestHelper.h>

namespace UnitTest
{
    //! Fixture to hold the tests data.
    class NvClothConstraints
        : public ::testing::Test
    {
    public:
        const AZStd::vector<float> MeshMotionConstraintsData = {{
            1.0f, 0.5f, 0.0f
        }};
        const float MotionConstraintsMaxDistance = 3.0f;
        
        const AZStd::vector<AZ::Vector2> MeshBackstopOffsetAndRadiusData = {{
            AZ::Vector2(1.0f, 1.0f),
            AZ::Vector2(0.0f, 0.5f),
            AZ::Vector2(-1.0f, 0.1f)
        }};
        const float BackstopMaxRadius = 3.0f;
        const float BackstopMaxBackOffset = 2.0f;
        const float BackstopMaxFrontOffset = 5.0f;

        const AZStd::vector<int> MeshRemappedVertices = {{
            0, 1, 2
        }};

        const AZStd::vector<NvCloth::SimParticleFormat> SimulationParticles = {{
            NvCloth::SimParticleFormat(1.0f,0.0f,0.0f,1.0f),
            NvCloth::SimParticleFormat(0.0f,1.0f,0.0f,0.0f),
            NvCloth::SimParticleFormat(0.0f,0.0f,1.0f,1.0f),
        }};
        const AZStd::vector<NvCloth::SimIndexType> SimulationIndices = {{
            0, 1, 2
        }};
    };

    TEST_F(NvClothConstraints, ClothConstraints_DefaultConstruct_ReturnsEmptyData)
    {
        NvCloth::ClothConstraints clothConstraints;

        EXPECT_TRUE(clothConstraints.GetMotionConstraints().empty());
        EXPECT_TRUE(clothConstraints.GetSeparationConstraints().empty());

        clothConstraints.CalculateConstraints({}, {});

        EXPECT_TRUE(clothConstraints.GetMotionConstraints().empty());
        EXPECT_TRUE(clothConstraints.GetSeparationConstraints().empty());
        
        clothConstraints.CalculateConstraints(SimulationParticles, SimulationIndices);

        EXPECT_TRUE(clothConstraints.GetMotionConstraints().empty());
        EXPECT_TRUE(clothConstraints.GetSeparationConstraints().empty());
    }

    TEST_F(NvClothConstraints, ClothConstraints_CreateWithNoInfo_ReturnsEmptyData)
    {
        AZStd::unique_ptr<NvCloth::ClothConstraints> clothConstraints = NvCloth::ClothConstraints::Create(
            {},
            0.0f,
            {},
            0.0f,
            0.0f,
            0.0f,
            {},
            {},
            {});

        EXPECT_TRUE(clothConstraints->GetMotionConstraints().empty());
        EXPECT_TRUE(clothConstraints->GetSeparationConstraints().empty());
    }

    TEST_F(NvClothConstraints, ClothConstraints_CreateWithMotionConstraintsInfo_ReturnsValidMotionConstraints)
    {
        AZStd::unique_ptr<NvCloth::ClothConstraints> clothConstraints = NvCloth::ClothConstraints::Create(
            MeshMotionConstraintsData,
            MotionConstraintsMaxDistance,
            {},
            0.0f,
            0.0f,
            0.0f,
            SimulationParticles,
            SimulationIndices,
            MeshRemappedVertices);

        const AZStd::vector<AZ::Vector4>& motionConstraints = clothConstraints->GetMotionConstraints();

        EXPECT_TRUE(motionConstraints.size() == SimulationParticles.size());
        EXPECT_THAT(motionConstraints[0].GetAsVector3(), IsCloseTolerance(SimulationParticles[0].GetAsVector3(), Tolerance));
        EXPECT_THAT(motionConstraints[1].GetAsVector3(), IsCloseTolerance(SimulationParticles[1].GetAsVector3(), Tolerance));
        EXPECT_THAT(motionConstraints[2].GetAsVector3(), IsCloseTolerance(SimulationParticles[2].GetAsVector3(), Tolerance));
        EXPECT_NEAR(motionConstraints[0].GetW(), 3.0f, Tolerance);
        EXPECT_NEAR(motionConstraints[1].GetW(), 0.0f, Tolerance);
        EXPECT_NEAR(motionConstraints[2].GetW(), 0.0f, Tolerance);
    }
    
    TEST_F(NvClothConstraints, ClothConstraints_SetMotionConstraintMaxDistance_UpdatesMotionsConstraints)
    {
        AZStd::unique_ptr<NvCloth::ClothConstraints> clothConstraints = NvCloth::ClothConstraints::Create(
            MeshMotionConstraintsData,
            MotionConstraintsMaxDistance,
            {},
            0.0f,
            0.0f,
            0.0f,
            SimulationParticles,
            SimulationIndices,
            MeshRemappedVertices);

        const float newMotionConstraintsMaxDistance = 6.0f;
        clothConstraints->SetMotionConstraintMaxDistance(newMotionConstraintsMaxDistance);

        const AZStd::vector<AZ::Vector4>& motionConstraints = clothConstraints->GetMotionConstraints();

        EXPECT_TRUE(motionConstraints.size() == SimulationParticles.size());
        EXPECT_THAT(motionConstraints[0].GetAsVector3(), IsCloseTolerance(SimulationParticles[0].GetAsVector3(), Tolerance));
        EXPECT_THAT(motionConstraints[1].GetAsVector3(), IsCloseTolerance(SimulationParticles[1].GetAsVector3(), Tolerance));
        EXPECT_THAT(motionConstraints[2].GetAsVector3(), IsCloseTolerance(SimulationParticles[2].GetAsVector3(), Tolerance));
        EXPECT_NEAR(motionConstraints[0].GetW(), 6.0f, Tolerance);
        EXPECT_NEAR(motionConstraints[1].GetW(), 0.0f, Tolerance);
        EXPECT_NEAR(motionConstraints[2].GetW(), 0.0f, Tolerance);
    }

    TEST_F(NvClothConstraints, ClothConstraints_CreateWithBackstopInfo_ReturnsValidSeparationConstraints)
    {
        AZStd::unique_ptr<NvCloth::ClothConstraints> clothConstraints = NvCloth::ClothConstraints::Create(
            {},
            0.0f,
            MeshBackstopOffsetAndRadiusData,
            BackstopMaxRadius,
            BackstopMaxBackOffset,
            BackstopMaxFrontOffset,
            SimulationParticles,
            SimulationIndices,
            MeshRemappedVertices);

        const AZStd::vector<AZ::Vector4>& separationConstraints = clothConstraints->GetSeparationConstraints();

        EXPECT_TRUE(separationConstraints.size() == SimulationParticles.size());
        EXPECT_NEAR(separationConstraints[0].GetW(), 3.0f, Tolerance);
        EXPECT_NEAR(separationConstraints[1].GetW(), 1.5f, Tolerance);
        EXPECT_NEAR(separationConstraints[2].GetW(), 0.3f, Tolerance);
        EXPECT_THAT(separationConstraints[0].GetAsVector3(), IsCloseTolerance(AZ::Vector3(-1.88675f, -2.88675f, -2.88675f), Tolerance));
        EXPECT_THAT(separationConstraints[1].GetAsVector3(), IsCloseTolerance(AZ::Vector3(-0.866025f, 0.133975f, -0.866025f), Tolerance));
        EXPECT_THAT(separationConstraints[2].GetAsVector3(), IsCloseTolerance(AZ::Vector3(3.05996f, 3.05996f, 4.05996f), Tolerance));
    }

    TEST_F(NvClothConstraints, ClothConstraints_SetBackstopMaxRadius_UpdatesSeparationConstraints)
    {
        AZStd::unique_ptr<NvCloth::ClothConstraints> clothConstraints = NvCloth::ClothConstraints::Create(
            {},
            0.0f,
            MeshBackstopOffsetAndRadiusData,
            BackstopMaxRadius,
            BackstopMaxBackOffset,
            BackstopMaxFrontOffset,
            SimulationParticles,
            SimulationIndices,
            MeshRemappedVertices);

        const float newBackstopMaxRadius = 6.0f;
        clothConstraints->SetBackstopMaxRadius(newBackstopMaxRadius);

        const AZStd::vector<AZ::Vector4>& separationConstraints = clothConstraints->GetSeparationConstraints();

        EXPECT_TRUE(separationConstraints.size() == SimulationParticles.size());
        EXPECT_NEAR(separationConstraints[0].GetW(), 6.0f, Tolerance);
        EXPECT_NEAR(separationConstraints[1].GetW(), 3.0f, Tolerance);
        EXPECT_NEAR(separationConstraints[2].GetW(), 0.6f, Tolerance);
        EXPECT_THAT(separationConstraints[0].GetAsVector3(), IsCloseTolerance(AZ::Vector3(-3.6188f, -4.6188f, -4.6188f), Tolerance));
        EXPECT_THAT(separationConstraints[1].GetAsVector3(), IsCloseTolerance(AZ::Vector3(-1.73205f, -0.732051f, -1.73205f), Tolerance));
        EXPECT_THAT(separationConstraints[2].GetAsVector3(), IsCloseTolerance(AZ::Vector3(3.23316f, 3.23316f, 4.23316f), Tolerance));
    }
    
    TEST_F(NvClothConstraints, ClothConstraints_SetBackstopMaxOffsets_UpdatesSeparationConstraints)
    {
        AZStd::unique_ptr<NvCloth::ClothConstraints> clothConstraints = NvCloth::ClothConstraints::Create(
            {},
            0.0f,
            MeshBackstopOffsetAndRadiusData,
            BackstopMaxRadius,
            BackstopMaxBackOffset,
            BackstopMaxFrontOffset,
            SimulationParticles,
            SimulationIndices,
            MeshRemappedVertices);

        const float newBackstopMaxBackOffset = -4.0f;
        const float newBackstopMaxFrontOffset = 3.0f;
        clothConstraints->SetBackstopMaxOffsets(newBackstopMaxBackOffset, newBackstopMaxFrontOffset);

        const AZStd::vector<AZ::Vector4>& separationConstraints = clothConstraints->GetSeparationConstraints();

        EXPECT_TRUE(separationConstraints.size() == SimulationParticles.size());
        EXPECT_NEAR(separationConstraints[0].GetW(), 3.0f, Tolerance);
        EXPECT_NEAR(separationConstraints[1].GetW(), 1.5f, Tolerance);
        EXPECT_NEAR(separationConstraints[2].GetW(), 0.3f, Tolerance);
        EXPECT_THAT(separationConstraints[0].GetAsVector3(), IsCloseTolerance(AZ::Vector3(5.04145f, 4.04145f, 4.04145f), Tolerance));
        EXPECT_THAT(separationConstraints[1].GetAsVector3(), IsCloseTolerance(AZ::Vector3(-0.866025f, 0.133975f, -0.866025f), Tolerance));
        EXPECT_THAT(separationConstraints[2].GetAsVector3(), IsCloseTolerance(AZ::Vector3(1.90526f, 1.90526f, 2.90526f), Tolerance));
    }

    TEST_F(NvClothConstraints, ClothConstraints_CalculateConstraintsWithEmptyData_ConstraintsRemainUnchanged)
    {
        AZStd::unique_ptr<NvCloth::ClothConstraints> clothConstraints = NvCloth::ClothConstraints::Create(
            MeshMotionConstraintsData,
            MotionConstraintsMaxDistance,
            MeshBackstopOffsetAndRadiusData,
            BackstopMaxRadius,
            BackstopMaxBackOffset,
            BackstopMaxFrontOffset,
            SimulationParticles,
            SimulationIndices,
            MeshRemappedVertices);

        const AZStd::vector<AZ::Vector4> motionConstraints = clothConstraints->GetMotionConstraints();
        const AZStd::vector<AZ::Vector4> separationConstraints = clothConstraints->GetSeparationConstraints();

        clothConstraints->CalculateConstraints({}, {});

        EXPECT_THAT(motionConstraints, ::testing::Pointwise(ContainerIsCloseTolerance(Tolerance), clothConstraints->GetMotionConstraints()));
        EXPECT_THAT(separationConstraints, ::testing::Pointwise(ContainerIsCloseTolerance(Tolerance), clothConstraints->GetSeparationConstraints()));
    }
    
    TEST_F(NvClothConstraints, ClothConstraints_CalculateConstraintsWithSameData_ConstraintsRemainUnchanged)
    {
        AZStd::unique_ptr<NvCloth::ClothConstraints> clothConstraints = NvCloth::ClothConstraints::Create(
            MeshMotionConstraintsData,
            MotionConstraintsMaxDistance,
            MeshBackstopOffsetAndRadiusData,
            BackstopMaxRadius,
            BackstopMaxBackOffset,
            BackstopMaxFrontOffset,
            SimulationParticles,
            SimulationIndices,
            MeshRemappedVertices);

        const AZStd::vector<AZ::Vector4> motionConstraints = clothConstraints->GetMotionConstraints();
        const AZStd::vector<AZ::Vector4> separationConstraints = clothConstraints->GetSeparationConstraints();

        clothConstraints->CalculateConstraints(SimulationParticles, SimulationIndices);

        EXPECT_THAT(motionConstraints, ::testing::Pointwise(ContainerIsCloseTolerance(Tolerance), clothConstraints->GetMotionConstraints()));
        EXPECT_THAT(separationConstraints, ::testing::Pointwise(ContainerIsCloseTolerance(Tolerance), clothConstraints->GetSeparationConstraints()));
    }

    TEST_F(NvClothConstraints, ClothConstraints_CalculateConstraintsWithNewParticles_ContraintsAreModified)
    {
        AZStd::unique_ptr<NvCloth::ClothConstraints> clothConstraints = NvCloth::ClothConstraints::Create(
            MeshMotionConstraintsData,
            MotionConstraintsMaxDistance,
            MeshBackstopOffsetAndRadiusData,
            BackstopMaxRadius,
            BackstopMaxBackOffset,
            BackstopMaxFrontOffset,
            SimulationParticles,
            SimulationIndices,
            MeshRemappedVertices);
        
        const AZStd::vector<NvCloth::SimParticleFormat> newParticles = {{
            NvCloth::SimParticleFormat(0.0f,0.0f,1.0f,1.0f),
            NvCloth::SimParticleFormat(0.0f,1.0f,0.0f,1.0f),
            NvCloth::SimParticleFormat(1.0f,0.0f,1.0f,0.0f),
        }};

        clothConstraints->CalculateConstraints(newParticles, SimulationIndices);

        const AZStd::vector<AZ::Vector4>& motionConstraints = clothConstraints->GetMotionConstraints();
        const AZStd::vector<AZ::Vector4>& separationConstraints = clothConstraints->GetSeparationConstraints();

        EXPECT_TRUE(motionConstraints.size() == newParticles.size());
        EXPECT_THAT(motionConstraints[0].GetAsVector3(), IsCloseTolerance(newParticles[0].GetAsVector3(), Tolerance));
        EXPECT_THAT(motionConstraints[1].GetAsVector3(), IsCloseTolerance(newParticles[1].GetAsVector3(), Tolerance));
        EXPECT_THAT(motionConstraints[2].GetAsVector3(), IsCloseTolerance(newParticles[2].GetAsVector3(), Tolerance));
        EXPECT_NEAR(motionConstraints[0].GetW(), 3.0f, Tolerance);
        EXPECT_NEAR(motionConstraints[1].GetW(), 1.5f, Tolerance);
        EXPECT_NEAR(motionConstraints[2].GetW(), 0.0f, Tolerance);

        EXPECT_TRUE(separationConstraints.size() == newParticles.size());
        EXPECT_NEAR(separationConstraints[0].GetW(), 3.0f, Tolerance);
        EXPECT_NEAR(separationConstraints[1].GetW(), 1.5f, Tolerance);
        EXPECT_NEAR(separationConstraints[2].GetW(), 0.3f, Tolerance);
        EXPECT_THAT(separationConstraints[0].GetAsVector3(), IsCloseTolerance(AZ::Vector3(0.0f, 3.53553f, 4.53553f), Tolerance));
        EXPECT_THAT(separationConstraints[1].GetAsVector3(), IsCloseTolerance(AZ::Vector3(0.0f, 2.06066f, 1.06066f), Tolerance));
        EXPECT_THAT(separationConstraints[2].GetAsVector3(), IsCloseTolerance(AZ::Vector3(1.0f, -3.74767f, -2.74767f), Tolerance));
    }
} // namespace UnitTest
