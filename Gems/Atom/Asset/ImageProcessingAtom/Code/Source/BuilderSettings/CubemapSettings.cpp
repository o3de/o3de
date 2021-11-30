/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <BuilderSettings/CubemapSettings.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace ImageProcessingAtom
{
    bool CubemapSettings::operator!=(const CubemapSettings& other)
    {
        return !(*this == other);
    }

    bool CubemapSettings::operator==(const CubemapSettings& other)
    {
        return
            m_filter == other.m_filter &&
            m_angle == other.m_angle &&
            m_mipAngle == other.m_mipAngle &&
            m_mipSlope == other.m_mipSlope &&
            m_edgeFixup == other.m_edgeFixup &&
            m_generateIBLSpecular == other.m_generateIBLSpecular &&
            m_iblSpecularPreset == other.m_iblSpecularPreset &&
            m_generateIBLDiffuse == other.m_generateIBLDiffuse &&
            m_iblDiffusePreset == other.m_iblDiffusePreset &&
            m_requiresConvolve == other.m_requiresConvolve &&
            m_subId == other.m_subId;
    }

    void CubemapSettings::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<CubemapSettings>()
                ->Version(2)
                ->Field("Filter", &CubemapSettings::m_filter)
                ->Field("Angle", &CubemapSettings::m_angle)
                ->Field("MipAngle", &CubemapSettings::m_mipAngle)
                ->Field("MipSlope", &CubemapSettings::m_mipSlope)
                ->Field("EdgeFixup", &CubemapSettings::m_edgeFixup)
                ->Field("GenerateIBLSpecular", &CubemapSettings::m_generateIBLSpecular)
                ->Field("IBLSpecularPreset", &CubemapSettings::m_iblSpecularPreset)
                ->Field("GenerateIBLDiffuse", &CubemapSettings::m_generateIBLDiffuse)
                ->Field("IBLDiffusePreset", &CubemapSettings::m_iblDiffusePreset)
                ->Field("RequiresConvolve", &CubemapSettings::m_requiresConvolve)
                ->Field("SubId", &CubemapSettings::m_subId);
        }
    }
} // namespace ImageProcessingAtom
