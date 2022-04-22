/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma optimize("", off)
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
        const AzToolsFramework::EntityList& entitiesToUseForCreation,
        AZStd::vector<AZStd::unique_ptr<Instance>>&& nestedInstances, PrefabSystemComponent* prefabSystemComponent)
    {
        AZStd::unique_ptr<AzToolsFramework::Prefab::Instance> createdPrefab =
            prefabSystemComponent->CreatePrefab(entitiesToUseForCreation, AZStd::move(nestedInstances), "test/path");
        EXPECT_NE(nullptr, createdPrefab);

        AZStd::unique_ptr<AzToolsFramework::Prefab::Instance> instantiatedPrefab =
            prefabSystemComponent->InstantiatePrefab(createdPrefab->GetTemplateId());
        EXPECT_NE(nullptr, instantiatedPrefab);

        instantiatedPrefab->GetAllEntitiesInHierarchy(
            [](const AZStd::unique_ptr<AZ::Entity>& entity)
            {
                // Activate the entities so that we can later validate that entities stay activated throughout the deserialization.
                entity->Init();
                entity->Activate();
                return true;
            });

        return { AZStd::move(createdPrefab), AZStd::move(instantiatedPrefab) };
    }

    void ValidateEntityState(const InstanceUniquePointer& instanceToLookUnder, AZStd::string_view entityName, AZ::Entity::State expectedEntityState)
    {
        bool isEntityFound = false;
        instanceToLookUnder->GetEntities(
            [&isEntityFound, &entityName, &expectedEntityState](const AZStd::unique_ptr<AZ::Entity>& entity)
            {
                if (entity->GetName() == entityName)
                {
                    EXPECT_EQ(entity->GetState(), expectedEntityState);
                    isEntityFound = true;
                }
                return true;
            });
        EXPECT_TRUE(isEntityFound); 
    }


    TEST_F(InstanceDeserializationTest, ReloadInstanceUponComponentUpdate)
    {
        AZ::Entity* entity1 = CreateEntity("Entity1",false);
        AzToolsFramework::Components::TransformComponent* transformComponent = aznew AzToolsFramework::Components::TransformComponent;
        transformComponent->SetWorldTranslation(AZ::Vector3(10.0, 0, 0));
        entity1->AddComponent(transformComponent);

        AZ::Entity* entity2 = CreateEntity("Entity2", false);

        AZStd::pair<InstanceUniquePointer, InstanceUniquePointer> prefabInstances =
            SetupPrefabInstances(AzToolsFramework::EntityList{ entity1, entity2}, {}, m_prefabSystemComponent);
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
                else if (entity->GetName() == "Entity1")
                {
                    // Validate that the entity is in 'constructed' state, which indicates that it got reloaded.
                    EXPECT_EQ(entity->GetState(), AZ::Entity::State::Constructed);
                    AZStd::vector<AZ::Component*> entity1Components = entity->GetComponents();
                    EXPECT_EQ(1, entity1Components.size());
                    AzToolsFramework::Components::TransformComponent* transformComponentInInstantiatedPrefab =
                        reinterpret_cast<AzToolsFramework::Components::TransformComponent*>(entity1Components.front());
                    EXPECT_NE(nullptr, transformComponentInInstantiatedPrefab);

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
            SetupPrefabInstances(AzToolsFramework::EntityList{ entity1, entity2 }, {}, m_prefabSystemComponent);
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
                else if (entity->GetName() == "Entity1")
                {
                    // Validate that the entity is in 'constructed' state, which indicates that it got reloaded.
                    EXPECT_EQ(entity->GetState(), AZ::Entity::State::Constructed);
                    AZStd::vector<AZ::Component*> entity1Components = entity->GetComponents();
                    EXPECT_EQ(1, entity1Components.size());
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
            SetupPrefabInstances(AzToolsFramework::EntityList{ entity1, entity2 }, {}, m_prefabSystemComponent);
        InstanceUniquePointer createdPrefab = AZStd::move(prefabInstances.first);
        InstanceUniquePointer instantiatedPrefab = AZStd::move(prefabInstances.second);

        createdPrefab->GetEntities(
            [](const AZStd::unique_ptr<AZ::Entity>& entity)
            {
                if (entity->GetName() == "Entity1")
                {
                    // Remove the transform component from entity1 of createdPrefab;
                    AZ::Component* transformComponent = entity->GetComponents().front();
                    EXPECT_NE(nullptr, transformComponent);
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
                else if(entity->GetName() == "Entity1")
                {
                    EXPECT_EQ(entity->GetState(), AZ::Entity::State::Constructed);
                    AZStd::vector<AZ::Component*> entity1Components = entity->GetComponents();

                    // Validate that the transform component can't be found in entity1 of instantiatedPrefab.
                    EXPECT_TRUE(entity1Components.empty());
                }
                return true;
            });
    }

    TEST_F(InstanceDeserializationTest, ReloadInstanceUponAddingEntityToExistingEntities)
    {
        AZ::Entity* entity1 = CreateEntity("Entity1", false);

        AZStd::pair<InstanceUniquePointer, InstanceUniquePointer> prefabInstances =
            SetupPrefabInstances(AzToolsFramework::EntityList{ entity1 }, {}, m_prefabSystemComponent);
        InstanceUniquePointer createdPrefab = AZStd::move(prefabInstances.first);
        InstanceUniquePointer instantiatedPrefab = AZStd::move(prefabInstances.second);

        createdPrefab->AddEntity(AZStd::move(AZStd::make_unique<AZ::Entity>("Entity2")));
        GenerateDomAndReloadInstantiatedPrefab(createdPrefab, instantiatedPrefab);

        bool isEntity1Found = false, isEntity2Found = false;
        instantiatedPrefab->GetEntities(
            [&isEntity1Found, &isEntity2Found](const AZStd::unique_ptr<AZ::Entity>& entity)
            {
                if (entity->GetName() == "Entity1")
                {
                    // Since we didn't touch entity1 in createdPrefab, it should remain untouched in instantiatedPrefab and thus retain its
                    // active state.
                    EXPECT_EQ(entity->GetState(), AZ::Entity::State::Active);
                    isEntity1Found = true;
                }
                else if (entity->GetName() == "Entity2")
                {
                    // Validate that the entity2 is in 'constructed' state, which indicates that it got added from dom.
                    EXPECT_EQ(entity->GetState(), AZ::Entity::State::Constructed);
                    isEntity2Found = true;
                }
                return true;
            });
        EXPECT_TRUE(isEntity1Found);
        EXPECT_TRUE(isEntity2Found);
    }

    TEST_F(InstanceDeserializationTest, ReloadInstanceUponAddingTheFirstEntity)
    {
        AZStd::pair<InstanceUniquePointer, InstanceUniquePointer> prefabInstances =
            SetupPrefabInstances(AzToolsFramework::EntityList{}, {}, m_prefabSystemComponent);
        InstanceUniquePointer createdPrefab = AZStd::move(prefabInstances.first);
        InstanceUniquePointer instantiatedPrefab = AZStd::move(prefabInstances.second);

        createdPrefab->AddEntity(AZStd::move(AZStd::make_unique<AZ::Entity>("Entity1")));
        GenerateDomAndReloadInstantiatedPrefab(createdPrefab, instantiatedPrefab);

        bool isEntity1Found = false;
        instantiatedPrefab->GetEntities(
            [&isEntity1Found](const AZStd::unique_ptr<AZ::Entity>& entity)
            {
                if (entity->GetName() == "Entity1")
                {
                    isEntity1Found = true;
                }
                return true;
            });
        EXPECT_TRUE(isEntity1Found);
    }

    TEST_F(InstanceDeserializationTest, ReloadInstanceUponDeletingOneAmongManyEntities)
    {
        AZ::Entity* entity1 = CreateEntity("Entity1", false);
        AZ::Entity* entity2 = CreateEntity("Entity2", false);

        AZStd::pair<InstanceUniquePointer, InstanceUniquePointer> prefabInstances =
            SetupPrefabInstances(AzToolsFramework::EntityList{ entity1, entity2 }, {}, m_prefabSystemComponent);
        InstanceUniquePointer createdPrefab = AZStd::move(prefabInstances.first);
        InstanceUniquePointer instantiatedPrefab = AZStd::move(prefabInstances.second);

        createdPrefab->DetachEntity(entity2->GetId());
        GenerateDomAndReloadInstantiatedPrefab(createdPrefab, instantiatedPrefab);

        bool isEntity1Found = false, isEntity2Found = false;
        instantiatedPrefab->GetEntities(
            [&isEntity1Found, &isEntity2Found](const AZStd::unique_ptr<AZ::Entity>& entity)
            {
                if (entity->GetName() == "Entity1")
                {
                    // Since we didn't touch entity1 in createdPrefab, it should remain untouched in instantiatedPrefab and thus retain its
                    // active state.
                    EXPECT_EQ(entity->GetState(), AZ::Entity::State::Active);
                    isEntity1Found = true;
                }
                else if (entity->GetName() == "Entity2")
                {
                    // This shouldn't be hit since Entity2 should have been removed. Mark the boolean as true and throw an error later.
                    isEntity2Found = true;
                }
                return true;
            });
        EXPECT_TRUE(isEntity1Found);
        EXPECT_FALSE(isEntity2Found);
    }

    TEST_F(InstanceDeserializationTest, ReloadInstanceUponDeletingTheOnlyEntity)
    {
        AZ::Entity* entity1 = CreateEntity("Entity1", false);

        AZStd::pair<InstanceUniquePointer, InstanceUniquePointer> prefabInstances =
            SetupPrefabInstances(AzToolsFramework::EntityList{ entity1 }, {}, m_prefabSystemComponent);
        InstanceUniquePointer createdPrefab = AZStd::move(prefabInstances.first);
        InstanceUniquePointer instantiatedPrefab = AZStd::move(prefabInstances.second);

        createdPrefab->DetachEntity(entity1->GetId());
        GenerateDomAndReloadInstantiatedPrefab(createdPrefab, instantiatedPrefab);

        bool isEntity1Found = false;
        instantiatedPrefab->GetEntities(
            [&isEntity1Found](const AZStd::unique_ptr<AZ::Entity>& entity)
            {
                if (entity->GetName() == "Entity1")
                {
                    // Since we didn't touch entity1 in createdPrefab, it should remain untouched in instantiatedPrefab and thus retain its
                    // active state.
                    EXPECT_EQ(entity->GetState(), AZ::Entity::State::Active);
                    isEntity1Found = true;
                }
                return true;
            });
        EXPECT_FALSE(isEntity1Found);
    }

    TEST_F(InstanceDeserializationTest, ReloadInstanceUponAddingTheFirstNestedInstance)
    {
        AZ::Entity* entity1 = CreateEntity("Entity1", false);
        AZStd::pair<InstanceUniquePointer, InstanceUniquePointer> prefabInstances =
            SetupPrefabInstances(AzToolsFramework::EntityList{ entity1 }, {}, m_prefabSystemComponent);
        InstanceUniquePointer createdPrefab = AZStd::move(prefabInstances.first);
        InstanceUniquePointer instantiatedPrefab = AZStd::move(prefabInstances.second);

        AZStd::unique_ptr<AzToolsFramework::Prefab::Instance> nestedPrefab = m_prefabSystemComponent->CreatePrefab(
            AzToolsFramework::EntityList{ CreateEntity("Entity1", false) }, {}, "test/nestedPrefabPath");

        // Extract the template id from the instance and store it in a variable before moving the instance.
        TemplateId nestedPrefabTemplateId = nestedPrefab->GetTemplateId();
        createdPrefab->AddInstance(AZStd::move(nestedPrefab));
        GenerateDomAndReloadInstantiatedPrefab(createdPrefab, instantiatedPrefab);

        ValidateEntityState(instantiatedPrefab, "Entity1", AZ::Entity::State::Active);
        EXPECT_EQ(instantiatedPrefab->GetNestedInstanceAliases(nestedPrefabTemplateId).size(), 1);
    }

    TEST_F(InstanceDeserializationTest, ReloadInstanceUponAddingNestedInstanceToExistingNestedInstances)
    {
        AZ::Entity* entity1 = CreateEntity("EntityUnderParentPrefab", false);
        InstanceUniquePointer nestedInstanceToUseForCreation = m_prefabSystemComponent->CreatePrefab(
            AzToolsFramework::EntityList{ CreateEntity("EntityUnderNestedPrefab", false) }, {}, "test/nestedPrefabPath");

        // Extract the template id from the instance and store it in a variable before moving the instance.
        TemplateId nestedPrefabTemplateId = nestedInstanceToUseForCreation->GetTemplateId();

        AZStd::vector<InstanceUniquePointer> nestedInstances;
        nestedInstances.emplace_back(AZStd::move(nestedInstanceToUseForCreation));
        
        AZStd::pair<InstanceUniquePointer, InstanceUniquePointer> prefabInstances = SetupPrefabInstances(
            AzToolsFramework::EntityList{ entity1 }, AZStd::move(nestedInstances),
            m_prefabSystemComponent);
        InstanceUniquePointer createdPrefab = AZStd::move(prefabInstances.first);
        InstanceUniquePointer instantiatedPrefab = AZStd::move(prefabInstances.second);

        InstanceUniquePointer nestedInstanceToAdd = m_prefabSystemComponent->InstantiatePrefab(nestedPrefabTemplateId);
        Instance& instanceAdded = createdPrefab->AddInstance(AZStd::move(nestedInstanceToAdd));
        InstanceAlias aliasOfInstanceAdded = instanceAdded.GetInstanceAlias();

        GenerateDomAndReloadInstantiatedPrefab(createdPrefab, instantiatedPrefab);

        ValidateEntityState(instantiatedPrefab, "EntityUnderParentPrefab", AZ::Entity::State::Active);
        EXPECT_EQ(instantiatedPrefab->GetNestedInstanceAliases(nestedPrefabTemplateId).size(), 2);
        instantiatedPrefab->GetNestedInstances(
            [&aliasOfInstanceAdded](AZStd::unique_ptr<Instance>& nestedInstance)
            {
                if (nestedInstance->GetInstanceAlias() == aliasOfInstanceAdded)
                {
                    ValidateEntityState(nestedInstance, "EntityUnderNestedPrefab", AZ::Entity::State::Constructed);
                }
                else
                {
                    ValidateEntityState(nestedInstance, "EntityUnderNestedPrefab", AZ::Entity::State::Active);
                }
                
        });
    }

    TEST_F(InstanceDeserializationTest, ReloadInstanceUponDeletingTheOnlyNestedInstance)
    {
        AZ::Entity* entity1 = CreateEntity("EntityUnderParentPrefab", false);
        InstanceUniquePointer nestedInstanceToUseForCreation = m_prefabSystemComponent->CreatePrefab(
            AzToolsFramework::EntityList{ CreateEntity("EntityUnderNestedPrefab", false) }, {}, "test/nestedPrefabPath");

        // Extract the template id from the instance and store it in a variable before moving the instance.
        TemplateId nestedPrefabTemplateId = nestedInstanceToUseForCreation->GetTemplateId();

        AZStd::vector<InstanceUniquePointer> nestedInstances;
        nestedInstances.emplace_back(AZStd::move(nestedInstanceToUseForCreation));

        AZStd::pair<InstanceUniquePointer, InstanceUniquePointer> prefabInstances =
            SetupPrefabInstances(AzToolsFramework::EntityList{ entity1 }, AZStd::move(nestedInstances), m_prefabSystemComponent);
        InstanceUniquePointer createdPrefab = AZStd::move(prefabInstances.first);
        InstanceUniquePointer instantiatedPrefab = AZStd::move(prefabInstances.second);

        ASSERT_EQ(createdPrefab->GetNestedInstanceAliases(nestedPrefabTemplateId).size(), 1);
        InstanceUniquePointer nestedInstanceToRemove =
            createdPrefab->DetachNestedInstance(createdPrefab->GetNestedInstanceAliases(nestedPrefabTemplateId).front());
        nestedInstanceToRemove.reset();

        GenerateDomAndReloadInstantiatedPrefab(createdPrefab, instantiatedPrefab);

        ValidateEntityState(instantiatedPrefab, "EntityUnderParentPrefab", AZ::Entity::State::Active);
        EXPECT_EQ(instantiatedPrefab->GetNestedInstanceAliases(nestedPrefabTemplateId).size(), 0);
    }

    TEST_F(InstanceDeserializationTest, ReloadInstanceUponDeletingOneAmongManyNestedInstances)
    {
        AZ::Entity* entity1 = CreateEntity("EntityUnderParentPrefab", false);
        InstanceUniquePointer nestedInstance1 = m_prefabSystemComponent->CreatePrefab(
            AzToolsFramework::EntityList{ CreateEntity("EntityUnderNestedPrefab", false) }, {}, "test/nestedPrefabPath");

        // Extract the template id from the instance and store it in a variable before moving the instance.
        TemplateId nestedPrefabTemplateId = nestedInstance1->GetTemplateId();

        InstanceUniquePointer nestedInstance2 = m_prefabSystemComponent->InstantiatePrefab(nestedPrefabTemplateId);

        AZStd::vector<InstanceUniquePointer> nestedInstances;
        nestedInstances.emplace_back(AZStd::move(nestedInstance1));
        nestedInstances.emplace_back(AZStd::move(nestedInstance2));

        AZStd::pair<InstanceUniquePointer, InstanceUniquePointer> prefabInstances =
            SetupPrefabInstances(AzToolsFramework::EntityList{ entity1 }, AZStd::move(nestedInstances), m_prefabSystemComponent);
        InstanceUniquePointer createdPrefab = AZStd::move(prefabInstances.first);
        InstanceUniquePointer instantiatedPrefab = AZStd::move(prefabInstances.second);

        AZStd::vector<InstanceAlias> nestedInstanceAliases = createdPrefab->GetNestedInstanceAliases(nestedPrefabTemplateId);
        ASSERT_EQ(nestedInstanceAliases.size(), 2);
        InstanceUniquePointer nestedInstanceToRemove = createdPrefab->DetachNestedInstance(nestedInstanceAliases.front());
        nestedInstanceToRemove.reset();

        GenerateDomAndReloadInstantiatedPrefab(createdPrefab, instantiatedPrefab);

        ValidateEntityState(instantiatedPrefab, "EntityUnderParentPrefab", AZ::Entity::State::Active);
        EXPECT_EQ(instantiatedPrefab->GetNestedInstanceAliases(nestedPrefabTemplateId).size(), 1);
        instantiatedPrefab->GetNestedInstances(
            [&nestedInstanceAliases](AZStd::unique_ptr<Instance>& nestedInstance)
            {
                if (nestedInstance->GetInstanceAlias() == nestedInstanceAliases.back())
                {
                    ValidateEntityState(nestedInstance, "EntityUnderNestedPrefab", AZ::Entity::State::Active);
                }
            });
    }
} // namespace UnitTest
#pragma optimize("", on)
