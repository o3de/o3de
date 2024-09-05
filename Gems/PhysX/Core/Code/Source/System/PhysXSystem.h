/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/TickBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzFramework/Physics/PhysicsSystem.h>
#include <AzFramework/Physics/Configuration/SystemConfiguration.h>

#include <Configuration/PhysXSettingsRegistryManager.h>
#include <Debug/PhysXDebug.h>
#include <Scene/PhysXSceneInterface.h>
#include <System/PhysXAllocator.h>
#include <System/PhysXSdkCallbacks.h>

#include <PhysX/Configuration/PhysXConfiguration.h>
#include <System/PhysXJointInterface.h>

namespace AZ::Debug
{
    class PerformanceCollector;
}

namespace physx
{
    class PxFoundation;
    class PxPhysics;
    class PxCooking;
    class PxCpuDispatcher;
}

namespace PhysX
{
    class PhysXSystem
        : public AZ::Interface<AzPhysics::SystemInterface>::Registrar
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL;
        AZ_RTTI(PhysXSystem, "{B6F4D92A-061B-4CB3-AAB5-984B599A53AE}", AzPhysics::SystemInterface);

        PhysXSystem(AZStd::unique_ptr<PhysXSettingsRegistryManager> registryManager, const physx::PxCookingParams& cookingParams);
        virtual ~PhysXSystem();

        // SystemInterface interface ...
        void Initialize(const AzPhysics::SystemConfiguration* config) override;
        void Reinitialize() override;
        void Shutdown() override;
        void Simulate(float deltaTime) override;
        AzPhysics::SceneHandle AddScene(const AzPhysics::SceneConfiguration& config) override;
        AzPhysics::SceneHandleList AddScenes(const AzPhysics::SceneConfigurationList& configs) override;
        AzPhysics::SceneHandle GetSceneHandle(const AZStd::string& sceneName) override;
        AzPhysics::Scene* GetScene(AzPhysics::SceneHandle handle) override;
        AzPhysics::SceneList GetScenes(const AzPhysics::SceneHandleList& handles) override;
        AzPhysics::SceneList& GetAllScenes() override;
        void RemoveScene(AzPhysics::SceneHandle handle) override;
        void RemoveScenes(const AzPhysics::SceneHandleList& handles) override;
        void RemoveAllScenes() override;
        AZStd::pair<AzPhysics::SceneHandle, AzPhysics::SimulatedBodyHandle> FindAttachedBodyHandleFromEntityId(AZ::EntityId entityId) override;
        const AzPhysics::SystemConfiguration* GetConfiguration() const override;
        void UpdateConfiguration(const AzPhysics::SystemConfiguration* newConfig, bool forceReinitialization = false) override;
        void UpdateDefaultSceneConfiguration(const AzPhysics::SceneConfiguration& sceneConfiguration) override;
        const AzPhysics::SceneConfiguration& GetDefaultSceneConfiguration() const override;

        //! Accessor to get the current PhysX configuration data.
        const PhysXSystemConfiguration& GetPhysXConfiguration() const;

        //! Accessor to get the Settings Registry Manager.
        const PhysXSettingsRegistryManager& GetSettingsRegistryManager() const;

        //TEMP -- until these are fully moved over here
        physx::PxPhysics* GetPxPhysics() { return m_physXSdk.m_physics; }
        physx::PxCooking* GetPxCooking() { return m_physXSdk.m_cooking; }
        physx::PxCpuDispatcher* GetPxCpuDispathcher()
        {
            AZ_Assert(m_cpuDispatcher, "PhysX CPU dispatcher was not created");
            return m_cpuDispatcher;
        }
        void SetCollisionLayerName(int index, const AZStd::string& layerName);
        void CreateCollisionGroup(const AZStd::string& groupName, const AzPhysics::CollisionGroup& group);
        //TEMP -- until these are fully moved over here

        AZ::Debug::PerformanceCollector* GetPerformanceCollector();

    private:
        //! Initializes the PhysX SDK.
        //! This sets up the PhysX Foundation, Cooking, and other PhysX sub-systems.
        //! @param cookingParams The cooking params to use when setting up PhysX cooking interface.
        void InitializePhysXSdk(const physx::PxCookingParams& cookingParams);
        void ShutdownPhysXSdk();

        void InitializePerformanceCollector();

        PhysXSystemConfiguration m_systemConfig;
        AzPhysics::SceneConfiguration m_defaultSceneConfiguration;
        AzPhysics::SceneList m_sceneList;
        AZStd::queue<AzPhysics::SceneIndex> m_freeSceneSlots; //when a scene is removed cache its index here to be used for the next add.

        float m_accumulatedTime = 0.0f;

        struct PhysXSdk
        {
            physx::PxFoundation* m_foundation = nullptr;
            physx::PxPhysics* m_physics = nullptr;
            physx::PxCooking* m_cooking = nullptr;
        };
        PhysXSdk m_physXSdk;
        PxAzAllocatorCallback m_physXAllocatorCallback;
        PxAzErrorCallback m_physXErrorCallback;
        PxAzProfilerCallback m_pxAzProfilerCallback;

        physx::PxCpuDispatcher* m_cpuDispatcher = nullptr;

        enum class State : AZ::u8
        {
            Uninitialized = 0,
            Initialized,
            Shutdown
        };
        State m_state = State::Uninitialized;

        Debug::PhysXDebug m_physXDebug; //! Handler for the PhysXDebug Interface.
        AZStd::unique_ptr<PhysXSettingsRegistryManager> m_registryManager; //! Handles all settings registry interactions.
        PhysXSceneInterface m_sceneInterface; //! Implemented the Scene Az::Interface.
        PhysXJointHelpersInterface m_jointHelperInterface; //! Implementation of the JointHelpersInterface.

        static constexpr AZStd::string_view PerformanceLogCategory = "PhysX";
        static constexpr AZStd::string_view PerformanceSpecPhysXSimulationTime = "PhysX Simulation Time";

        AZStd::unique_ptr<AZ::Debug::PerformanceCollector> m_performanceCollector;
    };

    //! Helper function for getting the PhysX System interface from inside the PhysX gem.
    PhysXSystem* GetPhysXSystem();
}
