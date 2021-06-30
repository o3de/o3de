/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/PostFxLayerComponentConstants.h>
#include <PostProcess/PostFxLayerComponent.h>

namespace AZ
{
    namespace Render
    {
        class EditorPostFxLayerComponent final
            : public AzToolsFramework::Components::EditorComponentAdapter<PostFxLayerComponentController, PostFxLayerComponent, PostFxLayerComponentConfig>
        {
        public:

            using BaseClass = AzToolsFramework::Components::EditorComponentAdapter<PostFxLayerComponentController, PostFxLayerComponent, PostFxLayerComponentConfig>;
            AZ_EDITOR_COMPONENT(AZ::Render::EditorPostFxLayerComponent, EditorPostFxLayerComponentTypeId, BaseClass);

            static void Reflect(AZ::ReflectContext* context);

            EditorPostFxLayerComponent() = default;
            EditorPostFxLayerComponent(const PostFxLayerComponentConfig& config);

            //! EditorRenderComponentAdapter overrides...
            AZ::u32 OnConfigurationChanged() override;
        };

    } // namespace Render
} // namespace AZ
