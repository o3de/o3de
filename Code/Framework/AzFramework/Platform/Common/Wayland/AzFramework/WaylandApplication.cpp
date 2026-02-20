/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Console/IConsole.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/tokenize.h>

#include <AzFramework/WaylandApplication.h>
#include <AzFramework/WaylandConnectionManager.h>
#include <AzFramework/WaylandInterface.h>
#include <AzFramework/Protocols/ProtocolManager.h>
#include <AzFramework/Protocols/XdgManager.h>
#include <AzFramework/Protocols/SeatManager.h>

#include <sys/poll.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

#include "WaylandProtocolNames.h"

AZStd::vector<AZStd::string> g_blockedProtocols;

static void wl_blocklist_updated(const AZStd::string& unseparatedList)
{
    const AZStd::string deliminator = ",";
    AZStd::vector<AZStd::string> list;

    AZStd::tokenize(unseparatedList, deliminator, list);

    g_blockedProtocols.clear();
    for (const auto& proto : list)
    {
        g_blockedProtocols.push_back(proto);
    }
}

AZ_CVAR(
    AZStd::string,
    wl_blocklist,
    "",
    wl_blocklist_updated,
    AZ::ConsoleFunctorFlags::DontReplicate,
    "WAYLAND ONLY: comma seperated list of protocols to block, highly recommend to only update this on startup.");

namespace AzFramework
{
    class WaylandConnectionManagerImpl
        : public WaylandConnectionManagerBus::Handler
    {
    public:
        WaylandConnectionManagerImpl()
            : m_waylandDisplay(wl_display_connect(nullptr))
        {
            AZ_Error("Application", m_waylandDisplay != nullptr, "Failed to connect to Wayland Display.");
            m_fd = wl_display_get_fd(m_waylandDisplay.get());

            m_registry = wl_display_get_registry(m_waylandDisplay.get());
            AZ_Error("Application", m_registry != nullptr, "Failed to get Wayland Registry.");

            m_xkbContext = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
            AZ_Error("Application", m_xkbContext != nullptr, "Failed to get XKB context.");

            wl_registry_add_listener(m_registry, &s_registryListener, this);

            WaylandConnectionManagerBus::Handler::BusConnect();
        }

        ~WaylandConnectionManagerImpl() override
        {
            WaylandConnectionManagerBus::Handler::BusDisconnect();
            wl_registry_destroy(m_registry);
            wl_compositor_destroy(m_compositor);
        }

        void DoRoundtrip() const override
        {
            wl_display_roundtrip(m_waylandDisplay.get());
        }

        void CheckErrors() const override
        {
            int errorCode = wl_display_get_error(m_waylandDisplay.get());
            if (errorCode == EPROTO)
            {
                const wl_interface* anInterface;
                uint32_t interfaceId;
                auto code = wl_display_get_protocol_error(m_waylandDisplay.get(), &anInterface, &interfaceId);
                if (anInterface != nullptr)
                {
                    WaylandInterfaceNotificationsBus::Event(
                        interfaceId, &WaylandInterfaceNotificationsBus::Events::OnProtocolError, interfaceId, code);
                }

                // Man page says, "Note: Errors are fatal. If this function returns non-zero, the display can no longer be used."
                // let's crash
                AZ_Fatal("Wayland", "Protocol error occurred %d, please check above for more info.", code);
                AZ_Crash();
            }
            else if (errorCode != 0)
            {
                AZ_Fatal("Wayland", "Wayland error occurred %d", errorCode);
                AZ_Crash();
            }
        }

        int GetDisplayFD() const override
        {
            return m_fd;
        }

        wl_display* GetWaylandDisplay() const override
        {
            return m_waylandDisplay.get();
        }

        wl_registry* GetWaylandRegistry() const override
        {
            return m_registry;
        }

        wl_compositor* GetWaylandCompositor() const override
        {
            return m_compositor;
        }

        xkb_context* GetXkbContext() const override
        {
            return m_xkbContext;
        }

        static void GlobalRegistryHandler(void* data, wl_registry* registry, uint32_t id, const char* interface,
                                          uint32_t version)
        {
            auto self = static_cast<WaylandConnectionManagerImpl*>(data);

            if (AZStd::find(g_blockedProtocols.begin(), g_blockedProtocols.end(), interface) != g_blockedProtocols.
                end())
            {
                AZ_Info("Wayland", "Blocked protocol %s", interface);
                return;
            }

            if (strcmp(interface, wl_compositor_interface.name) == 0)
            {
                self->m_compositor = static_cast<wl_compositor*>(wl_registry_bind(
                    registry, id, &wl_compositor_interface, version));
            }
            else
            {
                WaylandRegistryEventsBus::Broadcast(
                    &WaylandRegistryEventsBus::Events::OnRegister,
                    registry,
                    id,
                    AZ::Crc32(interface),
                    version);
            }
        }

        static void GlobalRegistryRemove(void*, wl_registry* registry, uint32_t id)
        {
            WaylandRegistryEventsBus::Broadcast(&WaylandRegistryEventsBus::Events::OnUnregister, registry, id);
        }

    private:
        int m_fd = -1;
        WaylandUniquePtr<wl_display, wl_display_disconnect> m_waylandDisplay = nullptr;
        wl_registry* m_registry = nullptr;
        wl_compositor* m_compositor = nullptr;
        xkb_context* m_xkbContext = nullptr;

        const wl_registry_listener s_registryListener = {
            .global = GlobalRegistryHandler, .global_remove = GlobalRegistryRemove
        };
    };

