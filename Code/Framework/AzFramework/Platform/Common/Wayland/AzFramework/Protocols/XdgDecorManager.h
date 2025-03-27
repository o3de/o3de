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

#include <xdg-decoration-unstable-v1-client-protocol.h>

namespace AzFramework
{
    class XdgDecorConnectionManager
    {
    public:
        AZ_RTTI(XdgDecorConnectionManager, "{6F24AD68-0927-440F-8DFE-B7BE9DCAABDF}");

        virtual ~XdgDecorConnectionManager() = default;

        virtual zxdg_decoration_manager_v1* GetXdgDecor() const = 0;
    };

    using XdgDecorConnectionManagerInterface = AZ::Interface<XdgDecorConnectionManager>;
} // namespace AzFramework