/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>

namespace AZ::Render
{
    class RayTracingDebugRequests : public ComponentBus
    {
    public:
        AZ_RTTI(RayTracingDebugRequests, "{86A36D89-6799-440C-AEFE-0E42CE4C0A20}");

        static const EBusHandlerPolicy HandlerPolicy{ EBusHandlerPolicy::Single };

        virtual ~RayTracingDebugRequests()
        {
        }

        // clang-format off
        // Generate virtual getters and setters
        #include <Atom/Feature/ParamMacros/StartParamFunctionsVirtual.inl>
        #include <Atom/Feature/Debug/RayTracingDebugParams.inl>
        #include <Atom/Feature/ParamMacros/EndParams.inl>
        // clang-format on
    };

    using RayTracingDebugRequestBus = AZ::EBus<RayTracingDebugRequests>;
} // namespace AZ::Render
