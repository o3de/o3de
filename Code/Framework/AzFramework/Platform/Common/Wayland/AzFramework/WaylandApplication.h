/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/API/ApplicationAPI_Platform.h>
#include <AzFramework/Application/Application.h>
#include <AzFramework/Protocols/OutputManager.h>
#include <AzFramework/Protocols/ProtocolManager.h>
#include <AzFramework/Protocols/CursorShapeManager.h>
#include <AzFramework/WaylandConnectionManager.h>

#include "Protocols/SeatManager.h"

namespace AzFramework
{
    class XdgManagerImpl;

    class WaylandApplication final
        : public Application::Implementation
        , public LinuxLifecycleEvents::Bus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(WaylandApplication, AZ::SystemAllocator);
        WaylandApplication();
        ~WaylandApplication() override;

        bool HasEventsWaiting() const;

        void PumpSystemEventLoopOnce() override;
        void PumpSystemEventLoopUntilEmpty() override;

    private:
        AZStd::unique_ptr<WaylandConnectionManager> m_waylandConnectionManager;

        // These are protocol managers we will keep alive
        AZStd::unique_ptr<ProtocolManager> m_protocolManger;
        AZStd::unique_ptr<CursorShapeManager> m_cursorShapeManger;
        AZStd::unique_ptr<SeatManager> m_seatManager;
        AZStd::unique_ptr<OutputManager> m_outputManager;
        AZStd::unique_ptr<XdgManagerImpl> m_xdgManager;
    };
} // namespace AzFramework
