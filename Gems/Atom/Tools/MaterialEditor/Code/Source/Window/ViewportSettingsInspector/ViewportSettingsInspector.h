/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#if !defined(Q_MOC_RUN)
#include <Atom/Viewport/MaterialViewportNotificationBus.h>
#include <Atom/Feature/Utils/LightingPreset.h>
#include <Atom/Feature/Utils/ModelPreset.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI_Internals.h>
#include <AtomToolsFramework/Inspector/InspectorWidget.h>
#endif

namespace MaterialEditor
{
    struct GeneralViewportSettings
    {
        AZ_TYPE_INFO(GeneralViewportSettings, "{16150503-A314-4765-82A3-172670C9EA90}");
        AZ_CLASS_ALLOCATOR(GeneralViewportSettings, AZ::SystemAllocator, 0);
        static void Reflect(AZ::ReflectContext* context);

        bool m_enableGrid = true;
        bool m_enableShadowCatcher = true;
        bool m_enableAlternateSkybox = false;
        float m_fieldOfView = 90.0f;
    };

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
        void Popuate();
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

        GeneralViewportSettings m_generalSettings;
        AZ::Render::ModelPresetPtr m_modelPreset;
        AZ::Render::LightingPresetPtr m_lightingPreset;
    };
} // namespace MaterialEditor
