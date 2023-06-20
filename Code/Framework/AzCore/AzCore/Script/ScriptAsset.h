/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/std/containers/vector.h>

namespace UnitTest
{
    class ScriptComponentTest;
}

namespace AZ
{
    class ReflectContext;
    class ScriptAsset;

    class LuaScriptData
    {
    public:
        AZ_CLASS_ALLOCATOR(LuaScriptData, AZ::SystemAllocator);
        AZ_TYPE_INFO(LuaScriptData, "{C62098FE-8E3F-4DD3-88F8-B11FD2609A43}");

        static void Reflect(AZ::ReflectContext* context);

        AZStd::string m_debugName;
        AZStd::vector<AZ::Data::Asset<ScriptAsset>> m_dependencies;
        AZStd::vector<char> m_luaScript;

        IO::MemoryStream CreateScriptReadStream();
        IO::ByteContainerStream<AZStd::vector<char>> CreateScriptWriteStream();
        const char* GetDebugName();
        const AZStd::vector<char>& GetScriptBuffer();
    };

    /**
    * Script Asset - contains the source code for a script
    */
    class ScriptAsset
        : public Data::AssetData
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptAsset, AZ::SystemAllocator);
        AZ_RTTI(ScriptAsset, "{82557326-4AE3-416C-95D6-C70635AB7588}", Data::AssetData);

        static const u32 CompiledAssetSubId = 1;

        ScriptAsset(const Data::AssetId& assetId = Data::AssetId());
        ~ScriptAsset() override = default;

        LuaScriptData m_data;

    private:
        friend class ScriptSystemComponent;
        friend UnitTest::ScriptComponentTest;
    };
}
