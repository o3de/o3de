/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <Prefab/PrefabTestData.h>
#include <Prefab/PrefabTestUtils.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class InstanceEntityMapperInterface;
        class PrefabLoaderInterface;
    }

    class PrefabEditorEntityOwnershipInterface;
}

namespace UnitTest
{
    using namespace AzToolsFramework::Prefab;
    using namespace PrefabTestUtils;

    //! Defines prefab test tools application.
    class PrefabTestToolsApplication
        : public ToolsTestApplication
    {
    public:
        PrefabTestToolsApplication(AZStd::string appName);
    };

    //! Defines prefab testing environment.
    class PrefabTestFixture
        : public ToolsApplicationFixture<>
        , public AzToolsFramework::EditorRequestBus::Handler
    {
    protected:

        inline static const char* PrefabMockFilePath = "SomePath";
        inline static const char* NestedPrefabMockFilePath = "SomePathToNested";
        inline static const char* WheelPrefabMockFilePath = "SomePathToWheel";
        inline static const char* AxlePrefabMockFilePath = "SomePathToAxle";
        inline static const char* CarPrefabMockFilePath = "SomePathToCar";

        void SetUpEditorFixtureImpl() override;
        void TearDownEditorFixtureImpl() override;

        AZStd::unique_ptr<ToolsTestApplication> CreateTestApplication() override;

        //! Creates root prefab and focus on the root prefab instance as level.
        void CreateRootPrefab();

        //! Functions that internally call prefab public interface APIs to create editor entities and prefab instances.
        //! Entities and instances are updated correctly, and template DOM representations are also updated accordingly.
        //! Note:
        //!   1. It may create entities and instances as overrides depending on which instance is currently focused.
        //!   2. The returned entity id cannot always be depended on. For example, if the new entity is used to create a new prefab,
        //!      the entity id changes after it is added to the instance.
        //! @{
        AZ::EntityId CreateEditorEntityUnderRoot(const AZStd::string& entityName);
        AZ::EntityId CreateEditorEntity(const AZStd::string& entityName, AZ::EntityId parentId);
        AZ::EntityId CreateEditorPrefab(AZ::IO::PathView filePath, const AzToolsFramework::EntityIdList& entityIds);
        AZ::EntityId InstantiateEditorPrefab(AZ::IO::PathView filePath, AZ::EntityId parentId);
        //! @}

        //! Creates a loose entity object with no components.
        //! This creates an entity object through aznew rather than calling public prefab APIs.
        //! Note: Editor components can be added manually or by calling AddRequiredEditorComponents.
        //! @param entityName Name of the new entity.
        //! @param shouldActivate Flag that decides if the new entity should be activated after creation.
        //! @return The new entity.
        AZ::Entity* CreateEntity(const AZStd::string& entityName, bool shouldActivate = true);

        //! Helper function to get container entity id of root prefab.
        //! @return Container entity id for root prefab.
        AZ::EntityId GetRootContainerEntityId();

        //! After prefab template is updated, we need to propagate the changes to all prefab instances.
        //! Note that any instances involved inside this template may be destroyed and recreated, so do not hold
        //! onto Entity* or Component* pointers into objects that may be affected by this call.  They may not be valid.
        void PropagateAllTemplateChanges();

        //! Helper function to compare two instances and asserts will be thrown if two instances are not identical.
        //! @param instanceA The given first instance.
        //! @param instanceB The given second instance.
        //! @param shouldCompareLinkIds Flag of whether it compares two link ids.
        //! @param shouldCompareContainerEntities Flag of whether it compares two container entities.
        void CompareInstances(const Instance& instanceA, const Instance& instanceB, bool shouldCompareLinkIds = true,
            bool shouldCompareContainerEntities = true);

        void DeleteInstances(const InstanceList& instancesToDelete);
        
        //! Helper function for finding entity alias by entity name in owning instance.
        //! If there are multiple aliases, then return the first one it found.
        //! @param containerEntityId The given container entity id of the owning instance.
        //! @param entityName Entity name for the entity alias to find.
        //! @return Entity alias for the given entity name. Returns "" if not found.
        EntityAlias FindEntityAliasInInstance(AZ::EntityId containerEntityId, const AZStd::string& entityName);

        //! Helper function for finding nested instance alias by container entity name in owning instance.
        //! If there are multiple aliases, then return the first one it found.
        //! @param containerEntityId The given container entity id of the owning instance.
        //! @param nestedContainerEntityName Container entity name for the instance alias to find.
        //! @return Nested instance alias for the given container entity name. Returns "" if not found.
        InstanceAlias FindNestedInstanceAliasInInstance(AZ::EntityId containerEntityId, const AZStd::string& nestedContainerEntityName);

        //! Rename an entity.
        //! Note: Renaming a container entity would be an override edit on the focused prefab.
        //! @param entityId Entity id for entity to be renamed.
        //! @param newName New entity name.
        void RenameEntity(AZ::EntityId entityId, const AZStd::string& newName);

        //! Helper functions for validation.
        //! @{
        void ValidateEntityUnderInstance(AZ::EntityId containerEntityId, const EntityAlias& entityAlias, const AZStd::string& entityName);
        void ValidateEntityNotUnderInstance(AZ::EntityId containerEntityId, const EntityAlias& entityAlias);
        void ValidateNestedInstanceUnderInstance(AZ::EntityId containerEntityId, const InstanceAlias& nestedInstanceAlias);
        void ValidateNestedInstanceNotUnderInstance(AZ::EntityId containerEntityId, const InstanceAlias& nestedInstanceAlias);

        // Validates that all entities within a prefab instance are in 'Active' state.
        void ValidateInstanceEntitiesActive(Instance& instance);
        //! @}

        // Kicks off any updates scheduled for the next tick
        virtual void ProcessDeferredUpdates();

        // Performs an undo operation and ensures the tick-scheduled updates happen
        void Undo();

        // Performs a redo operation and ensures the tick-scheduled updates happen
        void Redo();

        //! Adds required editor components to entity.
        //! This function does similar work as CreateEditorRepresentation does. But we use this one to manually add
        //! editor components when we create a new entity via PrefabTestFixture::CreateEntity() or aznew operator.
        //! Note: This will add a transform component, so you should not add the transform by yourself before
        //! and after calling this for the same entity.
        //! @param entity The entity that components will be added to.
        void AddRequiredEditorComponents(const AzToolsFramework::EntityIdList& entityIds);

        //! EditorRequestBus.
        //! CreateEditorRepresentation is implemented in this test fixture. Then the required editor components
        //! will be added during entity and prefab creation, eg transform component, child entity sort component.
        //! @{
        void CreateEditorRepresentation(AZ::Entity* entity) override;
        void BrowseForAssets([[maybe_unused]] AzToolsFramework::AssetBrowser::AssetSelectionModel& selection) override
        {
        }
        //! @}

        // Prefab interfaces
        PrefabSystemComponent* m_prefabSystemComponent = nullptr;
        PrefabLoaderInterface* m_prefabLoaderInterface = nullptr;
        PrefabPublicInterface* m_prefabPublicInterface = nullptr;
        InstanceEntityMapperInterface* m_instanceEntityMapperInterface = nullptr;
        InstanceUpdateExecutorInterface* m_instanceUpdateExecutorInterface = nullptr;
        InstanceToTemplateInterface* m_instanceToTemplateInterface = nullptr;
        AzToolsFramework::PrefabEditorEntityOwnershipInterface* m_prefabEditorEntityOwnershipInterface = nullptr;

        // SettingsRegistryInterface
        AZ::SettingsRegistryInterface* m_settingsRegistryInterface = nullptr;

        // Undo Stack
        AzToolsFramework::UndoSystem::UndoStack* m_undoStack = nullptr;
    };
} // namespace UnitTest
