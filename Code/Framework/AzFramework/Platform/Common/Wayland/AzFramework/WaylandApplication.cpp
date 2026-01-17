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

#include <AzFramework/Protocols/CursorShapeManager.h>
#include <AzFramework/Protocols/PointerConstraintsManager.h>
#include <AzFramework/Protocols/RelativePointerManager.h>
#include <AzFramework/Protocols/XdgManager.h>
#include <AzFramework/Protocols/SeatManager.h>
#include <AzFramework/WaylandApplication.h>
#include <AzFramework/WaylandConnectionManager.h>
#include <AzFramework/WaylandInterface.h>

#include <sys/poll.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

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
    struct WaylandSeat
    {
        explicit WaylandSeat(wl_seat* inseat)
            : m_seat(inseat)
        {
        }

        wl_seat* m_seat = nullptr;
        uint32_t m_registryId = 0;
        uint32_t m_playerIdx = 0;
        bool m_supportsPointer = false;
        bool m_supportsKeyboard = false;
        bool m_supportsTouch = false;

        AZStd::string m_name = "";
    };

    class WaylandConnectionManagerImpl
        : public WaylandConnectionManagerBus::Handler
        , public SeatManager
    {
    public:
        WaylandConnectionManagerImpl()
            : m_waylandDisplay(wl_display_connect(nullptr))
        {
            AZ_Error("Application", m_waylandDisplay != nullptr, "Unable to connect to Wayland Display.");
            m_fd = wl_display_get_fd(m_waylandDisplay.get());

            m_registry = wl_display_get_registry(m_waylandDisplay.get());
            AZ_Error("Application", m_registry != nullptr, "Unable to get Wayland Registry.");

            m_xkbContext = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
            AZ_Error("Application", m_xkbContext != nullptr, "Unable to get XKB context.");

            wl_registry_add_listener(m_registry, &s_registry_listener, this);

            WaylandConnectionManagerBus::Handler::BusConnect();

            if (SeatManagerInterface::Get() == nullptr)
            {
                SeatManagerInterface::Register(this);
            }
        }

        ~WaylandConnectionManagerImpl() override
        {
            {
                // GlobalRegistryRemove will modify the vector
                // copy it for local
                auto seats = m_seats;
                for (auto& seat : seats)
                {
                    GlobalRegistryRemove(this, m_registry, seat.first);
                }
            }
            m_seats.clear();

            if (SeatManagerInterface::Get() == this)
            {
                SeatManagerInterface::Unregister(this);
            }

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
            if (errorCode != 0)
            {
                const wl_interface* anInterface;
                uint32_t interfaceId;
                auto code = wl_display_get_protocol_error(m_waylandDisplay.get(), &anInterface, &interfaceId);
                if (anInterface != nullptr)
                {
                    WaylandInterfaceNotificationsBus::Event(
                        interfaceId, &WaylandInterfaceNotificationsBus::Events::OnProtocolError, interfaceId, code);
                }

                // Man page says: Note: Errors are fatal. If this function returns non-zero, the display can no longer be used.
                // let's crash
                AZ_Fatal("Wayland", "Protocol error occurred %d, please check above for more info.", errorCode);
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

        uint32_t GetSeatCount() const override
        {
            return m_seats.size();
        }

        wl_pointer* GetSeatPointer(uint32_t playerIdx) const override
        {
            if (auto seat = GetSeatFromPlayerIdx(playerIdx))
            {
                if (!seat->m_supportsPointer)
                    return nullptr;
                return wl_seat_get_pointer(seat->m_seat);
            }
            return nullptr;
        }

        wl_keyboard* GetSeatKeyboard(uint32_t playerIdx) const override
        {
            if (auto seat = GetSeatFromPlayerIdx(playerIdx))
            {
                if (!seat->m_supportsKeyboard)
                    return nullptr;
                return wl_seat_get_keyboard(seat->m_seat);
            }
            return nullptr;
        }

        wl_touch* GetSeatTouch(uint32_t playerIdx) const override
        {
            if (auto seat = GetSeatFromPlayerIdx(playerIdx))
            {
                if (!seat->m_supportsTouch)
                    return nullptr;
                return wl_seat_get_touch(seat->m_seat);
            }
            return nullptr;
        }

        static void GlobalRegistryHandler(void* data, wl_registry* registry, uint32_t id, const char* interface, uint32_t version)
        {
            auto self = static_cast<WaylandConnectionManagerImpl*>(data);

            if (AZStd::find(g_blockedProtocols.begin(), g_blockedProtocols.end(), interface) != g_blockedProtocols.end())
            {
                AZ_Info("Wayland", "Blocked protocol %s", interface);
                return;
            }

            if (WL_IS_INTERFACE(wl_compositor_interface))
            {
                self->m_compositor = static_cast<wl_compositor*>(wl_registry_bind(registry, id, &wl_compositor_interface, version));
                self->m_compositorId = id;
            }
            else if (WL_IS_INTERFACE(wl_seat_interface))
            {
                auto seat = static_cast<wl_seat*>(wl_registry_bind(registry, id, &wl_seat_interface, version));
                auto info = new WaylandSeat(seat);
                info->m_registryId = id;
                info->m_playerIdx = self->GetAvailablePlayerIdx();
                wl_seat_add_listener(seat, &self->s_seat_listener, info);

                self->m_seats.emplace(id, info);
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

        static void GlobalRegistryRemove(void* data, wl_registry* registry, uint32_t id)
        {
            auto self = static_cast<WaylandConnectionManagerImpl*>(data);
            if (self->m_seats.find(id) != self->m_seats.end())
            {
                auto seat = self->m_seats[id];

                AzFramework::SeatNotificationsBus::Event(seat->m_playerIdx, &AzFramework::SeatNotificationsBus::Events::ReleaseSeat);

                self->m_seats.erase(id);
                wl_seat_destroy(seat->m_seat);
                delete seat;
            }
            else
            {
                WaylandRegistryEventsBus::Broadcast(&WaylandRegistryEventsBus::Events::OnUnregister, registry, id);
            }
        }

        static void SeatCaps(void* data, struct wl_seat* wl_seat, uint32_t capabilities)
        {
            auto self = static_cast<WaylandSeat*>(data);

            self->m_supportsPointer = capabilities & WL_SEAT_CAPABILITY_POINTER;
            self->m_supportsKeyboard = capabilities & WL_SEAT_CAPABILITY_KEYBOARD;
            self->m_supportsTouch = capabilities & WL_SEAT_CAPABILITY_TOUCH;

            AZ_Info("Wayland", "Seat capabilities updated for player idx: %i name: \"%s\"", self->m_playerIdx, self->m_name.c_str());
            AZ_Info("Wayland", "Supports Pointer? %s", self->m_supportsPointer ? "yes" : "no");
            AZ_Info("Wayland", "Supports Keyboard? %s", self->m_supportsKeyboard ? "yes" : "no");
            AZ_Info("Wayland", "Supports Touch? %s", self->m_supportsTouch ? "yes" : "no");

            AzFramework::SeatNotificationsBus::Event(self->m_playerIdx, &AzFramework::SeatNotificationsBus::Events::SeatCapsChanged);
        }

        static void SeatName(void* data, struct wl_seat* wl_seat, const char* name)
        {
            auto self = static_cast<WaylandSeat*>(data);
            self->m_name = AZStd::string(name);
        }

    private:
        WaylandSeat* GetSeatFromPlayerIdx(uint32_t playerIdx) const
        {
            for (auto& seat : m_seats)
            {
                if (seat.second->m_playerIdx == playerIdx)
                {
                    return seat.second;
                }
            }

            return nullptr;
        }

        uint32_t GetAvailablePlayerIdx() const
        {
            for (uint32_t i = 0; i < UINT32_MAX; ++i)
            {
                if (GetSeatFromPlayerIdx(i) == nullptr)
                {
                    return i;
                }
            }

            return UINT32_MAX;
        }

        int m_fd = -1;
        WaylandUniquePtr<wl_display, wl_display_disconnect> m_waylandDisplay = nullptr;
        wl_registry* m_registry = nullptr;
        wl_compositor* m_compositor = nullptr;
        uint32_t m_compositorId = 0;
        xkb_context* m_xkbContext = nullptr;

        // Registry id -> WaylandSeat Ptr
        AZStd::unordered_map<uint32_t, WaylandSeat*> m_seats = {};

        const wl_registry_listener s_registry_listener = { .global = GlobalRegistryHandler, .global_remove = GlobalRegistryRemove };

        const wl_seat_listener s_seat_listener = { .capabilities = SeatCaps, .name = SeatName };
    };

    struct OutputInfo
    {
        wl_output* m_output = nullptr;
        uint32_t m_id = 0;
        bool m_isDone = false;
        int32_t m_x = 0;
        int32_t m_y = 0;
        int32_t m_width = 0;
        int32_t m_height = 0;
        int32_t m_refreshRateMhz = 0;
        int32_t m_physicalWidth = 0;
        int32_t m_physicalHeight = 0;
        wl_output_subpixel m_subpixel = {};
        AZStd::string m_make = "";
        AZStd::string m_model = "";
        wl_output_transform m_transform = {};

        AZStd::string m_name = "";
        AZStd::string m_desc = "";

        int32_t m_scaleFactor = 0;
    };

    class OutputManagerImpl
        : public OutputManager
        , public WaylandRegistryEventsBus::Handler
    {
    public:
        OutputManagerImpl()
        {
            WaylandRegistryEventsBus::Handler::BusConnect();

            if (OutputManagerInterface::Get() == nullptr)
            {
                OutputManagerInterface::Register(this);
            }
        }

        ~OutputManagerImpl() override
        {
            WaylandRegistryEventsBus::Handler::BusDisconnect();

            if (OutputManagerInterface::Get() == this)
            {
                OutputManagerInterface::Unregister(this);
            }
        }

        uint32_t GetRefreshRateMhz(wl_output* output) override
        {
            OutputInfo* info = static_cast<OutputInfo*>(wl_output_get_user_data(output));
            if (info == nullptr || !info->m_isDone)
                return 0;

            return info->m_refreshRateMhz;
        }

        AZStd::string GetOutputName(wl_output* output) override
        {
            OutputInfo* info = static_cast<OutputInfo*>(wl_output_get_user_data(output));
            if (info == nullptr || !info->m_isDone)
                return {};

            return info->m_name;
        }

        AZStd::string GetOutputDesc(wl_output* output) override
        {
            OutputInfo* info = static_cast<OutputInfo*>(wl_output_get_user_data(output));
            if (info == nullptr || !info->m_isDone)
                return {};

            return info->m_desc;
        }

        void OnRegister(wl_registry* registry, uint32_t id, const AZ::Crc32 interface, uint32_t version) override
        {
            if (interface != AZ_CRC("wl_output"))
            {
                return;
            }

            wl_output* output = static_cast<wl_output*>(wl_registry_bind(registry, id, &wl_output_interface, version));

            auto info = azcreate(OutputInfo);
            info->m_output = output;
            info->m_id = id;

            wl_output_set_user_data(output, info);

            wl_output_add_listener(output, &s_output_listener, info);

            m_outputs.emplace(id, info);
        }

        void OnUnregister(wl_registry* registry, uint32_t id) override
        {
            if (m_outputs.find(id) == m_outputs.end())
            {
                return;
            }

            auto info = m_outputs[id];
            m_outputs.erase(id);

            wl_output_destroy(info->m_output);

            azdestroy(info);
        }

        static void OutputGeometry(
            void* data,
            struct wl_output* wl_output,
            int32_t x,
            int32_t y,
            int32_t physical_width,
            int32_t physical_height,
            int32_t subpixel,
            const char* make,
            const char* model,
            int32_t transform)
        {
            auto self = static_cast<OutputInfo*>(data);
            self->m_x = x;
            self->m_y = y;
            self->m_physicalWidth = physical_width;
            self->m_physicalHeight = physical_height;
            self->m_subpixel = static_cast<wl_output_subpixel>(subpixel);
            self->m_make = AZStd::string(make);
            self->m_model = AZStd::string(model);
            self->m_transform = static_cast<wl_output_transform>(transform);
        }

        static void OutputMode(void* data, struct wl_output* wl_output, uint32_t flags, int32_t width, int32_t height, int32_t refresh)
        {
            auto self = static_cast<OutputInfo*>(data);
            if (flags & wl_output_mode::WL_OUTPUT_MODE_CURRENT)
            {
                // We only really care for the current mode.
                self->m_width = width;
                self->m_height = height;
                self->m_refreshRateMhz = refresh;
            }
        }

        static void OutputDone(void* data, struct wl_output* wl_output)
        {
            auto self = static_cast<OutputInfo*>(data);
            self->m_isDone = true;
        }

        static void OutputScale(void* data, struct wl_output* wl_output, int32_t factor)
        {
            auto self = static_cast<OutputInfo*>(data);
            self->m_scaleFactor = factor;
        }

        static void OutputName(void* data, struct wl_output* wl_output, const char* name)
        {
            auto self = static_cast<OutputInfo*>(data);
            self->m_name = AZStd::string(name);
        }

        static void OutputDesc(void* data, struct wl_output* wl_output, const char* description)
        {
            auto self = static_cast<OutputInfo*>(data);
            self->m_desc = AZStd::string(description);
        }

        const wl_output_listener s_output_listener = { .geometry = OutputGeometry,
                                                       .mode = OutputMode,
                                                       .done = OutputDone,
                                                       .scale = OutputScale,
                                                       .name = OutputName,
                                                       .description = OutputDesc };

    private:
        AZStd::unordered_map<uint32_t, OutputInfo*> m_outputs = {};
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
        m_outputManager = AZStd::make_unique<OutputManagerImpl>();
        m_xdgManager = AZStd::make_unique<XdgManagerImpl>();
        m_relativePointerManager = AZStd::make_unique<RelativePointerManagerImpl>();
        m_pointerConstraintsManager = AZStd::make_unique<PointerConstraintsManagerImpl>();
        m_cursorShapeManager = AZStd::make_unique<CursorShapeManagerImpl>();

        WaylandConnectionManagerBus::Broadcast(&WaylandConnectionManagerBus::Events::DoRoundtrip);
        PumpSystemEventLoopOnce();
    }

    WaylandApplication::~WaylandApplication()
    {
        m_outputManager.reset();
        m_relativePointerManager.reset();
        m_pointerConstraintsManager.reset();
        m_cursorShapeManager.reset();
        m_xdgManager.reset();

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
        struct pollfd pfd = { fd, POLLIN };

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