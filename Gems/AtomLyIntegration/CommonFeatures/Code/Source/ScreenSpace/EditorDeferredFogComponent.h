/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <ScreenSpace/DeferredFogComponent.h>

namespace AZ
{
    namespace Render
    {
        namespace DeferredFog
        {
            static constexpr const char* const EditorDeferredFogComponentTypeId =
                "{6459274F-54C8-4C22-9448-B2B13B69182C}";
        }

        class EditorDeferredFogComponent final
            : public AzToolsFramework::Components::EditorComponentAdapter<DeferredFogComponentController, DeferredFogComponent, DeferredFogComponentConfig>
        {
        public:

            using BaseClass = AzToolsFramework::Components::EditorComponentAdapter<DeferredFogComponentController, DeferredFogComponent, DeferredFogComponentConfig>;
            AZ_EDITOR_COMPONENT(AZ::Render::EditorDeferredFogComponent, DeferredFog::EditorDeferredFogComponentTypeId, BaseClass);

            static void Reflect(AZ::ReflectContext* context);

            EditorDeferredFogComponent() = default;
            EditorDeferredFogComponent(const DeferredFogComponentConfig& config);

            //! EditorRenderComponentAdapter overrides...
            AZ::u32 OnConfigurationChanged() override;
        };

    } // namespace Render
} // namespace AZ
