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
#include <Components/MixedGradientComponent.h>

namespace GradientSignal
{
    template<>
    struct HasCustomSetSamplerOwner<MixedGradientConfig>
        : AZStd::true_type
    {
    };

    template<typename T>
    bool ValidateGradientEntityIds(T& configuration, AZStd::enable_if_t<AZStd::is_same<T, MixedGradientConfig>::value>* = nullptr)
    {
        bool validated = true;

        for (auto& layer : configuration.m_layers)
        {
            validated &= layer.m_gradientSampler.ValidateGradientEntityId();
        }

        return validated;
    }

    template<typename T>
    void SetSamplerOwnerEntity(T& configuration, AZ::EntityId entityId, AZStd::enable_if_t<AZStd::is_same<T, MixedGradientConfig>::value>* = nullptr)
    {
        for (auto& layer : configuration.m_layers)
        {
            layer.m_gradientSampler.m_ownerEntityId = entityId;
        }
    }

    class EditorMixedGradientComponent
        : public EditorGradientComponentBase<MixedGradientComponent, MixedGradientConfig>
    {
    public:
        using BaseClassType = EditorGradientComponentBase<MixedGradientComponent, MixedGradientConfig>;
        AZ_EDITOR_COMPONENT(EditorMixedGradientComponent, EditorMixedGradientComponentTypeId, BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        // AZ::Component overrides ...
        void Init() override;
        void Activate() override;

        static constexpr const char* const s_categoryName = "Gradient Modifiers";
        static constexpr const char* const s_componentName = "Gradient Mixer";
        static constexpr const char* const s_componentDescription = "Generates a new gradient by combining other gradients";
        static constexpr const char* const s_icon = "Editor/Icons/Components/GradientModifier.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/GradientModifier.png";
        static constexpr const char* const s_helpUrl = "https://docs.aws.amazon.com/console/lumberyard/gradientmodifiers/gradient-mixer";

    protected:

        AZ::u32 ConfigurationChanged() override;

    private:
        void ForceOneEntry();
    };
}
