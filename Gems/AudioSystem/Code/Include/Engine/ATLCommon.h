/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <IAudioSystem.h>
#include <AzCore/AzCore_Traits_Platform.h>


namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////
    namespace ATLXmlTags
    {
        static constexpr const char* PlatformName = AZ_TRAIT_OS_PLATFORM_NAME;
        static constexpr const char* PlatformCodeName = AZ_TRAIT_OS_PLATFORM_CODENAME;

        static constexpr const char* RootNodeTag = "ATLConfig";
        static constexpr const char* TriggersNodeTag = "AudioTriggers";
        static constexpr const char* RtpcsNodeTag = "AudioRtpcs";
        static constexpr const char* SwitchesNodeTag = "AudioSwitches";
        static constexpr const char* PreloadsNodeTag = "AudioPreloads";
        static constexpr const char* EnvironmentsNodeTag = "AudioEnvironments";

        static constexpr const char* ATLTriggerTag = "ATLTrigger";
        static constexpr const char* ATLSwitchTag = "ATLSwitch";
        static constexpr const char* ATLRtpcTag = "ATLRtpc";
        static constexpr const char* ATLSwitchStateTag = "ATLSwitchState";
        static constexpr const char* ATLEnvironmentTag = "ATLEnvironment";
        static constexpr const char* ATLPlatformsTag = "ATLPlatforms";
        static constexpr const char* ATLConfigGroupTag = "ATLConfigGroup";
        static constexpr const char* PlatformNodeTag = "Platform";

        static constexpr const char* ATLTriggerRequestTag = "ATLTriggerRequest";
        static constexpr const char* ATLSwitchRequestTag = "ATLSwitchRequest";
        static constexpr const char* ATLRtpcRequestTag = "ATLRtpcRequest";
        static constexpr const char* ATLPreloadRequestTag = "ATLPreloadRequest";
        static constexpr const char* ATLEnvironmentRequestTag = "ATLEnvironmentRequest";
        static constexpr const char* ATLValueTag = "ATLValue";

        static constexpr const char* ATLNameAttribute = "atl_name";
        static constexpr const char* ATLInternalNameAttribute = "atl_internal_name";
        static constexpr const char* ATLTypeAttribute = "atl_type";
        static constexpr const char* ATLConfigGroupAttribute = "atl_config_group_name";

        static constexpr const char* ATLDataLoadType = "AutoLoad";

    } // namespace ATLXmlTags

    ///////////////////////////////////////////////////////////////////////////////////////////////
    namespace ATLInternalControlNames
    {
        static constexpr const char* LoseFocusName = "lose_focus";
        static constexpr const char* GetFocusName = "get_focus";
        static constexpr const char* MuteAllName = "mute_all";
        static constexpr const char* UnmuteAllName = "unmute_all";
        static constexpr const char* DoNothingName = "do_nothing";
        static constexpr const char* ObjectSpeedName = "object_speed";
        static constexpr const char* ObstructionOcclusionCalcName = "ObstructionOcclusionCalculationType";
        static constexpr const char* OOCIgnoreStateName = "Ignore";
        static constexpr const char* OOCSingleRayStateName = "SingleRay";
        static constexpr const char* OOCMultiRayStateName = "MultiRay";
        static constexpr const char* ObjectVelocityTrackingName = "object_velocity_tracking";
        static constexpr const char* OVTOnStateName = "on";
        static constexpr const char* OVTOffStateName = "off";
        static constexpr const char* GlobalPreloadRequestName = "global_atl_preloads";

    } // namespace ATLInternalControlNames

    ///////////////////////////////////////////////////////////////////////////////////////////////
    namespace ATLInternalControlIDs
    {
        static constexpr TAudioControlID LoseFocusTriggerID = static_cast<TAudioControlID>(AZ_CRC_CE(ATLInternalControlNames::LoseFocusName));
        static constexpr TAudioControlID GetFocusTriggerID = static_cast<TAudioControlID>(AZ_CRC_CE(ATLInternalControlNames::GetFocusName));
        static constexpr TAudioControlID MuteAllTriggerID = static_cast<TAudioControlID>(AZ_CRC_CE(ATLInternalControlNames::MuteAllName));
        static constexpr TAudioControlID UnmuteAllTriggerID = static_cast<TAudioControlID>(AZ_CRC_CE(ATLInternalControlNames::UnmuteAllName));
        static constexpr TAudioControlID DoNothingTriggerID = static_cast<TAudioControlID>(AZ_CRC_CE(ATLInternalControlNames::DoNothingName));
        static constexpr TAudioControlID ObjectSpeedRtpcID = static_cast<TAudioControlID>(AZ_CRC_CE(ATLInternalControlNames::ObjectSpeedName));

        static constexpr TAudioControlID ObstructionOcclusionCalcSwitchID = static_cast<TAudioControlID>(AZ_CRC_CE(ATLInternalControlNames::ObstructionOcclusionCalcName));
        static constexpr TAudioSwitchStateID OOCStateIDs[] =
        {
            static_cast<TAudioSwitchStateID>(AZ_CRC_CE(ATLInternalControlNames::OOCIgnoreStateName)),
            static_cast<TAudioSwitchStateID>(AZ_CRC_CE(ATLInternalControlNames::OOCSingleRayStateName)),
            static_cast<TAudioSwitchStateID>(AZ_CRC_CE(ATLInternalControlNames::OOCMultiRayStateName))
        };

        static constexpr TAudioControlID ObjectVelocityTrackingSwitchID = static_cast<TAudioControlID>(AZ_CRC_CE(ATLInternalControlNames::ObjectVelocityTrackingName));
        static constexpr TAudioSwitchStateID OVTOnStateID = static_cast<TAudioSwitchStateID>(AZ_CRC_CE(ATLInternalControlNames::OVTOnStateName));
        static constexpr TAudioSwitchStateID OVTOffStateID = static_cast<TAudioSwitchStateID>(AZ_CRC_CE(ATLInternalControlNames::OVTOffStateName));

        static constexpr TAudioPreloadRequestID GlobalPreloadRequestID = static_cast<TAudioPreloadRequestID>(AZ_CRC_CE(ATLInternalControlNames::GlobalPreloadRequestName));

    } // namespace ATLInternalControlIDs

} // namespace Audio
