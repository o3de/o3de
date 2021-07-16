/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <Atom/Feature/PostProcess/DepthOfField/DepthOfFieldConstants.h>
#include <PostProcess/Bloom/BloomComponent.h>

namespace AZ
{
    namespace Render
    {
        namespace Bloom
        {
            static constexpr const char* const EditorBloomComponentTypeId =
                "{33789179-AB9C-4891-9DA3-1972EAED6719}";
        }

        class EditorBloomComponent final
            : public AzToolsFramework::Components::EditorComponentAdapter<BloomComponentController, BloomComponent, BloomComponentConfig>
        {
        public:

            using BaseClass = AzToolsFramework::Components::EditorComponentAdapter<BloomComponentController, BloomComponent, BloomComponentConfig>;
            AZ_EDITOR_COMPONENT(AZ::Render::EditorBloomComponent, Bloom::EditorBloomComponentTypeId, BaseClass);

            static void Reflect(AZ::ReflectContext* context);

            EditorBloomComponent() = default;
            EditorBloomComponent(const BloomComponentConfig& config);

            //! EditorRenderComponentAdapter overrides...
            AZ::u32 OnConfigurationChanged() override;
        };

    } // namespace Render
} // namespace AZ
