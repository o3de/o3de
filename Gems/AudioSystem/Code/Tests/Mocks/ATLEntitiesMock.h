/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ATLEntities.h>
#include <AzTest/AzTest.h>

namespace Audio
{
    class ATLDebugNameStoreMock
#if !defined(AUDIO_RELEASE)
        : public CATLDebugNameStore
#endif
    {
    public:
#if !defined(AUDIO_RELEASE)
        MOCK_METHOD1(SyncChanges, void(const CATLDebugNameStore&));
#endif
        MOCK_METHOD2(AddAudioObject, void(const TAudioObjectID, const char* const));
        MOCK_METHOD2(AddAudioTrigger, void(const TAudioControlID, const char* const));
        MOCK_METHOD2(AddAudioRtpc, void(const TAudioControlID, const char* const));
        MOCK_METHOD2(AddAudioSwitch, void(const TAudioControlID, const char* const));
        MOCK_METHOD3(AddAudioSwitchState, void(const TAudioControlID, const TAudioSwitchStateID, const char* const));
        MOCK_METHOD2(AddAudioPreloadRequest, void(const TAudioPreloadRequestID, const char* const));
        MOCK_METHOD2(AddAudioEnvironment, void(const TAudioEnvironmentID, const char* const));

        MOCK_METHOD1(RemoveAudioObject, void(const TAudioObjectID));
        MOCK_METHOD1(RemoveAudioTrigger, void(const TAudioControlID));
        MOCK_METHOD1(RemoveAudioRtpc, void(const TAudioControlID));
        MOCK_METHOD1(RemoveAudioSwitch, void(const TAudioControlID));
        MOCK_METHOD2(RemoveAudioSwitchState, void(const TAudioControlID, const TAudioSwitchStateID));
        MOCK_METHOD1(RemoveAudioPreloadRequest, void(const TAudioPreloadRequestID));
        MOCK_METHOD1(RemoveAudioEnvironment, void(const TAudioEnvironmentID));

        MOCK_CONST_METHOD0(AudioObjectsChanged, bool());
        MOCK_CONST_METHOD0(AudioTriggersChanged, bool());
        MOCK_CONST_METHOD0(AudioRtpcsChanged, bool());
        MOCK_CONST_METHOD0(AudioSwitchesChanged, bool());
        MOCK_CONST_METHOD0(AudioPreloadsChanged, bool());
        MOCK_CONST_METHOD0(AudioEnvironmentsChanged, bool());

        MOCK_CONST_METHOD1(LookupAudioObjectName, const char*(const TAudioObjectID));
        MOCK_CONST_METHOD1(LookupAudioTriggerName, const char*(const TAudioControlID));
        MOCK_CONST_METHOD1(LookupAudioRtpcName, const char*(const TAudioControlID));
        MOCK_CONST_METHOD1(LookupAudioSwitchName, const char*(const TAudioControlID));
        MOCK_CONST_METHOD2(LookupAudioSwitchStateName, const char*(const TAudioControlID, const TAudioSwitchStateID));
        MOCK_CONST_METHOD1(LookupAudioPreloadRequestName, const char*(const TAudioPreloadRequestID));
        MOCK_CONST_METHOD1(LookupAudioEnvironmentName, const char*(const TAudioEnvironmentID));
    };

} // namespace Audio
