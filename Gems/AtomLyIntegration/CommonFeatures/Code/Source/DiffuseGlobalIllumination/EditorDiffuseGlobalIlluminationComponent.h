/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <DiffuseGlobalIllumination/DiffuseGlobalIlluminationComponent.h>

namespace AZ
{
    namespace Render
    {
        class EditorDiffuseGlobalIlluminationComponent final
            : public AzToolsFramework::Components::EditorComponentAdapter<DiffuseGlobalIlluminationComponentController, DiffuseGlobalIlluminationComponent, DiffuseGlobalIlluminationComponentConfig>
        {
        public:

            using BaseClass = AzToolsFramework::Components::EditorComponentAdapter<DiffuseGlobalIlluminationComponentController, DiffuseGlobalIlluminationComponent, DiffuseGlobalIlluminationComponentConfig>;
            AZ_EDITOR_COMPONENT(AZ::Render::EditorDiffuseGlobalIlluminationComponent, EditorDiffuseGlobalIlluminationComponentTypeId, BaseClass);

            static void Reflect(AZ::ReflectContext* context);

            EditorDiffuseGlobalIlluminationComponent() = default;
            EditorDiffuseGlobalIlluminationComponent(const DiffuseGlobalIlluminationComponentConfig& config);

            //! EditorRenderComponentAdapter overrides...
            AZ::u32 OnConfigurationChanged() override;
        };

    } // namespace Render
} // namespace AZ
