/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/Document/AtomToolsDocumentApplication.h>
#include <AzToolsFramework/API/EditorWindowRequestBus.h>
#include <Viewport/MaterialViewportSettingsSystem.h>
#include <Window/MaterialEditorWindow.h>

namespace MaterialEditor
{
    class MaterialEditorApplication
        : public AtomToolsFramework::AtomToolsDocumentApplication
        , private AzToolsFramework::EditorWindowRequestBus::Handler
    {
    public:
        AZ_TYPE_INFO(MaterialEditor::MaterialEditorApplication, "{30F90CA5-1253-49B5-8143-19CEE37E22BB}");

        using Base = AtomToolsFramework::AtomToolsDocumentApplication;

        MaterialEditorApplication(int* argc, char*** argv);
        ~MaterialEditorApplication();

        // AzFramework::Application overrides...
        void Reflect(AZ::ReflectContext* context) override;
        const char* GetCurrentConfigurationName() const override;
        void StartCommon(AZ::Entity* systemEntity) override;
        void Destroy() override;

        // AtomToolsFramework::AtomToolsApplication overrides...
        AZStd::vector<AZStd::string> GetCriticalAssetFilters() const override;

        // AzToolsFramework::EditorWindowRequests::Bus::Handler
        QWidget* GetAppMainWindow() override;

    private:
        AZStd::unique_ptr<MaterialEditorWindow> m_window;
        AZStd::unique_ptr<MaterialViewportSettingsSystem> m_viewportSettingsSystem;
    };
} // namespace MaterialEditor
