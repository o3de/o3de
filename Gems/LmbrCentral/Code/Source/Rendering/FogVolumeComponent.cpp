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
#include <AzCore/RTTI/BehaviorContext.h>
#include <IEntityRenderState.h>

#include "FogVolumeComponent.h"

namespace LmbrCentral
{
    const float FogVolumeComponent::s_renderNodeRequestBusOrder = 500.f;

    void FogVolumeComponent::Activate()
    {
        const auto entityId = GetEntityId();

        m_configuration.SetEntityId(entityId);
        m_configuration.UpdateSizeFromEntityShape();

        m_fogVolume.SetEntityId(entityId);
        m_fogVolume.CreateFogVolumeRenderNode(m_configuration);

        RefreshFog();

        RenderNodeRequestBus::Handler::BusConnect(entityId);
        FogVolumeComponentRequestBus::Handler::BusConnect(entityId);
        ShapeComponentNotificationsBus::Handler::BusConnect(entityId);
        AZ::TransformNotificationBus::Handler::BusConnect(entityId);
    }

    void FogVolumeComponent::Deactivate()
    {
        m_fogVolume.DestroyRenderNode();
        m_fogVolume.SetEntityId(AZ::EntityId());
        m_configuration.SetEntityId(AZ::EntityId());

        RenderNodeRequestBus::Handler::BusDisconnect();
        FogVolumeComponentRequestBus::Handler::BusDisconnect();
        ShapeComponentNotificationsBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
    }

