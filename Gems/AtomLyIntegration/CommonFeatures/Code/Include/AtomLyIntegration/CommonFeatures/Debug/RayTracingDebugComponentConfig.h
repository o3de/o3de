/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/Debug/RayTracingDebugSettingsInterface.h>
#include <AzCore/Component/ComponentBus.h>

namespace AZ::Render
{
    class RayTracingDebugComponentConfig final : public ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(RayTracingDebugComponentConfig, SystemAllocator)
        AZ_RTTI(RayTracingDebugComponentConfig, "{856B64FB-A019-47B8-A9D4-F74E18BE847A}", ComponentConfig);

        static void Reflect(ReflectContext* context);

        // clang-format off
        // Generate members
        #include <Atom/Feature/ParamMacros/StartParamMembers.inl>
        #include <Atom/Feature/Debug/RayTracingDebugParams.inl>
        #include <Atom/Feature/ParamMacros/EndParams.inl>

        // Generate Getters/Setters
        #include <Atom/Feature/ParamMacros/StartParamFunctions.inl>
        #include <Atom/Feature/Debug/RayTracingDebugParams.inl>
        #include <Atom/Feature/ParamMacros/EndParams.inl>
        // clang-format on

        void CopySettingsFrom(RayTracingDebugSettingsInterface* settings);
        void CopySettingsTo(RayTracingDebugSettingsInterface* settings);
    };
} // namespace AZ::Render
