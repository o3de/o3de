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

            m_entityMap["passenger1"] = CreateEntity("Passenger1");
            m_entityMap["passenger2"] = CreateEntity("Passenger2");
            m_entityMap["city"] = CreateEntity("City");

            AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
                &AzToolsFramework::EditorEntityContextRequests::HandleEntitiesAdded,
                AzToolsFramework::EntityList{ m_entityMap["passenger1"], m_entityMap["passenger2"], m_entityMap["city"] });

            AZStd::unique_ptr<AzToolsFramework::Prefab::Instance> carInstance =
                m_prefabSystemComponent->CreatePrefab({ m_entityMap["passenger1"] }, {}, "test/car");
            ASSERT_TRUE(carInstance);
            m_instanceMap["car"] = carInstance.get();

            AZStd::unique_ptr<AzToolsFramework::Prefab::Instance> sportsCarInstance =
                m_prefabSystemComponent->CreatePrefab({ m_entityMap["passenger2"] }, {}, "test/sportsCar");
            ASSERT_TRUE(sportsCarInstance);
            m_instanceMap["sportsCar"] = sportsCarInstance.get();

            AZStd::unique_ptr<AzToolsFramework::Prefab::Instance> streetInstance =
                m_prefabSystemComponent->CreatePrefab({}, MakeInstanceList( AZStd::move(carInstance), AZStd::move(sportsCarInstance) ), "test/street");
            ASSERT_TRUE(streetInstance);
            m_instanceMap["street"] = streetInstance.get();

            m_rootInstance =
                m_prefabSystemComponent->CreatePrefab({ m_entityMap["city"] }, MakeInstanceList(AZStd::move(streetInstance)), "test/city");
            ASSERT_TRUE(m_rootInstance);
            m_instanceMap["city"] = m_rootInstance.get();
        }

        AZStd::unordered_map<AZStd::string, AZ::Entity*> m_entityMap;
        AZStd::unordered_map<AZStd::string, Instance*> m_instanceMap;

        AZStd::unique_ptr<AzToolsFramework::Prefab::Instance> m_rootInstance;
    };

    TEST_F(PrefabFocusTests, PrefabFocus_FocusOnOwningPrefab)
    {
        GenerateTestHierarchy();

        PrefabFocusInterface* prefabFocusInterface = AZ::Interface<PrefabFocusInterface>::Get();
        EXPECT_TRUE(prefabFocusInterface != nullptr);
        
        // Verify FocusOnOwningPrefab works when passing the container entity of the root prefab.
        {
            prefabFocusInterface->FocusOnOwningPrefab(m_instanceMap["city"]->GetContainerEntityId());
            EXPECT_EQ(prefabFocusInterface->GetFocusedPrefabTemplateId(), m_instanceMap["city"]->GetTemplateId());

            auto instance = prefabFocusInterface->GetFocusedPrefabInstance();
            EXPECT_TRUE(instance.has_value());
            EXPECT_EQ(&instance->get(), m_instanceMap["city"]);
        }

        // Verify FocusOnOwningPrefab works when passing a nested entity of the root prefab.
        {
            prefabFocusInterface->FocusOnOwningPrefab(m_entityMap["city"]->GetId());
            EXPECT_EQ(prefabFocusInterface->GetFocusedPrefabTemplateId(), m_instanceMap["city"]->GetTemplateId());

            auto instance = prefabFocusInterface->GetFocusedPrefabInstance();
            EXPECT_TRUE(instance.has_value());
            EXPECT_EQ(&instance->get(), m_instanceMap["city"]);
        }

        // Verify FocusOnOwningPrefab works when passing the container entity of a nested prefab.
        {
            prefabFocusInterface->FocusOnOwningPrefab(m_instanceMap["car"]->GetContainerEntityId());
            EXPECT_EQ(prefabFocusInterface->GetFocusedPrefabTemplateId(), m_instanceMap["car"]->GetTemplateId());

            auto instance = prefabFocusInterface->GetFocusedPrefabInstance();
            EXPECT_TRUE(instance.has_value());
            EXPECT_EQ(&instance->get(), m_instanceMap["car"]);
        }

        // Verify FocusOnOwningPrefab works when passing a nested entity of the a nested prefab.
        {
            prefabFocusInterface->FocusOnOwningPrefab(m_entityMap["passenger1"]->GetId());
            EXPECT_EQ(prefabFocusInterface->GetFocusedPrefabTemplateId(), m_instanceMap["car"]->GetTemplateId());

            auto instance = prefabFocusInterface->GetFocusedPrefabInstance();
            EXPECT_TRUE(instance.has_value());
            EXPECT_EQ(&instance->get(), m_instanceMap["car"]);
        }

        // Verify FocusOnOwningPrefab points to the root prefab when the focus is cleared.
        {
            AzToolsFramework::PrefabEditorEntityOwnershipInterface* prefabEditorEntityOwnershipInterface =
                AZ::Interface<AzToolsFramework::PrefabEditorEntityOwnershipInterface>::Get();
            AzToolsFramework::Prefab::InstanceOptionalReference rootPrefabInstance =
                prefabEditorEntityOwnershipInterface->GetRootPrefabInstance();
            EXPECT_TRUE(rootPrefabInstance.has_value());

            prefabFocusInterface->FocusOnOwningPrefab(AZ::EntityId());
            EXPECT_EQ(prefabFocusInterface->GetFocusedPrefabTemplateId(), rootPrefabInstance->get().GetTemplateId());

            auto instance = prefabFocusInterface->GetFocusedPrefabInstance();
            EXPECT_TRUE(instance.has_value());
            EXPECT_EQ(&instance->get(), &rootPrefabInstance->get());
        }

        m_rootInstance.release();
    }

    TEST_F(PrefabFocusTests, PrefabFocus_IsOwningPrefabBeingFocused)
    {
        GenerateTestHierarchy();

        PrefabFocusInterface* prefabFocusInterface = AZ::Interface<PrefabFocusInterface>::Get();
        EXPECT_TRUE(prefabFocusInterface != nullptr);

        // Verify IsOwningPrefabBeingFocused returns true for all entities in a focused prefab (container/nested)
        {
            prefabFocusInterface->FocusOnOwningPrefab(m_instanceMap["city"]->GetContainerEntityId());

            EXPECT_TRUE(prefabFocusInterface->IsOwningPrefabBeingFocused(m_instanceMap["city"]->GetContainerEntityId()));
            EXPECT_TRUE(prefabFocusInterface->IsOwningPrefabBeingFocused(m_entityMap["city"]->GetId()));
        }

        // Verify IsOwningPrefabBeingFocused returns false for all entities not in a focused prefab (ancestors/descendants)
        {
            prefabFocusInterface->FocusOnOwningPrefab(m_instanceMap["street"]->GetContainerEntityId());

            EXPECT_TRUE(prefabFocusInterface->IsOwningPrefabBeingFocused(m_instanceMap["street"]->GetContainerEntityId()));
            EXPECT_FALSE(prefabFocusInterface->IsOwningPrefabBeingFocused(m_instanceMap["city"]->GetContainerEntityId()));
            EXPECT_FALSE(prefabFocusInterface->IsOwningPrefabBeingFocused(m_entityMap["city"]->GetId()));
            EXPECT_FALSE(prefabFocusInterface->IsOwningPrefabBeingFocused(m_instanceMap["car"]->GetContainerEntityId()));
            EXPECT_FALSE(prefabFocusInterface->IsOwningPrefabBeingFocused(m_entityMap["passenger1"]->GetId()));
        }

        // Verify IsOwningPrefabBeingFocused returns false for all entities not in a focused prefab (siblings)
        {
            prefabFocusInterface->FocusOnOwningPrefab(m_instanceMap["sportsCar"]->GetContainerEntityId());

            EXPECT_TRUE(prefabFocusInterface->IsOwningPrefabBeingFocused(m_instanceMap["sportsCar"]->GetContainerEntityId()));
            EXPECT_TRUE(prefabFocusInterface->IsOwningPrefabBeingFocused(m_entityMap["passenger2"]->GetId()));
            EXPECT_FALSE(prefabFocusInterface->IsOwningPrefabBeingFocused(m_instanceMap["car"]->GetContainerEntityId()));
            EXPECT_FALSE(prefabFocusInterface->IsOwningPrefabBeingFocused(m_entityMap["passenger1"]->GetId()));
        }

        m_rootInstance.release();
    }

}
