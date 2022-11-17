/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <System/PhysXSystem.h>

#include <PxPhysicsAPI.h>
#include <PhysX/Debug/PhysXDebugConfiguration.h>
#include <Scene/PhysXScene.h>
#include <System/PhysXAllocator.h>
#include <System/PhysXCpuDispatcher.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/ComponentApplicationLifecycle.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Debug/PerformanceCollector.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/PlatformId/PlatformId.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

// only enable physx timestep warning when not running debug or in Release
#if !defined(DEBUG) && !defined(RELEASE)
#define ENABLE_PHYSX_TIMESTEP_WARNING
#endif


namespace PhysX
{
    AZ_CVAR(bool, physx_batchTransformSync, false, nullptr, AZ::ConsoleFunctorFlags::Null,
        "Batch entity transform syncs for the entire simulation pass. "
        "True: Sync entity transform once per Simulate call. "
        "False: Sync entity transform for every simulation sub-step.");

    AZ_CLASS_ALLOCATOR_IMPL(PhysXSystem, AZ::SystemAllocator, 0);

#ifdef ENABLE_PHYSX_TIMESTEP_WARNING
    namespace FrameTimeWarning
    {
        static constexpr int MaxSamples = 1000;
        static int NumSamples = 0;
        static int NumSamplesOverLimit = 0;
        static float LostTime = 0.0f;
    }

    AZ_CVAR(bool, physx_reportTimestepWarnings, false, nullptr, AZ::ConsoleFunctorFlags::Null, "A flag providing ability to turn on/off reporting of PhysX timestep warnings");
#endif

    // A helper function.
    AZ::Debug::PerformanceCollector::DataLogType GetDataLogTypeFromCVar(const AZ::CVarFixedString& newCaptureType)
    {
        if (newCaptureType.starts_with('a') || newCaptureType.starts_with('A'))
        {
            return AZ::Debug::PerformanceCollector::DataLogType::LogAllSamples;
        }
        else
        {
            return AZ::Debug::PerformanceCollector::DataLogType::LogStatistics;
        }
    }

    AZ_CVAR(AZ::u32, physx_metricsFrameCountPerCaptureBatch, 60,
        [](const AZ::u32& newValue)
        {
            PhysX::GetPhysXSystem()->GetPerformanceCollector()->UpdateFrameCountPerCaptureBatch(newValue);
        },
        AZ::ConsoleFunctorFlags::DontReplicate, "Number of frames in which performance will be measured per batch.");

    AZ_CVAR(AZ::u32, physx_metricsNumberOfCaptureBatches, 0,
        [](const AZ::u32& newValue)
        {
            PhysX::GetPhysXSystem()->GetPerformanceCollector()->UpdateNumberOfCaptureBatches(newValue);
        },
        AZ::ConsoleFunctorFlags::DontReplicate,
            "Collects and reports PhysX performance in this number of batches. "
            "Starts at 0, which means do not capture performance data. "
            "When this variable changes to > 0 we'll start performance capture.");

    AZ_CVAR(AZ::CVarFixedString, physx_metricsDataLogType, "statistical",
        [](const AZ::CVarFixedString& newValue)
        {
            PhysX::GetPhysXSystem()->GetPerformanceCollector()->UpdateDataLogType(GetDataLogTypeFromCVar(newValue));
        },
        AZ::ConsoleFunctorFlags::DontReplicate, "Defines the kind of data collection and logging. "
            "If starts with 's' it will log statistical summaries (average, min, max, stdev), "
            "if starts with 'a' or 'A' will log all samples of data (high verbosity). Default=s");

    AZ_CVAR(AZ::u32, physx_metricsWaitTimePerCaptureBatch, 0,
        [](const AZ::u32& newValue)
        {
            PhysX::GetPhysXSystem()->GetPerformanceCollector()->UpdateWaitTimeBeforeEachBatch(AZStd::chrono::seconds(newValue));
        },
        AZ::ConsoleFunctorFlags::DontReplicate, "How many seconds to wait before each batch of performance capture.");


    PhysXSystem::PhysXSystem(AZStd::unique_ptr<PhysXSettingsRegistryManager> registryManager, const physx::PxCookingParams& cookingParams)
        : m_registryManager(AZStd::move(registryManager))
        , m_sceneInterface(this)
    {
        // Start PhysX allocator
        AZ::AllocatorInstance<PhysXAllocator>::Create();

        InitializePhysXSdk(cookingParams);

        InitializePerformanceCollector();
    }

