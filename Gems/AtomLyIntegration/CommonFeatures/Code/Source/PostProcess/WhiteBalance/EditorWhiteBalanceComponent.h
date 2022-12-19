/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <PostProcess/WhiteBalance/WhiteBalanceComponent.h>

namespace AZ
{
    namespace Render
    {
        namespace WhiteBalance
        {
            static constexpr const char* const EditorWhiteBalanceComponentTypeId = "{32C7C44E-AABD-4575-A485-C820BF1F0873}";
        }

        class EditorWhiteBalanceComponent final
            : public AzToolsFramework::Components::
                  EditorComponentAdapter<WhiteBalanceComponentController, WhiteBalanceComponent, WhiteBalanceComponentConfig>
        {
        public:
            using BaseClass = AzToolsFramework::Components::
                EditorComponentAdapter<WhiteBalanceComponentController, WhiteBalanceComponent, WhiteBalanceComponentConfig>;
            AZ_EDITOR_COMPONENT(AZ::Render::EditorWhiteBalanceComponent, WhiteBalance::EditorWhiteBalanceComponentTypeId, BaseClass);

            static void Reflect(AZ::ReflectContext* context);

            EditorWhiteBalanceComponent() = default;
            EditorWhiteBalanceComponent(const WhiteBalanceComponentConfig& config);

            //! EditorRenderComponentAdapter overrides...
            AZ::u32 OnConfigurationChanged() override;
        };

    } // namespace Render
} // namespace AZ
