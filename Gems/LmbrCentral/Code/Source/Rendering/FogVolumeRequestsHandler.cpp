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
#include "LmbrCentral_precompiled.h"

#include "FogVolumeRequestsHandler.h"
#include "FogVolumeCommon.h"

namespace LmbrCentral
{
    FogVolumeType FogVolumeComponentRequestsBusHandler::GetVolumeType()
    {
        return GetConfiguration().m_volumeType;
    }

    void FogVolumeComponentRequestsBusHandler::SetVolumeType(FogVolumeType volumeType)
    {
        auto& configuration = GetConfiguration();
        if (configuration.m_volumeType != volumeType)
        {
            configuration.m_volumeType = volumeType;
            RefreshFog();
        }
    }

    AZ::Color FogVolumeComponentRequestsBusHandler::GetColor()
    {
        return GetConfiguration().m_color;
    }

    void FogVolumeComponentRequestsBusHandler::SetColor(AZ::Color color)
    {
        GetConfiguration().m_color = color;
        RefreshFog();
    }

    float FogVolumeComponentRequestsBusHandler::GetHdrDynamic()
    {
        return GetConfiguration().m_hdrDynamic;
    }

    void FogVolumeComponentRequestsBusHandler::SetHdrDynamic(float hdrDynamic)
    {
        GetConfiguration().m_hdrDynamic = hdrDynamic;
        RefreshFog();
    }

    bool FogVolumeComponentRequestsBusHandler::GetUseGlobalFogColor()
    {
        return GetConfiguration().m_useGlobalFogColor;
    }

    void FogVolumeComponentRequestsBusHandler::SetUseGlobalFogColor(bool useGlobalFogColor)
    {
        GetConfiguration().m_useGlobalFogColor = useGlobalFogColor;
        RefreshFog();
    }

    float FogVolumeComponentRequestsBusHandler::GetGlobalDensity()
    {
        return GetConfiguration().m_globalDensity;
    }

    void FogVolumeComponentRequestsBusHandler::SetGlobalDensity(float globalDensity)
    {
        GetConfiguration().m_globalDensity = globalDensity;
        RefreshFog();
    }

    float FogVolumeComponentRequestsBusHandler::GetDensityOffset()
    {
        return GetConfiguration().m_densityOffset;
    }

    void FogVolumeComponentRequestsBusHandler::SetDensityOffset(float densityOffset)
    {
        GetConfiguration().m_densityOffset = densityOffset;
        RefreshFog();
    }

    float FogVolumeComponentRequestsBusHandler::GetNearCutoff()
    {
        return GetConfiguration().m_nearCutoff;
    }

    void FogVolumeComponentRequestsBusHandler::SetNearCutoff(float nearCutoff)
    {
        GetConfiguration().m_nearCutoff = nearCutoff;
        RefreshFog();
    }

    float FogVolumeComponentRequestsBusHandler::GetFallOffDirLong()
    {
        return GetConfiguration().m_fallOffDirLong;
    }

    void FogVolumeComponentRequestsBusHandler::SetFallOffDirLong(float fallOffDirLong)
    {
        GetConfiguration().m_fallOffDirLong = fallOffDirLong;
        RefreshFog();
    }

    float FogVolumeComponentRequestsBusHandler::GetFallOffDirLatitude()
    {
        return GetConfiguration().m_fallOffDirLatitude;
    }

    void FogVolumeComponentRequestsBusHandler::SetFallOffDirLatitude(float fallOffDirLatitude)
    {
        GetConfiguration().m_fallOffDirLatitude = fallOffDirLatitude;
        RefreshFog();
    }

    float FogVolumeComponentRequestsBusHandler::GetFallOffShift()
    {
        return GetConfiguration().m_fallOffShift;
    }

    void FogVolumeComponentRequestsBusHandler::SetFallOffShift(float fallOffShift)
    {
        GetConfiguration().m_fallOffShift = fallOffShift;
        RefreshFog();
    }

    float FogVolumeComponentRequestsBusHandler::GetFallOffScale()
    {
        return GetConfiguration().m_fallOffScale;
    }

    void FogVolumeComponentRequestsBusHandler::SetFallOffScale(float fallOffScale)
    {
        GetConfiguration().m_fallOffScale = fallOffScale;
        RefreshFog();
    }

