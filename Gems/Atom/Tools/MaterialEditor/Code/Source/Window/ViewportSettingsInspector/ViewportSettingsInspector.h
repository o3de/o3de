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
#include <AtomToolsFramework/Inspector/InspectorWidget.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI_Internals.h>
#include <Viewport/MaterialViewportSettings.h>
#include <Viewport/MaterialViewportSettingsNotificationBus.h>
#endif

namespace MaterialEditor
{
    //! Provides controls for viewing and editing lighting and model preset settings.
    class ViewportSettingsInspector
        : public AtomToolsFramework::InspectorWidget
        , private AzToolsFramework::IPropertyEditorNotify
        , private MaterialViewportSettingsNotificationBus::Handler
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(ViewportSettingsInspector, AZ::SystemAllocator, 0);

        ViewportSettingsInspector(const AZ::Crc32& toolId, QWidget* parent = nullptr);
        ~ViewportSettingsInspector() override;

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

        // AtomToolsFramework::InspectorRequestBus::Handler overrides...
        void Reset() override;

        // MaterialViewportSettingsNotificationBus::Handler overrides...
        void OnViewportSettingsChanged() override;

        // AzToolsFramework::IPropertyEditorNotify overrides...
        void BeforePropertyModified([[maybe_unused]] AzToolsFramework::InstanceDataNode* pNode) override {}
        void AfterPropertyModified(AzToolsFramework::InstanceDataNode* pNode) override;
        void SetPropertyEditingActive([[maybe_unused]] AzToolsFramework::InstanceDataNode* pNode) override {}
        void SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* pNode) override;
        void SealUndoStack() override {}
        void RequestPropertyContextMenu(AzToolsFramework::InstanceDataNode*, const QPoint&) override {}
        void PropertySelectionChanged(AzToolsFramework::InstanceDataNode*, bool) override {}

        AZ::Crc32 GetGroupSaveStateKey(const AZStd::string& groupName) const;

<<<<<<< HEAD
        AZ::Crc32 GetGroupSaveStateKey(const AZStd::string& groupName) const;
        bool ShouldGroupAutoExpanded(const AZStd::string& groupName) const override;
        void OnGroupExpanded(const AZStd::string& groupName) override;
        void OnGroupCollapsed(const AZStd::string& groupName) override;

        AZ::Render::ModelPresetPtr m_modelPreset;
        AZ::Render::LightingPresetPtr m_lightingPreset;
        AZStd::intrusive_ptr<MaterialViewportSettings> m_viewportSettings;
        AZStd::intrusive_ptr<MaterialEditorWindowSettings> m_windowSettings;
=======
        const AZ::Crc32 m_toolId = {};
        AZ::Render::ModelPreset m_modelPreset;
        AZ::Render::LightingPreset m_lightingPreset;
        MaterialViewportSettings m_viewportSettings;
>>>>>>> development
    };
} // namespace MaterialEditor
