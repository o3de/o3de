/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AZTestShared/Math/MathTestHelpers.h>

#include <AzCore/Interface/Interface.h>
#include <AzCore/Component/TickBus.h>

#include <UnitTestHelper.h>
#include <TriangleInputHelper.h>

#include <NvCloth/IClothSystem.h>
#include <NvCloth/ISolver.h>
#include <NvCloth/ICloth.h>
#include <NvCloth/IFabricCooker.h>

#include <System/Cloth.h>
#include <System/Solver.h>

namespace UnitTest
{
    TEST(NvClothSystem, ClothSystem_DefaultSolver_Exists)
    {
        NvCloth::ISolver* defaultSolver = AZ::Interface<NvCloth::IClothSystem>::Get()->GetSolver(NvCloth::DefaultSolverName);

        EXPECT_TRUE(defaultSolver != nullptr);
        EXPECT_TRUE(defaultSolver->GetName() == NvCloth::DefaultSolverName);
    }

    TEST(NvClothSystem, ClothSystem_FindOrCreateSolverDefaultName_ReturnsDefaultSolver)
    {
        NvCloth::ISolver* defaultSolverFromGetter = AZ::Interface<NvCloth::IClothSystem>::Get()->GetSolver(NvCloth::DefaultSolverName);
        NvCloth::ISolver* defaultSolverFromFindOrCreateFunc = AZ::Interface<NvCloth::IClothSystem>::Get()->FindOrCreateSolver(NvCloth::DefaultSolverName);

        EXPECT_EQ(defaultSolverFromGetter, defaultSolverFromFindOrCreateFunc);
    }

    TEST(NvClothSystem, ClothSystem_FindOrCreateSolverEmptyName_ReturnsNull)
    {
        NvCloth::ISolver* solver = AZ::Interface<NvCloth::IClothSystem>::Get()->FindOrCreateSolver("");

        EXPECT_TRUE(solver == nullptr);
    }

    TEST(NvClothSystem, ClothSystem_FindOrCreateSolver_ReturnsValidSolver)
    {
        const AZStd::string solverName = "Solver_FindOrCreateSolver";

        NvCloth::ISolver* solver = AZ::Interface<NvCloth::IClothSystem>::Get()->FindOrCreateSolver(solverName);

        EXPECT_TRUE(solver != nullptr);
        EXPECT_TRUE(solver->GetName() == solverName);

        // NOTE: IClothSystem is persistent as it's part of the test environment.
        //       Destroying solver to avoid leaving it in the environment.
        AZ::Interface<NvCloth::IClothSystem>::Get()->DestroySolver(solver);
    }

    TEST(NvClothSystem, ClothSystem_FindOrCreateSolverTwice_ReturnsSameSolver)
    {
        const AZStd::string solverName = "Solver_FindOrCreateSolverTwice";

        NvCloth::ISolver* solverA = AZ::Interface<NvCloth::IClothSystem>::Get()->FindOrCreateSolver(solverName);
        NvCloth::ISolver* solverB = AZ::Interface<NvCloth::IClothSystem>::Get()->FindOrCreateSolver(solverName);

        EXPECT_EQ(solverA, solverB);

        // NOTE: IClothSystem is persistent as it's part of the test environment.
        //       Destroying solver to avoid leaving it in the environment.
        AZ::Interface<NvCloth::IClothSystem>::Get()->DestroySolver(solverA);
    }

    TEST(NvClothSystem, ClothSystem_DestroySolverNullptr_DoesNotFail)
    {
        NvCloth::ISolver* solver = nullptr;

        AZ::Interface<NvCloth::IClothSystem>::Get()->DestroySolver(solver);

        EXPECT_TRUE(solver == nullptr);
    }

    TEST(NvClothSystem, ClothSystem_DestroySolver_SolverIsDestroyed)
    {
        const AZStd::string solverName = "Solver_DestroySolver";

        NvCloth::ISolver* solver = AZ::Interface<NvCloth::IClothSystem>::Get()->FindOrCreateSolver(solverName);

        EXPECT_TRUE(solver != nullptr);

        AZ::Interface<NvCloth::IClothSystem>::Get()->DestroySolver(solver);

        EXPECT_TRUE(solver == nullptr);
    }

