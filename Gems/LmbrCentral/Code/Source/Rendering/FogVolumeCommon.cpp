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

#include <AzCore/Serialization/SerializeContext.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <MathConversion.h>
#include <I3DEngine.h>

#include "FogVolumeCommon.h"
#include "FogVolumeComponent.h"

#include <AzCore/Component/TransformBus.h>

namespace LmbrCentral
{
    FogVolume::FogVolume()
        : m_fogRenderNode(nullptr)
    {
    }

    FogVolume::~FogVolume()
    {
        DestroyRenderNode();
    }

    void FogVolume::CreateFogVolumeRenderNode(const FogVolumeConfiguration& fogVolumeConfig)
    {
        AZ_Assert(m_entityId.IsValid(), "[FogVolumeCommon/FogVolumeComponent Component] Entity id is invalid");
        DestroyRenderNode();

        m_fogRenderNode = static_cast<IFogVolumeRenderNode*>(gEnv->p3DEngine->CreateRenderNode(eERType_FogVolume));
        m_fogRenderNode->SetMinSpec(static_cast<int>(fogVolumeConfig.m_minSpec));
        m_fogRenderNode->SetViewDistanceMultiplier(fogVolumeConfig.m_viewDistMultiplier);
        
        UpdateFogVolumeProperties(fogVolumeConfig);
        UpdateFogVolumeTransform();
    }

    void FogVolume::DestroyRenderNode()
    {
        if (m_fogRenderNode != nullptr)
        {
            m_fogRenderNode->ReleaseNode();
            m_fogRenderNode = nullptr;
        }
    }

    void FogVolume::UpdateFogVolumeProperties(const FogVolumeConfiguration& fogVolumeConfig)
    {
        AZ_Assert(m_fogRenderNode != nullptr, "[FogVolumeCommon/FogVolumeComponent Component] Trying to updated null render node");

        SFogVolumeProperties fogProperties;
        FogUtils::FogConfigToFogParams(fogVolumeConfig, fogProperties);

        m_fogRenderNode->SetFogVolumeProperties(fogProperties);
    }

    void FogVolume::UpdateFogVolumeTransform()
    {
        AZ_Assert(m_fogRenderNode != nullptr, "[FogVolumeCommon/FogVolumeComponent Component] Trying to updated null render node");

        AZ::Transform parentTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(parentTransform, m_entityId, &AZ::TransformBus::Events::GetWorldTM);

        m_fogRenderNode->SetScale(AZVec3ToLYVec3(parentTransform.GetScale()));
        m_fogRenderNode->SetMatrix(AZTransformToLYTransform(parentTransform));
    }

    void FogVolume::UpdateRenderingFlags(const FogVolumeConfiguration& fogVolumeConfig)
    {
        if (m_fogRenderNode)
        {
            m_fogRenderNode->SetMinSpec(static_cast<AZ::u32>(fogVolumeConfig.m_minSpec));

            if (gEnv && gEnv->p3DEngine)
            {
                const int configSpec = gEnv->pSystem->GetConfigSpec(true);

                AZ::u32 rendFlags = static_cast<AZ::u32>(m_fogRenderNode->GetRndFlags());

                const bool hidden = static_cast<AZ::u32>(configSpec) < static_cast<AZ::u32>(fogVolumeConfig.m_minSpec);
                if (hidden)
                {
                    rendFlags |= ERF_HIDDEN;
                }
                else
                {
                    rendFlags &= ~ERF_HIDDEN;
                }

                rendFlags |= ERF_COMPONENT_ENTITY;
                m_fogRenderNode->SetRndFlags(rendFlags);
            }
        }
    }

