/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AtomLyIntegration/CommonFeatures/Material/EditorMaterialSystemComponentNotificationBus.h>
#include <AtomLyIntegration/CommonFeatures/Material/MaterialComponentBus.h>
#include <AtomToolsFramework/DynamicProperty/DynamicPropertyGroup.h>
#include <AtomToolsFramework/Inspector/InspectorWidget.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/function/function_base.h>
#include <AzCore/std/string/string.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI_Internals.h>
#include <Material/EditorMaterialComponentUtil.h>
#endif

class QLabel;

namespace AZ
{
    namespace Render
    {
        namespace EditorMaterialComponentInspector
        {
            //! Inspector window for displaying and editing entity material instance properties
            //! If multiple entities are selected and pinned to this inspector then their corresponding properties will also be updated
            class MaterialPropertyInspector
                : public AtomToolsFramework::InspectorWidget
                , public AzToolsFramework::IPropertyEditorNotify
                , public AZ::EntitySystemBus::Handler
                , public AZ::SystemTickBus::Handler
                , public MaterialComponentNotificationBus::MultiHandler
                , public EditorMaterialSystemComponentNotificationBus::Handler
            {
                Q_OBJECT
            public:
                AZ_CLASS_ALLOCATOR(MaterialPropertyInspector, AZ::SystemAllocator);

                MaterialPropertyInspector(QWidget* parent = nullptr);
                ~MaterialPropertyInspector() override;

                //! Loads the material edit data for the active material on the primary entity ID.
                //! The function will fail and return false if the data cannot be loaded or the rest of the entities are not compatible with
                //! the primary entity materials.
                bool LoadMaterial(
                    const AZ::EntityId& primaryEntityId,
                    const AzToolsFramework::EntityIdSet& entityIdsToEdit,
                    const AZ::Render::MaterialAssignmentId& materialAssignmentId);

                //! Releases all of the edit data and assets, clearing the inspector of all content
                void UnloadMaterial();

                //! Returns true if all of the edit data has been loaded, the instance has been created, the primary entity and material
                //! slot has not changed the assigned material, and all of the entities share the same material type
                bool IsLoaded() const;

                // AtomToolsFramework::InspectorRequestBus::Handler overrides...
                void Reset() override;

                //! Builds all of the properties and generates the user interface for the inspector
                void Populate();

                AZStd::string GetRelativePath(const AZStd::string& path) const;
                AZStd::string GetFileName(const AZStd::string& path) const;
                bool IsSourceMaterial(const AZStd::string& path) const;
                bool SaveMaterial(const AZStd::string& path) const;
                void OpenMenu();
                const EditorMaterialComponentUtil::MaterialEditData& GetEditData() const;

            private:
                AZ::Data::AssetId GetActiveMaterialAssetIdFromEntity() const;

                // AzToolsFramework::IPropertyEditorNotify overrides...
                void BeforePropertyModified([[maybe_unused]] AzToolsFramework::InstanceDataNode* pNode) override{};
                void AfterPropertyModified(AzToolsFramework::InstanceDataNode* pNode) override;
                void SetPropertyEditingActive([[maybe_unused]] AzToolsFramework::InstanceDataNode* pNode) override{};
                void SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* pNode) override;
                void SealUndoStack() override{};
                void RequestPropertyContextMenu([[maybe_unused]] AzToolsFramework::InstanceDataNode*, const QPoint&) override{};
                void PropertySelectionChanged([[maybe_unused]] AzToolsFramework::InstanceDataNode*, bool) override{};

                // AZ::EntitySystemBus::Handler overrides...
                void OnEntityInitialized(const AZ::EntityId& entityId) override;
                void OnEntityDestroyed(const AZ::EntityId& entityId) override;
                void OnEntityActivated(const AZ::EntityId& entityId) override;
                void OnEntityDeactivated(const AZ::EntityId& entityId) override;
                void OnEntityNameChanged(const AZ::EntityId& entityId, const AZStd::string& name) override;

                //! AZ::SystemTickBus::Handler overrides...
                void OnSystemTick() override;

                //! MaterialComponentNotificationBus::MultiHandler overrides...
                void OnMaterialsEdited() override;

                //! EditorMaterialSystemComponentNotificationBus::Handler overrides...
                void OnRenderMaterialPreviewReady(
                    const AZ::EntityId& entityId,
                    const AZ::Render::MaterialAssignmentId& materialAssignmentId,
                    const QPixmap& pixmap) override;

                void UpdateUI();

                void CreateHeading();
                void UpdateHeading();

                void AddUvNamesGroup();
                void AddPropertiesGroup();

                void LoadOverridesFromEntity();
                void SaveOverrideToEntities(const AtomToolsFramework::DynamicProperty& property, bool commitChanges);
                void RunEditorMaterialFunctors();
                void UpdateMaterialInstanceProperty(const AtomToolsFramework::DynamicProperty& property);

                bool AddEditorMaterialFunctors(
                    const AZStd::vector<AZ::RPI::Ptr<AZ::RPI::MaterialFunctorSourceDataHolder>>& functorSourceDataHolders,
                    const AZ::RPI::MaterialNameContext& nameContext);

                AZ::Crc32 GetGroupSaveStateKey(const AZStd::string& groupName) const;
                bool IsInstanceNodePropertyModifed(const AzToolsFramework::InstanceDataNode* node) const;
                const char* GetInstanceNodePropertyIndicator(const AzToolsFramework::InstanceDataNode* node) const;

                AZ::EntityId m_primaryEntityId;
                AzToolsFramework::EntityIdSet m_entityIdsToEdit;
                AZ::Render::MaterialAssignmentId m_materialAssignmentId;
                EditorMaterialComponentUtil::MaterialEditData m_editData;
                AZ::Data::Instance<AZ::RPI::Material> m_materialInstance = {};
                AZStd::vector<AZ::RPI::Ptr<AZ::RPI::MaterialFunctor>> m_editorFunctors = {};
                AZ::RPI::MaterialPropertyFlags m_dirtyPropertyFlags = {};
                AZStd::unordered_map<AZStd::string, AtomToolsFramework::DynamicPropertyGroup> m_groups = {};
                bool m_internalEditNotification = {};
                bool m_updateUI = {};
                bool m_updatePreview = {};
                QLabel* m_overviewText = {};
                QLabel* m_overviewImage = {};
            };
        } // namespace EditorMaterialComponentInspector
    } // namespace Render
} // namespace AZ
