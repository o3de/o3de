/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
