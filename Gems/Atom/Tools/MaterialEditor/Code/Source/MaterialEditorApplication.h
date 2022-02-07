/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/AssetBrowser/AtomToolsAssetBrowserInteractions.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentApplication.h>
#include <AtomToolsFramework/Window/AtomToolsMainWindowFactoryRequestBus.h>
#include <AzToolsFramework/API/EditorWindowRequestBus.h>
#include <Window/MaterialEditorWindow.h>

namespace MaterialEditor
{
    class MaterialThumbnailRenderer;

    class MaterialEditorApplication
        : public AtomToolsFramework::AtomToolsDocumentApplication
        , private AzToolsFramework::EditorWindowRequestBus::Handler
        , private AtomToolsFramework::AtomToolsMainWindowFactoryRequestBus::Handler
    {
    public:
        AZ_TYPE_INFO(MaterialEditor::MaterialEditorApplication, "{30F90CA5-1253-49B5-8143-19CEE37E22BB}");

        using Base = AtomToolsFramework::AtomToolsDocumentApplication;

        MaterialEditorApplication(int* argc, char*** argv);
        ~MaterialEditorApplication();

        // AzFramework::Application overrides...
        void Reflect(AZ::ReflectContext* context) override;
        void CreateStaticModules(AZStd::vector<AZ::Module*>& outModules) override;
        const char* GetCurrentConfigurationName() const override;
        void StartCommon(AZ::Entity* systemEntity) override;

        // AtomToolsFramework::AtomToolsApplication overrides...
        AZStd::string GetBuildTargetName() const override;
        AZStd::vector<AZStd::string> GetCriticalAssetFilters() const override;

        // AtomToolsMainWindowFactoryRequestBus::Handler overrides...
        void CreateMainWindow() override;
        void DestroyMainWindow() override;

        // AzToolsFramework::EditorWindowRequests::Bus::Handler
        QWidget* GetAppMainWindow() override;

        AZStd::unique_ptr<MaterialEditorWindow> m_window;
        AZStd::unique_ptr<AtomToolsFramework::AtomToolsAssetBrowserInteractions> m_assetBrowserInteractions;
    };
} // namespace MaterialEditor