    void PhysXSystem::InitializePerformanceCollector()
    {
        auto performanceMetrics = AZStd::to_array<AZStd::string_view>({
            PerformanceSpecPhysXSimulationTime,
        });

        AZStd::string platformName = AZ::GetPlatformName(AZ::g_currentPlatform);
        auto logCategory =
            AZStd::string::format("%.*s-%s", AZ_STRING_ARG(PerformanceLogCategory), platformName.c_str());
        m_performanceCollector = AZStd::make_unique<AZ::Debug::PerformanceCollector>(
            logCategory,
            performanceMetrics,
            [](AZ::u32)
            {
            });

        m_performanceCollector->UpdateDataLogType(GetDataLogTypeFromCVar(physx_metricsDataLogType));
        m_performanceCollector->UpdateFrameCountPerCaptureBatch(physx_metricsFrameCountPerCaptureBatch);
        m_performanceCollector->UpdateWaitTimeBeforeEachBatch(AZStd::chrono::seconds(physx_metricsWaitTimePerCaptureBatch));
        m_performanceCollector->UpdateNumberOfCaptureBatches(physx_metricsNumberOfCaptureBatches);
    }

    PhysXSystem::~PhysXSystem()
    {
        Shutdown();
        ShutdownPhysXSdk();
        AZ::AllocatorInstance<PhysXAllocator>::Destroy();
    }

    void PhysXSystem::Initialize(const AzPhysics::SystemConfiguration* config)
    {
        if (m_state == State::Initialized)
        {
            AZ_Warning("PhysXSystem", false, "PhysX system already initialized, Shutdown must be called first OR call Reinitialize or UpdateConfiguration(forceReinit=true) to reboot");
            return;
        }

        if (const auto* physXConfig = azdynamic_cast<const PhysXSystemConfiguration*>(config))
        {
            m_systemConfig = *physXConfig;
        }

        m_state = State::Initialized;
        m_initializeEvent.Signal(&m_systemConfig);
    }

    void PhysXSystem::Reinitialize()
    {
        //To be implemented with LYN-1146
        AZ_Warning("PhysXSystem", false, "PhysX Reinitialize currently not supported.");
    }

    void PhysXSystem::Shutdown()
    {
        if (m_state != State::Initialized)
        {
            return;
        }

        RemoveAllScenes();

        m_accumulatedTime = 0.0f;
        m_state = State::Shutdown;
    }

