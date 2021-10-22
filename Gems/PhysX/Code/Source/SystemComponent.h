/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <PxPhysicsAPI.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Interface/Interface.h>
#include <AzFramework/Physics/Character.h>
#include <AzFramework/Physics/SimulatedBodies/RigidBody.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/Material.h>
#include <AzFramework/Physics/CollisionBus.h>
#include <AzFramework/Physics/Configuration/CollisionConfiguration.h>
#include <AzFramework/Physics/Collision/CollisionGroups.h>
#include <AzFramework/Physics/Collision/CollisionLayers.h>
#include <AzFramework/Physics/Common/PhysicsEvents.h>

#include <PhysX/SystemComponentBus.h>
#include <PhysX/Configuration/PhysXConfiguration.h>
#include <Configuration/PhysXSettingsRegistryManager.h>
#include <DefaultWorldComponent.h>
#include <Material.h>

namespace AzPhysics
{
    struct StaticRigidBodyConfiguration;
    struct RigidBodyConfiguration;
    struct SimulatedBody;
}

namespace PhysX
{
    class WindProvider;
    class PhysXSystem;

    /// System component for PhysX.
    /// The system component handles underlying tasks such as initialization and shutdown of PhysX, managing a
    /// Open 3D Engine memory allocator for PhysX allocations, scheduling for PhysX jobs, and connections to the PhysX
    /// Visual Debugger.  It also owns fundamental PhysX objects which manage worlds, rigid bodies, shapes, materials,
    /// constraints etc., and perform cooking (processing assets such as meshes and heightfields ready for use in PhysX).
    class SystemComponent
        : public AZ::Component
        , public Physics::SystemRequestBus::Handler
        , public PhysX::SystemRequestsBus::Handler
        , private Physics::CollisionRequestBus::Handler
        , private AZ::TickBus::Handler
    {
    public:
        AZ_COMPONENT(SystemComponent, "{85F90819-4D9A-4A77-AB89-68035201F34B}");

        SystemComponent();
        ~SystemComponent();

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        SystemComponent(const SystemComponent&) = delete;

        // SystemRequestsBus
        physx::PxConvexMesh* CreateConvexMesh(const void* vertices, AZ::u32 vertexNum, AZ::u32 vertexStride) override; // should we use AZ::Vector3* or physx::PxVec3 here?
        physx::PxConvexMesh* CreateConvexMeshFromCooked(const void* cookedMeshData, AZ::u32 bufferSize) override;
        physx::PxTriangleMesh* CreateTriangleMeshFromCooked(const void* cookedMeshData, AZ::u32 bufferSize) override;
        physx::PxHeightField* CreateHeightField(const physx::PxHeightFieldSample* samples, AZ::u32 numRows, AZ::u32 numColumns) override;


        bool CookConvexMeshToFile(const AZStd::string& filePath, const AZ::Vector3* vertices, AZ::u32 vertexCount) override;
        
        bool CookConvexMeshToMemory(const AZ::Vector3* vertices, AZ::u32 vertexCount, AZStd::vector<AZ::u8>& result) override;

        bool CookTriangleMeshToFile(const AZStd::string& filePath, const AZ::Vector3* vertices, AZ::u32 vertexCount,
            const AZ::u32* indices, AZ::u32 indexCount) override;

        bool CookTriangleMeshToMemory(const AZ::Vector3* vertices, AZ::u32 vertexCount,
            const AZ::u32* indices, AZ::u32 indexCount, AZStd::vector<AZ::u8>& result) override;

        physx::PxFilterData CreateFilterData(const AzPhysics::CollisionLayer& layer, const AzPhysics::CollisionGroup& group) override;
        physx::PxCooking* GetCooking() override;

        // CollisionRequestBus
        AzPhysics::CollisionLayer GetCollisionLayerByName(const AZStd::string& layerName) override;
        AZStd::string GetCollisionLayerName(const AzPhysics::CollisionLayer& layer) override;
        bool TryGetCollisionLayerByName(const AZStd::string& layerName, AzPhysics::CollisionLayer& layer) override;
        AzPhysics::CollisionGroup GetCollisionGroupByName(const AZStd::string& groupName) override;
        bool TryGetCollisionGroupByName(const AZStd::string& layerName, AzPhysics::CollisionGroup& group) override;
        AZStd::string GetCollisionGroupName(const AzPhysics::CollisionGroup& collisionGroup) override;
        AzPhysics::CollisionGroup GetCollisionGroupById(const AzPhysics::CollisionGroups::Id& groupId) override;
        void SetCollisionLayerName(int index, const AZStd::string& layerName) override;
        void CreateCollisionGroup(const AZStd::string& groupName, const AzPhysics::CollisionGroup& group) override;

        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        // Physics::SystemRequestBus::Handler
        AZStd::shared_ptr<Physics::Shape> CreateShape(const Physics::ColliderConfiguration& colliderConfiguration, const Physics::ShapeConfiguration& configuration) override;
        AZStd::shared_ptr<Physics::Material> CreateMaterial(const Physics::MaterialConfiguration& materialConfiguration) override;

        void ReleaseNativeMeshObject(void* nativeMeshObject) override;
        void ReleaseNativeHeightfieldObject(void* nativeHeightfieldObject) override;

        // Assets related data
        AZStd::vector<AZStd::unique_ptr<AZ::Data::AssetHandler>> m_assetHandlers;
        PhysX::MaterialsManager m_materialManager;

        static bool VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);

    private:
        // AZ::TickBus::Handler ...
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        int GetTickOrder() override;

        void EnableAutoManagedPhysicsTick(bool shouldTick);

        void ActivatePhysXSystem();

        bool m_enabled; ///< If false, this component will not activate itself in the Activate() function.

        AZStd::unique_ptr<WindProvider> m_windProvider;
        DefaultWorldComponent m_defaultWorldComponent;
        AZ::Interface<Physics::CollisionRequests> m_collisionRequests;
        AZ::Interface<Physics::System> m_physicsSystem;

        PhysXSystem* m_physXSystem = nullptr;
        bool m_isTickingPhysics = false;
        AzPhysics::SystemEvents::OnInitializedEvent::Handler m_onSystemInitializedHandler;
        AzPhysics::SystemEvents::OnConfigurationChangedEvent::Handler m_onSystemConfigChangedHandler;
    };
} // namespace PhysX
