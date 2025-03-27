/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzCore/RTTI/RTTI.h>

#include <xdg-shell-client-protocol.h>

namespace AzFramework
{
    class XdgShellConnectionManager
    {
    public:
        AZ_RTTI(XdgShellConnectionManager, "{F1176D31-02EB-4154-BE66-A58F40DA3027}");

        virtual ~XdgShellConnectionManager() = default;

        virtual uint32_t GetXdgWmBaseRegistryId() const = 0;
        virtual xdg_wm_base* GetXdgWmBase() const = 0;
    };

    using XdgShellConnectionManagerInterface = AZ::Interface<XdgShellConnectionManager>;
} // namespace AzFramework