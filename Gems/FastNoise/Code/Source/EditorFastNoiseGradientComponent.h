/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <GradientSignal/Editor/EditorGradientComponentBase.h>
#include "FastNoiseGradientComponent.h"

namespace FastNoiseGem
{
    class EditorFastNoiseGradientComponent
        : public GradientSignal::EditorGradientComponentBase<FastNoiseGradientComponent, FastNoiseGradientConfig>
    {
    public:
        using BaseClassType = GradientSignal::EditorGradientComponentBase<FastNoiseGradientComponent, FastNoiseGradientConfig>;
        AZ_EDITOR_COMPONENT(EditorFastNoiseGradientComponent, "{FD018DE5-5EB4-4219-9D0C-CB3C55DE656B}", BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        AZ::u32 ConfigurationChanged() override;

        static constexpr const char* const s_categoryName = "Gradients";
        static constexpr const char* const s_componentName = "FastNoise Gradient";
        static constexpr const char* const s_componentDescription = "Generates gradient values using FastNoise a noise generation library with a collection of realtime noise algorithms";
        static constexpr const char* const s_icon = "Editor/Icons/Components/Gradient.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/Gradient.png";
        static constexpr const char* const s_helpUrl = "https://o3de.org/docs/user-guide/components/";

    private:
        AZ::Crc32 OnGenerateRandomSeed();
    };
} //namespace FastNoiseGem
