/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
    /**
    * Script Asset - contains the source code for a script
    */
    class ScriptAsset
        : public Data::AssetData
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptAsset, AZ::SystemAllocator, 0);
        AZ_RTTI(ScriptAsset, "{82557326-4AE3-416C-95D6-C70635AB7588}", Data::AssetData);

        enum : u32
        {
            // Asset sub-ids
            CompiledAssetSubId = 1,
            CopiedAssetSubId = 2,
        };

        enum LuaScriptInfo : u8 // u8 to avoid endian issues
        {
            // Current latest-asset version
            AssetVersion = 3,

            // Asset type field for Lua bytecode files
            AssetTypeCompiled = 0,
            // Asset type field for Lua text files
            AssetTypeText = 1,

            // Used to initialize fields that will be read in
            Invalid = UCHAR_MAX,
        };

        ScriptAsset(const Data::AssetId& assetId = Data::AssetId());
        ~ScriptAsset() override = default;

        ScriptAsset(const ScriptAsset& rhs) = delete;
        ScriptAsset& operator=(const ScriptAsset& rhs) = delete;

        //////////////////////////////////////////////////////////////////////////
        // Script data
        const AZStd::vector<char>& GetScriptBuffer() { return m_scriptBuffer; }
        IO::MemoryStream CreateMemoryStream() { return IO::MemoryStream(m_scriptBuffer.data(), m_scriptBuffer.size()); }
        IO::ByteContainerStream<AZStd::vector<char>> CreateWriteStream() { return IO::ByteContainerStream<AZStd::vector<char>>(&m_scriptBuffer, m_scriptBuffer.size()); }

        const char* GetDebugName() { return m_debugName.empty() ? nullptr : m_debugName.c_str(); }

    private:
        // This will be the buffer of either the precompiled script or the source (depending on asset selected)
        AZStd::vector<char> m_scriptBuffer;
        // Debug name of the script. Will be empty if script is precompiled
        AZStd::string m_debugName;

        friend class ScriptSystemComponent;
        friend UnitTest::ScriptComponentTest;
    };
}