    float FogVolumeComponentRequestsBusHandler::GetSoftEdges()
    {
        return GetConfiguration().m_softEdges;
    }

    void FogVolumeComponentRequestsBusHandler::SetSoftEdges(float softEdges)
    {
        GetConfiguration().m_softEdges = softEdges;
        RefreshFog();
    }

    float FogVolumeComponentRequestsBusHandler::GetRampStart()
    {
        return GetConfiguration().m_rampStart;
    }

    void FogVolumeComponentRequestsBusHandler::SetRampStart(float rampStart)
    {
        GetConfiguration().m_rampStart = rampStart;
        RefreshFog();
    }

    float FogVolumeComponentRequestsBusHandler::GetRampEnd()
    {
        return GetConfiguration().m_rampEnd;
    }

    void FogVolumeComponentRequestsBusHandler::SetRampEnd(float rampEnd)
    {
        GetConfiguration().m_rampEnd = rampEnd;
        RefreshFog();
    }

    float FogVolumeComponentRequestsBusHandler::GetRampInfluence()
    {
        return GetConfiguration().m_rampInfluence;
    }

    void FogVolumeComponentRequestsBusHandler::SetRampInfluence(float rampInfluence)
    {
        GetConfiguration().m_rampInfluence = rampInfluence;
        RefreshFog();
    }

    float FogVolumeComponentRequestsBusHandler::GetWindInfluence()
    {
        return GetConfiguration().m_windInfluence;
    }

    void FogVolumeComponentRequestsBusHandler::SetWindInfluence(float windInfluence)
    {
        GetConfiguration().m_windInfluence = windInfluence;
        RefreshFog();
    }

    float FogVolumeComponentRequestsBusHandler::GetDensityNoiseScale()
    {
        return GetConfiguration().m_densityNoiseScale;
    }

    void FogVolumeComponentRequestsBusHandler::SetDensityNoiseScale(float densityNoiseScale)
    {
        GetConfiguration().m_densityNoiseScale = densityNoiseScale;
        RefreshFog();
    }

    float FogVolumeComponentRequestsBusHandler::GetDensityNoiseOffset()
    {
        return GetConfiguration().m_densityNoiseOffset;
    }

    void FogVolumeComponentRequestsBusHandler::SetDensityNoiseOffset(float densityNoiseOffset)
    {
        GetConfiguration().m_densityNoiseOffset = densityNoiseOffset;
        RefreshFog();
    }

    float FogVolumeComponentRequestsBusHandler::GetDensityNoiseTimeFrequency()
    {
        return GetConfiguration().m_densityNoiseTimeFrequency;
    }

    void FogVolumeComponentRequestsBusHandler::SetDensityNoiseTimeFrequency(float timeFrequency)
    {
        GetConfiguration().m_densityNoiseTimeFrequency = timeFrequency;
        RefreshFog();
    }

    AZ::Vector3 FogVolumeComponentRequestsBusHandler::GetDensityNoiseFrequency()
    {
        return GetConfiguration().m_densityNoiseFrequency;
    }

    void FogVolumeComponentRequestsBusHandler::SetDensityNoiseFrequency(AZ::Vector3 densityNoiseFrequency)
    {
        GetConfiguration().m_densityNoiseFrequency = densityNoiseFrequency;
        RefreshFog();
    }

    bool FogVolumeComponentRequestsBusHandler::GetIgnoresVisAreas()
    {
        return GetConfiguration().m_ignoresVisAreas;
    }

    void FogVolumeComponentRequestsBusHandler::SetIgnoresVisAreas(bool ignoresVisAreas)
    {
        GetConfiguration().m_ignoresVisAreas = ignoresVisAreas;
        RefreshFog();
    }

    bool FogVolumeComponentRequestsBusHandler::GetAffectsThisAreaOnly()
    {
        return GetConfiguration().m_affectsThisAreaOnly;
    }

    void FogVolumeComponentRequestsBusHandler::SetAffectsThisAreaOnly(bool affectsThisAreaOnly)
    {
        GetConfiguration().m_affectsThisAreaOnly = affectsThisAreaOnly;
        RefreshFog();
    }
}