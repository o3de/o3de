/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <IAudioSystemImplementation.h>
#include <AzTest/AzTest.h>


namespace Audio
{
    struct AudioSystemImplementationMock
        : public AudioSystemImplementation
    {
    public:
        MOCK_METHOD1(Update, void(float));

        MOCK_METHOD0(Initialize, EAudioRequestStatus());

        MOCK_METHOD0(ShutDown, EAudioRequestStatus());

        MOCK_METHOD0(Release, EAudioRequestStatus());

        MOCK_METHOD0(OnAudioSystemRefresh, void());

        MOCK_METHOD0(OnLoseFocus, EAudioRequestStatus());

        MOCK_METHOD0(OnGetFocus, EAudioRequestStatus());

        MOCK_METHOD0(MuteAll, EAudioRequestStatus());

        MOCK_METHOD0(UnmuteAll, EAudioRequestStatus());

        MOCK_METHOD0(StopAllSounds, EAudioRequestStatus());

        MOCK_METHOD2(RegisterAudioObject, EAudioRequestStatus(IATLAudioObjectData*, const char*));

        MOCK_METHOD1(RegisterAudioObject, EAudioRequestStatus(IATLAudioObjectData*));

        MOCK_METHOD1(UnregisterAudioObject, EAudioRequestStatus(IATLAudioObjectData*));

        MOCK_METHOD1(ResetAudioObject, EAudioRequestStatus(IATLAudioObjectData*));

        MOCK_METHOD1(UpdateAudioObject, EAudioRequestStatus(IATLAudioObjectData*));

        MOCK_METHOD2(PrepareTriggerSync, EAudioRequestStatus(IATLAudioObjectData*, const IATLTriggerImplData*));

        MOCK_METHOD2(UnprepareTriggerSync, EAudioRequestStatus(IATLAudioObjectData*, const IATLTriggerImplData*));

        MOCK_METHOD3(PrepareTriggerAsync, EAudioRequestStatus(IATLAudioObjectData*, const IATLTriggerImplData*, IATLEventData*));

        MOCK_METHOD3(UnprepareTriggerAsync, EAudioRequestStatus(IATLAudioObjectData*, const IATLTriggerImplData*, IATLEventData*));

        MOCK_METHOD4(ActivateTrigger, EAudioRequestStatus(IATLAudioObjectData*, const IATLTriggerImplData*, IATLEventData*, const SATLSourceData*));

        MOCK_METHOD2(StopEvent, EAudioRequestStatus(IATLAudioObjectData*, const IATLEventData*));

        MOCK_METHOD1(StopAllEvents, EAudioRequestStatus(IATLAudioObjectData*));

        MOCK_METHOD2(SetPosition, EAudioRequestStatus(IATLAudioObjectData*, const SATLWorldPosition&));

        MOCK_METHOD2(SetMultiplePositions, EAudioRequestStatus(IATLAudioObjectData*, const MultiPositionParams&));

        MOCK_METHOD3(SetRtpc, EAudioRequestStatus(IATLAudioObjectData*, const IATLRtpcImplData*, float));

        MOCK_METHOD2(SetSwitchState, EAudioRequestStatus(IATLAudioObjectData*, const IATLSwitchStateImplData*));

        MOCK_METHOD3(SetObstructionOcclusion, EAudioRequestStatus(IATLAudioObjectData*, float, float));

        MOCK_METHOD3(SetEnvironment, EAudioRequestStatus(IATLAudioObjectData*, const IATLEnvironmentImplData*, float));

        MOCK_METHOD2(SetListenerPosition, EAudioRequestStatus(IATLListenerData*, const SATLWorldPosition&));

        MOCK_METHOD1(RegisterInMemoryFile, EAudioRequestStatus(SATLAudioFileEntryInfo*));

        MOCK_METHOD1(UnregisterInMemoryFile, EAudioRequestStatus(SATLAudioFileEntryInfo*));

        MOCK_METHOD2(ParseAudioFileEntry, EAudioRequestStatus(const AZ::rapidxml::xml_node<char>*, SATLAudioFileEntryInfo*));

        MOCK_METHOD1(DeleteAudioFileEntryData, void(IATLAudioFileEntryData*));

        MOCK_METHOD1(GetAudioFileLocation, const char* const(SATLAudioFileEntryInfo*));

        MOCK_METHOD1(NewAudioTriggerImplData, IATLTriggerImplData*(const AZ::rapidxml::xml_node<char>*));

        MOCK_METHOD1(DeleteAudioTriggerImplData, void(IATLTriggerImplData*));

        MOCK_METHOD1(NewAudioRtpcImplData, IATLRtpcImplData*(const AZ::rapidxml::xml_node<char>*));

        MOCK_METHOD1(DeleteAudioRtpcImplData, void(IATLRtpcImplData*));

        MOCK_METHOD1(NewAudioSwitchStateImplData, IATLSwitchStateImplData*(const AZ::rapidxml::xml_node<char>*));

        MOCK_METHOD1(DeleteAudioSwitchStateImplData, void(IATLSwitchStateImplData*));

        MOCK_METHOD1(NewAudioEnvironmentImplData, IATLEnvironmentImplData*(const AZ::rapidxml::xml_node<char>*));

        MOCK_METHOD1(DeleteAudioEnvironmentImplData, void(IATLEnvironmentImplData*));

        MOCK_METHOD1(NewGlobalAudioObjectData, IATLAudioObjectData*(TAudioObjectID));

        MOCK_METHOD1(NewAudioObjectData, IATLAudioObjectData*(TAudioObjectID));

        MOCK_METHOD1(DeleteAudioObjectData, void(IATLAudioObjectData*));

        MOCK_METHOD1(NewDefaultAudioListenerObjectData, IATLListenerData*(TATLIDType));

        MOCK_METHOD1(NewAudioListenerObjectData, IATLListenerData*(TATLIDType));

        MOCK_METHOD1(DeleteAudioListenerObjectData, void(IATLListenerData*));

        MOCK_METHOD1(NewAudioEventData, IATLEventData*(TAudioEventID));

        MOCK_METHOD1(DeleteAudioEventData, void(IATLEventData*));

        MOCK_METHOD1(ResetAudioEventData, void(IATLEventData*));

        MOCK_METHOD1(SetLanguage, void(const char*));

        MOCK_CONST_METHOD0(GetImplSubPath, const char* const());

        MOCK_CONST_METHOD0(GetImplementationNameString, const char* const());

        MOCK_CONST_METHOD1(GetMemoryInfo, void(SAudioImplMemoryInfo&));

        MOCK_METHOD1(CreateAudioSource, bool(const SAudioInputConfig&));

        MOCK_METHOD1(DestroyAudioSource, void(TAudioSourceId));
    };

} // namespace Audio
