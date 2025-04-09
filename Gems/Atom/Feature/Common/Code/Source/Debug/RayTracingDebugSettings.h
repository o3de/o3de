/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/Debug/RayTracingDebugSettingsInterface.h>

namespace AZ::Render
{
    // The settings class for the ray tracing debug view
    class RayTracingDebugSettings final : public RayTracingDebugSettingsInterface
    {
    public:
        AZ_CLASS_ALLOCATOR(RayTracingDebugSettings, SystemAllocator);
        AZ_RTTI(RayTracingDebugSettings, "{5EED94CE-79FF-4F78-9ADC-A6D186F346E0}");

        // clang-format off
        // Generate getters and setters
        #include <Atom/Feature/ParamMacros/StartParamFunctionsOverrideImpl.inl>
        #include <Atom/Feature/Debug/RayTracingDebugParams.inl>
        #include <Atom/Feature/ParamMacros/EndParams.inl>
        // clang-format on

    private:
        // clang-format off
        // Generate members
        #include <Atom/Feature/ParamMacros/StartParamMembers.inl>
        #include <Atom/Feature/Debug/RayTracingDebugParams.inl>
        #include <Atom/Feature/ParamMacros/EndParams.inl>
        // clang-format on
    };
} // namespace AZ::Render
