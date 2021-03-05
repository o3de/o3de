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

#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Color.h>
#include <IEntityRenderState.h>

#include <LmbrCentral/Rendering/FogVolumeComponentBus.h>

struct SFogVolumeProperties;
struct IFogVolumeRenderNode;

namespace LmbrCentral
{
    /**
     * Stores configuration settings for the Fog Volume.
     */
    class FogVolumeConfiguration
    {
    public:
        AZ_TYPE_INFO(FogVolumeConfiguration, "{3B786BBB-0B1D-4EF2-9181-CC75C783C26E}");
        virtual ~FogVolumeConfiguration() {};

        static void Reflect(AZ::ReflectContext* context);

        // Universal rendering properties
        EngineSpec m_minSpec = EngineSpec::Low;
        float m_viewDistMultiplier = 1.0f;

        FogVolumeType m_volumeType = FogVolumeType::Ellipsoid;
        AZ::Color m_color = AZ::Color(1.f, 1.f, 1.f, 1.f);
        // Size is not reflected: taken from BoxShape
        AZ::Vector3 m_size = AZ::Vector3(1.0f, 1.0f, 1.0f);

        float m_hdrDynamic = false;
        bool m_useGlobalFogColor = false;

        float m_globalDensity = 1.0f;
        float m_densityOffset = 0.0f;
        float m_nearCutoff = 0.0f;

        float m_fallOffDirLong = 0.0f;
        float m_fallOffDirLatitude = 90.0f;
        float m_fallOffShift = 0.0f;
        float m_fallOffScale = 1.0f;

        float m_softEdges = 1.0f;

        float m_rampStart = 1.0f;
        float m_rampEnd = 50.0f;
        float m_rampInfluence = 0.0f;
        float m_windInfluence = 1.0f;

        float m_densityNoiseScale = 1.0f;
        float m_densityNoiseOffset = 1.0f;
        float m_densityNoiseTimeFrequency = 0.0f;

        AZ::Vector3 m_densityNoiseFrequency = AZ::Vector3(10.0f, 10.0f, 10.0f);
        bool m_ignoresVisAreas = false;
        bool m_affectsThisAreaOnly = 0.0f;

        void UpdateSizeFromEntityShape();

        // Implemented in EditorFogVolumeConfiguration
        virtual AZ::Crc32 PropertyChanged() { return 0; }

        void SetEntityId(AZ::EntityId id) { m_entityId = id; }

    protected:
        // Not reflected
        AZ::EntityId m_entityId;
    };

    class FogVolume
    {
    public:
        FogVolume();
        FogVolume(const FogVolume&) = delete;
        FogVolume& operator= (const FogVolume&) = delete;

        virtual ~FogVolume();

        void SetEntityId(AZ::EntityId id) { m_entityId = id; }

        void CreateFogVolumeRenderNode(const FogVolumeConfiguration& fogVolumeConfig);
        void DestroyRenderNode();

        IFogVolumeRenderNode* GetRenderNode() const
        {
            return m_fogRenderNode;
        }

        void UpdateFogVolumeProperties(const FogVolumeConfiguration& fogVolumeConfig);
        void UpdateRenderingFlags(const FogVolumeConfiguration& fogVolumeConfig);
        void UpdateFogVolumeTransform();

    private:
        IFogVolumeRenderNode* m_fogRenderNode;
        AZ::EntityId m_entityId;
    };

    namespace FogUtils
    {
        void FogConfigToFogParams(const LmbrCentral::FogVolumeConfiguration& configuration, SFogVolumeProperties& fogVolumeProperties);
    }
}