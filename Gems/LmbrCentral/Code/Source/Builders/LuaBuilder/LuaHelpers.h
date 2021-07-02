/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/IO/GenericStreams.h>
#include <AzCore/Script/ScriptContext.h>

namespace LuaBuilder
{
    // Write a lua function to a stream
    bool LuaDumpToStream(AZ::IO::GenericStream& stream, lua_State* lua);
}