    void FogVolumeComponent::Reflect(AZ::ReflectContext* context)
    {
        FogVolumeConfiguration::Reflect(context);

        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<FogVolumeComponent, AZ::Component>()
                ->Version(1)
                ->Field("FogVolumeConfiguration", &FogVolumeComponent::m_configuration);
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<FogVolumeComponent>()->RequestBus("FogVolumeComponentRequestBus");
            FogVolumeComponent::ExposeRequestsBusInBehaviorContext(behaviorContext, "FogVolumeComponentRequestBus");
        }
    }

    IRenderNode* FogVolumeComponent::GetRenderNode()
    {
        return m_fogVolume.GetRenderNode();
    }

    float FogVolumeComponent::GetRenderNodeRequestBusOrder() const
    {
        return s_renderNodeRequestBusOrder;
    }

    void FogVolumeComponent::RefreshFog()
    {
        m_fogVolume.UpdateFogVolumeProperties(m_configuration);
        m_fogVolume.UpdateRenderingFlags(m_configuration);
        m_fogVolume.UpdateFogVolumeTransform();
    }

    void FogVolumeComponent::OnTransformChanged([[maybe_unused]] const AZ::Transform & local, [[maybe_unused]] const AZ::Transform & world)
    {
        RefreshFog();
    }

    void FogVolumeComponent::OnShapeChanged(ShapeComponentNotifications::ShapeChangeReasons changeReason)
    {
        if (changeReason == ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged)
        {
            m_configuration.UpdateSizeFromEntityShape();
            RefreshFog();
        }
    }

    void FogVolumeComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType & required)
    {
        required.push_back(AZ_CRC("BoxShapeService", 0x946a0032));
    }

    void FogVolumeComponent::ExposeRequestsBusInBehaviorContext(AZ::BehaviorContext* behaviorContext, const char* name)
    {
        behaviorContext->EBus<FogVolumeComponentRequestBus>(name)
            ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::All)
            ->Event("RefreshFog", &FogVolumeComponentRequestBus::Events::RefreshFog)

            ->Event("GetVolumeType", &FogVolumeComponentRequestBus::Events::GetVolumeType)
            ->Event("SetVolumeType", &FogVolumeComponentRequestBus::Events::SetVolumeType)
            ->VirtualProperty("VolumeType", "GetVolumeType", "SetVolumeType")

            ->Event("GetColor", &FogVolumeComponentRequestBus::Events::GetColor)
            ->Event("SetColor", &FogVolumeComponentRequestBus::Events::SetColor)
            ->VirtualProperty("Color", "GetColor", "SetColor")

            ->Event("GetHdrDynamic", &FogVolumeComponentRequestBus::Events::GetHdrDynamic)
            ->Event("SetHdrDynamic", &FogVolumeComponentRequestBus::Events::SetHdrDynamic)
            ->VirtualProperty("HdrDynamic", "GetHdrDynamic", "SetHdrDynamic")

            ->Event("GetUseGlobalFogColor", &FogVolumeComponentRequestBus::Events::GetUseGlobalFogColor)
            ->Event("SetUseGlobalFogColor", &FogVolumeComponentRequestBus::Events::SetUseGlobalFogColor)
            ->VirtualProperty("UseGlobalFogColor", "GetUseGlobalFogColor", "SetUseGlobalFogColor")

            ->Event("GetGlobalDensity", &FogVolumeComponentRequestBus::Events::GetGlobalDensity)
            ->Event("SetGlobalDensity", &FogVolumeComponentRequestBus::Events::SetGlobalDensity)
            ->VirtualProperty("GlobalDensity", "GetGlobalDensity", "SetGlobalDensity")

            ->Event("GetDensityOffset", &FogVolumeComponentRequestBus::Events::GetDensityOffset)
            ->Event("SetDensityOffset", &FogVolumeComponentRequestBus::Events::SetDensityOffset)
            ->VirtualProperty("DensityOffset", "GetDensityOffset", "SetDensityOffset")

            ->Event("GetNearCutoff", &FogVolumeComponentRequestBus::Events::GetNearCutoff)
            ->Event("SetNearCutoff", &FogVolumeComponentRequestBus::Events::SetNearCutoff)
            ->VirtualProperty("NearCutoff", "GetNearCutoff", "SetNearCutoff")

            ->Event("GetFallOffDirLong", &FogVolumeComponentRequestBus::Events::GetFallOffDirLong)
            ->Event("SetFallOffDirLong", &FogVolumeComponentRequestBus::Events::SetFallOffDirLong)
            ->VirtualProperty("FallOffDirLong", "GetFallOffDirLong", "SetFallOffDirLong")

            ->Event("GetFallOffDirLatitude", &FogVolumeComponentRequestBus::Events::GetFallOffDirLatitude)
            ->Event("SetFallOffDirLatitude", &FogVolumeComponentRequestBus::Events::SetFallOffDirLatitude)
            ->VirtualProperty("FallOffDirLatitude", "GetFallOffDirLatitude", "SetFallOffDirLatitude")

            ->Event("GetFallOffShift", &FogVolumeComponentRequestBus::Events::GetFallOffShift)
            ->Event("SetFallOffShift", &FogVolumeComponentRequestBus::Events::SetFallOffShift)
            ->VirtualProperty("FallOffShift", "GetFallOffShift", "SetFallOffShift")

            ->Event("GetFallOffScale", &FogVolumeComponentRequestBus::Events::GetFallOffScale)
            ->Event("SetFallOffScale", &FogVolumeComponentRequestBus::Events::SetFallOffScale)
            ->VirtualProperty("FallOffScale", "GetFallOffScale", "SetFallOffScale")

            ->Event("GetSoftEdges", &FogVolumeComponentRequestBus::Events::GetSoftEdges)
            ->Event("SetSoftEdges", &FogVolumeComponentRequestBus::Events::SetSoftEdges)
            ->VirtualProperty("SoftEdges", "GetSoftEdges", "SetSoftEdges")

            ->Event("GetRampStart", &FogVolumeComponentRequestBus::Events::GetRampStart)
            ->Event("SetRampStart", &FogVolumeComponentRequestBus::Events::SetRampStart)
            ->VirtualProperty("RampStart", "GetRampStart", "SetRampStart")

            ->Event("GetRampEnd", &FogVolumeComponentRequestBus::Events::GetRampEnd)
            ->Event("SetRampEnd", &FogVolumeComponentRequestBus::Events::SetRampEnd)
            ->VirtualProperty("RampEnd", "GetRampEnd", "SetRampEnd")

            ->Event("GetRampInfluence", &FogVolumeComponentRequestBus::Events::GetRampInfluence)
            ->Event("SetRampInfluence", &FogVolumeComponentRequestBus::Events::SetRampInfluence)
            ->VirtualProperty("RampInfluence", "GetRampInfluence", "SetRampInfluence")

            ->Event("GetWindInfluence", &FogVolumeComponentRequestBus::Events::GetWindInfluence)
            ->Event("SetWindInfluence", &FogVolumeComponentRequestBus::Events::SetWindInfluence)
            ->VirtualProperty("WindInfluence", "GetWindInfluence", "SetWindInfluence")

            ->Event("GetDensityNoiseScale", &FogVolumeComponentRequestBus::Events::GetDensityNoiseScale)
            ->Event("SetDensityNoiseScale", &FogVolumeComponentRequestBus::Events::SetDensityNoiseScale)
            ->VirtualProperty("DensityNoiseScale", "GetDensityNoiseScale", "SetDensityNoiseScale")

            ->Event("GetDensityNoiseOffset", &FogVolumeComponentRequestBus::Events::GetDensityNoiseOffset)
            ->Event("SetDensityNoiseOffset", &FogVolumeComponentRequestBus::Events::SetDensityNoiseOffset)
            ->VirtualProperty("DensityNoiseOffset", "GetDensityNoiseOffset", "SetDensityNoiseOffset")

            ->Event("GetDensityNoiseTimeFrequency", &FogVolumeComponentRequestBus::Events::GetDensityNoiseTimeFrequency)
            ->Event("SetDensityNoiseTimeFrequency", &FogVolumeComponentRequestBus::Events::SetDensityNoiseTimeFrequency)
            ->VirtualProperty("DensityNoiseTimeFrequency", "GetDensityNoiseTimeFrequency", "SetDensityNoiseTimeFrequency")

            ->Event("GetDensityNoiseFrequency", &FogVolumeComponentRequestBus::Events::GetDensityNoiseFrequency)
            ->Event("SetDensityNoiseFrequency", &FogVolumeComponentRequestBus::Events::SetDensityNoiseFrequency)
            ->VirtualProperty("DensityNoiseFrequency", "GetDensityNoiseFrequency", "SetDensityNoiseFrequency")

            ->Event("GetIgnoresVisAreas", &FogVolumeComponentRequestBus::Events::GetIgnoresVisAreas)
            ->Event("SetIgnoresVisAreas", &FogVolumeComponentRequestBus::Events::SetIgnoresVisAreas)
            ->VirtualProperty("IgnoresVisAreas", "GetIgnoresVisAreas", "SetIgnoresVisAreas")

            ->Event("GetAffectsThisAreaOnly", &FogVolumeComponentRequestBus::Events::GetAffectsThisAreaOnly)
            ->Event("SetAffectsThisAreaOnly", &FogVolumeComponentRequestBus::Events::SetAffectsThisAreaOnly)
            ->VirtualProperty("AffectsThisAreaOnly", "GetAffectsThisAreaOnly", "SetAffectsThisAreaOnly")
            ;
    }
}