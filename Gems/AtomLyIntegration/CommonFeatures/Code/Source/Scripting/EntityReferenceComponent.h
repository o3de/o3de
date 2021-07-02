/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Components/ComponentAdapter.h>
#include <AtomLyIntegration/CommonFeatures/Scripting/EntityReferenceComponentConfig.h>
#include <AtomLyIntegration/CommonFeatures/Scripting/EntityReferenceConstants.h>
#include <Scripting/EntityReferenceComponentController.h>

namespace AZ
{
    namespace Render
    {
        class EntityReferenceComponent final
            : public AzFramework::Components::ComponentAdapter<EntityReferenceComponentController, EntityReferenceComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<EntityReferenceComponentController, EntityReferenceComponentConfig>;
            AZ_COMPONENT(AZ::Render::EntityReferenceComponent, EntityReferenceComponentTypeId, BaseClass);

            EntityReferenceComponent() = default;
            EntityReferenceComponent(const EntityReferenceComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    }
}
