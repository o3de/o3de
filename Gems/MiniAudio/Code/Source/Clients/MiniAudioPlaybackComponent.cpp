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
            serializeContext->Class<MiniAudioPlaybackComponent, BaseClass>()
                ->Version(1);
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->ConstantProperty("MiniAudioPlaybackComponentTypeId",
                BehaviorConstant(AZ::Uuid(MiniAudioPlaybackComponentTypeId)))
                ->Attribute(AZ::Script::Attributes::Module, "MiniAudio")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common);

            AZ::BehaviorParameterOverrides GetInnerConeAngleInRadiansParam = {"Get Inner Cone Angle In Radians", "Get Inner Cone Angle In Radians"};
            AZ::BehaviorParameterOverrides SetInnerConeAngleInRadiansParam = {"Set Inner Cone Angle In Radians", "Set Inner Cone Angle In Radians"};
            AZ::BehaviorParameterOverrides GetInnerConeAngleInDegreesParam = {"Get Inner Cone Angle In Degrees", "Get Inner Cone Angle In Degrees"};
            AZ::BehaviorParameterOverrides SetInnerConeAngleInDegreesParam = {"Set Inner Cone Angle In Degrees", "Set Inner Cone Angle In Degrees"};
            AZ::BehaviorParameterOverrides GetOuterConeAngleInRadiansParam = {"Get Outer Cone Angle In Radians", "Get Outer Cone Angle In Radians"};
            AZ::BehaviorParameterOverrides SetOuterConeAngleInRadiansParam = {"Set Outer Cone Angle In Radians", "Set Outer Cone Angle In Radians"};
            AZ::BehaviorParameterOverrides GetOuterConeAngleInDegreesParam = {"Get Outer Cone Angle In Degrees", "Get Outer Cone Angle In Degrees"};
            AZ::BehaviorParameterOverrides SetOuterConeAngleInDegreesParam = {"Set Outer Cone Angle In Degrees", "Set Outer Cone Angle In Degrees"};
            AZ::BehaviorParameterOverrides GetOuterVolumeParam = {"Get Percent Volume Outside Outer Cone", "Get Percent Volume Outside Outer Cone"};
            AZ::BehaviorParameterOverrides SetOuterVolumeParam = {"Set Percent Volume Outside Outer Cone", "Set Percent Volume Outside Outer Cone"};
            AZ::BehaviorParameterOverrides GetFixedDirectionParam = {"Get Fixed Direction", "Get Whether Direction Is Fixed"};
            AZ::BehaviorParameterOverrides SetFixedDirectionParam = {"Set Fixed Direction", "Set Whether Direction Is Fixed"};
            AZ::BehaviorParameterOverrides GetDirectionalAttenuationFactorParam = {"Get Directional Attenuation Factor", "Get Directional Attenuation Factor"};
            AZ::BehaviorParameterOverrides SetDirectionalAttenuationFactorParam = {"Set Directional Attenuation Factor", "Set Directional Attenuation Factor"};
            AZ::BehaviorParameterOverrides GetDirectionParam = {"Get Direction", "Get Playback Direction"};
            AZ::BehaviorParameterOverrides SetDirectionParam = {"Set Direction", "Set Playback Direction"};
            behaviorContext->EBus<MiniAudioPlaybackRequestBus>("MiniAudioPlaybackRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "audio")
                ->Attribute(AZ::Script::Attributes::Category, "MiniAudio Playback")
                ->Event("Play", &MiniAudioPlaybackRequests::Play)
                ->Event("Stop", &MiniAudioPlaybackRequests::Stop)
                ->Event("SetVolume", &MiniAudioPlaybackRequests::SetVolume)
                ->Event("SetLooping", &MiniAudioPlaybackRequests::SetLooping)
                ->Event("IsLooping", &MiniAudioPlaybackRequests::IsLooping)
                ->Event("SetSoundAsset", &MiniAudioPlaybackRequests::SetSoundAssetRef)
                ->Event("GetSoundAsset", &MiniAudioPlaybackRequests::GetSoundAssetRef)
                ->Event("GetInnerConeAngleInRadians", &MiniAudioPlaybackRequests::GetInnerAngleInRadians, {GetInnerConeAngleInRadiansParam})
                ->Event("SetInnerConeAngleInRadians", &MiniAudioPlaybackRequests::SetInnerAngleInRadians, {SetInnerConeAngleInRadiansParam})
                ->Event("GetInnerConeAngleInDegrees", &MiniAudioPlaybackRequests::GetInnerAngleInDegrees, {GetInnerConeAngleInDegreesParam})
                ->Event("SetInnerConeAngleInDegrees", &MiniAudioPlaybackRequests::SetInnerAngleInDegrees, {SetInnerConeAngleInDegreesParam})
                ->Event("GetOuterConeAngleInRadians", &MiniAudioPlaybackRequests::GetOuterAngleInRadians, {GetOuterConeAngleInRadiansParam})
                ->Event("SetOuterConeAngleInRadians", &MiniAudioPlaybackRequests::SetOuterAngleInRadians, {SetOuterConeAngleInRadiansParam})
                ->Event("GetOuterConeAngleInDegrees", &MiniAudioPlaybackRequests::GetOuterAngleInDegrees, {GetOuterConeAngleInDegreesParam})
                ->Event("SetOuterConeAngleInDegrees", &MiniAudioPlaybackRequests::SetOuterAngleInDegrees, {SetOuterConeAngleInDegreesParam})
                ->Event("GetOuterVolume", &MiniAudioPlaybackRequests::GetOuterVolume, {GetOuterVolumeParam})
                ->Event("SetOuterVolume", &MiniAudioPlaybackRequests::SetOuterVolume, {SetOuterVolumeParam})
                ->Event("GetFixedDirection", &MiniAudioPlaybackRequests::GetFixedDirecion, {GetFixedDirectionParam})
                ->Event("SetFixedDirection", &MiniAudioPlaybackRequests::SetFixedDirecion, {SetFixedDirectionParam})
                ->Event("GetDirectionalAttenuationFactor", &MiniAudioPlaybackRequests::GetDirectionalAttenuationFactor, {GetDirectionalAttenuationFactorParam})
                ->Event("SetDirectionalAttenuationFactor", &MiniAudioPlaybackRequests::SetDirectionalAttenuationFactor, {SetDirectionalAttenuationFactorParam})
                ->Event("GetDirection", &MiniAudioPlaybackRequests::GetDirection, {GetDirectionParam})
                ->Event("SetDirection", &MiniAudioPlaybackRequests::SetDirection, {SetDirectionParam})
            ;

            behaviorContext->Class<MiniAudioPlaybackComponentController>()->RequestBus("MiniAudioPlaybackRequestBus");
        }
    }
} // namespace MiniAudio
