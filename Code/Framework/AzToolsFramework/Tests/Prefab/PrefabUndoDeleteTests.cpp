/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Prefab/PrefabTestFixture.h>

#include <AzToolsFramework/Prefab/Undo/PrefabUndo.h>
#include <AzToolsFramework/Prefab/Undo/PrefabUndoDelete.h>
#include <AzToolsFramework/Prefab/Undo/PrefabUndoDeleteAsOverride.h>

namespace UnitTest
{
    using PrefabUndoDeleteTests = PrefabTestFixture;

    TEST_F(PrefabUndoDeleteTests, PrefabUndoDeleteTests_DeleteEntity)
    {
        // Level
        // | Car         <-- focused
        //   | Tire      <-- delete
        // | Car
        //   | Tire

        const AZStd::string carPrefabName = "Car";
        const AZStd::string tireEntityName = "Tire";

        AZ::IO::Path engineRootPath;
        m_settingsRegistryInterface->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
        AZ::IO::Path carPrefabFilepath(engineRootPath);
        carPrefabFilepath.Append(carPrefabName);

        AZ::EntityId tireEntityId = CreateEditorEntity(tireEntityName, GetRootContainerEntityId());
        AZ::EntityId carContainerId = CreateEditorPrefab(carPrefabFilepath, { tireEntityId });
        
        EntityAlias tireEntityAlias = FindEntityAliasInInstance(carContainerId, tireEntityName);
        ASSERT_FALSE(tireEntityAlias.empty());

        AZ::EntityId secondCarContainerId = InstantiateEditorPrefab(carPrefabFilepath, GetRootContainerEntityId());

        auto firstCarInstance = m_instanceEntityMapperInterface->FindOwningInstance(carContainerId);
        ASSERT_TRUE(firstCarInstance.has_value());

        auto secondCarInstance = m_instanceEntityMapperInterface->FindOwningInstance(secondCarContainerId);
        ASSERT_TRUE(secondCarInstance.has_value());

        // Validate before deletion
        ValidateEntityUnderInstance(firstCarInstance->get().GetContainerEntityId(), tireEntityAlias, tireEntityName);
        ValidateEntityUnderInstance(secondCarInstance->get().GetContainerEntityId(), tireEntityAlias, tireEntityName);

        // Create an undo node
        PrefabUndoDeleteEntity undoDeleteNode("Undo Deleting Entity");

        EntityOptionalReference parentEntityToUpdate = firstCarInstance->get().GetContainerEntity();
        ASSERT_TRUE(parentEntityToUpdate.has_value());

        AZ::EntityId firstTireEntityId = firstCarInstance->get().GetEntityId(tireEntityAlias);
        ASSERT_TRUE(firstTireEntityId.IsValid());

        AZStd::string firstTireEntityAliasPath = m_instanceToTemplateInterface->GenerateEntityAliasPath(firstTireEntityId);

        firstCarInstance->get().DetachEntity(firstTireEntityId).reset();

        undoDeleteNode.Capture({ firstTireEntityAliasPath }, { &(parentEntityToUpdate->get()) }, firstCarInstance->get());

        // Redo
        undoDeleteNode.Redo();
        PropagateAllTemplateChanges();
        
        ValidateEntityNotUnderInstance(firstCarInstance->get().GetContainerEntityId(), tireEntityAlias);
        ValidateEntityNotUnderInstance(secondCarInstance->get().GetContainerEntityId(), tireEntityAlias);

        // Undo
        undoDeleteNode.Undo();
        PropagateAllTemplateChanges();

        ValidateEntityUnderInstance(firstCarInstance->get().GetContainerEntityId(), tireEntityAlias, tireEntityName);
        ValidateEntityUnderInstance(secondCarInstance->get().GetContainerEntityId(), tireEntityAlias, tireEntityName);
    }

