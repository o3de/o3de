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

#include <System/Factory.h>
#include <System/Solver.h>
#include <System/Fabric.h>
#include <System/FabricCooker.h>
#include <System/Cloth.h>

#include <NvCloth/IClothSystem.h>

namespace UnitTest
{
    //! Sets up a solver and cloth for each test case.
    //! It also allows to create additional cloths and solvers.
    class NvClothSystemSolver
        : public ::testing::Test
    {
    protected:
        // ::testing::Test overrides ...
        void SetUp() override;
        void TearDown() override;

        AZStd::unique_ptr<NvCloth::Solver> CreateSolver(const AZStd::string& name);
        AZStd::unique_ptr<NvCloth::Cloth> CreateCloth();

        const AZStd::string m_solverName = "SolverTest";
        AZStd::unique_ptr<NvCloth::Solver> m_solver;
        AZStd::unique_ptr<NvCloth::Cloth> m_cloth;

    private:
        void CreateFabric();

        NvCloth::Factory m_factory;
        AZStd::unique_ptr<NvCloth::Fabric> m_fabric;
    };

    void NvClothSystemSolver::SetUp()
    {
        m_factory.Init();
        m_solver = CreateSolver(m_solverName);
        CreateFabric();
        m_cloth = CreateCloth();
    }

    void NvClothSystemSolver::TearDown()
    {
        m_cloth.reset();
        m_fabric.reset();
        m_solver.reset();
        m_factory.Destroy();
    }

    AZStd::unique_ptr<NvCloth::Solver> NvClothSystemSolver::CreateSolver(const AZStd::string& name)
    {
        return m_factory.CreateSolver(name);
    }

    AZStd::unique_ptr<NvCloth::Cloth> NvClothSystemSolver::CreateCloth()
    {
        return m_factory.CreateCloth(m_fabric->m_cookedData.m_particles, m_fabric.get());
    }

    void NvClothSystemSolver::CreateFabric()
    {
        const NvCloth::FabricCookedData fabricCookedData = CreateTestFabricCookedData();

        m_fabric = m_factory.CreateFabric(fabricCookedData);
    }

    TEST_F(NvClothSystemSolver, Solver_AddAndRemoveCloth_NumClothsIncrementAndDecrementInSolver)
    {
        EXPECT_EQ(m_solver->GetNumCloths(), 0);
        EXPECT_EQ(m_cloth->GetSolver(), nullptr);

        m_solver->AddCloth(m_cloth.get());

        EXPECT_EQ(m_solver->GetNumCloths(), 1);
        EXPECT_EQ(m_cloth->GetSolver(), m_solver.get());

        m_solver->RemoveCloth(m_cloth.get());

        EXPECT_EQ(m_solver->GetNumCloths(), 0);
        EXPECT_EQ(m_cloth->GetSolver(), nullptr);
    }

    TEST_F(NvClothSystemSolver, Solver_AddSameClothTwice_SameClothIsNotAddedTwiceToSolver)
    {
        m_solver->AddCloth(m_cloth.get());

        EXPECT_EQ(m_solver->GetNumCloths(), 1);
        EXPECT_EQ(m_cloth->GetSolver(), m_solver.get());

        m_solver->AddCloth(m_cloth.get()); // Second addition of the same m_cloth

        EXPECT_EQ(m_solver->GetNumCloths(), 1); // Number of Cloths should remain the same
        EXPECT_EQ(m_cloth->GetSolver(), m_solver.get());
    }

    TEST_F(NvClothSystemSolver, Solver_RemoveClothNotInSolver_DoesNotAffectSolver)
    {
        m_solver->AddCloth(m_cloth.get());

        EXPECT_EQ(m_solver->GetNumCloths(), 1);
        EXPECT_EQ(m_cloth->GetSolver(), m_solver.get());

        auto newCloth = CreateCloth();
        m_solver->RemoveCloth(newCloth.get());

        EXPECT_EQ(m_solver->GetNumCloths(), 1);
        EXPECT_EQ(m_cloth->GetSolver(), m_solver.get());
    }

    TEST_F(NvClothSystemSolver, Solver_ClothDestroyedWhileInASolver_ClothIsRemovedFromSolver)
    {
        auto newCloth = CreateCloth();
        m_solver->AddCloth(newCloth.get());

        EXPECT_EQ(m_solver->GetNumCloths(), 1);
        EXPECT_EQ(newCloth->GetSolver(), m_solver.get());

        newCloth.reset();

        EXPECT_EQ(m_solver->GetNumCloths(), 0);
    }

