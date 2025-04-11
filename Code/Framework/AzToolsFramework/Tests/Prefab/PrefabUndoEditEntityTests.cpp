/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Prefab/PrefabTestFixture.h>

#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Prefab/Undo/PrefabUndo.h>
#include <AzToolsFramework/Prefab/Undo/PrefabUndoEntityOverrides.h>
#include <Prefab/PrefabTestComponent.h>

namespace UnitTest
{
    using PrefabUndoEditEntityTests = PrefabTestFixture;

    TEST_F(PrefabUndoEditEntityTests, EditEntity)
    {
        const AZStd::string wheelEntityName = "Wheel";

        AZ::IO::Path engineRootPath;
        m_settingsRegistryInterface->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);

        AZ::EntityId wheelEntityId = CreateEditorEntityUnderRoot(wheelEntityName);

        // Modify the transform component
        AZ::TransformBus::Event(wheelEntityId, &AZ::TransformInterface::SetWorldX, 10.0f);

        // Add a new comopnent
        AZ::Entity* wheelEntity = AzToolsFramework::GetEntityById(wheelEntityId);
        wheelEntity->Deactivate();
        wheelEntity->AddComponent(aznew PrefabTestComponent());
        wheelEntity->Activate();

        // Get after-state DOM value
        wheelEntity = AzToolsFramework::GetEntityById(wheelEntityId);
        EXPECT_TRUE(wheelEntity) << "Could not get entity object.";
        PrefabDom entityDomAfterEdit;
        m_instanceToTemplateInterface->GenerateEntityDomBySerializing(entityDomAfterEdit, *wheelEntity);
        EXPECT_TRUE(entityDomAfterEdit.IsObject()) << "Could not create after-state entity DOM.";

        // Get before-state DOM value
        AZStd::string entityAliasPath = m_instanceToTemplateInterface->GenerateEntityAliasPath(wheelEntityId);
        EXPECT_FALSE(entityAliasPath.empty());

        InstanceOptionalReference owningInstance = m_instanceEntityMapperInterface->FindOwningInstance(wheelEntityId);
        TemplateId templateId = owningInstance->get().GetTemplateId();
        EXPECT_TRUE(templateId != InvalidTemplateId);
        PrefabDom& templateDom = m_prefabSystemComponent->FindTemplateDom(templateId);
        PrefabDomValue* entityDomInTemplate = PrefabDomPath(entityAliasPath.c_str()).Get(templateDom);
        EXPECT_TRUE(entityDomInTemplate) << "Could not retrieve entity DOM from template.";

        // Create an undo node
        PrefabUndoEntityUpdate undoNode("Undo Editing Entity");
        undoNode.Capture(*entityDomInTemplate, entityDomAfterEdit, wheelEntityId);

        // Redo
        undoNode.Redo();
        PropagateAllTemplateChanges();

        // Note for the following code and all other code in this file, 
        // PropagateAllTemplateChanges() may delete and re-create entities, it is not safe
        // to hold onto entity* pointers across this call.  You must fetch them by ID again.

        wheelEntity = AzToolsFramework::GetEntityById(wheelEntityId);
        ASSERT_FLOAT_EQ(10.0f, wheelEntity->GetTransform()->GetWorldX());
        ASSERT_TRUE(wheelEntity->FindComponent<PrefabTestComponent>());

        // Undo
        undoNode.Undo();
        PropagateAllTemplateChanges();

        wheelEntity = AzToolsFramework::GetEntityById(wheelEntityId);
        ASSERT_FLOAT_EQ(0.0f, wheelEntity->GetTransform()->GetWorldX());
        ASSERT_FALSE(wheelEntity->FindComponent<PrefabTestComponent>());

        // Redo
        undoNode.Redo();
        PropagateAllTemplateChanges();