    TEST_F(PrefabUndoDeleteTests, PrefabUndoDeleteTests_DeleteEntityAsOverride)
    {
        // Level        <-- focused
        // | Car
        //   | Tire     <-- delete
        // | Car
        //   | Tire

        const AZStd::string carPrefabName = "Car";
        const AZStd::string tireEntityName = "Tire";

        AZ::IO::Path engineRootPath;
        m_settingsRegistryInterface->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
        AZ::IO::Path carPrefabFilepath(engineRootPath);
        carPrefabFilepath.Append(carPrefabName);

        AZ::EntityId tireEntityId = CreateEditorEntity(tireEntityName, GetRootContainerEntityId());
        AZ::EntityId carContainerId = CreateEditorPrefab(carPrefabFilepath, { tireEntityId });

        EntityAlias tireEntityAlias = FindEntityAliasInInstance(carContainerId, tireEntityName);
        ASSERT_FALSE(tireEntityAlias.empty());

        AZ::EntityId secondCarContainerId = InstantiateEditorPrefab(carPrefabFilepath, GetRootContainerEntityId());

        auto firstCarInstance = m_instanceEntityMapperInterface->FindOwningInstance(carContainerId);
        ASSERT_TRUE(firstCarInstance.has_value());

        auto secondCarInstance = m_instanceEntityMapperInterface->FindOwningInstance(secondCarContainerId);
        ASSERT_TRUE(secondCarInstance.has_value());

        // Validate before deletion
        ValidateEntityUnderInstance(firstCarInstance->get().GetContainerEntityId(), tireEntityAlias, tireEntityName);
        ValidateEntityUnderInstance(secondCarInstance->get().GetContainerEntityId(), tireEntityAlias, tireEntityName);

        // Create an undo node
        PrefabUndoDeleteAsOverride undoDeleteNode("Undo Deleting Entity As Override");

        EntityOptionalReference parentEntityToUpdate = firstCarInstance->get().GetContainerEntity();
        ASSERT_TRUE(parentEntityToUpdate.has_value());

        AZ::EntityId firstTireEntityId = firstCarInstance->get().GetEntityId(tireEntityAlias);
        ASSERT_TRUE(firstTireEntityId.IsValid());

        AZStd::string firstTireEntityAliasPath = m_instanceToTemplateInterface->GenerateEntityAliasPath(firstTireEntityId);

        firstCarInstance->get().DetachEntity(firstTireEntityId).reset();

        auto levelRootInstance = m_instanceEntityMapperInterface->FindOwningInstance(GetRootContainerEntityId());
        ASSERT_TRUE(levelRootInstance.has_value());

        undoDeleteNode.Capture({ firstTireEntityAliasPath }, {}, { &(parentEntityToUpdate->get()) },
            firstCarInstance->get(), levelRootInstance->get());

        // Redo
        undoDeleteNode.Redo();
        PropagateAllTemplateChanges();

        ValidateEntityNotUnderInstance(firstCarInstance->get().GetContainerEntityId(), tireEntityAlias);
        ValidateEntityUnderInstance(secondCarInstance->get().GetContainerEntityId(), tireEntityAlias, tireEntityName);

        // Undo
        undoDeleteNode.Undo();
        PropagateAllTemplateChanges();
        
        ValidateEntityUnderInstance(firstCarInstance->get().GetContainerEntityId(), tireEntityAlias, tireEntityName);
        ValidateEntityUnderInstance(secondCarInstance->get().GetContainerEntityId(), tireEntityAlias, tireEntityName);
    }

    TEST_F(PrefabUndoDeleteTests, PrefabUndoDeleteTests_DeleteNestedInstance)
    {
        // Level
        // | Car         <-- focused
        //   | Wheel     <-- delete
        //     | Tire
        // | Car
        //   | Wheel
        //     | Tire

        const AZStd::string carPrefabName = "Car";
        const AZStd::string wheelPrefabName = "Wheel";
        const AZStd::string tireEntityName = "Tire";

        AZ::IO::Path engineRootPath;
        m_settingsRegistryInterface->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);

