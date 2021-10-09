/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AtomLyIntegration/CommonFeatures/Material/EditorMaterialSystemComponentRequestBus.h>
#include <AtomToolsFramework/PreviewRenderer/PreviewRenderer.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Component.h>
#include <AzFramework/Application/Application.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/Viewport/ActionBus.h>
#include <Material/MaterialBrowserInteractions.h>

namespace AZ
{
    namespace Render
    {
        //! System component that manages launching and maintaining connections with the material editor.
        class EditorMaterialSystemComponent final
            : public AZ::Component
            , public EditorMaterialSystemComponentRequestBus::Handler
            , public AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler
            , public AzToolsFramework::EditorMenuNotificationBus::Handler
            , public AzToolsFramework::EditorEvents::Bus::Handler
            , public AzFramework::AssetCatalogEventBus::Handler
            , public AzFramework::ApplicationLifecycleEvents::Bus::Handler
        {
        public:
            AZ_COMPONENT(EditorMaterialSystemComponent, "{96652157-DA0B-420F-B49C-0207C585144C}");

            static void Reflect(AZ::ReflectContext* context);

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        protected:
            // AZ::Component interface overrides...
            void Init() override;
            void Activate() override;
            void Deactivate() override;

        private:
            //! EditorMaterialSystemComponentRequestBus::Handler overrides...
            void OpenMaterialEditor(const AZStd::string& sourcePath) override;
            void OpenMaterialInspector(const AZ::EntityId& entityId, const AZ::Render::MaterialAssignmentId& materialAssignmentId) override;
            void RenderMaterialPreview(
                const AZ::EntityId& entityId, const AZ::Render::MaterialAssignmentId& materialAssignmentId) override;

            //! AssetBrowserInteractionNotificationBus::Handler overrides...
            AzToolsFramework::AssetBrowser::SourceFileDetails GetSourceFileDetails(const char* fullSourceFileName) override;

            // EditorMenuNotificationBus::Handler overrides ...
            void OnPopulateToolMenuItems() override;
            void OnResetToolMenuItems() override;

            // AztoolsFramework::EditorEvents::Bus::Handler overrides...
            void NotifyRegisterViews() override;


            // AzFramework::AssetCatalogEventBus::Handler overrides ...
            void OnCatalogLoaded(const char* catalogFile) override;

            // AzFramework::ApplicationLifecycleEvents overrides...
            void OnApplicationAboutToStop() override;

            QAction* m_openMaterialEditorAction = nullptr;
            AZStd::unique_ptr<MaterialBrowserInteractions> m_materialBrowserInteractions;
            AZStd::unique_ptr<AtomToolsFramework::PreviewRenderer> m_previewRenderer;
        };
    } // namespace Render
} // namespace AZ
