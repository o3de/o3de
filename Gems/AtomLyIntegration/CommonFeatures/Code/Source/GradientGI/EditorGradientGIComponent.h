/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <GradientGI/GradientGIComponent.h>
#include <Atom/Feature/Utils/EditorRenderComponentAdapter.h>
#include <AtomLyIntegration/CommonFeatures/GradientGI/GradientGIComponentConstants.h>
#include <AtomLyIntegration/CommonFeatures/GradientGI/GradientGIComponentBus.h>

namespace AZ
{
    namespace Render
    {
        class EditorGradientGIComponent final
            : public EditorRenderComponentAdapter<GradientGIComponentController, GradientGIComponent, GradientGIComponentConfig>
        {
        public:
            using BaseClass = EditorRenderComponentAdapter<GradientGIComponentController, GradientGIComponent, GradientGIComponentConfig>;
            AZ_EDITOR_COMPONENT(AZ::Render::EditorGradientGIComponent, EditorGradientGIComponentTypeId, BaseClass);

            static void Reflect(AZ::ReflectContext* context);

            EditorGradientGIComponent() = default;
            EditorGradientGIComponent(const GradientGIComponentConfig& config);

        private:
            AZ::u32 OnColorChanged();
            AZ::u32 OnExposureChanged();
            AZ::u32 OnResolutionChanged();
        };
    } // namespace Render
} // namespace AZ
