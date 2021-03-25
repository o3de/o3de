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
#include <Atom/RPI.Reflect/Model/MorphTargetMetaAsset.h>

namespace AZ::RPI
{
    //! Constructs an instance of a MorphTargetMetaAsset
    class MorphTargetMetaAssetCreator
        : public AssetCreator<MorphTargetMetaAsset>
    {
    public:
        MorphTargetMetaAssetCreator() = default;
        ~MorphTargetMetaAssetCreator() = default;

        //! Begin construction of a new MorphTargetMetaAsset instance. Resets the builder to a fresh state.
        //! @param assetId The unique id to use when creating the asset.
        void Begin(const Data::AssetId& assetId);

        //! Add meta data for a morph target.
        //! @param morphTarget Meta target for the morph target.
        void AddMorphTarget(const MorphTargetMetaAsset::MorphTarget& morphTarget);

        //! Check if any morph target meta datas got added.
        bool IsEmpty();

        //! Finalize and assigns ownership of the asset to result, if successful. 
        //! Otherwise false is returned and result is left untouched.
        bool End(Data::Asset<MorphTargetMetaAsset>& result);
    };
} // namespace AZ::RPI
