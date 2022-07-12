/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/unordered_map.h>

#include <AzFramework/Physics/Material/PhysicsMaterialSlots.h>
#include <AzFramework/Physics/Material/PhysicsMaterialAsset.h>

// O3DE_DEPRECATION_NOTICE(GHI-9840)
// Utilities used for legacy material conversion.

namespace Physics::Utils
{
    inline const char* FbxAssetInfoExtension = "assetinfo";

    using LegacyMaterialIdToNewAssetIdMap = AZStd::unordered_map<AZ::Uuid, AZ::Data::AssetId>;

    AZStd::optional<AZStd::string> GetFullSourceAssetPathById(AZ::Data::AssetId assetId);

    AZ::Data::Asset<Physics::MaterialAsset> ConvertLegacyMaterialIdToMaterialAsset(
        const PhysicsLegacy::MaterialId& legacyMaterialId,
        const LegacyMaterialIdToNewAssetIdMap& legacyMaterialIdToNewAssetIdMap);

    Physics::MaterialSlots ConvertLegacyMaterialSelectionToMaterialSlots(
        const PhysicsLegacy::MaterialSelection& legacyMaterialSelection,
        const LegacyMaterialIdToNewAssetIdMap& legacyMaterialIdToNewAssetIdMap);

    AZStd::set<AZStd::string> CollectFbxManifestsFromAssetType(AZ::TypeId assetType);

    bool IsDefaultMaterialSlots(const Physics::MaterialSlots& materialSlots);

    //! Bus to allow any gem to fix their assets that are using
    //! legacy material libraries.
    class PhysicsMaterialConversionRequests
        : public AZ::EBusTraits
    {
    public:
        virtual void FixPhysicsLegacyMaterials(const LegacyMaterialIdToNewAssetIdMap& legacyMaterialIdToNewAssetIdMap) = 0;
    };

    using PhysicsMaterialConversionRequestBus = AZ::EBus<PhysicsMaterialConversionRequests>;
} // namespace Physics::Utils
