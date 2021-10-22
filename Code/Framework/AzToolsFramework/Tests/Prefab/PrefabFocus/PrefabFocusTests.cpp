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

            // Call HandleEntitiesAdded to the loose entities to register them with the Prefab EOS
            AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
                &AzToolsFramework::EditorEntityContextRequests::HandleEntitiesAdded,
                AzToolsFramework::EntityList{ m_entityMap[Passenger1EntityName], m_entityMap[Passenger2EntityName], m_entityMap[CityEntityName] });

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

            // Create a city prefab that nests the street instances created above and the city entity. The container entity will be created as part of the process.
            m_rootInstance =
                m_prefabSystemComponent->CreatePrefab({ m_entityMap[CityEntityName] }, MakeInstanceList(AZStd::move(streetInstance)), "test/city");
            ASSERT_TRUE(m_rootInstance);
            m_instanceMap[CityEntityName] = m_rootInstance.get();
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
            m_rootInstance.release();

            PrefabTestFixture::TearDownEditorFixtureImpl();
        }

        AZStd::unordered_map<AZStd::string, AZ::Entity*> m_entityMap;
        AZStd::unordered_map<AZStd::string, Instance*> m_instanceMap;

        AZStd::unique_ptr<AzToolsFramework::Prefab::Instance> m_rootInstance;

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

    TEST_F(PrefabFocusTests, PrefabFocus_FocusOnOwningPrefab_RootContainer)
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

    TEST_F(PrefabFocusTests, PrefabFocus_FocusOnOwningPrefab_RootEntity)
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

    TEST_F(PrefabFocusTests, PrefabFocus_FocusOnOwningPrefab_NestedContainer)
    {
        // Verify FocusOnOwningPrefab works when passing the container entity of a nested prefab.
        {
            m_prefabFocusPublicInterface->FocusOnOwningPrefab(m_instanceMap[CarEntityName]->GetContainerEntityId());
            EXPECT_EQ(
                m_prefabFocusInterface->GetFocusedPrefabTemplateId(m_editorEntityContextId), m_instanceMap[CarEntityName]->GetTemplateId());

            auto instance = m_prefabFocusInterface->GetFocusedPrefabInstance(m_editorEntityContextId);
            EXPECT_TRUE(instance.has_value());
            EXPECT_EQ(&instance->get(), m_instanceMap[CarEntityName]);
        }
    }

    TEST_F(PrefabFocusTests, PrefabFocus_FocusOnOwningPrefab_NestedEntity)
    {
        // Verify FocusOnOwningPrefab works when passing a nested entity of the a nested prefab.
        {
            m_prefabFocusPublicInterface->FocusOnOwningPrefab(m_entityMap[Passenger1EntityName]->GetId());
            EXPECT_EQ(
                m_prefabFocusInterface->GetFocusedPrefabTemplateId(m_editorEntityContextId), m_instanceMap[CarEntityName]->GetTemplateId());

            auto instance = m_prefabFocusInterface->GetFocusedPrefabInstance(m_editorEntityContextId);
            EXPECT_TRUE(instance.has_value());
            EXPECT_EQ(&instance->get(), m_instanceMap[CarEntityName]);
        }
    }

    TEST_F(PrefabFocusTests, PrefabFocus_FocusOnOwningPrefab_Clear)
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
                m_prefabFocusInterface->GetFocusedPrefabTemplateId(m_editorEntityContextId), rootPrefabInstance->get().GetTemplateId());

            auto instance = m_prefabFocusInterface->GetFocusedPrefabInstance(m_editorEntityContextId);
            EXPECT_TRUE(instance.has_value());
            EXPECT_EQ(&instance->get(), &rootPrefabInstance->get());
        }
    }

    TEST_F(PrefabFocusTests, PrefabFocus_IsOwningPrefabBeingFocused_Content)
    {
        // Verify IsOwningPrefabBeingFocused returns true for all entities in a focused prefab (container/nested)
        {
            m_prefabFocusPublicInterface->FocusOnOwningPrefab(m_instanceMap[CityEntityName]->GetContainerEntityId());

            EXPECT_TRUE(m_prefabFocusPublicInterface->IsOwningPrefabBeingFocused(m_instanceMap[CityEntityName]->GetContainerEntityId()));
            EXPECT_TRUE(m_prefabFocusPublicInterface->IsOwningPrefabBeingFocused(m_entityMap[CityEntityName]->GetId()));
        }
    }

    TEST_F(PrefabFocusTests, PrefabFocus_IsOwningPrefabBeingFocused_AncestorsDescendants)
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

    TEST_F(PrefabFocusTests, PrefabFocus_IsOwningPrefabBeingFocused_Siblings)
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
