/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Prefab/SpawnableRemoveEditorInfoTestFixture.h>

#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/ToolsComponents/EditorOnlyEntityComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorOnlyEntityComponentBus.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>

namespace UnitTest
{
    TestExportRuntimeComponentWithCallback::TestExportRuntimeComponentWithCallback(bool returnPointerToSelf, bool exportHandled)
        : m_returnPointerToSelf(returnPointerToSelf)
        , m_exportHandled(exportHandled)
    {}

    void TestExportRuntimeComponentWithCallback::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<TestExportRuntimeComponentWithCallback, AZ::Component>()->Version(1)
                ->Field("ExportHandled", &TestExportRuntimeComponentWithCallback::m_exportHandled)
                ->Field("ReturnPointerToSelf", &TestExportRuntimeComponentWithCallback::m_returnPointerToSelf);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<TestExportRuntimeComponentWithCallback>(
                    "Test Export Runtime Component", "Validate different options for exporting runtime components")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::RuntimeExportCallback, &TestExportRuntimeComponentWithCallback::ExportComponent)
                    ;
            }
        }
    }

    AZ::ExportedComponent TestExportRuntimeComponentWithCallback::ExportComponent(AZ::Component* thisComponent, const AZ::PlatformTagSet& /*platformTags*/)
    {
        return AZ::ExportedComponent(m_returnPointerToSelf? thisComponent : nullptr, false, m_exportHandled);
    }

    void TestExportRuntimeComponentWithoutCallback::Reflect(AZ::ReflectContext* context)
    {
        auto* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<TestExportRuntimeComponentWithoutCallback, AZ::Component>()
                ;
        }
    }

    TestExportEditorComponent::TestExportEditorComponent(
        TestExportEditorComponent::ExportComponentType exportType, bool exportHandled)
        : m_exportType(exportType)
        , m_exportHandled(exportHandled)
    {}

    void TestExportEditorComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<TestExportEditorComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Version(1)
                ->Field("ExportHandled", &TestExportEditorComponent::m_exportHandled)
                ->Field("ExportType", &TestExportEditorComponent::m_exportType)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<TestExportEditorComponent>(
                    "Test Export Editor Component", "Validate different options for exporting editor components")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::RuntimeExportCallback, &TestExportEditorComponent::ExportComponent)
                    ;
            }
        }
    }

    AZ::ExportedComponent TestExportEditorComponent::ExportComponent(
        AZ::Component* thisComponent, const AZ::PlatformTagSet& /*platformTags*/)
    {
        switch (m_exportType)
        {
        case ExportComponentType::ExportEditorComponent:
            return AZ::ExportedComponent(thisComponent, false, m_exportHandled);
        case ExportComponentType::ExportRuntimeComponentWithCallBack:
            return AZ::ExportedComponent(aznew TestExportRuntimeComponentWithCallback(true, true), true, m_exportHandled);
        case ExportComponentType::ExportRuntimeComponentWithoutCallBack:
            return AZ::ExportedComponent(aznew TestExportRuntimeComponentWithoutCallback, true, m_exportHandled);
        case ExportComponentType::ExportNullComponent:
            return AZ::ExportedComponent(nullptr, false, m_exportHandled);
        default:
            return AZ::ExportedComponent();
        }
    }

    void TestExportEditorComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<TestExportRuntimeComponentWithCallback>(true, true);
    }

    void SpawnableRemoveEditorInfoTestFixture::SetUpEditorFixtureImpl()
    {
        auto* app = GetApplication();

        // Acquire the system entity
        AZ::Entity* systemEntity = app->FindEntity(AZ::SystemEntityId);
        EXPECT_TRUE(systemEntity);

        // Acquire the prefab system component to gain access to its APIs for testing
        m_prefabSystemComponent = systemEntity->FindComponent<AzToolsFramework::Prefab::PrefabSystemComponent>();
        EXPECT_TRUE(m_prefabSystemComponent);

        m_serializeContext = app->GetSerializeContext();
        EXPECT_TRUE(m_serializeContext);

        app->RegisterComponentDescriptor(TestExportRuntimeComponentWithCallback::CreateDescriptor());
        app->RegisterComponentDescriptor(TestExportRuntimeComponentWithoutCallback::CreateDescriptor());
        app->RegisterComponentDescriptor(TestExportEditorComponent::CreateDescriptor());
    }

    void SpawnableRemoveEditorInfoTestFixture::TearDownEditorFixtureImpl()
    {
        for (AZ::Entity* entity : m_runtimeEntities)
        {
            delete entity;
        }

        m_sourceEntities.clear();
        m_runtimeEntities.clear();
    }

    void SpawnableRemoveEditorInfoTestFixture::CreateSourceEntity(const char* name, bool editorOnly)
    {
        AZ::Entity* entity = aznew AZ::Entity(name);
        entity->CreateComponent<AzToolsFramework::Components::TransformComponent>();
        entity->CreateComponent<AzToolsFramework::Components::EditorOnlyEntityComponent>();
        m_sourceEntities.push_back(entity);

        entity->Init();
        EXPECT_EQ(AZ::Entity::State::Init, entity->GetState());
        entity->Activate();
        EXPECT_EQ(AZ::Entity::State::Active, entity->GetState());

        AzToolsFramework::EditorOnlyEntityComponentRequestBus::Event(
            entity->GetId(),
            &AzToolsFramework::EditorOnlyEntityComponentRequests::SetIsEditorOnlyEntity,
            editorOnly);
    }

    void SpawnableRemoveEditorInfoTestFixture::CreateSourceTestExportRuntimeEntity(
        const char* name, bool returnPointerToSelf, bool exportHandled)
    {
        AZ::Entity* entity = aznew AZ::Entity(name);
        entity->CreateComponent<AzToolsFramework::Components::TransformComponent>();
        entity->CreateComponent<TestExportRuntimeComponentWithCallback>(returnPointerToSelf, exportHandled);
        m_sourceEntities.push_back(entity);
    }

    void SpawnableRemoveEditorInfoTestFixture::CreateSourceTestExportEditorEntity(
        const char* name,
        TestExportEditorComponent::ExportComponentType exportType,
        bool exportHandled)
    {
        AZ::Entity* entity = aznew AZ::Entity(name);
        entity->CreateComponent<AzToolsFramework::Components::TransformComponent>();
        entity->CreateComponent<TestExportEditorComponent>(exportType, exportHandled);
        m_sourceEntities.push_back(entity);
    }

    AZ::Entity* SpawnableRemoveEditorInfoTestFixture::GetRuntimeEntity(const char* entityName)
    {
        for (AZ::Entity* entity : m_runtimeEntities)
        {
            const AZStd::string& name = entity->GetName();
            if (name == entityName)
            {
                return entity;
            }
        }

        return nullptr;
    }

    void SpawnableRemoveEditorInfoTestFixture::ConvertSourceEntitiesToPrefab()
    {
        AZStd::unique_ptr<AzToolsFramework::Prefab::Instance> sourceInstance(
            m_prefabSystemComponent->CreatePrefab(m_sourceEntities, {}, "test/path"));
        ASSERT_TRUE(sourceInstance);

        auto& prefabTemplateDom = m_prefabSystemComponent->FindTemplateDom(sourceInstance->GetTemplateId());
        m_prefabDom.CopyFrom(prefabTemplateDom, m_prefabDom.GetAllocator());
    }

    void SpawnableRemoveEditorInfoTestFixture::ConvertRuntimePrefab(bool expectedResult)
    {
        ConvertSourceEntitiesToPrefab();

        const bool actualResult =
            m_editorInfoRemover.RemoveEditorInfo(m_prefabDom, m_serializeContext, m_prefabProcessorContext).IsSuccess();

        EXPECT_EQ(expectedResult, actualResult);

        AZStd::unique_ptr<Instance> convertedInstance(aznew Instance());
        ASSERT_TRUE(AzToolsFramework::Prefab::PrefabDomUtils::LoadInstanceFromPrefabDom(*convertedInstance, m_prefabDom));

        convertedInstance->DetachAllEntitiesInHierarchy(
            [this](AZStd::unique_ptr<AZ::Entity> entity)
            {
                m_runtimeEntities.emplace_back(entity.release());
            }
        );
    }
}
