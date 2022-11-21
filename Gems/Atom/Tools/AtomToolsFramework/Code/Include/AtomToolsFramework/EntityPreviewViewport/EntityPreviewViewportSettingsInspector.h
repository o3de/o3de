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
#include <AtomToolsFramework/AssetSelection/AssetSelectionGrid.h>
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportSettings.h>
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportSettingsNotificationBus.h>
#include <AtomToolsFramework/Inspector/InspectorWidget.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI_Internals.h>
#endif

#include <QFutureWatcher>

namespace AtomToolsFramework
{
    //! EntityPreviewViewportSettingsInspector provides controls for viewing and editing presets and other common viewport settings.
    class EntityPreviewViewportSettingsInspector
        : public InspectorWidget
        , private AzToolsFramework::IPropertyEditorNotify
        , private EntityPreviewViewportSettingsNotificationBus::Handler
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(EntityPreviewViewportSettingsInspector, AZ::SystemAllocator, 0);

        EntityPreviewViewportSettingsInspector(const AZ::Crc32& toolId, QWidget* parent = nullptr);
        ~EntityPreviewViewportSettingsInspector() override;

    private:
        void Populate();
        void AddGeneralGroup();

        void AddModelGroup();
        void CreateModelPreset();
        void SelectModelPreset();
        void SaveModelPreset();

        void AddLightingGroup();
        void CreateLightingPreset();
        void SelectLightingPreset();
        void SaveLightingPreset();

        void SaveSettings();
        void LoadSettings();

        // InspectorRequestBus::Handler overrides...
        void Reset() override;

        // EntityPreviewViewportSettingsNotificationBus::Handler overrides...
        void OnViewportSettingsChanged() override;
        void OnModelPresetAdded(const AZStd::string& path) override;
        void OnLightingPresetAdded(const AZStd::string& path) override;

        // AzToolsFramework::IPropertyEditorNotify overrides...
        void BeforePropertyModified([[maybe_unused]] AzToolsFramework::InstanceDataNode* pNode) override{};
        void AfterPropertyModified(AzToolsFramework::InstanceDataNode* pNode) override;
        void SetPropertyEditingActive([[maybe_unused]] AzToolsFramework::InstanceDataNode* pNode) override{};
        void SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* pNode) override;
        void SealUndoStack() override{};
        void RequestPropertyContextMenu(AzToolsFramework::InstanceDataNode*, const QPoint&) override{};
        void PropertySelectionChanged(AzToolsFramework::InstanceDataNode*, bool) override{};

        AZ::Crc32 GetGroupSaveStateKey(const AZStd::string& groupName) const;

        const AZ::Crc32 m_toolId = {};

        AZStd::string m_modelPresetPath;
        AZ::Render::ModelPreset m_modelPreset;
        AZStd::unique_ptr<AssetSelectionGrid> m_modelPresetDialog;

        AZStd::string m_lightingPresetPath;
        AZ::Render::LightingPreset m_lightingPreset;
        AZStd::unique_ptr<AssetSelectionGrid> m_lightingPresetDialog;

        EntityPreviewViewportSettings m_viewportSettings;

        QFutureWatcher<AZStd::vector<AZStd::string>> m_watcher;
    };
} // namespace AtomToolsFramework
