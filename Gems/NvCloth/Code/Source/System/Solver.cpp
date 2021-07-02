/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <System/Solver.h>
#include <System/Cloth.h>

#include <AzCore/Jobs/JobFunction.h>

// NvCloth library includes
#include <NvCloth/Solver.h>


namespace NvCloth
{
    Solver::Solver(const AZStd::string& name, NvSolverUniquePtr nvSolver)
        : m_name(name)
        , m_nvSolver(AZStd::move(nvSolver))
    {
    }

    Solver::~Solver()
    {
        AZ_Assert(!m_isSimulating, "Please make sure the ongoing simulation is finished");

        // Remove any remaining cloths from the solver
        while (!m_cloths.empty())
        {
            RemoveClothInternal(m_cloths.begin());
        }
    }

    void Solver::AddCloth(Cloth* cloth)
    {
        AZ_Assert(!m_isSimulating, "Please make sure the ongoing simulation is finished before attempting to add cloth");

        // If the cloth was already added to a solver then remove it first.
        if (Solver* previousSolver = cloth->GetSolver())
        {
            // If it's already added to this solver then don't do anything.
            if (previousSolver->GetName() == GetName())
            {
                return;
            }

            previousSolver->RemoveCloth(cloth);
        }

        m_cloths.push_back(cloth);

        cloth->m_solver = this;

        m_nvSolver->addCloth(cloth->m_nvCloth.get());
    }

    void Solver::RemoveCloth(Cloth* cloth)
    {
        AZ_Assert(!m_isSimulating, "Please make sure the ongoing simulation is finished before attempting to remove cloth");

        if (cloth->GetSolver() != nullptr &&
            cloth->GetSolver()->GetName() == GetName())
        {
            auto clothIt = AZStd::find(m_cloths.begin(), m_cloths.end(), cloth);
            AZ_Assert(clothIt != m_cloths.end(), "Cloth indicates it is part of solver %s, but the solver doesn't contain it.", GetName().c_str());

            RemoveClothInternal(clothIt);
        }
    }

    size_t Solver::GetNumCloths() const
    {
        return m_cloths.size();
    }

    const AZStd::string& Solver::GetName() const
    {
        return m_name;
    }

    void Solver::Enable(bool value)
    {
        m_enabled = value;
    }

    bool Solver::IsEnabled() const
    {
        return m_enabled;
    }

    void Solver::SetUserSimulated(bool value)
    {
        m_userSimulated = value;
    }

    bool Solver::IsUserSimulated() const
    {
        return m_userSimulated;
    }

    void Solver::StartSimulation(float deltaTime)
    {
        if (!IsEnabled() || m_cloths.empty())
        {
            return;
        }

        AZ_Assert(!m_isSimulating, "Please make sure the ongoing simulation is finished before attempting to start a new one");

        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Cloth);

        m_deltaTime = deltaTime;
        m_simulationCompletion.Reset(true /*isClearDependent*/);

        m_preSimulationEvent.Signal(m_name, deltaTime);

        // Set isSimulating flag after the pre-simulation event is sent in case if there are handlers adding/removing cloth from the solver.
        m_isSimulating = true;

        // Setup the chain of jobs for the simulation pass

        // Post simulation jobs will unlock the entire simulation pass completion.
        ClothsPostSimulationJob* clothsPostSimulationJob = aznew ClothsPostSimulationJob(&m_cloths, m_deltaTime, &m_simulationCompletion);
        clothsPostSimulationJob->SetDependent(&m_simulationCompletion);

        // Simulation jobs will unlock the post simulation job.
        ClothsSimulationJob* clothsSimulationJob = aznew ClothsSimulationJob(m_nvSolver.get(), m_deltaTime, clothsPostSimulationJob);
        clothsSimulationJob->SetDependent(clothsPostSimulationJob);

        // Pre-simulation jobs will unlock the simulation job.
        ClothsPreSimulationJob* clothsPreSimulationJob = aznew ClothsPreSimulationJob(&m_cloths, m_deltaTime, clothsSimulationJob);
        clothsPreSimulationJob->SetDependent(clothsSimulationJob);

