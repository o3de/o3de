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

#include <Atom/RPI.Reflect/Asset/AssetHandler.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>

namespace AZ
{
    namespace RPI
    {
        //! The StreamingImageAsset's handler with customized loading steps in LoadAssetData override.
        class StreamingImageAssetHandler final
            : public AssetHandler<StreamingImageAsset>
        {
            using Base = AssetHandler<StreamingImageAsset>;
        public:
            Data::AssetHandler::LoadResult LoadAssetData(
                const Data::Asset<Data::AssetData>& asset,
                AZStd::shared_ptr<Data::AssetDataStream> stream,
                const Data::AssetFilterCB& assetLoadFilterCB) override;
            
            //! Enable/disable loading the referenced image mip chain assets when loading StreamingImageAssets
            void SetLoadMipChainsEnabled(bool enabled);

            //! Get the state of whether to load referenced image mip chain assets 
            bool GetLoadMipChainsEnabled() const;

        private:
            bool m_enableLoadMipChains = false;
        };
    }
}
