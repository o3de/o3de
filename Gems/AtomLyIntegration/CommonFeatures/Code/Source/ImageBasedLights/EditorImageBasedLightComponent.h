/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ImageBasedLights/ImageBasedLightComponent.h>
#include <Atom/Feature/Utils/EditorRenderComponentAdapter.h>
#include <AtomLyIntegration/CommonFeatures/ImageBasedLights/ImageBasedLightComponentConstants.h>
#include <AtomLyIntegration/CommonFeatures/ImageBasedLights/ImageBasedLightComponentBus.h>

namespace AZ
{
    namespace Render
    {
        //! In-editor image based lighting component
        class EditorImageBasedLightComponent final
            : public EditorRenderComponentAdapter<ImageBasedLightComponentController, ImageBasedLightComponent, ImageBasedLightComponentConfig>
        {
        public:
            using BaseClass = EditorRenderComponentAdapter<ImageBasedLightComponentController, ImageBasedLightComponent, ImageBasedLightComponentConfig>;
            AZ_EDITOR_COMPONENT(AZ::Render::EditorImageBasedLightComponent, EditorImageBasedLightComponentTypeId, BaseClass);

            static void Reflect(AZ::ReflectContext* context);

            EditorImageBasedLightComponent() = default;
            EditorImageBasedLightComponent(const ImageBasedLightComponentConfig& config);

            // AZ::Component overrides
            void Activate() override;

            AZ::u32 OnDiffuseImageAssetChanged();
            AZ::u32 OnSpecularImageAssetChanged();
            AZ::u32 OnExposureChanged();

        private:

            bool UpdateImageAsset(Data::Asset<RPI::StreamingImageAsset>& asset1, const char* asset1Suffix, const char* asset1Name,
                                  Data::Asset<RPI::StreamingImageAsset>& asset2, const char* asset2Suffix, const char* asset2Name);

            Data::Asset<RPI::StreamingImageAsset> m_specularImageAsset;
            Data::Asset<RPI::StreamingImageAsset> m_diffuseImageAsset;
            float m_exposure = 0.0f;
        };
    } // namespace Render
} // namespace AZ
