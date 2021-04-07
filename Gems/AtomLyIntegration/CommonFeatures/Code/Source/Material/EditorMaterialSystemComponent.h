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

#include <AzCore/Component/Component.h>

#include <AzFramework/Application/Application.h>
#include <AzFramework/TargetManagement/TargetManagementAPI.h>

#include <AzToolsFramework/Thumbnails/Thumbnail.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/Viewport/ActionBus.h>

#include <AtomLyIntegration/CommonFeatures/Material/EditorMaterialSystemComponentRequestBus.h>

namespace AZ
{
    namespace Render
    {
        //! System component that manages launching and maintaining connections with the material editor.
        class EditorMaterialSystemComponent
            : public AZ::Component
            , private EditorMaterialSystemComponentRequestBus::Handler
            , private AzFramework::TargetManagerClient::Bus::Handler
            , private AzFramework::ApplicationLifecycleEvents::Bus::Handler
            , public AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler
            , public AzToolsFramework::EditorMenuNotificationBus::Handler
        {
        public:
            AZ_COMPONENT(EditorMaterialSystemComponent, "{96652157-DA0B-420F-B49C-0207C585144C}");

            static void Reflect(AZ::ReflectContext* context);

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        protected:
            // AZ::Component interface overrides...
            void Init() override;
            void Activate() override;
            void Deactivate() override;

        private:
            //! EditorMaterialSystemComponentRequestBus::Handler overrides...
            void OpenInMaterialEditor(const AZStd::string& sourcePath) override;

            //! AzFramework::TargetManagerClient::Bus::Handler overrides...
            void TargetJoinedNetwork(AzFramework::TargetInfo info) override;
            void TargetLeftNetwork(AzFramework::TargetInfo info) override;
            
            // AzFramework::ApplicationLifecycleEvents overrides...
            void OnApplicationAboutToStop() override;

            //! AssetBrowserInteractionNotificationBus::Handler overrides...
            AzToolsFramework::AssetBrowser::SourceFileDetails GetSourceFileDetails(const char* fullSourceFileName) override;

            // EditorMenuNotificationBus::Handler overrides ...
            void OnPopulateToolMenuItems() override;
            void OnResetToolMenuItems() override;

            void SetupThumbnails();
            void TeardownThumbnails();

            // Material Editor target for interprocess communication with MaterialEditor
            AzFramework::TargetInfo m_materialEditorTarget;

            QAction* m_openMaterialEditorAction = nullptr;
        };
    } // namespace Render
} // namespace AZ