    TEST_F(NvClothSystemSolver, Solver_SolverDestroyedWhileStillHavingACloth_ClothIsRemovedFromSolver)
    {
        auto newSolver = CreateSolver("NewSolver");
        auto newCloth = CreateCloth();

        newSolver->AddCloth(newCloth.get());

        EXPECT_EQ(newSolver->GetNumCloths(), 1);
        EXPECT_EQ(newCloth->GetSolver(), newSolver.get());

        newSolver.reset();

        EXPECT_EQ(newCloth->GetSolver(), nullptr);
    }

    TEST_F(NvClothSystemSolver, Solver_ClothAddedToASecondSolver_ClothIsRemovedFromTheFirstSolver)
    {
        m_solver->AddCloth(m_cloth.get());

        EXPECT_EQ(m_solver->GetNumCloths(), 1);
        EXPECT_EQ(m_cloth->GetSolver(), m_solver.get());

        auto anotherSolver = CreateSolver("AnotherSolver");
        anotherSolver->AddCloth(m_cloth.get());

        EXPECT_EQ(m_solver->GetNumCloths(), 0);
        EXPECT_EQ(anotherSolver->GetNumCloths(), 1);
        EXPECT_EQ(m_cloth->GetSolver(), anotherSolver.get());
    }

    TEST_F(NvClothSystemSolver, Solver_StartAndFinishSimulation_SimulationEventsSignaledWhenEnabled)
    {
        const float deltaTimeSim = 1.0f / 60.0f;

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

        m_solver->ConnectPreSimulationEventHandler(solverPreSimulationEventHandler);
        m_solver->ConnectPostSimulationEventHandler(solverPostSimulationEventHandler);

        m_solver->AddCloth(m_cloth.get()); // Solver needs at least one cloth to simulate

        m_solver->StartSimulation(deltaTimeSim);
        m_solver->FinishSimulation();

        EXPECT_TRUE(solverPreSimulationEventSignaled);
        EXPECT_TRUE(solverPostSimulationEventSignaled);

        solverPreSimulationEventSignaled = false;
        solverPostSimulationEventSignaled = false;
        m_solver->Enable(false);

        m_solver->StartSimulation(deltaTimeSim);
        m_solver->FinishSimulation();

        EXPECT_FALSE(solverPreSimulationEventSignaled);
        EXPECT_FALSE(solverPostSimulationEventSignaled);
    }

    TEST_F(NvClothSystemSolver, Solver_StartAndFinishSimulationWithNoCloths_SimulationEventsNotSignaled)
    {
        const float deltaTimeSim = 1.0f / 60.0f;

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

        m_solver->ConnectPreSimulationEventHandler(solverPreSimulationEventHandler);
        m_solver->ConnectPostSimulationEventHandler(solverPostSimulationEventHandler);

        m_solver->AddCloth(m_cloth.get()); // Solver needs at least one cloth to simulate

        m_solver->StartSimulation(deltaTimeSim);
        m_solver->FinishSimulation();

        EXPECT_TRUE(solverPreSimulationEventSignaled);
        EXPECT_TRUE(solverPostSimulationEventSignaled);

        solverPreSimulationEventSignaled = false;
        solverPostSimulationEventSignaled = false;
        m_solver->RemoveCloth(m_cloth.get()); // Leave solver without have any cloths

        m_solver->StartSimulation(deltaTimeSim);
        m_solver->FinishSimulation();

        EXPECT_FALSE(solverPreSimulationEventSignaled);
        EXPECT_FALSE(solverPostSimulationEventSignaled);
    }

    TEST_F(NvClothSystemSolver, Solver_PreAndPostSimulationEvent_SolverNameAndDeltaTimePassedAsArgumentsMatch)
    {
        const float deltaTimeSim = 1.0f / 60.0f;

        NvCloth::ISolver::PreSimulationEvent::Handler solverPreSimulationEventHandler(
            [this, deltaTimeSim](const AZStd::string& solverName, float deltaTime)
            {
                EXPECT_EQ(m_solverName, solverName);
                EXPECT_NEAR(deltaTimeSim, deltaTime, Tolerance);
            });

        NvCloth::ISolver::PostSimulationEvent::Handler solverPostSimulationEventHandler(
            [this, deltaTimeSim](const AZStd::string& solverName, float deltaTime)
            {
                EXPECT_EQ(m_solverName, solverName);
                EXPECT_NEAR(deltaTimeSim, deltaTime, Tolerance);
            });

        m_solver->ConnectPreSimulationEventHandler(solverPreSimulationEventHandler);
        m_solver->ConnectPostSimulationEventHandler(solverPostSimulationEventHandler);

        m_solver->AddCloth(m_cloth.get()); // It needs at least one cloth to simulate

        m_solver->StartSimulation(deltaTimeSim);
        m_solver->FinishSimulation();
    }

