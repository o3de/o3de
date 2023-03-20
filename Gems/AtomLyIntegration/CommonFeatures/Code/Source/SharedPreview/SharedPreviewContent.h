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
#include <AtomToolsFramework/PreviewRenderer/PreviewContent.h>

namespace AZ
{
    namespace LyIntegration
    {
        //! Creates a simple scene used for most previews and thumbnails
        class SharedPreviewContent final : public AtomToolsFramework::PreviewContent
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

        private:
            void UpdateModel();
            void UpdateLighting();
            void UpdateCamera();

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
        };
    } // namespace LyIntegration
} // namespace AZ
