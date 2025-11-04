/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/EntityId.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Entity/PrefabEditorEntityOwnershipInterface.h>
#include <AzToolsFramework/Prefab/PrefabFocusInterface.h>
#include <AzToolsFramework/Prefab/PrefabFocusPublicInterface.h>
#include <Prefab/PrefabTestFixture.h>

namespace UnitTest
{
    class PrefabFocusTests
        : public PrefabTestFixture
    {
    protected:
        void GenerateTestHierarchy()
        {
            /*
             *  City (Prefab Container)
             *  |_  City
             *  |_  Street (Prefab Container)
             *      |_  Car (Prefab Container)
             *      |   |_ Passenger
             *      |_  SportsCar (Prefab Container)
             *          |_ Passenger
             */

            // Create loose entities
            m_entityMap[Passenger1EntityName] = CreateEntity(Passenger1EntityName);
            m_entityMap[Passenger2EntityName] = CreateEntity(Passenger2EntityName);
            m_entityMap[CityEntityName] = CreateEntity(CityEntityName);

            AddRequiredEditorComponents({
                m_entityMap[Passenger1EntityName]->GetId(),
                m_entityMap[Passenger2EntityName]->GetId(),
                m_entityMap[CityEntityName]->GetId() });

            // Call HandleEntitiesAdded to the loose entities to register them with the Prefab EOS
            AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
                &AzToolsFramework::EditorEntityContextRequests::HandleEntitiesAdded,
                AzToolsFramework::EntityList{ m_entityMap[Passenger1EntityName], m_entityMap[Passenger2EntityName], m_entityMap[CityEntityName] });

            // Initialize Prefab EOS Interface
            AzToolsFramework::PrefabEditorEntityOwnershipInterface* prefabEditorEntityOwnershipInterface =
                AZ::Interface<AzToolsFramework::PrefabEditorEntityOwnershipInterface>::Get();
            ASSERT_TRUE(prefabEditorEntityOwnershipInterface);

            // Create a car prefab from the passenger1 entity. The container entity will be created as part of the process.
            AZStd::unique_ptr<AzToolsFramework::Prefab::Instance> carInstance =
                m_prefabSystemComponent->CreatePrefab({ m_entityMap[Passenger1EntityName] }, {}, "test/car");
            ASSERT_TRUE(carInstance);
            m_instanceMap[CarEntityName] = carInstance.get();

            // Create a sportscar prefab from the passenger2 entity. The container entity will be created as part of the process.
            AZStd::unique_ptr<AzToolsFramework::Prefab::Instance> sportsCarInstance =
                m_prefabSystemComponent->CreatePrefab({ m_entityMap[Passenger2EntityName] }, {}, "test/sportsCar");
            ASSERT_TRUE(sportsCarInstance);
            m_instanceMap[SportsCarEntityName] = sportsCarInstance.get();

            // Create a street prefab that nests the car and sportscar instances created above. The container entity will be created as part of the process.
            AZStd::unique_ptr<AzToolsFramework::Prefab::Instance> streetInstance =
                m_prefabSystemComponent->CreatePrefab({}, MakeInstanceList(AZStd::move(carInstance), AZStd::move(sportsCarInstance)), "test/street");
            ASSERT_TRUE(streetInstance);
            m_instanceMap[StreetEntityName] = streetInstance.get();

            // Use the Prefab EOS root instance as the City instance. This will ensure functions that go through the EOS work in these tests too.
            m_rootInstance = prefabEditorEntityOwnershipInterface->GetRootPrefabInstance();
            ASSERT_TRUE(m_rootInstance.has_value());

            m_rootInstance->get().AddEntity(*m_entityMap[CityEntityName]);
            m_rootInstance->get().AddInstance(AZStd::move(streetInstance));

            m_instanceMap[CityEntityName] = &m_rootInstance->get();
        }

        void SetUpEditorFixtureImpl() override
        {
            PrefabTestFixture::SetUpEditorFixtureImpl();

            m_prefabFocusInterface = AZ::Interface<PrefabFocusInterface>::Get();
            ASSERT_TRUE(m_prefabFocusInterface != nullptr);

            m_prefabFocusPublicInterface = AZ::Interface<PrefabFocusPublicInterface>::Get();
            ASSERT_TRUE(m_prefabFocusPublicInterface != nullptr);

            AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
                m_editorEntityContextId, &AzToolsFramework::EditorEntityContextRequestBus::Events::GetEditorEntityContextId);

