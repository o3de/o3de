/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <Atom/RPI.Reflect/System/AnyAsset.h>
#include <AtomLyIntegration/CommonFeatures/Material/MaterialAssignment.h>
#include <AtomLyIntegration/CommonFeatures/Mesh/MeshHandleStateBus.h>
#include <AtomToolsFramework/PreviewRenderer/PreviewContent.h>

namespace AZ
{
    namespace LyIntegration
    {
        //! Creates a simple scene used for most previews and thumbnails
        class SharedPreviewContent final : public AtomToolsFramework::PreviewContent, public AZ::Render::MeshHandleStateNotificationBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(SharedPreviewContent, AZ::SystemAllocator);

            SharedPreviewContent(
                RPI::ScenePtr scene,
                RPI::ViewPtr view,
                AZ::Uuid entityContextId,
                const Data::Asset<RPI::ModelAsset>& modelAsset,
                const Data::Asset<RPI::MaterialAsset>& materialAsset,
                const Data::Asset<RPI::AnyAsset>& lightingPresetAsset,
                const Render::MaterialPropertyOverrideMap& materialPropertyOverrides);

            ~SharedPreviewContent() override;

            void Load() override;
            bool IsReady() const override;
            bool IsError() const override;
            void ReportErrors() override;
            void Update() override;
            bool IsReadyToRender() override;

        private:
            void UpdateModel();
            void UpdateLighting();
            void UpdateCamera();

            ///////////////////////////////////////////////////////////////
            //AZ::Render::MeshHandleStateNotificationBus::Handler overrides
            void OnMeshHandleSet(const AZ::Render::MeshFeatureProcessorInterface::MeshHandle* meshHandle) override;
            ///////////////////////////////////////////////////////////////

            // Called by @m_meshUpdatedHandler
            void OnMeshDrawPacketUpdated(
                const AZ::Render::ModelDataInstanceInterface& meshHandleIface,
                uint32_t lodIndex, uint32_t meshIndex, const AZ::RPI::MeshDrawPacket& meshDrawPacket);

            static constexpr float AspectRatio = 1.0f;
            static constexpr float NearDist = 0.001f;
            static constexpr float FarDist = 100.0f;
            static constexpr float FieldOfView = Constants::HalfPi;
            static constexpr float CameraRotationAngle = Constants::QuarterPi / 3.0f;

            RPI::ScenePtr m_scene;
            RPI::ViewPtr m_view;
            AZ::Uuid m_entityContextId;
            Entity* m_modelEntity = {};

            Data::Asset<RPI::ModelAsset> m_modelAsset;
            Data::Asset<RPI::MaterialAsset> m_materialAsset;
            Data::Asset<RPI::AnyAsset> m_lightingPresetAsset;
            Render::MaterialPropertyOverrideMap m_materialPropertyOverrides;

            // We need @m_meshHandle to be able to register for MeshDrawPacketUpdatedEvent(s).
            // These events are the signals we need to have assurance that the scene is fully available
            // on GPU and we are ready to render and generate the Thumbnail.
            const AZ::Render::MeshFeatureProcessorInterface::MeshHandle* m_meshHandle = nullptr;
            AZ::Render::ModelDataInstanceInterface::MeshDrawPacketUpdatedEvent::Handler m_meshUpdatedHandler;
            uint32_t m_meshDrawPacketUpdateCount = 0;
        };
    } // namespace LyIntegration
} // namespace AZ
