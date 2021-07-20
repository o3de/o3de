/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <System/Factory.h>
#include <System/Solver.h>
#include <System/Fabric.h>
#include <System/Cloth.h>
#include <System/SystemComponent.h>

// NvCloth library includes
#include <NvCloth/Range.h>
#include <NvCloth/Factory.h>

namespace NvCloth
{
    namespace
    {
        AZ::u64 ClothIdCounter = 1;
    }

    void Factory::Init()
    {
        // Create a CPU NvCloth Factory
        if (!m_nvFactory)
        {
            m_nvFactory = NvFactoryUniquePtr(NvClothCreateFactoryCPU());
            AZ_Assert(m_nvFactory, "Failed to create CPU cloth factory");

            if (SystemComponent::CheckLastClothError())
            {
                AZ_Printf("Cloth", "NVIDIA NvCloth Gem using CPU for cloth simulation.\n");
            }
            else
            {
                AZ_Error("Cloth", false, "NvCloth library failed to create CPU factory.");
            }
        }
    }

    void Factory::Destroy()
    {
        m_nvFactory.reset();
    }

    AZStd::unique_ptr<Solver> Factory::CreateSolver(const AZStd::string& name)
    {
        if (name.empty())
        {
            AZ_Warning("NvCloth", false, "Factory failed to create solver because name passed is empty.");
            return nullptr;
        }

        NvSolverUniquePtr nvSolver(
            m_nvFactory->createSolver());
        if (!nvSolver)
        {
            AZ_Warning("NvCloth", false, "Factory failed to create solver %s.", name.c_str());
            return nullptr;
        }

        return AZStd::make_unique<Solver>(
            name,
            AZStd::move(nvSolver));
    }

    AZStd::unique_ptr<Fabric> Factory::CreateFabric(const FabricCookedData& fabricCookedData)
    {
        if (!fabricCookedData.m_id.IsValid())
        {
            AZ_Warning("NvCloth", false, "Factory failed to create fabric because the id of the fabric cooked data passed is not valid.");
            return nullptr;
        }

        NvFabricUniquePtr nvFabric(
            m_nvFactory->createFabric(
                fabricCookedData.m_internalData.m_numParticles,
                ToNvRange(fabricCookedData.m_internalData.m_phaseIndices),
                ToNvRange(fabricCookedData.m_internalData.m_sets),
                ToNvRange(fabricCookedData.m_internalData.m_restValues),
                ToNvRange(fabricCookedData.m_internalData.m_stiffnessValues),
                ToNvRange(fabricCookedData.m_internalData.m_indices),
                ToNvRange(fabricCookedData.m_internalData.m_anchors),
                ToNvRange(fabricCookedData.m_internalData.m_tetherLengths),
                ToNvRange(fabricCookedData.m_internalData.m_triangles)));
        if (!nvFabric)
        {
            AZ_Warning("NvCloth", false, "Factory failed to create fabric.");
            return nullptr;
        }

        return AZStd::make_unique<Fabric>(
            fabricCookedData,
            AZStd::move(nvFabric));
    }

    AZStd::unique_ptr<Cloth> Factory::CreateCloth(
        const AZStd::vector<SimParticleFormat>& initialParticles,
        Fabric* fabric)
    {
        if (initialParticles.empty())
        {
            AZ_Warning("NvCloth", false, "Factory failed to create cloth because no particles were provided.");
            return nullptr;
        }

        if (!fabric)
        {
            AZ_Warning("NvCloth", false, "Factory failed to create cloth because fabric provided is invalid.");
            return nullptr;
        }

        if (initialParticles.size() != fabric->m_cookedData.m_particles.size())
        {
            AZ_Warning("NvCloth", false, "Factory failed to create cloth because the number of initial particles provided (%d) didn't match the fabric's (%d).",
                initialParticles.size(), fabric->m_cookedData.m_particles.size());
            return nullptr;
        }

        NvClothUniquePtr nvCloth(
            m_nvFactory->createCloth(
                ToPxVec4NvRange(initialParticles),
                *fabric->m_nvFabric.get()));
        if (!nvCloth)
        {
            AZ_Warning("NvCloth", false, "Factory failed to create cloth.");
            return nullptr;
        }

        return AZStd::make_unique<Cloth>(
            ClothId(ClothIdCounter++),
            initialParticles,
            fabric,
            AZStd::move(nvCloth));
    }

} // namespace NvCloth
