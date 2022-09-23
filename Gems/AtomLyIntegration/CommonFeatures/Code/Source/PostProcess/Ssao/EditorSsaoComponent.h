/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            inline constexpr AZ::TypeId EditorSsaoComponentTypeId{ "{5A807489-4FB2-4421-A4D2-9D9E523ABF83}" };
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
