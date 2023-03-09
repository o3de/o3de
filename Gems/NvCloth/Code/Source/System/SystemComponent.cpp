/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include <System/SystemComponent.h>
#include <Utils/Allocators.h>

// NvCloth library includes
#include <foundation/PxAllocatorCallback.h>
#include <foundation/PxErrorCallback.h>
#include <NvCloth/Callbacks.h>
#include <NvCloth/Solver.h>

namespace NvCloth
{
    namespace
    {
        // Implementation of the memory allocation callback interface using nvcloth allocator.
        class AzClothAllocatorCallback
            : public physx::PxAllocatorCallback
        {
            // NvCloth requires 16-byte alignment
            static const size_t alignment = 16;

            void* allocate(size_t size, [[maybe_unused]] const char* typeName, const char* filename, int line) override
            {
                void* ptr = AZ::AllocatorInstance<AzClothAllocator>::Get().Allocate(size, alignment, 0, "NvCloth", filename, line);
                AZ_Assert((reinterpret_cast<size_t>(ptr) & (alignment-1)) == 0, "NvCloth requires %zu-byte aligned memory allocations.", alignment);
                return ptr;
            }

            void deallocate(void* ptr) override
            {
                AZ::AllocatorInstance<AzClothAllocator>::Get().DeAllocate(ptr);
            }
        };

        // Implementation of the error callback interface directing nvcloth library errors to Open 3D Engine error output.
        class AzClothErrorCallback
            : public physx::PxErrorCallback
        {
        public:
            void reportError(physx::PxErrorCode::Enum code, [[maybe_unused]] const char* message, [[maybe_unused]] const char* file, [[maybe_unused]] int line) override
            {
                switch (code)
                {
                case physx::PxErrorCode::eDEBUG_INFO:
                case physx::PxErrorCode::eNO_ERROR:
                    AZ_TracePrintf("NvCloth", "PxErrorCode %i: %s (line %i in %s)", code, message, line, file);
                    break;

                case physx::PxErrorCode::eDEBUG_WARNING:
                case physx::PxErrorCode::ePERF_WARNING:
                    AZ_Warning("NvCloth", false, "PxErrorCode %i: %s (line %i in %s)", code, message, line, file);
                    break;

                default:
                    AZ_Error("NvCloth", false, "PxErrorCode %i: %s (line %i in %s)", code, message, line, file);
                    m_lastError = code;
                    break;
                }
            }

            physx::PxErrorCode::Enum GetLastError() const
            {
                return m_lastError;
            }

            void ResetLastError()
            {
                m_lastError = physx::PxErrorCode::eNO_ERROR;
            }

        private:
            physx::PxErrorCode::Enum m_lastError = physx::PxErrorCode::eNO_ERROR;
        };

        // Implementation of the assert handler interface directing nvcloth asserts to Open 3D Engine assertion system.
        class AzClothAssertHandler
            : public nv::cloth::PxAssertHandler
        {
        public:
            void operator()([[maybe_unused]] const char* exp, [[maybe_unused]] const char* file, [[maybe_unused]] int line, bool& ignore) override
            {
                AZ_UNUSED(ignore);
                AZ_Assert(false, "NvCloth library assertion failed in file %s:%d: %s", file, line, exp);
            }
        };

        // Implementation of the profiler callback interface for NvCloth.
        class AzClothProfilerCallback
            : public physx::PxProfilerCallback
        {
        public:
            void* zoneStart(const char* eventName, bool detached,
                [[maybe_unused]] uint64_t contextId) override
            {
                if (detached)
                {
                    AZ_PROFILE_INTERVAL_START(Cloth, AZ::Crc32(eventName), eventName);
                }
                else
                {
                    AZ_PROFILE_BEGIN(Cloth, eventName);
                }
                return nullptr;
            }

            void zoneEnd([[maybe_unused]] void* profilerData,
                [[maybe_unused]] const char* eventName, bool detached,
                [[maybe_unused]] uint64_t contextId) override
            {
                if (detached)
                {
                    AZ_PROFILE_INTERVAL_END(Cloth, AZ::Crc32(eventName));
                }
                else
                {
                    AZ_PROFILE_END(Cloth);
                }
            }
        };

