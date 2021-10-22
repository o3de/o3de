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
            class MaterialPropertyInspector
                : public AtomToolsFramework::InspectorWidget
                , public AzToolsFramework::IPropertyEditorNotify
                , public AZ::EntitySystemBus::Handler
                , public AZ::TickBus::Handler
                , public MaterialComponentNotificationBus::Handler
                , public EditorMaterialSystemComponentNotificationBus::Handler
           {
                Q_OBJECT
            public:
                AZ_CLASS_ALLOCATOR(MaterialPropertyInspector, AZ::SystemAllocator, 0);

                MaterialPropertyInspector(QWidget* parent = nullptr);
                ~MaterialPropertyInspector() override;

                bool LoadMaterial(const AZ::EntityId& entityId, const AZ::Render::MaterialAssignmentId& materialAssignmentId);
                void UnloadMaterial();
                bool IsLoaded() const;

                // AtomToolsFramework::InspectorRequestBus::Handler overrides...
                void Reset() override;

                void Populate();

                bool SaveMaterial() const;
                bool SaveMaterialToSource() const;
                bool HasMaterialSource() const;
                bool HasMaterialParentSource() const;
                void OpenMaterialSourceInEditor() const;
                void OpenMaterialParentSourceInEditor() const;
                void OpenMenu();
                const EditorMaterialComponentUtil::MaterialEditData& GetEditData() const;

            private:

                // AzToolsFramework::IPropertyEditorNotify overrides...
                void BeforePropertyModified(AzToolsFramework::InstanceDataNode* pNode) override;
                void AfterPropertyModified(AzToolsFramework::InstanceDataNode* pNode) override;
                void SetPropertyEditingActive([[maybe_unused]] AzToolsFramework::InstanceDataNode* pNode) override {}
                void SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* pNode) override;
                void SealUndoStack() override {}
                void RequestPropertyContextMenu([[maybe_unused]] AzToolsFramework::InstanceDataNode*, const QPoint&) override {}
                void PropertySelectionChanged([[maybe_unused]] AzToolsFramework::InstanceDataNode*, bool) override {}

                // AZ::EntitySystemBus::Handler overrides...
                void OnEntityInitialized(const AZ::EntityId& entityId) override;
                void OnEntityDestroyed(const AZ::EntityId& entityId) override;
                void OnEntityActivated(const AZ::EntityId& entityId) override;
                void OnEntityDeactivated(const AZ::EntityId& entityId) override;
                void OnEntityNameChanged(const AZ::EntityId& entityId, const AZStd::string& name) override;

                //! AZ::TickBus::Handler overrides...
                void OnTick(float deltaTime, ScriptTimePoint time) override;

                //! MaterialComponentNotificationBus::Handler overrides...
                void OnMaterialsEdited() override;

                //! EditorMaterialSystemComponentNotificationBus::Handler overrides...
                void OnRenderMaterialPreviewComplete(
                    const AZ::EntityId& entityId,
                    const AZ::Render::MaterialAssignmentId& materialAssignmentId,
                    const QPixmap& pixmap) override;

                void UpdateUI();

                void CreateHeading();
                void UpdateHeading();

                void AddUvNamesGroup();
                void AddPropertiesGroup();

                void LoadOverridesFromEntity();
                void SaveOverridesToEntity(bool commitChanges);
                void RunEditorMaterialFunctors();
                void UpdateMaterialInstanceProperty(const AtomToolsFramework::DynamicProperty& property);

                AZ::Crc32 GetGroupSaveStateKey(const AZStd::string& groupName) const;
                bool IsInstanceNodePropertyModifed(const AzToolsFramework::InstanceDataNode* node) const;
                const char* GetInstanceNodePropertyIndicator(const AzToolsFramework::InstanceDataNode* node) const;

                // Tracking the property that is actively being edited in the inspector
                const AtomToolsFramework::DynamicProperty* m_activeProperty = {};

                AZ::EntityId m_entityId;
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