    TEST_F(NvClothSystemSolver, Solver_StartAndFinishSimulation_SignalsClothSimulationEvents)
    {
        const float deltaTimeSim = 1.0f / 60.0f;

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

        m_cloth->ConnectPreSimulationEventHandler(clothPreSimulationEventHandler);
        m_cloth->ConnectPostSimulationEventHandler(clothPostSimulationEventHandler);

        m_solver->AddCloth(m_cloth.get());

        m_solver->StartSimulation(deltaTimeSim);
        m_solver->FinishSimulation();

        EXPECT_TRUE(clothPreSimulationEventSignaled);
        EXPECT_TRUE(clothPostSimulationEventSignaled);
    }

    TEST_F(NvClothSystemSolver, Solver_StartAndFinishSimulation_ClothSimulationEventParametersMatch)
    {
        const float deltaTimeSim = 1.0f / 60.0f;

        NvCloth::ICloth::PreSimulationEvent::Handler clothPreSimulationEventHandler(
            [cloth = m_cloth.get(), deltaTimeSim](NvCloth::ClothId clothId, float deltaTime)
            {
                EXPECT_EQ(cloth->GetId(), clothId);
                EXPECT_NEAR(deltaTimeSim, deltaTime, Tolerance);
            });

        NvCloth::ICloth::PostSimulationEvent::Handler clothPostSimulationEventHandler(
            [cloth = m_cloth.get(), deltaTimeSim](NvCloth::ClothId clothId, float deltaTime, const AZStd::vector<NvCloth::SimParticleFormat>& updatedParticles)
            {
                EXPECT_EQ(cloth->GetId(), clothId);
                EXPECT_NEAR(deltaTimeSim, deltaTime, Tolerance);
                EXPECT_THAT(cloth->GetParticles(), ::testing::Pointwise(ContainerIsCloseTolerance(Tolerance), updatedParticles));
            });

        m_cloth->ConnectPreSimulationEventHandler(clothPreSimulationEventHandler);
        m_cloth->ConnectPostSimulationEventHandler(clothPostSimulationEventHandler);

        m_solver->AddCloth(m_cloth.get());

        m_solver->StartSimulation(deltaTimeSim);
        m_solver->FinishSimulation();
    }

    // This test uses Cloth System to check if the system's tick will update a solver in user simulated mode.
    // Since it relies on cloth system, the test has to use a solver and a cloth created from the system.
    // NvClothSystemSolver fixture is not necessary for this test.
    TEST(NvClothSystem, Solver_StartAndFinishSimulationCalledBySystem_SimulationEventsSignaledWhenSolverIsNotUserSimulated)
    {
        const float deltaTimeSim = 1.0f / 60.0f;

        // Create a solver using Cloth System
        NvCloth::ISolver* solver = AZ::Interface<NvCloth::IClothSystem>::Get()->FindOrCreateSolver("Solver_UserSimulatedTest");

        // Create a cloth using Cloth System
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

        solver->ConnectPreSimulationEventHandler(solverPreSimulationEventHandler);
        solver->ConnectPostSimulationEventHandler(solverPostSimulationEventHandler);

        AZ::Interface<NvCloth::IClothSystem>::Get()->AddCloth(cloth, solver->GetName()); // Solver needs at least one cloth to simulate

        // Ticking Cloth System updates all its solvers
        AZ::TickBus::Broadcast(&AZ::TickEvents::OnTick,
            deltaTimeSim,
            AZ::ScriptTimePoint(AZStd::chrono::steady_clock::now()));

        EXPECT_TRUE(solverPreSimulationEventSignaled);
        EXPECT_TRUE(solverPostSimulationEventSignaled);

        solverPreSimulationEventSignaled = false;
        solverPostSimulationEventSignaled = false;
        solver->SetUserSimulated(true);

        // Ticking Cloth System updates all its solvers
        AZ::TickBus::Broadcast(&AZ::TickEvents::OnTick,
            deltaTimeSim,
            AZ::ScriptTimePoint(AZStd::chrono::steady_clock::now()));

        EXPECT_FALSE(solverPreSimulationEventSignaled);
        EXPECT_FALSE(solverPostSimulationEventSignaled);

        // Manually calling simulation (as expected when solver is in used simulated mode)
        solver->StartSimulation(deltaTimeSim);
        solver->FinishSimulation();

        EXPECT_TRUE(solverPreSimulationEventSignaled);
        EXPECT_TRUE(solverPostSimulationEventSignaled);

        AZ::Interface<NvCloth::IClothSystem>::Get()->DestroySolver(solver);
        AZ::Interface<NvCloth::IClothSystem>::Get()->DestroyCloth(cloth);
    }
} // namespace UnitTest
