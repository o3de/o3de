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

            void Begin(const Data::AssetId& assetId, MaterialAsset& parentMaterial);
            void Begin(const Data::AssetId& assetId, MaterialTypeAsset& materialType);
            bool End(Data::Asset<MaterialAsset>& result);

        private:
            const MaterialPropertiesLayout* m_materialPropertiesLayout = nullptr;
        };
    } // namespace RPI
} // namespace AZ
