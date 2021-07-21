/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AZTestShared/Math/MathTestHelpers.h>

#include <AzCore/Interface/Interface.h>

#include <UnitTestHelper.h>
#include <TriangleInputHelper.h>

#include <System/Factory.h>
#include <System/Solver.h>
#include <System/Fabric.h>
#include <System/FabricCooker.h>
#include <System/Cloth.h>

namespace UnitTest
{
    //! Sets up a factory for each test case.
    class NvClothSystemFactory
        : public ::testing::Test
    {
    protected:
        // ::testing::Test overrides ...
        void SetUp() override;
        void TearDown() override;

        NvCloth::Factory m_factory;
    };

    void NvClothSystemFactory::SetUp()
    {
        m_factory.Init();
    }

    void NvClothSystemFactory::TearDown()
    {
        m_factory.Destroy();
    }

    TEST_F(NvClothSystemFactory, Factory_CreateSolverEmptyName_ReturnsNull)
    {
        AZStd::unique_ptr<NvCloth::Solver> solver = m_factory.CreateSolver("");

        EXPECT_TRUE(solver.get() == nullptr);
    }

    TEST_F(NvClothSystemFactory, Factory_CreateSolver_ReturnsValidSolver)
    {
        const AZStd::string solverName = "NewSolver";

        AZStd::unique_ptr<NvCloth::Solver> solver = m_factory.CreateSolver(solverName);

        EXPECT_TRUE(solver.get() != nullptr);
        EXPECT_EQ(solver->GetName(), solverName);
    }

    TEST_F(NvClothSystemFactory, Factory_CreateFabricInvalidId_ReturnsNull)
    {
        NvCloth::FabricCookedData emptyFabricCookedData;
        EXPECT_FALSE(emptyFabricCookedData.m_id.IsValid());

        AZStd::unique_ptr<NvCloth::Fabric> fabric = m_factory.CreateFabric(emptyFabricCookedData);

        EXPECT_TRUE(fabric.get() == nullptr);
    }

    TEST_F(NvClothSystemFactory, Factory_CreateFabric_ReturnsValidFabric)
    {
        const NvCloth::FabricCookedData fabricCookedData = CreateTestFabricCookedData();

        AZStd::unique_ptr<NvCloth::Fabric> fabric = m_factory.CreateFabric(fabricCookedData);

        EXPECT_TRUE(fabric.get() != nullptr);
        EXPECT_TRUE(fabric->m_id.IsValid());
        EXPECT_TRUE(fabric->m_nvFabric.get() != nullptr);
        EXPECT_TRUE(fabric->m_numClothsUsingFabric == 0);
        EXPECT_EQ(fabric->m_id, fabricCookedData.m_id);
        ExpectEq(fabric->m_cookedData, fabricCookedData);
    }

    TEST_F(NvClothSystemFactory, Factory_CreateClothNoInitialParticles_ReturnsNull)
    {
        AZStd::unique_ptr<NvCloth::Cloth> cloth = m_factory.CreateCloth({}, nullptr);

        EXPECT_TRUE(cloth.get() == nullptr);
    }

    TEST_F(NvClothSystemFactory, Factory_CreateClothInvalidFabric_ReturnsNull)
    {
        const float width = 1.0f;
        const float height = 1.0f;
        const AZ::u32 segmentsX = 5;
        const AZ::u32 segmentsY = 5;

        const TriangleInput planeXY = CreatePlane(width, height, segmentsX, segmentsY);

        AZStd::unique_ptr<NvCloth::Cloth> cloth = m_factory.CreateCloth(planeXY.m_vertices, nullptr);

        EXPECT_TRUE(cloth.get() == nullptr);
    }

    TEST_F(NvClothSystemFactory, Factory_CreateClothInitialParticlesMismatchFabricNumParticles_ReturnsNull)
    {
        const NvCloth::FabricCookedData fabricCookedData = CreateTestFabricCookedData();

        auto otherVertices = fabricCookedData.m_particles;
        otherVertices.resize(otherVertices.size() / 2);

        AZStd::unique_ptr<NvCloth::Fabric> fabric = m_factory.CreateFabric(fabricCookedData);

        AZStd::unique_ptr<NvCloth::Cloth> cloth = m_factory.CreateCloth(
            otherVertices, // otherVertices has a different number of vertices than fabric
            fabric.get());

        EXPECT_TRUE(cloth.get() == nullptr);
    }

    TEST_F(NvClothSystemFactory, Factory_CreateCloth_ReturnsValidCloth)
    {
        const NvCloth::FabricCookedData fabricCookedData = CreateTestFabricCookedData();

        AZStd::unique_ptr<NvCloth::Fabric> fabric = m_factory.CreateFabric(fabricCookedData);

        AZStd::unique_ptr<NvCloth::Cloth> cloth = m_factory.CreateCloth(fabricCookedData.m_particles, fabric.get());

        EXPECT_TRUE(cloth.get() != nullptr);
        EXPECT_TRUE(cloth->GetId().IsValid());
        EXPECT_EQ(cloth->GetFabric(), fabric.get());
        EXPECT_EQ(cloth->GetSolver(), nullptr);
        EXPECT_THAT(cloth->GetInitialParticles(), ::testing::Pointwise(ContainerIsCloseTolerance(Tolerance), fabricCookedData.m_particles));
        EXPECT_EQ(cloth->GetInitialIndices(), fabricCookedData.m_indices);
        EXPECT_THAT(cloth->GetParticles(), ::testing::Pointwise(ContainerIsCloseTolerance(Tolerance), fabricCookedData.m_particles));
        EXPECT_NE(cloth->GetClothConfigurator(), nullptr);
        ExpectEq(cloth->GetFabricCookedData(), fabricCookedData);
    }
} // namespace UnitTest
