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

namespace AZ
{
    namespace RPI
    {
        //! Use a MaterialAssetCreator to create and configure a new MaterialAsset.
        //! The MaterialAsset will be based on a MaterialTypeAsset or another MaterialAsset.
        //! Either way, the base provides the necessary data to define the layout 
        //! and behavior of the material. The MaterialAsset only provides property value overrides.
        //! Note however that the MaterialTypeAsset does not have to be loaded and available yet;
        //! only the AssetId is required. The resulting MaterialAsset will be in a non-finalized state;
        //! it must be finalized afterwards when the MaterialTypeAsset is available before it can be used.
        class MaterialAssetCreator  
            : public AssetCreator<MaterialAsset>
        {
        public:
            friend class MaterialSourceData;
            
            void Begin(const Data::AssetId& assetId, const Data::Asset<MaterialTypeAsset>& materialType);
            bool End(Data::Asset<MaterialAsset>& result);

            void SetMaterialTypeVersion(uint32_t version);
            
            void SetPropertyValue(const Name& name, const MaterialPropertyValue& value);
        };
    } // namespace RPI
} // namespace AZ
