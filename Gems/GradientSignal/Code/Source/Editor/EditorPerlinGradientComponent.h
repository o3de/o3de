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

#include <AzCore/Module/Module.h>
#include <GradientSignal/Editor/EditorGradientComponentBase.h>
#include <Components/PerlinGradientComponent.h>

namespace GradientSignal
{
    class EditorPerlinGradientComponent
        : public EditorGradientComponentBase<PerlinGradientComponent, PerlinGradientConfig>
    {
    public:
        using BaseClassType = EditorGradientComponentBase<PerlinGradientComponent, PerlinGradientConfig>;
        AZ_EDITOR_COMPONENT(EditorPerlinGradientComponent, EditorPerlinGradientComponentTypeId, BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        static constexpr const char* const s_categoryName = "Gradients";
        static constexpr const char* const s_componentName = "Perlin Noise Gradient";
        static constexpr const char* const s_componentDescription = "Generates a gradient by sampling a perlin noise generator";
        static constexpr const char* const s_icon = "Editor/Icons/Components/Gradient.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/Gradient.png";
        static constexpr const char* const s_helpUrl = "https://docs.aws.amazon.com/console/lumberyard/gradients/perlin-noise-gradient";

    private:
        AZ::Crc32 OnGenerateRandomSeed();
    };
}
