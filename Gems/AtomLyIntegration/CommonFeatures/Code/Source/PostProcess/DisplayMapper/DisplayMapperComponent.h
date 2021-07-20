/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzFramework/Components/ComponentAdapter.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/DisplayMapper/DisplayMapperComponentConfig.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/DisplayMapper/DisplayMapperComponentConstants.h>
#include <PostProcess/DisplayMapper/DisplayMapperComponentController.h>

namespace AZ
{
    namespace Render
    {
        class DisplayMapperComponent final
            : public AzFramework::Components::ComponentAdapter<DisplayMapperComponentController, DisplayMapperComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<DisplayMapperComponentController, DisplayMapperComponentConfig>;
            AZ_COMPONENT(AZ::Render::DisplayMapperComponent, DisplayMapperComponentTypeId , BaseClass);

            DisplayMapperComponent() = default;
            DisplayMapperComponent(const DisplayMapperComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    } // namespace Render
} // namespace AZ
