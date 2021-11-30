/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Component.h>
#include <AzFramework/Application/Application.h>
#include <AzToolsFramework/API/EditorLevelNotificationBus.h>
#include <AzToolsFramework/AssetBrowser/Previewer/PreviewerBus.h>
#include <AzToolsFramework/Entity/SliceEditorEntityOwnershipServiceBus.h>
#include <SharedPreview/SharedPreviewerFactory.h>
#include <SharedPreview/SharedThumbnailRenderer.h>

namespace AZ
{
    namespace Render
    {
        class SkinnedMeshDebugDisplay;

        //! This is the editor-counterpart to this gem's main CommonSystemComponent class.
        class EditorCommonFeaturesSystemComponent
            : public AZ::Component
            , public AzToolsFramework::EditorLevelNotificationBus::Handler
            , public AzToolsFramework::SliceEditorEntityOwnershipServiceNotificationBus::Handler
            , public AzToolsFramework::AssetBrowser::PreviewerRequestBus::Handler
            , public AzFramework::AssetCatalogEventBus::Handler
            , public AzFramework::ApplicationLifecycleEvents::Bus::Handler
        {
        public:
            AZ_COMPONENT(EditorCommonFeaturesSystemComponent, "{D73D77CF-D5AF-428B-909B-324E96D3DEF5}");

            EditorCommonFeaturesSystemComponent();
            ~EditorCommonFeaturesSystemComponent();

            static void Reflect(AZ::ReflectContext* context);

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        protected:
            // AZ::Component interface implementation
            void Init() override;
            void Activate() override;
            void Deactivate() override;

            // EditorLevelNotificationBus overrides ...
            void OnNewLevelCreated() override;

            // SliceEditorEntityOwnershipServiceBus overrides ...
            void OnSliceInstantiated(
                const AZ::Data::AssetId&, AZ::SliceComponent::SliceInstanceAddress&, const AzFramework::SliceInstantiationTicket&) override;
            void OnSliceInstantiationFailed(const AZ::Data::AssetId&, const AzFramework::SliceInstantiationTicket&) override;

            // AzFramework::AssetCatalogEventBus::Handler overrides ...
            void OnCatalogLoaded(const char* catalogFile) override;

            // AzToolsFramework::AssetBrowser::PreviewerRequestBus::Handler overrides...
            const AzToolsFramework::AssetBrowser::PreviewerFactory* GetPreviewerFactory(
                const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry) const override;

            // AzFramework::ApplicationLifecycleEvents overrides...
            void OnApplicationAboutToStop() override;

            void SetupThumbnails();
            void TeardownThumbnails();

        private:
            AZStd::unique_ptr<SkinnedMeshDebugDisplay> m_skinnedMeshDebugDisplay;

            AZ::Data::AssetId m_levelDefaultSliceAssetId;
            AZStd::string m_atomLevelDefaultAssetPath{ "LevelAssets/default.slice" };
            float m_envProbeHeight{ 200.0f };

            AZStd::unique_ptr<AZ::LyIntegration::SharedThumbnailRenderer> m_thumbnailRenderer;
            AZStd::unique_ptr<LyIntegration::SharedPreviewerFactory> m_previewerFactory;
        };
    } // namespace Render
} // namespace AZ
