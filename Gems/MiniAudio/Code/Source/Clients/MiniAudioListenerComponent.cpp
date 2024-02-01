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
            serializeContext->Class<MiniAudioListenerComponent, BaseClass>()
                ->Version(1);
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->ConstantProperty("MiniAudioListenerComponentTypeId",
                BehaviorConstant(AZ::Uuid(MiniAudioListenerComponentTypeId)))
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
            behaviorContext->EBus<MiniAudioListenerRequestBus>("MiniAudioListenerRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "audio")
                ->Attribute(AZ::Script::Attributes::Category, "MiniAudio Listener")
                ->Event("SetPosition", &MiniAudioListenerRequests::SetPosition)
                ->Event("SetFollowEntity", &MiniAudioListenerRequests::SetFollowEntity)
                ->Event("GetChannelCount", &MiniAudioListenerRequests::GetChannelCount)
                ->Event("GetInnerConeAngleInRadians", &MiniAudioListenerRequests::GetInnerAngleInRadians, {GetInnerConeAngleInRadiansParam})
                ->Event("SetInnerConeAngleInRadians", &MiniAudioListenerRequests::SetInnerAngleInRadians, {SetInnerConeAngleInRadiansParam})
                ->Event("GetInnerConeAngleInDegrees", &MiniAudioListenerRequests::GetInnerAngleInDegrees, {GetInnerConeAngleInDegreesParam})
                ->Event("SetInnerConeAngleInDegrees", &MiniAudioListenerRequests::SetInnerAngleInDegrees, {SetInnerConeAngleInDegreesParam})
                ->Event("GetOuterConeAngleInRadians", &MiniAudioListenerRequests::GetOuterAngleInRadians, {GetOuterConeAngleInRadiansParam})
                ->Event("SetOuterConeAngleInRadians", &MiniAudioListenerRequests::SetOuterAngleInRadians, {SetOuterConeAngleInRadiansParam})
                ->Event("GetOuterConeAngleInDegrees", &MiniAudioListenerRequests::GetOuterAngleInDegrees, {GetOuterConeAngleInDegreesParam})
                ->Event("SetOuterConeAngleInDegrees", &MiniAudioListenerRequests::SetOuterAngleInDegrees, {SetOuterConeAngleInDegreesParam})
                ->Event("GetOuterVolume", &MiniAudioListenerRequests::GetOuterVolume, {GetOuterVolumeParam})
                ->Event("SetOuterVolume", &MiniAudioListenerRequests::SetOuterVolume, {SetOuterVolumeParam})
                ;

            behaviorContext->Class<MiniAudioListenerComponent>()->RequestBus("MiniAudioListenerRequestBus");
        }
    }
} // namespace MiniAudio
