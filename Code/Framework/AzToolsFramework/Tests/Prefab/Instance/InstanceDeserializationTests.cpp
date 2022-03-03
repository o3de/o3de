/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <Prefab/PrefabTestFixture.h>

namespace UnitTest
{
    using InstanceDeserializationTest = PrefabTestFixture;
    using InstanceUniquePointer = AZStd::unique_ptr<AzToolsFramework::Prefab::Instance>;

    static void GenerateDomAndReloadInstantiatedPrefab(InstanceUniquePointer& createdPrefab, InstanceUniquePointer& instantiatedPrefab)
    {
        PrefabDom tempDomForStoringAndLoading;
        PrefabDomUtils::StoreInstanceInPrefabDom(*createdPrefab, tempDomForStoringAndLoading);

        PrefabDomUtils::LoadInstanceFromPrefabDom(
            *instantiatedPrefab, tempDomForStoringAndLoading, PrefabDomUtils::LoadFlags::UseSelectiveDeserialization);
    }

    AZStd::pair<InstanceUniquePointer, InstanceUniquePointer> SetupPrefabInstances(
        const AzToolsFramework::EntityList& entitiesToUseForCreation, PrefabSystemComponent* prefabSystemComponent)
    {
        AZStd::unique_ptr<AzToolsFramework::Prefab::Instance> createdPrefab =
            prefabSystemComponent->CreatePrefab(entitiesToUseForCreation, {}, "test/path");
        EXPECT_TRUE(createdPrefab != nullptr);

        AZStd::unique_ptr<AzToolsFramework::Prefab::Instance> instantiatedPrefab =
            prefabSystemComponent->InstantiatePrefab(createdPrefab->GetTemplateId());
        EXPECT_TRUE(instantiatedPrefab != nullptr);

        instantiatedPrefab->GetEntities(
            [](const AZStd::unique_ptr<AZ::Entity>& entity)
            {
                // Activate the entities so that we can later validate that entities stay activated throughout the deserialization.
                entity->Init();
                entity->Activate();
                return true;
            });

        return AZStd::make_pair<InstanceUniquePointer, InstanceUniquePointer>(AZStd::move(createdPrefab), AZStd::move(instantiatedPrefab));
    }

    TEST_F(InstanceDeserializationTest, ReloadInstanceUponComponentUpdate)
    {
        AZ::Entity* entity1 = CreateEntity("Entity1",false);
        AzToolsFramework::Components::TransformComponent* transformComponent = aznew AzToolsFramework::Components::TransformComponent;
        transformComponent->SetWorldTranslation(AZ::Vector3(10.0, 0, 0));
        entity1->AddComponent(transformComponent);

        AZ::Entity* entity2 = CreateEntity("Entity2", false);

        AZStd::pair<InstanceUniquePointer, InstanceUniquePointer> prefabInstances =
            SetupPrefabInstances(AzToolsFramework::EntityList{ entity1, entity2}, m_prefabSystemComponent);
        InstanceUniquePointer createdPrefab = AZStd::move(prefabInstances.first);
        InstanceUniquePointer instantiatedPrefab = AZStd::move(prefabInstances.second);

        createdPrefab->GetEntities(
            [](const AZStd::unique_ptr<AZ::Entity>& entity)
            {
                if (entity->GetName() == "Entity1")
                {
                    // Activate the entity to access the transform interface and use it to modify the transform component.
                    entity->Init();
                    entity->Activate();
                    AZ::TransformBus::Event(entity->GetId(), &AZ::TransformInterface::SetWorldX, 20.0f);
                }
                return true;
            });
        
        
        GenerateDomAndReloadInstantiatedPrefab(createdPrefab, instantiatedPrefab);

        instantiatedPrefab->GetEntities(
            [](const AZStd::unique_ptr<AZ::Entity>& entity)
            {
                if (entity->GetName() == "Entity2")
                {
                    // Since we didn't touch entity2 in createdPrefab, it should remain untouched in instantiatedPrefab and thus retain its
                    // active state.
                    EXPECT_EQ(entity->GetState(), AZ::Entity::State::Active);
                }
                else
                {
                    // Validate that the entity is in 'constructed' state, which indicates that it got reloaded.
                    EXPECT_EQ(entity->GetState(), AZ::Entity::State::Constructed);
                    AZStd::vector<AZ::Component*> entity1Components = entity->GetComponents();
                    EXPECT_TRUE(entity1Components.size() == 1);
                    AzToolsFramework::Components::TransformComponent* transformComponentInInstantiatedPrefab =
                        reinterpret_cast<AzToolsFramework::Components::TransformComponent*>(entity1Components.front());
                    EXPECT_TRUE(transformComponentInInstantiatedPrefab != nullptr);

                    // Validate that the transform component is correctly updated after reloading.
                    EXPECT_EQ(transformComponentInInstantiatedPrefab->GetWorldX(), 20.0f);
                }
                return true;
            });
    }

