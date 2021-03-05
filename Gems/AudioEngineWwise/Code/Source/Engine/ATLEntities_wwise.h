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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#pragma once

#include <ATLEntityData.h>
#include <IAudioInterfacesCommonData.h>
#include <AudioAllocators.h>

#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/vector.h>

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/AkWwiseSDKVersion.h>

namespace Audio
{
    using TAKUniqueIDVector = AZStd::vector<AkUniqueID, Audio::AudioImplStdAllocator>;

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct SATLAudioObjectData_wwise
        : public IATLAudioObjectData
    {
        // convert to ATLMapLookupType
        using TEnvironmentImplMap = AZStd::map<AkAuxBusID, float, AZStd::less<AkAuxBusID>, Audio::AudioImplStdAllocator>;

        SATLAudioObjectData_wwise(const AkGameObjectID nPassedAKID, const bool bPassedHasPosition)
            : bNeedsToUpdateEnvironments(false)
            , bHasPosition(bPassedHasPosition)
            , nAKID(nPassedAKID)
        {}

        ~SATLAudioObjectData_wwise() override {}

        bool bNeedsToUpdateEnvironments;
        const bool bHasPosition;
        const AkGameObjectID nAKID;
        TEnvironmentImplMap cEnvironmentImplAmounts;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct SATLListenerData_wwise
        : public IATLListenerData
    {
        explicit SATLListenerData_wwise(const AkGameObjectID passedObjectId)
            : nAKListenerObjectId(passedObjectId)
        {}

        ~SATLListenerData_wwise() override {}

        const AkGameObjectID nAKListenerObjectId = AK_INVALID_GAME_OBJECT;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct SATLTriggerImplData_wwise
        : public IATLTriggerImplData
    {
        explicit SATLTriggerImplData_wwise(const AkUniqueID nPassedAKID)
            : nAKID(nPassedAKID)
        {}

        ~SATLTriggerImplData_wwise() override {}

        const AkUniqueID nAKID;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct SATLRtpcImplData_wwise
        : public IATLRtpcImplData
    {
        SATLRtpcImplData_wwise(const AkRtpcID nPassedAKID, const float m_fPassedMult, const float m_fPassedShift)
            : m_fMult(m_fPassedMult)
            , m_fShift(m_fPassedShift)
            , nAKID(nPassedAKID)
        {}

        ~SATLRtpcImplData_wwise() override {}

        const float m_fMult;
        const float m_fShift;
        const AkRtpcID nAKID;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    enum EWwiseSwitchType : TATLEnumFlagsType
    {
        eWST_NONE   = 0,
        eWST_SWITCH = 1,
        eWST_STATE  = 2,
        eWST_RTPC   = 3,
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct SATLSwitchStateImplData_wwise
        : public IATLSwitchStateImplData
    {
        SATLSwitchStateImplData_wwise(
            const EWwiseSwitchType ePassedType,
            const AkUInt32 nPassedAKSwitchID,
            const AkUInt32 nPassedAKStateID,
            const float fPassedRtpcValue = 0.0f)
            : eType(ePassedType)
            , nAKSwitchID(nPassedAKSwitchID)
            , nAKStateID(nPassedAKStateID)
            , fRtpcValue(fPassedRtpcValue)
        {}

        ~SATLSwitchStateImplData_wwise() override {}

        const EWwiseSwitchType eType;
        const AkUInt32 nAKSwitchID;
        const AkUInt32 nAKStateID;
        const float fRtpcValue;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    enum EWwiseAudioEnvironmentType : TATLEnumFlagsType
    {
        eWAET_NONE      = 0,
        eWAET_AUX_BUS   = 1,
        eWAET_RTPC      = 2,
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct SATLEnvironmentImplData_wwise
        : public IATLEnvironmentImplData
    {
        explicit SATLEnvironmentImplData_wwise(const EWwiseAudioEnvironmentType ePassedType)
            : eType(ePassedType)
        {}

        SATLEnvironmentImplData_wwise(const EWwiseAudioEnvironmentType ePassedType, const AkAuxBusID nPassedAKBusID)
            : eType(ePassedType)
            , nAKBusID(nPassedAKBusID)
        {
            AZ_Assert(ePassedType == eWAET_AUX_BUS, "SATLEnvironmentImplData_wwise - type is incorrect, expected an Aux Bus!");
        }

        SATLEnvironmentImplData_wwise(
            const EWwiseAudioEnvironmentType ePassedType,
            const AkRtpcID nPassedAKRtpcID,
            const float fPassedMult,
            const float fPassedShift)
            : eType(ePassedType)
            , nAKRtpcID(nPassedAKRtpcID)
            , fMult(fPassedMult)
            , fShift(fPassedShift)
        {
            AZ_Assert(ePassedType == eWAET_RTPC, "SATLEnvironmentImplData_wwise - type is incorrect, expected an RTPC!");
        }

        ~SATLEnvironmentImplData_wwise() override {}

        const EWwiseAudioEnvironmentType eType;

        union
        {
            // Aux Bus implementation
            struct
            {
                AkAuxBusID nAKBusID;
            };

            // Rtpc implementation
            struct
            {
                AkRtpcID nAKRtpcID;
                float fMult;
                float fShift;
            };
        };
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct SATLEventData_wwise
        : public IATLEventData
    {
        explicit SATLEventData_wwise(const TAudioEventID nPassedID)
            : audioEventState(eAES_NONE)
            , nAKID(AK_INVALID_UNIQUE_ID)
            , nATLID(nPassedID)
            , nSourceId(INVALID_AUDIO_SOURCE_ID)
        {}

        ~SATLEventData_wwise() override {}

        EAudioEventState audioEventState;
        AkUniqueID nAKID;
        const TAudioEventID nATLID;
        TAudioSourceId nSourceId;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct SATLAudioFileEntryData_wwise
        : public IATLAudioFileEntryData
    {
        SATLAudioFileEntryData_wwise()
            : nAKBankID(AK_INVALID_BANK_ID)
        {}

        ~SATLAudioFileEntryData_wwise() override {}

        AkBankID nAKBankID;
    };
} // namespace Audio
