/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Spawnable/Spawnable.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/Prefab/Spawnable/AssetPlatformComponentRemover.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <Prefab/PrefabDomTypes.h>
#include <Prefab/Spawnable/PrefabProcessorContext.h>

namespace UnitTest
{
    class PrefabProcessingTestFixture : public ::testing::Test
    {

    public:
        void SetUp() override
        {
            m_application.reset(aznew AzToolsFramework::ToolsApplication());
            m_application.get()->Start(AzFramework::Application::Descriptor());

            using AzToolsFramework::Prefab::PrefabConversionUtils::PrefabProcessorContext;

            // Create a prefab
            AZStd::vector<AZ::Entity*> entities;
            entities.emplace_back(CreateSourceEntity("Entity_1", AZ::Transform::CreateIdentity()));
            entities.emplace_back(CreateSourceEntity("Entity_2", AZ::Transform::CreateIdentity()));
            ConvertEntitiesToPrefab(entities, m_prefabDom, "test/my_prefab");
        }

        void TearDown() override
        {
            AZ::Interface<AzToolsFramework::Prefab::PrefabSystemComponentInterface>::Get()->RemoveAllTemplates();

            m_application.get()->Stop();
            m_application.reset();

        }

        static void ConvertEntitiesToPrefab(const AZStd::vector<AZ::Entity*>& entities, AzToolsFramework::Prefab::PrefabDom& prefabDom, AZ::IO::PathView filePath)
        {
            auto* prefabSystem = AZ::Interface<AzToolsFramework::Prefab::PrefabSystemComponentInterface>::Get();
            AZStd::unique_ptr<AzToolsFramework::Prefab::Instance> sourceInstance(prefabSystem->CreatePrefab(entities, {}, filePath));
            ASSERT_TRUE(sourceInstance);

            auto& prefabTemplateDom = prefabSystem->FindTemplateDom(sourceInstance->GetTemplateId());
            prefabDom.CopyFrom(prefabTemplateDom, prefabDom.GetAllocator());
        }

        static AZ::Entity* CreateSourceEntity(const char* name, const AZ::Transform& tm, AZ::Entity* parent = nullptr)
        {
            AZ::Entity* entity = aznew AZ::Entity(name);
            auto* transformComponent = entity->CreateComponent<AzFramework::TransformComponent>();

            if (parent)
            {
                transformComponent->SetParent(parent->GetId());
                transformComponent->SetLocalTM(tm);
            }
            else
            {
                transformComponent->SetWorldTM(tm);
            }

            return entity;
        }

        const AZStd::string m_staticEntityName = "static_floor";
        const AZStd::string m_netEntityName = "networked_entity";

        AZStd::unique_ptr<AzToolsFramework::ToolsApplication> m_application = {};
        AzToolsFramework::Prefab::PrefabDom m_prefabDom;
    };

    TEST_F(PrefabProcessingTestFixture, PrefabProcessorRemoveComponentPerPlatform_SingleEntity)
    {
        using namespace AzToolsFramework::Prefab::PrefabConversionUtils;

        // Add the prefab into the Prefab Processor Context
        const AZStd::string prefabName = "testPrefab";
        PrefabProcessorContext prefabProcessorContext{ AZ::Uuid::CreateRandom() };
        PrefabDocument document(prefabName);
        ASSERT_TRUE(document.SetPrefabDom(AZStd::move(m_prefabDom)));
        prefabProcessorContext.AddPrefab(AZStd::move(document));

        // Request PrefabProcessor to process the prefab
        AssetPlatformComponentRemover processor;
        processor.Process(prefabProcessorContext);
        
        // Validate results
        EXPECT_TRUE(prefabProcessorContext.HasCompletedSuccessfully());
    }
} // namespace UnitTest
