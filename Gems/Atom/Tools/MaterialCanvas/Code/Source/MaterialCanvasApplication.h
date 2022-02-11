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
#include <AzToolsFramework/API/EditorWindowRequestBus.h>
#include <Window/MaterialCanvasMainWindow.h>

namespace MaterialCanvas
{
    class MaterialThumbnailRenderer;

    class MaterialCanvasApplication
        : public AtomToolsFramework::AtomToolsDocumentApplication
        , private AzToolsFramework::EditorWindowRequestBus::Handler
    {
    public:
        AZ_TYPE_INFO(MaterialCanvas::MaterialCanvasApplication, "{65B57D58-16EF-4CC2-BB22-A293A5240BB9}");

        using Base = AtomToolsFramework::AtomToolsDocumentApplication;

        MaterialCanvasApplication(int* argc, char*** argv);
        ~MaterialCanvasApplication();

        // AzFramework::Application overrides...
        void Reflect(AZ::ReflectContext* context) override;
        void CreateStaticModules(AZStd::vector<AZ::Module*>& outModules) override;
        const char* GetCurrentConfigurationName() const override;
        void StartCommon(AZ::Entity* systemEntity) override;
        void Destroy() override;

        // AtomToolsFramework::AtomToolsApplication overrides...
        AZStd::vector<AZStd::string> GetCriticalAssetFilters() const override;

        // AzToolsFramework::EditorWindowRequests::Bus::Handler
        QWidget* GetAppMainWindow() override;

        AZStd::unique_ptr<MaterialCanvasMainWindow> m_window;
        AZStd::unique_ptr<AtomToolsFramework::AtomToolsAssetBrowserInteractions> m_assetBrowserInteractions;
    };
} // namespace MaterialCanvas
