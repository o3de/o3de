/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <SpecularReflections/SpecularReflectionsComponent.h>

namespace AZ
{
    namespace Render
    {
        class EditorSpecularReflectionsComponent final
            : public AzToolsFramework::Components::EditorComponentAdapter<SpecularReflectionsComponentController, SpecularReflectionsComponent, SpecularReflectionsComponentConfig>
        {
        public:
            using BaseClass = AzToolsFramework::Components::EditorComponentAdapter<SpecularReflectionsComponentController, SpecularReflectionsComponent, SpecularReflectionsComponentConfig>;
            AZ_EDITOR_COMPONENT(AZ::Render::EditorSpecularReflectionsComponent, EditorSpecularReflectionsComponentTypeId, BaseClass);

            static void Reflect(AZ::ReflectContext* context);

            EditorSpecularReflectionsComponent() = default;
            EditorSpecularReflectionsComponent(const SpecularReflectionsComponentConfig& config);

            //! EditorRenderComponentAdapter overrides...
            AZ::u32 OnConfigurationChanged() override;

        private:
            AZ::u32 GetSSRVisibilitySetting();
        };

    } // namespace Render
} // namespace AZ
