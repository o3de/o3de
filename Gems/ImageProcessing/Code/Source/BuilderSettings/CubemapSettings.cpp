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

#include "ImageProcessing_precompiled.h"
#include <BuilderSettings/CubemapSettings.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace ImageProcessing
{

    bool CubemapSettings::operator!=(const CubemapSettings& other)
    {
        return !(*this == other);
    }

    bool CubemapSettings::operator==(const CubemapSettings& other)
    {
        return
            m_angle == other.m_angle &&
            m_mipAngle == other.m_mipAngle &&
            m_mipSlope == other.m_mipSlope &&
            m_edgeFixup == other.m_edgeFixup &&
            m_generateDiff == other.m_generateDiff &&
            m_diffuseGenPreset == other.m_diffuseGenPreset;
    }

    void CubemapSettings::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<CubemapSettings>()
                ->Version(1)
                ->Field("Filter", &CubemapSettings::m_filter)
                ->Field("Angle", &CubemapSettings::m_angle)
                ->Field("MipAngle", &CubemapSettings::m_mipAngle)
                ->Field("MipSlope", &CubemapSettings::m_mipSlope)
                ->Field("EdgeFixup", &CubemapSettings::m_edgeFixup)
                ->Field("GenerateDiff", &CubemapSettings::m_generateDiff)
                ->Field("DiffuseProbePreset", &CubemapSettings::m_diffuseGenPreset);
        }
    }
} // namespace ImageProcessing