    void PhysXSystem::Simulate(float deltaTime)
    {
        AZ_PROFILE_FUNCTION(Physics);

        if (m_state != State::Initialized)
        {
            AZ_Warning("PhysXSystem", false, "Call Simulate when PhysX system is not initialized");
            return;
        }

        auto simulateScenes = [this](float timeStep)
        {
            for (auto& scenePtr : m_sceneList)
            {
                if (scenePtr != nullptr && scenePtr->IsEnabled())
                {
                    AZ::Debug::ScopeDuration performanceScopeDuration(m_performanceCollector.get(), PerformanceSpecPhysXSimulationTime);
                    scenePtr->StartSimulation(timeStep);
                    scenePtr->FinishSimulation();
                }
            }
        };

#ifdef ENABLE_PHYSX_TIMESTEP_WARNING
        if (FrameTimeWarning::NumSamples < FrameTimeWarning::MaxSamples)
        {
            FrameTimeWarning::NumSamples++;
            if (deltaTime > m_systemConfig.m_maxTimestep)
            {
                FrameTimeWarning::NumSamplesOverLimit++;
                FrameTimeWarning::LostTime += deltaTime - m_systemConfig.m_maxTimestep;
            }
        }
        else
        {
            AZ_Warning("PhysXSystem", !physx_reportTimestepWarnings || FrameTimeWarning::NumSamplesOverLimit <= 0,
                "[%d] of [%d] frames had a deltatime over the Max physics timestep[%.6f]. Physx timestep was clamped on those frames, losing [%.6f] seconds.",
                FrameTimeWarning::NumSamplesOverLimit, FrameTimeWarning::NumSamples, m_systemConfig.m_maxTimestep, FrameTimeWarning::LostTime);
            FrameTimeWarning::NumSamples = 0;
            FrameTimeWarning::NumSamplesOverLimit = 0;
            FrameTimeWarning::LostTime = 0.0f;
        }
#endif
        deltaTime = AZ::GetClamp(deltaTime, 0.0f, m_systemConfig.m_maxTimestep);

        AZ_Assert(m_systemConfig.m_fixedTimestep >= 0.0f, "PhysXSystem - fixed timestep is negitive.");
        float tickTime = deltaTime;
        if (m_systemConfig.m_fixedTimestep > 0.0f) //use the fixed timestep
        {
            m_accumulatedTime += tickTime;
            //divide accumulated time by the fixed step and floor it to get the number of steps that would occur. Then multiply by fixedTimeStep to get the total executed time.
            tickTime = AZStd::floorf(m_accumulatedTime / m_systemConfig.m_fixedTimestep) * m_systemConfig.m_fixedTimestep;
            m_preSimulateEvent.Signal(tickTime);

            while (m_accumulatedTime >= m_systemConfig.m_fixedTimestep)
            {
                simulateScenes(m_systemConfig.m_fixedTimestep);
                m_accumulatedTime -= m_systemConfig.m_fixedTimestep;
            }
        }
        else
        {
            m_preSimulateEvent.Signal(tickTime);

            simulateScenes(tickTime);
        }
        
        // Flush performance data for this tick
        m_performanceCollector->FrameTick();
        
        if (physx_batchTransformSync)
        {
            for (auto& scenePtr : m_sceneList)
            {
                if (scenePtr != nullptr && scenePtr->IsEnabled())
                {
                    PhysXScene* physxScene = static_cast<PhysXScene*>(scenePtr.get());
                    physxScene->FlushTransformSync();
                }
            }
        }

        m_postSimulateEvent.Signal(tickTime);
    }

    AzPhysics::SceneHandle PhysXSystem::AddScene(const AzPhysics::SceneConfiguration& config)
    {
        if (config.m_sceneName.empty())
        {
            AZ_Error("PhysXSystem", false, "AddScene: Trying to Add a scene without a name. SceneConfiguration::m_sceneName must have a value");
            return AzPhysics::InvalidSceneHandle;
        }

        if (!m_freeSceneSlots.empty()) //fill any free slots first before increasing the size of the scene list vector.
        {
            AzPhysics::SceneIndex freeIndex = m_freeSceneSlots.front();
            m_freeSceneSlots.pop();
            AZ_Assert(freeIndex < m_sceneList.size(), "PhysXSystem::AddScene: Free scene index is out of bounds!");
            AZ_Assert(m_sceneList[freeIndex] == nullptr, "PhysXSystem::AddScene: Free scene index is not free");

            const AzPhysics::SceneHandle sceneHandle(AZ::Crc32(config.m_sceneName), freeIndex);
            m_sceneList[freeIndex] = AZStd::make_unique<PhysXScene>(config, sceneHandle);
            m_sceneAddedEvent.Signal(sceneHandle);
            return sceneHandle;
        }

        if (m_sceneList.size() < std::numeric_limits<AzPhysics::SceneIndex>::max()) //add a new scene if it is under the limit
        {
            const AzPhysics::SceneHandle sceneHandle(AZ::Crc32(config.m_sceneName), static_cast<AzPhysics::SceneIndex>(m_sceneList.size()));
            m_sceneList.emplace_back(AZStd::make_unique<PhysXScene>(config, sceneHandle));
            m_sceneAddedEvent.Signal(sceneHandle);
            return sceneHandle;
        }
        AZ_Warning("Physx", false, "Scene Limit reached[%d], unable to add new scene [%s]",
            std::numeric_limits<AzPhysics::SceneIndex>::max(),
            config.m_sceneName.c_str());
        return AzPhysics::InvalidSceneHandle;
    }

    AzPhysics::SceneHandleList PhysXSystem::AddScenes(const AzPhysics::SceneConfigurationList& configs)
    {
        AzPhysics::SceneHandleList sceneHandles;
        sceneHandles.reserve(configs.size());
        for (const auto& config : configs)
        {
            AzPhysics::SceneHandle sceneHandle = AddScene(config);
            sceneHandles.emplace_back(sceneHandle);
        }
        return sceneHandles;
    }