    TEST(NvClothSystem, ClothSystem_GetSolverEmpyName_ReturnsNull)
    {
        NvCloth::ISolver* solver = AZ::Interface<NvCloth::IClothSystem>::Get()->GetSolver("");

        EXPECT_TRUE(solver == nullptr);
    }

    TEST(NvClothSystem, ClothSystem_GetSolverUnknownName_ReturnsNull)
    {
        const AZStd::string solverName = "Solver_GetSolverUnknownName";

        NvCloth::ISolver* solver = AZ::Interface<NvCloth::IClothSystem>::Get()->GetSolver(solverName);

        EXPECT_TRUE(solver == nullptr);
    }

    TEST(NvClothSystem, ClothSystem_GetSolver_ReturnsSolver)
    {
        const AZStd::string solverName = "Solver_GetSolver";

        NvCloth::ISolver* solver = AZ::Interface<NvCloth::IClothSystem>::Get()->FindOrCreateSolver(solverName);
        NvCloth::ISolver* solverFromGetter = AZ::Interface<NvCloth::IClothSystem>::Get()->GetSolver(solverName);

        EXPECT_TRUE(solver != nullptr);
        EXPECT_EQ(solver, solverFromGetter);

        AZ::Interface<NvCloth::IClothSystem>::Get()->DestroySolver(solver);

        solverFromGetter = AZ::Interface<NvCloth::IClothSystem>::Get()->GetSolver(solverName);

        EXPECT_TRUE(solverFromGetter == nullptr);
    }

    TEST(NvClothSystem, ClothSystem_CreateClothNoInitialParticles_ReturnsNull)
    {
        NvCloth::ICloth* cloth = AZ::Interface<NvCloth::IClothSystem>::Get()->CreateCloth({}, {});

        EXPECT_TRUE(cloth == nullptr);
    }

    TEST(NvClothSystem, ClothSystem_CreateClothInvalidFabric_ReturnsNull)
    {
        const float width = 1.0f;
        const float height = 1.0f;
        const AZ::u32 segmentsX = 5;
        const AZ::u32 segmentsY = 5;

        const TriangleInput planeXY = CreatePlane(width, height, segmentsX, segmentsY);

        NvCloth::ICloth* cloth = AZ::Interface<NvCloth::IClothSystem>::Get()->CreateCloth(planeXY.m_vertices, {});

        EXPECT_TRUE(cloth == nullptr);
    }

    TEST(NvClothSystem, ClothSystem_CreateClothInitialParticlesMismatchFabricNumParticles_ReturnsNull)
    {
        const NvCloth::FabricCookedData fabricCookedData = CreateTestFabricCookedData();

        auto otherVertices = fabricCookedData.m_particles;
        otherVertices.resize(otherVertices.size() / 2);

        NvCloth::ICloth* cloth = AZ::Interface<NvCloth::IClothSystem>::Get()->CreateCloth(
            otherVertices, // otherVertices has a different number of vertices than FabricCookedData
            fabricCookedData);

        EXPECT_TRUE(cloth == nullptr);
    }

    TEST(NvClothSystem, ClothSystem_CreateCloth_ReturnsValidCloth)
    {
        const NvCloth::FabricCookedData fabricCookedData = CreateTestFabricCookedData();

        NvCloth::ICloth* cloth = AZ::Interface<NvCloth::IClothSystem>::Get()->CreateCloth(fabricCookedData.m_particles, fabricCookedData);

        EXPECT_TRUE(cloth != nullptr);
        EXPECT_TRUE(cloth->GetId().IsValid());
        EXPECT_THAT(cloth->GetInitialParticles(), ::testing::Pointwise(ContainerIsCloseTolerance(Tolerance), fabricCookedData.m_particles));
        EXPECT_EQ(cloth->GetInitialIndices(), fabricCookedData.m_indices);
        EXPECT_THAT(cloth->GetParticles(), ::testing::Pointwise(ContainerIsCloseTolerance(Tolerance), fabricCookedData.m_particles));
        EXPECT_NE(cloth->GetClothConfigurator(), nullptr);
        ExpectEq(cloth->GetFabricCookedData(), fabricCookedData);

        // NOTE: IClothSystem is persistent as it's part of the test environment.
        //       Destroying cloth to avoid leaving it in the environment.
        AZ::Interface<NvCloth::IClothSystem>::Get()->DestroyCloth(cloth);
    }

