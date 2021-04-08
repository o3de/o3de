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

#include <Prefab/PrefabTestFixture.h>

namespace UnitTest
{
    using PrefabInstantiateTest = PrefabTestFixture;

    TEST_F(PrefabInstantiateTest, PrefabInstantiate_InstantiateInvalidTemplate_InstantiateFails)
    {
        EXPECT_FALSE(m_prefabSystemComponent->InstantiatePrefab(AzToolsFramework::Prefab::InvalidTemplateId));
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

    TEST_F(PrefabInstantiateTest, PrefabInstantiate_TripleNestingTemplate_InstantiateSucceeds)
    {
        AZ::Entity* newEntity = CreateEntity("New Entity");
        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
            &AzToolsFramework::EditorEntityContextRequests::HandleEntitiesAdded, AzToolsFramework::EntityList{newEntity});
        // Build a 3 level deep nested Template
        AZStd::unique_ptr<AzToolsFramework::Prefab::Instance> firstInstance = m_prefabSystemComponent->CreatePrefab({ newEntity }, {}, "test/path1");
        ASSERT_TRUE(firstInstance);

        AZStd::unique_ptr<AzToolsFramework::Prefab::Instance> secondInstance = m_prefabSystemComponent->CreatePrefab({},
            MakeInstanceList( AZStd::move(firstInstance) ), "test/path2");
        ASSERT_TRUE(secondInstance);

        AZStd::unique_ptr<AzToolsFramework::Prefab::Instance> thirdInstance = m_prefabSystemComponent->CreatePrefab({},
            MakeInstanceList( AZStd::move(secondInstance) ), "test/path3");
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
