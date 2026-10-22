/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MiniAudioPlaybackComponent.h"

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/EditContext.h>
#include <MiniAudio/SoundAssetRef.h>

namespace MiniAudio
{
    AZ::ComponentDescriptor* MiniAudioPlaybackComponent_CreateDescriptor()
    {
        return MiniAudioPlaybackComponent::CreateDescriptor();
    }

    AZ::TypeId MiniAudioPlaybackComponent_GetUUID()
    {
        return azrtti_typeid<MiniAudioPlaybackComponent>();
    }

    MiniAudioPlaybackComponent::MiniAudioPlaybackComponent(const MiniAudioPlaybackComponentConfig& config)
        : BaseClass(config)
    {
    }

    void MiniAudioPlaybackComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClass::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MiniAudioPlaybackComponent, BaseClass>()->Version(1);
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext
                ->ConstantProperty("MiniAudioPlaybackComponentTypeId", BehaviorConstant(AZ::Uuid(MiniAudioPlaybackComponentTypeId)))
                ->Attribute(AZ::Script::Attributes::Module, "MiniAudio")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common);

            AZ::BehaviorParameterOverrides SetVolumeParam = { "Volume", "Set Volume Percent" };
            AZ::BehaviorParameterOverrides SetVolumeDecibelsParam = { "Volume Decibels", "Set Volume Decibels" };
            AZ::BehaviorParameterOverrides SetInnerConeAngleInRadiansParam = { "Inner Cone Angle In Radians",
                                                                               "Set Inner Cone Angle In Radians" };
            AZ::BehaviorParameterOverrides SetInnerConeAngleInDegreesParam = { "Inner Cone Angle In Degrees",
                                                                               "Set Inner Cone Angle In Degrees" };
            AZ::BehaviorParameterOverrides SetOuterConeAngleInRadiansParam = { "Outer Cone Angle In Radians",
                                                                               "Set Outer Cone Angle In Radians" };
            AZ::BehaviorParameterOverrides SetOuterConeAngleInDegreesParam = { "Outer Cone Angle In Degrees",
                                                                               "Set Outer Cone Angle In Degrees" };
            AZ::BehaviorParameterOverrides SetOuterVolumeParam = { "Outer Volume", "Set Volume Percent Outside Outer Cone" };
            AZ::BehaviorParameterOverrides SetOuterVolumeDecibelsParam = { "Outer Volume Decibels",
                                                                           "Set Volume Decibels Outside Outer Cone" };
            AZ::BehaviorParameterOverrides SetFixedDirectionParam = { "Fixed Direction", "Set whether Direction is fixed" };
            AZ::BehaviorParameterOverrides SetDirectionalAttenuationFactorParam = { "Directional Attenuation Factor",
                                                                                    "Set Directional Attenuation Factor" };
            AZ::BehaviorParameterOverrides SetDirectionParam = { "Direction", "Set Playback Direction" };
            behaviorContext->EBus<MiniAudioPlaybackRequestBus>("MiniAudioPlaybackRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "audio")
                ->Attribute(AZ::Script::Attributes::Category, "MiniAudio Playback")
                ->Event("Play", &MiniAudioPlaybackRequests::Play)
                ->Event("Stop", &MiniAudioPlaybackRequests::Stop)
                ->Event("Pause", &MiniAudioPlaybackRequests::Pause)
                ->Event("SetLooping", &MiniAudioPlaybackRequests::SetLooping)
                ->Event("IsLooping", &MiniAudioPlaybackRequests::IsLooping)
                ->Event("GetSoundAsset", &MiniAudioPlaybackRequests::GetSoundAssetRef)
                ->Event("SetSoundAsset", &MiniAudioPlaybackRequests::SetSoundAssetRef)
                ->Event("GetVolumePercentage", &MiniAudioPlaybackRequests::GetVolumePercentage)
                ->Event("SetVolumePercentage", &MiniAudioPlaybackRequests::SetVolumePercentage, { SetVolumeParam })
                ->Event("GetVolumeDecibels", &MiniAudioPlaybackRequests::GetVolumeDecibels)
                ->Event("SetVolumeDecibels", &MiniAudioPlaybackRequests::SetVolumeDecibels, { SetVolumeDecibelsParam })
                ->Event("GetInnerConeAngleInRadians", &MiniAudioPlaybackRequests::GetInnerAngleInRadians)
                ->Event(
                    "SetInnerConeAngleInRadians", &MiniAudioPlaybackRequests::SetInnerAngleInRadians, { SetInnerConeAngleInRadiansParam })
                ->Event("GetInnerConeAngleInDegrees", &MiniAudioPlaybackRequests::GetInnerAngleInDegrees)
                ->Event(
                    "SetInnerConeAngleInDegrees", &MiniAudioPlaybackRequests::SetInnerAngleInDegrees, { SetInnerConeAngleInDegreesParam })
                ->Event("GetOuterConeAngleInRadians", &MiniAudioPlaybackRequests::GetOuterAngleInRadians)
                ->Event(
                    "SetOuterConeAngleInRadians", &MiniAudioPlaybackRequests::SetOuterAngleInRadians, { SetOuterConeAngleInRadiansParam })
                ->Event("GetOuterConeAngleInDegrees", &MiniAudioPlaybackRequests::GetOuterAngleInDegrees)
                ->Event(
                    "SetOuterConeAngleInDegrees", &MiniAudioPlaybackRequests::SetOuterAngleInDegrees, { SetOuterConeAngleInDegreesParam })
                ->Event("GetOuterVolumePercentage", &MiniAudioPlaybackRequests::GetOuterVolumePercentage)
                ->Event("SetOuterVolumePercentage", &MiniAudioPlaybackRequests::SetOuterVolumePercentage, { SetOuterVolumeParam })
                ->Event("GetOuterVolumeDecibels", &MiniAudioPlaybackRequests::GetOuterVolumeDecibels)
                ->Event("SetOuterVolumeDecibels", &MiniAudioPlaybackRequests::SetOuterVolumeDecibels, { SetOuterVolumeDecibelsParam })
                ->Event("GetFixedDirection", &MiniAudioPlaybackRequests::GetFixedDirecion)
                ->Event("SetFixedDirection", &MiniAudioPlaybackRequests::SetFixedDirecion, { SetFixedDirectionParam })
                ->Event("GetDirectionalAttenuationFactor", &MiniAudioPlaybackRequests::GetDirectionalAttenuationFactor)
                ->Event(
                    "SetDirectionalAttenuationFactor",
                    &MiniAudioPlaybackRequests::SetDirectionalAttenuationFactor,
                    { SetDirectionalAttenuationFactorParam })
                ->Event("GetDirection", &MiniAudioPlaybackRequests::GetDirection)
                ->Event("SetDirection", &MiniAudioPlaybackRequests::SetDirection, { SetDirectionParam });

            behaviorContext->Class<MiniAudioPlaybackComponentController>()->RequestBus("MiniAudioPlaybackRequestBus");
        }
    }
} // namespace MiniAudio
