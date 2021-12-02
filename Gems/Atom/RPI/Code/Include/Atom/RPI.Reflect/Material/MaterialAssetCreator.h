/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Reflect/AssetCreator.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialAssetCreatorCommon.h>

namespace AZ
{
    namespace RPI
    {
        //! Use a MaterialAssetCreator to create and configure a new MaterialAsset.
        //! The MaterialAsset will be based on a MaterialTypeAsset or another MaterialAsset.
        //! Either way, the base provides the necessary data to define the layout 
        //! and behavior of the material. The MaterialAsset only provides property value overrides.
        class MaterialAssetCreator
            : public AssetCreator<MaterialAsset>
            , public MaterialAssetCreatorCommon
        {
        public:
            friend class MaterialSourceData;

            void Begin(const Data::AssetId& assetId, MaterialAsset& parentMaterial, bool includeMaterialPropertyNames = true);
            void Begin(const Data::AssetId& assetId, MaterialTypeAsset& materialType, bool includeMaterialPropertyNames = true);
            bool End(Data::Asset<MaterialAsset>& result);

        private:
            void PopulatePropertyNameList();

            const MaterialPropertiesLayout* m_materialPropertiesLayout = nullptr;
        };
    } // namespace RPI
} // namespace AZ
