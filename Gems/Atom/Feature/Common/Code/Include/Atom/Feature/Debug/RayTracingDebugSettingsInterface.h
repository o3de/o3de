/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/Debug/RayTracingDebugConstants.h>
#include <AzCore/RTTI/RTTI.h>

namespace AZ::Render
{
    class RayTracingDebugSettingsInterface
    {
    public:
        AZ_RTTI(RayTracingDebugSettingsInterface, "{E1C76937-7F82-4553-87AA-D6DC8885DC56}");

        // clang-format off
        // Generate virtual getter and setter functions
        #include <Atom/Feature/ParamMacros/StartParamFunctionsVirtual.inl>
        #include <Atom/Feature/Debug/RayTracingDebugParams.inl>
        #include <Atom/Feature/ParamMacros/EndParams.inl>
        // clang-format on
    };
} // namespace AZ::Render
