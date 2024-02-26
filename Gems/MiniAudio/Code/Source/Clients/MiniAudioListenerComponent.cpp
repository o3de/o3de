/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MiniAudioListenerComponent.h"

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/EditContext.h>

namespace MiniAudio
{
    AZ::ComponentDescriptor* MiniAudioListenerComponent_CreateDescriptor()
    {
        return MiniAudioListenerComponent::CreateDescriptor();
    }

    MiniAudioListenerComponent::MiniAudioListenerComponent(const MiniAudioListenerComponentConfig& config)
        : BaseClass(config)
    {
    }

    void MiniAudioListenerComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClass::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MiniAudioListenerComponent, BaseClass>()->Version(1);
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext
                ->ConstantProperty("MiniAudioListenerComponentTypeId", BehaviorConstant(AZ::Uuid(MiniAudioListenerComponentTypeId)))
                ->Attribute(AZ::Script::Attributes::Module, "MiniAudio")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common);

            AZ::BehaviorParameterOverrides GetChannelCountParam = { "Channel Count", "Get Channel Count" };
            AZ::BehaviorParameterOverrides GetGlobalVolumeParam = { "Global Volume", "Get Global Volume Percent" };
            AZ::BehaviorParameterOverrides SetGlobalVolumeParam = { "Global Volume", "Set Global Volume Percent" };
            AZ::BehaviorParameterOverrides GetGlobalVolumeDecibelsParam = { "Global Volume Decibels", "Get Global Volume Decibels" };
            AZ::BehaviorParameterOverrides SetGlobalVolumeDecibelsParam = { "Global Volume Decibels", "Set Global Volume Decibels" };
            AZ::BehaviorParameterOverrides GetInnerConeAngleInRadiansParam = { "Inner Cone Angle In Radians",
                                                                               "Get Inner Cone Angle In Radians" };
            AZ::BehaviorParameterOverrides SetInnerConeAngleInRadiansParam = { "Inner Cone Angle In Radians",
                                                                               "Set Inner Cone Angle In Radians" };
            AZ::BehaviorParameterOverrides GetInnerConeAngleInDegreesParam = { "Inner Cone Angle In Degrees",
                                                                               "Get Inner Cone Angle In Degrees" };
            AZ::BehaviorParameterOverrides SetInnerConeAngleInDegreesParam = { "Inner Cone Angle In Degrees",
                                                                               "Set Inner Cone Angle In Degrees" };
            AZ::BehaviorParameterOverrides GetOuterConeAngleInRadiansParam = { "Outer Cone Angle In Radians",
                                                                               "Get Outer Cone Angle In Radians" };
            AZ::BehaviorParameterOverrides SetOuterConeAngleInRadiansParam = { "Outer Cone Angle In Radians",
                                                                               "Set Outer Cone Angle In Radians" };
            AZ::BehaviorParameterOverrides GetOuterConeAngleInDegreesParam = { "Outer Cone Angle In Degrees",
                                                                               "Get Outer Cone Angle In Degrees" };
            AZ::BehaviorParameterOverrides SetOuterConeAngleInDegreesParam = { "Outer Cone Angle In Degrees",
                                                                               "Set Outer Cone Angle In Degrees" };
            AZ::BehaviorParameterOverrides GetOuterVolumeParam = { "Outer Volume", "Get Volume Percent Outside Outer Cone" };
            AZ::BehaviorParameterOverrides SetOuterVolumeParam = { "Outer Volume", "Set Volume Percent Outside Outer Cone" };
            AZ::BehaviorParameterOverrides GetOuterVolumeDecibelsParam = { "Outer Volume Decibels",
                                                                           "Get Volume Decibels Outside Outer Cone" };
            AZ::BehaviorParameterOverrides SetOuterVolumeDecibelsParam = { "Outer Volume Decibels",
                                                                           "Set Volume Decibels Outside Outer Cone" };
            behaviorContext->EBus<MiniAudioListenerRequestBus>("MiniAudioListenerRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "audio")
                ->Attribute(AZ::Script::Attributes::Category, "MiniAudio Listener")
                ->Event("SetPosition", &MiniAudioListenerRequests::SetPosition)
                ->Event("SetFollowEntity", &MiniAudioListenerRequests::SetFollowEntity)
                ->Event("GetChannelCount", &MiniAudioListenerRequests::GetChannelCount, { GetChannelCountParam })
                ->Event("GetGlobalVolumePercentage", &MiniAudioListenerRequests::GetGlobalVolumePercentage, { GetGlobalVolumeParam })
                ->Event("SetGlobalVolumePercentage", &MiniAudioListenerRequests::SetGlobalVolumePercentage, { SetGlobalVolumeParam })
                ->Event("GetGlobalVolumeDecibels", &MiniAudioListenerRequests::GetGlobalVolumeDecibels, { GetGlobalVolumeDecibelsParam })
                ->Event("SetGlobalVolumeDecibels", &MiniAudioListenerRequests::SetGlobalVolumeDecibels, { SetGlobalVolumeDecibelsParam })
                ->Event(
                    "GetInnerConeAngleInRadians", &MiniAudioListenerRequests::GetInnerAngleInRadians, { GetInnerConeAngleInRadiansParam })
                ->Event(
                    "SetInnerConeAngleInRadians", &MiniAudioListenerRequests::SetInnerAngleInRadians, { SetInnerConeAngleInRadiansParam })
                ->Event(
                    "GetInnerConeAngleInDegrees", &MiniAudioListenerRequests::GetInnerAngleInDegrees, { GetInnerConeAngleInDegreesParam })
                ->Event(
                    "SetInnerConeAngleInDegrees", &MiniAudioListenerRequests::SetInnerAngleInDegrees, { SetInnerConeAngleInDegreesParam })
                ->Event(
                    "GetOuterConeAngleInRadians", &MiniAudioListenerRequests::GetOuterAngleInRadians, { GetOuterConeAngleInRadiansParam })
                ->Event(
                    "SetOuterConeAngleInRadians", &MiniAudioListenerRequests::SetOuterAngleInRadians, { SetOuterConeAngleInRadiansParam })
                ->Event(
                    "GetOuterConeAngleInDegrees", &MiniAudioListenerRequests::GetOuterAngleInDegrees, { GetOuterConeAngleInDegreesParam })
                ->Event(
                    "SetOuterConeAngleInDegrees", &MiniAudioListenerRequests::SetOuterAngleInDegrees, { SetOuterConeAngleInDegreesParam })
                ->Event("GetOuterVolumePercentage", &MiniAudioListenerRequests::GetOuterVolumePercentage, { GetOuterVolumeParam })
                ->Event("SetOuterVolumePercentage", &MiniAudioListenerRequests::SetOuterVolumePercentage, { SetOuterVolumeParam })
                ->Event("GetOuterVolumeDecibels", &MiniAudioListenerRequests::GetOuterVolumeDecibels, { GetOuterVolumeDecibelsParam })
                ->Event("SetOuterVolumeDecibels", &MiniAudioListenerRequests::SetOuterVolumeDecibels, { SetOuterVolumeDecibelsParam });

            behaviorContext->Class<MiniAudioListenerComponent>()->RequestBus("MiniAudioListenerRequestBus");
        }
    }
} // namespace MiniAudio
