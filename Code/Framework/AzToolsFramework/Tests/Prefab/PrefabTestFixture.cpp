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
        EXPECT_TRUE(m_prefabEditorEntityOwnershipInterface);

        // This is for calling CreateEditorRepresentation that adds required editor components.
        AzToolsFramework::EditorRequestBus::Handler::BusConnect();

        GetApplication()->RegisterComponentDescriptor(PrefabTestComponent::CreateDescriptor());
        GetApplication()->RegisterComponentDescriptor(PrefabTestComponentWithUnReflectedTypeMember::CreateDescriptor());

        // Gets undo stack.
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
            m_undoStack, &AzToolsFramework::ToolsApplicationRequestBus::Events::GetUndoStack);
        AZ_Assert(m_undoStack, "Failed to look up undo stack from tools application");

        // This ensures that the flag (if root prefab is assigned) in prefab editor entity ownership service is set to true.
        // Public prefab operations like "create prefab" will fail if the flag is off.
        CreateRootPrefab();
    }

    void PrefabTestFixture::TearDownEditorFixtureImpl()
    {
        m_undoStack = nullptr;

        AzToolsFramework::EditorRequestBus::Handler::BusDisconnect();
    }

    AZStd::unique_ptr<ToolsTestApplication> PrefabTestFixture::CreateTestApplication()
    {
        return AZStd::make_unique<PrefabTestToolsApplication>("PrefabTestApplication");
    }

    void PrefabTestFixture::CreateRootPrefab()
    {
        m_prefabEditorEntityOwnershipInterface->CreateNewLevelPrefab("UnitTestRoot.prefab", "");

        InstanceOptionalReference rootInstance = m_prefabEditorEntityOwnershipInterface->GetRootPrefabInstance();
        ASSERT_TRUE(rootInstance.has_value());
        
        EntityOptionalReference rootContainerEntity = rootInstance->get().GetContainerEntity();
        ASSERT_TRUE(rootContainerEntity.has_value());
        if (rootContainerEntity->get().GetState() == AZ::Entity::State::Constructed)
        {
            rootContainerEntity->get().Init();
        }

        // Focus on root prefab instance.
        auto* prefabFocusPublicInterface = AZ::Interface<PrefabFocusPublicInterface>::Get();
        EXPECT_TRUE(prefabFocusPublicInterface != nullptr);
        PrefabFocusOperationResult focusResult = prefabFocusPublicInterface->FocusOnOwningPrefab(
            rootContainerEntity->get().GetId());
        EXPECT_TRUE(focusResult.IsSuccess());
    }

    void PrefabTestFixture::PropagateAllTemplateChanges()
    {
        m_prefabSystemComponent->OnSystemTick();
    }

    AZ::EntityId PrefabTestFixture::CreateEditorEntityUnderRoot(const AZStd::string& entityName)
    {
        return CreateEditorEntity(entityName, GetRootContainerEntityId());
    }

    AZ::EntityId PrefabTestFixture::CreateEditorEntity(const AZStd::string& entityName, AZ::EntityId parentId)
    {
        PrefabEntityResult createResult = m_prefabPublicInterface->CreateEntity(parentId, AZ::Vector3());
        AZ_Assert(createResult.IsSuccess(), "CreateEditorEntity - Failed to create entity %s. Error: %s",
            entityName.c_str(), createResult.GetError().c_str());

        // Verify new entity.
        AZ::EntityId newEntityId = createResult.GetValue();
        EXPECT_TRUE(newEntityId.IsValid());

        AZ::Entity* newEntity = AzToolsFramework::GetEntityById(newEntityId);
        EXPECT_TRUE(newEntity != nullptr);

        newEntity->SetName(entityName);
        m_prefabPublicInterface->GenerateUndoNodesForEntityChangeAndUpdateCache(newEntityId, m_undoStack->GetTop());

        PropagateAllTemplateChanges();

        return newEntityId;
    }

    AZ::EntityId PrefabTestFixture::CreateEditorPrefab(AZ::IO::PathView filePath, const AzToolsFramework::EntityIdList& entityIds)
    {
        // New prefab instance is reparent under the common root entity of the input entities.
        CreatePrefabResult createResult = m_prefabPublicInterface->CreatePrefabInMemory(entityIds, filePath);
        AZ_Assert(createResult.IsSuccess(), "CreateEditorPrefab - Failed to create prefab %s. Error: %s",
            AZStd::string(filePath.Native()).c_str(),
            createResult.GetError().c_str());

        // Verify new container entity.
        AZ::EntityId prefabContainerId = createResult.GetValue();
        EXPECT_TRUE(prefabContainerId.IsValid());

        AZ::Entity* prefabContainerEntity = AzToolsFramework::GetEntityById(prefabContainerId);
        EXPECT_TRUE(prefabContainerEntity != nullptr);

        PropagateAllTemplateChanges();

        return prefabContainerId;
    }

    AZ::EntityId PrefabTestFixture::InstantiateEditorPrefab(AZ::IO::PathView filePath, AZ::EntityId parentId)
    {
        InstantiatePrefabResult instantiateResult = m_prefabPublicInterface->InstantiatePrefab(
            filePath.Native(), parentId, AZ::Vector3());
        AZ_Assert(instantiateResult.IsSuccess(), "InstantiateEditorPrefab - Failed to instantiate prefab %s. Error: %s",
            AZStd::string(filePath.Native()).c_str(),
            instantiateResult.GetError().c_str());

        // Verify new container entity.
        AZ::EntityId prefabContainerId = instantiateResult.GetValue();
        EXPECT_TRUE(prefabContainerId.IsValid());

        AZ::Entity* prefabContainerEntity = AzToolsFramework::GetEntityById(prefabContainerId);
        EXPECT_TRUE(prefabContainerEntity != nullptr);
        
        PropagateAllTemplateChanges();

        return prefabContainerId;
    }

    AZ::Entity* PrefabTestFixture::CreateEntity(const AZStd::string& entityName, bool shouldActivate)
    {
        AZ::Entity* newEntity = aznew AZ::Entity(entityName);
        
        if (shouldActivate)
        {
            newEntity->Init();
            newEntity->Activate();
        }

        return newEntity;
    }

    AZ::EntityId PrefabTestFixture::GetRootContainerEntityId()
    {
        auto rootInstance = m_prefabEditorEntityOwnershipInterface->GetRootPrefabInstance();
        EXPECT_TRUE(rootInstance.has_value());
        AZ::EntityId rootContainerId = rootInstance->get().GetContainerEntityId();
        EXPECT_TRUE(rootContainerId.IsValid());
        return rootContainerId;
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
        instance.GetAllEntityIdsInHierarchy([&entityIdVector](AZ::EntityId entityId) {
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

    void PrefabTestFixture::AddRequiredEditorComponents(const AzToolsFramework::EntityIdList& entityIds)
    {
        for (AZ::EntityId entityId : entityIds)
        {
            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, entityId);
            EXPECT_TRUE(entity != nullptr) << "The entity to be added required editor components is nullptr.";

            entity->Deactivate();
            AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
                &AzToolsFramework::EditorEntityContextRequests::AddRequiredComponents, *entity);
            entity->Activate();
        }
    }

    // EditorRequestBus
    void PrefabTestFixture::CreateEditorRepresentation(AZ::Entity* entity)
    {
        if (!entity)
        {
            EXPECT_TRUE(false) << "Cannot call CreateEditorRepresentation for a null entity.";
            return;
        }

        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
            &AzToolsFramework::EditorEntityContextRequestBus::Events::AddRequiredComponents, *entity);
    }
} // namespace UnitTest