    void FogUtils::FogConfigToFogParams(const LmbrCentral::FogVolumeConfiguration& configuration, SFogVolumeProperties& fogVolumeProperties)
    {
        AZ_Assert(configuration.m_volumeType != FogVolumeType::None, "[FogConfigToFogParams] Attempting to create a fog with invalid volume type")

        fogVolumeProperties.m_volumeType = static_cast<int>(configuration.m_volumeType);
        fogVolumeProperties.m_size = AZVec3ToLYVec3(configuration.m_size);
        fogVolumeProperties.m_color = AZColorToLYVec3(configuration.m_color);
        fogVolumeProperties.m_useGlobalFogColor = configuration.m_useGlobalFogColor;
        fogVolumeProperties.m_ignoresVisAreas = configuration.m_ignoresVisAreas;
        fogVolumeProperties.m_affectsThisAreaOnly = configuration.m_affectsThisAreaOnly;

        fogVolumeProperties.m_globalDensity = max(0.01f, configuration.m_globalDensity);
        fogVolumeProperties.m_densityOffset = configuration.m_densityOffset;
        fogVolumeProperties.m_nearCutoff = configuration.m_nearCutoff;
        fogVolumeProperties.m_fHDRDynamic = configuration.m_hdrDynamic;
        fogVolumeProperties.m_softEdges = configuration.m_softEdges;

        fogVolumeProperties.m_heightFallOffDirLong = configuration.m_fallOffDirLong;
        fogVolumeProperties.m_heightFallOffDirLati = configuration.m_fallOffDirLatitude;
        fogVolumeProperties.m_heightFallOffShift = configuration.m_fallOffShift;
        fogVolumeProperties.m_heightFallOffScale = configuration.m_fallOffScale;

        fogVolumeProperties.m_rampStart = configuration.m_rampStart;
        fogVolumeProperties.m_rampEnd = configuration.m_rampEnd;
        fogVolumeProperties.m_rampInfluence = configuration.m_rampInfluence;
        fogVolumeProperties.m_windInfluence = configuration.m_windInfluence;

        fogVolumeProperties.m_densityNoiseScale = configuration.m_densityNoiseScale;
        fogVolumeProperties.m_densityNoiseOffset = configuration.m_densityNoiseOffset;
        fogVolumeProperties.m_densityNoiseTimeFrequency = configuration.m_densityNoiseTimeFrequency;
        fogVolumeProperties.m_densityNoiseFrequency = AZVec3ToLYVec3(configuration.m_densityNoiseFrequency);
    }

    void FogVolumeConfiguration::Reflect(AZ::ReflectContext * context)
    {
        if (AZ::SerializeContext *serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<FogVolumeConfiguration>()
                ->Version(1)
                ->Field("VolumeType", &FogVolumeConfiguration::m_volumeType)
                ->Field("Color", &FogVolumeConfiguration::m_color)
                ->Field("HdrDynamic", &FogVolumeConfiguration::m_hdrDynamic)
                ->Field("UseGlobalFogColor", &FogVolumeConfiguration::m_useGlobalFogColor)
                ->Field("SoftEdges", &FogVolumeConfiguration::m_softEdges)
                ->Field("WindInfluence", &FogVolumeConfiguration::m_windInfluence)
                ->Field("GlobalDensity", &FogVolumeConfiguration::m_globalDensity)
                ->Field("DensityOffset", &FogVolumeConfiguration::m_densityOffset)
                ->Field("NearCutoff", &FogVolumeConfiguration::m_nearCutoff)
                ->Field("EngineSpec", &FogVolumeConfiguration::m_minSpec)
                ->Field("DistMult", &FogVolumeConfiguration::m_viewDistMultiplier)
                ->Field("IgnoresVisAreas", &FogVolumeConfiguration::m_ignoresVisAreas)
                ->Field("AffectsThisAreaOnly", &FogVolumeConfiguration::m_affectsThisAreaOnly)
                ->Field("FallOffDirLong", &FogVolumeConfiguration::m_fallOffDirLong)
                ->Field("FallOffDirLatitude", &FogVolumeConfiguration::m_fallOffDirLatitude)
                ->Field("FallOffShift", &FogVolumeConfiguration::m_fallOffShift)
                ->Field("FallOffScale", &FogVolumeConfiguration::m_fallOffScale)
                ->Field("RampStart", &FogVolumeConfiguration::m_rampStart)
                ->Field("RampEnd", &FogVolumeConfiguration::m_rampEnd)
                ->Field("RampInfluence", &FogVolumeConfiguration::m_rampInfluence)
                ->Field("DensityNoiseScale", &FogVolumeConfiguration::m_densityNoiseScale)
                ->Field("DensityNoiseOffset", &FogVolumeConfiguration::m_densityNoiseOffset)
                ->Field("DensityNoiseTimeFrequency", &FogVolumeConfiguration::m_densityNoiseTimeFrequency)
                ->Field("DensityNoiseFrequency", &FogVolumeConfiguration::m_densityNoiseFrequency)
                ;
        }
    }

    void FogVolumeConfiguration::UpdateSizeFromEntityShape()
    {
        AZ_Assert(m_entityId.IsValid(), "[FogVolumeConfiguration] Entity id is invalid");
        BoxShapeComponentRequestsBus::EventResult(m_size, m_entityId, &BoxShapeComponentRequestsBus::Events::GetBoxDimensions);
    }
}
