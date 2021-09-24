/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>

#include <AzToolsFramework/API/EditorViewportIconDisplayInterface.h>

#include <Atom/RPI.Public/DynamicDraw/DynamicDrawInterface.h>
#include <Atom/Bootstrap/BootstrapNotificationBus.h>

#include <QImage>
#include <QSize>
#include <QString>

namespace AZ
{
    class TickRequests;

    namespace Render
    {
        class AtomViewportDisplayIconsSystemComponent
            : public AZ::Component
            , public AzToolsFramework::EditorViewportIconDisplayInterface
            , public AZ::Render::Bootstrap::NotificationBus::Handler
            , private Data::AssetBus::Handler
        {
        public:
            AZ_COMPONENT(AtomViewportDisplayIconsSystemComponent, "{AEC1D3E1-1D9A-437A-B4C6-CFAEE620C160}");

            static void Reflect(AZ::ReflectContext* context);

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        protected:
            // AZ::Component overrides...
            void Activate() override;
            void Deactivate() override;

            // AzToolsFramework::EditorViewportIconDisplayInterface overrides...
            void DrawIcon(const DrawParameters& drawParameters) override;
            IconId GetOrLoadIconForPath(AZStd::string_view path) override;
            IconLoadStatus GetIconLoadStatus(IconId icon) override;

            // AZ::Render::Bootstrap::NotificationBus::Handler overrides...
            void OnBootstrapSceneReady(AZ::RPI::Scene* bootstrapScene) override;

            // Data::AssetBus::Handler overrides...
            void OnAssetReady(Data::Asset<Data::AssetData> asset) override;

        private:
            static constexpr const char* DrawContextShaderPath = "Shaders/TexturedIcon.azshader";
            static constexpr QSize MinimumRenderedSvgSize = QSize(128, 128);
            static constexpr QImage::Format QtImageFormat = QImage::Format_RGBA8888;

            QString FindAssetPath(const QString& path) const;
            QImage RenderSvgToImage(const QString& svgPath) const;
            AZ::Data::Instance<AZ::RPI::Image> ConvertToAtomImage(AZ::Uuid assetId, QImage image) const;

            Name m_drawContextName = Name("ViewportIconDisplay");
            bool m_shaderIndexesInitialized = false;
            RHI::ShaderInputNameIndex m_textureParameterIndex = "m_texture";
            RHI::ShaderInputNameIndex m_viewportSizeIndex = "m_viewportSize";

            struct IconData
            {
                AZStd::string m_path;
                AZ::Data::Instance<AZ::RPI::Image> m_image = nullptr;
            };
            AZStd::unordered_map<IconId, IconData> m_iconData;
            IconId m_currentId = 0;

            bool m_drawContextRegistered = false;
        };
    } // namespace Render
} // namespace AZ
