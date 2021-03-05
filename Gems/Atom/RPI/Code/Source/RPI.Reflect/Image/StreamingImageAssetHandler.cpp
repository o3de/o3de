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

#include <Atom/RPI.Reflect/Image/StreamingImageAssetHandler.h>

namespace AZ
{
    namespace RPI
    {
        void StreamingImageAssetHandler::SetLoadMipChainsEnabled(bool enabled)
        {
            m_enableLoadMipChains = enabled;
        }

        bool StreamingImageAssetHandler::GetLoadMipChainsEnabled() const
        {
            return m_enableLoadMipChains;
        }

        Data::AssetHandler::LoadResult StreamingImageAssetHandler::LoadAssetData(
            const Data::Asset<Data::AssetData>& asset,
            AZStd::shared_ptr<Data::AssetDataStream> stream,
            const Data::AssetFilterCB& assetLoadFilterCB)
        {
            // Using customized filter instead using the input one. 
            // We can do this because the StreamingImageAsset owns all the ImageMipChainAssets that it references. 
            // NOTE: We can't set the mip chain assets' flags in StreamingImageAsset's constructor since the array is empty at that time.
            AZ_UNUSED(assetLoadFilterCB);

            StreamingImageAsset* assetData = asset.GetAs<StreamingImageAsset>();
            AZ_Assert(assetData, "Asset is of the wrong type.");
            AZ_Assert(m_serializeContext, "Unable to retrieve serialize context.");

            Data::AssetHandler::LoadResult loadResult = Data::AssetHandler::LoadResult::Error;
            if (assetData)
            {
                // Setup filter to not load the mipchain assets referenced internally. 
                ObjectStream::FilterDescriptor filter(m_enableLoadMipChains? nullptr : Data::AssetFilterNoAssetLoading, 0);

                loadResult = Utils::LoadObjectFromStreamInPlace<StreamingImageAsset>(*stream, *assetData, m_serializeContext, filter) ?
                    Data::AssetHandler::LoadResult::LoadComplete :
                    Data::AssetHandler::LoadResult::Error;
                
                if (loadResult == Data::AssetHandler::LoadResult::LoadComplete)
                {
                    // ImageMipChainAsset has some internal variables need to initialized after it was loaded.
                    StreamingImageAsset* assetData2 = asset.GetAs<StreamingImageAsset>();
                    assetData2->m_tailMipChain.Init();
                }
            }

            return loadResult;
        }
    }
}
