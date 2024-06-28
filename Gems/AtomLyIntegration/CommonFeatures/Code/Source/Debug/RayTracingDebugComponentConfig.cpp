/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomLyIntegration/CommonFeatures/Debug/RayTracingDebugComponentConfig.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ::Render
{
    void RayTracingDebugComponentConfig::Reflect(ReflectContext* context)
    {
        if (auto serializeContext{ azrtti_cast<SerializeContext*>(context) })
        {
            // clang-format off
            serializeContext->Class<RayTracingDebugComponentConfig, ComponentConfig>()
                ->Version(0)
                ->Field("Enabled", &RayTracingDebugComponentConfig::m_enabled)
            ;
            // clang-format on
        }
    }

    void RayTracingDebugComponentConfig::CopySettingsFrom(RayTracingDebugSettingsInterface* settings)
    {
        m_enabled = settings->GetEnabled();
    }

    void RayTracingDebugComponentConfig::CopySettingsTo(RayTracingDebugSettingsInterface* settings)
    {
        settings->SetEnabled(m_enabled);
    }
} // namespace AZ::Render
