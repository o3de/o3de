/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ReflectionProbe/ReflectionProbeComponentController.h>
#include <ReflectionProbe/ReflectionProbeComponentConstants.h>
#include <AzFramework/Components/ComponentAdapter.h>

namespace AZ
{
    namespace Render
    {
        class ReflectionProbeComponent final
            : public AzFramework::Components::ComponentAdapter<ReflectionProbeComponentController, ReflectionProbeComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<ReflectionProbeComponentController, ReflectionProbeComponentConfig>;
            AZ_COMPONENT(AZ::Render::ReflectionProbeComponent, ReflectionProbeComponentTypeId, BaseClass);

            ReflectionProbeComponent() = default;
            ReflectionProbeComponent(const ReflectionProbeComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };

    } // namespace Render
} // namespace AZ
