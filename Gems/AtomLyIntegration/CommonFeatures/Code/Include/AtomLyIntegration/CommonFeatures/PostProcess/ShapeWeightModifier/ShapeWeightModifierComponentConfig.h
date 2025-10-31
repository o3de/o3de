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
        class ShapeWeightModifierComponentConfig final
            : public ComponentConfig
        {
        public:
            AZ_CLASS_ALLOCATOR(ShapeWeightModifierComponentConfig, SystemAllocator)
            AZ_RTTI(AZ::Render::ShapeWeightModifierComponentConfig, "{2CB13B7C-C532-4DFB-8FF4-79EF9F860868}", AZ::ComponentConfig);

            static void Reflect(ReflectContext* context);

            float m_falloffDistance = 1.0f;
        };
    }
}
