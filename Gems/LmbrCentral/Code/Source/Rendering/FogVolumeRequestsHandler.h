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

#include <AzCore/Math/Color.h>
#include <LmbrCentral/Rendering/FogVolumeComponentBus.h>

namespace LmbrCentral
{
    class FogVolumeComponentRequestsBusHandler
        : public FogVolumeComponentRequestBus::Handler
    {
        /////////////////////////////////////////////
        // FogVolumeComponentRequestBus::Handler
        FogVolumeType GetVolumeType() override;
        void SetVolumeType(FogVolumeType volumeType) override;

        AZ::Color GetColor() override;
        void SetColor(AZ::Color color) override;

        float GetHdrDynamic() override;
        void SetHdrDynamic(float hdrDynamic) override;

        bool GetUseGlobalFogColor() override;
        void SetUseGlobalFogColor(bool useGlobalFogColor) override;

        float GetGlobalDensity() override;
        void SetGlobalDensity(float globalDensity) override;

        float GetDensityOffset() override;
        void SetDensityOffset(float densityOffset) override;

        float GetNearCutoff() override;
        void SetNearCutoff(float nearCutoff) override;

        float GetFallOffDirLong() override;
        void SetFallOffDirLong(float fallOffDirLong) override;

        float GetFallOffDirLatitude() override;
        void SetFallOffDirLatitude(float fallOffDirLatitude) override;

        float GetFallOffShift() override;
        void SetFallOffShift(float fallOffShift) override;

        float GetFallOffScale() override;
        void SetFallOffScale(float fallOffScale) override;

        float GetSoftEdges() override;
        void SetSoftEdges(float softEdges) override;

        float GetRampStart() override;
        void SetRampStart(float rampStart) override;

        float GetRampEnd() override;
        void SetRampEnd(float rampEnd) override;

        float GetRampInfluence() override;
        void SetRampInfluence(float rampInfluence) override;

        float GetWindInfluence() override;
        void SetWindInfluence(float windInfluence) override;

        float GetDensityNoiseScale() override;
        void SetDensityNoiseScale(float densityNoiseScale) override;

        float GetDensityNoiseOffset() override;
        void SetDensityNoiseOffset(float densityNoiseOffset) override;

        float GetDensityNoiseTimeFrequency() override;
        void SetDensityNoiseTimeFrequency(float timeFrequency) override;

        AZ::Vector3 GetDensityNoiseFrequency() override;
        void SetDensityNoiseFrequency(AZ::Vector3 densityNoiseFrequency) override;

        bool GetIgnoresVisAreas() override;
        void SetIgnoresVisAreas(bool ignoresVisAreas) override;

        bool GetAffectsThisAreaOnly() override;
        void SetAffectsThisAreaOnly(bool affectsThisAreaOnly) override;
        /////////////////////////////////////////////

    protected:
        virtual FogVolumeConfiguration& GetConfiguration() = 0;
    };
}