        AZ::IO::Path carPrefabFilepath(engineRootPath);
        carPrefabFilepath.Append(carPrefabName);

        AZ::IO::Path wheelPrefabFilepath(engineRootPath);
        wheelPrefabFilepath.Append(wheelPrefabName);

        AZ::EntityId tireEntityId = CreateEditorEntity(tireEntityName, GetRootContainerEntityId());
        AZ::EntityId wheelContainerId = CreateEditorPrefab(wheelPrefabFilepath, { tireEntityId });
        AZ::EntityId carContainerId = CreateEditorPrefab(carPrefabFilepath, { wheelContainerId });

        InstanceAlias wheelInstanceAlias = FindNestedInstanceAliasInInstance(carContainerId, wheelPrefabName);
        ASSERT_FALSE(wheelInstanceAlias.empty());

        AZ::EntityId secondCarContainerId = InstantiateEditorPrefab(carPrefabFilepath, GetRootContainerEntityId());

        auto firstCarInstance = m_instanceEntityMapperInterface->FindOwningInstance(carContainerId);
        ASSERT_TRUE(firstCarInstance.has_value());

        auto secondCarInstance = m_instanceEntityMapperInterface->FindOwningInstance(secondCarContainerId);
        ASSERT_TRUE(secondCarInstance.has_value());

        // Validate before deletion
        ValidateNestedInstanceUnderInstance(firstCarInstance->get().GetContainerEntityId(), wheelInstanceAlias);
        ValidateNestedInstanceUnderInstance(secondCarInstance->get().GetContainerEntityId(), wheelInstanceAlias);

        // Create an undo node
        // Note: Currently for deleting a nested instance, it depends on the PrefabUndoInstanceLink undo node.
        PrefabUndoInstanceLink undoDeleteNestedInstance("Undo Delete Nested Instance");

        AZStd::unique_ptr<Instance> firstWheelInstance = firstCarInstance->get().DetachNestedInstance(wheelInstanceAlias);
        TemplateId wheelTemplateId = firstWheelInstance->GetTemplateId();
        LinkId wheelLinkId = firstWheelInstance->GetLinkId();

        LinkReference firstWheelLink = m_prefabSystemComponent->FindLink(wheelLinkId);
        ASSERT_TRUE(firstWheelLink.has_value());

        // Generate link patches needed for redo and undo support.
        PrefabDom patchesCopyForUndoSupport;
        PrefabDom wheelInstanceLinkDom;
        firstWheelLink->get().GetLinkDom(wheelInstanceLinkDom, wheelInstanceLinkDom.GetAllocator());
        PrefabDomValueReference wheelInstanceLinkPatches =
            PrefabDomUtils::FindPrefabDomValue(wheelInstanceLinkDom, PrefabDomUtils::PatchesName);
        if (wheelInstanceLinkPatches.has_value())
        {
            patchesCopyForUndoSupport.CopyFrom(wheelInstanceLinkPatches->get(), patchesCopyForUndoSupport.GetAllocator());
        }

        firstWheelInstance.reset();

        undoDeleteNestedInstance.Capture(firstCarInstance->get().GetTemplateId(), wheelTemplateId,
            wheelInstanceAlias, AZStd::move(patchesCopyForUndoSupport), wheelLinkId);

        // Redo
        undoDeleteNestedInstance.Redo();
        PropagateAllTemplateChanges();

        ValidateNestedInstanceNotUnderInstance(firstCarInstance->get().GetContainerEntityId(), wheelInstanceAlias);
        ValidateNestedInstanceNotUnderInstance(secondCarInstance->get().GetContainerEntityId(), wheelInstanceAlias);

        // Undo
        undoDeleteNestedInstance.Undo();
        PropagateAllTemplateChanges();

