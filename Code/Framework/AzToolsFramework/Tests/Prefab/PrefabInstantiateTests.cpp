/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Prefab/PrefabTestFixture.h>

namespace UnitTest
{
    using PrefabInstantiateTest = PrefabTestFixture;

    TEST_F(PrefabInstantiateTest, PrefabInstantiate_InstantiateInvalidTemplate_InstantiateFails)
    {
        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(m_prefabSystemComponent->InstantiatePrefab(AzToolsFramework::Prefab::InvalidTemplateId));
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(PrefabInstantiateTest, PrefabInstantiate_NoNestingTemplate_InstantiateSucceeds)
    {
        AZ::Entity* newEntity = CreateEntity("New Entity");
        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
            &AzToolsFramework::EditorEntityContextRequests::HandleEntitiesAdded, AzToolsFramework::EntityList{newEntity});
        AZStd::unique_ptr<AzToolsFramework::Prefab::Instance> firstInstance = m_prefabSystemComponent->CreatePrefab({ newEntity }, {}, "test/path");
        ASSERT_TRUE(firstInstance);

        AZStd::unique_ptr<AzToolsFramework::Prefab::Instance> secondInstance = m_prefabSystemComponent->InstantiatePrefab(firstInstance->GetTemplateId());
        ASSERT_TRUE(secondInstance);

        CompareInstances(*firstInstance, *secondInstance, true, false);
    }

    // TODO: Issue #3398 will re-enable
    TEST_F(PrefabInstantiateTest, DISABLED_PrefabInstantiate_TripleNestingTemplate_InstantiateSucceeds)
    {
        AZ::Entity* newEntity = CreateEntity("New Entity");
        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
            &AzToolsFramework::EditorEntityContextRequests::HandleEntitiesAdded, AzToolsFramework::EntityList{newEntity});
        // Build a 3 level deep nested Template
        AZStd::unique_ptr<AzToolsFramework::Prefab::Instance> firstInstance = m_prefabSystemComponent->CreatePrefab({ newEntity }, {}, "test/path1");
        ASSERT_TRUE(firstInstance);

        AZStd::unique_ptr<AzToolsFramework::Prefab::Instance> secondInstance = m_prefabSystemComponent->CreatePrefab({},
            MakeInstanceList(AZStd::move(firstInstance)), "test/path2");
        ASSERT_TRUE(secondInstance);

        AZStd::unique_ptr<AzToolsFramework::Prefab::Instance> thirdInstance = m_prefabSystemComponent->CreatePrefab({},
            MakeInstanceList(AZStd::move(secondInstance)), "test/path3");
        ASSERT_TRUE(thirdInstance);

        //Instantiate it
        AZStd::unique_ptr<AzToolsFramework::Prefab::Instance> fourthInstance =
            m_prefabSystemComponent->InstantiatePrefab(thirdInstance->GetTemplateId());

        ASSERT_TRUE(fourthInstance);

        CompareInstances(*thirdInstance, *fourthInstance, true, false);
    }

    TEST_F(PrefabInstantiateTest, PrefabInstantiate_Instantiate10Times_InstantiatesSucceed)
    {
        AZ::Entity* newEntity = CreateEntity("New Entity");
        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
            &AzToolsFramework::EditorEntityContextRequests::HandleEntitiesAdded, AzToolsFramework::EntityList{newEntity});
        AZStd::unique_ptr<AzToolsFramework::Prefab::Instance> firstInstance = m_prefabSystemComponent->CreatePrefab({ newEntity }, {}, "test/path");

        // Store the generated instances so that the unique_ptrs are destroyed at the end of the test
        // This allows us to have all the instances around at the same time
        AZStd::vector<AZStd::unique_ptr<AzToolsFramework::Prefab::Instance>> newInstances;

        for (int instanceCount = 0; instanceCount < 10; ++instanceCount)
        {
            AZStd::unique_ptr<AzToolsFramework::Prefab::Instance>
                newInstance(m_prefabSystemComponent->InstantiatePrefab(firstInstance->GetTemplateId()));

            ASSERT_TRUE(newInstance);

            CompareInstances(*firstInstance, *newInstance, true, false);

            newInstances.push_back(AZStd::move(newInstance));
        }
    }
}
