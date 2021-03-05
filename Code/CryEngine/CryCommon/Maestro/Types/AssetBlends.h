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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#pragma once

#include <AzCore/Serialization/SerializeContext.h>
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