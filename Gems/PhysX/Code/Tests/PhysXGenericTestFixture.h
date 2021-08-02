/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzTest/AzTest.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/containers/variant.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/Common/PhysicsTypes.h>
#include <AzFramework/Physics/Collision/CollisionLayers.h>

namespace AzPhysics
{
    struct RigidBody;
    struct StaticRigidBody;
}

namespace PhysX
{
    class GenericPhysicsFixture
        : protected Physics::DefaultWorldBus::Handler
    {
    public:
        // Helper functions for setting up test scenes using API only
        // These can be implemented here as they should not require any gem specific functions
        AzPhysics::SceneHandle CreateTestScene();
        void DestroyTestScene();

        void SetUpInternal();
        void TearDownInternal();

        // Helper function for creating multishape entity
        struct MultiShapeConfig
        {
            AZ::Vector3 m_position; // Position of entity
            AZ::Vector3 m_rotation = AZ::Vector3(0.f, 0.f, 0.f); // Euler rotation of entity in radians
            AzPhysics::CollisionLayer m_layer = AzPhysics::CollisionLayer::Default; // Collision layer

            struct ShapeList
            {
                struct ShapeData
                {
                    struct Box
                    {
                        AZ::Vector3 m_extent;
                    };
                    struct Sphere
                    {
                        float m_radius;
                    };
                    struct Capsule
                    {
                        float m_height;
                        float m_radius;
                    };

                    AZ::Vector3 m_offset;
                    AZStd::variant<AZStd::monostate, Box, Sphere, Capsule> m_data;
                };

                void AddBox(AZ::Vector3 extent, AZ::Vector3 offset)
                {
                    ShapeData box;
                    ShapeData::Box boxData{ extent };
                    box.m_data = boxData;
                    box.m_offset = offset;
                    m_shapesData.push_back(box);
                }
                void AddSphere(float radius, AZ::Vector3 offset)
                {
                    ShapeData sphere;
                    ShapeData::Sphere sphereData{ radius };
                    sphere.m_data = sphereData;
                    sphere.m_offset = offset;
                    m_shapesData.push_back(sphere);
                }
                void AddCapsule(float height, float radius, AZ::Vector3 offset)
                {
                    ShapeData capsule;
                    ShapeData::Capsule capsuleData{ height, radius };
                    capsule.m_data = capsuleData;
                    capsule.m_offset = offset;
                    m_shapesData.push_back(capsule);
                }

                AZStd::vector<ShapeData> m_shapesData;
            };
            ShapeList m_shapes;
        };
        AZStd::unique_ptr<AZ::Entity> AddMultiShapeEntity(const MultiShapeConfig& config);

    protected:
        // DefaultWorldBus
        AzPhysics::SceneHandle GetDefaultSceneHandle() const override;

        AzPhysics::Scene* m_defaultScene = nullptr;
        AzPhysics::SceneHandle m_testSceneHandle = AzPhysics::InvalidSceneHandle;
    };

    class GenericPhysicsInterfaceTest
        : public GenericPhysicsFixture
        , public testing::Test
    {
    public:
        void SetUp() override
        {
            SetUpInternal();
        }
        void TearDown() override
        {
            TearDownInternal();

            DestroyTestScene(); //cleanup any physics scene if created.
        }
    };
} // namespace Physics
