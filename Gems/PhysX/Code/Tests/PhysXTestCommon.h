/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

#include <AzFramework/Physics/Material.h>

#include <PhysX/HeightFieldAsset.h>
#include <BoxColliderComponent.h>

#include <Configuration/PhysXSettingsRegistryManager.h>

namespace AzPhysics
{
    class Scene;
}

namespace Physics
{
    class RigidBodyStatic;
}

namespace PhysX
{
    using EntityPtr = AZStd::shared_ptr<AZ::Entity>;
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

        // Create a flat "Terrain" for testing
        EntityPtr CreateFlatTestTerrain(float width = 1.0f, float depth = 1.0f);

        // Create spheres
        EntityPtr CreateSphereEntity(const AZ::Vector3& position, const float radius, const AZStd::shared_ptr<Physics::ColliderConfiguration>& colliderConfig);

        // Create dynamic boxes
        EntityPtr CreateBoxEntity(const AZ::Vector3& position, const AZ::Vector3& dimensions, bool isTrigger = false);
        EntityPtr CreateBoxEntity(const AZ::Vector3& position, const AZ::Vector3& dimensions,
            const AZStd::shared_ptr<Physics::ColliderConfiguration>& colliderConfig);

        // Create static boxes
        EntityPtr CreateStaticBoxEntity(const AZ::Vector3& position, const AZ::Vector3& dimensions);
        AZStd::unique_ptr<Physics::RigidBodyStatic> CreateStaticTriangleMeshCube(float halfExtent);

        // Collision Filtering
        void SetCollisionLayer(EntityPtr& entity, const AZStd::string& layerName, const AZStd::string& colliderTag = "");
        void SetCollisionGroup(EntityPtr& entity, const AZStd::string& groupName, const AZStd::string& colliderTag = "");
        void ToggleCollisionLayer(EntityPtr& entity, const AZStd::string& layerName, bool enabled, const AZStd::string& colliderTag = "");

        // Generic creation functions
        template<typename ColliderType = BoxColliderComponent>
        EntityPtr AddUnitTestObject(const AZ::Vector3& position, const char* name = "TestObjectEntity");

        template<typename ColliderType = BoxColliderComponent>
        EntityPtr AddStaticUnitTestObject(const AZ::Vector3& position, const char* name = "TestObjectEntity");

        template<typename ColliderT>
        EntityPtr CreateTriggerAtPosition(const AZ::Vector3& position);

        template<typename ColliderT>
        EntityPtr CreateDynamicTriggerAtPosition(const AZ::Vector3& position);

        // Misc
        EntityPtr AddUnitTestBoxComponentsMix(const AZ::Vector3& position, const char* name = "TestBoxEntity");

        // Mesh data generation
        PointList GeneratePyramidPoints(float length);
        VertexIndexData GenerateCubeMeshData(float halfExtent);

    } // namespace TestUtils
} // namespace PhysX

#include "PhysXTestCommon.inl"
