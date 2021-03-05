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
#include <GradientSignal/Editor/EditorGradientTypeIds.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>
#include <Components/RandomGradientComponent.h>

namespace GradientSignal
{
    class EditorRandomGradientComponent
        : public EditorGradientComponentBase<RandomGradientComponent, RandomGradientConfig>
    {
    public:
        using BaseClassType = EditorGradientComponentBase<RandomGradientComponent, RandomGradientConfig>;
        AZ_EDITOR_COMPONENT(EditorRandomGradientComponent, EditorRandomGradientComponentTypeId, BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        static constexpr const char* const s_categoryName = "Gradients";
        static constexpr const char* const s_componentName = "Random Noise Gradient";
        static constexpr const char* const s_componentDescription = "Generates a gradient by sampling a random noise generator";
        static constexpr const char* const s_icon = "Editor/Icons/Components/Gradient.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/Gradient.png";
        static constexpr const char* const s_helpUrl = "https://docs.aws.amazon.com/console/lumberyard/gradients/random-noise-gradient";

    private:
        AZ::Crc32 OnGenerateRandomSeed();
    };
}
