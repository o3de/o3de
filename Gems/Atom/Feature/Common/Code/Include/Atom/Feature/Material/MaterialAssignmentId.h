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

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/StringFunc/StringFunc.h>

namespace AZ
{
    namespace Render
    {
        using MaterialAssignmentLodIndex = AZ::u64;

        struct MaterialAssignmentId final
        {
            AZ_RTTI(AZ::Render::MaterialAssignmentId, "{EB603581-4654-4C17-B6DE-AE61E79EDA97}");
            AZ_CLASS_ALLOCATOR(AZ::Render::MaterialAssignmentId, SystemAllocator, 0);
            static void Reflect(ReflectContext* context);

            MaterialAssignmentId() = default;

            MaterialAssignmentId(MaterialAssignmentLodIndex lodIndex, const AZ::Data::AssetId& materialAssetId)
                : m_lodIndex(lodIndex)
                , m_materialAssetId(materialAssetId)
            {
            }

            static MaterialAssignmentId CreateDefault()
            {
                return MaterialAssignmentId(NonLodIndex, AZ::Data::AssetId());
            }

            static MaterialAssignmentId CreateFromAssetOnly(AZ::Data::AssetId materialAssetId)
            {
                return MaterialAssignmentId(NonLodIndex, materialAssetId);
            }

            static MaterialAssignmentId CreateFromLodAndAsset(MaterialAssignmentLodIndex lodIndex, AZ::Data::AssetId materialAssetId)
            {
                return MaterialAssignmentId(lodIndex, materialAssetId);
            }

            bool IsDefault() const
            {
                return m_lodIndex == NonLodIndex && !m_materialAssetId.IsValid();
            }

            bool IsAssetOnly() const
            {
                return m_lodIndex == NonLodIndex && m_materialAssetId.IsValid();
            }

            bool IsLodAndAsset() const
            {
                return m_lodIndex != NonLodIndex && m_materialAssetId.IsValid();
            }


            AZStd::string ToString() const
            {
                AZStd::string assetPathString;
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPathString, &AZ::Data::AssetCatalogRequests::GetAssetPathById, m_materialAssetId);
                AZ::StringFunc::Path::StripPath(assetPathString);
                AZ::StringFunc::Path::StripExtension(assetPathString);
                return AZStd::string::format("%s:%llu", assetPathString.c_str(), m_lodIndex);
            }

            size_t GetHash() const
            {
                size_t seed = 0;
                AZStd::hash_combine(seed, m_lodIndex);
                AZStd::hash_combine(seed, m_materialAssetId);
                return seed;
            }

            bool operator==(const MaterialAssignmentId& rhs) const
            {
                return m_lodIndex == rhs.m_lodIndex && m_materialAssetId == rhs.m_materialAssetId;
            }

            bool operator!=(const MaterialAssignmentId& rhs) const
            {
                return m_lodIndex != rhs.m_lodIndex || m_materialAssetId != rhs.m_materialAssetId;
            }

            static constexpr MaterialAssignmentLodIndex NonLodIndex = -1;
            MaterialAssignmentLodIndex m_lodIndex = NonLodIndex;
            AZ::Data::AssetId m_materialAssetId = AZ::Data::AssetId();
        };

    } // namespace Render
} // namespace AZ

namespace AZStd
{
    template<>
    struct hash<AZ::Render::MaterialAssignmentId>
    {
        size_t operator()(const AZ::Render::MaterialAssignmentId& id) const
        {
            return id.GetHash();
        }
    };
} //namespace AZStd
