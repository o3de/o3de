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

#include <AzCore/Component/TransformBus.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/PrefabLoaderInterface.h>
#include <Prefab/PrefabTestComponent.h>
#include <Prefab/PrefabTestDomUtils.h>

namespace UnitTest
{
    void PrefabTestFixture::SetUpEditorFixtureImpl()
    {
        // Acquire the system entity
        AZ::Entity* systemEntity = GetApplication()->FindEntity(AZ::SystemEntityId);
        EXPECT_TRUE(systemEntity);

        // Acquire the prefab system component to gain access to its APIs for testing
        m_prefabSystemComponent = systemEntity->FindComponent<AzToolsFramework::Prefab::PrefabSystemComponent>();
        EXPECT_TRUE(m_prefabSystemComponent);

        // Acquire the interface of PrefabLoader to gain access to its APIs for testing
        m_prefabLoaderInterface = AZ::Interface<AzToolsFramework::Prefab::PrefabLoaderInterface>::Get();
        EXPECT_TRUE(m_prefabLoaderInterface);

        // Acquire the interface of InstanceUpdateQueueInterface to gain access to its APIs for testing
        m_instanceUpdateExecutorInterface = AZ::Interface<AzToolsFramework::Prefab::InstanceUpdateExecutorInterface>::Get();
        EXPECT_TRUE(m_instanceUpdateExecutorInterface);

        // Acquire the interface of InstanceToTemplate to gain access to its APIs for testing
        m_instanceToTemplateInterface = AZ::Interface<AzToolsFramework::Prefab::InstanceToTemplateInterface>::Get();
        EXPECT_TRUE(m_instanceToTemplateInterface);

        GetApplication()->RegisterComponentDescriptor(PrefabTestComponent::CreateDescriptor());
    }

    AZ::Entity* PrefabTestFixture::CreateEntity(const char* entityName, const bool shouldActivate)
    {
        // Circumvent the EntityContext system and generate a new entity with a transformcomponent
        AZ::Entity* newEntity = aznew AZ::Entity(entityName);
        
        if(shouldActivate)
        {
            newEntity->Init();
            newEntity->Activate();
        }

        return newEntity;
    }

    void PrefabTestFixture::CompareInstances(const AzToolsFramework::Prefab::Instance& instanceA,
        const AzToolsFramework::Prefab::Instance& instanceB, bool shouldCompareLinkIds)
    {
        AzToolsFramework::Prefab::TemplateId templateAId = instanceA.GetTemplateId();
        AzToolsFramework::Prefab::TemplateId templateBId = instanceB.GetTemplateId();

        ASSERT_TRUE(templateAId != AzToolsFramework::Prefab::InvalidTemplateId);
        ASSERT_TRUE(templateBId != AzToolsFramework::Prefab::InvalidTemplateId);
        EXPECT_EQ(templateAId, templateBId);

        AzToolsFramework::Prefab::TemplateReference templateA =
            m_prefabSystemComponent->FindTemplate(templateAId);

        ASSERT_TRUE(templateA.has_value());

        AzToolsFramework::Prefab::PrefabDom prefabDomA;
        ASSERT_TRUE(AzToolsFramework::Prefab::PrefabDomUtils::StoreInstanceInPrefabDom(instanceA, prefabDomA));

        AzToolsFramework::Prefab::PrefabDom prefabDomB;
        ASSERT_TRUE(AzToolsFramework::Prefab::PrefabDomUtils::StoreInstanceInPrefabDom(instanceB, prefabDomB));

        // Validate that both instances match when serialized
        PrefabTestDomUtils::ComparePrefabDoms(prefabDomA, prefabDomB);

        // Validate that the serialized instances match the shared template when serialized
        PrefabTestDomUtils::ComparePrefabDoms(templateA->get().GetPrefabDom(), prefabDomB, shouldCompareLinkIds);
    }

    void PrefabTestFixture::DeleteInstances(const InstanceList& instancesToDelete)
    {
        for (Instance* instanceToDelete : instancesToDelete)
        {
            ASSERT_TRUE(instanceToDelete);
            delete instanceToDelete;
            instanceToDelete = nullptr;
        }
    }
}
