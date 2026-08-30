/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/Document/AtomToolsDocumentSystem.h>
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportSettingsSystem.h>
#include <AtomToolsFramework/Graph/AssetStatusReporterSystem.h>
#include <AtomToolsFramework/Graph/DynamicNode/DynamicNodeManager.h>
#include <AtomToolsFramework/Graph/GraphTemplateFileDataCache.h>
#include <AtomToolsFramework/Graph/GraphViewSettings.h>
#include <AzCore/Component/Component.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ActionManager/ActionManagerRegistrationNotificationBus.h>
#include <GraphModel/Model/GraphContext.h>

namespace MaterialCanvas
{
    class MaterialCanvasPaneWindow;

    //! Hosts Material Canvas inside the O3DE Editor as a view pane, as an alternative to launching the standalone
    //! MaterialCanvas application.
    //!
    //! WHY THIS EXISTS
    //!
    //! The standalone tool is its own process, which means a second AzFramework::Application, a second Qt application, a
    //! second Asset Processor connection and asset catalog, a second RPI system, and a second copy of every gem DLL the
    //! project enables. Running it alongside the Editor roughly doubles the memory footprint of a look-dev session. Every
    //! system the standalone application owns is really just a toolId-scoped object constructed with aznew and nothing about
    //! them requires an Application, so they are constructed here instead and the Editor's process is shared.
    //!
    //! RELATIONSHIP TO THE STANDALONE APPLICATION
    //!
    //! MaterialCanvasApplication and MaterialCanvasMainWindow are deliberately untouched and still build and run exactly as
    //! before. The setup performed in EnsureSystemsInitialized below mirrors MaterialCanvasApplication::StartCommon. That
    //! duplication is intentional for now: it keeps the working tool at zero risk while this path is brought up. Once the
    //! pane is proven, the shared setup should be lifted into a single helper owned by both.
    //!
    //! Keep the two in sync. If a data type, node edit-data setting, or document type is added to
    //! MaterialCanvasApplication, it must be added here too or the pane will silently differ from the standalone tool.
    class MaterialCanvasEditorSystemComponent
        : public AZ::Component
        , private AzToolsFramework::EditorEvents::Bus::Handler
        , private AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(MaterialCanvasEditorSystemComponent, "{8C5764FF-281D-4985-A08C-7E90406D4329}");

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        //! Bus address shared by every Material Canvas system and widget in the Editor process. AtomToolsFramework addresses
        //! all of its buses by tool id specifically so that several tools can coexist in one process without colliding, which
        //! is what makes hosting this in the Editor viable at all. It must stay distinct from any other tool's id.
        static const AZ::Crc32 ToolId;

        //! The pane window is default-constructed by AzToolsFramework::RegisterViewPane and has no way to be handed its
        //! dependencies, so it looks the component up instead. This mirrors LandscapeCanvas::GraphContext::SetInstance, which
        //! solves the same problem the same way. Valid between Activate and Deactivate; null outside that window.
        static MaterialCanvasEditorSystemComponent* GetInstance();

        MaterialCanvasEditorSystemComponent();
        ~MaterialCanvasEditorSystemComponent() override;

        //! Called by MaterialCanvasPaneWindow on construction and destruction. The document type view factories need the
        //! window that owns the document tabs, and unlike the standalone application this component does not own it -- the
        //! Editor creates and destroys it as the pane is opened and closed.
        void SetPaneWindow(MaterialCanvasPaneWindow* paneWindow);

        //! Also brings the tool systems up on first use. The pane window asks for these in its constructor's initializer
        //! list, before its body runs, which makes this the earliest reliable hook for lazy initialization.
        AtomToolsFramework::GraphViewSettingsPtr GetGraphViewSettings();

        //! Persists the current Material Canvas registry subtree while the Editor is still running.
        void SaveSettings();

    private:
        MaterialCanvasEditorSystemComponent(const MaterialCanvasEditorSystemComponent&) = delete;
        MaterialCanvasEditorSystemComponent& operator=(const MaterialCanvasEditorSystemComponent&) = delete;

        // AZ::Component overrides...
        void Activate() override;
        void Deactivate() override;

        // AzToolsFramework::EditorEvents::Bus::Handler overrides...
        void NotifyRegisterViews() override;

        // AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler overrides...
        void OnActionContextRegistrationHook() override;
        void OnActionRegistrationHook() override;

        //! Constructs the tool systems if they are not up yet, and does nothing if they are. Called when the pane opens
        //! rather than when the Editor starts, so an Editor session that never opens Material Canvas pays nothing for it.
        void EnsureSystemsInitialized();

        //! Loads settings written by the standalone Material Canvas application before creating the shared settings object.
        void LoadSettings();

        //! Copies the shader build and preview pipeline setreg stubs into the project user registry, mirroring
        //! MaterialCanvasApplication. The pane offers both toggles in its settings dialog but had no equivalent of these, so flipping
        //! one in the Editor set a registry value that nothing ever acted on and the Asset Processor never saw the override.
        void ApplyShaderBuildSettings();
        void ApplyPreviewMaterialPipelineSettings();

        //! Tears the tool systems back down. Idempotent. Called when the pane closes and again on Deactivate, so that
        //! closing the pane actually stops the graph compiler, the asset status reporter thread and the preview viewport
        //! instead of leaving them running for the rest of the Editor session.
        void ReleaseSystems();

        // These mirror the equivalent Init functions on MaterialCanvasApplication.
        void InitDynamicNodeManager();
        void InitDynamicNodeEditData();
        void InitSharedGraphContext();
        void InitGraphViewSettings();
        void InitMaterialGraphDocumentType();
        void InitMaterialGraphNodeDocumentType();
        void InitShaderSourceDataDocumentType();

        AZStd::unique_ptr<AtomToolsFramework::DynamicNodeManager> m_dynamicNodeManager;
        AZStd::unique_ptr<AtomToolsFramework::AssetStatusReporterSystem> m_assetStatusReporterSystem;
        AZStd::unique_ptr<AtomToolsFramework::EntityPreviewViewportSettingsSystem> m_viewportSettingsSystem;
        AZStd::unique_ptr<AtomToolsFramework::AtomToolsDocumentSystem> m_documentSystem;
        AZStd::shared_ptr<GraphModel::GraphContext> m_graphContext;
        AZStd::shared_ptr<AtomToolsFramework::GraphTemplateFileDataCache> m_graphTemplateFileDataCache;
        AtomToolsFramework::GraphViewSettingsPtr m_graphViewSettingsPtr;

        //! Not owned. Valid only while the pane is open.
        MaterialCanvasPaneWindow* m_paneWindow = nullptr;

        static MaterialCanvasEditorSystemComponent* s_instance;
    };
} // namespace MaterialCanvas
