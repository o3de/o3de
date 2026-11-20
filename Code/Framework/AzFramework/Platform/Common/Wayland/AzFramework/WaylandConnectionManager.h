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
#include <AzCore/RTTI/RTTI.h>

#include <wayland-client.h>

struct xkb_context;
namespace AzFramework
{
    //We should only have one display connection,
    //either Qt or AzFramework::WaylandConnectionManagerImpl will try to be the provider for this.
    class WaylandDisplayProvider
    {
    public:
        AZ_RTTI(WaylandDisplayProvider, "{5C0FB229-0D37-42A2-B6DE-513BB421D6DA}");
        virtual ~WaylandDisplayProvider() = default;

        virtual int GetDisplayFD() const = 0;
        virtual wl_display* GetWaylandDisplay() const = 0;
    };

    using WaylandDisplayProviderInterface = AZ::Interface<WaylandDisplayProvider>;

    class WaylandConnectionManager
    {
    public:
        AZ_RTTI(WaylandConnectionManager, "{120B08F8-C381-436C-806D-5439CE816223}");
        virtual ~WaylandConnectionManager() = default;

        virtual void DoRoundtrip() const = 0;
        virtual void CheckErrors() const = 0;

        //Returns null when it's unnecessary; we use event queues when someone else is providing the
        // display connection for us (Qt) so we don't mess with their default event queue.
        virtual wl_event_queue* GetWaylandEventQueue() const = 0;
        virtual wl_registry* GetWaylandRegistry() const = 0;
        virtual wl_compositor* GetWaylandCompositor() const = 0;

        virtual xkb_context* GetXkbContext() const = 0;
    };

    class WaylandConnectionManagerBusTraits : public AZ::EBusTraits
    {
    public:
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    };

    using WaylandConnectionManagerBus = AZ::EBus<WaylandConnectionManager, WaylandConnectionManagerBusTraits>;
    using WaylandConnectionManagerInterface = AZ::Interface<WaylandConnectionManager>;
} // namespace AzFramework
