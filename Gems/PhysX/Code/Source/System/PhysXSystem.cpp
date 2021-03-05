/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include <PhysX_precompiled.h>

#include <AzCore/Math/MathUtils.h>

#include <Scene/PhysXScene.h>
#include <System/PhysXSystem.h>
#include <System/PhysXAllocator.h>
#include <System/PhysXCpuDispatcher.h>

#include <PxPhysicsAPI.h>
namespace PhysX
{
    PhysXSystem::MaterialLibraryAssetHelper::MaterialLibraryAssetHelper(PhysXSystem* physXSystem)
        : m_physXSystem(physXSystem)
    {

    }

    void PhysXSystem::MaterialLibraryAssetHelper::Connect(const AZ::Data::AssetId& materialLibraryId)
    {
        if (!AZ::Data::AssetBus::Handler::BusIsConnectedId(materialLibraryId))
        {
            AZ::Data::AssetBus::Handler::BusDisconnect();
            AZ::Data::AssetBus::Handler::BusConnect(materialLibraryId);
        }
    }

    void PhysXSystem::MaterialLibraryAssetHelper::Disconnect()
    {
        AZ::Data::AssetBus::Handler::BusDisconnect();
    }

    void PhysXSystem::MaterialLibraryAssetHelper::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (m_physXSystem == nullptr || m_physXSystem->GetDefaultMaterialLibrary() != asset)
        {
            return;
        }
        m_physXSystem->UpdateDefaultMaterialLibrary(asset);
    }
    
    PhysXSystem::PhysXSystem(PhysXSettingsRegistryManager* registryManager, const physx::PxCookingParams& cookingParams)
        : m_registryManager(*registryManager)
        , m_materialLibraryAssetHelper(this)
    {
        // Start PhysX allocator
        PhysXAllocator::Descriptor allocatorDescriptor;
        allocatorDescriptor.m_custom = &AZ::AllocatorInstance<AZ::SystemAllocator>::Get();
        AZ::AllocatorInstance<PhysXAllocator>::Create();

        InitializePhysXSdk(cookingParams);
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

        AzFramework::AssetCatalogEventBus::Handler::BusConnect();

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

        AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
        m_materialLibraryAssetHelper.Disconnect();
        // Clear the asset reference in deactivate. The asset system is shut down before destructors are called
        // for system components, causing any hanging asset references to become crashes on shutdown in release builds.
        m_systemConfig.m_defaultMaterialLibrary.Reset();

        m_accumulatedTime = 0.0f;
        m_state = State::Shutdown;
    }

    void PhysXSystem::Simulate(float deltaTime)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Physics);

        if (m_state != State::Initialized)
        {
            AZ_Warning("PhysXSystem", false, "Call Simulate when PhysX system is not initialized");
            return;
        }

        auto simulateScenes = [this](float timeStep)
        {
            for (auto* scene : m_sceneList)
            {
                if (scene != nullptr && scene->IsEnabled())
                {
                    scene->StartSimulation(timeStep);
                    scene->FinishSimulation();
                }
            }
        };

        deltaTime = AZ::GetClamp(deltaTime, 0.0f, m_systemConfig.m_maxTimestep);

        AZ_Assert(m_systemConfig.m_fixedTimestep >= 0.0f, "PhysXSystem - fixed timestep is negitive.");
        if (m_systemConfig.m_fixedTimestep > 0.0f) //use the fixed timestep
        {
            m_accumulatedTime += deltaTime;
            //divide accumulated time by the fixed step and floor it to get the number of steps that would occur. Then multiply by fixedTimeStep to get the total executed time.
            const float tickTime = AZStd::floorf(m_accumulatedTime / m_systemConfig.m_fixedTimestep) * m_systemConfig.m_fixedTimestep;
            m_preSimulateEvent.Signal(tickTime);
            while (m_accumulatedTime >= m_systemConfig.m_fixedTimestep)
            {
                simulateScenes(m_systemConfig.m_fixedTimestep);
                m_accumulatedTime -= m_systemConfig.m_fixedTimestep;
            }
        }
        else
        {
            m_preSimulateEvent.Signal(deltaTime);
            simulateScenes(deltaTime);
        }
        m_postSimulateEvent.Signal();
    }

    AzPhysics::SceneHandle PhysXSystem::AddScene(const AzPhysics::SceneConfiguration& config)
    {
        if (!m_freeSceneSlots.empty()) //fill any free slots first before increasing the size of the scene list vector.
        {
            AZ::u64 freeIndex = m_freeSceneSlots.front();
            m_freeSceneSlots.pop();
            AZ_Assert(freeIndex < m_sceneList.size(), "PhysXSystem::AddScene: Free scene index is out of bounds!");
            AZ_Assert(m_sceneList[freeIndex] == nullptr, "PhysXSystem::AddScene: Free scene index is not free");

            m_sceneList[freeIndex] = new PhysXScene(config);
            return AzPhysics::SceneHandle(config.m_legacyId, freeIndex);
        }

        if (m_sceneList.size() < std::numeric_limits<AzPhysics::SceneIndex>::max()) //add a new scene if it is under the limit
        {
            m_sceneList.emplace_back(new PhysXScene(config));
            return AzPhysics::SceneHandle(config.m_legacyId, (m_sceneList.size() - 1));
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

    AzPhysics::Scene* PhysXSystem::GetScene(AzPhysics::SceneHandle handle)
    {
        if (handle == AzPhysics::InvalidSceneHandle)
        {
            return nullptr;
        }

        AzPhysics::SceneIndex index = AZStd::get<AzPhysics::SceneHandleValues::Index>(handle);
        if (index < m_sceneList.size())
        {
            if (AzPhysics::Scene* scene = m_sceneList[index])
            {
                if (scene->GetConfiguration().m_legacyId == AZStd::get<AzPhysics::SceneHandleValues::Crc>(handle))
                {
                    return scene;
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

        AZ::u64 index = AZStd::get<AzPhysics::SceneHandleValues::Index>(handle);
        if (index < m_sceneList.size())
        {
            delete m_sceneList[index];
            m_sceneList[index] = nullptr;
            m_freeSceneSlots.push(index);
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
        for (auto* scene : m_sceneList)
        {
            delete scene;
            scene = nullptr;
        }
        m_sceneList.clear();

        //clear the free slots queue
        AZStd::queue<AzPhysics::SceneIndex> empty;
        m_freeSceneSlots.swap(empty);
    }

    const AzPhysics::SystemConfiguration* PhysXSystem::GetConfiguration() const
    {
        return &m_systemConfig;
    }

    void PhysXSystem::OnCatalogLoaded([[maybe_unused]]const char* catalogFile)
    {
        //now that assets can be resolved, lets load the default material library.
        LoadDefaultMaterialLibrary();
    }

    void PhysXSystem::UpdateConfiguration(const AzPhysics::SystemConfiguration* newConfig, [[maybe_unused]] bool forceReinitialization /*= false*/)
    {
        if (const auto* physXConfig = azdynamic_cast<const PhysXSystemConfiguration*>(newConfig);
            m_systemConfig != (*physXConfig))
        {
            const bool newMaterialLibrary = m_systemConfig.m_defaultMaterialLibrary != physXConfig->m_defaultMaterialLibrary;
            m_systemConfig = (*physXConfig);
            m_configChangeEvent.Signal(physXConfig);

            //LYN-1146 -- Restarting the simulation if required

            if (newMaterialLibrary)
            {
                LoadDefaultMaterialLibrary();
                m_onDefaultMaterialLibraryChangedEvent.Signal(m_systemConfig.m_defaultMaterialLibrary.GetId());
            }
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

    void PhysXSystem::UpdateDefaultMaterialLibrary(const AZ::Data::Asset<Physics::MaterialLibraryAsset>& materialLibrary)
    {
        if (m_systemConfig.m_defaultMaterialLibrary == materialLibrary)
        {
            return;
        }
        m_systemConfig.m_defaultMaterialLibrary = materialLibrary;

        LoadDefaultMaterialLibrary();
        m_onDefaultMaterialLibraryChangedEvent.Signal(materialLibrary.GetId());
    }

    const AZ::Data::Asset<Physics::MaterialLibraryAsset>& PhysXSystem::GetDefaultMaterialLibrary() const
    {
        return m_systemConfig.m_defaultMaterialLibrary;
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
        return m_registryManager;
    }

    bool PhysXSystem::LoadDefaultMaterialLibrary()
    {
        AZ::Data::Asset<Physics::MaterialLibraryAsset>& materialLibrary = m_systemConfig.m_defaultMaterialLibrary;
        const AZ::Data::AssetId& materialLibraryId = materialLibrary.GetId();
        if (!materialLibraryId.IsValid())
        {
            AZ_Warning("PhysX", false,
                "LoadDefaultMaterialLibrary: Default Material Library asset ID is invalid.");
            return false;
        }
        // Listen for material library asset modification events
        m_materialLibraryAssetHelper.Connect(materialLibraryId);

        const AZ::Data::AssetFilterCB assetLoadFilterCB = nullptr;
        materialLibrary = AZ::Data::AssetManager::Instance().GetAsset<Physics::MaterialLibraryAsset>(materialLibrary.GetId(), materialLibrary.GetAutoLoadBehavior(), AZ::Data::AssetLoadParameters{ assetLoadFilterCB });

        materialLibrary.BlockUntilLoadComplete();

        AZ_Warning("PhysX", (materialLibrary.GetData() != nullptr),
            "LoadDefaultMaterialLibrary: Default Material Library asset data is invalid.");
        
        return materialLibrary.GetData() != nullptr;
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

    PhysXSystem* GetPhysXSystem()
    {
        return azdynamic_cast<PhysXSystem*>(AZ::Interface<AzPhysics::SystemInterface>::Get());
    }
} //namespace PhysX
