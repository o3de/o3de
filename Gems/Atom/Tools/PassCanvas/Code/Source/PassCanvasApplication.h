/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/Document/AtomToolsDocumentApplication.h>
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportSettingsSystem.h>
#include <AtomToolsFramework/Graph/DynamicNode/DynamicNodeManager.h>
#include <AzToolsFramework/API/EditorWindowRequestBus.h>
#include <GraphModel/Model/GraphContext.h>
#include <Window/PassCanvasMainWindow.h>

namespace PassCanvas
{
    //! The main application class for Pass Canvas, setting up top level systems, document types, and the main window. 
    class PassCanvasApplication
        : public AtomToolsFramework::AtomToolsDocumentApplication
        , private AzToolsFramework::EditorWindowRequestBus::Handler
        , private AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(PassCanvasApplication, AZ::SystemAllocator)
        AZ_TYPE_INFO(PassCanvas::PassCanvasApplication, "{792D3C47-F380-44BC-B47D-621D8C526360}");

        using Base = AtomToolsFramework::AtomToolsDocumentApplication;

        PassCanvasApplication(int* argc, char*** argv);
        ~PassCanvasApplication();

        // AzFramework::Application overrides...
        void Reflect(AZ::ReflectContext* context) override;
        const char* GetCurrentConfigurationName() const override;
        void StartCommon(AZ::Entity* systemEntity) override;
        void Destroy() override;

    private:

        // AtomToolsFramework::AtomToolsApplication overrides...
        AZStd::vector<AZStd::string> GetCriticalAssetFilters() const override;

        // AzToolsFramework::EditorWindowRequests::Bus::Handler
        QWidget* GetAppMainWindow() override;

        void InitDynamicNodeManager();
        void InitDynamicNodeEditData();
        void InitSharedGraphContext();
        void InitGraphViewSettings();
        void InitPassGraphDocumentType();
        void InitMainWindow();
        void InitDefaultDocument();

        AZStd::unique_ptr<PassCanvasMainWindow> m_window;
        AZStd::unique_ptr<AtomToolsFramework::EntityPreviewViewportSettingsSystem> m_viewportSettingsSystem;
        AZStd::unique_ptr<AtomToolsFramework::DynamicNodeManager> m_dynamicNodeManager;
        AZStd::shared_ptr<GraphModel::GraphContext> m_graphContext;
        AtomToolsFramework::GraphViewSettingsPtr m_graphViewSettingsPtr;
    };
} // namespace PassCanvas
