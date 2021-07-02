/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/Material/MaterialAssignment.h>
#include <Atom/RPI.Edit/Material/MaterialSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialTypeAsset.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    namespace Render
    {
        namespace EditorMaterialComponentUtil
        {
            struct MaterialEditData
            {
                AZ::Data::AssetId m_materialAssetId = {};
                AZ::Data::Asset<AZ::RPI::MaterialAsset> m_materialAsset = {};
                AZ::Data::Asset<AZ::RPI::MaterialTypeAsset> m_materialTypeAsset = {};
                AZ::Data::Asset<AZ::RPI::MaterialAsset> m_materialParentAsset = {};
                AZ::RPI::MaterialSourceData m_materialSourceData;
                AZ::RPI::MaterialTypeSourceData m_materialTypeSourceData;
                AZStd::string m_materialSourcePath;
                AZStd::string m_materialTypeSourcePath;
                AZStd::string m_materialParentSourcePath;
                MaterialPropertyOverrideMap m_materialPropertyOverrideMap = {};
            };

            bool LoadMaterialEditDataFromAssetId(const AZ::Data::AssetId& assetId, MaterialEditData& editData);
            bool SaveSourceMaterialFromEditData(const AZStd::string& path, const MaterialEditData& editData);
        } // namespace EditorMaterialComponentUtil
    } // namespace Render
} // namespace AZ
