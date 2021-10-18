/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>

namespace Profiler
{
    class ProfilerImGuiRequests
    {
    public:
        AZ_RTTI(ProfilerImGuiRequests, "{E0443400-D108-4D3F-8FF5-4F076FCF6D13}");
        virtual ~ProfilerImGuiRequests() = default;

        // special request to render the CPU profiler window in a non-standard way
        // e.g not through ImGuiUpdateListenerBus::OnImGuiUpdate
        virtual void ShowCpuProfilerWindow(bool& keepDrawing) = 0;
    };

    class ProfilerImGuiBusTraits
        : public AZ::EBusTraits
    {
    public:
        // EBusTraits overrides
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    };

    using ProfilerImGuiInterface = AZ::Interface<ProfilerImGuiRequests>;
    using ProfilerImGuiRequestBus = AZ::EBus<ProfilerImGuiRequests, ProfilerImGuiBusTraits>;
} // namespace Profiler
