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
