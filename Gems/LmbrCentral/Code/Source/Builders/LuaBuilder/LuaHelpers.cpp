/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <LmbrCentral_precompiled.h>
#include "LuaHelpers.h"
#include "LuaBuilderWorker.h"

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Script/ScriptAsset.h>
#include <AzCore/Script/lua/lua.h>

#include <AssetBuilderSDK/AssetBuilderSDK.h>

namespace LuaBuilder
{
    //////////////////////////////////////////////////////////////////////////
    namespace
    {
        static int LuaStreamWriter(lua_State*, const void* data, size_t size, void* userData)
        {
            using namespace AZ::IO;
            GenericStream* stream = static_cast<GenericStream*>(userData);

            SizeType bytesToWrite = aznumeric_caster(size);
            SizeType bytesWritten = stream->Write(bytesToWrite, data);

            // Return whether or not there was an error
            return bytesWritten == bytesToWrite ? 0 : 1;
        }
    }

    bool LuaDumpToStream(AZ::IO::GenericStream& stream, lua_State* l)
    {
        if (!lua_isfunction(l, -1))
        {
            AZ_TracePrintf(AssetBuilderSDK::ErrorWindow, "Top of stack is not function!");
        }
        const int stripBinaryInformation = 0;
        return lua_dump(l, LuaStreamWriter, &stream, stripBinaryInformation) == 0;
    }
}
