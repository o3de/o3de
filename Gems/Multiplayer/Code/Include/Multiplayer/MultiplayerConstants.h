/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/Event.h>
#include <AzCore/Name/Name.h>
#include <AzCore/RTTI/TypeSafeIntegral.h>
#include <AzCore/std/string/fixed_string.h>
#include <AzNetworking/Serialization/ISerializer.h>
#include <AzNetworking/ConnectionLayer/ConnectionEnums.h>

namespace Multiplayer
{
    constexpr AZStd::string_view MpNetworkInterfaceName("MultiplayerNetworkInterface");
    constexpr AZStd::string_view MpEditorInterfaceName("MultiplayerEditorNetworkInterface");
    constexpr AZStd::string_view LocalHost("127.0.0.1");

    constexpr uint16_t DefaultServerPort = 33450;
    constexpr uint16_t DefaultServerEditorPort = 33451;
}
