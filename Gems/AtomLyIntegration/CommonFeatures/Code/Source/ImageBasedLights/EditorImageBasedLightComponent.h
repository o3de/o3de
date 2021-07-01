/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

        };
    } // namespace Render
} // namespace AZ
