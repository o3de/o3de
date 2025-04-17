/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Name/Name.h>
#include <AzCore/std/string/fixed_string.h>

namespace AZ::Metrics
{
    enum class EventLoggerId : AZ::u32;
}

namespace Multiplayer
{
    constexpr AZStd::string_view MpNetworkInterfaceName("MultiplayerNetworkInterface");
    constexpr AZStd::string_view MpEditorInterfaceName("MultiplayerEditorNetworkInterface");
    constexpr AZStd::string_view LocalHost("127.0.0.1");
    constexpr AZStd::string_view NetworkFileExtension(".network");
    constexpr AZStd::string_view NetworkSpawnableFileExtension(".network.spawnable");

    constexpr uint16_t DefaultServerPort = 33450;
    constexpr uint16_t DefaultServerEditorPort = 33451;

    constexpr AZ::Metrics::EventLoggerId NetworkingMetricsId{ static_cast<AZ::u32>(AZStd::hash<AZStd::string_view>{}("Networking")) };
}
