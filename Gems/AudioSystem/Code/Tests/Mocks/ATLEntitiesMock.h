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
#if !defined(AUDIO_RELEASE)
    class ATLDebugNameStoreMock
        : public CATLDebugNameStore
    {
    public:
        MOCK_METHOD2(AddAudioObject, bool(const TAudioObjectID, const char* const));
        MOCK_METHOD2(AddAudioTrigger, bool(const TAudioControlID, const char* const));
        MOCK_METHOD2(AddAudioRtpc, bool(const TAudioControlID, const char* const));
        MOCK_METHOD2(AddAudioSwitch, bool(const TAudioControlID, const char* const));
        MOCK_METHOD3(AddAudioSwitchState, bool(const TAudioControlID, const TAudioSwitchStateID, const char* const));
        MOCK_METHOD2(AddAudioPreloadRequest, bool(const TAudioPreloadRequestID, const char* const));
        MOCK_METHOD2(AddAudioEnvironment, bool(const TAudioEnvironmentID, const char* const));

        MOCK_METHOD1(RemoveAudioObject, bool(const TAudioObjectID));
        MOCK_METHOD1(RemoveAudioTrigger, bool(const TAudioControlID));
        MOCK_METHOD1(RemoveAudioRtpc, bool(const TAudioControlID));
        MOCK_METHOD1(RemoveAudioSwitch, bool(const TAudioControlID));
        MOCK_METHOD2(RemoveAudioSwitchState, bool(const TAudioControlID, const TAudioSwitchStateID));
        MOCK_METHOD1(RemoveAudioPreloadRequest, bool(const TAudioPreloadRequestID));
        MOCK_METHOD1(RemoveAudioEnvironment, bool(const TAudioEnvironmentID));

        MOCK_CONST_METHOD1(LookupAudioObjectName, const char*(const TAudioObjectID));
        MOCK_CONST_METHOD1(LookupAudioTriggerName, const char*(const TAudioControlID));
        MOCK_CONST_METHOD1(LookupAudioRtpcName, const char*(const TAudioControlID));
        MOCK_CONST_METHOD1(LookupAudioSwitchName, const char*(const TAudioControlID));
        MOCK_CONST_METHOD2(LookupAudioSwitchStateName, const char*(const TAudioControlID, const TAudioSwitchStateID));
        MOCK_CONST_METHOD1(LookupAudioPreloadRequestName, const char*(const TAudioPreloadRequestID));
        MOCK_CONST_METHOD1(LookupAudioEnvironmentName, const char*(const TAudioEnvironmentID));
    };
#endif // !AUDIO_RELEASE

} // namespace Audio
