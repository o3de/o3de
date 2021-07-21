/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/StringFunc/StringFunc.h>

namespace AZ
{
    namespace Render
    {
        using MaterialAssignmentLodIndex = AZ::u64;

        //! MaterialAssignmentId is used to address available and overridable material slots on a model.
        //! The LOD and one of the model's original material asset IDs are used as coordinates that identify
        //! a specific material slot or a set of slots matching either.
        struct MaterialAssignmentId final
        {
            AZ_RTTI(AZ::Render::MaterialAssignmentId, "{EB603581-4654-4C17-B6DE-AE61E79EDA97}");
            AZ_CLASS_ALLOCATOR(AZ::Render::MaterialAssignmentId, SystemAllocator, 0);
            static void Reflect(ReflectContext* context);

            MaterialAssignmentId() = default;

            MaterialAssignmentId(MaterialAssignmentLodIndex lodIndex, const AZ::Data::AssetId& materialAssetId);

            //! Create an ID that maps to all material slots, regardless of asset ID or LOD, effectively applying to an entire model.
            static MaterialAssignmentId CreateDefault();

            //! Create an ID that maps to all material slots with a corresponding asset ID, regardless of LOD.
            static MaterialAssignmentId CreateFromAssetOnly(AZ::Data::AssetId materialAssetId);

            //! Create an ID that maps to a specific material slot with a corresponding asset ID and LOD.
            static MaterialAssignmentId CreateFromLodAndAsset(MaterialAssignmentLodIndex lodIndex, AZ::Data::AssetId materialAssetId);

            //! Returns true if the asset ID and LOD are invalid
            bool IsDefault() const;

            //! Returns true if the asset ID is valid and LOD is invalid
            bool IsAssetOnly() const;

            //! Returns true if the asset ID and LOD are both valid
            bool IsLodAndAsset() const;

            //! Creates a string composed of the asset path and LOD
            AZStd::string ToString() const;

            //! Creates a hash composed of the asset ID sub ID and LOD
            size_t GetHash() const;

            //! Returns true if both asset ID sub IDs and LODs match
            bool operator==(const MaterialAssignmentId& rhs) const;

            //! Returns true if both asset ID sub IDs and LODs do not match
            bool operator!=(const MaterialAssignmentId& rhs) const;

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
} // namespace AZStd
