/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <GradientGlobalIllumination/GradientGlobalIlluminationComponent.h>
#include <Atom/Feature/Utils/EditorRenderComponentAdapter.h>
#include <AtomLyIntegration/CommonFeatures/GradientGlobalIllumination/GradientGlobalIlluminationComponentConstants.h>
#include <AtomLyIntegration/CommonFeatures/GradientGlobalIllumination/GradientGlobalIlluminationComponentBus.h>

namespace AZ::Render
{
    class EditorGradientGlobalIlluminationComponent final
        : public EditorRenderComponentAdapter<GradientGlobalIlluminationComponentController, GradientGlobalIlluminationComponent, GradientGlobalIlluminationComponentConfig>
    {
    public:
        using BaseClass = EditorRenderComponentAdapter<GradientGlobalIlluminationComponentController, GradientGlobalIlluminationComponent, GradientGlobalIlluminationComponentConfig>;
        AZ_EDITOR_COMPONENT(AZ::Render::EditorGradientGlobalIlluminationComponent, EditorGradientGlobalIlluminationComponentTypeId, BaseClass);

        static void Reflect(AZ::ReflectContext* context);

        EditorGradientGlobalIlluminationComponent() = default;
        EditorGradientGlobalIlluminationComponent(const GradientGlobalIlluminationComponentConfig& config);

    private:
        //! Snap CPU resolution before reactivation and inspector refresh.
        AZ::u32 OnConfigurationChanged() override;
    };
} // namespace AZ::Render