    TEST(NvClothSystem, ClothSystem_DestroyClothNullptr_DoesNotFail)
    {
        NvCloth::ICloth* cloth = nullptr;

        AZ::Interface<NvCloth::IClothSystem>::Get()->DestroyCloth(cloth);

        EXPECT_TRUE(cloth == nullptr);
    }

    TEST(NvClothSystem, ClothSystem_DestroyCloth_ClothIsDestroyed)
    {
        const NvCloth::FabricCookedData fabricCookedData = CreateTestFabricCookedData();

        NvCloth::ICloth* cloth = AZ::Interface<NvCloth::IClothSystem>::Get()->CreateCloth(fabricCookedData.m_particles, fabricCookedData);

        EXPECT_TRUE(cloth != nullptr);
        EXPECT_TRUE(cloth->GetId().IsValid());

        AZ::Interface<NvCloth::IClothSystem>::Get()->DestroyCloth(cloth);

        EXPECT_TRUE(cloth == nullptr);
    }

    TEST(NvClothSystem, ClothSystem_GetClothInvalidId_ReturnsNull)
    {
        const NvCloth::ClothId clothId;

        NvCloth::ICloth* cloth = AZ::Interface<NvCloth::IClothSystem>::Get()->GetCloth(clothId);

        EXPECT_TRUE(cloth == nullptr);
    }

    TEST(NvClothSystem, ClothSystem_GetClothUnknownId_ReturnsNull)
    {
        const NvCloth::ClothId clothId(5);

        NvCloth::ICloth* cloth = AZ::Interface<NvCloth::IClothSystem>::Get()->GetCloth(clothId);

        EXPECT_TRUE(cloth == nullptr);
    }

    TEST(NvClothSystem, ClothSystem_GetCloth_ReturnsCloth)
    {
        const NvCloth::FabricCookedData fabricCookedData = CreateTestFabricCookedData();

        NvCloth::ICloth* cloth = AZ::Interface<NvCloth::IClothSystem>::Get()->CreateCloth(fabricCookedData.m_particles, fabricCookedData);

        EXPECT_TRUE(cloth != nullptr);
        EXPECT_TRUE(cloth->GetId().IsValid());

        NvCloth::ICloth* clothFromGetter = AZ::Interface<NvCloth::IClothSystem>::Get()->GetCloth(cloth->GetId());

        EXPECT_TRUE(clothFromGetter != nullptr);

        // NOTE: IClothSystem is persistent as it's part of the test environment.
        //       Destroying cloth to avoid leaving it in the environment.
        AZ::Interface<NvCloth::IClothSystem>::Get()->DestroyCloth(cloth);
    }

    TEST(NvClothSystem, ClothSystem_AddClothNull_ReturnsFalse)
    {
        NvCloth::ICloth* cloth = nullptr;

        bool clothAdded = AZ::Interface<NvCloth::IClothSystem>::Get()->AddCloth(cloth);

        EXPECT_FALSE(clothAdded);
    }

    TEST(NvClothSystem, ClothSystem_AddClothToInvalidSolver_ReturnsFalse)
    {
        const AZStd::string solverName = "";

        const NvCloth::FabricCookedData fabricCookedData = CreateTestFabricCookedData();
        NvCloth::ICloth* cloth = AZ::Interface<NvCloth::IClothSystem>::Get()->CreateCloth(fabricCookedData.m_particles, fabricCookedData);

        bool clothAdded = AZ::Interface<NvCloth::IClothSystem>::Get()->AddCloth(cloth, solverName);

        EXPECT_FALSE(clothAdded);

        // NOTE: IClothSystem is persistent as it's part of the test environment.
        //       Destroying cloth to avoid leaving it in the environment.
        AZ::Interface<NvCloth::IClothSystem>::Get()->DestroyCloth(cloth);
    }