        ValidateNestedInstanceUnderInstance(firstCarInstance->get().GetContainerEntityId(), wheelInstanceAlias);
        ValidateNestedInstanceUnderInstance(secondCarInstance->get().GetContainerEntityId(), wheelInstanceAlias);
    }

    TEST_F(PrefabUndoDeleteTests, PrefabUndoDeleteTests_DeleteInstanceAsOverride)
    {
        // Level          <-- focused
        // | First Car
        //   | Wheel      <-- delete
        //     | Tire
        // | Second Car
        //   | Wheel
        //     | Tire

        const AZStd::string carPrefabName = "Car";
        const AZStd::string wheelPrefabName = "Wheel";
        const AZStd::string tireEntityName = "Tire";

        AZ::IO::Path engineRootPath;
        m_settingsRegistryInterface->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);

        AZ::IO::Path carPrefabFilepath(engineRootPath);
        carPrefabFilepath.Append(carPrefabName);

        AZ::IO::Path wheelPrefabFilepath(engineRootPath);
        wheelPrefabFilepath.Append(wheelPrefabName);

        AZ::EntityId tireEntityId = CreateEditorEntity(tireEntityName, GetRootContainerEntityId());
        AZ::EntityId wheelContainerId = CreateEditorPrefab(wheelPrefabFilepath, { tireEntityId });
        AZ::EntityId carContainerId = CreateEditorPrefab(carPrefabFilepath, { wheelContainerId });

        InstanceAlias wheelInstanceAlias = FindNestedInstanceAliasInInstance(carContainerId, wheelPrefabName);
        ASSERT_FALSE(wheelInstanceAlias.empty());

        AZ::EntityId secondCarContainerId = InstantiateEditorPrefab(carPrefabFilepath, GetRootContainerEntityId());

        auto firstCarInstance = m_instanceEntityMapperInterface->FindOwningInstance(carContainerId);
        ASSERT_TRUE(firstCarInstance.has_value());

        auto secondCarInstance = m_instanceEntityMapperInterface->FindOwningInstance(secondCarContainerId);
        ASSERT_TRUE(secondCarInstance.has_value());

        // Validate before deletion
        ValidateNestedInstanceUnderInstance(firstCarInstance->get().GetContainerEntityId(), wheelInstanceAlias);
        ValidateNestedInstanceUnderInstance(secondCarInstance->get().GetContainerEntityId(), wheelInstanceAlias);

        // Create an undo node
        PrefabUndoDeleteAsOverride undoDeleteAsOverride("Undo Delete Instance As Override");

        auto parentEntityToUpdate = firstCarInstance->get().GetContainerEntity();
        ASSERT_TRUE(parentEntityToUpdate.has_value());

        const AZStd::string firstWheelInstanceAliasPath = PrefabDomUtils::PathStartingWithInstances + wheelInstanceAlias;

        firstCarInstance->get().DetachNestedInstance(wheelInstanceAlias).reset();

        auto levelRootInstance = m_instanceEntityMapperInterface->FindOwningInstance(GetRootContainerEntityId());
        ASSERT_TRUE(levelRootInstance.has_value());

        undoDeleteAsOverride.Capture({}, { firstWheelInstanceAliasPath }, { &(parentEntityToUpdate->get()) },
            firstCarInstance->get(), levelRootInstance->get());

        // Redo
        undoDeleteAsOverride.Redo();
        PropagateAllTemplateChanges();

        ValidateNestedInstanceNotUnderInstance(firstCarInstance->get().GetContainerEntityId(), wheelInstanceAlias);
        ValidateNestedInstanceUnderInstance(secondCarInstance->get().GetContainerEntityId(), wheelInstanceAlias);

        // Undo
        undoDeleteAsOverride.Undo();
        PropagateAllTemplateChanges();

        ValidateNestedInstanceUnderInstance(firstCarInstance->get().GetContainerEntityId(), wheelInstanceAlias);
        ValidateNestedInstanceUnderInstance(secondCarInstance->get().GetContainerEntityId(), wheelInstanceAlias);
    }
} // namespace UnitTest
