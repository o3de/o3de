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
    constexpr AZ::Uuid Uuid_RemoveThisComponent("{6E29CD1C-D2CF-4763-80E1-F45FFA439A6A}");
    constexpr AZ::Uuid Uuid_ServerComponent("{9218A873-1525-4278-AC07-17AD6A6B8374}");
    const char* PlatformTag = "platform_1";
    const char* EntityName = "Entity1";

    class ServerComponent : public AZ::Component
    {
    public:
        AZ_COMPONENT(ServerComponent, Uuid_ServerComponent);
        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<ServerComponent, AZ::Component>()->Version(1);
            }
        }

        ServerComponent() = default;
        void Activate() override {}
        void Deactivate() override {}
    };


    class RemoveThisComponent : public AZ::Component
    {
    public:
        AZ_COMPONENT(RemoveThisComponent, Uuid_RemoveThisComponent);
        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<RemoveThisComponent, AZ::Component>()->Version(1);
            }        }

        RemoveThisComponent() = default;
        void Activate() override {}
        void Deactivate() override {}
    };


    class PrefabProcessingTestFixture : public ::testing::Test
    {
    public:
        void SetUp() override
        {
            m_application.reset(aznew AzToolsFramework::ToolsApplication());
            m_application.get()->Start(AzFramework::Application::Descriptor());
            m_application.get()->RegisterComponentDescriptor(ServerComponent::CreateDescriptor());
            m_application.get()->RegisterComponentDescriptor(RemoveThisComponent::CreateDescriptor());


            using AzToolsFramework::Prefab::PrefabConversionUtils::PrefabProcessorContext;

            // Create a prefab
            AZStd::vector<AZ::Entity*> entities;
            entities.emplace_back(CreateSourceEntity(EntityName, AZ::Transform::CreateIdentity()));
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

            entity->CreateComponent<RemoveThisComponent>();
            entity->CreateComponent<ServerComponent>();
            entity->Init();
            entity->Activate();
            return entity;
        }

        AZStd::unique_ptr<AzToolsFramework::ToolsApplication> m_application = {};
        AzToolsFramework::Prefab::PrefabDom m_prefabDom;
    };

    TEST_F(PrefabProcessingTestFixture, PrefabProcessorRemoveComponentPerPlatform_SingleEntity)
    {
        using namespace AzToolsFramework::Prefab::PrefabConversionUtils;

        // Add the prefab into the Prefab Processor Context
        const AZStd::string prefabName = "testPrefab";
        PrefabProcessorContext prefabProcessorContext{ AZ::Uuid::CreateRandom() };
        prefabProcessorContext.SetPlatformTags({ AZ::Crc32(PlatformTag) });

        PrefabDocument document(prefabName);
        ASSERT_TRUE(document.SetPrefabDom(AZStd::move(m_prefabDom)));
        prefabProcessorContext.AddPrefab(AZStd::move(document));

        // Request PrefabProcessor to process the prefab
        AssetPlatformComponentRemover processor;
        AZStd::map<AZStd::string, AZStd::set<AZ::Uuid>> platformExcludedComponents;
        AZStd::set<AZ::Uuid> excludedComponents = { Uuid_RemoveThisComponent };
        platformExcludedComponents.emplace(PlatformTag, excludedComponents);

        // Validate the component exists before processing and removed afterward
        prefabProcessorContext.ListPrefabs(
            [](PrefabDocument& prefab) -> void
            {
                prefab.GetInstance().GetAllEntitiesInHierarchy(
                    [](AZStd::unique_ptr<AZ::Entity>& entity) -> bool
                    {
                        if (entity->GetName() == EntityName)
                        {
                            EXPECT_NE(entity->FindComponent(Uuid_RemoveThisComponent), nullptr);
                            EXPECT_NE(entity->FindComponent(Uuid_ServerComponent), nullptr);
                        }
                        return true;
                    }
                );
            }
        );

        // Process Prefab
        processor.m_platformExcludedComponents = platformExcludedComponents;
        processor.Process(prefabProcessorContext);
        ASSERT_TRUE(prefabProcessorContext.HasPrefabs());

        // Validate the component is removed
        prefabProcessorContext.ListPrefabs(
            [](PrefabDocument& prefab) -> void
            {
                prefab.GetInstance().GetAllEntitiesInHierarchy(
                    [](AZStd::unique_ptr<AZ::Entity>& entity) -> bool
                    {
                        if (entity->GetName() == EntityName)
                        {
                            EXPECT_EQ(entity->FindComponent(Uuid_RemoveThisComponent), nullptr);
                            EXPECT_NE(entity->FindComponent(Uuid_ServerComponent), nullptr);
                        }
                        return true;
                    }
                );
            }
        );
    }
} // namespace UnitTest