    TEST(NvClothSystem, ClothSystem_AddClothToNonExistentSolver_ReturnsFalse)
    {
        const AZStd::string solverName = "Solver_AddClothToNonExistentSolver";

        const NvCloth::FabricCookedData fabricCookedData = CreateTestFabricCookedData();
        NvCloth::ICloth* cloth = AZ::Interface<NvCloth::IClothSystem>::Get()->CreateCloth(fabricCookedData.m_particles, fabricCookedData);

        bool clothAdded = AZ::Interface<NvCloth::IClothSystem>::Get()->AddCloth(cloth, solverName);

        EXPECT_FALSE(clothAdded);

        // NOTE: IClothSystem is persistent as it's part of the test environment.
        //       Destroying cloth to avoid leaving it in the environment.
        AZ::Interface<NvCloth::IClothSystem>::Get()->DestroyCloth(cloth);
    }

    TEST(NvClothSystem, ClothSystem_AddClothToDefaultSolver_ReturnsTrue)
    {
        NvCloth::ISolver* defaultSolver = AZ::Interface<NvCloth::IClothSystem>::Get()->GetSolver(NvCloth::DefaultSolverName);
        EXPECT_TRUE(azrtti_cast<NvCloth::Solver*>(defaultSolver)->GetNumCloths() == 0);

        const NvCloth::FabricCookedData fabricCookedData = CreateTestFabricCookedData();
        NvCloth::ICloth* cloth = AZ::Interface<NvCloth::IClothSystem>::Get()->CreateCloth(fabricCookedData.m_particles, fabricCookedData);

        bool clothAdded = AZ::Interface<NvCloth::IClothSystem>::Get()->AddCloth(cloth);

        EXPECT_TRUE(clothAdded);
        EXPECT_TRUE(azrtti_cast<NvCloth::Cloth*>(cloth)->GetSolver()->GetName() == NvCloth::DefaultSolverName);
        EXPECT_TRUE(azrtti_cast<NvCloth::Solver*>(defaultSolver)->GetNumCloths() == 1);

        // NOTE: IClothSystem is persistent as it's part of the test environment.
        //       Destroying cloth to avoid leaving it in the environment.
        AZ::Interface<NvCloth::IClothSystem>::Get()->DestroyCloth(cloth);
    }

    TEST(NvClothSystem, ClothSystem_AddClothToSolver_ReturnsTrue)
    {
        NvCloth::ISolver* solver = AZ::Interface<NvCloth::IClothSystem>::Get()->FindOrCreateSolver("Solver_AddClothToSolver");
        EXPECT_TRUE(azrtti_cast<NvCloth::Solver*>(solver)->GetNumCloths() == 0);

        const NvCloth::FabricCookedData fabricCookedData = CreateTestFabricCookedData();
        NvCloth::ICloth* cloth = AZ::Interface<NvCloth::IClothSystem>::Get()->CreateCloth(fabricCookedData.m_particles, fabricCookedData);

        bool clothAdded = AZ::Interface<NvCloth::IClothSystem>::Get()->AddCloth(cloth, solver->GetName());

        EXPECT_TRUE(clothAdded);
        EXPECT_TRUE(azrtti_cast<NvCloth::Cloth*>(cloth)->GetSolver()->GetName() == solver->GetName());
        EXPECT_TRUE(azrtti_cast<NvCloth::Solver*>(solver)->GetNumCloths() == 1);

        // NOTE: IClothSystem is persistent as it's part of the test environment.
        //       Destroying cloth and solver to avoid leaving it in the environment.
        AZ::Interface<NvCloth::IClothSystem>::Get()->DestroyCloth(cloth);
        AZ::Interface<NvCloth::IClothSystem>::Get()->DestroySolver(solver);
    }

