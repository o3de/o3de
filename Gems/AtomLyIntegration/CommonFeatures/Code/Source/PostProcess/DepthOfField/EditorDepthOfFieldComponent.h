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
#include <Atom/Feature/PostProcess/DepthOfField/DepthOfFieldConstants.h>
#include <PostProcess/DepthOfField/DepthOfFieldComponent.h>

namespace AZ
{
    namespace Render
    {
        namespace DepthOfField
        {
            static constexpr const char* const EditorDepthOfFieldComponentTypeId = "{E9B85017-18F3-4CD6-9EEC-221B0E6B0619}";
        }

        class EditorDepthOfFieldComponent final
            : public AzToolsFramework::Components::EditorComponentAdapter<DepthOfFieldComponentController, DepthOfFieldComponent, DepthOfFieldComponentConfig>
        {
        public:

            using BaseClass = AzToolsFramework::Components::EditorComponentAdapter<DepthOfFieldComponentController, DepthOfFieldComponent, DepthOfFieldComponentConfig>;
            AZ_EDITOR_COMPONENT(AZ::Render::EditorDepthOfFieldComponent, DepthOfField::EditorDepthOfFieldComponentTypeId, BaseClass);

            static void Reflect(AZ::ReflectContext* context);

            EditorDepthOfFieldComponent() = default;
            EditorDepthOfFieldComponent(const DepthOfFieldComponentConfig& config);

            //! EditorRenderComponentAdapter overrides...
            AZ::u32 OnConfigurationChanged() override;
        };

    } // namespace Render
} // namespace AZ
