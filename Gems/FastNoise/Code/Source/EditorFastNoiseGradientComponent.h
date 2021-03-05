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
        static constexpr const char* const s_helpUrl = "https://docs.aws.amazon.com/console/lumberyard/gradients/fast-noise-gradient";

    private:
        AZ::Crc32 OnGenerateRandomSeed();
    };
} //namespace FastNoiseGem