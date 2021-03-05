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
#include <Atom/RPI.Reflect/Model/ModelAsset.h>

namespace AZ
{
    namespace RPI
    {
        //! Constructs an instance of a ModelAsset
        class ModelAssetCreator
            : public AssetCreator<ModelAsset>
        {
        public:
            ModelAssetCreator() = default;
            ~ModelAssetCreator() = default;

            //! Begins construction of a new ModelAsset instance. Resets the builder to a fresh state
            //! @param assetId The unique id to use when creating the asset
            void Begin(const Data::AssetId& assetId);

            //! Assigns a name to the model
            void SetName(AZStd::string_view name);

            //! Adds a Lod to the model.
            void AddLodAsset(Data::Asset<ModelLodAsset>&& lodAsset);
            
            //! Finalizes and assigns ownership of the asset to result, if successful. 
            //! Otherwise false is returned and result is left untouched.
            bool End(Data::Asset<ModelAsset>& result);

        private:
            AZ::Aabb m_modelAabb;
        };
    } // namespace RPI
} // namespace AZ
