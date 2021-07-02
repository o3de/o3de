/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ImageBasedLights/ImageBasedLightComponentController.h>
#include <AtomLyIntegration/CommonFeatures/ImageBasedLights/ImageBasedLightComponentConstants.h>
#include <AzFramework/Components/ComponentAdapter.h>

namespace AZ
{
    namespace Render
    {
        class ImageBasedLightComponent final
            : public AzFramework::Components::ComponentAdapter<ImageBasedLightComponentController, ImageBasedLightComponentConfig>
        {
        public:

            using BaseClass = AzFramework::Components::ComponentAdapter<ImageBasedLightComponentController, ImageBasedLightComponentConfig>;
            AZ_COMPONENT(AZ::Render::ImageBasedLightComponent, ImageBasedLightComponentTypeId, BaseClass);

            ImageBasedLightComponent() = default;
            ImageBasedLightComponent(const ImageBasedLightComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    } // namespace Render
} // namespace AZ
