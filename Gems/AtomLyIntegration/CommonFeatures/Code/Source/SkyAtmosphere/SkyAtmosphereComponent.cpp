/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SkyAtmosphere/SkyAtmosphereComponent.h>

namespace AZ::Render
{
    SkyAtmosphereComponent::SkyAtmosphereComponent(const SkyAtmosphereComponentConfig& config)
        : BaseClass(config)
    {
    }

    void SkyAtmosphereComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClass::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SkyAtmosphereComponent, BaseClass>();
        }

        if (auto* behaviorContext = azrtti_cast<BehaviorContext*>(context))
        {
            behaviorContext->Class<SkyAtmosphereComponent>()
                ->RequestBus("SkyAtmosphereRequestBus");
        }
    }
} // namespace AZ::Render
