/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <AzCore/Asset/AssetCommon.h>

namespace Maestro
{
    /**
    * AssetBlend type
    */
    struct AssetBlend
    {
        AZ_TYPE_INFO(AssetBlend, "{90EB921C-456C-4CD8-A487-414219CF123B}");

        AssetBlend()
            : m_time(0.0f)
            , m_blendInTime(0.0f)
            , m_blendOutTime(0.0f)
        {
            m_assetId.SetInvalid();
        }

        AssetBlend(const AZ::Data::AssetId& assetId, float time, float blendInTime, float blendOutTime)
            : m_assetId(assetId)
            , m_time(time)
            , m_blendInTime(blendInTime)
            , m_blendOutTime(blendOutTime)
        {
        }

        bool IsClose(const AssetBlend& rhs, float tolerance) const
        {
            return m_assetId == rhs.m_assetId
                && (fabsf(m_time - rhs.m_time) <= tolerance)
                && (fabsf(m_blendInTime - rhs.m_blendInTime) <= tolerance)
                && (fabsf(m_blendOutTime - rhs.m_blendOutTime) <= tolerance);
        }

        AZ::Data::AssetId m_assetId;
        float m_time;
        float m_blendInTime;
        float m_blendOutTime;
    };

    /**
    * AssetBlends type
    */
    template<typename AssetType>
    struct AssetBlends
    {
        AZ_TYPE_INFO(AssetBlends<AssetType>, "{636A51DA-48E8-4AF9-8310-541E735F2703}");
        AZStd::vector<AssetBlend> m_assetBlends;

        bool IsClose(const AssetBlends& rhs, float tolerance) const
        {
            bool result = m_assetBlends.size() == rhs.m_assetBlends.size();

            for (int x = 0; result && (x < m_assetBlends.size()); x++)
            {
                result = m_assetBlends[x].IsClose(rhs.m_assetBlends[x], tolerance);
            }

            return result;
        }
    };

} // namespace Maestro
