/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <AzFramework/Physics/Common/PhysicsTypes.h>

#include <PhysX/HeightFieldAsset.h>
#include <BoxColliderComponent.h>

#include <Configuration/PhysXSettingsRegistryManager.h>

namespace AzPhysics
{
    struct RigidBody;
    class Scene;
    struct StaticRigidBody;
}

namespace PhysX
{
    using EntityPtr = AZStd::shared_ptr<AZ::Entity>;
    using EntityList = AZStd::vector<EntityPtr>;
    using PointList = AZStd::vector<AZ::Vector3>;
    using VertexIndexData = AZStd::pair<PointList, AZStd::vector<AZ::u32>>;

    namespace TestUtils
    {
        //Don't load the registry files from disk, just return defaults.
        //save functions not overloaded as they don't do any saving
        class Test_PhysXSettingsRegistryManager : public PhysXSettingsRegistryManager
        {
        public:
            AZStd::optional<PhysXSystemConfiguration> LoadSystemConfiguration() const override;
            AZStd::optional<AzPhysics::SceneConfiguration> LoadDefaultSceneConfiguration() const override;
            AZStd::optional<Debug::DebugConfiguration> LoadDebugConfiguration() const override;
        };

        void ResetPhysXSystem();

        // Updates the default world
        void UpdateScene(AzPhysics::Scene* scene, float timeStep, AZ::u32 numSteps);
        void UpdateScene(AzPhysics::SceneHandle sceneHandle, float timeStep, AZ::u32 numSteps);

        // Create a flat "Terrain" for testing
        EntityPtr CreateFlatTestTerrain(AzPhysics::SceneHandle sceneHandle, float width = 1.0f, float depth = 1.0f);

        // Create spheres
        EntityPtr CreateSphereEntity(AzPhysics::SceneHandle sceneHandle, const AZ::Vector3& position,
            const float radius, const AzPhysics::CollisionLayer& layer = AzPhysics::CollisionLayer::Default);
        EntityPtr CreateSphereEntity(AzPhysics::SceneHandle sceneHandle, const AZ::Vector3& position,
            const float radius, const AZStd::shared_ptr<Physics::ColliderConfiguration>& colliderConfig);

        // Create static spheres
        EntityPtr CreateStaticSphereEntity(AzPhysics::SceneHandle sceneHandle,
            const AZ::Vector3& position,
            const float radius,
            const AzPhysics::CollisionLayer& layer = AzPhysics::CollisionLayer::Default);

        // Create dynamic boxes
        EntityPtr CreateBoxEntity(AzPhysics::SceneHandle sceneHandle, const AZ::Vector3& position, const AZ::Vector3& dimensions,
            const AzPhysics::CollisionLayer& layer = AzPhysics::CollisionLayer::Default, bool isTrigger = false);
        EntityPtr CreateBoxEntity(AzPhysics::SceneHandle sceneHandle, const AZ::Vector3& position, const AZ::Vector3& dimensions,
            const AZStd::shared_ptr<Physics::ColliderConfiguration>& colliderConfig);

        // Create static boxes
        EntityPtr CreateStaticBoxEntity(AzPhysics::SceneHandle sceneHandle, const AZ::Vector3& position,
            const AZ::Vector3& dimensions = AZ::Vector3(1.0f),
            const AzPhysics::CollisionLayer& layer = AzPhysics::CollisionLayer::Default);

        // Create Capsules
        EntityPtr CreateCapsuleEntity(AzPhysics::SceneHandle sceneHandle, const AZ::Vector3& position,
            const float height, const float radius,
            const AzPhysics::CollisionLayer& layer = AzPhysics::CollisionLayer::Default);

        // Create static Capsules
        EntityPtr CreateStaticCapsuleEntity(AzPhysics::SceneHandle sceneHandle, const AZ::Vector3& position,
            const float height, const float radius,
            const AzPhysics::CollisionLayer& layer = AzPhysics::CollisionLayer::Default);

