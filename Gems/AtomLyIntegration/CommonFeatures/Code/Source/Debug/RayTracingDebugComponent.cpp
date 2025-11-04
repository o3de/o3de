/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Debug/RayTracingDebugComponent.h>

namespace AZ::Render
{
    RayTracingDebugComponent::RayTracingDebugComponent(const RayTracingDebugComponentConfig& config)
        : BaseClass{ config }
    {
    }

    void RayTracingDebugComponent::Reflect(ReflectContext* context)
    {
        BaseClass::Reflect(context);

        if (auto* serializeContext{ azrtti_cast<SerializeContext*>(context) })
        {
            // clang-format off
            serializeContext->Class<RayTracingDebugComponent, BaseClass>()
                ->Version(0)
            ;
            // clang-format on
        }
    }
} // namespace AZ::Render
