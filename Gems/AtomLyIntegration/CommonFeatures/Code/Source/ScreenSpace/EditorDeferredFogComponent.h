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