    TEST(NvClothSystem, ClothSystem_AddClothTwice_NothingHappensSecondTime)
    {
        NvCloth::ISolver* defaultSolver = AZ::Interface<NvCloth::IClothSystem>::Get()->GetSolver(NvCloth::DefaultSolverName);
        EXPECT_TRUE(azrtti_cast<NvCloth::Solver*>(defaultSolver)->GetNumCloths() == 0);

        const NvCloth::FabricCookedData fabricCookedData = CreateTestFabricCookedData();
        NvCloth::ICloth* cloth = AZ::Interface<NvCloth::IClothSystem>::Get()->CreateCloth(fabricCookedData.m_particles, fabricCookedData);

        EXPECT_TRUE(azrtti_cast<NvCloth::Cloth*>(cloth)->GetSolver() == nullptr);

        bool clothAdded = AZ::Interface<NvCloth::IClothSystem>::Get()->AddCloth(cloth);
        EXPECT_TRUE(clothAdded);
        EXPECT_TRUE(azrtti_cast<NvCloth::Cloth*>(cloth)->GetSolver()->GetName() == NvCloth::DefaultSolverName);
        EXPECT_TRUE(azrtti_cast<NvCloth::Solver*>(defaultSolver)->GetNumCloths() == 1);

        clothAdded = AZ::Interface<NvCloth::IClothSystem>::Get()->AddCloth(cloth);
        EXPECT_TRUE(clothAdded);
        EXPECT_TRUE(azrtti_cast<NvCloth::Cloth*>(cloth)->GetSolver()->GetName() == NvCloth::DefaultSolverName);
        EXPECT_TRUE(azrtti_cast<NvCloth::Solver*>(defaultSolver)->GetNumCloths() == 1);

        // NOTE: IClothSystem is persistent as it's part of the test environment.
        //       Destroying cloth to avoid leaving it in the environment.
        AZ::Interface<NvCloth::IClothSystem>::Get()->DestroyCloth(cloth);
    }

    TEST(NvClothSystem, ClothSystem_RemoveClothNull_NothingHappens)
    {
        NvCloth::ICloth* cloth = nullptr;

        AZ::Interface<NvCloth::IClothSystem>::Get()->RemoveCloth(cloth);

        EXPECT_TRUE(cloth == nullptr);
    }

    TEST(NvClothSystem, ClothSystem_RemoveClothTwice_NothingHappensSecondTime)
    {
        NvCloth::ISolver* defaultSolver = AZ::Interface<NvCloth::IClothSystem>::Get()->GetSolver(NvCloth::DefaultSolverName);
        EXPECT_TRUE(azrtti_cast<NvCloth::Solver*>(defaultSolver)->GetNumCloths() == 0);

        const NvCloth::FabricCookedData fabricCookedData = CreateTestFabricCookedData();
        NvCloth::ICloth* cloth = AZ::Interface<NvCloth::IClothSystem>::Get()->CreateCloth(fabricCookedData.m_particles, fabricCookedData);

        bool clothAdded = AZ::Interface<NvCloth::IClothSystem>::Get()->AddCloth(cloth);
        EXPECT_TRUE(clothAdded);
        EXPECT_TRUE(azrtti_cast<NvCloth::Cloth*>(cloth)->GetSolver()->GetName() == NvCloth::DefaultSolverName);
        EXPECT_TRUE(azrtti_cast<NvCloth::Solver*>(defaultSolver)->GetNumCloths() == 1);

        AZ::Interface<NvCloth::IClothSystem>::Get()->RemoveCloth(cloth);
        EXPECT_TRUE(azrtti_cast<NvCloth::Cloth*>(cloth)->GetSolver() == nullptr);
        EXPECT_TRUE(azrtti_cast<NvCloth::Solver*>(defaultSolver)->GetNumCloths() == 0);

        AZ::Interface<NvCloth::IClothSystem>::Get()->RemoveCloth(cloth);
        EXPECT_TRUE(azrtti_cast<NvCloth::Cloth*>(cloth)->GetSolver() == nullptr);
        EXPECT_TRUE(azrtti_cast<NvCloth::Solver*>(defaultSolver)->GetNumCloths() == 0);

        // NOTE: IClothSystem is persistent as it's part of the test environment.
        //       Destroying cloth to avoid leaving it in the environment.
        AZ::Interface<NvCloth::IClothSystem>::Get()->DestroyCloth(cloth);
    }

