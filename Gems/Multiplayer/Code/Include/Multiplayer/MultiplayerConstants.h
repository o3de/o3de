/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
    static constexpr AZStd::string_view MPNetworkInterfaceName("MultiplayerNetworkInterface");
    static constexpr AZStd::string_view MPEditorInterfaceName("MultiplayerEditorNetworkInterface");

    static constexpr AZStd::string_view LocalHost("127.0.0.1");
    static constexpr uint16_t DefaultServerPort = 30090;
    static constexpr uint16_t DefaultServerEditorPort = 30091;

}