        AZStd::unique_ptr<AzClothAllocatorCallback> ClothAllocatorCallback;
        AZStd::unique_ptr<AzClothErrorCallback> ClothErrorCallback;
        AZStd::unique_ptr<AzClothAssertHandler> ClothAssertHandler;
        AZStd::unique_ptr<AzClothProfilerCallback> ClothProfilerCallback;
    }

    void SystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SystemComponent, AZ::Component>()
                ->Version(0);

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<SystemComponent>("NvCloth", "Provides functionality for simulating cloth using NvCloth")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void SystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("NvClothService"));
    }

    void SystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("NvClothService"));
    }

    void SystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void SystemComponent::InitializeNvClothLibrary()
    {
        ClothAllocatorCallback = AZStd::make_unique<AzClothAllocatorCallback>();
        ClothErrorCallback = AZStd::make_unique<AzClothErrorCallback>();
        ClothAssertHandler = AZStd::make_unique<AzClothAssertHandler>();
        ClothProfilerCallback = AZStd::make_unique<AzClothProfilerCallback>();

        nv::cloth::InitializeNvCloth(
            ClothAllocatorCallback.get(),
            ClothErrorCallback.get(),
            ClothAssertHandler.get(),
            ClothProfilerCallback.get());
        AZ_Assert(CheckLastClothError(), "Failed to initialize NvCloth library");
    }

    void SystemComponent::TearDownNvClothLibrary()
    {
        // NvCloth library doesn't need any destruction
        ClothProfilerCallback.reset();
        ClothAssertHandler.reset();
        ClothErrorCallback.reset();
        ClothAllocatorCallback.reset();
    }

    bool SystemComponent::CheckLastClothError()
    {
        if (ClothErrorCallback)
        {
            return ClothErrorCallback->GetLastError() == physx::PxErrorCode::eNO_ERROR;
        }
        return false;
    }

    void SystemComponent::ResetLastClothError()
    {
        if (ClothErrorCallback)
        {
            return ClothErrorCallback->ResetLastError();
        }
    }

    void SystemComponent::Activate()
    {
        InitializeSystem();
    }

    void SystemComponent::Deactivate()
    {
        DestroySystem();
    }

    ISolver* SystemComponent::FindOrCreateSolver(const AZStd::string& name)
    {
        if (ISolver* solver = GetSolver(name))
        {
            return solver;
        }

        if (AZStd::unique_ptr<Solver> newSolver = m_factory->CreateSolver(name))
        {
            m_solvers.push_back(AZStd::move(newSolver));
            return m_solvers.back().get();
        }

        return nullptr;
    }

    void SystemComponent::DestroySolver(ISolver*& solver)
    {
        if (solver)
        {
            const AZStd::string& solverName = solver->GetName();

            auto solverIt = AZStd::find_if(m_solvers.begin(), m_solvers.end(),
                [&solverName](const auto& solverInstance)
                {
                    return solverInstance->GetName() == solverName;
                });

            if (solverIt != m_solvers.end())
            {
                // The solver will remove all its remaining cloths from it when destroyed
                m_solvers.erase(solverIt);
                solver = nullptr;
            }
        }
    }

    ISolver* SystemComponent::GetSolver(const AZStd::string& name)
    {
        auto solverIt = AZStd::find_if(m_solvers.begin(), m_solvers.end(),
            [&name](const auto& solverInstance)
            {
                return solverInstance->GetName() == name;
            });

        if (solverIt != m_solvers.end())
        {
            return solverIt->get();
        }

        return nullptr;
    }

    FabricId SystemComponent::FindOrCreateFabric(const FabricCookedData& fabricCookedData)
    {
        if (m_fabrics.count(fabricCookedData.m_id) != 0)
        {
            return fabricCookedData.m_id;
        }

        if (AZStd::unique_ptr<Fabric> newFabric = m_factory->CreateFabric(fabricCookedData))
        {
            m_fabrics[fabricCookedData.m_id] = AZStd::move(newFabric);
            return fabricCookedData.m_id;
        }

        return {}; // Returns invalid fabric id
    }

    void SystemComponent::DestroyFabric(FabricId fabricId)
    {
        if (auto fabricIt = m_fabrics.find(fabricId);
            fabricIt != m_fabrics.end())
        {
            // Destroy the fabric only if not used by any cloth
            if (fabricIt->second->m_numClothsUsingFabric <= 0)
            {
                m_fabrics.erase(fabricIt);
            }
        }
    }

    ICloth* SystemComponent::CreateCloth(
        const AZStd::vector<SimParticleFormat>& initialParticles,
        const FabricCookedData& fabricCookedData)
    {
        AZ_PROFILE_FUNCTION(Cloth);

        FabricId fabricId = FindOrCreateFabric(fabricCookedData);
        if (!fabricId.IsValid())
        {
            AZ_Warning("NvCloth", false, "Failed to create cloth because it couldn't create the fabric.");
            return nullptr;
        }

        if (auto newCloth = m_factory->CreateCloth(initialParticles, m_fabrics[fabricId].get()))
        {
            ClothId newClothId = newCloth->GetId();
            auto newClothIt = m_cloths.insert({ newClothId, AZStd::move(newCloth) }).first;
            return newClothIt->second.get();
        }
        else
        {
            DestroyFabric(fabricId);
        }

        return nullptr;
    }

    void SystemComponent::DestroyCloth(ICloth*& cloth)
    {
        if (cloth)
        {
            FabricId fabricId = cloth->GetFabricCookedData().m_id;

            // Cloth will decrement its fabric's counter on destruction.
            // In addition, if the cloth still remains added into a solver, it will remove itself from it.
            m_cloths.erase(cloth->GetId());
            cloth = nullptr;

            DestroyFabric(fabricId);
        }
    }

    ICloth* SystemComponent::GetCloth(ClothId clothId)
    {
        if (auto clothIt = m_cloths.find(clothId);
            clothIt != m_cloths.end())
        {
            return clothIt->second.get();
        }
        else
        {
            return nullptr;
        }
    }

    bool SystemComponent::AddCloth(ICloth* cloth, const AZStd::string& solverName)
    {
        if (cloth)
        {
            ISolver* solver = GetSolver(solverName);
            if (!solver)
            {
                return false;
            }

            Cloth* clothInstance = azdynamic_cast<Cloth*>(cloth);
            AZ_Assert(clothInstance, "Dynamic casting from ICloth to Cloth failed.");

            Solver* solverInstance = azdynamic_cast<Solver*>(solver);
            AZ_Assert(solverInstance, "Dynamic casting from ISolver to Solver failed.");

            solverInstance->AddCloth(clothInstance);

            return true;
        }

        return false;
    }

    void SystemComponent::RemoveCloth(ICloth* cloth)
    {
        if (cloth)
        {
            Cloth* clothInstance = azdynamic_cast<Cloth*>(cloth);
            AZ_Assert(clothInstance, "Dynamic casting from ICloth to Cloth failed.");

            Solver* solverInstance = clothInstance->GetSolver();
            if (solverInstance)
            {
                solverInstance->RemoveCloth(clothInstance);
            }
        }
    }

    void SystemComponent::OnTick(
        float deltaTime,
        [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        AZ_PROFILE_FUNCTION(Cloth);

        for (auto& solverIt : m_solvers)
        {
            if (!solverIt->IsUserSimulated())
            {
                solverIt->StartSimulation(deltaTime);
                solverIt->FinishSimulation();
            }
        }
    }

    int SystemComponent::GetTickOrder()
    {
        return AZ::TICK_PHYSICS;
    }

    void SystemComponent::InitializeSystem()
    {
        // Create Factory
        m_factory = AZStd::make_unique<Factory>();
        m_factory->Init();

        // Create Default Solver
        [[maybe_unused]] ISolver* solver = FindOrCreateSolver(DefaultSolverName);
        AZ_Assert(solver, "Error: Default solver failed to be created");

        AZ::Interface<IClothSystem>::Register(this);
        AZ::TickBus::Handler::BusConnect();
    }

    void SystemComponent::DestroySystem()
    {
        AZ::TickBus::Handler::BusDisconnect();
        AZ::Interface<IClothSystem>::Unregister(this);

        // Destroy Cloths
        m_cloths.clear();

        // Destroy Fabrics
        m_fabrics.clear();

        // Destroy Solvers
        m_solvers.clear();

        // Destroy Factory
        m_factory->Destroy();
        m_factory.reset();
    }

} // namespace NvCloth