    TEST_F(InstanceDeserializationTest, ReloadInstanceUponComponentAdd)
    {
        AZ::Entity* entity1 = CreateEntity("Entity1", false);
        AZ::Entity* entity2 = CreateEntity("Entity2", false);

        AZStd::pair<InstanceUniquePointer, InstanceUniquePointer> prefabInstances =
            SetupPrefabInstances(AzToolsFramework::EntityList{ entity1, entity2 }, m_prefabSystemComponent);
        InstanceUniquePointer createdPrefab = AZStd::move(prefabInstances.first);
        InstanceUniquePointer instantiatedPrefab = AZStd::move(prefabInstances.second);

        createdPrefab->GetEntities(
            [](const AZStd::unique_ptr<AZ::Entity>& entity)
            {
                if (entity->GetName() == "Entity1")
                {
                    // Add a transform component to entity1 of createdPrefab.
                    entity->CreateComponent(AZ::EditorTransformComponentTypeId);
                }
                return true;
            });

        GenerateDomAndReloadInstantiatedPrefab(createdPrefab, instantiatedPrefab);

        instantiatedPrefab->GetEntities(
            [](const AZStd::unique_ptr<AZ::Entity>& entity)
            {
                if (entity->GetName() == "Entity2")
                {
                    // Since we didn't touch entity2 in createdPrefab, it should remain untouched in instantiatedPrefab and thus retain its
                    // active state.
                    EXPECT_EQ(entity->GetState(), AZ::Entity::State::Active);
                }
                else
                {
                    // Validate that the entity is in 'constructed' state, which indicates that it got reloaded.
                    EXPECT_EQ(entity->GetState(), AZ::Entity::State::Constructed);
                    AZStd::vector<AZ::Component*> entity1Components = entity->GetComponents();
                    EXPECT_TRUE(entity1Components.size() == 1);
                    AzToolsFramework::Components::TransformComponent* transformComponentInInstantiatedPrefab =
                        reinterpret_cast<AzToolsFramework::Components::TransformComponent*>(entity1Components.front());

                    // Validate that a transform component exists in entity1 of instantiatedPrefab.
                    EXPECT_TRUE(transformComponentInInstantiatedPrefab != nullptr);
                }
                return true;
            });
    }

    TEST_F(InstanceDeserializationTest, ReloadInstanceUponComponentDelete)
    {
        AZ::Entity* entity1 = CreateEntity("Entity1", false);
        AzToolsFramework::Components::TransformComponent* transformComponent = aznew AzToolsFramework::Components::TransformComponent;
        transformComponent->SetWorldTranslation(AZ::Vector3(10.0, 0, 0));
        entity1->AddComponent(transformComponent);

        AZ::Entity* entity2 = CreateEntity("Entity2", false);

        AZStd::pair<InstanceUniquePointer, InstanceUniquePointer> prefabInstances =
            SetupPrefabInstances(AzToolsFramework::EntityList{ entity1, entity2 }, m_prefabSystemComponent);
        InstanceUniquePointer createdPrefab = AZStd::move(prefabInstances.first);
        InstanceUniquePointer instantiatedPrefab = AZStd::move(prefabInstances.second);

        createdPrefab->GetEntities(
            [](const AZStd::unique_ptr<AZ::Entity>& entity)
            {
                if (entity->GetName() == "Entity1")
                {
                    // Remove the transform component from entity1 of createdPrefab;
                    AZ::Component* transformComponent = entity->GetComponents().front();
                    EXPECT_TRUE(transformComponent != nullptr);
                    entity->RemoveComponent(transformComponent);
                    delete transformComponent;
                    transformComponent = nullptr;
                }
                return true;
            });

        GenerateDomAndReloadInstantiatedPrefab(createdPrefab, instantiatedPrefab);

        instantiatedPrefab->GetEntities(
            [](const AZStd::unique_ptr<AZ::Entity>& entity)
            {
                if (entity->GetName() == "Entity2")
                {
                    // Since we didn't touch entity2 in createdPrefab, it should remain untouched in instantiatedPrefab and thus retain its
                    // active state.
                    EXPECT_EQ(entity->GetState(), AZ::Entity::State::Active);
                }
                else
                {
                    EXPECT_EQ(entity->GetState(), AZ::Entity::State::Constructed);
                    AZStd::vector<AZ::Component*> entity1Components = entity->GetComponents();

                    // Validate that the transform component can't be found in entity1 of instantiatedPrefab.
                    EXPECT_TRUE(entity1Components.size() == 0);
                }
                return true;
            });
    }
} // namespace UnitTest