        wheelEntity = AzToolsFramework::GetEntityById(wheelEntityId);
        ASSERT_FLOAT_EQ(10.0f, wheelEntity->GetTransform()->GetWorldX());
        ASSERT_TRUE(wheelEntity->FindComponent<PrefabTestComponent>());
    }

    TEST_F(PrefabUndoEditEntityTests, EditEntityAsOverride)
    {
        PrefabOverridePublicInterface* overrideInterface = AZ::Interface<PrefabOverridePublicInterface>::Get();
        EXPECT_TRUE(overrideInterface) << "Could not get the override public interface.";

        const AZStd::string carPrefabName = "Car";
        const AZStd::string wheelEntityName = "Wheel";

        AZ::IO::Path engineRootPath;
        m_settingsRegistryInterface->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
        AZ::IO::Path carPrefabFilepath = engineRootPath / carPrefabName;

        AZ::EntityId wheelEntityId = CreateEditorEntityUnderRoot(wheelEntityName);
        AZ::EntityId carContainerId = CreateEditorPrefab(carPrefabFilepath, { wheelEntityId });

        EntityAlias wheelEntityAlias = FindEntityAliasInInstance(carContainerId, wheelEntityName);
        EXPECT_FALSE(wheelEntityAlias.empty());

        InstanceOptionalReference levelRootInstance = m_instanceEntityMapperInterface->FindOwningInstance(GetRootContainerEntityId());
        EXPECT_TRUE(levelRootInstance.has_value());

        InstanceOptionalReference carInstance = m_instanceEntityMapperInterface->FindOwningInstance(carContainerId);
        EXPECT_TRUE(carInstance.has_value());

        // Retrieve the wheel entity object and entity id after adding to instance
        EntityOptionalReference wheelEntityRef = carInstance->get().GetEntity(wheelEntityAlias);
        EXPECT_TRUE(wheelEntityRef.has_value());
        AZ::Entity* wheelEntity = &(wheelEntityRef->get());
        wheelEntityId = wheelEntity->GetId();

        // Modify the transform component as override
        AZ::TransformBus::Event(wheelEntityId, &AZ::TransformInterface::SetWorldX, 10.0f);

        // Add a new comopnent as override
        wheelEntity->Deactivate();
        wheelEntity->AddComponent(aznew PrefabTestComponent());
        wheelEntity->Activate();

        // Create an undo node and redo
        PrefabUndoEntityOverrides undoNode("Undo Editing Entity As Override");
        undoNode.CaptureAndRedo({ wheelEntity }, carInstance->get(), levelRootInstance->get());
        PropagateAllTemplateChanges();

        // Note for the following code and all other code in this file, 
        // PropagateAllTemplateChanges() may delete and re-create entities, it is not safe
        // to hold onto entity* pointers across this call.  You must fetch them by ID again.

        wheelEntity = AzToolsFramework::GetEntityById(wheelEntityId);
        ASSERT_TRUE(overrideInterface->AreOverridesPresent(wheelEntityId));
        ASSERT_FLOAT_EQ(10.0f, wheelEntity->GetTransform()->GetWorldX());
        ASSERT_TRUE(wheelEntity->FindComponent<PrefabTestComponent>());

        // Undo
        undoNode.Undo();
        PropagateAllTemplateChanges();

        wheelEntity = AzToolsFramework::GetEntityById(wheelEntityId);
        ASSERT_FALSE(overrideInterface->AreOverridesPresent(wheelEntityId));
        ASSERT_FLOAT_EQ(0.0f, wheelEntity->GetTransform()->GetWorldX());
        ASSERT_FALSE(wheelEntity->FindComponent<PrefabTestComponent>());

        // Redo
        undoNode.Redo();
        PropagateAllTemplateChanges();

        wheelEntity = AzToolsFramework::GetEntityById(wheelEntityId);
        ASSERT_TRUE(overrideInterface->AreOverridesPresent(wheelEntityId));
        ASSERT_FLOAT_EQ(10.0f, wheelEntity->GetTransform()->GetWorldX());
        ASSERT_TRUE(wheelEntity->FindComponent<PrefabTestComponent>());
    }

    TEST_F(PrefabUndoEditEntityTests, EditEntityAsOverrideOnAddEntityOverride)
    {
        // Level         <-- focused
        // | Car
        //   | Dummy
        //   | Entity    <-- add-entity override

        PrefabOverridePublicInterface* overrideInterface = AZ::Interface<PrefabOverridePublicInterface>::Get();
        EXPECT_TRUE(overrideInterface) << "Could not get the override public interface.";

        const AZStd::string carPrefabName = "Car";

        AZ::IO::Path engineRootPath;
        m_settingsRegistryInterface->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
        AZ::IO::Path carPrefabFilepath = engineRootPath / carPrefabName;

        AZ::EntityId tireEntityId = CreateEditorEntityUnderRoot("Dummy");
        AZ::EntityId carContainerId = CreateEditorPrefab(carPrefabFilepath, { tireEntityId });

        // Create a new entity as override under the car instance
        PrefabEntityResult createEntityResult = m_prefabPublicInterface->CreateEntity(carContainerId, AZ::Vector3());
        EXPECT_TRUE(createEntityResult.IsSuccess()) << "Could not add a wheel entity as override.";
        AZ::EntityId addedEntityId = createEntityResult.GetValue();
        AZ::Entity* addedEntity = AzToolsFramework::GetEntityById(addedEntityId);
        EXPECT_TRUE(addedEntity);

        InstanceOptionalReference levelRootInstance = m_instanceEntityMapperInterface->FindOwningInstance(GetRootContainerEntityId());
        EXPECT_TRUE(levelRootInstance.has_value());

        InstanceOptionalReference carInstance = m_instanceEntityMapperInterface->FindOwningInstance(carContainerId);
        EXPECT_TRUE(carInstance.has_value());

        // Modify the transform component as override
        AZ::TransformBus::Event(addedEntityId, &AZ::TransformInterface::SetWorldX, 10.0f);

        // Add a new comopnent as override
        addedEntity->Deactivate();
        addedEntity->AddComponent(aznew PrefabTestComponent());
        addedEntity->Activate();

        // Create an undo node and redo
        PrefabUndoEntityOverrides undoNode("Undo Editing Entity As Override");
        undoNode.CaptureAndRedo({ addedEntity }, carInstance->get(), levelRootInstance->get());
        PropagateAllTemplateChanges();

        // Note for the following code and all other code in this file, 
        // PropagateAllTemplateChanges() may delete and re-create entities, it is not safe
        // to hold onto entity* pointers across this call.  You must fetch them by ID again.

        addedEntity = AzToolsFramework::GetEntityById(addedEntityId);
        ASSERT_TRUE(overrideInterface->AreOverridesPresent(addedEntityId));
        ASSERT_FLOAT_EQ(10.0f, addedEntity->GetTransform()->GetWorldX());
        ASSERT_TRUE(addedEntity->FindComponent<PrefabTestComponent>());

        // Undo
        undoNode.Undo();
        PropagateAllTemplateChanges();

        addedEntity = AzToolsFramework::GetEntityById(addedEntityId);
        ASSERT_TRUE(overrideInterface->AreOverridesPresent(addedEntityId)); // The added entity itself is an override edit
        ASSERT_FLOAT_EQ(0.0f, addedEntity->GetTransform()->GetWorldX());
        ASSERT_FALSE(addedEntity->FindComponent<PrefabTestComponent>());

        // Redo
        undoNode.Redo();
        PropagateAllTemplateChanges();

        addedEntity = AzToolsFramework::GetEntityById(addedEntityId);
        ASSERT_TRUE(overrideInterface->AreOverridesPresent(addedEntityId));
        ASSERT_FLOAT_EQ(10.0f, addedEntity->GetTransform()->GetWorldX());
        ASSERT_TRUE(addedEntity->FindComponent<PrefabTestComponent>());
    }
} // namespace UnitTest
