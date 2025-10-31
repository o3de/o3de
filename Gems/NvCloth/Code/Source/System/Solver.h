/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Jobs/Job.h>
#include <AzCore/Jobs/JobCompletion.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/unordered_set.h>

#include <NvCloth/ISolver.h>

#include <System/NvTypes.h>

namespace NvCloth
{
    class Cloth;

    //! Implementation of the ISolver interface.
    //!
    //! When enabled, it runs the simulation on all its cloths and sends
    //! notifications before and after the simulation has been executed.
    class Solver
        : public ISolver
    {
    public:
        AZ_RTTI(Solver, "{111055FC-F590-4BCD-A7B9-D96B1C44E3E8}", ISolver);

        Solver(const AZStd::string& name, NvSolverUniquePtr nvSolver);
        ~Solver();

        void AddCloth(Cloth* cloth);
        void RemoveCloth(Cloth* cloth);
        size_t GetNumCloths() const;

        // ISolver overrides ...
        const AZStd::string& GetName() const override;
        void Enable(bool value) override;
        bool IsEnabled() const override;
        void SetUserSimulated(bool value) override;
        bool IsUserSimulated() const override;
        void StartSimulation(float deltaTime) override;
        void FinishSimulation() override;
        void SetInterCollisionDistance(float distance) override;
        void SetInterCollisionStiffness(float stiffness) override;
        void SetInterCollisionIterations(AZ::u32 iterations) override;

    private:
        using Cloths = AZStd::vector<Cloth*>;

        class ClothsPreSimulationJob
            : public AZ::Job
        {
        public:
            AZ_CLASS_ALLOCATOR(ClothsPreSimulationJob, AZ::ThreadPoolAllocator);

            ClothsPreSimulationJob(const Cloths* cloths, float deltaTime,
                AZ::Job* continuationJob, AZ::JobContext* context = nullptr);

            void Process() override;

        private:
            // List of cloths to do the pre-simulation work for.
            const Cloths* m_cloths = nullptr;

            // The job to run after all pre-simulation jobs are completed.
            AZ::Job* m_continuationJob = nullptr;

            // Delta time for the current simulation pass.
            float m_deltaTime = 0.0f;
        };

        class ClothsSimulationJob
            : public AZ::Job
        {
        public:
            AZ_CLASS_ALLOCATOR(Solver::ClothsSimulationJob, AZ::ThreadPoolAllocator)

            ClothsSimulationJob(nv::cloth::Solver* solver, float deltaTime,
                AZ::Job* continuationJob, AZ::JobContext* context = nullptr);

            void Process() override;

        private:
            // NvCloth solver object to simulate.
            nv::cloth::Solver* m_solver = nullptr;

            // The job to run after all simulation jobs are completed.
            AZ::Job* m_continuationJob = nullptr;

            // Delta time for the current simulation pass.
            float m_deltaTime = 0.0f;
        };

        class ClothsPostSimulationJob
            : public AZ::Job
        {
        public:
            AZ_CLASS_ALLOCATOR(ClothsPostSimulationJob, AZ::ThreadPoolAllocator);

            ClothsPostSimulationJob(const Cloths* cloths, float deltaTime,
                AZ::Job* continuationJob, AZ::JobContext* context = nullptr);

            void Process() override;

        private:
            // List of cloths to do the post-simulation work for.
            const Cloths* m_cloths = nullptr;

            // The job to run after all post-simulation jobs are completed.
            AZ::Job* m_continuationJob = nullptr;

            // Delta time for the current simulation pass.
            float m_deltaTime = 0.0f;
        };

        void RemoveClothInternal(Cloths::iterator clothIt);

        // Name of the solver.
        AZStd::string m_name;

        // NvCloth solver object.
        NvSolverUniquePtr m_nvSolver;

        // When enabled the solver will be simulated and its events signaled.
        bool m_enabled = true;

        // When user-simulated the user will have the responsibility of calling Simulate function.
        bool m_userSimulated = false;

        // List of Cloth instances added to this solver.
        Cloths m_cloths;

        // Stored delta time during the simulation.
        float m_deltaTime = 0.0f;

        // Flag indicating if the simulation jobs are currently running.
        bool m_isSimulating = false;

        // Simulation synchronization job
        AZ::JobCompletion m_simulationCompletion;
    };

} // namespace NvCloth
