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
