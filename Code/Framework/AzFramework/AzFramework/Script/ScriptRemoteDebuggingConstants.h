/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Name/Name.h>
#include <AzCore/Name/NameDictionary.h>

namespace AzFramework
{
    static const AZ::Name LuaToolsName = AZ::Name::FromStringLiteral("LuaRemoteTools", nullptr);
    static constexpr AZ::Crc32 LuaToolsKey("LuaRemoteTools");
    static constexpr uint16_t LuaToolsPort = 6777;
}