        // Start the jobs.
        clothsPreSimulationJob->Start();
        clothsSimulationJob->Start();
        clothsPostSimulationJob->Start();
    }

    void Solver::FinishSimulation()
    {
        if (!m_isSimulating)
        {
            return;
        }

        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Cloth);

        // Waiting for the simulation pass completition.
        m_simulationCompletion.StartAndWaitForCompletion();
        m_isSimulating = false;

        m_postSimulationEvent.Signal(m_name, m_deltaTime);
    }

    void Solver::SetInterCollisionDistance(float distance)
    {
        m_nvSolver->setInterCollisionDistance(distance);
    }

    void Solver::SetInterCollisionStiffness(float stiffness)
    {
        m_nvSolver->setInterCollisionStiffness(stiffness);
    }

    void Solver::SetInterCollisionIterations(AZ::u32 iterations)
    {
        m_nvSolver->setInterCollisionNbIterations(iterations);
    }

    // Note: Requires a valid cloth iterator that does not point to end()
    void Solver::RemoveClothInternal(Cloths::iterator clothIt)
    {
        m_nvSolver->removeCloth((*clothIt)->m_nvCloth.get());

        (*clothIt)->m_solver = nullptr;

        m_cloths.erase(clothIt);
    }

    Solver::ClothsSimulationJob::ClothsSimulationJob(nv::cloth::Solver* solver, float deltaTime,
        AZ::Job* continuationJob, AZ::JobContext* context) : Job(true /*isAutoDelete*/, context)
        , m_solver(solver)
        , m_continuationJob(continuationJob)
        , m_deltaTime(deltaTime)
    {
    }

    void Solver::ClothsSimulationJob::Process()
    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::Cloth, "NvCloth::BeginSimulationJob");

        if (m_solver->beginSimulation(m_deltaTime))
        {
            // Setup the end simulation job.
            AZ::Job* endSimulationJob = AZ::CreateJobFunction([solver = m_solver]
            {
                AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::Cloth, "NvCloth::EndSimulationJob");
                solver->endSimulation();
            }, true /*isAutoDelete*/);

            // Setup chunk simulation jobs.
            const int simulationChunkCount = m_solver->getSimulationChunkCount();

            for (int chunkIndex = 0; chunkIndex < simulationChunkCount; ++chunkIndex)
            {
                AZ::Job* chunkSimulationJob = AZ::CreateJobFunction([solver = m_solver, chunkIndex]
                {
                    AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::Cloth, "NvCloth::ChunkSimulationJob");
                    solver->simulateChunk(chunkIndex);
                }, true /*isAutoDelete*/);

                // Setup job dependency to make sure the End simulation job runs _after_ all chunks are finished simulating
                chunkSimulationJob->SetDependent(endSimulationJob);
                chunkSimulationJob->Start();
            }

            // After the end simulation job is done, the next job in the chain is allowed to run
            endSimulationJob->SetDependentStarted(m_continuationJob);
            endSimulationJob->Start();
        }

        // Note that if beginSimulation returns false, we don't block the continuation job from running.
        // This is expected behavior.
    }

    Solver::ClothsPostSimulationJob::ClothsPostSimulationJob(const Cloths* cloths, float deltaTime,
        AZ::Job* continuationJob, AZ::JobContext* context) : Job(true /*isAutoDelete*/, context)
        , m_cloths(cloths)
        , m_continuationJob(continuationJob)
        , m_deltaTime(deltaTime)
    {
    }

    void Solver::ClothsPostSimulationJob::Process()
    {
        for (Cloth* cloth : *m_cloths)
        {
            AZ::Job* eventSignalJob = AZ::CreateJobFunction([cloth, deltaTime = m_deltaTime]
            {
                AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::Cloth, "NvCloth::PostSimulationJob");

                // Update the cloth data after the simulation
                cloth->Update();

                // Issue post-simulation events
                cloth->m_postSimulationEvent.Signal(cloth->GetId(), deltaTime, cloth->GetParticles());
            }, true /*isAutoDelete*/);

            eventSignalJob->SetDependentStarted(m_continuationJob);
            eventSignalJob->Start();
        }
    }


    Solver::ClothsPreSimulationJob::ClothsPreSimulationJob(const Cloths* cloths, float deltaTime,
        AZ::Job* continuationJob, AZ::JobContext* context) : Job(true /*isAutoDelete*/, context)
        , m_cloths(cloths)
        , m_continuationJob(continuationJob)
        , m_deltaTime(deltaTime)
    {
    }

    void Solver::ClothsPreSimulationJob::Process()
    {
        for (Cloth* cloth : *m_cloths)
        {
            AZ::Job* eventSignalJob = AZ::CreateJobFunction([cloth, deltaTime = m_deltaTime]
            {
                AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::Cloth, "NvCloth::PreSimulationJob");

                // Issue pre-simulation events
                cloth->m_preSimulationEvent.Signal(cloth->GetId(), deltaTime);
            }, true /*isAutoDelete*/);

            eventSignalJob->SetDependentStarted(m_continuationJob);
            eventSignalJob->Start();
        }
    }

} // namespace NvCloth
