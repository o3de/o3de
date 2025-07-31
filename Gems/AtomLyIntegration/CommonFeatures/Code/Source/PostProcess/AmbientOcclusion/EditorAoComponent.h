/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <Atom/Feature/PostProcess/AmbientOcclusion/AoConstants.h>
#include <Atom/Feature/PostProcess/AmbientOcclusion/SsaoConstants.h>
#include <Atom/Feature/PostProcess/AmbientOcclusion/GtaoConstants.h>
#include <PostProcess/AmbientOcclusion/AoComponent.h>

namespace AZ
{
    namespace Render
    {
        namespace Ao
        {
            inline constexpr AZ::TypeId EditorAoComponentTypeId{ "{5A807489-4FB2-4421-A4D2-9D9E523ABF83}" };
        }

        class EditorAoComponent final
            : public AzToolsFramework::Components::EditorComponentAdapter<AoComponentController, AoComponent, AoComponentConfig>
        {
        public:

            using BaseClass = AzToolsFramework::Components::EditorComponentAdapter<AoComponentController, AoComponent, AoComponentConfig>;
            AZ_EDITOR_COMPONENT(AZ::Render::EditorAoComponent, Ao::EditorAoComponentTypeId, BaseClass);

            static void Reflect(AZ::ReflectContext* context);

            EditorAoComponent() = default;
            EditorAoComponent(const AoComponentConfig& config);

            //! EditorRenderComponentAdapter overrides...
            AZ::u32 OnConfigurationChanged() override;
        };

    } // namespace Render
} // namespace AZ
