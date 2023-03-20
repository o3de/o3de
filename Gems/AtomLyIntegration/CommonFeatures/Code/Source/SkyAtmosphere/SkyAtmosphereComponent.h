/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Components/ComponentAdapter.h>
#include <SkyAtmosphere/SkyAtmosphereComponentController.h>

namespace AZ::Render
{
    class SkyAtmosphereComponent final
        : public AzFramework::Components::ComponentAdapter<SkyAtmosphereComponentController, SkyAtmosphereComponentConfig>
    {
    public:
        using BaseClass = AzFramework::Components::ComponentAdapter<SkyAtmosphereComponentController, SkyAtmosphereComponentConfig>;
        AZ_COMPONENT(AZ::Render::SkyAtmosphereComponent, "{5287C268-2456-42A3-BF91-3B65A517F1B6}" , BaseClass);

        SkyAtmosphereComponent() = default;
        SkyAtmosphereComponent(const SkyAtmosphereComponentConfig& config);

        static void Reflect(AZ::ReflectContext* context);
    };
} // namespace AZ::Render