    AzPhysics::SceneHandle PhysXSystem::GetSceneHandle(const AZStd::string& sceneName)
    {
        const AZ::Crc32 sceneCrc(sceneName);
        auto sceneItr = AZStd::find_if(m_sceneList.begin(), m_sceneList.end(), [sceneCrc](auto& scene) {
            return scene != nullptr && sceneCrc == scene->GetId();
            });

        if (sceneItr != m_sceneList.end())
        {
            return AzPhysics::SceneHandle((*sceneItr)->GetId(), static_cast<AzPhysics::SceneIndex>(AZStd::distance(m_sceneList.begin(), sceneItr)));
        }
        return AzPhysics::InvalidSceneHandle;
    }

    AzPhysics::Scene* PhysXSystem::GetScene(AzPhysics::SceneHandle handle)
    {
        if (handle == AzPhysics::InvalidSceneHandle)
        {
            return nullptr;
        }

        AzPhysics::SceneIndex index = AZStd::get<AzPhysics::HandleTypeIndex::Index>(handle);
        if (index < m_sceneList.size())
        {
            if (auto& scenePtr = m_sceneList[index];
                scenePtr != nullptr)
            {
                if (scenePtr->GetId() == AZStd::get<AzPhysics::HandleTypeIndex::Crc>(handle))
                {
                    return scenePtr.get();
                }
            }
        }
        return nullptr;
    }

    AzPhysics::SceneList PhysXSystem::GetScenes(const AzPhysics::SceneHandleList& handles)
    {
        AzPhysics::SceneList requestedSceneList;
        requestedSceneList.reserve(handles.size());
        for (const auto& handle : handles)
        {
            AzPhysics::Scene* scene = GetScene(handle);
            requestedSceneList.emplace_back(scene);
        }
        return requestedSceneList;
    }

    AzPhysics::SceneList& PhysXSystem::GetAllScenes()
    {
        return m_sceneList;
    }

    void PhysXSystem::RemoveScene(AzPhysics::SceneHandle handle)
    {
        if (handle == AzPhysics::InvalidSceneHandle)
        {
            return;
        }

        AZ::u64 index = AZStd::get<AzPhysics::HandleTypeIndex::Index>(handle);
        if (index < m_sceneList.size() )
        {
            if (auto& scenePtr = m_sceneList[index];
                scenePtr != nullptr)
            {
                if (scenePtr->GetId() == AZStd::get<AzPhysics::HandleTypeIndex::Crc>(handle))
                {
                    m_sceneRemovedEvent.Signal(handle);
                    m_sceneList[index].reset();
                    m_freeSceneSlots.push(static_cast<AzPhysics::SceneIndex>(index));
                }
            }
        }
    }

    void PhysXSystem::RemoveScenes(const AzPhysics::SceneHandleList& handles)
    {
        for (const auto& handle : handles)
        {
            RemoveScene(handle);
        }
    }

    void PhysXSystem::RemoveAllScenes()
    {
        m_sceneList.clear();

        //clear the free slots queue
        AZStd::queue<AzPhysics::SceneIndex> empty;
        m_freeSceneSlots.swap(empty);
    }

    AZStd::pair<AzPhysics::SceneHandle, AzPhysics::SimulatedBodyHandle> PhysXSystem::FindAttachedBodyHandleFromEntityId(AZ::EntityId entityId)
    {
        for (auto& scenePtr : m_sceneList)
        {
            if (scenePtr == nullptr)
            {
                continue;
            }
            if (auto* physXScene = azdynamic_cast<PhysXScene*>(scenePtr.get()))
            {
                for (const auto& [_, body] : physXScene->GetSimulatedBodyList())
                {
                    if (body != nullptr && body->GetEntityId() == entityId)
                    {
                        return AZStd::make_pair(physXScene->GetSceneHandle(), body->m_bodyHandle);
                    }
                }
            }
        }
        return AZStd::make_pair(AzPhysics::InvalidSceneHandle, AzPhysics::InvalidSimulatedBodyHandle);
    }

    const AzPhysics::SystemConfiguration* PhysXSystem::GetConfiguration() const
    {
        return &m_systemConfig;
    }

