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

#include <IAudioSystem.h>

namespace Audio
{
    struct AudioProxyMock
        : public IAudioProxy
    {
        MOCK_METHOD2(Initialize, void(const char* const sObjectName, const bool bInitAsync /* = true */));
        MOCK_METHOD0(Release, void());
        MOCK_METHOD0(Reset, void());
        MOCK_METHOD1(StopTrigger, void(const TAudioControlID nTriggerID));
        MOCK_METHOD2(SetSwitchState, void(const TAudioControlID nSwitchID, const TAudioSwitchStateID nStateID));
        MOCK_METHOD2(SetRtpcValue, void(const TAudioControlID nRtpcID, const float fValue));
        MOCK_METHOD1(SetObstructionCalcType, void(const EAudioObjectObstructionCalcType eObstructionType));
        MOCK_METHOD1(SetPosition, void(const SATLWorldPosition& rPosition));
        MOCK_METHOD1(SetPosition, void(const AZ::Vector3& rPosition));
        MOCK_METHOD2(SetEnvironmentAmount, void(const TAudioEnvironmentID nEnvironmentID, const float fAmount));
        MOCK_METHOD0(SetCurrentEnvironments, void());
        MOCK_CONST_METHOD0(GetAudioObjectID, TAudioObjectID());
    };

    struct AudioSystemMock
        : public IAudioSystem
    {
        MOCK_METHOD0(Initialize, bool());
        MOCK_METHOD0(Release, void());
        MOCK_METHOD1(PushRequest, void(const SAudioRequest& rAudioRequestData));
        MOCK_METHOD4(AddRequestListener, void(AudioRequestCallbackType func, void* const pObjectToListenTo, const EAudioRequestType requestType /* = eART_AUDIO_ALL_REQUESTS */, const TATLEnumFlagsType specificRequestMask /* = ALL_AUDIO_REQUEST_SPECIFIC_TYPE_FLAGS */));
        MOCK_METHOD2(RemoveRequestListener, void(AudioRequestCallbackType func, void* const pObjectToListenTo));
        MOCK_METHOD0(ExternalUpdate, void());
        MOCK_CONST_METHOD1(GetAudioTriggerID, TAudioControlID(const char* const sAudioTriggerName));
        MOCK_CONST_METHOD1(GetAudioRtpcID, TAudioControlID(const char* const sAudioRtpcName));
        MOCK_CONST_METHOD1(GetAudioSwitchID, TAudioControlID(const char* const sAudioSwitchName));
        MOCK_CONST_METHOD2(GetAudioSwitchStateID, TAudioSwitchStateID(const TAudioControlID nSwitchID, const char* const sAudioStateName));
        MOCK_CONST_METHOD1(GetAudioPreloadRequestID, TAudioPreloadRequestID(const char* const sAudioPreloadRequestName));
        MOCK_CONST_METHOD1(GetAudioEnvironmentID, TAudioEnvironmentID(const char* const sAudioEnvironmentName));
        MOCK_METHOD1(ReserveAudioListenerID, bool(TAudioObjectID& rAudioListenerID));
        MOCK_METHOD1(ReleaseAudioListenerID, bool(const TAudioObjectID nAudioObjectID));
        MOCK_METHOD1(OnCVarChanged, void(ICVar* const pCVar));
        MOCK_METHOD1(GetInfo, void(SAudioSystemInfo& rAudioSystemInfo));
        MOCK_CONST_METHOD0(GetControlsPath, const char*());
        MOCK_METHOD0(UpdateControlsPath, void());
        MOCK_METHOD0(GetFreeAudioProxy, IAudioProxy*());
        MOCK_METHOD1(FreeAudioProxy, void(IAudioProxy* const pIAudioProxy));
        MOCK_CONST_METHOD2(GetAudioControlName, const char*(const EAudioControlType eAudioEntityType, const TATLIDType nAudioEntityID));
        MOCK_CONST_METHOD2(GetAudioSwitchStateName, const char*(const TAudioControlID switchID, const TAudioSwitchStateID stateID));
    };

} // namespace Audio
