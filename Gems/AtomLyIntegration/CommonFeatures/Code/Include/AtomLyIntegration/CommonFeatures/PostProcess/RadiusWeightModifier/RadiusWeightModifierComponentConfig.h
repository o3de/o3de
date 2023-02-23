/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>

namespace AZ
{
    namespace Render
    {
        class RadiusWeightModifierComponentConfig final
            : public ComponentConfig
        {
        public:
            AZ_CLASS_ALLOCATOR(RadiusWeightModifierComponentConfig, SystemAllocator)
            AZ_RTTI(AZ::Render::RadiusWeightModifierComponentConfig, "{80218725-E3B5-4A01-89FF-11314AB2EFF8}", AZ::ComponentConfig);

            static void Reflect(ReflectContext* context);

            float m_radius = 0;
        };
    }
}