    void PhysXSystem::UpdateConfiguration(const AzPhysics::SystemConfiguration* newConfig, [[maybe_unused]] bool forceReinitialization /*= false*/)
    {
        if (const auto* physXConfig = azdynamic_cast<const PhysXSystemConfiguration*>(newConfig);
            m_systemConfig != (*physXConfig))
        {
            m_systemConfig = (*physXConfig);
            m_configChangeEvent.Signal(physXConfig);

            //LYN-1146 -- Restarting the simulation if required
        }
    }

    void PhysXSystem::InitializePhysXSdk(const physx::PxCookingParams& cookingParams)
    {
        m_physXSdk.m_foundation = PxCreateFoundation(PX_PHYSICS_VERSION, m_physXAllocatorCallback, m_physXErrorCallback);

        physx::PxPvd* pvd = m_physXDebug.InitializePhysXPvd(m_physXSdk.m_foundation);

        // create PhysX basis
        bool physXTrackOutstandingAllocations = false;
#ifdef AZ_PHYSICS_DEBUG_ENABLED
        physXTrackOutstandingAllocations = true;
#endif
        m_physXSdk.m_physics = PxCreatePhysics(PX_PHYSICS_VERSION, *m_physXSdk.m_foundation, physx::PxTolerancesScale(), physXTrackOutstandingAllocations, pvd);
        PxInitExtensions(*m_physXSdk.m_physics, pvd);

        // set up cooking for height fields, meshes etc.
        m_physXSdk.m_cooking = PxCreateCooking(PX_PHYSICS_VERSION, *m_physXSdk.m_foundation, cookingParams);

        // Set up CPU dispatcher
#if defined(AZ_PLATFORM_LINUX)
        // Temporary workaround for linux. At the moment using AzPhysXCpuDispatcher results in an assert at
        // PhysX mutex indicating it must be unlocked only by the thread that has already acquired lock.
        m_cpuDispatcher = physx::PxDefaultCpuDispatcherCreate(0);
#else
        m_cpuDispatcher = PhysXCpuDispatcherCreate();
#endif

        PxSetProfilerCallback(&m_pxAzProfilerCallback);
    }

    void PhysXSystem::ShutdownPhysXSdk()
    {
        delete m_cpuDispatcher;
        m_cpuDispatcher = nullptr;

        m_physXSdk.m_cooking->release();
        m_physXSdk.m_cooking = nullptr;

        PxCloseExtensions();

        m_physXSdk.m_physics->release();
        m_physXSdk.m_physics = nullptr;

        m_physXDebug.ShutdownPhysXPvd();

        m_physXSdk.m_foundation->release();
        m_physXSdk.m_foundation = nullptr;
    }

    const PhysXSystemConfiguration& PhysXSystem::GetPhysXConfiguration() const
    {
        return m_systemConfig;
    }

    void PhysXSystem::UpdateDefaultSceneConfiguration(const AzPhysics::SceneConfiguration& sceneConfiguration)
    {
        if (m_defaultSceneConfiguration != sceneConfiguration)
        {
            m_defaultSceneConfiguration = sceneConfiguration;

            m_onDefaultSceneConfigurationChangedEvent.Signal(&m_defaultSceneConfiguration);
        }
    }

    const AzPhysics::SceneConfiguration& PhysXSystem::GetDefaultSceneConfiguration() const
    {
        return m_defaultSceneConfiguration;
    }

    const PhysXSettingsRegistryManager& PhysXSystem::GetSettingsRegistryManager() const
    {
        return *m_registryManager;
    }

    //TEMP -- until these are fully moved over here
    void PhysXSystem::SetCollisionLayerName(int index, const AZStd::string& layerName)
    {
        m_systemConfig.m_collisionConfig.m_collisionLayers.SetName(aznumeric_cast<AZ::u64>(index), layerName);
    }

    void PhysXSystem::CreateCollisionGroup(const AZStd::string& groupName, const AzPhysics::CollisionGroup& group)
    {
        m_systemConfig.m_collisionConfig.m_collisionGroups.CreateGroup(groupName, group);
    }

    AZ::Debug::PerformanceCollector* PhysXSystem::GetPerformanceCollector()
    {
        return m_performanceCollector.get();
    }

    PhysXSystem* GetPhysXSystem()
    {
        return azdynamic_cast<PhysXSystem*>(AZ::Interface<AzPhysics::SystemInterface>::Get());
    }
} //namespace PhysX
