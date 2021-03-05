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

#include <Components/GradientTransformComponent.h>
#include <LmbrCentral/Component/EditorWrappedComponentBase.h>

namespace GradientSignal
{
    class EditorGradientTransformComponent
        : public LmbrCentral::EditorWrappedComponentBase<GradientTransformComponent, GradientTransformConfig>
        , private LmbrCentral::DependencyNotificationBus::Handler
    {
    public:
        using BaseClassType = LmbrCentral::EditorWrappedComponentBase<GradientTransformComponent, GradientTransformConfig>;
        AZ_EDITOR_COMPONENT(EditorGradientTransformComponent, "{33B2AEB0-DD12-44E8-AAF0-5B227D3703FF}", BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        // AZ::Component interface
        void Activate() override;
        void Deactivate() override;

        //////////////////////////////////////////////////////////////////////////
        // DependencyNotificationBus
        void OnCompositionChanged() override;

        static constexpr const char* const s_categoryName = "Gradient Modifiers";
        static constexpr const char* const s_componentName = "Gradient Transform Modifier";
        static constexpr const char* const s_componentDescription = "Transforms coordinates into a space relative to a shape, allowing other transform and sampling modifications";
        static constexpr const char* const s_icon = "Editor/Icons/Components/GradientModifier.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/GradientModifier.png";
        static constexpr const char* const s_helpUrl = "https://docs.aws.amazon.com/console/lumberyard/gradients/gradient-transform-modifier";

    private:
        AZ::u32 ConfigurationChanged() override;

        void UpdateFromShape();
    };
} //namespace GradientSignal