    TEST(NvClothSystem, ClothSystem_Tick_SolverAndClothIsUpdated)
    {
        const float deltaTimeSim = 1.0f / 60.0f;

        NvCloth::ISolver* solver = AZ::Interface<NvCloth::IClothSystem>::Get()->FindOrCreateSolver("Solver_Tick");

        const NvCloth::FabricCookedData fabricCookedData = CreateTestFabricCookedData();
        NvCloth::ICloth* cloth = AZ::Interface<NvCloth::IClothSystem>::Get()->CreateCloth(fabricCookedData.m_particles, fabricCookedData);

        bool solverPreSimulationEventSignaled = false;
        NvCloth::ISolver::PreSimulationEvent::Handler solverPreSimulationEventHandler(
            [&solverPreSimulationEventSignaled](const AZStd::string&, float)
            {
                solverPreSimulationEventSignaled = true;
            });

        bool solverPostSimulationEventSignaled = false;
        NvCloth::ISolver::PostSimulationEvent::Handler solverPostSimulationEventHandler(
            [&solverPostSimulationEventSignaled](const AZStd::string&, float)
            {
                solverPostSimulationEventSignaled = true;
            });

        bool clothPreSimulationEventSignaled = false;
        NvCloth::ICloth::PreSimulationEvent::Handler clothPreSimulationEventHandler(
            [&clothPreSimulationEventSignaled](NvCloth::ClothId, float)
            {
                clothPreSimulationEventSignaled = true;
            });

        bool clothPostSimulationEventSignaled = false;
        NvCloth::ICloth::PostSimulationEvent::Handler clothPostSimulationEventHandler(
            [&clothPostSimulationEventSignaled](NvCloth::ClothId, float, const AZStd::vector<NvCloth::SimParticleFormat>&)
            {
                clothPostSimulationEventSignaled = true;
            });

        solver->ConnectPreSimulationEventHandler(solverPreSimulationEventHandler);
        solver->ConnectPostSimulationEventHandler(solverPostSimulationEventHandler);
        cloth->ConnectPreSimulationEventHandler(clothPreSimulationEventHandler);
        cloth->ConnectPostSimulationEventHandler(clothPostSimulationEventHandler);

        AZ::Interface<NvCloth::IClothSystem>::Get()->AddCloth(cloth, solver->GetName()); // It needs at least one cloth in the solver to simulate

        // Ticking Cloth System to update all its solvers
        AZ::TickBus::Broadcast(&AZ::TickEvents::OnTick,
            deltaTimeSim,
            AZ::ScriptTimePoint(AZStd::chrono::steady_clock::now()));

        EXPECT_TRUE(solverPreSimulationEventSignaled);
        EXPECT_TRUE(solverPostSimulationEventSignaled);
        EXPECT_TRUE(clothPreSimulationEventSignaled);
        EXPECT_TRUE(clothPostSimulationEventSignaled);

        solverPreSimulationEventSignaled = false;
        solverPostSimulationEventSignaled = false;
        clothPreSimulationEventSignaled = false;
        clothPostSimulationEventSignaled = false;
        AZ::Interface<NvCloth::IClothSystem>::Get()->RemoveCloth(cloth); // Leave solver without have any cloths

        // Ticking Cloth System to update all its solvers
        AZ::TickBus::Broadcast(&AZ::TickEvents::OnTick,
            deltaTimeSim,
            AZ::ScriptTimePoint(AZStd::chrono::steady_clock::now()));

        EXPECT_FALSE(solverPreSimulationEventSignaled);
        EXPECT_FALSE(solverPostSimulationEventSignaled);
        EXPECT_FALSE(clothPreSimulationEventSignaled);
        EXPECT_FALSE(clothPostSimulationEventSignaled);

        // NOTE: IClothSystem is persistent as it's part of the test environment.
        //       Destroying cloth and solver to avoid leaving it in the environment.
        AZ::Interface<NvCloth::IClothSystem>::Get()->DestroyCloth(cloth);
        AZ::Interface<NvCloth::IClothSystem>::Get()->DestroySolver(solver);
    }
} // namespace UnitTest
