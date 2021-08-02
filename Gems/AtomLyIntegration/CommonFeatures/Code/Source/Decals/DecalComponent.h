/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/TypeInfo.h>
#include <AzFramework/Components/ComponentAdapter.h>
#include <Decals/DecalComponentController.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AtomLyIntegration/CommonFeatures/Decals/DecalConstants.h>

namespace AZ
{
    namespace Render
    {
        class DecalComponent final
            : public AzFramework::Components::ComponentAdapter<DecalComponentController, DecalComponentConfig>
        {
        public:

            using BaseClass = AzFramework::Components::ComponentAdapter<DecalComponentController, DecalComponentConfig>;
            AZ_COMPONENT(AZ::Render::DecalComponent, DecalComponentTypeId, BaseClass);

            DecalComponent() = default;
            DecalComponent(const DecalComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    } // namespace Render
} // namespace AZ
