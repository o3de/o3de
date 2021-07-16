/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <IAudioSystem.h>

namespace Audio
{
    struct AudioProxyMock
        : public IAudioProxy
    {
        MOCK_METHOD2(Initialize, void(const char* const sObjectName, bool bInitAsync /* = true */));
        MOCK_METHOD0(Release, void());
        MOCK_METHOD0(Reset, void());
        MOCK_METHOD1(StopTrigger, void(TAudioControlID nTriggerID));
        MOCK_METHOD2(SetSwitchState, void(TAudioControlID nSwitchID, TAudioSwitchStateID nStateID));
        MOCK_METHOD2(SetRtpcValue, void(TAudioControlID nRtpcID, float fValue));
        MOCK_METHOD1(SetObstructionCalcType, void(EAudioObjectObstructionCalcType eObstructionType));
        MOCK_METHOD1(SetPosition, void(const SATLWorldPosition& rPosition));
        MOCK_METHOD1(SetPosition, void(const AZ::Vector3& rPosition));
        MOCK_METHOD2(SetEnvironmentAmount, void(TAudioEnvironmentID nEnvironmentID, float fAmount));
        MOCK_METHOD0(SetCurrentEnvironments, void());
        MOCK_CONST_METHOD0(GetAudioObjectID, TAudioObjectID());
    };

    struct AudioSystemMock
        : public IAudioSystem
    {
        MOCK_METHOD0(Initialize, bool());
        MOCK_METHOD0(Release, void());
        MOCK_METHOD1(PushRequest, void(const SAudioRequest& rAudioRequestData));
        MOCK_METHOD4(AddRequestListener, void(AudioRequestCallbackType func, void* pObjectToListenTo, EAudioRequestType requestType /* = eART_AUDIO_ALL_REQUESTS */, TATLEnumFlagsType specificRequestMask /* = ALL_AUDIO_REQUEST_SPECIFIC_TYPE_FLAGS */));
        MOCK_METHOD2(RemoveRequestListener, void(AudioRequestCallbackType func, void* pObjectToListenTo));
        MOCK_METHOD0(ExternalUpdate, void());
        MOCK_CONST_METHOD1(GetAudioTriggerID, TAudioControlID(const char* sAudioTriggerName));
        MOCK_CONST_METHOD1(GetAudioRtpcID, TAudioControlID(const char* sAudioRtpcName));
        MOCK_CONST_METHOD1(GetAudioSwitchID, TAudioControlID(const char* sAudioSwitchName));
        MOCK_CONST_METHOD2(GetAudioSwitchStateID, TAudioSwitchStateID(const TAudioControlID nSwitchID, const char* sAudioStateName));
        MOCK_CONST_METHOD1(GetAudioPreloadRequestID, TAudioPreloadRequestID(const char* sAudioPreloadRequestName));
        MOCK_CONST_METHOD1(GetAudioEnvironmentID, TAudioEnvironmentID(const char* sAudioEnvironmentName));
        MOCK_METHOD1(ReserveAudioListenerID, bool(TAudioObjectID& rAudioListenerID));
        MOCK_METHOD1(ReleaseAudioListenerID, bool(TAudioObjectID nAudioObjectID));
        MOCK_METHOD1(OnCVarChanged, void(ICVar* const pCVar));
        MOCK_METHOD1(GetInfo, void(SAudioSystemInfo& rAudioSystemInfo));
        MOCK_CONST_METHOD0(GetControlsPath, const char*());
        MOCK_METHOD0(UpdateControlsPath, void());
        MOCK_METHOD0(GetFreeAudioProxy, IAudioProxy*());
        MOCK_METHOD1(FreeAudioProxy, void(IAudioProxy* pIAudioProxy));
        MOCK_CONST_METHOD2(GetAudioControlName, const char*(EAudioControlType eAudioEntityType, TATLIDType nAudioEntityID));
        MOCK_CONST_METHOD2(GetAudioSwitchStateName, const char*(TAudioControlID switchID, TAudioSwitchStateID stateID));
    };

} // namespace Audio