    WaylandApplication::WaylandApplication()
    {
        LinuxLifecycleEvents::Bus::Handler::BusConnect();

        m_waylandConnectionManager = AZStd::make_unique<WaylandConnectionManagerImpl>();
        if (WaylandConnectionManagerInterface::Get() == nullptr)
        {
            WaylandConnectionManagerInterface::Register(m_waylandConnectionManager.get());
        }

        // Add needed protocols
        m_seatManager = AZStd::make_unique<SeatManager>();
        m_outputManager = AZStd::make_unique<OutputManager>();
        m_protocolManger = AZStd::make_unique<ProtocolManager>();
        m_cursorShapeManger = AZStd::make_unique<CursorShapeManager>();
        m_xdgManager = AZStd::make_unique<XdgManagerImpl>();

        WaylandConnectionManagerBus::Broadcast(&WaylandConnectionManagerBus::Events::DoRoundtrip);
        PumpSystemEventLoopOnce();

        //Check requirements
#define WL_RESTART_MSG ", restart with the WAYLAND_DISPLAY environment variable unset to fall back to X11/XCB"
        if (m_xdgManager->GetProxy(XdgWmBaseName) == nullptr)
        {
            AZ_Fatal("Wayland", "Compositor missing XDG shell v%i" WL_RESTART_MSG, XdgWmBaseVersion);
            AZ_Crash();
        }
        if (m_protocolManger->GetProxy(PointerConstraintsName) == nullptr)
        {
            AZ_Fatal("Wayland", "Compositor missing pointer constraints v%i" WL_RESTART_MSG, PointerConstraintsVersion);
            AZ_Crash();
        }
        if (m_protocolManger->GetProxy(RelativePointerManagerName) == nullptr)
        {
            AZ_Fatal("Wayland", "Compositor missing relative pointer v%i" WL_RESTART_MSG, RelativePointerManagerVersion);
            AZ_Crash();
        }

        WaylandRegistryEventsBus::Broadcast(&WaylandRegistryEventsBus::Events::OnRegistryFinished);
    }

    WaylandApplication::~WaylandApplication()
    {
        m_seatManager.reset();
        m_outputManager.reset();
        m_protocolManger.reset();
        m_xdgManager.reset();
        m_cursorShapeManger.reset();

        if (WaylandConnectionManagerInterface::Get() == m_waylandConnectionManager.get())
        {
            WaylandConnectionManagerInterface::Unregister(m_waylandConnectionManager.get());
        }
        m_waylandConnectionManager.reset();
        LinuxLifecycleEvents::Bus::Handler::BusDisconnect();
    }

    bool WaylandApplication::HasEventsWaiting() const
    {
        int fd = m_waylandConnectionManager->GetDisplayFD();
        struct pollfd pfd = {fd, POLLIN};

        return poll(&pfd, 1, 0) > 0;
    }

    void WaylandApplication::PumpSystemEventLoopOnce()
    {
        if (wl_display* display = m_waylandConnectionManager->GetWaylandDisplay())
        {
            if (wl_display_dispatch_pending(display) == 0)
            {
                // no pending events, read new events
                wl_display_flush(display);
                wl_display_prepare_read(display);

                if (HasEventsWaiting())
                {
                    wl_display_read_events(display);
                    wl_display_dispatch_pending(display);
                }
                else
                {
                    wl_display_cancel_read(display);
                }
            }
        }
        m_waylandConnectionManager->CheckErrors();
    }

    void WaylandApplication::PumpSystemEventLoopUntilEmpty()
    {
        if (wl_display* display = m_waylandConnectionManager->GetWaylandDisplay())
        {
            while (true)
            {
                int num_dispatched_events = wl_display_dispatch_pending(display);
                if (num_dispatched_events == 0)
                {
                    // no pending events, read new events
                    wl_display_flush(display);
                    wl_display_prepare_read(display);

                    if (HasEventsWaiting())
                    {
                        wl_display_read_events(display);
                        wl_display_dispatch_pending(display);
                    }
                    else
                    {
                        wl_display_cancel_read(display);
                        break; // no events are pending
                    }
                }
                else if (num_dispatched_events == -1)
                {
                    // error
                    m_waylandConnectionManager->CheckErrors();
                    return;
                }
                else
                {
                    break;
                }
            }
        }
        m_waylandConnectionManager->CheckErrors();
    }
} // namespace AzFramework
