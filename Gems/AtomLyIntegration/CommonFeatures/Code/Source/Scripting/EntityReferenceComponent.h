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
