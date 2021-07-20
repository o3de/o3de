/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CoreLights/LtcCommon.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>

namespace AZ::Render
{

    static const char* s_LtcGgxMatrixPath = "textures/ltc/ltc_mat_lutrgba32f.dds.streamingimage";
    static const char* s_LtcGgxAmplificationPath = "textures/ltc/ltc_amp_lutrg32f.dds.streamingimage";

    LtcCommon::LtcCommon()
    {
        Interface<ILtcCommon>::Register(this);
    }

    LtcCommon::~LtcCommon()
    {
        Interface<ILtcCommon>::Unregister(this);
    }

    void LtcCommon::LoadMatricesForSrg(Data::Instance<RPI::ShaderResourceGroup> srg)
    {
        if (!srg)
        {
            return;
        }

        AZ_Error("LtcCommon", srg->GetId().IsValid(), "Srg used to load matrices must have a valid Id.");

        auto callback = [srg](Data::Asset<Data::AssetData> asset, bool success, const char* srgName)
        {
            RHI::ShaderInputImageIndex index = srg->FindShaderInputImageIndex(Name(srgName));
            if (success && index.IsValid())
            {
                auto streamingImageInstance = RPI::StreamingImage::FindOrCreate(asset);
                srg->SetImage(index, streamingImageInstance);
            }
        };
        
        // De-duplicate load requests by the srg's guid to avoid holding a refernce to the srg itself.
        AZ::Uuid srgGuid = srg->GetId().m_guid;
        if (m_assetLoaders.find(srgGuid) == m_assetLoaders.end())
        {
            m_assetLoaders[srgGuid].push_back(RPI::AssetUtils::AsyncAssetLoader::Create<RPI::StreamingImageAsset>(s_LtcGgxAmplificationPath, 0,
                AZStd::bind(callback, AZStd::placeholders::_1, AZStd::placeholders::_2, "m_ltcAmplification")));
            m_assetLoaders[srgGuid].push_back(RPI::AssetUtils::AsyncAssetLoader::Create<RPI::StreamingImageAsset>(s_LtcGgxMatrixPath, 0,
                AZStd::bind(callback, AZStd::placeholders::_1, AZStd::placeholders::_2, "m_ltcMatrix")));
        }
    }
}
