/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/Document/AtomToolsDocumentApplication.h>
#include <AtomToolsFramework/DynamicNode/DynamicNodeManager.h>
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportSettingsSystem.h>
#include <AzToolsFramework/API/EditorWindowRequestBus.h>
#include <GraphModel/Model/GraphContext.h>
#include <Window/MaterialCanvasMainWindow.h>

namespace MaterialCanvas
{
    class MaterialCanvasApplication
        : public AtomToolsFramework::AtomToolsDocumentApplication
        , private AzToolsFramework::EditorWindowRequestBus::Handler
    {
    public:
        AZ_TYPE_INFO(MaterialCanvas::MaterialCanvasApplication, "{30F90CA5-1253-49B5-8143-19CEE37E22BB}");

        using Base = AtomToolsFramework::AtomToolsDocumentApplication;

        MaterialCanvasApplication(int* argc, char*** argv);
        ~MaterialCanvasApplication();

        // AzFramework::Application overrides...
        void Reflect(AZ::ReflectContext* context) override;
        const char* GetCurrentConfigurationName() const override;
        void StartCommon(AZ::Entity* systemEntity) override;
        void Destroy() override;

        // AtomToolsFramework::AtomToolsApplication overrides...
        AZStd::vector<AZStd::string> GetCriticalAssetFilters() const override;

        // AzToolsFramework::EditorWindowRequests::Bus::Handler
        QWidget* GetAppMainWindow() override;

        AZStd::unique_ptr<MaterialCanvasMainWindow> m_window;
        AZStd::unique_ptr<AtomToolsFramework::EntityPreviewViewportSettingsSystem> m_viewportSettingsSystem;
        AZStd::unique_ptr<AtomToolsFramework::DynamicNodeManager> m_dynamicNodeManager;
        AZStd::shared_ptr<GraphModel::GraphContext> m_graphContext;
    };
} // namespace MaterialCanvas