        AzPhysics::SimulatedBodyHandle AddStaticTriangleMeshCubeToScene(AzPhysics::SceneHandle scene, float halfExtent);
        AzPhysics::SimulatedBodyHandle AddKinematicTriangleMeshCubeToScene(AzPhysics::SceneHandle scene, float halfExtent, AzPhysics::MassComputeFlags massComputeFlags);

        // Collision Filtering
        void SetCollisionLayer(EntityPtr& entity, const AZStd::string& layerName, const AZStd::string& colliderTag = "");
        void SetCollisionGroup(EntityPtr& entity, const AZStd::string& groupName, const AZStd::string& colliderTag = "");
        void ToggleCollisionLayer(EntityPtr& entity, const AZStd::string& layerName, bool enabled, const AZStd::string& colliderTag = "");

        // Generic creation functions
        template<typename ColliderType = BoxColliderComponent>
        EntityPtr AddUnitTestObject(AzPhysics::SceneHandle sceneHandle, const AZ::Vector3& position, const char* name = "TestObjectEntity");

        template<typename ColliderType = BoxColliderComponent>
        EntityPtr AddStaticUnitTestObject(AzPhysics::SceneHandle sceneHandle, const AZ::Vector3& position, const char* name = "TestObjectEntity");

        template<typename ColliderT>
        EntityPtr CreateTriggerAtPosition(const AZ::Vector3& position);

        template<typename ColliderT>
        EntityPtr CreateDynamicTriggerAtPosition(const AZ::Vector3& position);

        // Misc
        EntityPtr AddUnitTestBoxComponentsMix(AzPhysics::SceneHandle sceneHandle, const AZ::Vector3& position, const char* name = "TestBoxEntity");

        // Mesh data generation
        PointList GeneratePyramidPoints(float length);
        AZStd::shared_ptr<Physics::Shape> CreatePyramidShape(
            float length, const Physics::ColliderConfiguration& colliderConfiguration = Physics::ColliderConfiguration());
        VertexIndexData GenerateCubeMeshData(float halfExtent);

        AzPhysics::StaticRigidBody* AddStaticFloorToScene(AzPhysics::SceneHandle sceneHandle,
            const AZ::Transform& transform = AZ::Transform::CreateIdentity());
        AzPhysics::StaticRigidBody* AddStaticUnitBoxToScene(AzPhysics::SceneHandle sceneHandle, const AZ::Vector3& position);
        AzPhysics::RigidBody* AddUnitBoxToScene(AzPhysics::SceneHandle sceneHandle, const AZ::Vector3& position);

        AzPhysics::SimulatedBodyHandle AddSphereToScene(AzPhysics::SceneHandle sceneHandle, const AZ::Vector3& position,
            const float radius = 0.5f, const AzPhysics::CollisionLayer& layer = AzPhysics::CollisionLayer::Default);
        AzPhysics::SimulatedBodyHandle AddCapsuleToScene(AzPhysics::SceneHandle sceneHandle, const AZ::Vector3& position,
            const float height = 2.0f, const float radius = 0.5f,
            const AzPhysics::CollisionLayer& layer = AzPhysics::CollisionLayer::Default);
        AzPhysics::SimulatedBodyHandle AddBoxToScene(AzPhysics::SceneHandle sceneHandle,
            const AZ::Vector3& position, const AZ::Vector3& dimensions = AZ::Vector3(1.0f),
            const AzPhysics::CollisionLayer& layer = AzPhysics::CollisionLayer::Default);
        AzPhysics::SimulatedBodyHandle AddStaticBoxToScene(AzPhysics::SceneHandle sceneHandle,
            const AZ::Vector3& position, const AZ::Vector3& dimensions = AZ::Vector3(1.0f),
            const AzPhysics::CollisionLayer& layer = AzPhysics::CollisionLayer::Default);

        float GetPositionElement(EntityPtr entity, int element);

    } // namespace TestUtils
} // namespace PhysX

#include "PhysXTestCommon.inl"
