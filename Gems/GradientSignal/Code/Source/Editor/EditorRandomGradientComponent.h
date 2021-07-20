/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        static constexpr const char* const s_helpUrl = "https://o3de.org/docs/user-guide/components/";

    private:
        AZ::Crc32 OnGenerateRandomSeed();
    };
}
