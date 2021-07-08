/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <Scripting/EntityReferenceComponent.h>
#include <AzCore/std/containers/vector.h>

namespace AZ
{
    namespace Render
    {
        class EditorEntityReferenceComponent final
            : public AzToolsFramework::Components::EditorComponentAdapter<EntityReferenceComponentController, EntityReferenceComponent, EntityReferenceComponentConfig>
        {
        public:
            using BaseClass = AzToolsFramework::Components::EditorComponentAdapter<EntityReferenceComponentController, EntityReferenceComponent, EntityReferenceComponentConfig>;
            AZ_EDITOR_COMPONENT(AZ::Render::EditorEntityReferenceComponent, EditorEntityReferenceComponentTypeId, BaseClass);

            static void Reflect(AZ::ReflectContext* context);

            EditorEntityReferenceComponent() = default;
            EditorEntityReferenceComponent(const EntityReferenceComponentConfig& config);

            AZ::u32 OnConfigurationChanged() override;
        };
    }
}
