/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzFramework/Components/ComponentAdapter.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/ExposureControl/ExposureControlComponentConfig.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/ExposureControl/ExposureControlComponentConstants.h>
#include <Atom/Feature/PostProcess/ExposureControl/ExposureControlConstants.h>
#include <PostProcess/ExposureControl/ExposureControlComponentController.h>

namespace AZ
{
    namespace Render
    {
        class ExposureControlComponent final
            : public AzFramework::Components::ComponentAdapter<ExposureControlComponentController, ExposureControlComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<ExposureControlComponentController, ExposureControlComponentConfig>;
            AZ_COMPONENT(AZ::Render::ExposureControlComponent, ExposureControlComponentTypeId , BaseClass);

            ExposureControlComponent() = default;
            ExposureControlComponent(const ExposureControlComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    } // namespace Render
} // namespace AZ
