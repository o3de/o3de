/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Physics/Configuration/RigidBodyConfiguration.h>

#include <RigidBodyComponent.h>
#include <StaticRigidBodyComponent.h>

namespace PhysX
{
    namespace TestUtils
    {
        template<typename ColliderType>
        EntityPtr AddUnitTestObject(AzPhysics::SceneHandle sceneHandle, const AZ::Vector3& position, const char* name)
        {
            EntityPtr entity = AZStd::make_shared<AZ::Entity>(name);

            AZ::TransformConfig transformConfig;
            transformConfig.m_worldTransform = AZ::Transform::CreateTranslation(position);
            entity->CreateComponent<AzFramework::TransformComponent>()->SetConfiguration(transformConfig);
            auto colliderComponent = entity->CreateComponent<ColliderType>();
            auto colliderConfig = AZStd::make_shared<Physics::ColliderConfiguration>();
            auto shapeConfig = AZStd::make_shared<typename ColliderType::Configuration>();
            colliderComponent->SetShapeConfigurationList({ AZStd::make_pair(colliderConfig, shapeConfig) });
            AzPhysics::RigidBodyConfiguration rigidBodyConfig;
            rigidBodyConfig.m_computeMass = false;
            entity->CreateComponent<RigidBodyComponent>(rigidBodyConfig, sceneHandle);
            entity->Init();
            entity->Activate();
            return entity;
        }

        template<typename ColliderType>
        EntityPtr AddStaticUnitTestObject(AzPhysics::SceneHandle sceneHandle, const AZ::Vector3& position, const char* name)
        {
            EntityPtr entity = AZStd::make_shared<AZ::Entity>(name);

            AZ::TransformConfig transformConfig;
            transformConfig.m_worldTransform = AZ::Transform::CreateTranslation(position);
            entity->CreateComponent<AzFramework::TransformComponent>()->SetConfiguration(transformConfig);
            auto colliderComponent = entity->CreateComponent<ColliderType>();
            auto colliderConfig = AZStd::make_shared<Physics::ColliderConfiguration>();
            auto shapeConfig = AZStd::make_shared<typename ColliderType::Configuration>();
            colliderComponent->SetShapeConfigurationList({ AZStd::make_pair(colliderConfig, shapeConfig) });

            entity->CreateComponent<StaticRigidBodyComponent>(sceneHandle);
            
            entity->Init();
            entity->Activate();
            return entity;
        }

        template<typename ColliderT>
        EntityPtr CreateTriggerAtPosition(const AZ::Vector3& position)
        {
            EntityPtr triggerEntity = AZStd::make_shared<AZ::Entity>("TriggerEntity");

            AZ::TransformConfig transformConfig;
            transformConfig.m_worldTransform = AZ::Transform::CreateTranslation(position);
            triggerEntity->CreateComponent<AzFramework::TransformComponent>()->SetConfiguration(transformConfig);

            auto colliderConfiguration = AZStd::make_shared<Physics::ColliderConfiguration>();
            colliderConfiguration->m_isTrigger = true;
            auto shapeConfiguration = AZStd::make_shared<typename ColliderT::Configuration>();
            auto colliderComponent = triggerEntity->CreateComponent<ColliderT>();
            colliderComponent->SetShapeConfigurationList({ AZStd::make_pair(colliderConfiguration, shapeConfiguration) });

            triggerEntity->CreateComponent<StaticRigidBodyComponent>();

            triggerEntity->Init();
            triggerEntity->Activate();
            
            return triggerEntity;
        }

        template<typename ColliderT>
        EntityPtr CreateDynamicTriggerAtPosition(const AZ::Vector3& position)
        {
            EntityPtr triggerEntity = AZStd::make_shared<AZ::Entity>("DynamicTriggerEntity");

            AZ::TransformConfig transformConfig;
            transformConfig.m_worldTransform = AZ::Transform::CreateTranslation(position);
            triggerEntity->CreateComponent<AzFramework::TransformComponent>()->SetConfiguration(transformConfig);

            auto colliderConfiguration = AZStd::make_shared<Physics::ColliderConfiguration>();
            colliderConfiguration->m_isTrigger = true;
            auto shapeConfiguration = AZStd::make_shared<typename ColliderT::Configuration>();
            auto colliderComponent = triggerEntity->CreateComponent<ColliderT>();
            colliderComponent->SetShapeConfigurationList({ AZStd::make_pair(colliderConfiguration, shapeConfiguration) });

            triggerEntity->CreateComponent<RigidBodyComponent>();

            triggerEntity->Init();
            triggerEntity->Activate();

            return triggerEntity;
        }
    }
}
