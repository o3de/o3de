/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if !defined(AZCORE_EXCLUDE_LUA)

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzCore/Script/ScriptAsset.h>

namespace AZ
{
    void LuaScriptData::Reflect(AZ::ReflectContext* reflectContext)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
        {
            const int NeverVersionAssets = 0;
            serializeContext->Class<LuaScriptData>()
                ->Version(NeverVersionAssets) // update the builder number if needed
                ->Field("debugName", &LuaScriptData::m_debugName)
                ->Field("dependencies", &LuaScriptData::m_dependencies)
                ->Field("luaScript", &LuaScriptData::m_luaScript)
                ;
        }
    }

    ScriptAsset::ScriptAsset(const Data::AssetId& assetId)
        : Data::AssetData(assetId)
    {
    }

    IO::MemoryStream LuaScriptData::CreateScriptReadStream()
    {
        return IO::MemoryStream(m_luaScript.data(), m_luaScript.size());
    }

    IO::ByteContainerStream<AZStd::vector<char>> LuaScriptData::CreateScriptWriteStream()
    {
        return IO::ByteContainerStream<AZStd::vector<char>>(&m_luaScript, m_luaScript.size());
    }

    const char* LuaScriptData::GetDebugName()
    {
        return m_debugName.empty() ? nullptr : m_debugName.c_str();
    }

    const AZStd::vector<char>& LuaScriptData::GetScriptBuffer()
    {
        return m_luaScript;
    }

}   // namespace AZ

#endif // #if !defined(AZCORE_EXCLUDE_LUA)
