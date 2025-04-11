/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
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
    constexpr AZ::Uuid Uuid_KeepThisComponent("{9218A873-1525-4278-AC07-17AD6A6B8374}");
    constexpr AZ::Uuid Uuid_DependentComponent("{95421870-F6FD-44D2-AA5F-AF85FD977F75}");
    const AZStd::set<AZ::Uuid> ExcludedComponents = { Uuid_RemoveThisComponent };

    const char* PlatformTag = "platform_1";
    const char* EntityName = "entity_1";
    const AZ::Crc32 ComponentService = AZ_CRC_CE("good_service");

    class KeepThisComponent : public AZ::Component
    {
    public:
        AZ_COMPONENT(KeepThisComponent, Uuid_KeepThisComponent);
        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<KeepThisComponent, AZ::Component>()->Version(1);
            }
        }

        KeepThisComponent() = default;
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
            }
        }

        RemoveThisComponent() = default;
        void Activate() override {}
        void Deactivate() override {}
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(ComponentService);
        }
    };

    class DependentComponent : public AZ::Component
    {
    public:
        AZ_COMPONENT(DependentComponent, Uuid_DependentComponent);
        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<DependentComponent, AZ::Component>()->Version(1);
            }
        }

        DependentComponent() = default;
        void Activate() override {}
        void Deactivate() override {}

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(ComponentService);
        }
    };

    class PrefabProcessingTestFixture
        : public LeakDetectionFixture
    {
    public:
        void SetUp() override
        {
            m_application.reset(aznew AzToolsFramework::ToolsApplication());
            m_application.get()->Start(AzFramework::Application::Descriptor());
            m_application.get()->RegisterComponentDescriptor(KeepThisComponent::CreateDescriptor());
            m_application.get()->RegisterComponentDescriptor(RemoveThisComponent::CreateDescriptor());
            m_application.get()->RegisterComponentDescriptor(DependentComponent::CreateDescriptor());

            AZStd::map<AZStd::string, AZStd::set<AZ::Uuid>> platformExcludedComponents;
            platformExcludedComponents.emplace(PlatformTag, ExcludedComponents);
            m_processor.m_platformExcludedComponents = platformExcludedComponents;
        }

        void TearDown() override
        {
            AZ::Interface<AzToolsFramework::Prefab::PrefabSystemComponentInterface>::Get()->RemoveAllTemplates();

            m_application.get()->Stop();
            m_application.reset();
        }

        static void ConvertEntitiesToPrefab(const AZStd::vector<AZ::Entity*>& entities, AzToolsFramework::Prefab::PrefabDom& prefabDom)
        {
            auto* prefabSystem = AZ::Interface<AzToolsFramework::Prefab::PrefabSystemComponentInterface>::Get();
            AZStd::unique_ptr<AzToolsFramework::Prefab::Instance> sourceInstance(prefabSystem->CreatePrefab(entities, {}, "test/my_prefab"));
            ASSERT_TRUE(sourceInstance);

            auto& prefabTemplateDom = prefabSystem->FindTemplateDom(sourceInstance->GetTemplateId());
            prefabDom.CopyFrom(prefabTemplateDom, prefabDom.GetAllocator());
        }

        static AZ::Entity* CreateSourceEntity(const char* name, AZStd::vector<AZ::Uuid> components, AZ::Entity* parent = nullptr)
        {
            AZ::Entity* entity = aznew AZ::Entity(name);
            auto* transformComponent = entity->CreateComponent<AzFramework::TransformComponent>();
            if (parent)
            {
                transformComponent->SetParent(parent->GetId());
            }

            for (auto componentUUID : components)
            {
                entity->CreateComponent(componentUUID);
            }

            entity->Init();
            entity->Activate();
            return entity;
        }

        AZStd::unique_ptr<AzToolsFramework::ToolsApplication> m_application = {};
        AzToolsFramework::Prefab::PrefabConversionUtils::AssetPlatformComponentRemover m_processor;
    };

    TEST_F(PrefabProcessingTestFixture, PrefabProcessorRemoveComponentPerPlatform_RemoveSingleComponent)
    {
        using namespace AzToolsFramework::Prefab::PrefabConversionUtils;

        // Add the prefab into the Prefab Processor Context
        PrefabProcessorContext prefabProcessorContext{ AZ::Uuid::CreateRandom() };
        prefabProcessorContext.SetPlatformTags({ AZ::Crc32(PlatformTag) });

        // Create a prefab
        PrefabDocument document("testPrefab");
        AzToolsFramework::Prefab::PrefabDom prefabDom;
        AZStd::vector<AZ::Entity*> entities;
        entities.emplace_back(CreateSourceEntity(EntityName, { Uuid_RemoveThisComponent, Uuid_KeepThisComponent }));
        ConvertEntitiesToPrefab(entities, prefabDom);

        ASSERT_TRUE(document.SetPrefabDom(AZStd::move(prefabDom)));
        prefabProcessorContext.AddPrefab(AZStd::move(document));

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
                            EXPECT_NE(entity->FindComponent(Uuid_KeepThisComponent), nullptr);
                        }
                        return true;
                    }
                );
            }
        );

        // Process Prefab
        m_processor.Process(prefabProcessorContext);
        ASSERT_TRUE(prefabProcessorContext.HasPrefabs());
        ASSERT_TRUE(prefabProcessorContext.HasCompletedSuccessfully());

        // Validate 1 component is removed
        prefabProcessorContext.ListPrefabs(
            [](PrefabDocument& prefab) -> void
            {
                prefab.GetInstance().GetAllEntitiesInHierarchy(
                    [](AZStd::unique_ptr<AZ::Entity>& entity) -> bool
                    {
                        if (entity->GetName() == EntityName)
                        {
                            EXPECT_EQ(entity->FindComponent(Uuid_RemoveThisComponent), nullptr);
                            EXPECT_NE(entity->FindComponent(Uuid_KeepThisComponent), nullptr);
                        }
                        return true;
                    }
                );
            }
        );
    }

    TEST_F(PrefabProcessingTestFixture, PrefabProcessorRemoveComponentPerPlatform_ComponentDependencyError)
    {
        using namespace AzToolsFramework::Prefab::PrefabConversionUtils;

        // Add the prefab into the Prefab Processor Context
        PrefabProcessorContext prefabProcessorContext{ AZ::Uuid::CreateRandom() };
        prefabProcessorContext.SetPlatformTags({ AZ::Crc32(PlatformTag) });

        // Create a prefab
        PrefabDocument document("testPrefab");
        AzToolsFramework::Prefab::PrefabDom prefabDom;
        AZStd::vector<AZ::Entity*> entities;
        entities.emplace_back(CreateSourceEntity(EntityName, { Uuid_RemoveThisComponent, Uuid_KeepThisComponent, Uuid_DependentComponent }));
        ConvertEntitiesToPrefab(entities, prefabDom);

        ASSERT_TRUE(document.SetPrefabDom(AZStd::move(prefabDom)));
        prefabProcessorContext.AddPrefab(AZStd::move(document));

        // Process Prefab
        AZ_TEST_START_TRACE_SUPPRESSION;
        m_processor.Process(prefabProcessorContext);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1); //< Expect 1 error due to missing a component dependency
        ASSERT_FALSE(prefabProcessorContext.HasCompletedSuccessfully());
    }
} // namespace UnitTest
