/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <ACES/Aces.h>
#include <Atom/Feature/Utils/LightingPreset.h>
#include <Atom/Feature/Utils/ModelPreset.h>
#include <Atom/Viewport/MaterialViewportNotificationBus.h>
#include <Atom/Viewport/MaterialViewportSettings.h>
#include <Atom/Window/MaterialEditorWindowSettings.h>
#include <AtomToolsFramework/Inspector/InspectorWidget.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI_Internals.h>
#endif

namespace MaterialEditor
{
    //! Provides controls for viewing and editing a material document settings.
    //! The settings can be divided into cards, with each one showing a subset of properties.
    class ViewportSettingsInspector
        : public AtomToolsFramework::InspectorWidget
        , private AzToolsFramework::IPropertyEditorNotify
        , private MaterialViewportNotificationBus::Handler
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(ViewportSettingsInspector, AZ::SystemAllocator, 0);

        explicit ViewportSettingsInspector(QWidget* parent = nullptr);
        ~ViewportSettingsInspector() override;

    private:
        void Populate();
        void AddGeneralGroup();

        void AddModelGroup();
        void AddModelPreset();
        void SelectModelPreset();
        void SaveModelPreset();

        void AddLightingGroup();
        void AddLightingPreset();
        void SelectLightingPreset();
        void SaveLightingPreset();

        void RefreshPresets();

        // AtomToolsFramework::InspectorRequestBus::Handler overrides...
        void Reset() override;

        // MaterialViewportNotificationBus::Handler overrides...
        void OnLightingPresetSelected([[maybe_unused]] AZ::Render::LightingPresetPtr preset) override;
        void OnModelPresetSelected([[maybe_unused]] AZ::Render::ModelPresetPtr preset) override;
        void OnShadowCatcherEnabledChanged(bool enable) override;
        void OnGridEnabledChanged(bool enable) override;
        void OnAlternateSkyboxEnabledChanged(bool enable) override;
        void OnFieldOfViewChanged(float fieldOfView) override;
        void OnDisplayMapperOperationTypeChanged(AZ::Render::DisplayMapperOperationType operationType) override;

        // AzToolsFramework::IPropertyEditorNotify overrides...
        void BeforePropertyModified(AzToolsFramework::InstanceDataNode* pNode) override;
        void AfterPropertyModified(AzToolsFramework::InstanceDataNode* pNode) override;
        void SetPropertyEditingActive([[maybe_unused]] AzToolsFramework::InstanceDataNode* pNode) override {}
        void SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* pNode) override;
        void ApplyChanges();
        void SealUndoStack() override {}
        void RequestPropertyContextMenu(AzToolsFramework::InstanceDataNode*, const QPoint&) override {}
        void PropertySelectionChanged(AzToolsFramework::InstanceDataNode*, bool) override {}

        AZStd::string GetDefaultUniqueSaveFilePath(const AZStd::string& baseName) const;

        AZ::Crc32 GetGroupSaveStateKey(const AZStd::string& groupName) const;
        bool ShouldGroupAutoExpanded(const AZStd::string& groupName) const override;
        void OnGroupExpanded(const AZStd::string& groupName) override;
        void OnGroupCollapsed(const AZStd::string& groupName) override;

        AZ::Render::ModelPresetPtr m_modelPreset;
        AZ::Render::LightingPresetPtr m_lightingPreset;
        AZStd::intrusive_ptr<MaterialViewportSettings> m_viewportSettings;
        AZStd::intrusive_ptr<MaterialEditorWindowSettings> m_windowSettings;
    };
} // namespace MaterialEditor
