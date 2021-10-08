/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <Atom/RPI.Reflect/System/AnyAsset.h>
#include <AtomToolsFramework/PreviewRenderer/PreviewContent.h>

namespace AZ
{
    namespace LyIntegration
    {
        namespace Thumbnails
        {
            //! Provides custom rendering of material and model previews
            class CommonThumbnailPreviewContent final
                : public AtomToolsFramework::PreviewContent
            {
            public:
                AZ_CLASS_ALLOCATOR(CommonThumbnailPreviewContent, AZ::SystemAllocator, 0);

                CommonThumbnailPreviewContent(
                    RPI::ScenePtr scene,
                    RPI::ViewPtr view,
                    AZ::Uuid entityContextId,
                    const Data::AssetId& modelAssetId,
                    const Data::AssetId& materialAssetId,
                    const Data::AssetId& lightingPresetAssetId);

                ~CommonThumbnailPreviewContent() override;

                void Load() override;
                bool IsReady() const override;
                bool IsError() const override;
                void ReportErrors() override;
                void UpdateScene() override;

            private:
                void UpdateModel();
                void UpdateLighting();
                void UpdateCamera();

                static constexpr float AspectRatio = 1.0f;
                static constexpr float NearDist = 0.001f;
                static constexpr float FarDist = 100.0f;
                static constexpr float FieldOfView = Constants::HalfPi;
                static constexpr float CameraRotationAngle = Constants::QuarterPi / 2.0f;

                RPI::ScenePtr m_scene;
                RPI::ViewPtr m_view;
                AZ::Uuid m_entityContextId;
                Entity* m_modelEntity = nullptr;

                static constexpr const char* DefaultLightingPresetPath = "lightingpresets/thumbnail.lightingpreset.azasset";
                const Data::AssetId DefaultLightingPresetAssetId = AZ::RPI::AssetUtils::GetAssetIdForProductPath(DefaultLightingPresetPath);
                Data::Asset<RPI::AnyAsset> m_defaultLightingPresetAsset;
                Data::Asset<RPI::AnyAsset> m_lightingPresetAsset;

                //! Model asset about to be rendered
                static constexpr const char* DefaultModelPath = "models/sphere.azmodel";
                const Data::AssetId DefaultModelAssetId = AZ::RPI::AssetUtils::GetAssetIdForProductPath(DefaultModelPath);
                Data::Asset<RPI::ModelAsset> m_defaultModelAsset;
                Data::Asset<RPI::ModelAsset> m_modelAsset;

                //! Material asset about to be rendered
                static constexpr const char* DefaultMaterialPath = "materials/basic_grey.azmaterial";
                const Data::AssetId DefaultMaterialAssetId = AZ::RPI::AssetUtils::GetAssetIdForProductPath(DefaultMaterialPath);
                Data::Asset<RPI::MaterialAsset> m_defaultMaterialAsset;
                Data::Asset<RPI::MaterialAsset> m_materialAsset;
            };
        } // namespace Thumbnails
    } // namespace LyIntegration
} // namespace AZ
