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
#include <Viewport/MaterialViewportNotificationBus.h>
#include <Viewport/MaterialViewportSettings.h>
#endif

namespace MaterialEditor
{
    //! Provides controls for viewing and editing lighting and model preset settings.
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

        // MaterialViewportNotificationBus::Handler overrides...
        void OnViewportSettingsChanged() override;

        // AzToolsFramework::IPropertyEditorNotify overrides...
        void BeforePropertyModified([[maybe_unused]] AzToolsFramework::InstanceDataNode* pNode) override {}
        void AfterPropertyModified(AzToolsFramework::InstanceDataNode* pNode) override;
        void SetPropertyEditingActive([[maybe_unused]] AzToolsFramework::InstanceDataNode* pNode) override {}
        void SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* pNode) override;
        void SealUndoStack() override {}
        void RequestPropertyContextMenu(AzToolsFramework::InstanceDataNode*, const QPoint&) override {}
        void PropertySelectionChanged(AzToolsFramework::InstanceDataNode*, bool) override {}

        AZStd::string GetDefaultUniqueSaveFilePath(const AZStd::string& baseName) const;

        AZ::Crc32 GetGroupSaveStateKey(const AZStd::string& groupName) const;

        AZ::Render::ModelPreset m_modelPreset;
        AZ::Render::LightingPreset m_lightingPreset;
        MaterialViewportSettings m_viewportSettings;
    };
} // namespace MaterialEditor
