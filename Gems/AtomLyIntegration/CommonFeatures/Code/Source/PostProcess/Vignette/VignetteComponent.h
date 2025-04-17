/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/PostProcess/Vignette/VignetteConstants.h>
#include <PostProcess/Vignette/VignetteComponentController.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/Vignette/VignetteComponentConfig.h>
#include <AzCore/Component/Component.h>
#include <AzFramework/Components/ComponentAdapter.h>

namespace AZ
{
    namespace Render
    {
        namespace Vignette
        {
            static constexpr const char* const VignetteComponentTypeId = "{93C2AD53-4722-4B33-BB23-BDBC1D423289}";
        }

        class VignetteComponent final
            : public AzFramework::Components::ComponentAdapter<VignetteComponentController, VignetteComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<VignetteComponentController, VignetteComponentConfig>;
            AZ_COMPONENT(AZ::Render::VignetteComponent, Vignette::VignetteComponentTypeId, BaseClass);

            VignetteComponent() = default;
            VignetteComponent(const VignetteComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    } // namespace Render
} // namespace AZ
