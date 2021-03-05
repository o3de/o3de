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
#include <Atom/Feature/PostProcess/Ssao/SsaoConstants.h>
#include <PostProcess/Ssao/SsaoComponent.h>

namespace AZ
{
    namespace Render
    {
        namespace Ssao
        {
            static constexpr const char* const EditorSsaoComponentTypeId = "{5A807489-4FB2-4421-A4D2-9D9E523ABF83}";
        }

        class EditorSsaoComponent final
            : public AzToolsFramework::Components::EditorComponentAdapter<SsaoComponentController, SsaoComponent, SsaoComponentConfig>
        {
        public:

            using BaseClass = AzToolsFramework::Components::EditorComponentAdapter<SsaoComponentController, SsaoComponent, SsaoComponentConfig>;
            AZ_EDITOR_COMPONENT(AZ::Render::EditorSsaoComponent, Ssao::EditorSsaoComponentTypeId, BaseClass);

            static void Reflect(AZ::ReflectContext* context);

            EditorSsaoComponent() = default;
            EditorSsaoComponent(const SsaoComponentConfig& config);

            //! EditorRenderComponentAdapter overrides...
            AZ::u32 OnConfigurationChanged() override;
        };

    } // namespace Render
} // namespace AZ
