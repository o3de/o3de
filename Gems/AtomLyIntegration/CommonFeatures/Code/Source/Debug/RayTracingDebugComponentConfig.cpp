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
        if (auto* serializeContext{ azrtti_cast<SerializeContext*>(context) })
        {
            // clang-format off
            serializeContext->Class<RayTracingDebugComponentConfig, ComponentConfig>()
                ->Version(0)
                // Auto-gen serialize context code
                #define SERIALIZE_CLASS RayTracingDebugComponentConfig
                #include <Atom/Feature/ParamMacros/StartParamSerializeContext.inl>
                #include <Atom/Feature/Debug/RayTracingDebugParams.inl>
                #include <Atom/Feature/ParamMacros/EndParams.inl>
                #undef SERIALIZE_CLASS
            ;
            // clang-format on
        }
    }

    void RayTracingDebugComponentConfig::CopySettingsFrom(RayTracingDebugSettingsInterface* settings)
    {
        if (!settings)
        {
            return;
        }

        // clang-format off
        #define COPY_SOURCE settings
        #include <Atom/Feature/ParamMacros/StartParamCopySettingsFrom.inl>
        #include <Atom/Feature/Debug/RayTracingDebugParams.inl>
        #include <Atom/Feature/ParamMacros/EndParams.inl>
        #undef COPY_SOURCE
        // clang-format on
    }

    void RayTracingDebugComponentConfig::CopySettingsTo(RayTracingDebugSettingsInterface* settings)
    {
        if (!settings)
        {
            return;
        }

        // clang-format off
        #define COPY_TARGET settings
        #include <Atom/Feature/ParamMacros/StartParamCopySettingsTo.inl>
        #include <Atom/Feature/Debug/RayTracingDebugParams.inl>
        #include <Atom/Feature/ParamMacros/EndParams.inl>
        #undef COPY_TARGET
        // clang-format on
    }
} // namespace AZ::Render
