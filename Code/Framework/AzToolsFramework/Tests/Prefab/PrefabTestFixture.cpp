/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Prefab/PrefabTestFixture.h>

#include <AzCore/Component/TransformBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Entity/PrefabEditorEntityOwnershipInterface.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityMapperInterface.h>
#include <AzToolsFramework/Prefab/PrefabLoaderInterface.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <Prefab/PrefabTestComponent.h>
#include <Prefab/PrefabTestDomUtils.h>

namespace UnitTest
{
    PrefabTestToolsApplication::PrefabTestToolsApplication(AZStd::string appName)
        : ToolsTestApplication(AZStd::move(appName))
    {
    }

    bool PrefabTestToolsApplication::IsPrefabSystemEnabled() const
    {
        // Make sure our prefab tests always run with prefabs enabled
        return true;
    }

    void PrefabTestFixture::SetUpEditorFixtureImpl()
    {
        // Acquire the system entity
        AZ::Entity* systemEntity = GetApplication()->FindEntity(AZ::SystemEntityId);
        EXPECT_TRUE(systemEntity);

        m_prefabSystemComponent = systemEntity->FindComponent<AzToolsFramework::Prefab::PrefabSystemComponent>();
        EXPECT_TRUE(m_prefabSystemComponent);

        m_prefabLoaderInterface = AZ::Interface<PrefabLoaderInterface>::Get();
        EXPECT_TRUE(m_prefabLoaderInterface);

        m_prefabPublicInterface = AZ::Interface<PrefabPublicInterface>::Get();
        EXPECT_TRUE(m_prefabPublicInterface);

        m_instanceEntityMapperInterface = AZ::Interface<InstanceEntityMapperInterface>::Get();
        EXPECT_TRUE(m_instanceEntityMapperInterface);

        m_instanceUpdateExecutorInterface = AZ::Interface<InstanceUpdateExecutorInterface>::Get();
        EXPECT_TRUE(m_instanceUpdateExecutorInterface);

        m_instanceToTemplateInterface = AZ::Interface<InstanceToTemplateInterface>::Get();
        EXPECT_TRUE(m_instanceToTemplateInterface);

        m_prefabEditorEntityOwnershipInterface = AZ::Interface<AzToolsFramework::PrefabEditorEntityOwnershipInterface>::Get();
        EXPECT_TRUE(m_instanceToTemplateInterface);

        InitializeRootPrefab();

        GetApplication()->RegisterComponentDescriptor(PrefabTestComponent::CreateDescriptor());
        GetApplication()->RegisterComponentDescriptor(PrefabTestComponentWithUnReflectedTypeMember::CreateDescriptor());

        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
            m_undoStack, &AzToolsFramework::ToolsApplicationRequestBus::Events::GetUndoStack);
        AZ_Assert(m_undoStack, "Failed to look up undo stack from tools application");
    }

    void PrefabTestFixture::TearDownEditorFixtureImpl()
    {
        m_undoStack = nullptr;
    }

    AZStd::unique_ptr<ToolsTestApplication> PrefabTestFixture::CreateTestApplication()
    {
        return AZStd::make_unique<PrefabTestToolsApplication>("PrefabTestApplication");
    }

    void PrefabTestFixture::CreateRootPrefab()
    {
        auto entityOwnershipService = AZ::Interface<AzToolsFramework::PrefabEditorEntityOwnershipInterface>::Get();
        ASSERT_TRUE(entityOwnershipService != nullptr);
        entityOwnershipService->CreateNewLevelPrefab("UnitTestRoot.prefab", "");

        InitializeRootPrefab();
        
        auto rootEntityReference = entityOwnershipService->GetRootPrefabInstance()->get().GetContainerEntity();
        ASSERT_TRUE(rootEntityReference.has_value());
        auto& rootEntity = rootEntityReference->get();
        rootEntity.Deactivate();
        rootEntity.CreateComponent<AzToolsFramework::Components::TransformComponent>();
        rootEntity.Activate();
    }

    void PrefabTestFixture::InitializeRootPrefab()
    {
        InstanceOptionalReference rootInstance = m_prefabEditorEntityOwnershipInterface->GetRootPrefabInstance();
        if (rootInstance.has_value())
        {
            EntityOptionalReference rootContainerEntity = rootInstance->get().GetContainerEntity();
            if (rootContainerEntity.has_value() && rootContainerEntity->get().GetState() == AZ::Entity::State::Constructed)
            {
                rootContainerEntity->get().Init();
            }

            auto* prefabFocusPublicInterface = AZ::Interface<PrefabFocusPublicInterface>::Get();
            ASSERT_TRUE(prefabFocusPublicInterface != nullptr);
            prefabFocusPublicInterface->FocusOnOwningPrefab(rootContainerEntity->get().GetId());
        }
    }

    void PrefabTestFixture::PropagateAllTemplateChanges()
    {
        m_prefabSystemComponent->OnSystemTick();
    }

    AZ::Entity* PrefabTestFixture::CreateEntity(const AZStd::string& entityName, bool shouldActivate)
    {
        // Circumvent the EntityContext system and generate a new entity with a transform component
        AZ::Entity* newEntity = aznew AZ::Entity(entityName);
        
        if(shouldActivate)
        {
            newEntity->Init();
            newEntity->Activate();
        }

        return newEntity;
    }

    AZ::EntityId PrefabTestFixture::CreateEntityUnderRootPrefab(AZStd::string name, AZ::EntityId parentId)
    {
        auto createResult = m_prefabPublicInterface->CreateEntity(parentId, AZ::Vector3());
        AZ_Assert(createResult.IsSuccess(), "Failed to create entity: %s", createResult.GetError().c_str());
        AZ::EntityId entityId = createResult.GetValue();

        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entityId);

        entity->Deactivate();

        entity->SetName(name);

        // Normally, in invalid parent ID should automatically parent us to the root prefab, but currently in the unit test
        // environment entities aren't created with a default transform component, so CreateEntity won't correctly parent.
        // We get the actual target parent ID here, then create our missing transform component.
        if (!parentId.IsValid())
        {
            auto prefabEditorEntityOwnershipInterface = AZ::Interface<AzToolsFramework::PrefabEditorEntityOwnershipInterface>::Get();
            parentId = prefabEditorEntityOwnershipInterface->GetRootPrefabInstance()->get().GetContainerEntityId();
        }

        auto transform = aznew AzToolsFramework::Components::TransformComponent;
        transform->SetParent(parentId);
        entity->AddComponent(transform);

        entity->Activate();

        // Update our undo cache entry to include the rename / reparent as one atomic operation.
        m_prefabPublicInterface->GenerateUndoNodesForEntityChangeAndUpdateCache(entityId, m_undoStack->GetTop());
        m_prefabSystemComponent->OnSystemTick();

        return entityId;
    }

    AZ::EntityId PrefabTestFixture::CreatePrefab(const AzToolsFramework::EntityIdList& entityIds, AZ::IO::PathView filePath)
    {
        CreatePrefabResult createPrefabResult = m_prefabPublicInterface->CreatePrefabInMemory(entityIds, filePath);

        // Verify that a valid prefab container entity is created.
        AZ::EntityId prefabContainerId = createPrefabResult.GetValue();
        AZ_Assert(prefabContainerId.IsValid(), "CreatePrefab operation resulted in an invalid container entity id.");
        AZ::Entity* prefabContainerEntity = AzToolsFramework::GetEntityById(prefabContainerId);
        AZ_Assert(prefabContainerEntity != nullptr, "ContainerEntity created with CreatePrefab() is a nullptr.");

        // PrefabTestFixture won't add required editor components by default. Hence we add them here.
        AddRequiredEditorComponents(prefabContainerEntity);

        return prefabContainerId;
    }

    void PrefabTestFixture::CompareInstances(const AzToolsFramework::Prefab::Instance& instanceA,
                                             const AzToolsFramework::Prefab::Instance& instanceB, bool shouldCompareLinkIds, bool shouldCompareContainerEntities)
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
        PrefabTestDomUtils::ComparePrefabDoms(prefabDomA, prefabDomB, true, shouldCompareContainerEntities);

        // Validate that the serialized instances match the shared template when serialized
        PrefabTestDomUtils::ComparePrefabDoms(templateA->get().GetPrefabDom(), prefabDomB, shouldCompareLinkIds,
            shouldCompareContainerEntities);
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

    void PrefabTestFixture::ValidateInstanceEntitiesActive(Instance& instance)
    {
        AZStd::vector<AZ::EntityId> entityIdVector;
        instance.GetNestedEntityIds([&entityIdVector](AZ::EntityId entityId) {
            entityIdVector.push_back(entityId);
            return true;
        });
        for (AZ::EntityId entityId : entityIdVector)
        {
            AZ::Entity* entityInInstance = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entityInInstance, &AZ::ComponentApplicationBus::Events::FindEntity, entityId);
            ASSERT_TRUE(entityInInstance);
            EXPECT_EQ(entityInInstance->GetState(), AZ::Entity::State::Active);
        }
    }

    void PrefabTestFixture::ProcessDeferredUpdates()
    {
        // Force a prefab propagation for updates that are deferred to the next tick.
        m_prefabSystemComponent->OnSystemTick();
    }

    void PrefabTestFixture::Undo()
    {
        m_undoStack->Undo();
        ProcessDeferredUpdates();
    }

    void PrefabTestFixture::Redo()
    {
        m_undoStack->Redo();
        ProcessDeferredUpdates();
    }

    void PrefabTestFixture::AddRequiredEditorComponents(AZ::Entity* entity)
    {
        ASSERT_TRUE(entity != nullptr);
        entity->Deactivate();
        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
            &AzToolsFramework::EditorEntityContextRequests::AddRequiredComponents, *entity);
        entity->Activate();
    }
}
