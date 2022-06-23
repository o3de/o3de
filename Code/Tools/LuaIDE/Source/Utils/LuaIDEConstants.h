/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Name/Name.h>

namespace LUADebugger
{
    static const AZ::Name LuaToolsName = AZ::Name("LuaRemoteTools");
    static constexpr AZ::Crc32 LuaToolsKey("LuaRemoteTools");
    static const uint16_t LuaToolsPort = 6777;
}
