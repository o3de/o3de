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

    //! Hosts Material Canvas as an Editor view pane, sharing the Editor process; keep in sync with MaterialCanvasApplication::StartCommon.
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

        //! Bus address for every Material Canvas system in the Editor; tool-id addressing lets tools coexist, so keep it unique.
        static const AZ::Crc32 ToolId;

        //! The default-constructed pane looks the component up here, as LandscapeCanvas does; null outside Activate/Deactivate.
        static MaterialCanvasEditorSystemComponent* GetInstance();

        MaterialCanvasEditorSystemComponent();
        ~MaterialCanvasEditorSystemComponent() override;

        //! Called by MaterialCanvasPaneWindow on construction and destruction; the view factories need the Editor-owned window.
        void SetPaneWindow(MaterialCanvasPaneWindow* paneWindow);

        //! Also brings the tool systems up on first use; the pane requests these in its initializer list, the earliest hook.
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

        //! Constructs the tool systems if needed; called when the pane opens so Editor sessions that never open it pay nothing.
        void EnsureSystemsInitialized();

        //! Loads settings written by the standalone Material Canvas application before creating the shared settings object.
        void LoadSettings();

        //! Copies the shader build and preview pipeline setreg stubs into the project user registry, as MaterialCanvasApplication does.
        void ApplyShaderBuildSettings();
        void ApplyPreviewMaterialPipelineSettings();

        //! Tears the tool systems down (idempotent) when the pane closes and on Deactivate, stopping compiles and background threads.
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
