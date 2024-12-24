/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef AZTOOLSFRAMEWORK_TOOLSAPPLICATION_H
#define AZTOOLSFRAMEWORK_TOOLSAPPLICATION_H

#include <AzCore/base.h>
#include <AzFramework/Application/Application.h>
#include <AzFramework/Asset/SimpleAsset.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/EditorEntityAPI.h>
#include <AzToolsFramework/Application/EditorEntityManager.h>
#include <AzToolsFramework/Prefab/PrefabPublicNotificationBus.h>

#pragma once

namespace AzToolsFramework
{
    namespace UndoSystem
    {
        class UndoCacheInterface;
    }

    class ToolsApplication
        : public AzFramework::Application
        , public ToolsApplicationRequests::Bus::Handler
        , public AzToolsFramework::Prefab::PrefabPublicNotificationBus::Handler
    {
    public:
        AZ_RTTI(ToolsApplication, "{2895561E-BE90-4CC3-8370-DD46FCF74C01}", AzFramework::Application);
        AZ_CLASS_ALLOCATOR(ToolsApplication, AZ::SystemAllocator);

        ToolsApplication(int* argc = nullptr, char*** argv = nullptr);
        explicit ToolsApplication(AZ::ComponentApplicationSettings componentAppSettings);
        ToolsApplication(int* argc, char*** argv, AZ::ComponentApplicationSettings componentAppSettings);
        ~ToolsApplication();

        void Stop() override;
        void CreateReflectionManager() override;
        void Reflect(AZ::ReflectContext* context) override;

        AZ::ComponentTypeList GetRequiredSystemComponents() const override;

        AzToolsFramework::ToolsApplicationRequests::ResolveToolPathOutcome ResolveConfigToolsPath(const char* toolApplicationName) const override;

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::Application
        void Start(const Descriptor& descriptor, const StartupParameters& startupParameters = StartupParameters()) override;

    protected:

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::Application
        void StartCommon(AZ::Entity* systemEntity) override;
        void CreateStaticModules(AZStd::vector<AZ::Module*>& outModules) override;
        bool AddEntity(AZ::Entity* entity) override;
        bool RemoveEntity(AZ::Entity* entity) override;
        const char* GetCurrentConfigurationName() const override;
        void SetSettingsRegistrySpecializations(AZ::SettingsRegistryInterface::Specializations& specializations) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::ApplicationRequests::Bus overrides ...
        void QueryApplicationType(AZ::ApplicationTypeQuery& appType) const override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // ToolsApplicationRequests::Bus::Handler
        void PreExportEntity(AZ::Entity& source, AZ::Entity& target) override;
        void PostExportEntity(AZ::Entity& source, AZ::Entity& target) override;

        void MarkEntitySelected(AZ::EntityId entityId) override;
        void MarkEntitiesSelected(const EntityIdList& entitiesToSelect) override;

        void MarkEntityDeselected(AZ::EntityId entityId) override;
        void MarkEntitiesDeselected(const EntityIdList& entitiesToDeselect) override;

        void SetEntityHighlighted(AZ::EntityId entityId, bool highlighted) override;

        void AddDirtyEntity(AZ::EntityId entityId) override;
        int RemoveDirtyEntity(AZ::EntityId entityId) override;
        void ClearDirtyEntities() override;
        void AddIgnoredEntity(AZ::EntityId entityId) override;
        int RemoveIgnoredEntity(AZ::EntityId entityId) override;
        void ClearIgnoredEntities() override;
        bool IsDuringUndoRedo() override { return m_isDuringUndoRedo; }
        void UndoPressed() override;
        void RedoPressed() override;
        void FlushUndo() override;
        void FlushRedo() override;
        UndoSystem::URSequencePoint* BeginUndoBatch(const char* label) override;
        UndoSystem::URSequencePoint* ResumeUndoBatch(UndoSystem::URSequencePoint* token, const char* label) override;
        void EndUndoBatch() override;

        bool IsEntityEditable(AZ::EntityId entityId) override;
        bool AreEntitiesEditable(const EntityIdList& entityIds) override;

        void CheckoutPressed() override;
        SourceControlFileInfo GetSceneSourceControlInfo() override;

        bool AreAnyEntitiesSelected() override { return !m_selectedEntities.empty(); }
        int GetSelectedEntitiesCount() override { return static_cast<int>(m_selectedEntities.size()); }
        const EntityIdList& GetSelectedEntities() override { return m_selectedEntities; }
        const EntityIdList& GetHighlightedEntities() override { return m_highlightedEntities; }
        void SetSelectedEntities(const EntityIdList& selectedEntities) override;
        bool IsSelectable(const AZ::EntityId& entityId) override;
        bool IsSelected(const AZ::EntityId& entityId) override;
        bool IsSliceRootEntity(const AZ::EntityId& entityId) override;

        UndoSystem::UndoStack* GetUndoStack() override { return m_undoStack; }
        UndoSystem::URSequencePoint* GetCurrentUndoBatch() override { return m_currentBatchUndo; }

        EntityIdSet GatherEntitiesAndAllDescendents(const EntityIdList& inputEntities) override;

        AZ::EntityId CreateNewEntity(AZ::EntityId parentId = AZ::EntityId()) override;
        AZ::EntityId CreateNewEntityAtPosition(const AZ::Vector3& pos, AZ::EntityId parentId = AZ::EntityId()) override;
        AZ::EntityId GetExistingEntity(AZ::u64 id) override;
        bool EntityExists(AZ::EntityId id) override;
        void DeleteSelected() override;
        void DeleteEntityById(AZ::EntityId entityId) override;
        void DeleteEntities(const EntityIdList& entities) override;
        void DeleteEntityAndAllDescendants(AZ::EntityId entityId) override;
        void DeleteEntitiesAndAllDescendants(const EntityIdList& entities) override;

        bool FindCommonRoot(const EntityIdSet& entitiesToBeChecked, AZ::EntityId& commonRootEntityId, EntityIdList* topLevelEntities = nullptr) override;
        bool FindCommonRootInactive(const EntityList& entitiesToBeChecked, AZ::EntityId& commonRootEntityId, EntityList* topLevelEntities = nullptr) override;
        void FindTopLevelEntityIdsInactive(const EntityIdList& entityIdsToCheck, EntityIdList& topLevelEntityIds) override;
        AZ::SliceComponent::SliceInstanceAddress FindCommonSliceInstanceAddress(const EntityIdList& entityIds) override;
        AZ::EntityId GetRootEntityIdOfSliceInstance(AZ::SliceComponent::SliceInstanceAddress sliceAddress) override;
        AZ::EntityId GetCurrentLevelEntityId() override;

        bool RequestEditForFileBlocking(const char* assetPath, const char* progressMessage, const RequestEditProgressCallback& progressCallback) override;
        bool CheckSourceControlConnectionAndRequestEditForFileBlocking(const char* assetPath, const char* progressMessage, const RequestEditProgressCallback& progressCallback) override;
        void RequestEditForFile(const char* assetPath, RequestEditResultCallback resultCallback) override;
        void CheckSourceControlConnectionAndRequestEditForFile(const char* assetPath, RequestEditResultCallback resultCallback) override;

        void EnterEditorIsolationMode() override;
        void ExitEditorIsolationMode() override;
        bool IsEditorInIsolationMode() override;

        void CreateAndAddEntityFromComponentTags(const AZStd::vector<AZ::Crc32>& requiredTags, const char* entityName) override;

        /* Open 3D Engine INTERNAL USE ONLY. */
        void RunRedoSeparately(UndoSystem::URSequencePoint* redoCommand) override;

        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::SimpleAssetRequests::Bus::Handler
        struct PathAssetEntry
        {
            explicit PathAssetEntry(const char* path)
                : m_path(path) {}
            explicit PathAssetEntry(AZStd::string&& path)
                : m_path(AZStd::move(path)) {}
            AZStd::string m_path;
        };
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // PrefabPublicNotificationBus::Handler

        void OnPrefabInstancePropagationBegin() override;
        void OnPrefabInstancePropagationEnd() override;

        //////////////////////////////////////////////////////////////////////////

        void CreateUndosForDirtyEntities();
        void ConsistencyCheckUndoCache();
        AZ::Aabb                            m_selectionBounds;
        EntityIdList                        m_selectedEntities;
        EntityIdList                        m_highlightedEntities;
        UndoSystem::UndoStack*              m_undoStack;
        UndoSystem::URSequencePoint*        m_currentBatchUndo;
        AZStd::unordered_set<AZ::EntityId>  m_dirtyEntities;
        AZStd::unordered_set<AZ::EntityId>  m_ignoredEntities;
        bool                                m_isDuringUndoRedo;
        bool                                m_isInIsolationMode;
        EntityIdSet                         m_isolatedEntityIdSet;
        bool                                m_freezeSelectionUpdates = false;

        EditorEntityAPI* m_editorEntityAPI = nullptr;

        EditorEntityManager m_editorEntityManager;

        UndoSystem::UndoCacheInterface*             m_undoCacheInterface = nullptr;
    };
} // namespace AzToolsFramework

#endif // AZTOOLSFRAMEWORK_TOOLSAPPLICATION_H
