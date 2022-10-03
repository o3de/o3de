/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzFramework/Components/ComponentAdapter.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/DepthOfField/DepthOfFieldComponentConfig.h>
#include <Atom/Feature/PostProcess/DepthOfField/DepthOfFieldConstants.h>
#include <PostProcess/DepthOfField/DepthOfFieldComponentController.h>

namespace AZ
{
    namespace Render
    {
        namespace DepthOfField
        {
            inline constexpr AZ::TypeId DepthOfFieldComponentTypeId{ "{405F505B-D900-461F-A17D-380C294BEE39}" };
        }

        class DepthOfFieldComponent final
            : public AzFramework::Components::ComponentAdapter<DepthOfFieldComponentController, DepthOfFieldComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<DepthOfFieldComponentController, DepthOfFieldComponentConfig>;
            AZ_COMPONENT(AZ::Render::DepthOfFieldComponent, DepthOfField::DepthOfFieldComponentTypeId , BaseClass);

            DepthOfFieldComponent() = default;
            DepthOfFieldComponent(const DepthOfFieldComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    } // namespace Render
} // namespace AZ