            GenerateTestHierarchy();
        }

        void TearDownEditorFixtureImpl() override
        {
            m_rootInstance->get().Reset();

            PrefabTestFixture::TearDownEditorFixtureImpl();
        }

        AZStd::unordered_map<AZStd::string, AZ::Entity*> m_entityMap;
        AZStd::unordered_map<AZStd::string, Instance*> m_instanceMap;

        InstanceOptionalReference m_rootInstance;

        PrefabFocusInterface* m_prefabFocusInterface = nullptr;
        PrefabFocusPublicInterface* m_prefabFocusPublicInterface = nullptr;
        AzFramework::EntityContextId m_editorEntityContextId = AzFramework::EntityContextId::CreateNull();

        inline static const char* CityEntityName = "City";
        inline static const char* StreetEntityName = "Street";
        inline static const char* CarEntityName = "Car";
        inline static const char* SportsCarEntityName = "SportsCar";
        inline static const char* Passenger1EntityName = "Passenger1";
        inline static const char* Passenger2EntityName = "Passenger2";
    };

    TEST_F(PrefabFocusTests, FocusOnOwningPrefabRootContainer)
    {
        // Verify FocusOnOwningPrefab works when passing the container entity of the root prefab.
        {
            m_prefabFocusPublicInterface->FocusOnOwningPrefab(m_instanceMap[CityEntityName]->GetContainerEntityId());
            EXPECT_EQ(
                m_prefabFocusInterface->GetFocusedPrefabTemplateId(m_editorEntityContextId),
                m_instanceMap[CityEntityName]->GetTemplateId());

            auto instance = m_prefabFocusInterface->GetFocusedPrefabInstance(m_editorEntityContextId);
            EXPECT_TRUE(instance.has_value());
            EXPECT_EQ(&instance->get(), m_instanceMap[CityEntityName]);
        }
    }

    TEST_F(PrefabFocusTests, FocusOnOwningPrefabRootEntity)
    {
        // Verify FocusOnOwningPrefab works when passing a nested entity of the root prefab.
        {
            m_prefabFocusPublicInterface->FocusOnOwningPrefab(m_entityMap[CityEntityName]->GetId());
            EXPECT_EQ(
                m_prefabFocusInterface->GetFocusedPrefabTemplateId(m_editorEntityContextId),
                m_instanceMap[CityEntityName]->GetTemplateId());

            auto instance = m_prefabFocusInterface->GetFocusedPrefabInstance(m_editorEntityContextId);
            EXPECT_TRUE(instance.has_value());
            EXPECT_EQ(&instance->get(), m_instanceMap[CityEntityName]);
        }
    }

    TEST_F(PrefabFocusTests, FocusOnOwningPrefabNestedContainer)
    {
        // Verify FocusOnOwningPrefab works when passing the container entity of a nested prefab.
        {
            m_prefabFocusPublicInterface->FocusOnOwningPrefab(m_instanceMap[CarEntityName]->GetContainerEntityId());
            EXPECT_EQ(
                m_prefabFocusInterface->GetFocusedPrefabTemplateId(m_editorEntityContextId),
                m_instanceMap[CarEntityName]->GetTemplateId());

            auto instance = m_prefabFocusInterface->GetFocusedPrefabInstance(m_editorEntityContextId);
            EXPECT_TRUE(instance.has_value());
            EXPECT_EQ(&instance->get(), m_instanceMap[CarEntityName]);
        }
    }

    TEST_F(PrefabFocusTests, FocusOnOwningPrefabNestedEntity)
    {
        // Verify FocusOnOwningPrefab works when passing a nested entity of the a nested prefab.
        {
            m_prefabFocusPublicInterface->FocusOnOwningPrefab(m_entityMap[Passenger1EntityName]->GetId());
            EXPECT_EQ(
                m_prefabFocusInterface->GetFocusedPrefabTemplateId(m_editorEntityContextId),
                m_instanceMap[CarEntityName]->GetTemplateId());

            auto instance = m_prefabFocusInterface->GetFocusedPrefabInstance(m_editorEntityContextId);
            EXPECT_TRUE(instance.has_value());
            EXPECT_EQ(&instance->get(), m_instanceMap[CarEntityName]);
        }
    }

    TEST_F(PrefabFocusTests, FocusOnOwningPrefabClear)
    {
        // Verify FocusOnOwningPrefab points to the root prefab when the focus is cleared.
        {
            AzToolsFramework::PrefabEditorEntityOwnershipInterface* prefabEditorEntityOwnershipInterface =
                AZ::Interface<AzToolsFramework::PrefabEditorEntityOwnershipInterface>::Get();
            AzToolsFramework::Prefab::InstanceOptionalReference rootPrefabInstance =
                prefabEditorEntityOwnershipInterface->GetRootPrefabInstance();
            EXPECT_TRUE(rootPrefabInstance.has_value());

            m_prefabFocusPublicInterface->FocusOnOwningPrefab(AZ::EntityId());
            EXPECT_EQ(
                m_prefabFocusInterface->GetFocusedPrefabTemplateId(m_editorEntityContextId),
                rootPrefabInstance->get().GetTemplateId());

            auto instance = m_prefabFocusInterface->GetFocusedPrefabInstance(m_editorEntityContextId);
            EXPECT_TRUE(instance.has_value());
            EXPECT_EQ(&instance->get(), &rootPrefabInstance->get());
        }
    }

    TEST_F(PrefabFocusTests, FocusOnParentOfFocusedPrefabLeaf)
    {
        // Call FocusOnParentOfFocusedPrefab on a leaf instance and verify the parent is focused correctly.
        {
            m_prefabFocusPublicInterface->FocusOnOwningPrefab(m_instanceMap[CarEntityName]->GetContainerEntityId());
            m_prefabFocusPublicInterface->FocusOnParentOfFocusedPrefab(m_editorEntityContextId);

            EXPECT_EQ(
                &m_prefabFocusInterface->GetFocusedPrefabInstance(m_editorEntityContextId)->get(),
                m_instanceMap[StreetEntityName]
            );
        }
    }

    TEST_F(PrefabFocusTests, FocusOnParentOfFocusedPrefabRoot)
    {
        // Call FocusOnParentOfFocusedPrefab on the root instance and verify the operation fails.
        {
            m_prefabFocusPublicInterface->FocusOnOwningPrefab(m_instanceMap[CityEntityName]->GetContainerEntityId());
            auto outcome = m_prefabFocusPublicInterface->FocusOnParentOfFocusedPrefab(m_editorEntityContextId);

            EXPECT_FALSE(outcome.IsSuccess());
        }
    }

    TEST_F(PrefabFocusTests, IsOwningPrefabBeingFocusedContent)
    {
        // Verify IsOwningPrefabBeingFocused returns true for all entities in a focused prefab (container/nested)
        {
            m_prefabFocusPublicInterface->FocusOnOwningPrefab(m_instanceMap[CityEntityName]->GetContainerEntityId());

            EXPECT_TRUE(m_prefabFocusPublicInterface->IsOwningPrefabBeingFocused(m_instanceMap[CityEntityName]->GetContainerEntityId()));
            EXPECT_TRUE(m_prefabFocusPublicInterface->IsOwningPrefabBeingFocused(m_entityMap[CityEntityName]->GetId()));
        }
    }

    TEST_F(PrefabFocusTests, IsOwningPrefabBeingFocusedAncestorsDescendants)
    {
        // Verify IsOwningPrefabBeingFocused returns false for all entities not in a focused prefab (ancestors/descendants)
        {
            m_prefabFocusPublicInterface->FocusOnOwningPrefab(m_instanceMap[StreetEntityName]->GetContainerEntityId());

            EXPECT_TRUE(m_prefabFocusPublicInterface->IsOwningPrefabBeingFocused(m_instanceMap[StreetEntityName]->GetContainerEntityId()));
            EXPECT_FALSE(m_prefabFocusPublicInterface->IsOwningPrefabBeingFocused(m_instanceMap[CityEntityName]->GetContainerEntityId()));
            EXPECT_FALSE(m_prefabFocusPublicInterface->IsOwningPrefabBeingFocused(m_entityMap[CityEntityName]->GetId()));
            EXPECT_FALSE(m_prefabFocusPublicInterface->IsOwningPrefabBeingFocused(m_instanceMap[CarEntityName]->GetContainerEntityId()));
            EXPECT_FALSE(m_prefabFocusPublicInterface->IsOwningPrefabBeingFocused(m_entityMap[Passenger1EntityName]->GetId()));
        }
    }

    TEST_F(PrefabFocusTests, IsOwningPrefabBeingFocusedSiblings)
    {
        // Verify IsOwningPrefabBeingFocused returns false for all entities not in a focused prefab (siblings)
        {
            m_prefabFocusPublicInterface->FocusOnOwningPrefab(m_instanceMap[SportsCarEntityName]->GetContainerEntityId());

            EXPECT_TRUE(m_prefabFocusPublicInterface->IsOwningPrefabBeingFocused(m_instanceMap[SportsCarEntityName]->GetContainerEntityId()));
            EXPECT_TRUE(m_prefabFocusPublicInterface->IsOwningPrefabBeingFocused(m_entityMap[Passenger2EntityName]->GetId()));
            EXPECT_FALSE(m_prefabFocusPublicInterface->IsOwningPrefabBeingFocused(m_instanceMap[CarEntityName]->GetContainerEntityId()));
            EXPECT_FALSE(m_prefabFocusPublicInterface->IsOwningPrefabBeingFocused(m_entityMap[Passenger1EntityName]->GetId()));
        }
    }

}
