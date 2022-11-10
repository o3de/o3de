/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <ScriptCanvas/Asset/AssetDescription.h>
#include <ScriptCanvas/Core/SubgraphInterface.h>

namespace ScriptCanvas
{
    constexpr const AZ::u32 SubgraphInterfaceSubId = AZ_CRC_CE("SubgraphInterface"); // 0xdfe6dc72

    class SubgraphInterfaceAsset;

    class SubgraphInterfaceAssetDescription
        : public AssetDescription
    {
    public:
        AZ_TYPE_INFO(SubgraphInterfaceAssetDescription, "{7F7BE1A5-9447-41C2-9190-18580075094C}");

        SubgraphInterfaceAssetDescription();
    };

    struct SubgraphInterfaceData
    {
        AZ_TYPE_INFO(SubgraphInterfaceData, "{1734C569-7D40-4491-9EEE-A225E333C9BA}");
        AZ_CLASS_ALLOCATOR(SubgraphInterfaceData, AZ::SystemAllocator, 0);
        SubgraphInterfaceData() = default;
        ~SubgraphInterfaceData() = default;
        SubgraphInterfaceData(const SubgraphInterfaceData&) = default;
        SubgraphInterfaceData& operator=(const SubgraphInterfaceData&) = default;
        SubgraphInterfaceData(SubgraphInterfaceData&&);
        SubgraphInterfaceData& operator=(SubgraphInterfaceData&&);

        static void Reflect(AZ::ReflectContext* reflectContext);

        AZStd::string m_name;
        Grammar::SubgraphInterface m_interface;
    };

    class SubgraphInterfaceAsset
        : public AZ::Data::AssetData
    {
    public:
        AZ_RTTI(SubgraphInterfaceAsset, "{E22967AC-7673-4778-9125-AF49D82CAF9F}", AZ::Data::AssetData);
        AZ_CLASS_ALLOCATOR(SubgraphInterfaceAsset, AZ::SystemAllocator, 0);

        static const char* GetFileExtension()
        {
            return "scriptcanvas_fn_compiled";
        }
        static const char* GetFileFilter()
        {
            return "*.scriptcanvas_fn_compiled";
        }

        SubgraphInterfaceData m_interfaceData;

        SubgraphInterfaceAsset(
            const AZ::Data::AssetId& assetId = AZ::Data::AssetId(),
            AZ::Data::AssetData::AssetStatus status = AZ::Data::AssetData::AssetStatus::NotLoaded)
            : AZ::Data::AssetData(assetId, status)
        {
        }
    };
} // namespace ScriptCanvas
