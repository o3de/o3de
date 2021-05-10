/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
*
*/
#pragma once

#include <AzCore/Asset/AssetCommon.h>

namespace AZ
{

/** IAssetBlendKey used in AssetBlend animation track.
*/
struct IAssetBlendKey
    : public ITimeRangeKey
{
    AZ::Data::AssetId m_assetId;    //!< Asset Id
    AZStd::string m_description;    //!< Description (filename part of path)
    float m_blendInTime;            //!< Blend in time in seconds;
    float m_blendOutTime;            //!< Blend in time in seconds;

    IAssetBlendKey()
        : ITimeRangeKey()
        , m_blendInTime(0.0f)
        , m_blendOutTime(0.0f)
    {
    }
};

    AZ_TYPE_INFO_SPECIALIZE(IAssetBlendKey, "{15B82C3A-6DB8-466F-AF7F-18298FCD25FD}");
}
