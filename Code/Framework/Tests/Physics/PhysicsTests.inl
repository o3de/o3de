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

#include "PhysicsTests.h"
#include <AzFramework/Physics/World.h>
#include <AzFramework/Physics/RigidBody.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzCore/Component/TransformBus.h>

namespace Physics
{
    // helper functions
    AZStd::shared_ptr<World> GenericPhysicsFixture::CreateTestWorld()
    {
        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            AzPhysics::SceneConfiguration sceneConfiguration = physicsSystem->GetDefaultSceneConfiguration();
            sceneConfiguration.m_legacyId = AZ_CRC_CE("testWorld");
            sceneConfiguration.m_legacyConfiguration.m_gravity = AZ::Vector3(0.0f, 0.0f, -10.0f);
            m_testSceneHandle = physicsSystem->AddScene(sceneConfiguration);
            m_defaultScene = physicsSystem->GetScene(m_testSceneHandle);
            return m_defaultScene->GetLegacyWorld();
        }
        return nullptr;
    }

    void GenericPhysicsFixture::DestroyTestScene()
    {
        //clean up the test scene
        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            physicsSystem->RemoveScene(m_testSceneHandle);
        }
    }

    AZStd::shared_ptr<RigidBodyStatic> AddStaticFloorToWorld(World* world, const AZ::Transform& transform)
    {
        WorldBodyConfiguration rigidBodySettings;
        AZStd::shared_ptr<RigidBodyStatic> floor;
        SystemRequestBus::BroadcastResult(floor, &SystemRequests::CreateStaticRigidBody, rigidBodySettings);

        Physics::ColliderConfiguration colliderConfig;
        Physics::BoxShapeConfiguration shapeConfiguration(AZ::Vector3(20.0f, 20.0f, 1.0f));
        AZStd::shared_ptr<Shape> shape;
        SystemRequestBus::BroadcastResult(shape, &SystemRequests::CreateShape, colliderConfig, shapeConfiguration);
        floor->AddShape(shape);

        world->AddBody(*floor);
        floor->SetTransform(transform);
        return floor;
    }

    AZStd::shared_ptr<RigidBodyStatic> AddStaticUnitBoxToWorld(World* world, const AZ::Vector3& position)
    {
        WorldBodyConfiguration rigidBodySettings;
        rigidBodySettings.m_position = position;
        AZStd::shared_ptr<RigidBodyStatic> box;
        SystemRequestBus::BroadcastResult(box, &SystemRequests::CreateStaticRigidBody, rigidBodySettings);

        Physics::ColliderConfiguration colliderConfig;
        Physics::BoxShapeConfiguration shapeConfiguration;
        AZStd::shared_ptr<Shape> shape;
        SystemRequestBus::BroadcastResult(shape, &SystemRequests::CreateShape, colliderConfig, shapeConfiguration);
        box->AddShape(shape);

        world->AddBody(*box);
        return box;
    }

    AZStd::shared_ptr<RigidBody> AddUnitBoxToWorld(World* world, const AZ::Vector3& position)
    {
        RigidBodyConfiguration rigidBodySettings;
        rigidBodySettings.m_linearDamping = 0.0f;
        AZStd::shared_ptr<RigidBody> box;
        SystemRequestBus::BroadcastResult(box, &SystemRequests::CreateRigidBody, rigidBodySettings);

        Physics::ColliderConfiguration colliderConfig;
        Physics::BoxShapeConfiguration shapeConfiguration;
        AZStd::shared_ptr<Shape> shape;
        SystemRequestBus::BroadcastResult(shape, &SystemRequests::CreateShape, colliderConfig, shapeConfiguration);
        box->AddShape(shape);

        world->AddBody(*box.get());
        box->SetTransform(AZ::Transform::CreateTranslation(position));
        return box;
    }

    AZStd::shared_ptr<RigidBody> AddSphereToWorld(World* world, const AZ::Vector3& position)
    {
        RigidBodyConfiguration rigidBodySettings;
        rigidBodySettings.m_linearDamping = 0.0f;
        AZStd::shared_ptr<RigidBody> sphere;
        SystemRequestBus::BroadcastResult(sphere, &SystemRequests::CreateRigidBody, rigidBodySettings);

        Physics::ColliderConfiguration colliderConfig;
        Physics::SphereShapeConfiguration shapeConfiguration;
        AZStd::shared_ptr<Shape> shape;
        SystemRequestBus::BroadcastResult(shape, &SystemRequests::CreateShape, colliderConfig, shapeConfiguration);
        sphere->AddShape(shape);

        world->AddBody(*sphere.get());
        sphere->SetTransform(AZ::Transform::CreateTranslation(position));
        return sphere;
    }

    AZStd::shared_ptr<RigidBody> AddCapsuleToWorld(World* world, const AZ::Vector3& position)
    {
        RigidBodyConfiguration rigidBodySettings;
        AZStd::shared_ptr<RigidBody> capsule = nullptr;
        SystemRequestBus::BroadcastResult(capsule, &SystemRequests::CreateRigidBody, rigidBodySettings);

        Physics::ColliderConfiguration colliderConfig;
        colliderConfig.m_rotation = AZ::Quaternion::CreateRotationX(AZ::Constants::HalfPi);
        Physics::CapsuleShapeConfiguration shapeConfig(2.0f, 0.5f);
        AZStd::shared_ptr<Shape> shape;
        SystemRequestBus::BroadcastResult(shape, &SystemRequests::CreateShape, colliderConfig, shapeConfig);
        capsule->AddShape(shape);

        world->AddBody(*capsule.get());
        capsule->SetTransform(AZ::Transform::CreateTranslation(position));
        return capsule;
    }

    void UpdateScene(AzPhysics::Scene* scene, float timeStep, AZ::u32 numSteps)
    {
        for (AZ::u32 i = 0; i < numSteps; i++)
        {
            scene->StartSimulation(timeStep);
            scene->FinishSimulation();
        }
    }

    float GetPositionElement(AZ::Entity* entity, int element)
    {
        AZ::Transform transform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(transform, entity->GetId(), &AZ::TransformInterface::GetWorldTM);
        return transform.GetTranslation().GetElement(element);
    }
} // namespace Physics
