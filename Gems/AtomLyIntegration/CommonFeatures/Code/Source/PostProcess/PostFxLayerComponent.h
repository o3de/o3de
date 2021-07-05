/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Components/ComponentAdapter.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/PostFxLayerComponentConfig.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/PostFxLayerComponentConstants.h>
#include <PostProcess/PostFxLayerComponentController.h>

namespace AZ
{
    namespace Render
    {
        class PostFxLayerComponent final
            : public AzFramework::Components::ComponentAdapter<PostFxLayerComponentController, PostFxLayerComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<PostFxLayerComponentController, PostFxLayerComponentConfig>;
            AZ_COMPONENT(AZ::Render::PostFxLayerComponent, PostFxLayerComponentTypeId , BaseClass);

            PostFxLayerComponent() = default;
            PostFxLayerComponent(const PostFxLayerComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    } // namespace Render
} // namespace AZ
